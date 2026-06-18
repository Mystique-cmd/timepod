#ifndef TIMEPOD_TIMER_H
#define TIMEPOD_TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;
    uint64_t seconds_left; /* countdown state */
    uint64_t seconds_total; /* initial duration */
} DomainTimer;

/*
 * Initialize a DomainTimer with a duration expressed in hours.
 */
void timer_init_hours(DomainTimer *dt, const char *name, uint64_t hours);

/*
 * Get monotonic time in nanoseconds.
 */
uint64_t timer_now_ns(void);

/*
 * Run countdown for dt until it reaches 0.
 * Prints a periodic tick (every 1s).
 * Returns 1 when finished.
 */
int timer_run_blocking(DomainTimer *dt);

/*
 * Format remaining time as HH:MM:SS into buf.
 * buf must be at least 9 bytes.
 */
void timer_format_hms(uint64_t seconds, char buf[9]);

#ifdef __cplusplus
}
#endif

#endif


