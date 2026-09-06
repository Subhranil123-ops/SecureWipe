#pragma once

#include <QDialog>

#include "StorageDevice.h"

class QLabel;

class DeviceDetailsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceDetailsDialog(
        const StorageDevice &device,
        QWidget *parent = nullptr
    );

private:
    QString formatCapacity(std::uint64_t bytes) const;

    QLabel *modelLabel_;
    QLabel *serialLabel_;
    QLabel *capacityLabel_;
    QLabel *interfaceLabel_;
    QLabel *deviceIdLabel_;
    QLabel *systemDiskLabel_;
    QLabel *removableLabel_;
};