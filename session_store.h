#ifndef TIMEPOD_SESSION_STORE_H
#define TIMEPOD_SESSION_STORE_H

#include <stdbool.h>
#include <stdint.h>

#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SESSION_MODE_FLEXIBLE = 0,
    SESSION_MODE_RIGID = 1,
} SessionMode;

/* Persisted active-session state (portable POD). */
typedef struct {
    /* magic for basic corruption detection */
    uint64_t magic;

    bool has_session;

    /* whether timer is paused */
    bool paused;

    int active_domain_idx;

    /* persisted countdown */
    uint64_t seconds_total;
    uint64_t seconds_left;

    /* wall-clock timestamps for resume continuity */
    /* when the session started (time_t) */
    int64_t session_start_time_t;

    /* when paused (time_t); 0 means not currently paused */
    int64_t pause_start_time_t;

    /* total paused duration in seconds */
    uint64_t paused_seconds;

    /* classification */
    uint32_t mode; /* SessionMode */

    /* free-text description (optional, best-effort) */
    char description[256];
} ActiveSessionRecord;

bool session_store_load_active(UiState *st);
bool session_store_save_active(const UiState *st);
void session_store_clear_active(UiState *st);

SessionMode classify_mode_from_description(const char *desc);

#ifdef __cplusplus
}
#endif

#endif

