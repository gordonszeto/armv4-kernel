#include <time.h>

#include <int_types.h>
#include <ts7200.h>

clock_t clock(void) {
    volatile uint32_t *timer4_value_low_reg = (volatile uint32_t *)(TIMER4_BASE + TIMER4_VAL_LOW_OFFSET);
    volatile uint32_t *timer4_value_high_reg = (volatile uint32_t *)(TIMER4_BASE + TIMER4_VAL_HIGH_OFFSET);

    clock_t ret = *timer4_value_low_reg;
    ret |= (clock_t)(*timer4_value_high_reg & TIMER4_VAL_MASK) << 32;
    return ret;
}
