#ifndef TIMEPOD_TIMER_NB_H
#define TIMEPOD_TIMER_NB_H

#include <stdint.h>
#include <stdbool.h>

#include "timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Non-blocking state machine helpers */

typedef struct {
    bool started;
    bool paused;
    uint64_t start_ns;
    uint64_t pause_accum_ns; /* total paused duration */
    uint64_t pause_started_ns;
    uint64_t last_reported_left;
} TimerNB;

void timer_nb_init(TimerNB *nb);

void timer_nb_start(TimerNB *nb, DomainTimer *dt);
void timer_nb_set_paused(TimerNB *nb, DomainTimer *dt, int paused);

/* advances timer by computing remaining based on time; does not sleep */
/* returns 1 if completed */
int timer_nb_step(TimerNB *nb, DomainTimer *dt);

#ifdef __cplusplus
}
#endif

#endif

