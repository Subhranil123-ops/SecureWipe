#include "SanitizationResultService.h"

#include "../../backend/sanitization/include/SanitizationCertificate.h"
#include "../../backend/sanitization/include/SanitizationPipeline.h"
#include "../../backend/sanitization/include/SanitizationResult.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace
{
constexpr const char *kApiBaseUrl = "http://localhost:5000";

QString sanitizationStatusToString(
    SecureWipe::SanitizationStatus status)
{
    switch (status)
    {
    case SecureWipe::SanitizationStatus::NOT_STARTED:
        return QStringLiteral("NOT_STARTED");

    case SecureWipe::SanitizationStatus::IN_PROGRESS:
        return QStringLiteral("IN_PROGRESS");

    case SecureWipe::SanitizationStatus::COMPLETED:
        return QStringLiteral("COMPLETED");

    case SecureWipe::SanitizationStatus::FAILED:
        return QStringLiteral("FAILED");

    case SecureWipe::SanitizationStatus::ABORTED:
        return QStringLiteral("ABORTED");

    default:
        return QStringLiteral("NOT_STARTED");
    }
}

QString verificationStatusToString(
    SecureWipe::VerificationStatus status)
{
    switch (status)
    {
    case SecureWipe::VerificationStatus::NOT_PERFORMED:
        return QStringLiteral("NOT_PERFORMED");

    case SecureWipe::VerificationStatus::IN_PROGRESS:
        return QStringLiteral("IN_PROGRESS");

    case SecureWipe::VerificationStatus::PASSED:
        return QStringLiteral("PASSED");

    case SecureWipe::VerificationStatus::FAILED:
        return QStringLiteral("FAILED");

    default:
        return QStringLiteral("NOT_PERFORMED");
    }
}

QString sanitizationMethodToString(
    SanitizationMethod method)
{
    switch (method)
    {
    case SanitizationMethod::NvmeSanitize:
        return QStringLiteral("NVME_SANITIZE");

    case SanitizationMethod::AtaSanitize:
        return QStringLiteral("ATA_SANITIZE");

    case SanitizationMethod::HostOverwrite:
        return QStringLiteral("HOST_OVERWRITE");

    case SanitizationMethod::Unsupported:
    default:
        return QStringLiteral("UNSUPPORTED");
    }
}

QNetworkRequest makeApiRequest(
    const QUrl &url,
    const QString &token)
{
    QNetworkRequest request(url);

    request.setRawHeader(
        "Authorization",
        QByteArray("Bearer ") + token.toUtf8());

    request.setRawHeader(
        "Accept",
        "application/json");

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QStringLiteral("application/json"));

    return request;
}

bool parseApiResponse(
    QNetworkReply *reply,
    QJsonObject &response,
    QString &errorMessage)
{
    const int statusCode =
        reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute)
            .toInt();

    const QByteArray body =
        reply->readAll();

    if (reply->error() !=
        QNetworkReply::NoError)
    {
        errorMessage =
            QStringLiteral(
                "Request failed (HTTP %1): %2")
                .arg(
                    statusCode)
                .arg(
                    reply->errorString());

        return false;
    }

    QJsonParseError parseError{};

    const QJsonDocument document =
        QJsonDocument::fromJson(
            body,
            &parseError);

    if (parseError.error !=
        QJsonParseError::NoError)
    {
        errorMessage =
            QStringLiteral(
                "Server returned invalid JSON: %1")
                .arg(
                    parseError.errorString());

        return false;
    }

    if (!document.isObject())
    {
        errorMessage =
            QStringLiteral(
                "Server returned an invalid JSON response.");

        return false;
    }

    response =
        document.object();

    if (!response.value(
            QStringLiteral("success"))
            .toBool())
    {
        errorMessage =
            response.value(
                QStringLiteral("message"))
                .toString(
                    QStringLiteral(
                        "Server rejected the request."));

        return false;
    }

    return true;
}
}

SanitizationResultService::SanitizationResultService(
    QObject *parent)
    : QObject(parent)
    , networkManager_(
          new QNetworkAccessManager(this))
{
}

void SanitizationResultService::submitResult(
    const QString &token,
    const QString &requestId,
    const SecureWipe::SanitizationPipelineResult
        &pipelineResult)
{
    if (token.trimmed().isEmpty())
    {
        emit resultSubmissionFailed(
            QStringLiteral(
                "Authentication token is missing."));
        return;
    }

    if (requestId.trimmed().isEmpty())
    {
        emit resultSubmissionFailed(
            QStringLiteral(
                "Request ID is missing."));
        return;
    }

    const QUrl url(
        QStringLiteral(
            "%1/api/sanitization-results/%2")
            .arg(
                QString::fromLatin1(
                    kApiBaseUrl),
                requestId));

    const QNetworkRequest request =
        makeApiRequest(
            url,
            token);

    const QJsonObject payload =
        resultToJson(
            pipelineResult);

    const QByteArray requestBody =
        QJsonDocument(payload)
            .toJson(
                QJsonDocument::Compact);

    QNetworkReply *reply =
        networkManager_->post(
            request,
            requestBody);

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply, requestId, token, pipelineResult]()
        {
            QJsonObject response;
            QString errorMessage;

            if (!parseApiResponse(
                    reply,
                    response,
                    errorMessage))
            {
                emit resultSubmissionFailed(
                    QStringLiteral(
                        "Failed to submit sanitization result for %1: %2")
                        .arg(
                            requestId,
                            errorMessage));

                reply->deleteLater();
                return;
            }

            emit resultSubmitted(
                requestId);

            reply->deleteLater();

            /*
             * The backend changes an IN_PROGRESS request
             * to VERIFYING after accepting a successful
             * sanitization result.
             *
             * Only a successful local pipeline that
             * generated a valid certificate should
             * continue to certificate submission.
             */
            if (
                pipelineResult.sanitization.isSuccess() &&
                pipelineResult.certificateGenerated &&
                pipelineResult.certificate.isValid())
            {
                submitCertificate(
                    token,
                    requestId,
                    pipelineResult);
            }
        });
}

void SanitizationResultService::submitCertificate(
    const QString &token,
    const QString &requestId,
    const SecureWipe::SanitizationPipelineResult
        &pipelineResult)
{
    if (token.trimmed().isEmpty())
    {
        emit certificateSubmissionFailed(
            QStringLiteral(
                "Authentication token is missing."));
        return;
    }

    if (requestId.trimmed().isEmpty())
    {
        emit certificateSubmissionFailed(
            QStringLiteral(
                "Request ID is missing."));
        return;
    }

    if (!pipelineResult.certificateGenerated)
    {
        emit certificateSubmissionFailed(
            QStringLiteral(
                "No sanitization certificate was generated by the pipeline."));
        return;
    }

    if (!pipelineResult.certificate.isValid())
    {
        emit certificateSubmissionFailed(
            QStringLiteral(
                "The generated sanitization certificate is not valid for submission."));
        return;
    }

    const QUrl url(
        QStringLiteral(
            "%1/api/sanitization-certificates/%2")
            .arg(
                QString::fromLatin1(
                    kApiBaseUrl),
                requestId));

    const QNetworkRequest request =
        makeApiRequest(
            url,
            token);

    const QJsonObject payload =
        certificateToJson(
            pipelineResult);

    const QByteArray requestBody =
        QJsonDocument(payload)
            .toJson(
                QJsonDocument::Compact);

    QNetworkReply *reply =
        networkManager_->post(
            request,
            requestBody);

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply, requestId, pipelineResult]()
        {
            QJsonObject response;
            QString errorMessage;

            if (!parseApiResponse(
                    reply,
                    response,
                    errorMessage))
            {
                emit certificateSubmissionFailed(
                    QStringLiteral(
                        "Failed to submit certificate for %1: %2")
                        .arg(
                            requestId,
                            errorMessage));

                reply->deleteLater();
                return;
            }

            QString certificateId;

            const QJsonValue dataValue =
                response.value(
                    QStringLiteral("data"));

            if (dataValue.isObject())
            {
                certificateId =
                    dataValue.toObject()
                        .value(
                            QStringLiteral(
                                "certificateId"))
                        .toString();
            }

            if (certificateId.isEmpty())
            {
                certificateId =
                    QString::fromStdString(
                        pipelineResult
                            .certificate
                            .certificateId);
            }

            emit certificateSubmitted(
                requestId,
                certificateId);

            reply->deleteLater();
        });
}

QJsonObject SanitizationResultService::resultToJson(
    const SecureWipe::SanitizationPipelineResult
        &pipelineResult) const
{
    const SecureWipe::SanitizationResult &result =
        pipelineResult.sanitization;

    QJsonObject object;

    object.insert(
        QStringLiteral("operationId"),
        QString::fromStdString(
            result.operationId));

    object.insert(
        QStringLiteral("deviceId"),
        QString::fromStdString(
            result.deviceId));

    object.insert(
        QStringLiteral("model"),
        QString::fromStdString(
            result.model));

    object.insert(
        QStringLiteral("serialNumber"),
        QString::fromStdString(
            result.serialNumber));

    object.insert(
        QStringLiteral("capacityBytes"),
        QString::number(
            static_cast<qulonglong>(
                result.capacityBytes)));

    object.insert(
        QStringLiteral("interfaceType"),
        QString::fromStdString(
            result.interfaceType));

    object.insert(
        QStringLiteral("method"),
        sanitizationMethodToString(
            result.method));

    object.insert(
        QStringLiteral("status"),
        sanitizationStatusToString(
            result.status));

    object.insert(
        QStringLiteral("bytesProcessed"),
        QString::number(
            static_cast<qulonglong>(
                result.bytesProcessed)));

    object.insert(
        QStringLiteral("operationDurationMs"),
        QString::number(
            static_cast<qulonglong>(
                result.operationDurationMs)));

    object.insert(
        QStringLiteral("verificationStatus"),
        verificationStatusToString(
            result.verificationStatus));

    object.insert(
        QStringLiteral("verificationPerformed"),
        result.verificationPerformed);

    object.insert(
        QStringLiteral("bytesVerified"),
        QString::number(
            static_cast<qulonglong>(
                result.bytesVerified)));

    object.insert(
        QStringLiteral("verificationSamples"),
        static_cast<int>(
            result.verificationSamples));

    object.insert(
        QStringLiteral("verificationMessage"),
        QString::fromStdString(
            result.verificationMessage));

    object.insert(
        QStringLiteral("nativeErrorCode"),
        static_cast<int>(
            result.nativeErrorCode));

    object.insert(
        QStringLiteral("message"),
        QString::fromStdString(
            result.message));

    object.insert(
        QStringLiteral("errorMessage"),
        QString::fromStdString(
            result.errorMessage));

    return object;
}

QJsonObject SanitizationResultService::certificateToJson(
    const SecureWipe::SanitizationPipelineResult
        &pipelineResult) const
{
    const SecureWipe::SanitizationCertificate
        &certificate =
            pipelineResult.certificate;

    QJsonObject object;

    object.insert(
        QStringLiteral("certificateId"),
        QString::fromStdString(
            certificate.certificateId));

    object.insert(
        QStringLiteral("operationId"),
        QString::fromStdString(
            certificate.operationId));

    object.insert(
        QStringLiteral("requestId"),
        QString::fromStdString(
            certificate.requestId));

    object.insert(
        QStringLiteral("deviceId"),
        QString::fromStdString(
            certificate.deviceId));

    object.insert(
        QStringLiteral("model"),
        QString::fromStdString(
            certificate.model));

    object.insert(
        QStringLiteral("serialNumber"),
        QString::fromStdString(
            certificate.serialNumber));

    object.insert(
        QStringLiteral("capacityBytes"),
        QString::number(
            static_cast<qulonglong>(
                certificate.capacityBytes)));

    object.insert(
        QStringLiteral("interfaceType"),
        QString::fromStdString(
            certificate.interfaceType));

    object.insert(
        QStringLiteral("method"),
        sanitizationMethodToString(
            certificate.method));

    object.insert(
        QStringLiteral("status"),
        sanitizationStatusToString(
            certificate.status));

    object.insert(
        QStringLiteral("bytesProcessed"),
        QString::number(
            static_cast<qulonglong>(
                certificate.bytesProcessed)));

    object.insert(
        QStringLiteral("operationDurationMs"),
        QString::number(
            static_cast<qulonglong>(
                certificate.operationDurationMs)));

    object.insert(
        QStringLiteral("verificationStatus"),
        verificationStatusToString(
            certificate.verificationStatus));

    object.insert(
        QStringLiteral("verificationPerformed"),
        certificate.verificationPerformed);

    object.insert(
        QStringLiteral("verificationPassed"),
        certificate.verificationPassed);

    object.insert(
        QStringLiteral("bytesVerified"),
        QString::number(
            static_cast<qulonglong>(
                certificate.bytesVerified)));

    object.insert(
        QStringLiteral("verificationSamples"),
        static_cast<int>(
            certificate.verificationSamples));

    object.insert(
        QStringLiteral("deviceReportedSuccess"),
        certificate.deviceReportedSuccess);

    object.insert(
        QStringLiteral("globalDataErased"),
        certificate.globalDataErased);

    object.insert(
        QStringLiteral("nativeErrorCode"),
        static_cast<int>(
            certificate.nativeErrorCode));

    object.insert(
        QStringLiteral("verificationMessage"),
        QString::fromStdString(
            certificate.verificationMessage));

    object.insert(
        QStringLiteral("generatedAt"),
        QString::fromStdString(
            certificate.generatedAt));

    object.insert(
        QStringLiteral("hashAlgorithm"),
        QString::fromStdString(
            certificate.hashAlgorithm));

    object.insert(
        QStringLiteral("certificateHash"),
        QString::fromStdString(
            certificate.certificateHash));

    object.insert(
        QStringLiteral("message"),
        QString::fromStdString(
            certificate.message));

    return object;
}