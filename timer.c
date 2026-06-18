#include "timer.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

void timer_init_hours(DomainTimer *dt, const char *name, uint64_t hours) {


    uint64_t total = hours * 60ULL * 60ULL;
    dt->name = name;
    dt->seconds_total = total;
    dt->seconds_left = total;
}

uint64_t timer_now_ns(void) {
    return (uint64_t)time(NULL) * 1000000000ULL;
}



void timer_format_hms(uint64_t seconds, char buf[9]) {
    uint64_t h = seconds / 3600ULL;
    uint64_t m = (seconds % 3600ULL) / 60ULL;
    uint64_t s = seconds % 60ULL;

    /* Clamp to 99 hours for display width; keep logic based on full seconds. */
    if (h > 99ULL) h = 99ULL;
    /* HH:MM:SS */
    snprintf(buf, 9, "%02llu:%02llu:%02llu",
             (unsigned long long)h,
             (unsigned long long)m,
             (unsigned long long)s);
}

int timer_run_blocking(DomainTimer *dt) {
    uint64_t start_ns = timer_now_ns();
    uint64_t last_sec_reported = (uint64_t)-1;

    while (dt->seconds_left > 0) {
        sleep(1); /* 1s granularity to avoid non-portable APIs */

        uint64_t now_ns = timer_now_ns();

        uint64_t elapsed_seconds = (now_ns - start_ns) / 1000000000ULL;
        uint64_t new_left = (dt->seconds_total > elapsed_seconds) ? (dt->seconds_total - elapsed_seconds) : 0;
        dt->seconds_left = new_left;

        if (dt->seconds_left != last_sec_reported) {
            last_sec_reported = dt->seconds_left;
            char buf[9];
            timer_format_hms(dt->seconds_left, buf);
            printf("\r[%s] Remaining: %s", dt->name, buf);
            fflush(stdout);
        }
    }


    printf("\r[%s] Remaining: 00:00:00\n", dt->name);
    printf("*** DOMAIN COMPLETE: %s ***\n", dt->name);
    fflush(stdout);

    return 1;
}

