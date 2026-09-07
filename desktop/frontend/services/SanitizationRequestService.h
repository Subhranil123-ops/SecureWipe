#pragma once

#include <QObject>
#include <QJsonArray>
#include <QString>
#include <QNetworkAccessManager>

class SanitizationRequestService : public QObject
{
    Q_OBJECT

public:
    explicit SanitizationRequestService(
        QObject *parent = nullptr
    );

    void fetchAssignedRequests(
        const QString &token
    );

    void updateRequestStatus(
        const QString &token,
        const QString &requestId,
        const QString &status
    );

signals:
    void assignedRequestsFetched(
        const QJsonArray &requests
    );

    void requestFetchFailed(
        const QString &message
    );

    void requestStatusUpdated(
        const QString &requestId,
        const QString &status
    );

    void requestStatusUpdateFailed(
        const QString &message
    );

private:
    QNetworkAccessManager *networkManager_;
};