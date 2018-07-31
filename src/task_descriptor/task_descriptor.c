#include <internal/task_descriptor.h>
#include <internal/queue.h>
#include <internal/mem.h>

void task_descriptor_init(
        task_descriptor_t *ctx, uint8_t tid, uint8_t priority,
        task_descriptor_t *parent, void (*code) (void))
{
    ctx->tid = tid;
    ctx->priority = priority;
    ctx->parent = parent;
    ctx->sp = (void *)(MEM_TASK_STACK_START(tid) - (14 * 4));
    ctx->spsr = 0x50;
    ctx->svc_lr = (void *)code;
    task_descriptor_set_return_value(ctx, 0);

    // Set stack variables properly
    // ((uint32_t *)ctx->sp)[0] = (uint32_t)code;
    ((uint32_t *)ctx->sp)[13] = (MEM_TASK_STACK_START(tid));

    queue_node_init(&ctx->ready_node, (void *)ctx);
    queue_node_init(&ctx->send_node, (void *)ctx);
    queue_node_init(&ctx->await_node, (void *)ctx);
    queue_init(&ctx->send_q);

    ctx->state = READY;
}

void task_descriptor_set_return_value(task_descriptor_t *ctx, int ret) {
    *((int *)ctx->sp) = ret;
}
