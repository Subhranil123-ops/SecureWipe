#include "ForensicService.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

#include <exception>

#include <QtConcurrent/QtConcurrentRun>

ForensicService::ForensicService(
    QObject *parent)
    : QObject(parent)
    , networkManager_(
          new QNetworkAccessManager(this))
{
    connect(
        &watcher_,
        &QFutureWatcher<EvidenceCollectionResult>::finished,
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

                for (const EvidenceItem &item :
                     result.evidence)
                {
                    results_.append(item);
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

                if (!summary_.sourceOpened)
                {
                    emit scanFailed(
                        QStringLiteral(
                            "Forensic source could not be opened."));
                    return;
                }

                if (!summary_.completed)
                {
                    emit scanFailed(
                        QStringLiteral(
                            "Forensic scan did not complete."));
                    return;
                }

                emit scanFinished();
            }
            catch (const std::exception &exception)
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
    if (watcher_.isRunning())
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
    if (isRunning())
    {
        return;
    }

    const QString cleanedSource =
        source.trimmed();

    if (cleanedSource.isEmpty())
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

    const std::string nativeSource =
        QFile::encodeName(
            cleanedSource)
            .toStdString();

    watcher_.setFuture(
        QtConcurrent::run(
            [nativeSource]()
            {
                EvidenceCollector collector;

                return collector.collectWithSummary(
                    nativeSource);
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
    QNetworkRequest request(url);

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QStringLiteral(
            "application/json"));

    request.setRawHeader(
        "Accept",
        "application/json");

    if (!authenticationToken_.isEmpty())
    {
        request.setRawHeader(
            "Authorization",
            QByteArray("Bearer ") +
                authenticationToken_.toUtf8());
    }

    return request;
}

void ForensicService::loadAssignedCases()
{
    if (authenticationToken_.isEmpty())
    {
        emit casesLoadFailed(
            QStringLiteral(
                "You must be authenticated before loading forensic cases."));
        return;
    }

    const QUrl url(
        QStringLiteral(
            "http://localhost:5000/api/forensics"));

    QNetworkRequest request =
        createAuthenticatedRequest(url);

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
                            "Could not load forensic cases (HTTP %1): %2")
                            .arg(statusCode)
                            .arg(message);
                }

                emit casesLoadFailed(message);

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
                    QStringLiteral("data"));

            if (!dataValue.isArray())
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

            for (const QJsonValue &value :
                 array)
            {
                if (!value.isObject())
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

    if (cleanedCaseId.isEmpty())
    {
        emit caseLoadFailed(
            QStringLiteral(
                "Forensic case ID is empty."));
        return;
    }

    if (authenticationToken_.isEmpty())
    {
        emit caseLoadFailed(
            QStringLiteral(
                "You must be authenticated before opening a forensic case."));
        return;
    }

    const QUrl url(
        QStringLiteral(
            "http://localhost:5000/api/forensics/%1")
            .arg(
                QString::fromUtf8(
                    QUrl::toPercentEncoding(
                        cleanedCaseId))));

    QNetworkRequest request =
        createAuthenticatedRequest(url);

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
                            "Could not load forensic case (HTTP %1): %2")
                            .arg(statusCode)
                            .arg(message);
                }

                emit caseLoadFailed(message);

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
                    QStringLiteral("data"));

            if (!dataValue.isObject())
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
            .value(QStringLiteral("caseId"))
            .toString();

    result.title =
        object
            .value(QStringLiteral("title"))
            .toString();

    result.description =
        object
            .value(QStringLiteral("description"))
            .toString();

    result.status =
        object
            .value(QStringLiteral("status"))
            .toString();

    result.sourceType =
        object
            .value(QStringLiteral("sourceType"))
            .toString();

    result.sourceName =
        object
            .value(QStringLiteral("sourceName"))
            .toString();

    result.sourceIdentifier =
        object
            .value(QStringLiteral("sourceIdentifier"))
            .toString();

    result.deviceType =
        object
            .value(QStringLiteral("deviceType"))
            .toString();

    result.capacity =
        object
            .value(QStringLiteral("capacity"))
            .toString();

    result.assetIdentifier =
        object
            .value(QStringLiteral("assetIdentifier"))
            .toString();

    result.readOnly =
        object
            .value(QStringLiteral("readOnly"))
            .toBool(true);

    const QJsonValue centerValue =
        object.value(
            QStringLiteral(
                "workstationCenter"));

    if (centerValue.isObject())
    {
        const QJsonObject center =
            centerValue.toObject();

        result.workstationCenterId =
            center
                .value(QStringLiteral("_id"))
                .toString();

        if (result.workstationCenterId.isEmpty())
        {
            result.workstationCenterId =
                center
                    .value(QStringLiteral("id"))
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

    if (employeeValue.isObject())
    {
        const QJsonObject employee =
            employeeValue.toObject();

        result.assignedEmployeeId =
            employee
                .value(QStringLiteral("_id"))
                .toString();

        if (result.assignedEmployeeId.isEmpty())
        {
            result.assignedEmployeeId =
                employee
                    .value(QStringLiteral("id"))
                    .toString();
        }

        result.assignedEmployeeName =
            employee
                .value(QStringLiteral("name"))
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

    if (workstationValue.isObject())
    {
        const QJsonObject workstation =
            workstationValue.toObject();

        result.assignedWorkstationMongoId =
            workstation
                .value(QStringLiteral("_id"))
                .toString();

        if (result.assignedWorkstationMongoId.isEmpty())
        {
            result.assignedWorkstationMongoId =
                workstation
                    .value(QStringLiteral("id"))
                    .toString();
        }

        result.assignedWorkstationId =
            workstation
                .value(QStringLiteral(
                    "workstationId"))
                .toString();

        result.assignedWorkstationName =
            workstation
                .value(QStringLiteral("name"))
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
    if (authenticationToken_.isEmpty())
    {
        emit caseStatusUpdateFailed(
            QStringLiteral(
                "You must be authenticated before starting acquisition."));
        return;
    }

    if (caseId.trimmed().isEmpty())
    {
        emit caseStatusUpdateFailed(
            QStringLiteral(
                "Forensic case ID is required."));
        return;
    }

    if (workstationId.trimmed().isEmpty())
    {
        emit caseStatusUpdateFailed(
            QStringLiteral(
                "Assigned workstation ID is required."));
        return;
    }

    updateCaseStatus(
        caseId,
        QStringLiteral("ACQUIRING"),
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
    const QUrl url(
        QStringLiteral(
            "http://localhost:5000/api/forensics/%1/status")
            .arg(
                QString::fromUtf8(
                    QUrl::toPercentEncoding(
                        caseId.trimmed()))));

    QNetworkRequest request =
        createAuthenticatedRequest(url);

    QJsonObject payload;

    payload.insert(
        QStringLiteral("status"),
        status);

    payload.insert(
        QStringLiteral("note"),
        note);

    if (!workstationId.trimmed().isEmpty())
    {
        payload.insert(
            QStringLiteral("workstationId"),
            workstationId.trimmed());
    }

    QNetworkReply *reply =
        networkManager_->sendCustomRequest(
            request,
            QByteArray("PATCH"),
            QJsonDocument(payload).toJson());

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply, status]()
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
                            "Could not update forensic case status (HTTP %1): %2")
                            .arg(statusCode)
                            .arg(message);
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
                !document.isObject()
            )
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
                    QStringLiteral("data"));

            if (dataValue.isObject())
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

            emit caseStatusUpdated(status);

            reply->deleteLater();
        });
}

QJsonObject
ForensicService::evidenceItemToJson(
    const EvidenceItem &item) const
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
        QStringLiteral("recoveredPath"),
        QString::fromStdString(
            item.recoveredPath));

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

    object.insert(
        QStringLiteral("confidenceScore"),
        item.confidenceScore);

    object.insert(
        QStringLiteral("confidenceLevel"),
        QString::fromStdString(
            item.getConfidenceString()));

    QJsonArray reasons;

    for (const std::string &reason :
         item.confidenceReasons)
    {
        reasons.append(
            QString::fromStdString(
                reason));
    }

    object.insert(
        QStringLiteral("confidenceReasons"),
        reasons);

    object.insert(
        QStringLiteral("sha256"),
        QString::fromStdString(
            item.sha256));

    object.insert(
        QStringLiteral("recovered"),
        item.recovered);

    object.insert(
        QStringLiteral("validated"),
        item.validated);

    return object;
}

void ForensicService::submitResults(
    const QString &caseId,
    const QString &workstationId)
{
    if (authenticationToken_.isEmpty())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "You must be authenticated before submitting forensic results."));
        return;
    }

    if (caseId.trimmed().isEmpty())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "Forensic case ID is required."));
        return;
    }

    if (workstationId.trimmed().isEmpty())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "Assigned workstation ID is required."));
        return;
    }

    if (isRunning())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "The forensic scan is still running."));
        return;
    }

    if (!summary_.completed)
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "The forensic scan has not completed successfully."));
        return;
    }

    if (!summary_.sourceOpened)
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "The forensic source was not opened successfully."));
        return;
    }

    const QUrl url(
        QStringLiteral(
            "http://localhost:5000/api/forensics/%1/results")
            .arg(
                QString::fromUtf8(
                    QUrl::toPercentEncoding(
                        caseId.trimmed()))));

    QNetworkRequest request =
        createAuthenticatedRequest(url);

    QJsonObject payload;

    payload.insert(
        QStringLiteral("workstationId"),
        workstationId.trimmed());

    if (selectedCase_.sourceType.trimmed().isEmpty())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "The forensic case has no source type."));
        return;
    }

    payload.insert(
        QStringLiteral("sourceType"),
        selectedCase_.sourceType.trimmed());

    if (
        selectedCase_.sourceType ==
            QStringLiteral("PHYSICAL_DEVICE") &&
        selectedCase_.sourceIdentifier.trimmed().isEmpty())
    {
        emit resultsSubmitFailed(
            QStringLiteral(
                "The forensic case has no physical device identifier."));
        return;
    }

    if (!selectedCase_.sourceIdentifier.trimmed().isEmpty())
    {
        payload.insert(
            QStringLiteral("sourceIdentifier"),
            selectedCase_.sourceIdentifier.trimmed());
    }

    payload.insert(
        QStringLiteral("bytesScanned"),
        static_cast<qint64>(
            summary_.bytesScanned));

    payload.insert(
        QStringLiteral("totalBytes"),
        static_cast<qint64>(
            summary_.totalBytes));

    payload.insert(
        QStringLiteral("candidatesFound"),
        static_cast<qint64>(
            summary_.candidatesFound));

    payload.insert(
        QStringLiteral("recoveredArtifacts"),
        static_cast<qint64>(
            summary_.recoveredArtifacts));

    payload.insert(
        QStringLiteral("validatedArtifacts"),
        static_cast<qint64>(
            summary_.validatedArtifacts));

    payload.insert(
        QStringLiteral("rejectedArtifacts"),
        static_cast<qint64>(
            summary_.rejectedArtifacts));

    payload.insert(
        QStringLiteral("highConfidenceArtifacts"),
        static_cast<qint64>(
            summary_.highConfidenceArtifacts));

    payload.insert(
        QStringLiteral("recoveredBytes"),
        static_cast<qint64>(
            summary_.recoveredBytes));

    QJsonArray artifacts;

    for (const EvidenceItem &item :
         results_)
    {
        artifacts.append(
            evidenceItemToJson(item));
    }

    payload.insert(
        QStringLiteral("artifacts"),
        artifacts);

    payload.insert(
        QStringLiteral("status"),
        QStringLiteral("COMPLETED"));

    QNetworkReply *reply =
        networkManager_->post(
            request,
            QJsonDocument(payload).toJson());

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
                            "Could not submit forensic results (HTTP %1): %2")
                            .arg(statusCode)
                            .arg(message);
                }

                qDebug()
                    << "Forensic result submission failed:"
                    << responseData;

                emit resultsSubmitFailed(message);

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
                emit resultsSubmitFailed(
                    QStringLiteral(
                        "Server returned invalid result-submission JSON."));
                reply->deleteLater();
                return;
            }

            const QJsonObject root =
                document.object();

            const QJsonValue dataValue =
                root.value(
                    QStringLiteral("data"));

            if (dataValue.isObject())
            {
                selectedCase_ =
                    caseFromJson(
                        dataValue.toObject());
            }
            else
            {
                selectedCase_.status =
                    QStringLiteral("COMPLETED");
            }

            emit resultsSubmitted();

            reply->deleteLater();
        });
}