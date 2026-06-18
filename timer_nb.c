#include "timer_nb.h"

#include <string.h>

void timer_nb_init(TimerNB *nb) {
    if (!nb) return;
    memset(nb, 0, sizeof(*nb));
    nb->last_reported_left = (uint64_t)-1;
}

void timer_nb_start(TimerNB *nb, DomainTimer *dt) {
    if (!nb || !dt) return;
    uint64_t now = timer_now_ns();
    nb->started = true;
    nb->paused = false;
    nb->start_ns = now;
    nb->pause_accum_ns = 0;
    nb->pause_started_ns = 0;
    nb->last_reported_left = (uint64_t)-1;

    dt->seconds_left = dt->seconds_total;
}

void timer_nb_set_paused(TimerNB *nb, DomainTimer *dt, int paused) {
    if (!nb || !dt) return;
    if (!nb->started) return;

    if (!!paused == nb->paused) return;

    uint64_t now = timer_now_ns();
    if (paused) {
        nb->paused = true;
        nb->pause_started_ns = now;
    } else {
        /* unpause */
        if (nb->pause_started_ns != 0) {
            nb->pause_accum_ns += (now - nb->pause_started_ns);
        }
        nb->paused = false;
    }
}

int timer_nb_step(TimerNB *nb, DomainTimer *dt) {
    if (!nb || !dt) return 1;
    if (!nb->started) return 0;

    uint64_t now = timer_now_ns();
    uint64_t effective_now = now;

    if (nb->paused) {
        /* freeze at pause instant */
        if (nb->pause_started_ns != 0) {
            effective_now = nb->pause_started_ns;
        }
    }

    uint64_t elapsed_ns = 0;
    if (effective_now >= nb->start_ns) elapsed_ns = effective_now - nb->start_ns;

    /* subtract accumulated paused time */
    if (elapsed_ns > nb->pause_accum_ns) elapsed_ns -= nb->pause_accum_ns;
    else elapsed_ns = 0;

    uint64_t elapsed_seconds = elapsed_ns / 1000000000ULL;
    uint64_t new_left = (dt->seconds_total > elapsed_seconds) ? (dt->seconds_total - elapsed_seconds) : 0;
    dt->seconds_left = new_left;

    return dt->seconds_left == 0 ? 1 : 0;
}

