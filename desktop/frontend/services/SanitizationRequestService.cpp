#include "SanitizationRequestService.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QCryptographicHash>
#include <QHostInfo>
#include <QSettings>
#include <QSysInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace
{
constexpr const char *kApiBaseUrl = "http://localhost:5000";
}

SanitizationRequestService::SanitizationRequestService(
    QObject *parent)
    : QObject(parent)
    , networkManager_(new QNetworkAccessManager(this))
{
}

void SanitizationRequestService::fetchAssignedRequests(
    const QString &token)
{
    if (token.trimmed().isEmpty())
    {
        emit requestFetchFailed(
            QStringLiteral(
                "Employee authentication token is missing."));
        return;
    }

    const QUrl url(
        QStringLiteral(
            "%1/api/sanitization-requests/employee")
            .arg(
                QString::fromLatin1(
                    kApiBaseUrl)));

    QNetworkRequest request(url);

    request.setRawHeader(
        "Authorization",
        QByteArray("Bearer ") +
            token.toUtf8());

    request.setRawHeader(
        "Accept",
        "application/json");

    QNetworkReply *reply =
        networkManager_->get(request);

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply]()
        {
            const int statusCode =
                reply->attribute(
                    QNetworkRequest::
                        HttpStatusCodeAttribute)
                    .toInt();

            const QByteArray body =
                reply->readAll();

            if (reply->error() !=
                QNetworkReply::NoError)
            {
                emit requestFetchFailed(
                    QStringLiteral(
                        "Unable to fetch assigned jobs "
                        "(HTTP %1): %2")
                        .arg(
                            statusCode)
                        .arg(
                            reply->errorString()));

                reply->deleteLater();
                return;
            }

            QJsonParseError parseError{};

            const QJsonDocument document =
                QJsonDocument::fromJson(
                    body,
                    &parseError);

            if (parseError.error !=
                    QJsonParseError::NoError ||
                !document.isObject())
            {
                emit requestFetchFailed(
                    QStringLiteral(
                        "Assigned-job API returned invalid JSON."));

                reply->deleteLater();
                return;
            }

            const QJsonObject response =
                document.object();

            if (!response.value(
                    QStringLiteral("success"))
                    .toBool())
            {
                emit requestFetchFailed(
                    response.value(
                        QStringLiteral("message"))
                        .toString(
                            QStringLiteral(
                                "Failed to fetch assigned jobs.")));

                reply->deleteLater();
                return;
            }

            const QJsonValue data =
                response.value(
                    QStringLiteral("data"));

            if (!data.isArray())
            {
                emit requestFetchFailed(
                    QStringLiteral(
                        "Assigned-job API returned invalid data."));

                reply->deleteLater();
                return;
            }

            emit assignedRequestsFetched(
                data.toArray());

            reply->deleteLater();
        });
}

void SanitizationRequestService::updateRequestStatus(
    const QString &token,
    const QString &requestId,
    const QString &status,
    const QString &workstationId)
{
    if (token.trimmed().isEmpty())
    {
        emit requestStatusUpdateFailed(
            QStringLiteral(
                "Authentication token is missing."));
        return;
    }

    if (requestId.trimmed().isEmpty())
    {
        emit requestStatusUpdateFailed(
            QStringLiteral(
                "Request ID is missing."));
        return;
    }

    if (status.trimmed().isEmpty())
    {
        emit requestStatusUpdateFailed(
            QStringLiteral(
                "Request status is missing."));
        return;
    }

    if (workstationId.trimmed().isEmpty())
    {
        emit requestStatusUpdateFailed(
            QStringLiteral(
                "Assigned workstation ID is missing."));
        return;
    }

    const QUrl url(
        QStringLiteral(
            "%1/api/sanitization-requests/%2/employee-status")
            .arg(
                QString::fromLatin1(
                    kApiBaseUrl))
            .arg(
                requestId));

    QNetworkRequest request(url);

    request.setRawHeader(
        "Authorization",
        QByteArray("Bearer ") +
            token.toUtf8());

    request.setRawHeader(
        "Accept",
        "application/json");

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QStringLiteral(
            "application/json"));

    QJsonObject body;

    body.insert(
        QStringLiteral("status"),
        status);

    body.insert(
        QStringLiteral("workstationId"),
        workstationId.trimmed());

    const QByteArray requestBody =
        QJsonDocument(body)
            .toJson(
                QJsonDocument::Compact);

    QNetworkReply *reply =
        networkManager_->sendCustomRequest(
            request,
            QByteArray("PATCH"),
            requestBody);

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this,
         reply,
         requestId,
         status]()
        {
            const int statusCode =
                reply->attribute(
                    QNetworkRequest::
                        HttpStatusCodeAttribute)
                    .toInt();

            const QByteArray responseBody =
                reply->readAll();

            if (reply->error() !=
                QNetworkReply::NoError)
            {
                emit requestStatusUpdateFailed(
                    QStringLiteral(
                        "Request %1 could not be moved to %2 "
                        "(HTTP %3): %4")
                        .arg(
                            requestId,
                            status,
                            QString::number(statusCode),
                            reply->errorString()));

                reply->deleteLater();
                return;
            }

            QJsonParseError parseError{};

            const QJsonDocument document =
                QJsonDocument::fromJson(
                    responseBody,
                    &parseError);

            if (parseError.error !=
                    QJsonParseError::NoError ||
                !document.isObject())
            {
                emit requestStatusUpdateFailed(
                    QStringLiteral(
                        "Status update API returned invalid JSON."));

                reply->deleteLater();
                return;
            }

            const QJsonObject response =
                document.object();

            if (!response.value(
                    QStringLiteral("success"))
                    .toBool())
            {
                emit requestStatusUpdateFailed(
                    response.value(
                        QStringLiteral("message"))
                        .toString(
                            QStringLiteral(
                                "Request status update failed.")));

                reply->deleteLater();
                return;
            }

            emit requestStatusUpdated(
                requestId,
                status);

            reply->deleteLater();
        });
}
void SanitizationRequestService::bindWorkstationIdentity(
    const QString &token,
    const QString &workstationId)
{
    if (token.trimmed().isEmpty())
    {
        emit workstationIdentityVerificationFailed(
            QStringLiteral(
                "Authentication token is missing."));
        return;
    }

    const QString normalizedWorkstationId =
        workstationId.trimmed();

    if (normalizedWorkstationId.isEmpty())
    {
        emit workstationIdentityVerificationFailed(
            QStringLiteral(
                "Assigned workstation ID is missing."));
        return;
    }

#ifdef Q_OS_WIN
    QSettings registry(
        QStringLiteral(
            "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography"),
        QSettings::NativeFormat);

    const QString machineGuid =
        registry.value(
            QStringLiteral("MachineGuid"))
            .toString()
            .trimmed();
#else
    const QString machineGuid =
        qEnvironmentVariable(
            "SECUREWIPE_MACHINE_GUID")
            .trimmed();
#endif

    if (machineGuid.isEmpty())
    {
        emit workstationIdentityVerificationFailed(
            QStringLiteral(
                "SecureWipe could not read the local machine identity."));
        return;
    }

    const QString fingerprintSource =
        QStringLiteral(
            "SecureWipe|MachineGuid|%1")
            .arg(machineGuid);

    const QString machineFingerprint =
        QString::fromLatin1(
            QCryptographicHash::hash(
                fingerprintSource.toUtf8(),
                QCryptographicHash::Sha256)
                .toHex());

    const QString legacyMachineFingerprint =
        QString::fromLatin1(
            QCryptographicHash::hash(
                machineGuid.toUtf8(),
                QCryptographicHash::Sha256)
                .toHex());

    const QString hostname =
        QHostInfo::localHostName().trimmed();

    QJsonObject operatingSystem;
    operatingSystem.insert(
        QStringLiteral("name"),
        QSysInfo::prettyProductName());
    operatingSystem.insert(
        QStringLiteral("version"),
        QSysInfo::kernelVersion());
    operatingSystem.insert(
        QStringLiteral("architecture"),
        QSysInfo::currentCpuArchitecture());

    const QUrl url(
        QStringLiteral(
            "%1/api/workstations/identity")
            .arg(
                QString::fromLatin1(
                    kApiBaseUrl)));

    QNetworkRequest request(url);

    request.setRawHeader(
        "Authorization",
        QByteArray("Bearer ") +
            token.toUtf8());

    request.setRawHeader(
        "Accept",
        "application/json");

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QStringLiteral(
            "application/json"));

    QJsonObject body;

    body.insert(
        QStringLiteral("workstationId"),
        normalizedWorkstationId);

    body.insert(
        QStringLiteral("machineFingerprint"),
        machineFingerprint);
    body.insert(
        QStringLiteral("legacyMachineFingerprint"),
        legacyMachineFingerprint);

    body.insert(
        QStringLiteral("hostname"),
        hostname);

    body.insert(
        QStringLiteral("operatingSystem"),
        operatingSystem);

    const QByteArray requestBody =
        QJsonDocument(body)
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
        [this,
         reply,
         normalizedWorkstationId]()
        {
            const int statusCode =
                reply->attribute(
                    QNetworkRequest::
                        HttpStatusCodeAttribute)
                    .toInt();

            const QByteArray responseBody =
                reply->readAll();

            if (reply->error() !=
                QNetworkReply::NoError)
            {
                emit workstationIdentityVerificationFailed(
                    QStringLiteral(
                        "Workstation identity verification failed "
                        "(HTTP %1): %2")
                        .arg(
                            statusCode)
                        .arg(
                            reply->errorString()));

                reply->deleteLater();
                return;
            }

            QJsonParseError parseError{};

            const QJsonDocument document =
                QJsonDocument::fromJson(
                    responseBody,
                    &parseError);

            if (
                parseError.error !=
                    QJsonParseError::NoError ||
                !document.isObject())
            {
                emit workstationIdentityVerificationFailed(
                    QStringLiteral(
                        "Workstation identity API returned invalid JSON."));

                reply->deleteLater();
                return;
            }

            const QJsonObject response =
                document.object();

            if (!response.value(
                    QStringLiteral("success"))
                    .toBool())
            {
                emit workstationIdentityVerificationFailed(
                    response.value(
                        QStringLiteral("message"))
                        .toString(
                            QStringLiteral(
                                "Workstation identity verification failed.")));

                reply->deleteLater();
                return;
            }

            emit workstationIdentityVerified(
                normalizedWorkstationId);

            reply->deleteLater();
        });
}
