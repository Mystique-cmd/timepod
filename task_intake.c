#include "task_intake.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *DOMAIN_NAMES[TIMEPOD_DOMAIN_UNKNOWN + 1] = {
    "The Portal",            /* 0 */
    "Benjamin's Game",       /* 1 */
    "The Factory",           /* 2 */
    "The Rabbit Hole",       /* 3 */
    "The Specter Spectacle", /* 4 */
    "The Matrix Manual",     /* 5 */
    "Unknown"                /* 6 */
};

const char *timepod_domain_name(TimepodDomain d) {
    if (d < 0 || d > TIMEPOD_DOMAIN_UNKNOWN) d = TIMEPOD_DOMAIN_UNKNOWN;
    return DOMAIN_NAMES[d];
}

/* ---------- helpers ---------- */

static void lowercase_inplace(char *buf, size_t n, const char *in) {
    if (!buf) return;
    size_t i = 0;
    while (i + 1 < n && in[i] != '\0') {
        char c = in[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        buf[i] = c;
        i++;
    }
    buf[i] = '\0';
}

static bool contains_any(const char *haystack, const char **needles, size_t count) {
    if (!haystack) return false;
    for (size_t i = 0; i < count; i++) {
        const char *kw = needles[i];
        if (kw && strstr(haystack, kw)) return true;
    }
    return false;
}

/* ---------- domain classifier ---------- */

TimepodDomain task_classify_rules(const char *text) {
    if (!text || !text[0]) return TIMEPOD_DOMAIN_UNKNOWN;

    char buf[512];
    lowercase_inplace(buf, sizeof(buf), text);

    /* The Portal — anything related to sleep */
    {
        const char *kw[] = {
            "sleep", "nap", "rest", "bed", "wake", "dream", "insomnia",
            "lie down", "snooze", "asleep", "night", "tired", "resting",
            "meditat", "breath", "wind down", "power down", "sleep schedule"
        };
        if (contains_any(buf, kw, sizeof(kw) / sizeof(kw[0]))) return TIMEPOD_DOMAIN_PORTAL;
    }

    /* Benjamin's Game — anything related to generating income */
    {
        const char *kw[] = {
            "income", "money", "revenue", "client", "invoice", "pay", "paid",
            "salary", "freelance", "business", "sales", "sell", "profit",
            "earn", "earning", "side hustle", "gig", "commission", "monetize",
            "ecommerce", "shop", "product", "price", "pricing", "pitch",
            "negotiat", "contract", "market", "invest", "dividend", "portfolio",
            "passive income", "affiliate", "sponsor", "upwork", "fiverr"
        };
        if (contains_any(buf, kw, sizeof(kw) / sizeof(kw[0]))) return TIMEPOD_DOMAIN_BENJAMINS_GAME;
    }

    /* The Factory — anything to do with studies */
    {
        const char *kw[] = {
            "study", "studies", "learn", "course", "class", "lecture", "homework",
            "assignment", "exam", "test", "quiz", "tutorial", "textbook",
            "school", "college", "university", "degree", "research paper",
            "essay", "read chapter", "review notes", "study session", "thesis",
            "seminar", "workshop", "lab report", "revision", "memorize",
            "curriculum", "grade", "gpa", "lesson", "syllabus"
        };
        if (contains_any(buf, kw, sizeof(kw) / sizeof(kw[0]))) return TIMEPOD_DOMAIN_FACTORY;
    }

    /* The Rabbit Hole — anything related to cybersecurity */
    {
        const char *kw[] = {
            "cyber", "security", "hack", "hacking", "exploit", "penetration",
            "pentest", "vulnerab", "malware", "ransomware", "phishing",
            "firewall", "encryption", "crypt", "cipher", "ctf", "capture the flag",
            "burp", "nmap", "wireshark", "metasploit", "sql injection", "xss",
            "osint", "threat", "breach", "zero day", "payload", "red team",
            "blue team", "forensic", "auth", "oauth", "token", "privacy",
            "vpn", "proxy", "tor", "network scan", "port scan", "privilege"
        };
        if (contains_any(buf, kw, sizeof(kw) / sizeof(kw[0]))) return TIMEPOD_DOMAIN_RABBIT_HOLE;
    }

    /* The Specter Spectacle — anything related to self improvement */
    {
        const char *kw[] = {
            "workout", "exercise", "gym", "fitness", "run", "running", "jog",
            "yoga", "meditat", "mindfulness", "habit", "discipline", "journal",
            "reading", "self improvement", "self-improvement", "goal", "morning",
            "routine", "diet", "nutrition", "meal prep", "health", "wellness",
            "read a book", "book", "affirm", "visualiz", "skill", "practice",
            "learn something", "gratitude", "stretch", "cold shower", "focus",
            "procrastinat", "motivat", "productivity", "deep work"
        };
        if (contains_any(buf, kw, sizeof(kw) / sizeof(kw[0]))) return TIMEPOD_DOMAIN_SPECTER;
    }

    /* The Matrix Manual — anything related to things against dogma */
    {
        const char *kw[] = {
            "dogma", "question", "philosophy", "critical thinking", "reason",
            "logic", "skeptic", "truth", "belief", "religion", "ideology",
            "propaganda", "media", "narrative", "paradigm", "worldview",
            "debate", "argument", "fallacy", "epistemology", "consciousness",
            "reality", "matrix", "free will", "determinism", "ethics", "morality",
            "unlearn", "challenge", "status quo", "norms", "convention"
        };
        if (contains_any(buf, kw, sizeof(kw) / sizeof(kw[0]))) return TIMEPOD_DOMAIN_MATRIX_MANUAL;
    }

    return TIMEPOD_DOMAIN_UNKNOWN;
}

/* ---------- time estimation (rules) ---------- */

static uint64_t minutes_uint64(uint64_t m) { return m * 60ULL; }

static bool parse_duration_hint(const char *buf, uint64_t *out_minutes) {
    /* Look for "<n> min", "<n> minutes", "<n> hour(s)", "<n> day(s)" patterns. */
    const char *p = buf;
    while ((p = strchr(p, ' ')) != NULL) {
        p++;
        while (*p == ' ') p++;
        if (isdigit((unsigned char)*p)) {
            char *end = NULL;
            long val = strtol(p, &end, 10);
            if (end && val > 0) {
                /* skip spaces */
                while (*end == ' ') end++;
                if (strncmp(end, "min", 3) == 0) {
                    *out_minutes = (uint64_t)val;
                    return true;
                }
                if (strncmp(end, "hour", 4) == 0) {
                    *out_minutes = (uint64_t)val * 60ULL;
                    return true;
                }
                if (strncmp(end, "day", 3) == 0) {
                    *out_minutes = (uint64_t)val * 60ULL * 24ULL;
                    return true;
                }
            }
        }
    }
    return false;
}

uint64_t task_estimate_rules(const char *text) {
    if (!text || !text[0]) return 30 * 60;

    char buf[512];
    lowercase_inplace(buf, sizeof(buf), text);

    /* Explicit duration hints win. */
    uint64_t hint_minutes = 0;
    if (parse_duration_hint(buf, &hint_minutes)) {
        return minutes_uint64(hint_minutes);
    }

    /* "quick" / "small" → short */
    {
        const char *kw[] = { "quick", "small", "short", "tiny", "fast", "brief",
                             "send a", "reply", "email", "call" };
        if (contains_any(buf, kw, sizeof(kw) / sizeof(kw[0]))) return minutes_uint64(15);
    }

    /* "big", "large", "all day", "long" → long */
    {
        const char *kw[] = { "all day", "long", "large", "big", "major",
                             "extensive", "full day", "hours" };
        if (contains_any(buf, kw, sizeof(kw) / sizeof(kw[0]))) return minutes_uint64(4 * 60);
    }

    /* Per-domain average estimates. */
    TimepodDomain d = task_classify_rules(text);
    switch (d) {
        case TIMEPOD_DOMAIN_PORTAL:
            return minutes_uint64(45);
        case TIMEPOD_DOMAIN_BENJAMINS_GAME:
            return minutes_uint64(90);
        case TIMEPOD_DOMAIN_FACTORY:
            return minutes_uint64(60);
        case TIMEPOD_DOMAIN_RABBIT_HOLE:
            return minutes_uint64(60);
        case TIMEPOD_DOMAIN_SPECTER:
            return minutes_uint64(40);
        case TIMEPOD_DOMAIN_MATRIX_MANUAL:
            return minutes_uint64(50);
        default:
            return minutes_uint64(30);
    }
}

/* ---------- deadline parsing ---------- */

static int64_t day_epoch(int y, int m, int d) {
    /* days since 1970-01-01 (proleptic Gregorian) */
    long long days = 0;
    /* years */
    for (int yy = 1970; yy < y; yy++) {
        int leap = (yy % 4 == 0 && (yy % 100 != 0 || yy % 400 == 0));
        days += leap ? 366 : 365;
    }
    /* months */
    static const int mdays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    for (int mm = 1; mm < m; mm++) {
        days += mdays[mm - 1];
        if (mm == 2) {
            int leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
            if (leap) days += 1;
        }
    }
    days += (d - 1);
    return (int64_t)days * 86400LL;
}

static int weekday_of(int64_t epoch) {
    /* 1970-01-01 was Thursday (4 if Mon=0). */
    long long d = (long long)(epoch / 86400LL);
    int wd = (int)((d + 4) % 7);
    if (wd < 0) wd += 7;
    return wd;
}

static int64_t parse_iso_date(const char *s) {
    int y = 0, m = 0, d = 0;
    if (sscanf(s, "%d-%d-%d", &y, &m, &d) == 3) {
        if (m >= 1 && m <= 12 && d >= 1 && d <= 31) return day_epoch(y, m, d);
    }
    return 0;
}

int64_t task_intake_parse_deadline(const char *text, int64_t now_epoch) {
    if (!text || !text[0]) return 0;

    char buf[512];
    lowercase_inplace(buf, sizeof(buf), text);

    /* today / tonight / today at ... */
    if (strstr(buf, "today")) return now_epoch + 20 * 3600;
    if (strstr(buf, "tonight")) return now_epoch + 12 * 3600;

    /* tomorrow */
    if (strstr(buf, "tomorrow")) return now_epoch + 24 * 3600;

    /* in N days/hours */
    {
        const char *p = buf;
        while ((p = strchr(p, ' ')) != NULL) {
            p++;
            while (*p == ' ') p++;
            if (isdigit((unsigned char)*p)) {
                char *end = NULL;
                long val = strtol(p, &end, 10);
                if (end && val > 0 && val < 1000) {
                    while (*end == ' ') end++;
                    if (strncmp(end, "day", 3) == 0)
                        return now_epoch + (int64_t)val * 86400LL;
                    if (strncmp(end, "hour", 4) == 0)
                        return now_epoch + (int64_t)val * 3600LL;
                    if (strncmp(end, "week", 4) == 0)
                        return now_epoch + (int64_t)val * 7LL * 86400LL;
                }
            }
        }
    }

    /* weekday names */
    static const char *wd_names[7] = { "monday", "tuesday", "wednesday", "thursday",
                                       "friday", "saturday", "sunday" };
    for (int i = 0; i < 7; i++) {
        if (strstr(buf, wd_names[i])) {
            int today_wd = weekday_of(now_epoch);
            int delta = (i - today_wd + 7) % 7;
            if (delta == 0) delta = 7; /* next occurrence */
            return now_epoch + (int64_t)delta * 86400LL;
        }
    }

    /* ISO date anywhere in text */
    {
        char *p = buf;
        while (*p) {
            if (isdigit((unsigned char)*p)) {
                int64_t t = parse_iso_date(p);
                if (t != 0) return t;
            }
            p++;
        }
    }

    return 0;
}

/* ---------- priority ---------- */

static int clamp_priority(int v) {
    if (v < 0) return 0;
    if (v > 100) return 100;
    return v;
}

void task_intake_reprioritize(TaskQueue *q, int64_t now_epoch) {
    if (!q) return;
    for (int i = 0; i < q->count; i++) {
        TaskItem *t = &q->items[i];
        int score = 0;

        /* Deadline proximity drives urgency. */
        if (t->deadline_epoch > 0) {
            int64_t remaining = t->deadline_epoch - now_epoch;
            if (remaining <= 0) {
                score += 60; /* overdue → very urgent */
            } else if (remaining <= 6 * 3600) {
                score += 50; /* within 6h */
            } else if (remaining <= 24 * 3600) {
                score += 40; /* within a day */
            } else if (remaining <= 3 * 86400) {
                score += 30; /* within 3 days */
            } else if (remaining <= 7 * 86400) {
                score += 20; /* within a week */
            } else {
                score += 10;
            }
        }

        /* Estimated duration adds weight (longer tasks scheduled earlier). */
        uint64_t est_min = (t->estimated_seconds + 59) / 60;
        if (est_min >= 240) score += 20;
        else if (est_min >= 120) score += 15;
        else if (est_min >= 60) score += 10;
        else if (est_min >= 30) score += 5;

        t->priority_score = clamp_priority(score);

        if (score >= 50) t->priority = TIMEPOD_PRIORITY_URGENT;
        else if (score >= 30) t->priority = TIMEPOD_PRIORITY_HIGH;
        else if (score >= 15) t->priority = TIMEPOD_PRIORITY_MEDIUM;
        else t->priority = TIMEPOD_PRIORITY_LOW;
    }
}

static int cmp_tasks(const void *a, const void *b) {
    const TaskItem *ta = (const TaskItem *)a;
    const TaskItem *tb = (const TaskItem *)b;
    /* pending first */
    if (ta->done != tb->done) return ta->done ? 1 : -1;
    /* then by priority score desc */
    if (tb->priority_score != ta->priority_score)
        return tb->priority_score - ta->priority_score;
    /* then by estimated time desc (longest first) */
    if (tb->estimated_seconds != ta->estimated_seconds)
        return tb->estimated_seconds > ta->estimated_seconds ? 1 : -1;
    return strcmp(ta->text, tb->text);
}

void task_intake_sort_by_priority(TaskQueue *q) {
    if (!q || q->count < 2) return;
    qsort(q->items, (size_t)q->count, sizeof(TaskItem), cmp_tasks);
}

/* ---------- queue ops ---------- */

static void init_task(TaskItem *t) {
    memset(t, 0, sizeof(*t));
    t->domain = TIMEPOD_DOMAIN_UNKNOWN;
    t->priority = TIMEPOD_PRIORITY_MEDIUM;
    t->estimated_seconds = 30 * 60;
}

int task_intake_add(TaskQueue *q, const char *text) {
    if (!q || !text) return -1;
    if (q->count >= TIMEPOD_MAX_TASKS) return -1;

    /* trim */
    const char *s = text;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' || s[len-1] == '\n' || s[len-1] == '\r')) len--;
    if (len == 0) return -1;

    TaskItem *t = &q->items[q->count];
    init_task(t);
    size_t n = len < (sizeof(t->text) - 1) ? len : (sizeof(t->text) - 1);
    memcpy(t->text, s, n);
    t->text[n] = '\0';

    /* Rule-based baseline immediately. */
    t->domain = task_classify_rules(t->text);
    t->estimated_seconds = task_estimate_rules(t->text);

    q->count++;
    return q->count - 1;
}

bool task_intake_remove(TaskQueue *q, int index) {
    if (!q || index < 0 || index >= q->count) return false;
    if (index != q->count - 1) {
        memmove(&q->items[index], &q->items[index + 1],
                (size_t)(q->count - index - 1) * sizeof(TaskItem));
    }
    q->count--;
    return true;
}

bool task_intake_set_done(TaskQueue *q, int index, bool done) {
    if (!q || index < 0 || index >= q->count) return false;
    q->items[index].done = done;
    return true;
}

bool task_intake_apply_ai(TaskQueue *q, int index,
                          TimepodDomain domain,
                          uint64_t estimated_seconds,
                          const char *deadline_text) {
    if (!q || index < 0 || index >= q->count) return false;
    TaskItem *t = &q->items[index];
    if (domain >= TIMEPOD_DOMAIN_PORTAL && domain <= TIMEPOD_DOMAIN_MATRIX_MANUAL)
        t->domain = domain;
    if (estimated_seconds > 0)
        t->estimated_seconds = estimated_seconds;
    if (deadline_text && deadline_text[0]) {
        int64_t de = task_intake_parse_deadline(deadline_text, (int64_t)time(NULL));
        if (de > 0) t->deadline_epoch = de;
    }
    t->ai_analyzed = true;
    return true;
}

