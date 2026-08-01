#include <iostream>
#include <string>
#include <unistd.h>

extern "C" {
#include "timer.h"
#include "timer_nb.h"
#include "ui.h"
}

/*
 * TUI is optional.
 * This repository snapshot may not contain io.h/terminal.h (TUI runtime),
 * so keep the TUI interactive loop behind a compile-time flag.
 */
#if defined(TIMEPOD_ENABLE_TUI)
extern "C" {
#include "io.h"
#include "terminal.h"
}
#endif

static void choose_domain_by_key(int key, DomainTimer *out_dt, int *out_idx) {
    /* legacy keyboard digits 1..6 */
    if (!out_dt || !out_idx) return;

    const char *names[6] = {
        "The Portal",
        "The Factory",
        "Benjamin's Game",
        "The Matrix Manual",
        "The Rabbit Hole",
        "Specter Spectacle"
    };
    const uint64_t hours[6] = {7, 4, 4, 1, 4, 4};

    if (key < '1' || key > '6') {
        *out_idx = -1;
        return;
    }
    int idx = key - '1';
    out_dt->name = names[idx];
    timer_init_hours(out_dt, names[idx], hours[idx]);
    *out_idx = idx;
}

int main() {
#if defined(TIMEPOD_ENABLE_TUI)
    io_clear_screen();

    TerminalRawMode trm{};
    terminal_enable_raw_mode(&trm);

    UiState st{};
    st.active_domain_idx = 0;
    st.has_session = false;
    st.paused = false;
    st.seconds_left = 0;
    st.seconds_total = 0;

    ui_load_day_record(&st);
    ui_refresh_layout(&st);
    ui_draw_frame(&st, "Idle");

    TimerNB nb{};
    timer_nb_init(&nb);

    DomainTimer dt{};

    bool running = true;
    while (running) {
        /* input */
        int key = terminal_read_key_nonblocking();
        if (key != -1) {
            if (key == 'q' || key == 'Q') {
                running = false;
            } else if (key == 'p' || key == 'P') {
                if (st.has_session) {
                    st.paused = !st.paused;
                    timer_nb_set_paused(&nb, &dt, st.paused ? 1 : 0);
                }
            } else if (key == 'c' || key == 'C') {
                if (st.has_session) {
                    st.paused = false;
                    timer_nb_set_paused(&nb, &dt, 0);
                }
            } else if (key >= '1' && key <= '6') {
                int idx = -1;
                choose_domain_by_key(key, &dt, &idx);
                if (idx >= 0) {
                    st.active_domain_idx = idx;
                    st.has_session = true;
                    st.paused = false;
                    st.active_domain_idx = idx;
                    timer_nb_start(&nb, &dt);
                    st.seconds_left = dt.seconds_left;
                    st.seconds_total = dt.seconds_total;
                }
            }
        }

        /* tick */
        if (st.has_session) {
            int completed = timer_nb_step(&nb, &dt);
            st.seconds_left = dt.seconds_left;
            st.seconds_total = dt.seconds_total;

            if (completed) {
                ui_mark_domain_completed_today(&st, st.active_domain_idx);
                st.has_session = false;
                st.paused = false;
                st.seconds_left = 0;
                st.seconds_total = 0;
            }
        }

        /* render */
        if (st.has_session) {
            ui_update_session(&st, dt.name);
        } else {
            ui_update_session(&st, "Idle");
        }

        /* fps */
        usleep(100000); /* 100ms */
    }

    terminal_disable_raw_mode(&trm);
    io_clear_screen();
    std::cout << "Exiting.\n";
    return 0;
#else
    /* Minimal non-TUI build: just link successfully. */
    UiState st{};
    st.active_domain_idx = 0;
    st.has_session = false;
    st.paused = false;
    st.seconds_left = 0;
    st.seconds_total = 0;

    ui_load_day_record(&st);
    std::cout << "timepod (non-TUI build): define TIMEPOD_ENABLE_TUI to enable interactive mode.\n";
    return 0;
#endif
}

