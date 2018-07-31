#ifndef IO_SERVER_INCLUDED_
#define IO_SERVER_INCLUDED_

#include <int_types.h>

#define IO_SERVER_NAME1             "io-server-1"
#define IO_SERVER_NAME2             "io-server-2"
#define IO_SERVER_MSG_MAX_DATA_LEN  31
#define IO_SERVER_BUFFER_SIZE       128

typedef enum {
    IO_SERVER_MSG_TYPE_EXIT,
    IO_SERVER_MSG_TYPE_CLIENT_READ,
    IO_SERVER_MSG_TYPE_CLIENT_WRITE,
    IO_SERVER_MSG_TYPE_NOTIF_RX,
    IO_SERVER_MSG_TYPE_NOTIF_TX,
    IO_SERVER_MSG_TYPE_NOTIF_CTS
} io_server_msg_type_t;

typedef struct {
    io_server_msg_type_t type;
    unsigned char data[IO_SERVER_MSG_MAX_DATA_LEN];
    size_t len;
} io_server_msg_t;

/* Wrapper Functions */

// Shuts down io server, will also shut down respective notifiers
void io_server_exit(uint8_t io_server_tid);

// Read single character
int Getc(int tid, int uart);

// Print single character
int Putc(int tid, int uart, char ch);

// Put string of characters
int Putstr(int tid, int uart, char *str, size_t len);

// Start io server task on specified COM port
// returns the tid of the started server
uint8_t io_server_start(uint8_t priority, uint8_t uart);

#endif // IO_SERVER_INCLUDED_
