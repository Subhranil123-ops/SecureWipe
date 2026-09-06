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

private:
    QLabel *sourceLabel_;
    QProgressBar *progressBar_;
};