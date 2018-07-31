#ifndef EVENT_HANDLER_H_INCLUDED_
#define EVENT_HANDLER_H_INCLUDED_

#include <sys_call.h>

#include <internal/queue.h>
#include <internal/scheduler.h>
#include <internal/task_descriptor.h>

#define EVENT_HANDLER_MAX_EVENTS 16

typedef struct {
    queue_t await_qs[EVENT_HANDLER_MAX_EVENTS];
    scheduler_t *sch;
} event_handler_t;

void event_handler_init(event_handler_t *ctx, scheduler_t *sch);
int event_handler_add_task(event_handler_t *ctx, event_t event, task_descriptor_t *td);
int event_handler_handle_event(event_handler_t *ctx, event_t event, int ret);
bool event_handler_empty(event_handler_t *ctx);

#endif // EVENT_HANDLER_H_INCLUDED_
