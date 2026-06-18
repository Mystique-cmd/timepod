#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

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
    /* Persist next to executable by using current working directory. */
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

static void ui_color_begin(void) {
    /* cyan */
    printf("\033[36m");
}

static void ui_color_reset(void) {
    printf("\033[0m");
}

static void ui_cursor_move(int row_1_based, int col_1_based) {
    printf("\033[%d;%dH", row_1_based, col_1_based);
}

static void ui_print_progress_bar_pct(int pct) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;

    int barw = 50;
    int filled = (pct * barw) / 100;
    printf(" Progress: [");
    for (int i = 0; i < barw; i++) putchar(i < filled ? '=' : ' ');
    printf("] %3d%%\n", pct);
}

static void ui_print_active_block(const UiState *st, const char *domain_name_if_active) {
    const char *domain = st->has_session
        ? (domain_name_if_active ? domain_name_if_active : "(none)")
        : "Idle";

    printf(" Domain : %-70.70s\n", domain);
    printf(" State  : %s\n", st->has_session ? (st->paused ? "PAUSED" : "RUNNING") : "NO SESSION");

    if (st->has_session) {
        char buf[9];
        timer_format_hms(st->seconds_left, buf);
        char buf2[9];
        timer_format_hms(st->seconds_total, buf2);
        printf(" Remaining: %s  (total %s)\n", buf, buf2);

        uint64_t done = (st->seconds_total > st->seconds_left) ? (st->seconds_total - st->seconds_left) : 0;
        uint64_t denom = (st->seconds_total == 0) ? 1 : st->seconds_total;
        int pct = (int)((done * 100ULL) / denom);
        ui_print_progress_bar_pct(pct);
    } else {
        printf(" Remaining: %-22s\n", "");
        printf(" Progress: [                                                  ]   0%%\n");
    }
}

void ui_draw_frame(const UiState *st, const char *domain_name_if_active) {
    printf("\033[2J\033[H");
    ui_color_begin();

    printf("+--------------------------------------------------------------------------------+\n");
    printf("|                                TIMEPOD // DESKTOP                             |\n");
    printf("+--------------------------------------------------------------------------------+\n\n");

    printf("[ACTIVE]\n");                    /* row 5 */
    printf(" Domain : %-70.70s\n", st->has_session ? (domain_name_if_active ? domain_name_if_active : "(none)") : "Idle"); /* row 6 */
    printf(" State  : %s\n", st->has_session ? (st->paused ? "PAUSED" : "RUNNING") : "NO SESSION"); /* row 7 */

    if (st->has_session) {
        char buf[9];
        timer_format_hms(st->seconds_left, buf);
        char buf2[9];
        timer_format_hms(st->seconds_total, buf2);
        printf(" Remaining: %s  (total %s)\n", buf, buf2); /* row 8 */

        uint64_t done = (st->seconds_total > st->seconds_left) ? (st->seconds_total - st->seconds_left) : 0;
        uint64_t denom = (st->seconds_total == 0) ? 1 : st->seconds_total;
        int pct = (int)((done * 100ULL) / denom);
        ui_print_progress_bar_pct(pct); /* row 9 */
    } else {
        printf(" Remaining: %-22s\n", "");
        printf(" Progress: [                                                  ]   0%%\n");
    }

    printf("\n[CONTROLS]  p=PAUSE  c=CONTINUE  q=QUIT\n\n");

    printf("[CALENDAR : %04d-%02d-%02d]\n", st->day.year, st->day.month, st->day.day); /* calendar starts at row 12 */
    printf(" Completed domains (ticks persist):\n\n");

    static const char *names[TIMEPOD_MAX_DOMAINS] = {
        "The Portal",
        "The Factory",
        "Benjamin's Game",
        "The Matrix Manual",
        "The Rabbit Hole",
        "Specter Spectacle"
    };

    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 2; c++) {
            int idx = r * 2 + c;
            if (idx >= TIMEPOD_MAX_DOMAINS) continue;
            bool ok = st->day.completed[idx];
            printf("| %c %-18.18s ", ok ? '*' : '.', names[idx]);
        }
        printf("|\n");
    }

    printf("+------------------------------------------------+\n");
    printf("| legend: ✓ completed today   . not completed  |\n");
    printf("+------------------------------------------------+\n");

    ui_color_reset();
    fflush(stdout);
}

void ui_update_session(const UiState *st, const char *domain_name_if_active) {
    ui_color_begin();

    /* rows 6-9 are dynamic */
    ui_cursor_move(6, 1);

    for (int i = 0; i < 4; i++) {
        printf("\033[K\n");
    }

    ui_cursor_move(6, 1);
    ui_print_active_block(st, domain_name_if_active);

    ui_color_reset();
    fflush(stdout);
}

void ui_update_calendar(const UiState *st) {
    ui_color_begin();

    /* redraw calendar block */
    ui_cursor_move(12, 1);
    printf("\033[J");

    printf("[CALENDAR : %04d-%02d-%02d]\n", st->day.year, st->day.month, st->day.day);
    printf(" Completed domains (ticks persist):\n\n");

    static const char *names[TIMEPOD_MAX_DOMAINS] = {
        "The Portal",
        "The Factory",
        "Benjamin's Game",
        "The Matrix Manual",
        "The Rabbit Hole",
        "Specter Spectacle"
    };

    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 2; c++) {
            int idx = r * 2 + c;
            if (idx >= TIMEPOD_MAX_DOMAINS) continue;
            bool ok = st->day.completed[idx];
            printf("| %c %-18.18s ", ok ? '*' : '.', names[idx]);
        }
        printf("|\n");
    }

    printf("+------------------------------------------------+\n");
    printf("| legend: ✓ completed today   . not completed  |\n");
    printf("+------------------------------------------------+\n");

    ui_color_reset();
    fflush(stdout);
}

