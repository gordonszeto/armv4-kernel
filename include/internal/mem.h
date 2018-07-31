#ifndef MEM_H_INCLUDED_
#define MEM_H_INCLUDED_

#include <int_types.h>

#define MEM_IRQ_STACK_START                 ((uintptr_t)&_irq_stack_start)
#define MEM_IRQ_STACK_SIZE                  (0x100)

#define MEM_KERNEL_STACK_START              ((uintptr_t)&_stack_start)
#define MEM_KERNEL_STACK_SIZE               (0x2000)            // 8KB stack

#define MEM_TASK_STACK_SIZE                 (0x2000)            // 8KB stack

#define MEM_TASK_STACK_START(task_num)      ((MEM_KERNEL_STACK_START - MEM_KERNEL_STACK_SIZE) \
                                            - (MEM_TASK_STACK_SIZE * (task_num)))
extern int _stack_start;

extern int _irq_stack_start;

#endif // MEM_H_INCLUDED_
