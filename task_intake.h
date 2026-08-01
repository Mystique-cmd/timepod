#ifndef TIMEPOD_TASK_INTAKE_H
#define TIMEPOD_TASK_INTAKE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIMEPOD_MAX_TASKS 64
#define TIMEPOD_TASK_TEXT_MAX 256
#define TIMEPOD_DOMAIN_NAME_MAX 48

/* The six TimePod domains. */
typedef enum {
    TIMEPOD_DOMAIN_PORTAL = 0,          /* The Portal       — sleep */
    TIMEPOD_DOMAIN_BENJAMINS_GAME = 1,  /* Benjamin's Game  — generating income */
    TIMEPOD_DOMAIN_FACTORY = 2,         /* The Factory      — studies */
    TIMEPOD_DOMAIN_RABBIT_HOLE = 3,     /* The Rabbit Hole  — cybersecurity */
    TIMEPOD_DOMAIN_SPECTER = 4,         /* The Specter Spectacle — self improvement */
    TIMEPOD_DOMAIN_MATRIX_MANUAL = 5,   /* The Matrix Manual — against dogma */
    TIMEPOD_DOMAIN_UNKNOWN = 6
} TimepodDomain;

typedef enum {
    TIMEPOD_PRIORITY_LOW = 0,
    TIMEPOD_PRIORITY_MEDIUM = 1,
    TIMEPOD_PRIORITY_HIGH = 2,
    TIMEPOD_PRIORITY_URGENT = 3
} TimepodPriority;

typedef struct {
    char text[TIMEPOD_TASK_TEXT_MAX];
    TimepodDomain domain;
    /* estimated completion time for an average human, in seconds */
    uint64_t estimated_seconds;
    /* deadline as unix epoch; 0 = no deadline */
    int64_t deadline_epoch;
    /* 0..100 combined urgency score used for sorting */
    int priority_score;
    TimepodPriority priority;
    bool done;
    bool ai_analyzed;
} TaskItem;

typedef struct {
    TaskItem items[TIMEPOD_MAX_TASKS];
    int count;
} TaskQueue;

/* Resolve a domain enum to its human-readable name. */
const char *timepod_domain_name(TimepodDomain d);

/* Add a task (text only). Uses rule-based classifier immediately. */
int task_intake_add(TaskQueue *q, const char *text);

/* Remove a task at index; returns true on success. */
bool task_intake_remove(TaskQueue *q, int index);

/* Mark task done / not done. */
bool task_intake_set_done(TaskQueue *q, int index, bool done);

/* Update a single task's AI-derived fields (domain/estimate/deadline). */
bool task_intake_apply_ai(TaskQueue *q, int index,
                          TimepodDomain domain,
                          uint64_t estimated_seconds,
                          const char *deadline_text);

/* Recompute priorities for all tasks (based on time + deadline). */
void task_intake_reprioritize(TaskQueue *q, int64_t now_epoch);

/* Sort the queue by priority score descending (highest urgency first). */
void task_intake_sort_by_priority(TaskQueue *q);

/* Parse a human deadline phrase like "tomorrow", "friday", "2025-06-01", "3 days".
 * Returns epoch seconds, or 0 if unparsable. */
int64_t task_intake_parse_deadline(const char *text, int64_t now_epoch);

/* Rule-based keyword classifier → one of the 6 domains. */
TimepodDomain task_classify_rules(const char *text);

/* Rule-based time estimator (average human), in seconds. */
uint64_t task_estimate_rules(const char *text);

#ifdef __cplusplus
}
#endif

#endif

