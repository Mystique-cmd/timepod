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
#include <algorithm>
#include <cstring>

extern "C" {
#include "timer.h"
#include "timer_nb.h"
#include "ui.h"
#include "notify.h"
}


class TimePodWidget : public QWidget {
public:
    TimePodWidget(QWidget *parent=nullptr) : QWidget(parent) {
        setFocusPolicy(Qt::StrongFocus);
        setMinimumSize(720, 420);

        std::memset(&st_, 0, sizeof(st_));
        st_.active_domain_idx = 0;
        st_.has_session = false;
        st_.paused = false;

        timer_nb_init(&nb_);
        ui_load_day_record(&st_);

        // Restore active session if present
        ui_session_load_active(&st_);
        if (st_.has_session) {
            int idx = st_.active_domain_idx;
            if (idx >= 0 && idx < TIMEPOD_MAX_DOMAINS) {
                dt_ = {};
                timer_init_hours(&dt_, "Session", 0);

                /* Restore persisted remaining + total */
                dt_.seconds_total = st_.seconds_total;
                dt_.seconds_left = st_.seconds_left;

                timer_nb_start(&nb_, &dt_);
                timer_nb_set_paused(&nb_, &dt_, st_.paused ? 1 : 0);

                /* Ensure timer state reflects persisted remaining */
                dt_.seconds_left = st_.seconds_left;
                dt_.seconds_total = st_.seconds_total;

                st_.seconds_left = dt_.seconds_left;
                st_.seconds_total = dt_.seconds_total;

                dt_initialized_ = true;
            }
        }

        // Task description input field
        task_input_ = new QLineEdit(this);
        task_input_->setPlaceholderText("Describe your task and press Enter to start...");
        task_input_->setStyleSheet(
            "QLineEdit {"
            "  background-color: #1a1a1a;"
            "  color: #ffffff;"
            "  border: 1px solid #555;"
            "  border-radius: 4px;"
            "  padding: 4px 8px;"
            "  font-size: 12pt;"
            "}"
            "QLineEdit:focus {"
            "  border-color: #00dcff;"
            "}"
        );
        if (st_.has_session) {
            task_input_->setEnabled(false);
            task_input_->setText(QString::fromUtf8(st_.description));
        }
        connect(task_input_, &QLineEdit::returnPressed, this, &TimePodWidget::onTaskSubmit);

        tick_timer_ = new QTimer(this);

        connect(tick_timer_, &QTimer::timeout, this, &TimePodWidget::onTick);
        connect(qApp, &QCoreApplication::aboutToQuit, this, &TimePodWidget::onAboutToQuit);

        tick_timer_->start(100);
    }

private slots:
    void onAboutToQuit() {
        ui_session_save_active(&st_);
    }

    void onTaskSubmit() {
        QString desc = task_input_->text().trimmed();
        if (desc.isEmpty()) return;

        // Store description in UiState
        QByteArray utf8 = desc.toUtf8();
        std::strncpy(st_.description, utf8.constData(), sizeof(st_.description) - 1);
        st_.description[sizeof(st_.description) - 1] = '\0';

        // Start a timer session (use default domain 0)
        startDomain(0);

        task_input_->clear();
        task_input_->setEnabled(false);
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

        // TODO: free-text description mode will be implemented next.
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing, true);

        p.fillRect(rect(), Qt::black);

        const int w = width();
        const int h = height() - 50; // reserve space for input field at bottom

        QRect left(20, 20, w/2 - 30, h - 40);
        QRect right(w/2 + 10, 20, w/2 - 30, h - 40);

        drawPanel(p, left, "[ACTIVE]");
        drawPanel(p, right, "[TODAY]");

        // Fonts
        QFont titleF = font();
        titleF.setPointSize(14);
        titleF.setBold(true);
        p.setFont(titleF);

        drawLeftContent(p, left);
        drawRightContent(p, right);
    }

    void resizeEvent(QResizeEvent *e) override {
        QWidget::resizeEvent(e);
        // Position input field at bottom center
        int iw = qMin(width() - 40, 600);
        int ih = 34;
        task_input_->setGeometry((width() - iw) / 2, height() - ih - 12, iw, ih);
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
            p.setFont(f);
            p.drawText(x, y, "Task: " + desc);
            y += 24;
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

            p.drawText(bar, Qt::AlignCenter, QString("Progress: %1%" ).arg(pct));
        } else {
            QRect bar(x, y, mw, 22);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(30, 30, 30));
            p.drawRect(bar);
            p.setPen(Qt::white);
            p.drawText(bar, Qt::AlignCenter, "Progress: 0%" );
        }

        // Controls
        y = r.bottom() - 60;
        QFont cf = font();
        cf.setPointSize(10);
        cf.setBold(false);
        p.setFont(cf);
        p.setPen(QColor(180, 180, 180));
        p.drawText(x, y, "Controls: 1..6 start | p pause/resume | c continue | q quit");
    }

    void drawRightContent(QPainter &p, const QRect &r) {
        int x = r.left() + 20;
        int y = r.top() + 40;

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

        // Single completion status for the day
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
    }

    void startDomain(int idx) {
        const uint64_t hours[TIMEPOD_MAX_DOMAINS] = {7, 4, 4, 1, 4, 4};

        dt_ = {};
        timer_init_hours(&dt_, "Session", hours[idx]);
        st_.active_domain_idx = idx;
        st_.has_session = true;
        st_.paused = false;

        timer_nb_start(&nb_, &dt_);
        st_.seconds_left = dt_.seconds_left;
        st_.seconds_total = dt_.seconds_total;
        dt_initialized_ = true;
    }

    UiState st_{};
    TimerNB nb_{};
    DomainTimer dt_{};
    bool dt_initialized_;
    QTimer *tick_timer_ = nullptr;
    QLineEdit *task_input_ = nullptr;

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
            timepod_notify_domain_completed(dt_.name);
            st_.has_session = false;
            st_.paused = false;
            st_.seconds_left = 0;
            st_.seconds_total = 0;
            st_.description[0] = '\0';
            ui_session_clear_active(&st_);
            task_input_->setEnabled(true);
            task_input_->setFocus();
        }

        update();
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TimePodWidget w;
    w.show();
    return app.exec();
}

