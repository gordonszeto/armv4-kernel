#ifndef SCHEDULER_H_INCLUDED_
#define SCHEDULER_H_INCLUDED_

#include <int_types.h>
#include <bool.h>

#include <internal/task_descriptor.h>
#include <internal/queue.h>

#define SCHEDULER_PRIORITY_COUNT 64

typedef struct {
    uint64_t bitmap;
    queue_t priority_qs[SCHEDULER_PRIORITY_COUNT];
} scheduler_t;

// initialize struct with empty queues and zero'd bitmap
void scheduler_init(scheduler_t *sch);

// Returns pointer to next TD to be run from the priority queues
// returns the highest priority TD that is ready to run or NULL if
// all ready queues are empty
task_descriptor_t *scheduler_get(scheduler_t *sch);

// add a task to its corresponding ready queue, returns non-zero if
// TD could not be inserted into the queue
uint8_t scheduler_put(scheduler_t *sch, task_descriptor_t *td);

// return true if empty, false otherwise
bool scheduler_empty(scheduler_t *sch);

#endif // SCHEDULER_H_INCLUDED_
