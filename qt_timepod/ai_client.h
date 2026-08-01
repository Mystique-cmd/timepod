#ifndef TIMEPOD_AI_CLIENT_H
#define TIMEPOD_AI_CLIENT_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

#include "task_intake.h"

/*
 * AI client for TimePod.
 *
 * Talks to a local Ollama server (default http://localhost:11434)
 * via its native /api/chat endpoint with JSON output mode.
 *
 * Environment overrides:
 *   TIMEPOD_AI_BASE_URL  (default http://localhost:11434)
 *   TIMEPOD_AI_MODEL     (default llama3.2)
 *
 * The response JSON is parsed into a structured result:
 *   { "category": "...", "estimated_minutes": N, "deadline": "..." }
 */
class AiClient : public QObject {
    Q_OBJECT
public:
    explicit AiClient(QObject *parent = nullptr);

    QString model() const { return model_; }
    void setModel(const QString &m) { model_ = m; }

    QString baseUrl() const { return baseUrl_; }
    void setBaseUrl(const QString &u) { baseUrl_ = u; }

public slots:
    /* Send one task text to the model and emit analysisFinished(). */
    void analyzeTask(const QString &text);

signals:
    void analysisFinished(const QString &taskText,
                          TimepodDomain domain,
                          uint64_t estimatedSeconds,
                          const QString &deadlineText,
                          bool ok,
                          const QString &error);

private:
    QNetworkAccessManager net_;
    QString model_;
    QString baseUrl_;
};

#endif // TIMEPOD_AI_CLIENT_H

