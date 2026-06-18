# TODO (Time Pod) — Display + Desktop UI

- [x] Implement persistent completion tracking (by day) for domain ticks

- [ ] Refactor timer to support pause/continue and non-blocking stepping
- [ ] Build terminal “desktop” renderer (split panels, hacker-style borders/colors via ANSI)
- [ ] Implement keyboard controls (p=pause, c=continue, q=quit) using POSIX termios + non-blocking stdin
- [ ] Wire main loop: redraw at ~5–10 FPS, update remaining time, update calendar grid, persist ticks on changes
- [ ] Test: compile, run, verify pause/resume, verify persistence across restarts

