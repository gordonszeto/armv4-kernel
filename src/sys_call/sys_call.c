#include <bwio.h>
#include <sys_call.h>
#include <string.h>

#include <internal/sys_codes.h>

int Create(int priority, void (*code) ()) {
    register int ret __asm__ ("r0");
    SWI(SYS_CODE_CREATE);
    return ret;
}

int MyTid() {
    register int ret __asm__ ("r0");
    SWI(SYS_CODE_MYTID);
    return ret;
}

int MyParentTid() {
    register int ret __asm__ ("r0");
    SWI(SYS_CODE_PARENTTID);
    return ret;
}

void Pass() {
    SWI(SYS_CODE_PASS);
}

void Exit() {
    SWI(SYS_CODE_EXIT);
}

int Send(uint8_t tid, void *msg, size_t msg_len, void *rep, size_t rep_len) {
    register int ret __asm__ ("r0");
    volatile send_params_t params = {tid, msg, msg_len, rep, rep_len};
    __asm__ volatile ("mov r0, %0\n\t"
                      :
                      : "r" (&params)
                      : "r0");
    SWI(SYS_CODE_SEND);
    return ret;
}

int Receive(uint8_t *tid, void *msg, size_t msg_len) {
    register int ret __asm__ ("r0");
    SWI(SYS_CODE_RECEIVE);
    return ret;
}

int Reply(uint8_t tid, void *rep, size_t rep_len) {
    register int ret __asm__ ("r0");
    SWI(SYS_CODE_REPLY);
    return ret;
}
int AwaitEvent(int eventid) {
    register int ret __asm__ ("r0");
    SWI(SYS_CODE_AWAITEVENT);
    return ret;
}

void panic(char *msg) {
    SWI(SYS_CODE_PANIC);
}

void Quit(void) {
    SWI(SYS_CODE_QUIT);
}
