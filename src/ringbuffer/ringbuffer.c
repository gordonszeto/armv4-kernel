#include <ringbuffer.h>

#include <int_types.h>
#include <stddef.h>

void ringbuffer_init(ringbuffer_t *ctx, uint8_t *buf, size_t buf_size) {
    ctx->read_index = 0;
    ctx->write_index = 0;
    ctx->size = buf_size;
    ctx->buf = buf;
}

uint8_t* ringbuffer_get(ringbuffer_t *ctx) {
    if (ringbuffer_empty(ctx)) {
        return NULL;
    }

    uint8_t *ret = &ctx->buf[ctx->read_index];
    ctx->read_index = (ctx->read_index + 1) % ctx->size;
    return ret;
}

bool ringbuffer_put(ringbuffer_t *ctx, uint8_t data) {
    if (ringbuffer_full(ctx)) {
        return false;
    }

    ctx->buf[ctx->write_index] = data;
    ctx->write_index = (ctx->write_index + 1) % ctx->size;
    return true;
}

bool ringbuffer_putn(ringbuffer_t *ctx, uint8_t *data, size_t data_len) {
    if ((ctx->size - 1) - ringbuffer_size(ctx) < data_len) {
        return false;
    }

    for (size_t i = 0; i < data_len; ++i) {
        ctx->buf[ctx->write_index] = data[i];
        ctx->write_index = (ctx->write_index + 1) % ctx->size;
    }

    return true;
}

bool ringbuffer_empty(ringbuffer_t *ctx) {
    return ctx->read_index == ctx->write_index;
}

bool ringbuffer_full(ringbuffer_t *ctx) {
    return ((ctx->write_index + 1) % ctx->size) == ctx->read_index;
}

size_t ringbuffer_size(ringbuffer_t *ctx) {
    size_t ret;

    if (ctx->write_index < ctx->read_index) {
        ret = (ctx->write_index + ctx->size) - ctx->read_index;
    } else {
        ret = ctx->write_index - ctx->read_index;
    }

    return ret;
}
