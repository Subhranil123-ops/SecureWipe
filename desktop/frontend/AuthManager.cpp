#include "AuthManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

AuthManager::AuthManager(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
{
}

void AuthManager::login(
    const QString &email,
    const QString &password)
{
    const QUrl url(
        QStringLiteral("http://localhost:5000/api/auth/login"));

    QNetworkRequest request(url);

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QStringLiteral("application/json"));

    request.setRawHeader(
        "Accept",
        "application/json");

    QJsonObject json;

    json.insert(
        QStringLiteral("email"),
        email);

    json.insert(
        QStringLiteral("password"),
        password);

    QNetworkReply *reply =
        networkManager->post(
            request,
            QJsonDocument(json).toJson());

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply]()
        {
            const int statusCode =
                reply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute)
                    .toInt();

            const QByteArray responseData =
                reply->readAll();

            if (reply->error() !=
                QNetworkReply::NoError)
            {
                QString message =
                    reply->errorString();

                if (statusCode > 0)
                {
                    message =
                        QStringLiteral(
                            "Login failed (HTTP %1): %2")
                            .arg(statusCode)
                            .arg(message);
                }

                clearToken();

                qDebug()
                    << "Login request failed:"
                    << message;

                emit loginFailed(message);

                reply->deleteLater();
                return;
            }

            QJsonParseError parseError;

            const QJsonDocument document =
                QJsonDocument::fromJson(
                    responseData,
                    &parseError);

            if (
                parseError.error !=
                    QJsonParseError::NoError ||
                !document.isObject()
            )
            {
                clearToken();

                const QString message =
                    QStringLiteral(
                        "Login failed: server returned invalid JSON.");

                qDebug()
                    << message
                    << parseError.errorString();

                emit loginFailed(message);

                reply->deleteLater();
                return;
            }

            const QJsonObject responseObject =
                document.object();

            const QString receivedToken =
                responseObject
                    .value(QStringLiteral("token"))
                    .toString()
                    .trimmed();

            const QJsonObject userObject =
                responseObject
                    .value(QStringLiteral("user"))
                    .toObject();

            const QString receivedRole =
                userObject
                    .value(QStringLiteral("role"))
                    .toString()
                    .trimmed();

            const QString receivedUserId =
                userObject
                    .value(QStringLiteral("_id"))
                    .toString()
                    .trimmed()
                    .isEmpty()
                    ? userObject
                          .value(QStringLiteral("id"))
                          .toString()
                          .trimmed()
                    : userObject
                          .value(QStringLiteral("_id"))
                          .toString()
                          .trimmed();

            const QString receivedName =
                userObject
                    .value(QStringLiteral("name"))
                    .toString()
                    .trimmed();

            const QString receivedEmail =
                userObject
                    .value(QStringLiteral("email"))
                    .toString()
                    .trimmed();

            QString receivedWorkstationCenter;

            const QJsonValue workstationCenterValue =
                userObject.value(
                    QStringLiteral("workstationCenter"));

            if (workstationCenterValue.isObject())
            {
                const QJsonObject centerObject =
                    workstationCenterValue.toObject();

                receivedWorkstationCenter =
                    centerObject
                        .value(QStringLiteral("_id"))
                        .toString()
                        .trimmed();

                if (receivedWorkstationCenter.isEmpty())
                {
                    receivedWorkstationCenter =
                        centerObject
                            .value(QStringLiteral("id"))
                            .toString()
                            .trimmed();
                }
            }
            else
            {
                receivedWorkstationCenter =
                    workstationCenterValue
                        .toString()
                        .trimmed();
            }

            if (receivedToken.isEmpty())
            {
                clearToken();

                emit loginFailed(
                    QStringLiteral(
                        "Login failed: server did not return an authentication token."));

                reply->deleteLater();
                return;
            }

            if (receivedRole !=
                    QStringLiteral("WORKSTATION_EMPLOYEE") &&
                receivedRole !=
                    QStringLiteral("ADMIN") &&
                receivedRole !=
                    QStringLiteral("WORKSTATION_HEAD"))
            {
                clearToken();

                emit loginFailed(
                    QStringLiteral(
                        "Only authorized workstation roles can access SecureWipe desktop."));

                reply->deleteLater();
                return;
            }

            token_ =
                receivedToken;

            role_ =
                receivedRole;

            userId_ =
                receivedUserId;

            name_ =
                receivedName;

            email_ =
                receivedEmail;

            workstationCenterId_ =
                receivedWorkstationCenter;

            qDebug()
                << "SecureWipe login successful."
                << "Role:"
                << role_
                << "User ID:"
                << userId_
                << "Workstation Center:"
                << workstationCenterId_;

            emit loginSuccessful();

            reply->deleteLater();
        });
}

QString AuthManager::token() const
{
    return token_;
}

QString AuthManager::role() const
{
    return role_;
}

QString AuthManager::userId() const
{
    return userId_;
}

QString AuthManager::name() const
{
    return name_;
}

QString AuthManager::email() const
{
    return email_;
}

QString AuthManager::workstationCenterId() const
{
    return workstationCenterId_;
}

void AuthManager::clearToken()
{
    token_.clear();
    role_.clear();
    userId_.clear();
    name_.clear();
    email_.clear();
    workstationCenterId_.clear();
}