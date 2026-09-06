#pragma once

#include <QObject>
#include <QFutureWatcher>
#include <QVector>
#include <QString>

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

class ForensicService : public QObject
{
    Q_OBJECT

public:
    explicit ForensicService(QObject *parent = nullptr);
    ~ForensicService() override;

    void scan(const QString &source);

    bool isRunning() const;

    const QVector<EvidenceItem> &results() const;

    const ForensicScanSummary &summary() const;

    QString lastSource() const;

signals:
    void scanFinished();
    void scanFailed(const QString &message);

private:
    QFutureWatcher<EvidenceCollectionResult> watcher_;

    QVector<EvidenceItem> results_;

    ForensicScanSummary summary_;

    QString lastSource_;
};