#include "DeviceTableModel.h"

#include <QString>

DeviceTableModel::DeviceTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int DeviceTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return static_cast<int>(devices_.size());
}

int DeviceTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return 6;
}

QVariant DeviceTableModel::data(
    const QModelIndex &index,
    int role) const
{
    if (!index.isValid())
    {
        return {};
    }

    if (index.row() < 0 ||
        index.row() >= static_cast<int>(devices_.size()))
    {
        return {};
    }

    const StorageDevice &device = devices_.at(index.row());

    if (role != Qt::DisplayRole)
    {
        return {};
    }

    switch (index.column())
    {
        case 0:
            return QString::fromStdString(device.getModel());

        case 1:
            return formatCapacity(device.getCapacityBytes());

        case 2:
            return QString::fromStdString(
                device.getInterfaceType()
            );

        case 3:
            return mediaType(device);

        case 4:
            return deviceType(device);

        case 5:
            return device.isSystemDisk()
                       ? QStringLiteral("Yes")
                       : QStringLiteral("No");

        default:
            return {};
    }
}

QVariant DeviceTableModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role) const
{
    if (role != Qt::DisplayRole)
    {
        return {};
    }

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case 0:
                return QStringLiteral("Model");

            case 1:
                return QStringLiteral("Capacity");

            case 2:
                return QStringLiteral("Interface");

            case 3:
                return QStringLiteral("Media");

            case 4:
                return QStringLiteral("Device Type");

            case 5:
                return QStringLiteral("System Disk");

            default:
                return {};
        }
    }

    return section + 1;
}

void DeviceTableModel::setDevices(
    const std::vector<StorageDevice> &devices)
{
    beginResetModel();

    devices_ = devices;

    endResetModel();
}

const StorageDevice *DeviceTableModel::deviceAt(int row) const
{
    if (row < 0 ||
        row >= static_cast<int>(devices_.size()))
    {
        return nullptr;
    }

    return &devices_.at(row);
}

QString DeviceTableModel::formatCapacity(
    std::uint64_t bytes) const
{
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;
    constexpr double GB = MB * 1024.0;
    constexpr double TB = GB * 1024.0;

    if (bytes >= static_cast<std::uint64_t>(TB))
    {
        return QString::number(
            static_cast<double>(bytes) / TB,
            'f',
            2
        ) + QStringLiteral(" TB");
    }

    if (bytes >= static_cast<std::uint64_t>(GB))
    {
        return QString::number(
            static_cast<double>(bytes) / GB,
            'f',
            1
        ) + QStringLiteral(" GB");
    }

    if (bytes >= static_cast<std::uint64_t>(MB))
    {
        return QString::number(
            static_cast<double>(bytes) / MB,
            'f',
            1
        ) + QStringLiteral(" MB");
    }

    if (bytes >= static_cast<std::uint64_t>(KB))
    {
        return QString::number(
            static_cast<double>(bytes) / KB,
            'f',
            1
        ) + QStringLiteral(" KB");
    }

    return QString::number(bytes) + QStringLiteral(" B");
}

QString DeviceTableModel::mediaType(
    const StorageDevice &device) const
{
    if (device.hasSeekPenalty())
    {
        return QStringLiteral("HDD");
    }

    return QStringLiteral("SSD");
}

QString DeviceTableModel::deviceType(
    const StorageDevice &device) const
{
    if (device.isRemovable())
    {
        return QStringLiteral("Removable");
    }

    return QStringLiteral("Internal");
}