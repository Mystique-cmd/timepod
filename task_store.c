#include "task_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void storage_path(char out_path[512]) {
    snprintf(out_path, 512, "./timepod_tasks.bin");
}

bool task_store_save(const TaskQueue *q) {
    if (!q) return false;

    char path[512];
    storage_path(path);

    FILE *fp = fopen(path, "wb");
    if (!fp) return false;

    uint64_t magic = TIMEPOD_TASK_STORE_MAGIC;
    fwrite(&magic, sizeof(magic), 1, fp);
    fwrite(&q->count, sizeof(q->count), 1, fp);

    int n = q->count;
    if (n > TIMEPOD_MAX_TASKS) n = TIMEPOD_MAX_TASKS;
    for (int i = 0; i < n; i++) {
        fwrite(&q->items[i], sizeof(TaskItem), 1, fp);
    }

    fclose(fp);
    return true;
}

bool task_store_load(TaskQueue *q) {
    if (!q) return false;

    char path[512];
    storage_path(path);

    FILE *fp = fopen(path, "rb");
    if (!fp) return false;

    uint64_t magic = 0;
    size_t r = fread(&magic, sizeof(magic), 1, fp);
    if (r != 1 || magic != TIMEPOD_TASK_STORE_MAGIC) {
        fclose(fp);
        return false;
    }

    int count = 0;
    r = fread(&count, sizeof(count), 1, fp);
    if (r != 1) {
        fclose(fp);
        return false;
    }

    if (count < 0 || count > TIMEPOD_MAX_TASKS) {
        fclose(fp);
        return false;
    }

    memset(q, 0, sizeof(*q));
    for (int i = 0; i < count; i++) {
        r = fread(&q->items[i], sizeof(TaskItem), 1, fp);
        if (r != 1) {
            q->count = i;
            fclose(fp);
            return q->count > 0;
        }
        q->count = i + 1;
    }

    fclose(fp);
    return q->count > 0;
}

void task_store_clear(void) {
    char path[512];
    storage_path(path);
    FILE *fp = fopen(path, "wb");
    if (fp) fclose(fp);
}

