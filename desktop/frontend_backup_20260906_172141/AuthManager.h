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

    void clearToken();

signals:
    void loginSuccessful();
    void loginFailed(const QString &message);

private:
    QNetworkAccessManager *networkManager;
    QString token_;
};

#endif // AUTHMANAGER_H