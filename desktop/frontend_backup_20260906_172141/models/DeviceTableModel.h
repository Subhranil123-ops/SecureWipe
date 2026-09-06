#pragma once

#include <QAbstractTableModel>

#include <vector>

#include "StorageDevice.h"

class DeviceTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit DeviceTableModel(QObject *parent = nullptr);

    int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(
        const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(
        const QModelIndex &index,
        int role = Qt::DisplayRole) const override;

    QVariant headerData(
        int section,
        Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    void setDevices(const std::vector<StorageDevice> &devices);

    const StorageDevice *deviceAt(int row) const;

private:
    std::vector<StorageDevice> devices_;

    QString formatCapacity(std::uint64_t bytes) const;
    QString mediaType(const StorageDevice &device) const;
    QString deviceType(const StorageDevice &device) const;
};