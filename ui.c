#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

/*
 * Qt GUI build keeps only the persistence + completion logic.
 * All legacy TUI rendering functions are stubbed out.
 */

static void ui_get_local_day(int *y, int *m, int *d) {
    time_t t = time(NULL);
    struct tm tmv;
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(__APPLE__)
    localtime_r(&t, &tmv);
#else
    struct tm *ptm = localtime(&t);
    if (ptm) tmv = *ptm;
    else memset(&tmv, 0, sizeof(tmv));
#endif
    *y = tmv.tm_year + 1900;
    *m = tmv.tm_mon + 1;
    *d = tmv.tm_mday;
}

static void ui_storage_path(char out_path[512]) {
    /* simple persistent file next to executable cwd */
    snprintf(out_path, 512, "./timepod_days.bin");
}

void ui_load_day_record(UiState *st) {
    memset(st, 0, sizeof(*st));
    ui_get_local_day(&st->day.year, &st->day.month, &st->day.day);

    for (int i = 0; i < TIMEPOD_MAX_DOMAINS; i++) st->day.completed[i] = false;

    char path[512];
    ui_storage_path(path);

    FILE *fp = fopen(path, "rb");
    if (!fp) return;

    while (1) {
        DayRecord rec;
        size_t n = fread(&rec, sizeof(rec), 1, fp);
        if (n != 1) break;
        if (rec.year == st->day.year && rec.month == st->day.month && rec.day == st->day.day) {
            st->day = rec;
            break;
        }
    }

    fclose(fp);
}

static void ui_save_day_record(const UiState *st) {
    char path[512];
    ui_storage_path(path);

    FILE *fp = fopen(path, "rb");
    DayRecord records[512];
    size_t count = 0;

    if (fp) {
        while (count < 512) {
            DayRecord rec;
            size_t n = fread(&rec, sizeof(rec), 1, fp);
            if (n != 1) break;
            records[count++] = rec;
        }
        fclose(fp);
    }

    bool found = false;
    for (size_t i = 0; i < count; i++) {
        if (records[i].year == st->day.year && records[i].month == st->day.month && records[i].day == st->day.day) {
            records[i] = st->day;
            found = true;
            break;
        }
    }

    if (!found && count < 512) {
        records[count++] = st->day;
    }

    fp = fopen(path, "wb");
    if (!fp) return;

    for (size_t i = 0; i < count; i++) {
        fwrite(&records[i], sizeof(records[i]), 1, fp);
    }
    fclose(fp);
}

void ui_mark_domain_completed_today(UiState *st, int domain_idx) {
    if (domain_idx < 0 || domain_idx >= TIMEPOD_MAX_DOMAINS) return;
    if (!st->day.completed[domain_idx]) {
        st->day.completed[domain_idx] = true;
        ui_save_day_record(st);
    }
}

void ui_refresh_layout(UiState *st) {
    (void)st;
    /* TUI removed */
}

void ui_draw_frame(const UiState *st, const char *domain_name_if_active) {
    (void)st;
    (void)domain_name_if_active;
    /* TUI removed */
}

void ui_update_session(const UiState *st, const char *domain_name_if_active) {
    (void)st;
    (void)domain_name_if_active;
    /* TUI removed */
}

void ui_update_calendar(const UiState *st) {
    (void)st;
    /* TUI removed */
}

