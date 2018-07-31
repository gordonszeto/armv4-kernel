#ifndef SYS_CODES_H_INCLUDED_
#define SYS_CODES_H_INCLUDED_

#define SWI(code) __asm__ volatile ("swi %0" : : "I" (code))

typedef enum {
    SYS_CODE_EXIT,
    SYS_CODE_PASS,
    SYS_CODE_CREATE,
    SYS_CODE_MYTID,
    SYS_CODE_PARENTTID,
    SYS_CODE_SEND,
    SYS_CODE_RECEIVE,
    SYS_CODE_REPLY,
    SYS_CODE_AWAITEVENT,
    SYS_CODE_PANIC,
    SYS_CODE_QUIT
} sys_code_t;

typedef struct {
    uint8_t tid;
    void *msg;
    size_t msg_len;
    void *rep;
    size_t rep_len;
} send_params_t;

#endif // SYS_CODES_H_INCLUDED_
