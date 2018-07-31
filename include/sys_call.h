#ifndef SYS_CALL_H_INCLUDED_
#define SYS_CALL_H_INCLUDED_

#include <int_types.h>

typedef enum {
    SYS_CALL_EVENT_TIMER,
    SYS_CALL_EVENT_UART1,
    SYS_CALL_EVENT_UART1_MS,
    SYS_CALL_EVENT_UART1_RX,
    SYS_CALL_EVENT_UART1_RT,
    SYS_CALL_EVENT_UART1_TX,
    SYS_CALL_EVENT_UART2,
    SYS_CALL_EVENT_UART2_RX,
    SYS_CALL_EVENT_UART2_RT,
    SYS_CALL_EVENT_UART2_TX,
} event_t;

// Task Management
int Create(int priority, void (*code) ());
int MyTid();
int MyParentTid();
void Pass();
void Exit();

// Message Passing

// Send a message to a specified task and block until a reply is received.
// tid: The task id to send the message to
// msg: A pointer to the message to send
// msg_len: The size of the message, in bytes
// rep: A pointer to a buffer to store the reply
// rep_len: The expected reply size, in bytes
//
// Returns 0 if no error occurred and a negative value otherwise.
int Send(uint8_t tid, void *msg, size_t msg_len, void *rep, size_t rep_len);

// Receive a message.
// tid: A pointer to where the task id of the sender is to be stored
// msg: A pointer to a buffer to store the received message
// msg_len: The expected message size, in bytes
//
// Returns the task id of the sender or a negative value if an error occurred.
int Receive(uint8_t *tid, void *msg, size_t msg_len);

// Reply to a message.
// tid: The task id of the task to reply to
// rep: A pointer to the reply to send
// rep_len: The size of the reply, in bytes
//
// Returns 0 if no error occurred and a negative value otherwise.
int Reply(uint8_t tid, void *rep, size_t rep_len);

// Interrupt Processing
int AwaitEvent(int eventid);

// Stop kernel
void Quit(void);

// stop kernel, print an error message
void panic(char *msg);

#endif // SYS_CALL_H_INCLUDED_
