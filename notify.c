#include "notify.h"

#include <stdio.h>

void timepod_notify_domain_completed(const char *domain_name) {
    /* MVP: keep build/linking simple; print only when domain_name is set. */
    if (domain_name && domain_name[0]) {
        fprintf(stderr, "[timepod] DOMAIN COMPLETE: %s\n", domain_name);
    }
}

