#include <internal/scheduler.h>

#include <stddef.h>
#include <bwio.h>

#define SCHEDULER_FULL_BITMAP   0xFFFFFFFFFFFFFFFFULL
#define SCHEDULER_EMPTY_BITMAP  0ULL

// find non-empty queue with highest priority from bitmap in O(lg(n))
uint8_t bitmap_leadingzeroes(uint64_t bitmap) {
    if (bitmap & 0x1ULL) {
        return 0;
    } else {
        uint8_t count = 1;
        if ((bitmap & 0xFFFFFFFFULL) == 0) {
            bitmap >>= 32;
            count += 32;
        }
        if ((bitmap & 0xFFFFULL) == 0) {
            bitmap >>= 16;
            count += 16;
        }
        if ((bitmap & 0xFFULL) == 0) {
            bitmap >>= 8;
            count += 8;
        }
        if ((bitmap & 0xFULL) == 0) {
            bitmap >>= 4;
            count += 4;
        }
        if ((bitmap & 0x3ULL) == 0) {
            bitmap >>= 2;
            count += 2;
        }
        count -= bitmap & 0x1ULL;

        return count;
    }
}

void scheduler_init(scheduler_t *sch) {
    sch->bitmap = SCHEDULER_EMPTY_BITMAP;
    for (uint8_t i = 0; i < SCHEDULER_PRIORITY_COUNT; i++) {
        queue_init(&(sch->priority_qs[i]));
    }
}

task_descriptor_t *scheduler_get(scheduler_t *sch) {
    if (scheduler_empty(sch)) {
        // no task in ready queue
        return NULL;
    }

    uint8_t idx = bitmap_leadingzeroes(sch->bitmap);
    queue_t *max_queue = &(sch->priority_qs[idx]);
    task_descriptor_t *ret = queue_get(max_queue);

    if (queue_empty(max_queue)) {
        sch->bitmap &= ~(1ULL << idx);
    }

    return ret;
}

uint8_t scheduler_put(scheduler_t *sch, task_descriptor_t *td) {
    uint8_t err = 0;
    err = queue_put(&(sch->priority_qs[td->priority]), &(td->ready_node));

    if (!err) {
        sch->bitmap |= (1ULL << td->priority);
        td->state = READY;
    }

    return err;
}

bool scheduler_empty(scheduler_t *sch) {
    return (sch->bitmap == SCHEDULER_EMPTY_BITMAP);
}
