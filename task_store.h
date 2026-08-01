#ifndef TIMEPOD_TASK_STORE_H
#define TIMEPOD_TASK_STORE_H

#include <stdbool.h>

#include "task_intake.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TIMEPOD_TASK_STORE_MAGIC 0x54534b53ULL /* 'TSKS' */

/* Persist the full task queue to disk (timepod_tasks.bin). */
bool task_store_save(const TaskQueue *q);

/* Load tasks from disk into q (replacing contents). Returns true if any loaded. */
bool task_store_load(TaskQueue *q);

/* Clear the persisted task file. */
void task_store_clear(void);

#ifdef __cplusplus
}
#endif

#endif

