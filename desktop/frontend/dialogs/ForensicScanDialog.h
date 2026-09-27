#pragma once

#include <QDialog>

class QLabel;
class QProgressBar;

class ForensicScanDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ForensicScanDialog(
        const QString &source,
        QWidget *parent = nullptr
    );

    void setProgress(
        int percentage,
        quint64 bytesScanned,
        quint64 totalBytes
    );

private:
    QLabel *sourceLabel_;
    QProgressBar *progressBar_;
    QLabel *progressLabel_;
};