#ifndef TIMEPOD_UI_H
#define TIMEPOD_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "timer.h"

#define TIMEPOD_MAX_DOMAINS 6

typedef struct {
    /* day in local time, YYYY-MM-DD */
    int year;
    int month; /* 1-12 */
    int day;   /* 1-31 */

    /* completion flags per domain for that day */
    bool completed[TIMEPOD_MAX_DOMAINS];
} DayRecord;

typedef struct {
    /* selected domain index [0..5] */
    int active_domain_idx;

    /* running state */
    bool has_session;
    bool paused;

    /* remaining seconds for active session */
    uint64_t seconds_left;
    uint64_t seconds_total;

    /* last known day record */
    DayRecord day;

    /* cached layout metrics (terminal-size aware rendering) */
    int term_rows;
    int term_cols;

    int content_width;     /* width inside the top/bottom borders */
    int progress_bar_w;    /* number of characters inside [....] */

    int header_row;        /* top border row (usually 1) */
    int active_block_row;  /* where ui_update_session writes */
    int calendar_row;      /* where ui_update_calendar writes */
} UiState;


void ui_load_day_record(UiState *st);
void ui_mark_domain_completed_today(UiState *st, int domain_idx);

void ui_refresh_layout(UiState *st);

void ui_draw_frame(const UiState *st, const char *domain_name_if_active);
void ui_update_session(const UiState *st, const char *domain_name_if_active);
void ui_update_calendar(const UiState *st);



#ifdef __cplusplus
}
#endif

#endif

