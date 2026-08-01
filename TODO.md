# TimePod — AI Task Intake (Local Ollama)

## Goal
Let the system accept tasks one at a time, store them in a list, categorize them into the 6 TimePod domains, estimate average human completion time, and prioritize by time + deadline — powered by a **local Ollama** model, with rule-based offline fallback.

## Steps
1. [x] Create `task_intake.h/c` — task queue, categories, deadline parsing, priority scoring, rule-based fallback classifier/estimator, rendering
2. [x] Create `task_store.h/c` — persist/load task list to `timepod_tasks.bin`
3. [x] Create `qt_timepod/ai_client.h/cpp` — Ollama HTTP client (`/api/chat`, JSON format), env-configurable (`TIMEPOD_AI_MODEL`, `TIMEPOD_AI_BASE_URL`)
4. [x] Overhaul `qt_timepod/main.cpp` — add-one-task-at-a-time UI, task list panel (category + estimate + priority + deadline), AI analyze, select task to start timer
5. [x] Update `qt_timepod/qt_timepod.pro` — add QtNetwork + new sources
6. [x] Update root `Makefile` — add new C modules
7. [ ] Install/configure Ollama, pull a model (e.g., `llama3.2`) — **IN PROGRESS** (bundle download in background)
8. [x] Build Qt app (`qmake && make`) — verified 0 errors
9. [ ] Run and verify end-to-end with Ollama model

## Notes
- App works **offline** now via rule-based classifier/estimator fallback.
- Once Ollama runtime is installed and `llama3.2` pulled, the AI client automatically upgrades each task analysis (category + estimate + deadline) and prioritizes the list.
- Configure via env vars: `TIMEPOD_AI_BASE_URL` (default `http://localhost:11434`), `TIMEPOD_AI_MODEL` (default `llama3.2`).

