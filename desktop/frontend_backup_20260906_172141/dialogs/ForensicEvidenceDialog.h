#pragma once

#include <QDialog>

#include "../../backend/forensic/evidence/include/EvidenceItem.h"

class QLabel;
class QGridLayout;
class QTableWidget;

class ForensicEvidenceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ForensicEvidenceDialog(
        const EvidenceItem &evidence,
        QWidget *parent = nullptr
    );

private:
    QLabel *createValueLabel(const QString &value);
    QLabel *createStatusLabel(bool valid);
    void addDetailRow(QGridLayout *layout, int row, const QString &label, const QString &value);
};