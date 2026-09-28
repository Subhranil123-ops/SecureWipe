#include "ForensicService.h"

#include "../AppConfig.h"

#include "../../backend/progress/include/LiveProgressReporter.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUuid>

#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

namespace
{

QString utcTimestamp()
{
    return QDateTime::currentDateTimeUtc()
        .toString(
            QStringLiteral(
                "yyyy-MM-ddTHH:mm:ss'Z'"));
}

QString normalizeCanonicalValue(
    const QString &value)
{
    QString normalized =
        value;

    normalized.replace(
        QRegularExpression(
            QStringLiteral(
                "[\\r\\n\\0]")),
        QStringLiteral(" "));

    return normalized;
}

QByteArray sha256(
    const QByteArray &value)
{
    return QCryptographicHash::hash(
        value,
        QCryptographicHash::Sha256);
}

QString sha256Hex(
    const QByteArray &value)
{
    return QString::fromLatin1(
        sha256(value)
            .toHex());
}

QJsonObject certificateArtifactFromItem(
    const EvidenceItem &item)
{
    QJsonObject object;

    object.insert(
        QStringLiteral("artifactId"),
        QString::fromStdString(
            item.artifactId));

    object.insert(
        QStringLiteral("fileName"),
        QString::fromStdString(
            item.fileName));

    object.insert(
        QStringLiteral("fileType"),
        QString::fromStdString(
            item.fileType));

    object.insert(
        QStringLiteral("offset"),
        static_cast<qint64>(
            item.offset));

    object.insert(
        QStringLiteral("size"),
        static_cast<qint64>(
            item.size));

    object.insert(
        QStringLiteral("confidenceScore"),
        item.confidenceScore);

    object.insert(
        QStringLiteral("confidenceLevel"),
        QString::fromStdString(
            item.getConfidenceString()));

    object.insert(
        QStringLiteral("sha256"),
        QString::fromStdString(
            item.sha256));

    object.insert(
        QStringLiteral("validated"),
        item.validated);

    object.insert(
        QStringLiteral("headerValid"),
        item.headerValid);

    object.insert(
        QStringLiteral("footerValid"),
        item.footerValid);

    object.insert(
        QStringLiteral("structureValid"),
        item.structureValid);

    object.insert(
        QStringLiteral("sizeValid"),
        item.sizeValid);

    object.insert(
        QStringLiteral("decodable"),
        item.decodable);

    return object;
}

QByteArray canonicalCertificate(
    const QJsonObject &certificate)
{
    struct ArtifactEntry
    {
        QString id;
        QJsonObject object;
    };

    QVector<ArtifactEntry>
        artifacts;

    const QJsonArray sourceArtifacts =
        certificate
            .value(
                QStringLiteral(
                    "artifacts"))
            .toArray();

    artifacts.reserve(
        sourceArtifacts.size());

    for (
        const QJsonValue &value :
        sourceArtifacts)
    {
        if (!value.isObject())
        {
            continue;
        }

        const QJsonObject object =
            value.toObject();

        artifacts.append(
            {
                object
                    .value(
                        QStringLiteral(
                            "artifactId"))
                    .toString(),
                object
            });
    }

    std::sort(
        artifacts.begin(),
        artifacts.end(),
        [](const ArtifactEntry &left,
           const ArtifactEntry &right)
        {
            return left.id <
                   right.id;
        });

    QStringList lines;

    lines.append(
        QStringLiteral(
            "schemaVersion=1"));

    lines.append(
        QStringLiteral(
            "certificateId=%1")
            .arg(
                normalizeCanonicalValue(
                    certificate.value(
                        QStringLiteral(
                            "certificateId"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "runId=%1")
            .arg(
                normalizeCanonicalValue(
                    certificate.value(
                        QStringLiteral(
                            "runId"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "caseId=%1")
            .arg(
                normalizeCanonicalValue(
                    certificate.value(
                        QStringLiteral(
                            "caseId"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "workstationId=%1")
            .arg(
                normalizeCanonicalValue(
                    certificate.value(
                        QStringLiteral(
                            "workstationId"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "sourceIdentifier=%1")
            .arg(
                normalizeCanonicalValue(
                    certificate.value(
                        QStringLiteral(
                            "sourceIdentifier"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "sourceName=%1")
            .arg(
                normalizeCanonicalValue(
                    certificate.value(
                        QStringLiteral(
                            "sourceName"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "model=%1")
            .arg(
                normalizeCanonicalValue(
                    certificate.value(
                        QStringLiteral(
                            "model"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "serialNumber=%1")
            .arg(
                normalizeCanonicalValue(
                    certificate.value(
                        QStringLiteral(
                            "serialNumber"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "capacityBytes=%1")
            .arg(
                certificate.value(
                    QStringLiteral(
                        "capacityBytes"))
                    .toVariant()
                    .toULongLong()));

    lines.append(
        QStringLiteral(
            "interfaceType=%1")
            .arg(
                normalizeCanonicalValue(
                    certificate.value(
                        QStringLiteral(
                            "interfaceType"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "bytesScanned=%1")
            .arg(
                certificate.value(
                    QStringLiteral(
                        "bytesScanned"))
                    .toVariant()
                    .toULongLong()));

    lines.append(
        QStringLiteral(
            "totalBytes=%1")
            .arg(
                certificate.value(
                    QStringLiteral(
                        "totalBytes"))
                    .toVariant()
                    .toULongLong()));

    lines.append(
        QStringLiteral(
            "candidatesFound=%1")
            .arg(
                certificate.value(
                    QStringLiteral(
                        "candidatesFound"))
                    .toVariant()
                    .toULongLong()));

    lines.append(
        QStringLiteral(
            "recoveredArtifacts=%1")
            .arg(
                certificate.value(
                    QStringLiteral(
                        "recoveredArtifacts"))
                    .toVariant()
                    .toULongLong()));

    lines.append(
        QStringLiteral(
            "validatedArtifacts=%1")
            .arg(
                certificate.value(
                    QStringLiteral(
                        "validatedArtifacts"))
                    .toVariant()
                    .toULongLong()));

    lines.append(
        QStringLiteral(
            "rejectedArtifacts=%1")
            .arg(
                certificate.value(
                    QStringLiteral(
                        "rejectedArtifacts"))
                    .toVariant()
                    .toULongLong()));

    lines.append(
        QStringLiteral(
            "highConfidenceArtifacts=%1")
            .arg(
                certificate.value(
                    QStringLiteral(
                        "highConfidenceArtifacts"))
                    .toVariant()
                    .toULongLong()));

    lines.append(
        QStringLiteral(
            "recoveredBytes=%1")
            .arg(
                certificate.value(
                    QStringLiteral(
                        "recoveredBytes"))
                    .toVariant()
                    .toULongLong()));

    lines.append(
        QStringLiteral(
            "auditAnchorHash=%1")
            .arg(
                normalizeCanonicalValue(
                    certificate.value(
                        QStringLiteral(
                            "auditAnchorHash"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "generatedAt=%1")
            .arg(
                normalizeCanonicalValue(
                    certificate.value(
                        QStringLiteral(
                            "generatedAt"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "hashAlgorithm=%1")
            .arg(
                normalizeCanonicalValue(
                    certificate.value(
                        QStringLiteral(
                            "hashAlgorithm"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "artifactCount=%1")
            .arg(
                artifacts.size()));

    for (
        const ArtifactEntry &entry :
        artifacts)
    {
        const QJsonObject &artifact =
            entry.object;

        lines.append(
            QStringLiteral(
                "artifact.artifactId=%1")
                .arg(
                    normalizeCanonicalValue(
                        artifact.value(
                            QStringLiteral(
                                "artifactId"))
                            .toString())));

        lines.append(
            QStringLiteral(
                "artifact.fileName=%1")
                .arg(
                    normalizeCanonicalValue(
                        artifact.value(
                            QStringLiteral(
                                "fileName"))
                            .toString())));

        lines.append(
            QStringLiteral(
                "artifact.fileType=%1")
                .arg(
                    normalizeCanonicalValue(
                        artifact.value(
                            QStringLiteral(
                                "fileType"))
                            .toString())));

        lines.append(
            QStringLiteral(
                "artifact.offset=%1")
                .arg(
                    artifact.value(
                        QStringLiteral(
                            "offset"))
                        .toVariant()
                        .toULongLong()));

        lines.append(
            QStringLiteral(
                "artifact.size=%1")
                .arg(
                    artifact.value(
                        QStringLiteral(
                            "size"))
                        .toVariant()
                        .toULongLong()));

        lines.append(
            QStringLiteral(
                "artifact.confidenceScore=%1")
                .arg(
                    artifact.value(
                        QStringLiteral(
                            "confidenceScore"))
                        .toInt()));

        lines.append(
            QStringLiteral(
                "artifact.confidenceLevel=%1")
                .arg(
                    normalizeCanonicalValue(
                        artifact.value(
                            QStringLiteral(
                                "confidenceLevel"))
                            .toString())));

        lines.append(
            QStringLiteral(
                "artifact.sha256=%1")
                .arg(
                    normalizeCanonicalValue(
                        artifact.value(
                            QStringLiteral(
                                "sha256"))
                            .toString())));

        lines.append(
            QStringLiteral(
                "artifact.validated=%1")
                .arg(
                    artifact.value(
                        QStringLiteral(
                            "validated"))
                        .toBool()
                        ? QStringLiteral(
                              "true")
                        : QStringLiteral(
                              "false")));

        lines.append(
            QStringLiteral(
                "artifact.headerValid=%1")
                .arg(
                    artifact.value(
                        QStringLiteral(
                            "headerValid"))
                        .toBool()
                        ? QStringLiteral(
                              "true")
                        : QStringLiteral(
                              "false")));

        lines.append(
            QStringLiteral(
                "artifact.footerValid=%1")
                .arg(
                    artifact.value(
                        QStringLiteral(
                            "footerValid"))
                        .toBool()
                        ? QStringLiteral(
                              "true")
                        : QStringLiteral(
                              "false")));

        lines.append(
            QStringLiteral(
                "artifact.structureValid=%1")
                .arg(
                    artifact.value(
                        QStringLiteral(
                            "structureValid"))
                        .toBool()
                        ? QStringLiteral(
                              "true")
                        : QStringLiteral(
                              "false")));

        lines.append(
            QStringLiteral(
                "artifact.sizeValid=%1")
                .arg(
                    artifact.value(
                        QStringLiteral(
                            "sizeValid"))
                        .toBool()
                        ? QStringLiteral(
                              "true")
                        : QStringLiteral(
                              "false")));

        lines.append(
            QStringLiteral(
                "artifact.decodable=%1")
                .arg(
                    artifact.value(
                        QStringLiteral(
                            "decodable"))
                        .toBool()
                        ? QStringLiteral(
                              "true")
                        : QStringLiteral(
                              "false")));
    }

    /*
     * Backend canonicalCertificate() appends
     * exactly one trailing newline.
     */
    return (
        lines.join(
            QStringLiteral(
                "\n")) +
        QStringLiteral(
            "\n"))
        .toUtf8();
}

QByteArray canonicalAuditEvent(
    const QString &caseId,
    const QString &runId,
    const QString &workstationId,
    const QString &sourceIdentifier,
    const QJsonObject &event)
{
    QStringList lines;

    lines.append(
        QStringLiteral(
            "schemaVersion=1"));

    lines.append(
        QStringLiteral(
            "caseId=%1")
            .arg(
                normalizeCanonicalValue(
                    caseId)));

    lines.append(
        QStringLiteral(
            "runId=%1")
            .arg(
                normalizeCanonicalValue(
                    runId)));

    lines.append(
        QStringLiteral(
            "workstationId=%1")
            .arg(
                normalizeCanonicalValue(
                    workstationId)));

    lines.append(
        QStringLiteral(
            "sourceIdentifier=%1")
            .arg(
                normalizeCanonicalValue(
                    sourceIdentifier)));

    lines.append(
        QStringLiteral(
            "sequence=%1")
            .arg(
                event.value(
                    QStringLiteral(
                        "sequence"))
                    .toInt()));

    lines.append(
        QStringLiteral(
            "eventType=%1")
            .arg(
                normalizeCanonicalValue(
                    event.value(
                        QStringLiteral(
                            "eventType"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "artifactId=%1")
            .arg(
                normalizeCanonicalValue(
                    event.value(
                        QStringLiteral(
                            "artifactId"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "timestampUtc=%1")
            .arg(
                normalizeCanonicalValue(
                    event.value(
                        QStringLiteral(
                            "timestampUtc"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "details=%1")
            .arg(
                normalizeCanonicalValue(
                    event.value(
                        QStringLiteral(
                            "details"))
                        .toString())));

    lines.append(
        QStringLiteral(
            "previousEventHash=%1")
            .arg(
                normalizeCanonicalValue(
                    event.value(
                        QStringLiteral(
                            "previousEventHash"))
                        .toString())));

    /*
     * Backend canonicalAuditEvent() has an empty line
     * after previousEventHash.
     */
    lines.append(
        QString());

    return lines.join(
        QStringLiteral(
            "\n"))
        .toUtf8();
}

QJsonObject makeAuditEvent(
    const QString &caseId,
    const QString &runId,
    const QString &workstationId,
    const QString &sourceIdentifier,
    int sequence,
    const QString &eventType,
    const QString &artifactId,
    const QString &details,
    const QString &previousEventHash)
{
    QJsonObject event;

    event.insert(
        QStringLiteral("sequence"),
        sequence);

    event.insert(
        QStringLiteral("eventType"),
        eventType);

    event.insert(
        QStringLiteral("artifactId"),
        artifactId);

    event.insert(
        QStringLiteral("timestampUtc"),
        utcTimestamp());

    event.insert(
        QStringLiteral("details"),
        details);

    event.insert(
        QStringLiteral("previousEventHash"),
        previousEventHash);

    const QByteArray canonical =
        canonicalAuditEvent(
            caseId,
            runId,
            workstationId,
            sourceIdentifier,
            event);

    event.insert(
        QStringLiteral("eventHash"),
        sha256Hex(
            canonical));

    return event;
}

QString createRunId()
{
    return QStringLiteral(
               "FRUN-%1")
        .arg(
            QUuid::createUuid()
                .toString(
                    QUuid::WithoutBraces));
}

QByteArray readFileBytes(
    const QString &path,
    QString &errorMessage)
{
    QFile file(path);

    if (!file.open(
            QIODevice::ReadOnly))
    {
        errorMessage =
            QStringLiteral(
                "Unable to open recovered artifact: %1")
                .arg(path);

        return {};
    }

    const QByteArray data =
        file.readAll();

    if (
        file.error() !=
        QFileDevice::NoError)
    {
        errorMessage =
            QStringLiteral(
                "Unable to read recovered artifact: %1")
                .arg(path);

        return {};
    }

    return data;
}

QString contentMimeType(
    const QString &fileType)
{
    const QString type =
        fileType
            .trimmed()
            .toUpper();

    if (
        type == QStringLiteral(
            "JPEG") ||
        type == QStringLiteral(
            "JPG"))
    {
        return QStringLiteral(
            "image/jpeg");
    }

    if (
        type == QStringLiteral(
            "PNG"))
    {
        return QStringLiteral(
            "image/png");
    }

    if (
        type == QStringLiteral(
            "PDF"))
    {
        return QStringLiteral(
            "application/pdf");
    }

    return QStringLiteral(
        "application/octet-stream");
}

QJsonObject buildNativeCertificate(
    const QString &caseId,
    const QString &workstationId,
    const QString &runId,
    const ForensicCaseInfo &caseInfo,
    const ForensicScanSummary &summary,
    const QVector<EvidenceItem> &results,
    const QString &auditAnchorHash)
{
    QJsonObject certificate;

    const QString certificateId =
        QStringLiteral(
            "FEC-%1")
            .arg(
                QUuid::createUuid()
                    .toString(
                        QUuid::WithoutBraces));

    certificate.insert(
        QStringLiteral(
            "certificateId"),
        certificateId);

    certificate.insert(
        QStringLiteral(
            "runId"),
        runId);

    certificate.insert(
        QStringLiteral(
            "caseId"),
        caseId);

    certificate.insert(
        QStringLiteral(
            "workstationId"),
        workstationId);

    certificate.insert(
        QStringLiteral(
            "sourceIdentifier"),
        caseInfo.sourceIdentifier.trimmed());

    certificate.insert(
        QStringLiteral(
            "sourceName"),
        caseInfo.sourceName.trimmed());

    certificate.insert(
        QStringLiteral(
            "model"),
        caseInfo.deviceType.trimmed());

    /*
     * For a physical acquisition the case sourceIdentifier
     * is the server-assigned physical serial identifier.
     * The backend explicitly verifies that serialNumber
     * equals sourceIdentifier.
     */
    certificate.insert(
        QStringLiteral(
            "serialNumber"),
        caseInfo.sourceIdentifier.trimmed());

    certificate.insert(
        QStringLiteral(
            "capacityBytes"),
        static_cast<qint64>(
            summary.totalBytes));

    certificate.insert(
        QStringLiteral(
            "interfaceType"),
        caseInfo.sourceType.trimmed());

    certificate.insert(
        QStringLiteral(
            "bytesScanned"),
        static_cast<qint64>(
            summary.bytesScanned));

    certificate.insert(
        QStringLiteral(
            "totalBytes"),
        static_cast<qint64>(
            summary.totalBytes));

    certificate.insert(
        QStringLiteral(
            "candidatesFound"),
        static_cast<qint64>(
            summary.candidatesFound));

    certificate.insert(
        QStringLiteral(
            "recoveredArtifacts"),
        static_cast<qint64>(
            summary.recoveredArtifacts));

    certificate.insert(
        QStringLiteral(
            "validatedArtifacts"),
        static_cast<qint64>(
            summary.validatedArtifacts));

    certificate.insert(
        QStringLiteral(
            "rejectedArtifacts"),
        static_cast<qint64>(
            summary.rejectedArtifacts));

    certificate.insert(
        QStringLiteral(
            "highConfidenceArtifacts"),
        static_cast<qint64>(
            summary.highConfidenceArtifacts));

    certificate.insert(
        QStringLiteral(
            "recoveredBytes"),
        static_cast<qint64>(
            summary.recoveredBytes));

    certificate.insert(
        QStringLiteral(
            "auditAnchorHash"),
        auditAnchorHash);

    certificate.insert(
        QStringLiteral(
            "generatedAt"),
        utcTimestamp());

    certificate.insert(
        QStringLiteral(
            "hashAlgorithm"),
        QStringLiteral(
            "SHA-256"));

    QJsonArray artifacts;

    for (
        const EvidenceItem &item :
        results)
    {
        artifacts.append(
            certificateArtifactFromItem(
                item));
    }

    certificate.insert(
        QStringLiteral(
            "artifactCount"),
        artifacts.size());

    certificate.insert(
        QStringLiteral(
            "artifacts"),
        artifacts);

    const QByteArray canonical =
        canonicalCertificate(
            certificate);

    certificate.insert(
        QStringLiteral(
            "certificateHash"),
        sha256Hex(
            canonical));

    return certificate;
}

QJsonObject buildEvidencePackage(
    const QString &caseId,
    const QString &workstationId,
    const QString &runId,
    const ForensicCaseInfo &caseInfo,
    const ForensicScanSummary &summary,
    const QVector<EvidenceItem> &results,
    QString &errorMessage)
{
    errorMessage.clear();

    if (
        caseId.trimmed().isEmpty())
    {
        errorMessage =
            QStringLiteral(
                "Forensic case ID is empty.");

        return {};
    }

    if (
        workstationId.trimmed().isEmpty())
    {
        errorMessage =
            QStringLiteral(
                "Workstation ID is empty.");

        return {};
    }

    if (
        runId.trimmed().isEmpty())
    {
        errorMessage =
            QStringLiteral(
                "Forensic run ID is empty.");

        return {};
    }

    if (
        caseInfo.sourceType.trimmed().isEmpty())
    {
        errorMessage =
            QStringLiteral(
                "Forensic case source type is empty.");

        return {};
    }

    if (
        caseInfo.sourceIdentifier.trimmed().isEmpty())
    {
        errorMessage =
            QStringLiteral(
                "Forensic case source identifier is empty.");

        return {};
    }

    if (results.isEmpty())
    {
        errorMessage =
            QStringLiteral(
                "At least one recovered evidence artifact is required.");

        return {};
    }

    /*
     * Backend MAX_UPLOAD_BYTES = 20 MiB.
     * Reject before creating/transmitting an oversized package.
     */
    constexpr quint64 MAX_UPLOAD_BYTES =
        20ULL * 1024ULL * 1024ULL;

    quint64 declaredBytes = 0;

    QVector<QJsonObject>
        artifactObjects;

    artifactObjects.reserve(
        results.size());

    for (
        const EvidenceItem &item :
        results)
    {
        if (
            item.recoveredPath.empty())
        {
            errorMessage =
                QStringLiteral(
                    "Recovered path is empty for artifact %1.")
                    .arg(
                        QString::fromStdString(
                            item.artifactId));

            return {};
        }

        const quint64 size =
            static_cast<quint64>(
                item.size);

        declaredBytes +=
            size;

        if (
            declaredBytes >
            MAX_UPLOAD_BYTES)
        {
            errorMessage =
                QStringLiteral(
                    "Recovered evidence exceeds the backend 20 MiB upload limit.");

            return {};
        }

        QString readError;

        const QByteArray content =
            readFileBytes(
                QString::fromStdString(
                    item.recoveredPath),
                readError);

        if (
            !readError.isEmpty())
        {
            errorMessage =
                readError;

            return {};
        }

        if (
            static_cast<quint64>(
                content.size()) !=
            size)
        {
            errorMessage =
                QStringLiteral(
                    "Artifact %1 content size does not match its declared size.")
                    .arg(
                        QString::fromStdString(
                            item.artifactId));

            return {};
        }

        const QString calculatedHash =
            sha256Hex(
                content);

        const QString claimedHash =
            QString::fromStdString(
                item.sha256)
                .trimmed()
                .toLower();

        if (
            claimedHash.isEmpty())
        {
            errorMessage =
                QStringLiteral(
                    "Artifact %1 has no SHA-256 hash.")
                    .arg(
                        QString::fromStdString(
                            item.artifactId));

            return {};
        }

        if (
            calculatedHash !=
            claimedHash)
        {
            errorMessage =
                QStringLiteral(
                    "Artifact %1 SHA-256 does not match the recovered file.")
                    .arg(
                        QString::fromStdString(
                            item.artifactId));

            return {};
        }

        QJsonObject artifact =
            certificateArtifactFromItem(
                item);

        artifact.insert(
            QStringLiteral(
                "recoveredPath"),
            QString::fromStdString(
                item.recoveredPath));

        artifact.insert(
            QStringLiteral(
                "recovered"),
            item.recovered);

        artifact.insert(
            QStringLiteral(
                "validated"),
            item.validated);

        artifact.insert(
            QStringLiteral(
                "contentBase64"),
            QString::fromLatin1(
                content.toBase64()));

        artifact.insert(
            QStringLiteral(
                "contentMimeType"),
            contentMimeType(
                QString::fromStdString(
                    item.fileType)));

        artifactObjects.append(
            artifact);
    }

    /*
     * Native audit event 1.
     */
    QJsonArray nativeEvents;

    QString previousEventHash;

    nativeEvents.append(
        makeAuditEvent(
            caseId,
            runId,
            workstationId,
            caseInfo.sourceIdentifier.trimmed(),
            1,
            QStringLiteral(
                "ACQUISITION_STARTED"),
            QString(),
            QStringLiteral(
                "sourceModel=%1;sourceSerial=%2;sourceCapacityBytes=%3;sourceInterface=%4")
                .arg(
                    caseInfo.deviceType.trimmed(),
                    caseInfo.sourceIdentifier.trimmed(),
                    QString::number(
                        summary.totalBytes),
                    caseInfo.sourceType.trimmed()),
            previousEventHash));

    previousEventHash =
        nativeEvents
            .last()
            .toObject()
            .value(
                QStringLiteral(
                    "eventHash"))
            .toString();

    /*
     * Native audit event 2.
     */
    nativeEvents.append(
        makeAuditEvent(
            caseId,
            runId,
            workstationId,
            caseInfo.sourceIdentifier.trimmed(),
            2,
            QStringLiteral(
                "SCAN_COMPLETED"),
            QString(),
            QStringLiteral(
                "bytesScanned=%1;totalBytes=%2;candidatesFound=%3;recoveredArtifacts=%4;validatedArtifacts=%5;rejectedArtifacts=%6")
                .arg(
                    QString::number(
                        summary.bytesScanned),
                    QString::number(
                        summary.totalBytes),
                    QString::number(
                        summary.candidatesFound),
                    QString::number(
                        summary.recoveredArtifacts),
                    QString::number(
                        summary.validatedArtifacts),
                    QString::number(
                        summary.rejectedArtifacts)),
            previousEventHash));

    previousEventHash =
        nativeEvents
            .last()
            .toObject()
            .value(
                QStringLiteral(
                    "eventHash"))
            .toString();

    /*
     * One native ARTIFACT_HASHED event per recovered artifact.
     */
    int sequence =
        3;

    for (
        const EvidenceItem &item :
        results)
    {
        nativeEvents.append(
            makeAuditEvent(
                caseId,
                runId,
                workstationId,
                caseInfo.sourceIdentifier.trimmed(),
                sequence++,
                QStringLiteral(
                    "ARTIFACT_HASHED"),
                QString::fromStdString(
                    item.artifactId),
                QStringLiteral(
                    "fileType=%1;size=%2;offset=%3;sha256=%4;validated=%5;confidence=%6")
                    .arg(
                        QString::fromStdString(
                            item.fileType),
                        QString::number(
                            static_cast<quint64>(
                                item.size)),
                        QString::number(
                            static_cast<quint64>(
                                item.offset)),
                        QString::fromStdString(
                            item.sha256),
                        item.validated
                            ? QStringLiteral(
                                  "true")
                            : QStringLiteral(
                                  "false"),
                        QString::fromStdString(
                            item.getConfidenceString())),
                previousEventHash));

        previousEventHash =
            nativeEvents
                .last()
                .toObject()
                .value(
                    QStringLiteral(
                        "eventHash"))
                .toString();
    }

    /*
     * The certificate anchors to the last event BEFORE
     * CERTIFICATE_GENERATED. This avoids a circular hash:
     *
     * certificate -> audit anchor
     * audit certificate event -> certificate hash
     */
    nativeEvents.append(
        makeAuditEvent(
            caseId,
            runId,
            workstationId,
            caseInfo.sourceIdentifier.trimmed(),
            sequence++,
            QStringLiteral(
                "ACQUISITION_SUMMARY"),
            QString(),
            QStringLiteral(
                "highConfidenceArtifacts=%1;recoveredBytes=%2")
                .arg(
                    QString::number(
                        summary.highConfidenceArtifacts),
                    QString::number(
                        summary.recoveredBytes)),
            previousEventHash));

    previousEventHash =
        nativeEvents
            .last()
            .toObject()
            .value(
                QStringLiteral(
                    "eventHash"))
            .toString();

    const QString auditAnchorHash =
        previousEventHash;

    QJsonObject certificate =
        buildNativeCertificate(
            caseId,
            workstationId,
            runId,
            caseInfo,
            summary,
            results,
            auditAnchorHash);

    const QString certificateHash =
        certificate
            .value(
                QStringLiteral(
                    "certificateHash"))
            .toString();

    /*
     * Bind certificate hash into the native audit chain.
     */
    nativeEvents.append(
        makeAuditEvent(
            caseId,
            runId,
            workstationId,
            caseInfo.sourceIdentifier.trimmed(),
            sequence++,
            QStringLiteral(
                "CERTIFICATE_GENERATED"),
            QString(),
            QStringLiteral(
                "certificateId=%1;certificateHash=%2;auditAnchorHash=%3")
                .arg(
                    certificate.value(
                        QStringLiteral(
                            "certificateId"))
                        .toString(),
                    certificateHash,
                    auditAnchorHash),
            previousEventHash));

    previousEventHash =
        nativeEvents
            .last()
            .toObject()
            .value(
                QStringLiteral(
                    "eventHash"))
            .toString();

    /*
     * Final native event.
     */
    nativeEvents.append(
        makeAuditEvent(
            caseId,
            runId,
            workstationId,
            caseInfo.sourceIdentifier.trimmed(),
            sequence,
            QStringLiteral(
                "ACQUISITION_COMPLETED"),
            QString(),
            QStringLiteral(
                "certificateHash=%1;completionMarker=1")
                .arg(
                    certificateHash),
            previousEventHash));

    QJsonObject nativeAudit;

    nativeAudit.insert(
        QStringLiteral(
            "hashAlgorithm"),
        QStringLiteral(
            "SHA-256"));

    nativeAudit.insert(
        QStringLiteral(
            "events"),
        nativeEvents);

    QJsonObject source;

    source.insert(
        QStringLiteral(
            "deviceId"),
        caseInfo.sourceIdentifier.trimmed());

    source.insert(
        QStringLiteral(
            "model"),
        caseInfo.deviceType.trimmed());

    source.insert(
        QStringLiteral(
            "serialNumber"),
        caseInfo.sourceIdentifier.trimmed());

    source.insert(
        QStringLiteral(
            "capacityBytes"),
        static_cast<qint64>(
            summary.totalBytes));

    source.insert(
        QStringLiteral(
            "interfaceType"),
        caseInfo.sourceType.trimmed());

    QJsonObject summaryObject;

    summaryObject.insert(
        QStringLiteral(
            "bytesScanned"),
        static_cast<qint64>(
            summary.bytesScanned));

    summaryObject.insert(
        QStringLiteral(
            "totalBytes"),
        static_cast<qint64>(
            summary.totalBytes));

    summaryObject.insert(
        QStringLiteral(
            "candidatesFound"),
        static_cast<qint64>(
            summary.candidatesFound));

    summaryObject.insert(
        QStringLiteral(
            "recoveredArtifacts"),
        static_cast<qint64>(
            summary.recoveredArtifacts));

    summaryObject.insert(
        QStringLiteral(
            "validatedArtifacts"),
        static_cast<qint64>(
            summary.validatedArtifacts));

    summaryObject.insert(
        QStringLiteral(
            "rejectedArtifacts"),
        static_cast<qint64>(
            summary.rejectedArtifacts));

    summaryObject.insert(
        QStringLiteral(
            "highConfidenceArtifacts"),
        static_cast<qint64>(
            summary.highConfidenceArtifacts));

    summaryObject.insert(
        QStringLiteral(
            "recoveredBytes"),
        static_cast<qint64>(
            summary.recoveredBytes));

    QJsonArray artifacts;

    for (
        const QJsonObject &artifact :
        artifactObjects)
    {
        artifacts.append(
            artifact);
    }

    QJsonObject package;

    package.insert(
        QStringLiteral(
            "schemaVersion"),
        1);

    package.insert(
        QStringLiteral(
            "runId"),
        runId);

    package.insert(
        QStringLiteral(
            "workstationId"),
        workstationId);

    package.insert(
        QStringLiteral(
            "sourceType"),
        caseInfo.sourceType.trimmed());

    package.insert(
        QStringLiteral(
            "sourceIdentifier"),
        caseInfo.sourceIdentifier.trimmed());

    package.insert(
        QStringLiteral(
            "status"),
        QStringLiteral(
            "COMPLETED"));

    package.insert(
        QStringLiteral(
            "source"),
        source);

    package.insert(
        QStringLiteral(
            "summary"),
        summaryObject);

    package.insert(
        QStringLiteral(
            "artifacts"),
        artifacts);

    package.insert(
        QStringLiteral(
            "nativeAudit"),
        nativeAudit);

    package.insert(
        QStringLiteral(
            "certificate"),
        certificate);

    return package;
}

} // namespace

ForensicService::ForensicService(
    QObject *parent)
    : QObject(parent)
    , networkManager_(
          new QNetworkAccessManager(this))
{
    connect(
        &watcher_,
        &QFutureWatcher<
            EvidenceCollectionResult>::finished,
        this,
        [this]()
        {
            try
            {
                const EvidenceCollectionResult result =
                    watcher_.result();

                results_.clear();

                results_.reserve(
                    static_cast<qsizetype>(
                        result.evidence.size()));

                for (
                    const EvidenceItem &item :
                    result.evidence)
                {
                    results_.append(
                        item);
                }

                summary_.sourceOpened =
                    result.summary.sourceOpened;

                summary_.completed =
                    result.summary.completed;

                summary_.bytesScanned =
                    result.summary.bytesScanned;

                summary_.totalBytes =
                    result.summary.totalBytes;

                summary_.candidatesFound =
                    result.summary.candidatesFound;

                summary_.recoveredArtifacts =
                    result.summary.recoveredArtifacts;

                summary_.validatedArtifacts =
                    result.summary.validatedArtifacts;

                summary_.rejectedArtifacts =
                    result.summary.rejectedArtifacts;

                summary_.highConfidenceArtifacts =
                    result.summary.highConfidenceArtifacts;

                summary_.recoveredBytes =
                    result.summary.recoveredBytes;

                if (
                    !summary_.sourceOpened)
                {
                    emit scanFailed(
                        QStringLiteral(
                            "Forensic source could not be opened."));
                    return;
                }

                if (
                    !summary_.completed)
                {
                    emit scanFailed(
                        QStringLiteral(
                            "Forensic scan did not complete."));
                    return;
                }

                emit scanFinished();
            }
            catch (
                const std::exception &exception)
            {
                emit scanFailed(
                    QString::fromLocal8Bit(
                        exception.what()));
            }
            catch (...)
            {
                emit scanFailed(
                    QStringLiteral(
                        "Forensic scan failed unexpectedly."));
            }
        });
}

ForensicService::~ForensicService()
{
    if (
        watcher_.isRunning())
    {
        watcher_.waitForFinished();
    }
}

void ForensicService::setAuthenticationToken(
    const QString &token)
{
    authenticationToken_ =
        token.trimmed();
}

QString ForensicService::authenticationToken() const
{
    return authenticationToken_;
}

void ForensicService::scan(
    const QString &source)
{
    if (
        isRunning())
    {
        return;
    }

    const QString cleanedSource =
        source.trimmed();

    if (
        cleanedSource.isEmpty())
    {
        emit scanFailed(
            QStringLiteral(
                "Forensic source path is empty."));
        return;
    }

    results_.clear();

    summary_ =
        ForensicScanSummary{};

    lastSource_ =
        cleanedSource;

    /*
     * A new immutable run identifier is generated before
     * the worker thread starts.
     */
    currentRunId_ =
        createRunId();

    const QString runId =
        currentRunId_;

    const QString apiBaseUrl =
        SecureWipe::AppConfig::
            apiBaseUrl(
                QStringLiteral(""))
            .toString();

    const QString token =
        authenticationToken_;

    const QString resourceId =
        selectedCase_.caseId.trimmed();

    const std::string nativeSource =
        QFile::encodeName(
            cleanedSource)
            .toStdString();

    watcher_.setFuture(
        QtConcurrent::run(
            [
                this,
                nativeSource,
                runId,
                apiBaseUrl,
                token,
                resourceId
            ]()
            {
                EvidenceCollector
                    collector;

                LiveProgressReporter::Config
                    progressConfig;

                progressConfig.baseUrl =
                    apiBaseUrl.toStdString();

                progressConfig.token =
                    token.toStdString();

                progressConfig.minimumUpdateIntervalMs =
                    1000;

                progressConfig.alwaysSendBoundaryProgress =
                    true;

                LiveProgressReporter
                    liveProgressReporter(
                        LiveProgressReporter::
                            OperationType::FORENSIC,
                        resourceId.toStdString(),
                        runId.toStdString(),
                        progressConfig);

                const EvidenceProgressCallback
                    progressCallback =
                        [
                            this,
                            &liveProgressReporter
                        ](
                            std::uint64_t bytesScanned,
                            std::uint64_t totalBytes)
                        {
                            int percentage =
                                -1;

                            if (
                                totalBytes >
                                0)
                            {
                                const std::uint64_t
                                    boundedScanned =
                                        bytesScanned >
                                                totalBytes
                                            ? totalBytes
                                            : bytesScanned;

                                percentage =
                                    static_cast<int>(
                                        (
                                            boundedScanned *
                                            100ULL
                                        ) /
                                        totalBytes);

                                if (
                                    percentage >
                                    100)
                                {
                                    percentage =
                                        100;
                                }

                                liveProgressReporter
                                    .reportForensicProgress(
                                        bytesScanned,
                                        totalBytes,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        "FORENSIC_SCAN",
                                        "Scanning forensic source.");
                            }

                            QMetaObject::invokeMethod(
                                this,
                                [
                                    this,
                                    percentage,
                                    bytesScanned,
                                    totalBytes
                                ]()
                                {
                                    emit scanProgress(
                                        percentage,
                                        static_cast<quint64>(
                                            bytesScanned),
                                        static_cast<quint64>(
                                            totalBytes));
                                },
                                Qt::QueuedConnection);
                        };

                EvidenceCollectionResult
                    result =
                        collector
                            .collectWithSummary(
                                nativeSource,
                                progressCallback);

                if (
                    result.summary.completed)
                {
                    liveProgressReporter
                        .finishForensic(
                            result.summary.bytesScanned,
                            result.summary.totalBytes,
                            result.summary.candidatesFound,
                            result.summary.recoveredArtifacts,
                            result.summary.validatedArtifacts,
                            result.summary.rejectedArtifacts,
                            result.summary.highConfidenceArtifacts,
                            result.summary.recoveredBytes,
                            "FORENSIC_SCAN",
                            "Forensic acquisition completed.");
                }
                else
                {
                    liveProgressReporter
                        .failForensic(
                            result.summary.bytesScanned,
                            result.summary.totalBytes,
                            result.summary.candidatesFound,
                            result.summary.recoveredArtifacts,
                            result.summary.validatedArtifacts,
                            result.summary.rejectedArtifacts,
                            result.summary.highConfidenceArtifacts,
                            result.summary.recoveredBytes,
                            "FORENSIC_SCAN",
                            "Forensic acquisition failed.");
                }

                return result;
            }));
}

bool ForensicService::isRunning() const
{
    return watcher_.isRunning();
}

const QVector<EvidenceItem> &
ForensicService::results() const
{
    return results_;
}

const ForensicScanSummary &
ForensicService::summary() const
{
    return summary_;
}

QString ForensicService::lastSource() const
{
    return lastSource_;
}

QNetworkRequest
ForensicService::createAuthenticatedRequest(
    const QUrl &url) const
{
    QNetworkRequest request(
        url);

    request.setHeader(
        QNetworkRequest::
            ContentTypeHeader,
        QStringLiteral(
            "application/json"));

    request.setRawHeader(
        "Accept",
        "application/json");

    if (
        !authenticationToken_.isEmpty())
    {
        request.setRawHeader(
            "Authorization",
            QByteArray(
                "Bearer ") +
                authenticationToken_
                    .toUtf8());
    }

    return request;
}

void ForensicService::loadAssignedCases()
{
    if (
        authenticationToken_.isEmpty())
    {
        emit casesLoadFailed(
            QStringLiteral(
                "You must be authenticated before loading forensic cases."));
        return;
    }

    const QUrl url =
        SecureWipe::AppConfig::
            apiUrl(
                QStringLiteral(
                    "/api/forensics"));

    QNetworkRequest request =
        createAuthenticatedRequest(
            url);

    QNetworkReply *reply =
        networkManager_->get(
            request);

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

            const QByteArray responseData =
                reply->readAll();

            if (
                reply->error() !=
                QNetworkReply::NoError)
            {
                QString message =
                    reply->errorString();

                if (
                    statusCode >
                    0)
                {
                    message =
                        QStringLiteral(
                            "Could not load forensic cases (HTTP %1): %2")
                            .arg(
                                statusCode)
                            .arg(
                                message);
                }

                emit casesLoadFailed(
                    message);

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
                !document.isObject())
            {
                emit casesLoadFailed(
                    QStringLiteral(
                        "Server returned invalid forensic case JSON."));

                reply->deleteLater();

                return;
            }

            const QJsonObject root =
                document.object();

            const QJsonValue dataValue =
                root.value(
                    QStringLiteral(
                        "data"));

            if (
                !dataValue.isArray())
            {
                emit casesLoadFailed(
                    QStringLiteral(
                        "Server returned an invalid forensic case list."));

                reply->deleteLater();

                return;
            }

            cases_.clear();

            const QJsonArray array =
                dataValue.toArray();

            for (
                const QJsonValue &value :
                array)
            {
                if (
                    !value.isObject())
                {
                    continue;
                }

                cases_.append(
                    caseFromJson(
                        value.toObject()));
            }

            emit casesLoaded();

            reply->deleteLater();
        });
}

void ForensicService::loadCase(
    const QString &caseId)
{
    const QString cleanedCaseId =
        caseId.trimmed();

    if (
        cleanedCaseId.isEmpty())
    {
        emit caseLoadFailed(
            QStringLiteral(
                "Forensic case ID is empty."));

        return;
    }

    if (
        authenticationToken_.isEmpty())
    {
        emit caseLoadFailed(
            QStringLiteral(
                "You must be authenticated before opening a forensic case."));

        return;
    }

    const QUrl url =
        SecureWipe::AppConfig::
            apiUrl(
                QStringLiteral(
                    "/api/forensics/%1")
                    .arg(
                        QString::fromUtf8(
                            QUrl::toPercentEncoding(
                                cleanedCaseId))));

    QNetworkRequest request =
        createAuthenticatedRequest(
            url);

    QNetworkReply *reply =
        networkManager_->get(
            request);

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

            const QByteArray responseData =
                reply->readAll();

            if (
                reply->error() !=
                QNetworkReply::NoError)
            {
                QString message =
                    reply->errorString();

                if (
                    statusCode >
                    0)
                {
                    message =
                        QStringLiteral(
                            "Could not load forensic case (HTTP %1): %2")
                            .arg(
                                statusCode)
                            .arg(
                                message);
                }

                emit caseLoadFailed(
                    message);

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
                !document.isObject())
            {
                emit caseLoadFailed(
                    QStringLiteral(
                        "Server returned invalid forensic case JSON."));

                reply->deleteLater();

                return;
            }

            const QJsonObject root =
                document.object();

            const QJsonValue dataValue =
                root.value(
                    QStringLiteral(
                        "data"));

            if (
                !dataValue.isObject())
            {
                emit caseLoadFailed(
                    QStringLiteral(
                        "Server returned invalid forensic case data."));

                reply->deleteLater();

                return;
            }

            selectedCase_ =
                caseFromJson(
                    dataValue.toObject());

            emit caseLoaded();

            reply->deleteLater();
        });
}

ForensicCaseInfo
ForensicService::caseFromJson(
    const QJsonObject &object) const
{
    ForensicCaseInfo result;

    result.caseId =
        object
            .value(
                QStringLiteral(
                    "caseId"))
            .toString();

    result.title =
        object
            .value(
                QStringLiteral(
                    "title"))
            .toString();

    result.description =
        object
            .value(
                QStringLiteral(
                    "description"))
            .toString();

    result.status =
        object
            .value(
                QStringLiteral(
                    "status"))
            .toString();

    result.sourceType =
        object
            .value(
                QStringLiteral(
                    "sourceType"))
            .toString();

    result.sourceName =
        object
            .value(
                QStringLiteral(
                    "sourceName"))
            .toString();

    result.sourceIdentifier =
        object
            .value(
                QStringLiteral(
                    "sourceIdentifier"))
            .toString();

    result.deviceType =
        object
            .value(
                QStringLiteral(
                    "deviceType"))
            .toString();

    result.capacity =
        object
            .value(
                QStringLiteral(
                    "capacity"))
            .toString();

    result.assetIdentifier =
        object
            .value(
                QStringLiteral(
                    "assetIdentifier"))
            .toString();

    result.readOnly =
        object
            .value(
                QStringLiteral(
                    "readOnly"))
            .toBool(
                true);

    const QJsonValue centerValue =
        object.value(
            QStringLiteral(
                "workstationCenter"));

    if (
        centerValue.isObject())
    {
        const QJsonObject center =
            centerValue.toObject();

        result.workstationCenterId =
            center
                .value(
                    QStringLiteral(
                        "_id"))
                .toString();

        if (
            result.workstationCenterId.isEmpty())
        {
            result.workstationCenterId =
                center
                    .value(
                        QStringLiteral(
                            "id"))
                    .toString();
        }
    }
    else
    {
        result.workstationCenterId =
            centerValue.toString();
    }

    const QJsonValue employeeValue =
        object.value(
            QStringLiteral(
                "assignedEmployee"));

    if (
        employeeValue.isObject())
    {
        const QJsonObject employee =
            employeeValue.toObject();

        result.assignedEmployeeId =
            employee
                .value(
                    QStringLiteral(
                        "_id"))
                .toString();

        if (
            result.assignedEmployeeId.isEmpty())
        {
            result.assignedEmployeeId =
                employee
                    .value(
                        QStringLiteral(
                            "id"))
                    .toString();
        }

        result.assignedEmployeeName =
            employee
                .value(
                    QStringLiteral(
                        "name"))
                .toString();
    }
    else
    {
        result.assignedEmployeeId =
            employeeValue.toString();
    }

    const QJsonValue workstationValue =
        object.value(
            QStringLiteral(
                "assignedWorkstation"));

    if (
        workstationValue.isObject())
    {
        const QJsonObject workstation =
            workstationValue.toObject();

        result.assignedWorkstationMongoId =
            workstation
                .value(
                    QStringLiteral(
                        "_id"))
                .toString();

        if (
            result.assignedWorkstationMongoId.isEmpty())
        {
            result.assignedWorkstationMongoId =
                workstation
                    .value(
                        QStringLiteral(
                            "id"))
                    .toString();
        }

        result.assignedWorkstationId =
            workstation
                .value(
                    QStringLiteral(
                        "workstationId"))
                .toString();

        result.assignedWorkstationName =
            workstation
                .value(
                    QStringLiteral(
                        "name"))
                .toString();
    }
    else
    {
        result.assignedWorkstationMongoId =
            workstationValue.toString();
    }

    return result;
}

bool ForensicService::hasSelectedCase() const
{
    return !selectedCase_.caseId.isEmpty();
}

const ForensicCaseInfo &
ForensicService::selectedCase() const
{
    return selectedCase_;
}

const QVector<ForensicCaseInfo> &
ForensicService::cases() const
{
    return cases_;
}

void ForensicService::startCaseAcquisition(
    const QString &caseId,
    const QString &workstationId)
{
    if (
        authenticationToken_.isEmpty())
    {
        emit caseStatusUpdateFailed(
            QStringLiteral(
                "You must be authenticated before starting acquisition."));

        return;
    }

    if (
        caseId.trimmed().isEmpty())
    {
        emit caseStatusUpdateFailed(
            QStringLiteral(
                "Forensic case ID is required."));

        return;
    }

    if (
        workstationId.trimmed().isEmpty())
    {
        emit caseStatusUpdateFailed(
            QStringLiteral(
                "Assigned workstation ID is required."));

        return;
    }

    updateCaseStatus(
        caseId,
        QStringLiteral(
            "ACQUIRING"),
        QStringLiteral(
            "Forensic acquisition started from SecureWipe desktop."),
        workstationId);
}

void ForensicService::updateCaseStatus(
    const QString &caseId,
    const QString &status,
    const QString &note,
    const QString &workstationId)
{
    const QUrl url =
        SecureWipe::AppConfig::
            apiUrl(
                QStringLiteral(
                    "/api/forensics/%1/status")
                    .arg(
                        QString::fromUtf8(
                            QUrl::toPercentEncoding(
                                caseId.trimmed()))));

    QNetworkRequest request =
        createAuthenticatedRequest(
            url);

    QJsonObject payload;

    payload.insert(
        QStringLiteral(
            "status"),
        status);

    payload.insert(
        QStringLiteral(
            "note"),
        note);

    if (
        !workstationId.trimmed().isEmpty())
    {
        payload.insert(
            QStringLiteral(
                "workstationId"),
            workstationId.trimmed());
    }

    QNetworkReply *reply =
        networkManager_->sendCustomRequest(
            request,
            QByteArray(
                "PATCH"),
            QJsonDocument(
                payload)
                .toJson());

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply, status]()
        {
            const int statusCode =
                reply->attribute(
                    QNetworkRequest::
                        HttpStatusCodeAttribute)
                    .toInt();

            const QByteArray responseData =
                reply->readAll();

            if (
                reply->error() !=
                QNetworkReply::NoError)
            {
                QString message =
                    reply->errorString();

                if (
                    statusCode >
                    0)
                {
                    message =
                        QStringLiteral(
                            "Could not update forensic case status (HTTP %1): %2")
                            .arg(
                                statusCode)
                            .arg(
                                message);
                }

                emit caseStatusUpdateFailed(
                    message);

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
                !document.isObject())
            {
                emit caseStatusUpdateFailed(
                    QStringLiteral(
                        "Server returned invalid status response."));

                reply->deleteLater();

                return;
            }

            const QJsonObject root =
                document.object();

            const QJsonValue dataValue =
                root.value(
                    QStringLiteral(
                        "data"));

            if (
                dataValue.isObject())
            {
                selectedCase_ =
                    caseFromJson(
                        dataValue.toObject());
            }
            else
            {
                selectedCase_.status =
                    status;
            }

            emit caseStatusUpdated(
                status);

            reply->deleteLater();
        });
}

QJsonObject
ForensicService::evidenceItemToJson(
    const EvidenceItem &item) const
{
    QJsonObject object;

    object.insert(
        QStringLiteral(
            "artifactId"),
        QString::fromStdString(
            item.artifactId));

    object.insert(
        QStringLiteral(
            "fileName"),
        QString::fromStdString(
            item.fileName));

    object.insert(
        QStringLiteral(
            "fileType"),
        QString::fromStdString(
            item.fileType));

    object.insert(
        QStringLiteral(
            "offset"),
        static_cast<qint64>(
            item.offset));

    object.insert(
        QStringLiteral(
            "size"),
        static_cast<qint64>(
            item.size));

    object.insert(
        QStringLiteral(
            "recoveredPath"),
        QString::fromStdString(
            item.recoveredPath));

    object.insert(
        QStringLiteral(
            "headerValid"),
        item.headerValid);

    object.insert(
        QStringLiteral(
            "footerValid"),
        item.footerValid);

    object.insert(
        QStringLiteral(
            "structureValid"),
        item.structureValid);

    object.insert(
        QStringLiteral(
            "sizeValid"),
        item.sizeValid);

    object.insert(
        QStringLiteral(
            "decodable"),
        item.decodable);

    object.insert(
        QStringLiteral(
            "confidenceScore"),
        item.confidenceScore);

    object.insert(
        QStringLiteral(
            "confidenceLevel"),
        QString::fromStdString(
            item.getConfidenceString()));

    QJsonArray reasons;

    for (
        const std::string &reason :
        item.confidenceReasons)
    {
        reasons.append(
            QString::fromStdString(
                reason));
    }

    object.insert(
        QStringLiteral(
            "confidenceReasons"),
        reasons);

    object.insert(
        QStringLiteral(
            "sha256"),
        QString::fromStdString(
            item.sha256));

    object.insert(
        QStringLiteral(
            "recovered"),
        item.recovered);

    object.insert(
        QStringLiteral(
            "validated"),
        item.validated);

    return object;
}

void ForensicService::submitResults(
    const QString &caseId,
    const QString &workstationId)
{
    if (
        authenticationToken_.isEmpty())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "You must be authenticated before submitting forensic results."));

        return;
    }

    if (
        caseId.trimmed().isEmpty())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "Forensic case ID is required."));

        return;
    }

    if (
        workstationId.trimmed().isEmpty())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "Assigned workstation ID is required."));

        return;
    }

    if (
        isRunning())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "The forensic scan is still running."));

        return;
    }

    if (
        !summary_.completed)
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "The forensic scan has not completed successfully."));

        return;
    }

    if (
        !summary_.sourceOpened)
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "The forensic source was not opened successfully."));

        return;
    }

    if (
        results_.isEmpty())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "At least one recovered evidence artifact is required."));

        return;
    }

    if (
        selectedCase_.sourceType.trimmed().isEmpty())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "The forensic case has no source type."));

        return;
    }

    if (
        selectedCase_.sourceIdentifier.trimmed().isEmpty())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "The forensic case has no source identifier."));

        return;
    }

    /*
     * The rich evidence-package endpoint is intentionally
     * physical-device bound by the current backend contract.
     *
     * Do not pretend that a forensic-image package has been
     * accepted by the backend when that backend currently
     * requires physical-device source binding.
     */
    if (
        selectedCase_.sourceType.trimmed() !=
        QStringLiteral(
            "PHYSICAL_DEVICE"))
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "The current server evidence-package workflow accepts PHYSICAL_DEVICE acquisition only. "
                "FORENSIC_IMAGE submission requires a separate backend contract."));

        return;
    }

    if (
        currentRunId_.trimmed().isEmpty())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "No forensic acquisition run ID is available. Run the acquisition again before submitting."));

        return;
    }

    QString packageError;

    const QJsonObject package =
        buildEvidencePackage(
            caseId.trimmed(),
            workstationId.trimmed(),
            currentRunId_.trimmed(),
            selectedCase_,
            summary_,
            results_,
            packageError);

    if (
        package.isEmpty())
    {
        emit resultsSubmitFailed(
            packageError.isEmpty()
                ? QStringLiteral(
                      "Unable to construct the native forensic evidence package.")
                : packageError);

        return;
    }

    const QUrl url =
        SecureWipe::AppConfig::
            apiUrl(
                QStringLiteral(
                    "/api/forensics/%1/evidence-package")
                    .arg(
                        QString::fromUtf8(
                            QUrl::toPercentEncoding(
                                caseId.trimmed()))));

    QNetworkRequest request =
        createAuthenticatedRequest(
            url);

    const QByteArray body =
        QJsonDocument(
            package)
            .toJson(
                QJsonDocument::Compact);

    QNetworkReply *reply =
        networkManager_->post(
            request,
            body);

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

            const QByteArray responseData =
                reply->readAll();

            if (
                reply->error() !=
                QNetworkReply::NoError)
            {
                QString message =
                    reply->errorString();

                if (
                    statusCode >
                    0)
                {
                    message =
                        QStringLiteral(
                            "Could not submit forensic evidence package (HTTP %1): %2")
                            .arg(
                                statusCode)
                            .arg(
                                message);
                }

                emit resultsSubmitFailed(
                    message);

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
                !document.isObject())
            {
                emit resultsSubmitFailed(
                    QStringLiteral(
                        "Server returned invalid evidence-package JSON."));

                reply->deleteLater();

                return;
            }

            const QJsonObject root =
                document.object();

            const QJsonValue dataValue =
                root.value(
                    QStringLiteral(
                        "data"));

            if (
                dataValue.isObject())
            {
                selectedCase_ =
                    caseFromJson(
                        dataValue.toObject());
            }

            /*
             * Do NOT emit resultsSubmitted merely because
             * /evidence-package returned 2xx.
             *
             * The backend's rich endpoint has already:
             *
             * - decoded every artifact
             * - checked content size
             * - checked SHA-256
             * - verified native audit chain
             * - verified native certificate
             * - stored recovered bytes in GridFS
             *
             * We now ask the normal case-status endpoint to
             * perform the final workflow transition.
             */
            updateCaseStatus(
                selectedCase_.caseId.isEmpty()
                    ? currentRunId_
                    : selectedCase_.caseId,
                QStringLiteral(
                    "COMPLETED"),
                QStringLiteral(
                    "Native forensic evidence package accepted, cryptographically verified, and stored."),
                selectedCase_.assignedWorkstationMongoId);

            /*
             * updateCaseStatus() emits caseStatusUpdated.
             * The page already reloads the case there.
             *
             * resultsSubmitted is emitted only after the status
             * transition succeeds in the revised status handler.
             */
            reply->deleteLater();
        });
}