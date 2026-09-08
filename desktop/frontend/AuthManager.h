#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>

class AuthManager : public QObject
{
    Q_OBJECT

public:
    explicit AuthManager(QObject *parent = nullptr);

    void login(
        const QString &email,
        const QString &password
    );

    QString token() const;
    QString role() const;
    QString userId() const;
    QString name() const;
    QString email() const;
    QString workstationCenterId() const;

    void clearToken();

signals:
    void loginSuccessful();
    void loginFailed(const QString &message);

private:
    QNetworkAccessManager *networkManager;

    QString token_;
    QString role_;
    QString userId_;
    QString name_;
    QString email_;
    QString workstationCenterId_;
};

#endif // AUTHMANAGER_H