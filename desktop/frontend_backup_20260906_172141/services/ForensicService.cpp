#include "ForensicService.h"

#include <QFile>
#include <exception>

#include <QtConcurrent/QtConcurrentRun>

ForensicService::ForensicService(QObject *parent)
    : QObject(parent)
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
                        result.evidence.size()
                    )
                );

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

                emit scanFinished();
            }
            catch (const std::exception &exception)
            {
                emit scanFailed(
                    QString::fromLocal8Bit(
                        exception.what()
                    )
                );
            }
            catch (...)
            {
                emit scanFailed(
                    QStringLiteral(
                        "Forensic scan failed unexpectedly."
                    )
                );
            }
        }
    );
}

ForensicService::~ForensicService()
{
    if (watcher_.isRunning())
    {
        watcher_.waitForFinished();
    }
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
        return;
    }

    results_.clear();

    summary_ =
        ForensicScanSummary{};

    lastSource_ =
        cleanedSource;

    const std::string nativeSource =
        QFile::encodeName(
            cleanedSource
        ).toStdString();

    watcher_.setFuture(
        QtConcurrent::run(
            [nativeSource]()
            {
                EvidenceCollector collector;

                return collector.collectWithSummary(
                    nativeSource
                );
            }
        )
    );
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