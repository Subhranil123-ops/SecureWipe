#pragma once

#include <QString>
#include <QUrl>

namespace SecureWipe::AppConfig
{

inline QString apiBaseUrl()
{
#ifdef SECUREWIPE_API_BASE_URL
    return QStringLiteral(SECUREWIPE_API_BASE_URL);
#else
    return QStringLiteral("http://localhost:5000");
#endif
}

inline QUrl apiUrl(const QString &path)
{
    QString base = apiBaseUrl().trimmed();

    while (base.endsWith('/'))
    {
        base.chop(1);
    }

    QString normalizedPath = path.trimmed();

    if (!normalizedPath.startsWith('/'))
    {
        normalizedPath.prepend('/');
    }

    return QUrl(base + normalizedPath);
}

}