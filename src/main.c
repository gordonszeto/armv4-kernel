#include <int_types.h>
#include <bwio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <ts7200.h>
#include <name_server.h>
#include <clock_server.h>

#include <internal/event_handler.h>
#include <internal/mem.h>
#include <internal/queue.h>
#include <internal/scheduler.h>
#include <internal/sys_codes.h>
#include <internal/task_descriptor.h>

#define IDLE_TASK_TID 1
#define IDLE_TASK_EXIT_OFFSET 0x100
#define IDLE_TASK_NON_IDLE_TIME_OFFSET 0x104

#define ARG0_OFFSET 0
#define ARG1_OFFSET 1
#define ARG2_OFFSET 2

#define REG(base, offset) (*(volatile uint32_t *)((base) + (offset)))

typedef struct {
    task_descriptor_t *tds;
    scheduler_t *sch;
    event_handler_t *eh;
    size_t next_free_td;
    struct {
        uint32_t last_idle_time;
    } metrics;
} kernel_context_t;

typedef uint32_t kernel_request_t;

// Forward declare the first user task. This must be defined elsewhere
void user_main(void);

// This forward declares the existence of a label named kernel_entry
void kernel_entry(void);

// This forward declares the existence of a label named irq_entry
void irq_entry(void);

static void activate(task_descriptor_t *td, kernel_request_t *req) {
    register int sys_code __asm__ ("r2");

    register void *active_sp __asm__ ("r0") = td->sp;
    register uint32_t active_spsr __asm__ ("r1") = td->spsr;
    register void *active_svc_lr __asm__ ("r3") = td->svc_lr;

    register void *irq_sp __asm__ ("r4") = (void *)MEM_IRQ_STACK_START;

    // Prepare to exit kernel
    // Step 1: Save kernel state
    __asm__("stmfd sp!, {%3-r12, lr}    @ Push all kernel registers onto kernel stack\n\t"
            "@ Step 2: Install active state\n\t"
            "msr spsr, %0               @ Store scratch register into SPSR\n\t"
            "mov lr, %2                 @ Store svc link register into link register\n\t"
            "mrs r1, cpsr               @ Load CPSR into scratch register\n\t"
            "orr r1, r1, #0x1F          @ Mask off lowest 5 bits for system mode\n\t"
            "msr cpsr, r1               @ Store into CPSR to enter system mode\n\t"
            "bic r1, r1, #0xC           @ Clear bits 2 and 3 (needed later to enter supervisor mode)\n\t"
            "mov sp, %1                 @ Load active task stack pointer\n\t"
            "mov r0, #0                 @ Move 0 into scratch register\n\t"
            "str r0, [%3]               @ Set bottom of irq stack to zero\n\t"
            "sub r0, %3, #4             @ Move to next stack space so we don't change bottom of stack\n\t"
            "ldmfd sp!, {r2, r3}        @ Pop 2 active task registers from user stack\n\t"
            "stmfd r0!, {r2, r3}        @ Push 2 active task registers to irq stack\n\t"
            "ldmfd sp!, {r2-r12, lr}    @ Pop all active task registers from user stack\n\t"
            "msr cpsr, r1               @ Store into CPSR to enter supervisor mode\n\t"
            "ldmfd r0, {r0, r1}         @ Pop 2 active task registers from irq stack\n\t"
            "movs pc, lr\n\t"
            :                                   // No output operands
            : "r" (active_spsr), "r" (active_sp), "r" (active_svc_lr), "r" (irq_sp));

    // IRQ entry point
    __asm__(".align 2\n\t"
            ".global irq_entry              @ Export irq_entry\n"
            "irq_entry:                     @ Here begins irq_entry\n"
            "   sub lr, lr, #4              @ Subtract 4 from link register\n"
            "   str lr, [sp]                @ Store irq link register onto irq stack (kills 3 birds with one stone!!)\n"
            "   mrs lr, spsr                @ Load SPSR into scratch register\n"
            "   str lr, [sp, #-4]           @ Push SPSR into irq stack\n"
            "   mrs lr, cpsr                @ Load CPSR into scratch register\n"
            "   orr lr, lr, #0x1            @ Set lowest bit for svc mode\n"
            "   msr cpsr, lr                @ Store scratch register into CPSR\n\t");

    // Kernel entry point after swi
    // Step 1: Save arguments passed into function
    __asm__(".align 2\n\t"
            ".global kernel_entry           @ Export kernel_entry\n"
            "kernel_entry:                  @ Here begins kernel_entry\n"
            "   str lr, [sp, #-4]!          @ Save svc link register into svc stack\n\t"

            "@ Step 2: Save active task state\n\t"
            "mrs lr, cpsr               @ Load CPSR into scratch register\n\t"
            "orr lr, lr, #0x1F          @ Mask off lowest 5 bits for system mode\n\t"
            "msr cpsr, lr               @ Store into CPSR to enter system mode\n\t"
            "stmfd sp!, {r0-r12, lr}    @ Store all active task registers onto user stack\n\t"
            "mov %0, sp                 @ Save user stack pointer into scratch register\n\t"
            "mrs r1, cpsr               @ Load CPSR into scratch register\n\t"
            "bic r1, r1, #0xC           @ Clear bits 2 and 3 for supervisor mode\n\t"
            "msr cpsr, r1               @ Store into CPSR to enter supervisor mode\n\t"

            "ldr lr, [sp], #4           @ Restore saved link register (only valid if swi)\n\t"
            "ldmfd sp!, {r4-r12}        @ Restore kernel state\n\t"
            "ldr %3, [r4]               @ Load data at bottom of irq stack into scratch register\n\t"
            "cmp %3, #0                 @ Check if we are handling swi or irq\n\t"
            "beq swi_label              @ If bottom of irq stack is zero, we're handling swi\n\n\t"

            "ldr %1, [r4, #-4]          @ Load SPSR from irq stack\n\t"
            "b swi_label_end            @ Done handling irq\n\n"

            "swi_label:                 @ Beginning of handling logic for swi\n\t"
            "mrs %1, spsr               @ Load SPSR into scratch register\n\t"
            "@ At this point:\n\t"
            "@   r0 - Active task stack pointer\n\t"
            "@   r1 - Active task SPSR\n\t"
            "@   r3 - Active task link register\n\n\t"

            "@ Step 3: Get swi code\n\t"
            "ldr r2, [lr, #-4]\n\t"
            "bic %2, r2, #0xFF000000\n\t"
            "mov %3, lr\n\n\t"

            "@ At this point:\n\t"
            "@   r0 - Active task stack pointer\n\t"
            "@   r1 - Active task SPSR\n\t"
            "@   r2 - swi code\n\t"
            "@   r3 - Active task link register\n\n"

            "swi_label_end:                  @ End of branch\n\t"
            "ldr lr, [sp], #4\n\t"
            : "=r" (active_sp), "=r" (active_spsr), "=r" (sys_code), "=r" (active_svc_lr));

    td->sp = active_sp;
    td->spsr = active_spsr;
    td->svc_lr = active_svc_lr;

    *req = sys_code;
}

static int kernel_create(kernel_context_t *ctx, task_descriptor_t *active_td, int priority, void (*code) (void)) {
    if (priority < 0 || priority > 63) {
        return -1;
    }
    if (ctx->next_free_td >= TASK_DESCRIPTOR_MAX_TASKS) {
        return -2;
    }

    task_descriptor_t *new_task = &ctx->tds[ctx->next_free_td];
    task_descriptor_init(new_task, ctx->next_free_td, (uint8_t)priority, active_td, code);

    // Add new task to priority queue
    scheduler_put(ctx->sch, new_task);
    scheduler_put(ctx->sch, active_td);

    ctx->next_free_td++;
    return new_task->tid;
}

static bool kernel_exit(kernel_context_t *ctx, task_descriptor_t *active_td) {
    active_td->state = EXITED;
    return scheduler_empty(ctx->sch);
}

static void kernel_pass(kernel_context_t *ctx, task_descriptor_t *active_td) {
    scheduler_put(ctx->sch, active_td);
}

static int kernel_mytid(kernel_context_t *ctx, task_descriptor_t *active_td) {
    scheduler_put(ctx->sch, active_td);
    return active_td->tid;
}

static int kernel_parenttid(kernel_context_t *ctx, task_descriptor_t *active_td) {
    scheduler_put(ctx->sch, active_td);

    if (!active_td->parent) {
        return -1;
    }

    return active_td->parent->tid;
}

static int kernel_send(kernel_context_t *ctx, task_descriptor_t *active_td,
                       uint8_t tid, void *msg, size_t msg_len,
                       void *rep, size_t rep_len) {
    task_descriptor_t *receiver = &ctx->tds[tid];
    if (receiver->tid != tid) {
        // Bad tid
        scheduler_put(ctx->sch, active_td);
        return -1;
    }

    if (receiver->state == SEND_BLOCKED) {
        if (receiver->message_params.msg_len != msg_len) {
            // TODO: Sender/receiver size mismatch
            scheduler_put(ctx->sch, active_td);
            return -2;
        }
        memcpy(receiver->message_params.msg, msg, msg_len);

        *receiver->message_params.tid = active_td->tid;
        task_descriptor_set_return_value(receiver, active_td->tid);
        scheduler_put(ctx->sch, receiver);

        active_td->state = REPLY_BLOCKED;
    } else {
        queue_put(&receiver->send_q, &active_td->send_node);
        active_td->state = RECEIVE_BLOCKED;

        active_td->message_params.msg = msg;
        active_td->message_params.msg_len = msg_len;
    }

    active_td->message_params.rep = rep;
    active_td->message_params.rep_len = rep_len;

    return 0;
}

static int kernel_receive(kernel_context_t *ctx, task_descriptor_t *active_td,
                          uint8_t *tid, void *msg, size_t msg_len) {
    if (!queue_empty(&active_td->send_q)) {
        task_descriptor_t *sender = (task_descriptor_t *)queue_get(&active_td->send_q);
        if (sender->message_params.msg_len != msg_len) {
            // TODO: Sender/receiver size mismatch
            scheduler_put(ctx->sch, active_td);
            return -1;
        }
        memcpy(msg, sender->message_params.msg, msg_len);

        *tid = sender->tid;
        scheduler_put(ctx->sch, active_td);

        sender->state = REPLY_BLOCKED;
        return *tid;
    } else {
        active_td->state = SEND_BLOCKED;

        active_td->message_params.tid = tid;
        active_td->message_params.msg = msg;
        active_td->message_params.msg_len = msg_len;
    }
    
    return 0;
}

static int kernel_reply(kernel_context_t *ctx, task_descriptor_t *active_td,
                        uint8_t tid, void *rep, size_t rep_len) {
    task_descriptor_t *reply_receiver = &ctx->tds[tid];

    if (reply_receiver->tid != tid) {
        // Bad tid
        scheduler_put(ctx->sch, active_td);
        return -1;
    }

    if (reply_receiver->message_params.rep_len != rep_len) {
        // TODO: Sender / receiver size mismatch
        scheduler_put(ctx->sch, active_td);
        return -2;
    }
    memcpy(reply_receiver->message_params.rep, rep, rep_len);

    task_descriptor_set_return_value(reply_receiver, 0);
    scheduler_put(ctx->sch, reply_receiver);
    scheduler_put(ctx->sch, active_td);

    return 0;
}

static int kernel_await_event(kernel_context_t *ctx, task_descriptor_t *active_td, event_t event) {
    if (event < 0 || event >= EVENT_HANDLER_MAX_EVENTS) {
        scheduler_put(ctx->sch, active_td);
        return -1;
    }

    // enable UART transmit interrupt when task awaits event on them
    switch(event) {
        case SYS_CALL_EVENT_UART1_TX:
            REG(UART1_BASE, UART_CTLR_OFFSET) |= TIEN_MASK;
            break;
        case SYS_CALL_EVENT_UART2_TX:
            REG(UART2_BASE, UART_CTLR_OFFSET) |= TIEN_MASK;
            break;
        case SYS_CALL_EVENT_UART1_RX:
            REG(UART1_BASE, UART_CTLR_OFFSET) |= RIEN_MASK;
            break;
        case SYS_CALL_EVENT_UART2_RX:
            REG(UART2_BASE, UART_CTLR_OFFSET) |= RIEN_MASK;
            break;
        case SYS_CALL_EVENT_UART1_MS:
            REG(UART1_BASE, UART_CTLR_OFFSET) |= MSIEN_MASK;
            break;
        default:
            break;
    }

    event_handler_add_task(ctx->eh, event, active_td);

    return 0;
}

static void kernel_panic(kernel_context_t *ctx, task_descriptor_t *active_td, char *msg) {
    // TODO more elegant solution
    ctx->sch->bitmap = 0ULL;
    bwprintf(COM2, "Panic: %s\n\r", msg);
}

void update_idle_task(kernel_context_t *ctx) {
    volatile bool *is_exit = (bool *)(MEM_TASK_STACK_START(IDLE_TASK_TID) - IDLE_TASK_EXIT_OFFSET);
    volatile uint32_t *non_idle_time = (uint32_t *)(MEM_TASK_STACK_START(IDLE_TASK_TID) - IDLE_TASK_NON_IDLE_TIME_OFFSET);

    *is_exit = event_handler_empty(ctx->eh);
    uint32_t cur_time = clock();
    *non_idle_time += cur_time - ctx->metrics.last_idle_time;
}

void end_idle_task(kernel_context_t *ctx) {
    ctx->metrics.last_idle_time = clock();
}

void idle_main(void) {
    volatile bool *is_exit = (bool *)(MEM_TASK_STACK_START(IDLE_TASK_TID) - IDLE_TASK_EXIT_OFFSET);
    volatile uint32_t *non_idle_time = (uint32_t *)(MEM_TASK_STACK_START(IDLE_TASK_TID) - IDLE_TASK_NON_IDLE_TIME_OFFSET);

    while (!(*is_exit));
    uint32_t cur_time = clock();
    bwprintf(COM2, "Non-idle time: %u/%u\n\r", *non_idle_time, cur_time);
    Exit();
}

static bool handle(kernel_context_t *ctx, task_descriptor_t *active_td, kernel_request_t *req) {

    if (*((uint32_t *)MEM_IRQ_STACK_START) == 0) {
        int ret = 0;
        send_params_t *send_params;
        int arg0, arg1, arg2;

        arg0 = ((int *)active_td->sp)[ARG0_OFFSET];
        arg1 = ((int *)active_td->sp)[ARG1_OFFSET];
        arg2 = ((int *)active_td->sp)[ARG2_OFFSET];

        switch (*req) {
            case SYS_CODE_PASS:
                kernel_pass(ctx, active_td);
                break;
            case SYS_CODE_CREATE:
                ret = kernel_create(ctx, active_td, arg0, (void (*) (void))arg1);
                break;
            case SYS_CODE_MYTID:
                ret = kernel_mytid(ctx, active_td);
                break;
            case SYS_CODE_PARENTTID:
                ret = kernel_parenttid(ctx, active_td);
                break;
            case SYS_CODE_SEND:
                send_params = (send_params_t *)arg0;
                ret = kernel_send(ctx, active_td, send_params->tid,
                                send_params->msg, send_params->msg_len,
                                send_params->rep, send_params->rep_len);
                break;
            case SYS_CODE_RECEIVE:
                ret = kernel_receive(ctx, active_td, (uint8_t *)arg0, (void *)arg1, (size_t)arg2);
                break;
            case SYS_CODE_REPLY:
                ret = kernel_reply(ctx, active_td, (uint8_t)arg0, (void *)arg1, (size_t)arg2);
                break;
            case SYS_CODE_AWAITEVENT:
                ret = kernel_await_event(ctx, active_td, (event_t)arg0);
                break;
            case SYS_CODE_PANIC:
                kernel_panic(ctx, active_td, (char *)arg0);
                break;
            case SYS_CODE_EXIT:
                return kernel_exit(ctx, active_td);
            case SYS_CODE_QUIT:
                return true;
            default:
                kernel_panic(ctx, active_td, "kernel got unknown SWI sys code");
        }

        task_descriptor_set_return_value(active_td, ret);
        return false;

    } else {
        event_t event = REG(VIC2_BASE, VIC_VEC_ADDR_OFFSET);

        // determine interrupt type, get retVal
        uint8_t UART1_INT_ID_INT_CLR = REG(UART1_BASE, UART_INTR_OFFSET);
        uint8_t UART2_INT_ID_INT_CLR = REG(UART2_BASE, UART_INTR_OFFSET);
        event_t single_event = 0;
        int retVal = 0;

        switch (event) {
            case SYS_CALL_EVENT_TIMER:
                event_handler_handle_event(ctx->eh, event, 0);
                REG(TIMER3_BASE, CLR_OFFSET) = 0;
                REG(VIC2_BASE, VIC_VEC_ADDR_OFFSET) = 0;
                break;
            case SYS_CALL_EVENT_UART1:
                // responded to in priority order
                if (UART1_INT_ID_INT_CLR & RIS_MASK) {
                    single_event = SYS_CALL_EVENT_UART1_RX;
                    retVal = REG(UART1_BASE, UART_DATA_OFFSET) & DATA_MASK;
                } else if (UART1_INT_ID_INT_CLR & MIS_MASK) {
                    single_event = SYS_CALL_EVENT_UART1_MS;
                    retVal = REG(UART1_BASE, UART_FLAG_OFFSET);
                    REG(UART1_BASE, UART_CTLR_OFFSET) &= ~MSIEN_MASK;
                    REG(UART1_BASE, UART_INTR_OFFSET) = 0;
                } else if (UART1_INT_ID_INT_CLR & TIS_MASK) {
                    single_event = SYS_CALL_EVENT_UART1_TX;
                    REG(UART1_BASE, UART_CTLR_OFFSET) &= ~TIEN_MASK;
                } else {
                    kernel_panic(ctx, active_td, "handler got unexpected event type");
                    bwprintf(COM2, "Handler got unexpected event type: %d\n\r", event);
                }

                REG(VIC2_BASE, VIC_VEC_ADDR_OFFSET) = 0;
                event_handler_handle_event(ctx->eh, single_event, retVal);
                break;
            case SYS_CALL_EVENT_UART2:
                // responded to in priority order
                if (UART2_INT_ID_INT_CLR & RIS_MASK) {
                    single_event = SYS_CALL_EVENT_UART2_RX;
                    retVal = REG(UART2_BASE, UART_DATA_OFFSET) & DATA_MASK;
                } else if (UART2_INT_ID_INT_CLR & TIS_MASK) {
                    single_event = SYS_CALL_EVENT_UART2_TX;
                    REG(UART2_BASE, UART_CTLR_OFFSET) &= ~TIEN_MASK;
                } else {
                    kernel_panic(ctx, active_td, "handler got unexpected event type");
                    bwprintf(COM2, "Handler got unexpected event type: %d\n\r", event);
                }

                REG(VIC2_BASE, VIC_VEC_ADDR_OFFSET) = 0;
                event_handler_handle_event(ctx->eh, single_event, retVal);
                break;
            default:
                kernel_panic(ctx, active_td, "kernel handle got unexpected interrupt");
                break;
        }
        
        scheduler_put(ctx->sch, active_td);
        return false;
    } 
}

void static inline enable_vectored_interrupt(uint32_t vic_base, event_t event,
        uint32_t int_src, uint8_t int_priority) {
    REG(VIC2_BASE, VIC_VEC_ADDR_N_OFFSET(int_priority)) = (uint32_t)event;
    REG(VIC2_BASE, VIC_VEC_CTRL_N_OFFSET(int_priority)) = int_src | VIC_VEC_CTRL_EN_MASK;
    REG(VIC2_BASE, VIC_INT_EN_OFFSET) |= (1 << int_src);
}

void setup_vectored_interrupts(void) {
    // Setup Timer3 as highest priority interrupt
    enable_vectored_interrupt(VIC2_BASE, SYS_CALL_EVENT_TIMER, VIC2_TIMER3_INT, 0);

    // Setup UART1 as second-highest priority interrupt
    enable_vectored_interrupt(VIC2_BASE, SYS_CALL_EVENT_UART1, VIC2_UART1_INT, 1);

    // Setup UART2 as third-highest priority interrupt
    enable_vectored_interrupt(VIC2_BASE, SYS_CALL_EVENT_UART2, VIC2_UART2_INT, 2);
}

void init(kernel_context_t *ctx, task_descriptor_t *tds, scheduler_t *sch, event_handler_t *eh) {
    // reset UARTs (needs to be done as other groups leave it in weird state)
    REG(UART1_BASE, UART_LCRL_OFFSET) = 0xBF;
    REG(UART1_BASE, UART_LCRM_OFFSET) = 0x0;
    REG(UART1_BASE, UART_LCRH_OFFSET) = STP2_MASK | WLEN_MASK;

    REG(UART2_BASE, UART_LCRL_OFFSET) = 0x3;
    REG(UART2_BASE, UART_LCRM_OFFSET) = 0x0;
    REG(UART2_BASE, UART_LCRH_OFFSET) = 0x60;

    // reset 40-bit timer, our timebase
    REG(TIMER4_BASE, TIMER4_VAL_HIGH_OFFSET) = 0;
    REG(TIMER4_BASE, TIMER4_VAL_HIGH_OFFSET) = TIMER4_ENABLE_MASK;

    // reset timer3, our systick
    REG(TIMER3_BASE, CRTL_OFFSET) = 0;
    REG(TIMER3_BASE, LDR_OFFSET) = 10 * (TIMER_SLOW_CLOCK_PER_SEC / 1000); // Set systick to 10ms
    REG(TIMER3_BASE, CRTL_OFFSET) = ENABLE_MASK | MODE_MASK;

    // set up exception vectors
    volatile uint32_t *swi_exception_vector = (volatile uint32_t *)0x28;
    *swi_exception_vector = (uint32_t)kernel_entry; // swi entry

    volatile uint32_t *irq_exception_vector = (volatile uint32_t *)0x38;
    *irq_exception_vector = (uint32_t)irq_entry; // hw interrupt entry

    *((uint32_t *)MEM_IRQ_STACK_START) = 0; // Clear bottom of irq stack

    // set up irq stack
    __asm__ volatile ("mrs r0, cpsr                 @ Save cpsr into scratch register\n\t"
                      "bic r0, r0, #0x1F            @ Clear mode bits\n\t"
                      "orr r0, r0, #0x12            @ Set mode bits for irq mode\n\t"
                      "msr cpsr, r0                 @ Store scratch register into cpsr for irq mode\n\t"
                      "mov sp, %0                   @ Store irq stack start into stack pointer\n\t"
                      "bic r0, r0, #0x1F            @ Clear mode bits\n\t"
                      "orr r0, r0, #0x13            @ Set mode bits for svc mode\n\t"
                      "msr cpsr, r0                 @ Store scratch register into cpsr for svc mode\n\t"
                      :                             // No output operands
                      : "r" (MEM_IRQ_STACK_START)
                      : "r0");

    // init first task
    task_descriptor_init(tds, 0, 1, NULL, (void (*)(void))user_main);
    task_descriptor_init(tds + IDLE_TASK_TID, IDLE_TASK_TID,
                    28, tds, (void (*)(void))idle_main);

    // init priority queues
    scheduler_init(sch);
    scheduler_put(sch, tds);
    scheduler_put(sch, tds + IDLE_TASK_TID);

    event_handler_init(eh, sch);

    ctx->tds = tds;
    ctx->sch = sch;
    ctx->next_free_td = 2;  // assumes IDLE_TASK_TID is 1
    ctx->eh = eh;
    ctx->metrics.last_idle_time = 0;

    *((volatile bool *)(MEM_TASK_STACK_START(IDLE_TASK_TID) - IDLE_TASK_EXIT_OFFSET)) = false;
    *((volatile uint32_t *)(MEM_TASK_STACK_START(IDLE_TASK_TID) - IDLE_TASK_NON_IDLE_TIME_OFFSET)) = 0;

    setup_vectored_interrupts();
}

void cleanup(void) {
    REG(0x38, 0) = 0x8348;
    REG(VIC2_BASE, VIC_VEC_ADDR_OFFSET) = 0;
    REG(VIC2_BASE, VIC_VEC_CTRL_N_OFFSET(0)) = 0;
    REG(VIC2_BASE, VIC_INT_EN_OFFSET) = 0;
    REG(TIMER3_BASE, CRTL_OFFSET) = 0;
}

int main(void) {
    task_descriptor_t tds[TASK_DESCRIPTOR_MAX_TASKS];
    scheduler_t sch;
    event_handler_t eh;
    kernel_context_t ctx;
    kernel_request_t req;

    init(&ctx, tds, &sch, &eh);

    while (true) {
        task_descriptor_t *active_td = scheduler_get(ctx.sch);

        if (!active_td) {
            break;
        }

        if (active_td->tid == IDLE_TASK_TID) {
            update_idle_task(&ctx);
        }

        activate(active_td, &req);

        if (active_td->tid == IDLE_TASK_TID) {
            end_idle_task(&ctx);
        }

        if (handle(&ctx, active_td, &req)) {
            break;
        }
    }

    volatile uint32_t *non_idle_time = (uint32_t *)(MEM_TASK_STACK_START(IDLE_TASK_TID) - IDLE_TASK_NON_IDLE_TIME_OFFSET);
    bwprintf(COM2, "Non-idle time: %u/%u\n\r", *non_idle_time, clock());

    cleanup();

    return 0;
}
