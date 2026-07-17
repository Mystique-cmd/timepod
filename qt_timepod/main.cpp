#include <QApplication>
#include <QCoreApplication>

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include <QFont>
#include <QVBoxLayout>
#include <QHBoxLayout>
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
            // Reconstruct DomainTimer from persisted values
            static const char *names[TIMEPOD_MAX_DOMAINS] = {
                "The Portal",
                "The Factory",
                "Benjamin's Game",
                "The Matrix Manual",
                "The Rabbit Hole",
                "Specter Spectacle"
            };

            int idx = st_.active_domain_idx;
            if (idx >= 0 && idx < TIMEPOD_MAX_DOMAINS) {
                dt_ = {};
                timer_init_hours(&dt_, names[idx], 0);

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



        tick_timer_ = new QTimer(this);

        connect(tick_timer_, &QTimer::timeout, this, &TimePodWidget::onTick);
        connect(qApp, &QCoreApplication::aboutToQuit, this, &TimePodWidget::onAboutToQuit);

        tick_timer_->start(100);
    }

private slots:
    void onAboutToQuit() {
        ui_session_save_active(&st_);
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
        const int h = height();

        QRect left(20, 20, w/2 - 30, h - 40);
        QRect right(w/2 + 10, 20, w/2 - 30, h - 40);

        drawPanel(p, left, "[ACTIVE]");
        drawPanel(p, right, "[CALENDAR]");

        // Fonts
        QFont titleF = font();
        titleF.setPointSize(14);
        titleF.setBold(true);
        p.setFont(titleF);

        drawLeftContent(p, left);
        drawRightContent(p, right);
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

        static const char *names[TIMEPOD_MAX_DOMAINS] = {
            "The Portal",
            "The Factory",
            "Benjamin's Game",
            "The Matrix Manual",
            "The Rabbit Hole",
            "Specter Spectacle"
        };

        QString domain = st_.has_session ? names[st_.active_domain_idx] : "Idle";
        QString state = st_.has_session ? (st_.paused ? "PAUSED" : "RUNNING") : "NO SESSION";

        QFont f = font();
        f.setPointSize(12);
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(x, y, "Domain: "+domain);
        y += 26;
        p.drawText(x, y, "State : "+state);
        y += 26;

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

        static const char *names[TIMEPOD_MAX_DOMAINS] = {
            "The Portal",
            "The Factory",
            "Benjamin's Game",
            "The Matrix Manual",
            "The Rabbit Hole",
            "Specter Spectacle"
        };

        p.setPen(QColor(0, 220, 255));

        int gridLeft = x;
        int gridTop = y;
        int cellW = (mw - 30) / 2;
        int cellH = 70;

        for (int i = 0; i < TIMEPOD_MAX_DOMAINS; i++) {
            int row = i / 2;
            int col = i % 2;
            QRect cell(gridLeft + col*(cellW+15), gridTop + row*(cellH+10), cellW, cellH);
            p.setPen(QPen(QColor(0, 220, 255)));
            p.setBrush(Qt::NoBrush);
            p.drawRect(cell);

            bool ok = st_.day.completed[i];
            p.setPen(Qt::white);
            p.setFont(font());
            QFont small = font(); small.setPointSize(10); small.setBold(true);
            p.setFont(small);
            p.drawText(cell.adjusted(8, 10, -8, -8), Qt::AlignLeft | Qt::AlignTop,
                       QString("%1 %2").arg(ok ? "✓" : "·").arg(names[i]));
        }

        // Legend
        y = r.bottom() - 70;
        p.setFont(font());
        QFont lf = font(); lf.setPointSize(10); lf.setBold(false);
        p.setFont(lf);
        p.setPen(QColor(180, 180, 180));
        p.drawText(x, y, "Legend: ✓ completed today   · not completed");
    }

    void startDomain(int idx) {
        static const char *names[TIMEPOD_MAX_DOMAINS] = {
            "The Portal",
            "The Factory",
            "Benjamin's Game",
            "The Matrix Manual",
            "The Rabbit Hole",
            "Specter Spectacle"
        };
        const uint64_t hours[TIMEPOD_MAX_DOMAINS] = {7, 4, 4, 1, 4, 4};

        dt_ = {};
        timer_init_hours(&dt_, names[idx], hours[idx]);
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
            ui_session_clear_active(&st_);
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

