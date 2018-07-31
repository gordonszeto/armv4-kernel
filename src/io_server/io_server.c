#include <clock_server.h>
#include <io_server.h>
#include <bwio.h>
#include <sys_call.h>
#include <ringbuffer.h>
#include <ts7200.h>
#include <name_server.h>
#include <stddef.h>
#include <string.h>

void static io_server_notifier_rx_main(void) {
    uint8_t io_server_tid = 0;
    uint8_t sender_tid;
    io_server_msg_t msg, rep;
    msg.type = IO_SERVER_MSG_TYPE_NOTIF_RX;
    msg.len = 1;
    uint8_t com_num;
    volatile uint32_t *uart_base = 0;
    event_t rx_event = SYS_CALL_EVENT_UART2_RX;
    char *io_server_name = NULL;

    // get com number from parent
    int res = Receive(&sender_tid, &com_num, sizeof(com_num));
    if (res < 0) {
        panic("rx notifier failed to get COM number");
    }
    Reply(sender_tid, NULL, 0);
    switch(com_num) {
        case COM1:
            uart_base = (volatile uint32_t*) UART1_BASE;
            io_server_name = IO_SERVER_NAME1;
            rx_event = SYS_CALL_EVENT_UART1_RX;
            break;
        case COM2:
            uart_base = (volatile uint32_t*) UART2_BASE;
            io_server_name = IO_SERVER_NAME2;
            rx_event = SYS_CALL_EVENT_UART2_RX;
            break;
        default:
            panic("rx notifier got unexpected com_num");
            break;
    }

    io_server_tid = WhoIs(io_server_name);

    do {
        int data = AwaitEvent(rx_event);
        if (res < 0) {
            panic("rx notifier failed to await event");
        }
        msg.data[0] = data;

        int res = Send(io_server_tid, &msg, sizeof(msg), &rep, sizeof(rep));
        if (res < 0) {
            panic("Send to io server failed\n\r");
            rep.type = IO_SERVER_MSG_TYPE_NOTIF_RX;
        }
    } while (rep.type != IO_SERVER_MSG_TYPE_EXIT);  // requires user to type char when exiting
    bwprintf(COM2, "IO server rx notifier shutting down\n\r");
    Exit();
}

void io_server_notifier_cts_main(void) {
    uint8_t tx_notifier_tid;
    int res;

    volatile uint32_t *uart_base = (volatile uint32_t*) UART1_BASE;

    uint8_t clock_server_tid = WhoIs(CLOCK_SERVER_NAME);

    res = Receive(&tx_notifier_tid, NULL, 0);
    Reply(tx_notifier_tid, NULL, 0);

    res = Send(tx_notifier_tid, NULL, 0, NULL, 0);
    if (res < 0) {
        panic("Send to io server failed\n\r");
    }

    do {
        int cur_time = Time(clock_server_tid); // TODO: Currently using artificial delay
        res = AwaitEvent(SYS_CALL_EVENT_UART1_MS);
        if (res < 0) {
            panic("cts notifier failed to await event");
        }

        if (/*(*(uart_base + UART_MDMSTS_OFFSET) & DCTS_MASK) && */!(*(uart_base + UART_FLAG_OFFSET) & CTS_MASK)) {
            DelayUntil(clock_server_tid, cur_time + 1);
            res = Send(tx_notifier_tid, NULL, 0, NULL, 0);
            if (res < 0) {
                panic("Send to io server failed\n\r");
            }
        }

    } while (true);

    bwprintf(COM2, "IO server cts notifier shutting down\n\r");
    Exit();
}

void static io_server_notifier_tx_main(void) {
    uint8_t io_server_tid = 0;
    uint8_t sender_tid;
    uint8_t cts_notifier_tid;
    io_server_msg_t msg, rep;
    msg.type = IO_SERVER_MSG_TYPE_NOTIF_TX;
    msg.len = rep.len = 0;
    uint8_t com_num;
    volatile uint32_t *uart_base = NULL;
    event_t tx_event = SYS_CALL_EVENT_UART2_TX;
    char *io_server_name = NULL;

    // get com number from parent
    int res = Receive(&sender_tid, &com_num, sizeof(com_num));
    if (res < 0) {
        panic("tx notifier failed to get COM number");
    }
    Reply(sender_tid, NULL, 0);
    switch(com_num) {
        case COM1:
            uart_base = (volatile uint32_t*) UART1_BASE;
            io_server_name = IO_SERVER_NAME1;
            tx_event = SYS_CALL_EVENT_UART1_TX;
            cts_notifier_tid = Create(1, io_server_notifier_cts_main);
            Send(cts_notifier_tid, NULL, 0, NULL, 0);

            break;
        case COM2:
            uart_base = (volatile uint32_t*) UART2_BASE;
            io_server_name = IO_SERVER_NAME2;
            tx_event = SYS_CALL_EVENT_UART2_TX;
            break;
        default:
            panic("tx notifier got unexpected com_num");
            break;
    }

    io_server_tid = WhoIs(io_server_name);

    do {
        // pass msg buffer to hardware one char at a time
        if (com_num == COM1) {
            // bwprintf(COM2, "%d\n\r", rep.len);
            for (int i = 0;i < rep.len; i++) {
                res = AwaitEvent(tx_event);
                Receive(&cts_notifier_tid, NULL, 0);
                Reply(cts_notifier_tid, NULL, 0);
                if (res < 0) {
                    panic("tx notifier failed to await event");
                }
                *(uart_base + UART_DATA_OFFSET) = 0xFF & rep.data[i];
            }
        } else {
            for (int i = 0;i < rep.len; i++) {
                res = AwaitEvent(tx_event);
                if (res < 0) {
                    panic("tx notifier failed to await event");
                }
                *(uart_base + UART_DATA_OFFSET) = 0xFF & rep.data[i];
            }
        }
        rep.len = 0;

        // done transmitting
        int res = Send(io_server_tid, &msg, sizeof(msg), &rep, sizeof(rep));
        if (res < 0) {
            panic("Send to io server failed\n\r");
            rep.len = 0;
            rep.type = IO_SERVER_MSG_TYPE_NOTIF_TX;
        }
    } while(rep.type != IO_SERVER_MSG_TYPE_EXIT);
    bwprintf(COM2, "IO server tx notifier shutting down\n\r");
    Exit();
}

// entry point for io server
void static io_server_main(void) {
    io_server_msg_t msg;
    uint8_t sender_tid, tx_notifier_tid, rx_notifier_tid;
    uint8_t in_buf[IO_SERVER_BUFFER_SIZE], out_buf[IO_SERVER_BUFFER_SIZE];
    ringbuffer_t rb_in, rb_out;
    ringbuffer_init(&rb_in, in_buf, IO_SERVER_BUFFER_SIZE);
    ringbuffer_init(&rb_out, out_buf, IO_SERVER_BUFFER_SIZE);
    bool tx_ready = false;
    bool client_ready = false;
    uint8_t client_tid = 0;
    uint8_t com_num;

    // get com number from parent
    int res = Receive(&sender_tid, &com_num, sizeof(com_num));
    if (res < 0) {
        panic("io server failed to get COM number");
    }
    Reply(sender_tid, NULL, 0);

    switch(com_num) {
        case COM1:
            //panic("Train io_server not finished yet");
            RegisterAs(IO_SERVER_NAME1);
            break;
        case COM2:
            RegisterAs(IO_SERVER_NAME2);
            break;
        default:
            panic("io server got unexpected com num");
            break;
    }

    // create notifier tasks
    tx_notifier_tid = Create(1, io_server_notifier_tx_main);
    rx_notifier_tid = Create(1, io_server_notifier_rx_main);
    res = Send(tx_notifier_tid, &com_num, sizeof(com_num), NULL, 0);
    if (res < 0) {
        panic("io server failed to send COM number to tx notifier");
    }
    res = Send(rx_notifier_tid, &com_num, sizeof(com_num), NULL, 0);
    if (res < 0) {
        panic("io server failed to send COM number to rx notifier");
    }

    do {
        int res = Receive(&sender_tid, &msg, sizeof(msg));
        if (res < 0) {
            panic("IO server error when receiving\n\r");
        }

        switch (msg.type) {
            case IO_SERVER_MSG_TYPE_CLIENT_READ:
                if (ringbuffer_empty(&rb_in)) {
                    // add ready client
                    client_ready = true;
                    client_tid = sender_tid;
                } else {
                    // flush buffer contents to receiving client
                    int len;
                    for (len = 0;len < msg.len && !ringbuffer_empty(&rb_in);len++) {
                        msg.data[len] = *ringbuffer_get(&rb_in);
                    }
                    msg.len = len;
                    Reply(sender_tid, &msg, sizeof(msg));
                }
                break;
            case IO_SERVER_MSG_TYPE_CLIENT_WRITE:
                if (!ringbuffer_putn(&rb_out, msg.data, msg.len)) {
                    panic("Not enough space in ringbuffer!\n\r");
                } else if (tx_ready) {
                    // send to TX notifier if ready
                    for (msg.len = 0;
                            !ringbuffer_empty(&rb_out) && msg.len < IO_SERVER_MSG_MAX_DATA_LEN;
                            msg.len++) {
                        msg.data[msg.len] = *ringbuffer_get(&rb_out);
                    }
                    Reply(tx_notifier_tid, &msg, sizeof(msg));
                    tx_ready = false;
                }
                msg.len = 0;
                Reply(sender_tid, &msg, sizeof(msg));
                break;
            case IO_SERVER_MSG_TYPE_NOTIF_RX:
                if (!ringbuffer_put(&rb_in, msg.data[0])) {
                    panic("Input ringbuffer full!\n\r");
                } else if (client_ready) {
                    msg.data[0] = *ringbuffer_get(&rb_in);
                    msg.len = 1;
                    Reply(client_tid, &msg, sizeof(msg));
                    client_ready = false;
                }
                msg.len = 0;
                Reply(sender_tid, &msg, sizeof(msg));
                break;
            case IO_SERVER_MSG_TYPE_NOTIF_TX:
                for (msg.len = 0;
                        !ringbuffer_empty(&rb_out) && msg.len < IO_SERVER_MSG_MAX_DATA_LEN;
                        msg.len++) {
                    msg.data[msg.len] = *ringbuffer_get(&rb_out);
                }
                if (msg.len > 0) {
                    Reply(sender_tid, &msg, sizeof(msg));
                } else {
                    tx_ready = true;
                }
                break;
            case IO_SERVER_MSG_TYPE_EXIT:
                break;
            default:
                panic("IO server received unknown message type\n\r");
                break;
        }
    } while (msg.type != IO_SERVER_MSG_TYPE_EXIT);

    // exit notifiers here
    int notifier_exit_count = 0;
    while (notifier_exit_count < 2) {
        Receive(&sender_tid, &msg, sizeof(msg));
        switch(msg.type) {
            case IO_SERVER_MSG_TYPE_NOTIF_RX:
            case IO_SERVER_MSG_TYPE_NOTIF_TX:
                msg.type = IO_SERVER_MSG_TYPE_EXIT;
                Reply(sender_tid, &msg, sizeof(msg));
                notifier_exit_count++;
            default:
                break;
        }
    }

    bwprintf(COM2, "IO server shutting down\n\r");
    Exit();
}

void io_server_exit(uint8_t io_server_tid) {
    io_server_msg_t msg, rep;
    msg.type = IO_SERVER_MSG_TYPE_EXIT;
    msg.len = 0;
    int res = Send(io_server_tid, &msg, sizeof(msg), &rep, sizeof(rep));
    if (res < 0) {
        panic("Error occurred shutting down io server\n\r");
    }
}

uint8_t io_server_start(uint8_t priority, uint8_t uart) {
    uint8_t io_server_tid = Create(priority, io_server_main);
    int res = Send(io_server_tid, &uart, sizeof(uart), NULL, 0);
    if (res < 0) {
        panic("failed to start io server");
    }
    return io_server_tid;
}

int Getc(int tid, int uart) {
    io_server_msg_t msg, rep;
    msg.type = IO_SERVER_MSG_TYPE_CLIENT_READ;
    msg.len = 1;
    int res = Send(tid, &msg, sizeof(msg), &rep, sizeof(rep));
    if (res < 0) {
        bwprintf(COM2, "Error occurred getting char\n\r");
        return -2;
    }
    return rep.data[0];
}

int Putc(int tid, int uart, char ch) {
    io_server_msg_t msg, rep;
    msg.type = IO_SERVER_MSG_TYPE_CLIENT_WRITE;
    msg.len = 1;
    msg.data[0] = ch;
    int res = Send(tid, &msg, sizeof(msg), &rep, sizeof(rep));
    if (res < 0) {
        bwprintf(COM2, "Error occurred putting char\n\r");
        return -2;
    }
    return 0;
}

int Putstr(int tid, int uart, char *str, size_t len) {
    io_server_msg_t msg, rep;
    msg.type = IO_SERVER_MSG_TYPE_CLIENT_WRITE;
    msg.len = (len < IO_SERVER_MSG_MAX_DATA_LEN) ? len : IO_SERVER_MSG_MAX_DATA_LEN;
    memcpy(msg.data, str, msg.len);
    int res = Send(tid, &msg, sizeof(msg), &rep, sizeof(rep));
    if (res < 0) {
        bwprintf(COM2, "Error occurred putting str\n\r");
        return -2;
    }
    return 0;
}
