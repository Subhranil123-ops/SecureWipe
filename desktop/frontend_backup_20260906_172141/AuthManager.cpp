#include "AuthManager.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

AuthManager::AuthManager(QObject *parent)
    : QObject(parent)
{
    networkManager = new QNetworkAccessManager(this);
}

void AuthManager::login(const QString &email, const QString &password)
{
    QUrl url("http://localhost:5000/api/auth/login");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["email"] = email;
    json["password"] = password;

    QJsonDocument document(json);

    QNetworkReply *reply = networkManager->post(
        request,
        document.toJson()
    );
    connect(reply, &QNetworkReply::finished, this, [this,reply]() {
        qDebug() << "Login request finished";
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Network error:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        QByteArray responseData = reply->readAll();

        qDebug() << "Server response:" << responseData;
        QJsonParseError parseError;

        QJsonDocument document =
            QJsonDocument::fromJson(responseData, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << parseError.errorString();
            reply->deleteLater();
            return;
        }

        QJsonObject responseObject = document.object();
        QString receivedToken = responseObject["token"].toString();
        token_ = receivedToken;
        QJsonObject userObject = responseObject["user"].toObject();
        QString role = userObject["role"].toString();
        if (role == "WORKSTATION_EMPLOYEE" || role == "ADMIN" || role=="WORKSTATION_HEAD"){
            qDebug() << "Operator access granted";
            emit loginSuccessful();
        } else {
            qDebug() << "Access denied. User is not an WORKSTATION_EMPLOYEE.";
            emit loginFailed("Only workstation operators can access SecureWipe.");
        }
        qDebug() << "User role:" << role;
        qDebug() << "JSON parsed successfully";

        reply->deleteLater();
    });
}

QString AuthManager::token() const
{
    return token_;
}


void AuthManager::clearToken()
{
    token_.clear();
}