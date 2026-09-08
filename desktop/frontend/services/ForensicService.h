#pragma once

#include <QObject>
#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QVector>
#include <QString>
#include <QJsonObject>

#include "../../backend/forensic/evidence/include/EvidenceItem.h"
#include "../../backend/forensic/evidence/include/EvidenceCollector.h"

struct ForensicScanSummary
{
    bool sourceOpened = false;
    bool completed = false;

    quint64 bytesScanned = 0;
    quint64 candidatesFound = 0;
    quint64 recoveredArtifacts = 0;
    quint64 validatedArtifacts = 0;
    quint64 rejectedArtifacts = 0;
    quint64 highConfidenceArtifacts = 0;
    quint64 recoveredBytes = 0;
};

struct ForensicCaseInfo
{
    QString caseId;
    QString title;
    QString description;

    QString status;

    QString sourceType;
    QString sourceName;
    QString sourceIdentifier;

    QString deviceType;
    QString capacity;
    QString assetIdentifier;

    QString workstationCenterId;

    QString assignedEmployeeId;
    QString assignedEmployeeName;

    QString assignedWorkstationId;
    QString assignedWorkstationMongoId;
    QString assignedWorkstationName;

    bool readOnly = true;
};

class ForensicService : public QObject
{
    Q_OBJECT

public:
    explicit ForensicService(QObject *parent = nullptr);
    ~ForensicService() override;

    void setAuthenticationToken(
        const QString &token);

    QString authenticationToken() const;

    void scan(
        const QString &source);

    bool isRunning() const;

    const QVector<EvidenceItem> &results() const;

    const ForensicScanSummary &summary() const;

    QString lastSource() const;

    void loadAssignedCases();

    void loadCase(
        const QString &caseId);

    void startCaseAcquisition(
        const QString &caseId,
        const QString &workstationId);

    void submitResults(
        const QString &caseId,
        const QString &workstationId);

    const QVector<ForensicCaseInfo> &cases() const;

    const ForensicCaseInfo &selectedCase() const;

    bool hasSelectedCase() const;

signals:
    void scanFinished();
    void scanFailed(
        const QString &message);

    void casesLoaded();
    void casesLoadFailed(
        const QString &message);

    void caseLoaded();
    void caseLoadFailed(
        const QString &message);

    void caseStatusUpdated(
        const QString &status);

    void caseStatusUpdateFailed(
        const QString &message);

    void resultsSubmitted();
    void resultsSubmitFailed(
        const QString &message);

private:
    void updateCaseStatus(
        const QString &caseId,
        const QString &status,
        const QString &note,
        const QString &workstationId);

    QNetworkRequest createAuthenticatedRequest(
        const QUrl &url) const;

    QJsonObject evidenceItemToJson(
        const EvidenceItem &item) const;

    ForensicCaseInfo caseFromJson(
        const QJsonObject &object) const;

private:
    QFutureWatcher<EvidenceCollectionResult> watcher_;

    QVector<EvidenceItem> results_;

    ForensicScanSummary summary_;

    QString lastSource_;

    QNetworkAccessManager *networkManager_;

    QString authenticationToken_;

    QVector<ForensicCaseInfo> cases_;

    ForensicCaseInfo selectedCase_;
};