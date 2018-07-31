#ifndef QUEUE_H_INCLUDED_
#define QUEUE_H_INCLUDED_

#include <bool.h>
#include <int_types.h>

typedef struct queue_node_t {
    void *data;
    struct queue_node_t *next;
} queue_node_t;

typedef struct {
    queue_node_t *front;
    queue_node_t *back;
    size_t elements;
} queue_t;

// Initialize a queue node with data pointer to data, next is null
void queue_node_init(queue_node_t *ctx, void *data);

// Initialize a queue to be empty
void queue_init(queue_t *ctx);

// Insert a the node to the end of the queue
// The queue does not own the node and its lifetime must extend beyond its time in the queue.
// Returns 0 if successful, 1 otherwise.
uint8_t queue_put(queue_t *ctx, queue_node_t *node);

// Insert a node at the front of the queue. The queue does not own the node
// and its lifetime must extend beyond its time in the queue. Returns 0 on
// success, 1 otherwise
uint8_t queue_put_front(queue_t *ctx, queue_node_t *node);

// Retrieves a pointer to the first element's data in the queue
// and removes the element from the queue.
// Returns a pointer to the first element's data, or NULL if the queue is empty.
void *queue_get(queue_t *ctx);

// Retrieves a pointer to first element's data in the queue
// without removing it from the queue.
// Returns a pointer to first element's, or NULL if the queue is empty.
void *queue_peek(queue_t *ctx);

// Returns the number of elements currently in the queue.
size_t queue_size(queue_t *ctx);

// Returns true if the queue is empty, false otherwise.
bool queue_empty(queue_t *ctx);

// moves the next node of parent to the front of queue ctx, returns 0 on
// success and 1 if an error occured. Does nothing if parent is NULL. Returns 1
// if parent does not have a next node. Non-NULL Parent must be a node in ctx
uint8_t queue_mtf(queue_t *ctx, queue_node_t *parent);

#endif // QUEUE_H_INCLUDED_
