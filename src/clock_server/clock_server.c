#include <clock_server.h>
#include <name_server.h>
#include <sys_call.h>
#include <bwio.h>
#include <int_types.h>
#include <stddef.h>

#include <internal/queue.h>
#include <internal/task_descriptor.h>

#define TICK_EVENT SYS_CALL_EVENT_TIMER

typedef enum {
    CLOCK_SERVER_MSG_TYPE_TICK,
    CLOCK_SERVER_MSG_TYPE_TIME,
    CLOCK_SERVER_MSG_TYPE_DELAY,
    CLOCK_SERVER_MSG_TYPE_DELAYUNTIL,
    CLOCK_SERVER_MSG_TYPE_EXIT,
    CLOCK_SERVER_MSG_TYPE_ERROR
} clock_server_msg_type_t;

#define CLOCK_SERVER_INVALID_TID    -1
#define CLOCK_SERVER_INVALID_DELAY  -2

typedef struct {
    clock_server_msg_type_t type;
    int time;
} clock_server_msg_t;

typedef struct {
    int ticks;
    uint8_t tid;
    queue_node_t node;
} clock_server_blocked_entry_t;

int Delay(int tid, int ticks) {
    clock_server_msg_t msg, rep;
    msg.type = CLOCK_SERVER_MSG_TYPE_DELAY;
    msg.time = ticks;
    int res = Send(tid, &msg, sizeof(msg), &rep, sizeof(rep));
    if (res) {
        bwprintf(COM2, "Send to clock server failed for Delay\n\r");
        return CLOCK_SERVER_INVALID_TID;
    }
    if (msg.type == CLOCK_SERVER_MSG_TYPE_ERROR) {
        bwprintf(COM2, "Delay failed with error\n\r");
        return CLOCK_SERVER_INVALID_DELAY;
    }
    return 0;
}

int Time(int tid) {
    clock_server_msg_t msg, rep;
    msg.type = CLOCK_SERVER_MSG_TYPE_TIME;
    int res = Send(tid, &msg, sizeof(msg), &rep, sizeof(rep));
    if (res) {
        bwprintf(COM2, "Send to clock server failed for Time\n\r");
        return CLOCK_SERVER_INVALID_TID;
    }
    return rep.time;
}

int DelayUntil(int tid, int ticks) {
    clock_server_msg_t msg, rep;
    msg.type = CLOCK_SERVER_MSG_TYPE_DELAYUNTIL;
    msg.time = ticks;
    int res = Send(tid, &msg, sizeof(msg), &rep, sizeof(rep));
    if (res) {
        bwprintf(COM2, "Send to clock server failed for DelayUntil\n\r");
        return CLOCK_SERVER_INVALID_TID;
    }
    if (msg.type == CLOCK_SERVER_MSG_TYPE_ERROR) {
        bwprintf(COM2, "DelayUntil failed with error\n\r");
        return CLOCK_SERVER_INVALID_DELAY;
    }
    return 0;
}

void clock_server_main(void) {
    uint8_t sender_tid = 0;
    clock_server_msg_t msg, rep;
    uint32_t ticks = 0;

    // init queues and nodes for list of blocked task
    queue_t blocked, free_list;
    clock_server_blocked_entry_t entries[TASK_DESCRIPTOR_MAX_TASKS];
    queue_init(&blocked);
    queue_init(&free_list);
    for (size_t i = 0; i < TASK_DESCRIPTOR_MAX_TASKS; i++) {
        queue_node_init(&entries[i].node, entries + i);
        queue_put(&free_list, &entries[i].node);
    }

    RegisterAs(CLOCK_SERVER_NAME);
    Create(2, clock_server_notifier_main);
    do {
        int res = Receive(&sender_tid, &msg, sizeof(msg));
        if (res < 0) {
            bwprintf(COM2, "Error occurred while receiving in clock server\n\r");
        }
        switch (msg.type) {
            case CLOCK_SERVER_MSG_TYPE_TICK:
                Reply(sender_tid, &msg, sizeof(msg));   // unblock notifier ASAP
                ticks++;

                // check if block task should be made ready
                if (!queue_empty(&blocked)) {
                    clock_server_blocked_entry_t *front = (clock_server_blocked_entry_t*) queue_peek(&blocked);
                    if (ticks > front->ticks) {
                        queue_get(&blocked);
                        queue_put(&free_list, &front->node);
                        msg.time = ticks;
                        Reply(front->tid, &msg, sizeof(msg));
                    }
                }
                break;
            case CLOCK_SERVER_MSG_TYPE_TIME:
                msg.time = ticks;
                Reply(sender_tid, &msg, sizeof(msg));
                break;
            case CLOCK_SERVER_MSG_TYPE_DELAY:
                if (msg.time <= 0) {
                    msg.type = CLOCK_SERVER_MSG_TYPE_ERROR;
                    msg.time = 0;
                    Reply(sender_tid, &msg, sizeof(msg));
                    break;
                }
                msg.time += ticks;
            case CLOCK_SERVER_MSG_TYPE_DELAYUNTIL:
                if (msg.time <= 0) {
                    msg.type = CLOCK_SERVER_MSG_TYPE_ERROR;
                    msg.time = 0;
                    Reply(sender_tid, &msg, sizeof(msg));
                } else if (msg.time < ticks) {
                    msg.type = CLOCK_SERVER_MSG_TYPE_TICK;
                    msg.time = ticks;
                    Reply(sender_tid, &msg, sizeof(msg));
                } else {
                    clock_server_blocked_entry_t *new_node_data = (clock_server_blocked_entry_t*)queue_get(&free_list);
                    queue_node_t *new_node = &new_node_data->node;
                    new_node_data->ticks = msg.time;
                    new_node_data->tid = sender_tid;

                    // insert into sorted order
                    if (queue_empty(&blocked)) {
                        queue_put_front(&blocked, new_node);
                    } else {
                        clock_server_blocked_entry_t *front_node_data = (clock_server_blocked_entry_t*)queue_peek(&blocked);
                        if (front_node_data->ticks > new_node_data->ticks) {
                            queue_put_front(&blocked, new_node);
                        } else {
                            queue_node_t *curr_node = blocked.front;
                            while (curr_node->next != NULL) {
                                clock_server_blocked_entry_t *next_node_data = (clock_server_blocked_entry_t*)curr_node->next->data;
                                if (next_node_data->ticks > new_node_data->ticks) {
                                    break;
                                }
                                curr_node = curr_node->next;
                            }
                            new_node->next = curr_node->next;
                            curr_node->next = new_node;
                            blocked.elements++;
                        }
                    }
                }
            case CLOCK_SERVER_MSG_TYPE_EXIT:
                break;
            default:
                bwprintf(COM2, "Clock server received unexpected msg type\n\r");
                break;
        }
    } while (msg.type != CLOCK_SERVER_MSG_TYPE_EXIT);
    rep.type = CLOCK_SERVER_MSG_TYPE_EXIT;
    rep.time = -1;
    uint8_t exiter_tid = sender_tid;

    // clear out any tasks that aren't the notifier
    while (msg.type != CLOCK_SERVER_MSG_TYPE_TICK) {
        Receive(&sender_tid, &msg, sizeof(msg));
    }
    Reply(sender_tid, &rep, sizeof(rep));
    bwprintf(COM2, "Clock server shutting down\n\r");
    Reply(exiter_tid, &rep, sizeof(rep));
    Exit();
}

void clock_server_exit(uint8_t clock_server_tid) {
    clock_server_msg_t msg, rep;
    msg.type = CLOCK_SERVER_MSG_TYPE_EXIT;
    int res = Send(clock_server_tid, &msg, sizeof(msg), &rep, sizeof(rep));
    if (res) {
        bwprintf(COM2, "Error shutting down clock server\n\r");
    }
}

// Blocks until interrupt event occurs, sends message to clock_server when an
// interrupt is received. Done nothing with reply
void clock_server_notifier_main(void) {
    clock_server_msg_t msg, rep;
    msg.type = CLOCK_SERVER_MSG_TYPE_TICK;
    int clock_server_tid;
    clock_server_tid = WhoIs(CLOCK_SERVER_NAME);

    rep.type = CLOCK_SERVER_MSG_TYPE_TICK;

    while (rep.type != CLOCK_SERVER_MSG_TYPE_EXIT) {
        AwaitEvent(TICK_EVENT);
        Send(clock_server_tid, &msg, sizeof(msg), &rep, sizeof(rep));
    }

    bwprintf(COM2, "Clock server notifier shutting down\n\r");
    Exit();
}
