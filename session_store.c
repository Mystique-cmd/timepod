#include "session_store.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stddef.h>


static void storage_path(char out_path[512]) {
    snprintf(out_path, 512, "./timepod_active_session.bin");
}

static void clamp_description(char out[256], const char *in) {
    if (!out) return;
    out[0] = '\0';
    if (!in) return;
    /* Best effort: truncate */
    strncpy(out, in, 255);
    out[255] = '\0';
}

SessionMode classify_mode_from_description(const char *desc) {
    if (!desc || !desc[0]) return SESSION_MODE_FLEXIBLE;

    /* Rule-based keyword classifier (MVP). */
    const char *s = desc;

    /* Lowercase copy for case-insensitive matching. */
    char buf[512];
    size_t n = 0;
    while (n < sizeof(buf) - 1 && s[n] != '\0') n++;

    for (size_t i = 0; i < n; i++) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        buf[i] = c;
    }
    buf[n] = '\0';

    const char *rigid_keywords[] = {
        "deadline", "due", "schedule", "block", "timeslot", "commitment",
        "meeting", "appointment", "fixed", "plan", "planned", "calendar",
        "reminder", "allocated", "allocation", "time block", "hours", "hour"
    };

    for (size_t i = 0; i < sizeof(rigid_keywords) / sizeof(rigid_keywords[0]); i++) {
        const char *kw = rigid_keywords[i];
        if (kw && strstr(buf, kw)) return SESSION_MODE_RIGID;
    }

    const char *flex_keywords[] = {
        "flexible", "adapt", "canvas", "emergent", "priority", "context",
        "iterate", "explore", "flow", "unsure", "change"
    };

    for (size_t i = 0; i < sizeof(flex_keywords) / sizeof(flex_keywords[0]); i++) {
        const char *kw = flex_keywords[i];
        if (kw && strstr(buf, kw)) return SESSION_MODE_FLEXIBLE;
    }

    /* Default: flexible. */
    return SESSION_MODE_FLEXIBLE;
}

static void active_session_record_from_state(ActiveSessionRecord *rec, const UiState *st) {
    memset(rec, 0, sizeof(*rec));
    rec->magic = 0x54504f44ULL; /* 'T POD' */

    if (!st) return;

    rec->has_session = st->has_session ? true : false;
    rec->paused = st->paused ? true : false;

    rec->active_domain_idx = st->active_domain_idx;
    rec->seconds_total = st->seconds_total;
    rec->seconds_left = st->seconds_left;

    rec->mode = st->session_mode;
    /* Preserve exact description if provided; otherwise keep empty. */
    rec->description[0] = '\0';
    strncpy(rec->description, st->description, sizeof(rec->description)-1);
    rec->description[sizeof(rec->description)-1] = '\0';


    /* Ensure we have monotonic-like continuity via wall-clock timestamps.
       Because UiState currently does not store these fields, we keep
       existing values by loading the current record first. */
    char path[512];
    storage_path(path);

    FILE *fp = fopen(path, "rb");
    if (fp) {
        ActiveSessionRecord prev;
        memset(&prev, 0, sizeof(prev));
        size_t n = fread(&prev, sizeof(prev), 1, fp);
        fclose(fp);
        if (n == 1 && prev.magic == 0x54504f44ULL) {
            rec->session_start_time_t = prev.session_start_time_t;
            rec->pause_start_time_t = prev.pause_start_time_t;
            rec->paused_seconds = prev.paused_seconds;
            rec->mode = prev.mode;
            strncpy(rec->description, prev.description, sizeof(rec->description)-1);
        }
    }

    /* If this is a brand new session (no persisted timestamps), initialize them. */
    if (rec->has_session) {
        time_t now = time(NULL);
        if (rec->session_start_time_t == 0) {
            rec->session_start_time_t = (int64_t)now;
        }
        if (rec->paused) {
            if (rec->pause_start_time_t == 0) {
                rec->pause_start_time_t = (int64_t)now;
            }
        } else {
            rec->pause_start_time_t = 0;
        }
        if (!rec->paused) {
            rec->pause_start_time_t = 0;
            rec->paused_seconds = 0;
        }

    }
}

static void state_from_active_session_record(UiState *st, const ActiveSessionRecord *rec) {
    if (!st || !rec) return;
    memset(st, 0, sizeof(*st));

    st->has_session = rec->has_session;
    st->paused = rec->paused;
    st->active_domain_idx = rec->active_domain_idx;

    st->seconds_left = rec->seconds_left;
    st->seconds_total = rec->seconds_total;

    st->session_mode = rec->mode;
    strncpy(st->description, rec->description, sizeof(st->description)-1);
    st->description[sizeof(st->description)-1] = '\0';

    st->session_start_time_t = rec->session_start_time_t;
    st->pause_start_time_t = rec->pause_start_time_t;
    st->paused_seconds = rec->paused_seconds;

    st->day = (DayRecord){0};
}

bool session_store_load_active(UiState *st) {
    if (!st) return false;

    char path[512];
    storage_path(path);

    FILE *fp = fopen(path, "rb");
    if (!fp) return false;

    ActiveSessionRecord rec;
    memset(&rec, 0, sizeof(rec));

    size_t n = fread(&rec, sizeof(rec), 1, fp);
    fclose(fp);
    if (n != 1) return false;

    if (rec.magic != 0x54504f44ULL) return false;

    state_from_active_session_record(st, &rec);
    return st->has_session;
}

bool session_store_save_active(const UiState *st) {
    if (!st) return false;

    char path[512];
    storage_path(path);

    ActiveSessionRecord rec;
    active_session_record_from_state(&rec, st);

    /* Apply wall-clock continuity: recompute seconds_left based on timestamps if present.
       Current MVP doesn't yet store start time in UiState, so we keep seconds_left as-is.
       Later change will fully resume. */

    FILE *fp = fopen(path, "wb");
    if (!fp) return false;

    fwrite(&rec, sizeof(rec), 1, fp);
    fclose(fp);

    return true;
}

void session_store_clear_active(UiState *st) {
    (void)st;
    char path[512];
    storage_path(path);
    FILE *fp = fopen(path, "wb");
    if (fp) {
        /* Write zero bytes header to invalidate magic */
        fclose(fp);
    }
}

