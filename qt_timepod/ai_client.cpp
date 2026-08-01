#include "ai_client.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <cstdlib>
#include <cstring>

AiClient::AiClient(QObject *parent)
    : QObject(parent) {
    const char *base = std::getenv("TIMEPOD_AI_BASE_URL");
    baseUrl_ = base && base[0] ? QString::fromUtf8(base) : QStringLiteral("http://localhost:11434");

    const char *model = std::getenv("TIMEPOD_AI_MODEL");
    model_ = model && model[0] ? QString::fromUtf8(model) : QStringLiteral("llama3.2");
}

void AiClient::analyzeTask(const QString &text) {
    QJsonObject systemMsg;
    systemMsg.insert(QStringLiteral("role"), QStringLiteral("system"));
    systemMsg.insert(QStringLiteral("content"),
        QStringLiteral(
            "You are TimePod's task analyzer. Analyze a task and return ONLY valid JSON with "
            "exactly these keys: category (one of: 'portal','benjamins_game','factory',"
            "'rabbit_hole','specter','matrix_manual'), estimated_minutes (integer, average "
            "human completion time), deadline (string, empty if none, or a phrase like "
            "'tomorrow', 'friday', '2025-06-01', 'in 3 days'). "
            "Category mapping: portal=sleep/rest, benjamins_game=generating income, "
            "factory=studies/learning, rabbit_hole=cybersecurity, specter=self improvement, "
            "matrix_manual=against dogma/questioning beliefs. "
            "Do not output anything besides the JSON object."));

    QJsonObject userMsg;
    userMsg.insert(QStringLiteral("role"), QStringLiteral("user"));
    userMsg.insert(QStringLiteral("content"), text);

    QJsonArray msgs;
    msgs.append(systemMsg);
    msgs.append(userMsg);

    QJsonObject body;
    body.insert(QStringLiteral("model"), model_);
    body.insert(QStringLiteral("messages"), msgs);
    body.insert(QStringLiteral("stream"), false);
    body.insert(QStringLiteral("format"), QStringLiteral("json"));

    QJsonObject options;
    options.insert(QStringLiteral("temperature"), 0.2);
    body.insert(QStringLiteral("options"), options);

    QUrl url(baseUrl_ + QStringLiteral("/api/chat"));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = net_.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, text]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit analysisFinished(text, TIMEPOD_DOMAIN_UNKNOWN, 0, QString(),
                                  false, reply->errorString());
            return;
        }

        const QByteArray data = reply->readAll();
        QJsonParseError parseErr;
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            emit analysisFinished(text, TIMEPOD_DOMAIN_UNKNOWN, 0, QString(),
                                  false, QStringLiteral("Invalid JSON from model"));
            return;
        }

        const QJsonObject root = doc.object();
        /* Ollama /api/chat returns { message: { role, content } } */
        QString content;
        if (root.contains(QStringLiteral("message")) && root.value(QStringLiteral("message")).isObject()) {
            const QJsonObject msg = root.value(QStringLiteral("message")).toObject();
            content = msg.value(QStringLiteral("content")).toString();
        } else if (root.contains(QStringLiteral("response"))) {
            content = root.value(QStringLiteral("response")).toString();
        } else {
            content = QString::fromUtf8(data);
        }

        if (content.isEmpty()) {
            emit analysisFinished(text, TIMEPOD_DOMAIN_UNKNOWN, 0, QString(),
                                  false, QStringLiteral("Empty model response"));
            return;
        }

        /* The content should be JSON. Find the first { and last }. */
        int start = content.indexOf(QChar('{'));
        int end = content.lastIndexOf(QChar('}'));
        if (start < 0 || end <= start) {
            emit analysisFinished(text, TIMEPOD_DOMAIN_UNKNOWN, 0, QString(),
                                  false, QStringLiteral("Model did not return JSON"));
            return;
        }

        const QString jsonStr = content.mid(start, end - start + 1);
        const QJsonDocument jd = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !jd.isObject()) {
            emit analysisFinished(text, TIMEPOD_DOMAIN_UNKNOWN, 0, QString(),
                                  false, QStringLiteral("Model JSON unparsable"));
            return;
        }

        const QJsonObject j = jd.object();

        /* category */
        TimepodDomain domain = TIMEPOD_DOMAIN_UNKNOWN;
        const QString cat = j.value(QStringLiteral("category")).toString().trimmed().toLower();
        if (cat == QLatin1String("portal")) domain = TIMEPOD_DOMAIN_PORTAL;
        else if (cat == QLatin1String("benjamins_game")) domain = TIMEPOD_DOMAIN_BENJAMINS_GAME;
        else if (cat == QLatin1String("factory")) domain = TIMEPOD_DOMAIN_FACTORY;
        else if (cat == QLatin1String("rabbit_hole")) domain = TIMEPOD_DOMAIN_RABBIT_HOLE;
        else if (cat == QLatin1String("specter")) domain = TIMEPOD_DOMAIN_SPECTER;
        else if (cat == QLatin1String("matrix_manual")) domain = TIMEPOD_DOMAIN_MATRIX_MANUAL;

        /* estimated_minutes */
        uint64_t estimatedSeconds = 0;
        const QJsonValue est = j.value(QStringLiteral("estimated_minutes"));
        if (est.isDouble()) {
            estimatedSeconds = (uint64_t)(est.toDouble() * 60.0);
        } else if (est.isString()) {
            bool ok = false;
            int v = est.toString().toInt(&ok);
            if (ok && v > 0) estimatedSeconds = (uint64_t)v * 60ULL;
        }

        /* deadline */
        const QString deadline = j.value(QStringLiteral("deadline")).toString().trimmed();

        emit analysisFinished(text, domain, estimatedSeconds, deadline, true, QString());
    });
}

