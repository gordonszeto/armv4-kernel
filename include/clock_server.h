#ifndef CLOCK_SERVER_H_INCLUDED_
#define CLOCK_SERVER_H_INCLUDED_

#define CLOCK_SERVER_NAME "clock_server"
#define CLOCK_SERVER_MS_PER_TICK 10

#include <int_types.h>

/* Clock Server wrapper functions */

// Blocks the caller until the given number of ticks has passed. There is no
// guarantee how long after that caller will be scheduled. tid arg is tid of
// the clock server.
// Return values:
// 0    success
// -1   clock server tid is invalid
// -2   delay was 0 or negative
int Delay(int tid, int ticks);

// Returns the number of ticks since the clock server was created and
// initialized. tid arg is tid of the clock server.
// Return values:
// >-1  ticks since clock server started
// -1   clock server tid is invalid
int Time(int tid);

// Blocks caller until ticks since clock server initialization is greater than
// the given number of ticks. There is no guarantee how long after that caller
// will be scheduled. tid arg is tid of clock server.
// Return values:
// 0    success
// -1   clock server tid is invalid
// -2   delay was 0 or negative
int DelayUntil(int tid, int ticks);

/* Clock Server functions */

// Entry point for a clock server task. Only one clock server should be run at
// any time.
void clock_server_main(void);

// Shuts down clock server, will also shut down respective notifier
void clock_server_exit(uint8_t clock_server_tid);

/* Clock Server notifier functions */

// entry point for a clock notifier task
void clock_server_notifier_main(void);

#endif // CLOCK_SERVER_H_INCLUDED_

