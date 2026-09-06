#include "SanitizationRequestService.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>


SanitizationRequestService::SanitizationRequestService(
    QObject *parent
)
    : QObject(parent)
    , networkManager(
        new QNetworkAccessManager(this)
    )
{
}


void SanitizationRequestService::fetchAssignedRequests(
    const QString &token
)
{
    if (token.trimmed().isEmpty())
    {
        emit requestFetchFailed(
            QStringLiteral(
                "Employee authentication token is missing."
            )
        );

        return;
    }


    const QUrl url(
        QStringLiteral(
            "http://localhost:5000/api/"
            "sanitization-requests/employee"
        )
    );


    QNetworkRequest request(url);

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QStringLiteral(
            "application/json"
        )
    );

    request.setRawHeader(
        "Authorization",
        QByteArray("Bearer ")
            + token.toUtf8()
    );


    QNetworkReply *reply =
        networkManager->get(request);


    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply]()
        {
            const int statusCode =
                reply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute
                ).toInt();


            if (reply->error()
                != QNetworkReply::NoError)
            {
                emit requestFetchFailed(
                    QStringLiteral(
                        "Unable to fetch assigned requests. "
                    )
                    + reply->errorString()
                    + QStringLiteral(
                        " (HTTP "
                    )
                    + QString::number(statusCode)
                    + QStringLiteral(")")
                );

                reply->deleteLater();

                return;
            }


            const QByteArray responseData =
                reply->readAll();


            QJsonParseError parseError;

            const QJsonDocument document =
                QJsonDocument::fromJson(
                    responseData,
                    &parseError
                );


            if (
                parseError.error
                != QJsonParseError::NoError
            )
            {
                emit requestFetchFailed(
                    QStringLiteral(
                        "Invalid response received "
                        "from assigned-request API."
                    )
                );

                reply->deleteLater();

                return;
            }


            if (!document.isObject())
            {
                emit requestFetchFailed(
                    QStringLiteral(
                        "Invalid assigned-request response."
                    )
                );

                reply->deleteLater();

                return;
            }


            const QJsonObject responseObject =
                document.object();


            if (
                !responseObject
                    .value(
                        QStringLiteral("success")
                    )
                    .toBool()
            )
            {
                emit requestFetchFailed(
                    responseObject
                        .value(
                            QStringLiteral("message")
                        )
                        .toString(
                            QStringLiteral(
                                "Failed to fetch assigned requests."
                            )
                        )
                );

                reply->deleteLater();

                return;
            }


            const QJsonValue dataValue =
                responseObject.value(
                    QStringLiteral("data")
                );


            if (!dataValue.isArray())
            {
                emit requestFetchFailed(
                    QStringLiteral(
                        "Assigned-request data is invalid."
                    )
                );

                reply->deleteLater();

                return;
            }


            emit assignedRequestsFetched(
                dataValue.toArray()
            );


            reply->deleteLater();
        }
    );
}

void SanitizationRequestService::updateRequestStatus(
    const QString &token,
    const QString &requestId,
    const QString &status)
{
    if (token.isEmpty())
    {
        emit requestStatusUpdateFailed(
            QStringLiteral("Authentication token is missing.")
        );
        return;
    }

    if (requestId.isEmpty())
    {
        emit requestStatusUpdateFailed(
            QStringLiteral("Request ID is missing.")
        );
        return;
    }

    QNetworkRequest request(
        QUrl(
            QStringLiteral(
                "http://localhost:5000/api/sanitization-requests/%1/employee-status"
            ).arg(requestId)
        )
    );

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QStringLiteral("application/json")
    );

    request.setRawHeader(
        "Authorization",
        QByteArray("Bearer ") + token.toUtf8()
    );

    QJsonObject body;
    body.insert(
        QStringLiteral("status"),
        status
    );

    QNetworkReply *reply =
        networkManager->sendCustomRequest(
            request,
            "PATCH",
            QJsonDocument(body).toJson(
                QJsonDocument::Compact
            )
        );

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply, requestId, status]()
        {
            const int statusCode =
                reply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute
                ).toInt();

            if (reply->error() != QNetworkReply::NoError)
            {
                emit requestStatusUpdateFailed(
                    QStringLiteral(
                        "Status update failed (%1): %2"
                    )
                    .arg(statusCode)
                    .arg(reply->errorString())
                );

                reply->deleteLater();
                return;
            }

            const QJsonDocument document =
                QJsonDocument::fromJson(
                    reply->readAll()
                );

            if (!document.isObject())
            {
                emit requestStatusUpdateFailed(
                    QStringLiteral(
                        "Invalid response from server."
                    )
                );

                reply->deleteLater();
                return;
            }

            const QJsonObject response =
                document.object();

            if (!response.value(
                    QStringLiteral("success")
                ).toBool())
            {
                emit requestStatusUpdateFailed(
                    response.value(
                        QStringLiteral("message")
                    ).toString(
                        QStringLiteral(
                            "Request status update failed."
                        )
                    )
                );

                reply->deleteLater();
                return;
            }

            emit requestStatusUpdated(
                requestId,
                status
            );

            reply->deleteLater();
        }
    );
}