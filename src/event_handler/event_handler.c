#include <internal/event_handler.h>

#include <bwio.h>

void event_handler_init(event_handler_t *ctx, scheduler_t *sch) {
    for (int i = 0; i < EVENT_HANDLER_MAX_EVENTS; ++i) {
        queue_init(&ctx->await_qs[i]);
    }
    ctx->sch = sch;
}

int event_handler_add_task(event_handler_t *ctx, event_t event, task_descriptor_t *td) {
    if (event < 0 || event >= EVENT_HANDLER_MAX_EVENTS) {
        return -1;
    }

    td->state = EVENT_BLOCKED;
    if (!queue_put(&ctx->await_qs[event], &td->await_node)) {
        return 0;
    }

    return -2;
}

int event_handler_handle_event(event_handler_t *ctx, event_t event, int ret) {
    if (event < 0 || event >= EVENT_HANDLER_MAX_EVENTS) {
        return -1;
    }

    while (!queue_empty(&ctx->await_qs[event])) {
        task_descriptor_t *td = queue_get(&ctx->await_qs[event]);
        task_descriptor_set_return_value(td, ret);
        scheduler_put(ctx->sch, td);
    }

    return 0;
}

bool event_handler_empty(event_handler_t *ctx) {
    for (int i = 0; i < EVENT_HANDLER_MAX_EVENTS; ++i) {
        if (!queue_empty(&ctx->await_qs[i])) {
            return false;
        }
    }
    return true;
}
