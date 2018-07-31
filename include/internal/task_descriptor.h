#ifndef TASK_DESCRIPTOR_H_INCLUDED_
#define TASK_DESCRIPTOR_H_INCLUDED_

#include <internal/queue.h>
#include <int_types.h>

#define TASK_DESCRIPTOR_MAX_TASKS 64

enum task_state_t {
    READY,
    ACTIVE,
    EXITED,
    SEND_BLOCKED,
    RECEIVE_BLOCKED,
    REPLY_BLOCKED,
    EVENT_BLOCKED
};

typedef struct task_descriptor {
    uint8_t tid;
    uint8_t priority;
    struct task_descriptor *parent;
    queue_node_t ready_node;
    queue_node_t send_node;
    queue_node_t await_node;
    queue_t send_q;
    enum task_state_t state;
    void *sp;
    void *svc_lr;
    uint32_t spsr;

    struct {
        uint8_t *tid;
        void *msg;
        size_t msg_len;
        void *rep;
        size_t rep_len;
    } message_params;
} task_descriptor_t;

// Initialize a task descriptor with the given parameters.
// queue node have data pointer set to the ctx task descriptor
// should spsr be initialized here too?
void task_descriptor_init(
    task_descriptor_t *ctx, uint8_t tid, uint8_t priority,
    task_descriptor_t *parent, void (*code) (void));

void task_descriptor_set_return_value(task_descriptor_t *ctx, int ret);

#endif // TASK_DESCRIPTOR_H_INCLUDED_
