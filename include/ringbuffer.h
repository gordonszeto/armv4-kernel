#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <int_types.h>
#include <bool.h>

typedef struct {
    size_t read_index;
    size_t write_index;
    size_t size;
    uint8_t *buf;
} ringbuffer_t;

void ringbuffer_init(ringbuffer_t *ctx, uint8_t *buf, size_t buf_size);
uint8_t* ringbuffer_get(ringbuffer_t *ctx);
bool ringbuffer_put(ringbuffer_t *ctx, uint8_t data);
bool ringbuffer_putn(ringbuffer_t *ctx, uint8_t *data, size_t data_len);
bool ringbuffer_empty(ringbuffer_t *ctx);
bool ringbuffer_full(ringbuffer_t *ctx);
size_t ringbuffer_size(ringbuffer_t *ctx);

#endif // RINGBUFFER_H_
