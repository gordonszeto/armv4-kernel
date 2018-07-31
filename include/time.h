#ifndef TIME_H_INCLUDED_
#define TIME_H_INCLUDED_

#include <int_types.h>

#define CLOCKS_PER_SEC (983040)

typedef uint64_t clock_t;

clock_t clock(void);

#endif // TIME_H_INCLUDED_
