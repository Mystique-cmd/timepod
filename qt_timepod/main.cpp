#include <QApplication>
#include <QCoreApplication>

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QFont>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QString>
#include <QStringList>
#include <QList>
#include <algorithm>
#include <cstring>
#include <ctime>

extern "C" {
#include "timer.h"
#include "timer_nb.h"
#include "ui.h"
#include "notify.h"
#include "task_intake.h"
#include "task_store.h"
}

#include "ai_client.h"

/* UI layout constants */
static const int kMargin = 20;
static const int kHeaderH = 40;
static const int kBottomReserve = 90;

static QString prioName(TimepodPriority p) {
    switch (p) {
        case TIMEPOD_PRIORITY_URGENT: return QStringLiteral("URGENT");
        case TIMEPOD_PRIORITY_HIGH:   return QStringLiteral("HIGH");
        case TIMEPOD_PRIORITY_MEDIUM: return QStringLiteral("MEDIUM");
        default:                      return QStringLiteral("LOW");
    }
}

static QString fmtDuration(uint64_t seconds) {
    uint64_t h = seconds / 3600;
    uint64_t m = (seconds % 3600) / 60;
    if (h > 0) return QStringLiteral("%1h %2m").arg(h).arg(m);
    if (m > 0) return QStringLiteral("%1m").arg(m);
    return QStringLiteral("<1m");
}

class TimePodWidget : public QWidget {
    Q_OBJECT
public:
    TimePodWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setFocusPolicy(Qt::StrongFocus);
        setMinimumSize(820, 520);

        std::memset(&st_, 0, sizeof(st_));
        st_.active_domain_idx = 0;
        st_.has_session = false;
        st_.paused = false;

        timer_nb_init(&nb_);
        ui_load_day_record(&st_);

        /* Restore persisted active session */
        ui_session_load_active(&st_);
        if (st_.has_session) {
            dt_ = {};
            timer_init_hours(&dt_, "Session", 0);
            dt_.seconds_total = st_.seconds_total;
            dt_.seconds_left = st_.seconds_left;
            timer_nb_start(&nb_, &dt_);
            timer_nb_set_paused(&nb_, &dt_, st_.paused ? 1 : 0);
            dt_.seconds_left = st_.seconds_left;
            dt_.seconds_total = st_.seconds_total;
            st_.seconds_left = dt_.seconds_left;
            st_.seconds_total = dt_.seconds_total;
            dt_initialized_ = true;
        }

        /* Task list */
        task_store_load(&tasks_);
        task_intake_reprioritize(&tasks_, (int64_t)::time(nullptr));
        task_intake_sort_by_priority(&tasks_);

        /* AI client */
        ai_ = new AiClient(this);
        connect(ai_, &AiClient::analysisFinished,
                this, &TimePodWidget::onAiAnalysisFinished);

        /* Task description input (single task at a time) */
        task_input_ = new QLineEdit(this);
        task_input_->setPlaceholderText("Add a task and press Enter...");
        task_input_->setStyleSheet(
            "QLineEdit {"
            "  background-color: #1a1a1a;"
            "  color: #ffffff;"
            "  border: 1px solid #555;"
            "  border-radius: 4px;"
            "  padding: 4px 8px;"
            "  font-size: 11pt;"
            "}"
            "QLineEdit:focus {"
            "  border-color: #00dcff;"
            "}"
        );
        connect(task_input_, &QLineEdit::returnPressed, this, &TimePodWidget::onTaskSubmit);

        /* Pending AI analysis queue (we analyze one task at a time) */
        pending_ai_ = false;
        aiReady_ = false;

        tick_timer_ = new QTimer(this);
        connect(tick_timer_, &QTimer::timeout, this, &TimePodWidget::onTick);
        connect(qApp, &QCoreApplication::aboutToQuit, this, &TimePodWidget::onAboutToQuit);

        tick_timer_->start(100);
    }

private slots:
    void onAboutToQuit() {
        task_store_save(&tasks_);
        ui_session_save_active(&st_);
    }

    void onTaskSubmit() {
        QString desc = task_input_->text().trimmed();
        if (desc.isEmpty()) return;

        int idx = task_intake_add(&tasks_, desc.toUtf8().constData());
        if (idx < 0) return;

        /* Save immediately so the list survives crash/restart. */
        task_store_save(&tasks_);
        task_input_->clear();

        /* Try AI analysis for this new task (async). */
        pending_ai_ = true;
        ai_->analyzeTask(desc);

        update();
    }

    void onAiAnalysisFinished(const QString &taskText,
                              TimepodDomain domain,
                              uint64_t estimatedSeconds,
                              const QString &deadlineText,
                              bool ok,
                              const QString &error) {
        (void)error;
        pending_ai_ = false;
        aiReady_ = ok;

        /* Match by text (newest matching pending task). */
        for (int i = tasks_.count - 1; i >= 0; i--) {
            if (QString::fromUtf8(tasks_.items[i].text) == taskText.trimmed()) {
                if (ok) {
                    task_intake_apply_ai(&tasks_, i, domain, estimatedSeconds,
                                         deadlineText.toUtf8().constData());
                }
                /* If not ok, keep the rule-based fallback values. */
                break;
            }
        }

        task_intake_reprioritize(&tasks_, (int64_t)::time(nullptr));
        task_intake_sort_by_priority(&tasks_);
        task_store_save(&tasks_);
        update();
    }

protected:
    void keyPressEvent(QKeyEvent *e) override {
        int key = e->key();
        if (key == Qt::Key_Q) {
            QApplication::quit();
            return;
        }
        if (key == Qt::Key_P) {
            if (st_.has_session) {
                st_.paused = !st_.paused;
                timer_nb_set_paused(&nb_, &dt_, st_.paused ? 1 : 0);
                ui_session_save_active(&st_);
            }
            return;
        }
        if (key == Qt::Key_C) {
            if (st_.has_session) {
                st_.paused = false;
                timer_nb_set_paused(&nb_, &dt_, 0);
                ui_session_save_active(&st_);
            }
            return;
        }
        /* 1..9 select a task from the list and start it */
        if (key >= Qt::Key_1 && key <= Qt::Key_9) {
            int idx = key - Qt::Key_1;
            if (idx < tasks_.count) {
                startTask(idx);
            }
            return;
        }
        if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
            /* delete first pending task */
            for (int i = 0; i < tasks_.count; i++) {
                if (!tasks_.items[i].done) {
                    task_intake_remove(&tasks_, i);
                    task_intake_sort_by_priority(&tasks_);
                    task_store_save(&tasks_);
                    update();
                    break;
                }
            }
            return;
        }
        if (key == Qt::Key_Tab) {
            /* toggle done on first pending task */
            for (int i = 0; i < tasks_.count; i++) {
                if (!tasks_.items[i].done) {
                    task_intake_set_done(&tasks_, i, true);
                    task_intake_sort_by_priority(&tasks_);
                    task_store_save(&tasks_);
                    update();
                    break;
                }
            }
            return;
        }
        QWidget::keyPressEvent(e);
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.fillRect(rect(), Qt::black);

        const int w = width();
        const int h = height() - kBottomReserve;

        QRect left(20, 20, w/2 - 30, h - 40);
        QRect right(w/2 + 10, 20, w/2 - 30, h - 40);

        drawPanel(p, left, "[ACTIVE]");
        drawPanel(p, right, "[TASK LIST]");

        QFont titleF = font();
        titleF.setPointSize(14);
        titleF.setBold(true);
        p.setFont(titleF);

        drawLeftContent(p, left);
        drawRightContent(p, right);

        drawFooter(p);
    }

    void resizeEvent(QResizeEvent *e) override {
        QWidget::resizeEvent(e);
        int iw = qMin(width() - 40, 700);
        int ih = 32;
        task_input_->setGeometry((width() - iw) / 2, height() - ih - 48, iw, ih);
    }

private:
    void drawPanel(QPainter &p, const QRect &r, const QString &label) {
        p.setPen(QPen(QColor(0, 220, 255)));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r);

        QFont f = font();
        f.setPointSize(10);
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(0, 220, 255));
        p.drawText(r.adjusted(10, 6, -10, -r.height()), label);
    }

    void drawFooter(QPainter &p) {
        int y = height() - 12;
        QFont f = font();
        f.setPointSize(9);
        f.setBold(false);
        p.setFont(f);
        p.setPen(QColor(150, 150, 150));
        p.drawText(20, y, "Enter: add task | 1..9: start task | p: pause | c: continue | Tab: mark done | Del: remove | q: quit");
        QString aiStatus = pending_ai_ ? " AI analyzing..." :
                           (aiReady_ ? " AI: Ollama" : " AI: offline (rules)");
        p.drawText(width() - 200, y, aiStatus);
    }

    void drawLeftContent(QPainter &p, const QRect &r) {
        int x = r.left() + 20;
        int y = r.top() + 40;
        const int mw = r.width() - 40;

        QString state = st_.has_session ? (st_.paused ? "PAUSED" : "RUNNING") : "NO SESSION";
        QString desc = QString::fromUtf8(st_.description);
        if (desc.isEmpty() && st_.has_session) desc = "Untitled task";

        QFont f = font();
        f.setPointSize(12);
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(x, y, "State : "+state);
        y += 24;

        if (st_.has_session) {
            p.setPen(QColor(0, 220, 255));
            f.setPointSize(11);
            f.setBold(false);
            p.setFont(f);
            p.drawText(x, y, "Task: " + desc);
            y += 22;
        }

        f.setPointSize(12);
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::white);

        if (st_.has_session) {
            char buf[9]; timer_format_hms(dt_.seconds_left, buf);
            char buf2[9]; timer_format_hms(dt_.seconds_total, buf2);
            p.drawText(x, y, QString("Remaining: %1 (total %2)").arg(buf).arg(buf2));
            y += 28;

            uint64_t done = (dt_.seconds_total > dt_.seconds_left) ? (dt_.seconds_total - dt_.seconds_left) : 0;
            uint64_t denom = (dt_.seconds_total == 0) ? 1 : dt_.seconds_total;
            int pct = (int)((done * 100ULL) / denom);

            QRect bar(x, y, mw, 22);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(30, 30, 30));
            p.drawRect(bar);
            p.setBrush(QColor(0, 220, 255));
            QRect fill = bar.adjusted(0, 0, -(bar.width()*(100-pct))/100, 0);
            p.drawRect(fill);
            p.setPen(Qt::white);
            p.setBrush(Qt::NoBrush);
            p.drawRect(bar);
            p.drawText(bar, Qt::AlignCenter, QString("Progress: %1%").arg(pct));
        } else {
            QRect bar(x, y, mw, 22);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(30, 30, 30));
            p.drawRect(bar);
            p.setPen(Qt::white);
            p.drawText(bar, Qt::AlignCenter, "Progress: 0%");
        }

        /* Up next summary */
        y += 40;
        p.setPen(QColor(0, 220, 255));
        f.setPointSize(11);
        f.setBold(true);
        p.setFont(f);
        p.drawText(x, y, "Up Next:");
        y += 20;

        f.setPointSize(10);
        f.setBold(false);
        p.setFont(f);
        int shown = 0;
        for (int i = 0; i < tasks_.count && shown < 5; i++) {
            if (tasks_.items[i].done) continue;
            TaskItem &t = tasks_.items[i];
            QString line = QString("#%1 %2").arg(i + 1).arg(QString::fromUtf8(t.text));
            int maxw = mw - 10;
            if (line.length() * 7 > maxw) {
                line = line.left(maxw / 7 - 3) + "...";
            }
            p.setPen(QColor(200, 200, 200));
            p.drawText(x, y, line);
            y += 18;
            shown++;
        }
    }

    void drawRightContent(QPainter &p, const QRect &r) {
        int x = r.left() + 20;
        int y = r.top() + 40;
        const int mw = r.width() - 40;

        QFont f = font();
        f.setPointSize(12);
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(x, y, QString("Date: %1-%2-%3")
                     .arg(st_.day.year)
                     .arg(st_.day.month, 2, 10, QChar('0'))
                     .arg(st_.day.day, 2, 10, QChar('0')));
        y += 30;

        /* Completion status */
        bool any_completed = false;
        for (int i = 0; i < TIMEPOD_MAX_DOMAINS; i++) {
            if (st_.day.completed[i]) { any_completed = true; break; }
        }
        QFont sf = font();
        sf.setPointSize(14);
        sf.setBold(true);
        p.setFont(sf);
        p.setPen(any_completed ? QColor(0, 220, 100) : QColor(180, 180, 180));
        p.drawText(x, y, any_completed ? "✓ Completed" : "· Not completed");
        y += 30;

        if (tasks_.count == 0) {
            p.setPen(QColor(120, 120, 120));
            f.setPointSize(11);
            f.setBold(false);
            p.setFont(f);
            p.drawText(x, y, "No tasks yet. Add a task above.");
            return;
        }

        /* Task list header */
        f.setPointSize(10);
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(0, 220, 255));
        p.drawText(x, y, "#  Category                          Est.   Prio      Task");
        y += 18;

        /* Draw each task row */
        f.setPointSize(9);
        f.setBold(false);
        p.setFont(f);

        for (int i = 0; i < tasks_.count && y < r.bottom() - 20; i++) {
            TaskItem &t = tasks_.items[i];
            QString num = QString::number(i + 1);
            QString domain = QString::fromUtf8(timepod_domain_name(t.domain));
            QString est = fmtDuration(t.estimated_seconds);
            QString prio = prioName(t.priority);
            QString taskText = QString::fromUtf8(t.text);

            static const QColor prioColors[] = {
                QColor(100, 100, 100),  /* LOW */
                QColor(200, 200, 100),  /* MEDIUM */
                QColor(220, 150, 50),   /* HIGH */
                QColor(220, 50, 50)     /* URGENT */
            };
            QColor pc = (t.priority >= 0 && t.priority <= 3) ? prioColors[t.priority] : QColor(200, 200, 200);

            int colX = x;
            int colW = 24;
            p.setPen(t.done ? QColor(60, 60, 60) : Qt::white);
            p.drawText(colX, y, colW, 16, Qt::AlignLeft, num);
            colX += colW + 2;

            colW = 34;
            p.setPen(t.done ? QColor(60, 60, 60) : QColor(0, 220, 255));
            p.drawText(colX, y, colW, 16, Qt::AlignLeft, domain.left(4));
            colX += colW + 2;

            colW = 60;
            p.setPen(t.done ? QColor(60, 60, 60) : QColor(180, 180, 180));
            p.drawText(colX, y, colW, 16, Qt::AlignLeft, est);
            colX += colW + 2;

            colW = 54;
            p.setPen(t.done ? QColor(60, 60, 60) : pc);
            p.drawText(colX, y, colW, 16, Qt::AlignLeft, prio);
            colX += colW + 2;

            int remaining = mw - (colX - x);
            if (remaining < 20) remaining = 20;
            if (t.done) {
                p.setPen(QColor(60, 60, 60));
                QFont sf2 = f;
                sf2.setStrikeOut(true);
                p.setFont(sf2);
            } else {
                p.setPen(Qt::white);
            }
            QString displayText = taskText;
            int maxChars = remaining / 7;
            if (displayText.length() > maxChars && maxChars > 5) {
                displayText = displayText.left(maxChars - 3) + "...";
            }
            p.drawText(colX, y, remaining, 16, Qt::AlignLeft, displayText);
            p.setFont(f);

            /* Deadline indicator */
            if (t.deadline_epoch > 0 && !t.done) {
                int64_t now = (int64_t)::time(nullptr);
                QString dl;
                if (t.deadline_epoch <= now) {
                    dl = " OVERDUE!";
                    p.setPen(QColor(255, 50, 50));
                } else {
                    int64_t rem = t.deadline_epoch - now;
                    if (rem < 86400) dl = " due soon";
                    else dl = " due " + fmtDuration((uint64_t)rem);
                    p.setPen(QColor(255, 200, 50));
                }
                p.drawText(colX, y + 14, remaining, 14, Qt::AlignLeft, dl);
            }

            y += 20;
        }
    }

    void startTask(int idx) {
        if (idx < 0 || idx >= tasks_.count) return;
        if (tasks_.items[idx].done) {
            task_intake_set_done(&tasks_, idx, false);
            return;
        }

        TaskItem &t = tasks_.items[idx];

        /* Store description in UiState */
        std::strncpy(st_.description, t.text, sizeof(st_.description) - 1);
        st_.description[sizeof(st_.description) - 1] = '\0';

        dt_ = {};
        timer_init_hours(&dt_, timepod_domain_name(t.domain), 0);
        dt_.seconds_total = t.estimated_seconds;
        dt_.seconds_left = t.estimated_seconds;

        st_.active_domain_idx = (int)t.domain;
        st_.has_session = true;
        st_.paused = false;
        st_.session_mode = t.ai_analyzed ? 1 : 0;
        st_.seconds_left = dt_.seconds_left;
        st_.seconds_total = dt_.seconds_total;

        timer_nb_start(&nb_, &dt_);
        dt_initialized_ = true;

        /* Save session so it can be resumed */
        ui_session_save_active(&st_);
        task_store_save(&tasks_);

        update();
    }

    void onTick() {
        if (!st_.has_session) {
            update();
            return;
        }

        int completed = timer_nb_step(&nb_, &dt_);
        st_.seconds_left = dt_.seconds_left;
        st_.seconds_total = dt_.seconds_total;

        if (completed) {
            ui_mark_domain_completed_today(&st_, st_.active_domain_idx);
            timepod_notify_domain_completed(timepod_domain_name(
                (TimepodDomain)st_.active_domain_idx));

            /* Mark the task as done */
            for (int i = 0; i < tasks_.count; i++) {
                if (std::strcmp(tasks_.items[i].text, st_.description) == 0) {
                    task_intake_set_done(&tasks_, i, true);
                    break;
                }
            }

            st_.has_session = false;
            st_.paused = false;
            st_.seconds_left = 0;
            st_.seconds_total = 0;
            st_.description[0] = '\0';
            ui_session_clear_active(&st_);

            task_intake_reprioritize(&tasks_, (int64_t)::time(nullptr));
            task_intake_sort_by_priority(&tasks_);
            task_store_save(&tasks_);
            task_input_->setEnabled(true);
            task_input_->setFocus();
        }

        update();
    }

    UiState st_{};
    TimerNB nb_{};
    DomainTimer dt_{};
    bool dt_initialized_ = false;
    QTimer *tick_timer_ = nullptr;
    QLineEdit *task_input_ = nullptr;
    AiClient *ai_ = nullptr;
    bool pending_ai_ = false;
    bool aiReady_ = false;
    TaskQueue tasks_{};
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TimePodWidget w;
    w.show();
    return app.exec();
}

#include "main.moc"
