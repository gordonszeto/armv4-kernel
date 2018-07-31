#include <stddef.h>
#include <internal/queue.h>

void queue_node_init(queue_node_t *ctx, void *data) {
    ctx->data = data;
    ctx->next = NULL;
}

void queue_init(queue_t *ctx) {
    ctx->front = NULL;
    ctx->back = NULL;
    ctx->elements = 0;
}

uint8_t queue_put(queue_t *ctx, queue_node_t *node) {
    if (node == NULL) {
        return 1;
    }

    if (ctx->elements == 0) {
        ctx->front = node;
    } else {
        ctx->back->next = node;
    }

    ctx->back = node;
    node->next = NULL;
    ctx->elements++;
    return 0;
}

uint8_t queue_put_front(queue_t *ctx, queue_node_t *node) {
    if (node == NULL) {
        return 1;
    }

    if (ctx->elements == 0) {
        ctx->back = node;
        node->next = NULL;
    } else {
        node->next = ctx->front;
    }

    ctx->front = node;
    ctx->elements++;
    return 0;
}

void *queue_get(queue_t *ctx) {
    if (queue_empty(ctx)) {
        return NULL;
    }

    void *ret = ctx->front->data;
    queue_node_t *old = ctx->front;
    ctx->front = ctx->front->next;
    old->next = NULL;
    ctx->elements--;

    return ret;
}

void *queue_peek(queue_t *ctx) {
    if (queue_empty(ctx)) {
        return NULL;
    }

    return ctx->front->data;
}

size_t queue_size(queue_t *ctx) {
    return ctx->elements;
}

bool queue_empty(queue_t *ctx) {
    return (queue_size(ctx) == 0);
}

uint8_t queue_mtf(queue_t *ctx, queue_node_t *parent) {
    if (parent == NULL) {
        return 0;
    } else if (parent->next == NULL || queue_empty(ctx)) {
        return 1;
    }

    // pull node out of queue and put back in front
    queue_node_t *new_front = parent->next;
    parent->next = new_front->next;
    new_front->next = parent;
    ctx->front = new_front;
    return 0;
}
