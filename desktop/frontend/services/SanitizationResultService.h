#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QNetworkAccessManager>

namespace SecureWipe
{
    struct SanitizationPipelineResult;
}

class SanitizationResultService : public QObject
{
    Q_OBJECT

public:
    explicit SanitizationResultService(
        QObject *parent = nullptr
    );

    void submitResult(
        const QString &token,
        const QString &requestId,
        const QString &workstationId,
        const SecureWipe::SanitizationPipelineResult &pipelineResult
    );

    void submitCertificate(
        const QString &token,
        const QString &requestId,
        const QString &workstationId,
        const SecureWipe::SanitizationPipelineResult &pipelineResult
    );

signals:
    void resultSubmitted(
        const QString &requestId
    );

    void resultSubmissionFailed(
        const QString &message
    );

    void certificateSubmitted(
        const QString &requestId,
        const QString &certificateId
    );

    void certificateSubmissionFailed(
        const QString &message
    );

private:
    QNetworkAccessManager *networkManager_;

    QJsonObject resultToJson(
        const SecureWipe::SanitizationPipelineResult &pipelineResult
    ) const;

    QJsonObject certificateToJson(
        const SecureWipe::SanitizationPipelineResult &pipelineResult
    ) const;
};