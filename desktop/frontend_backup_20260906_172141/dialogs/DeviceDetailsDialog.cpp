#include "DeviceDetailsDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

DeviceDetailsDialog::DeviceDetailsDialog(
    const StorageDevice &device,
    QWidget *parent)
    : QDialog(parent),
      modelLabel_(new QLabel(this)),
      serialLabel_(new QLabel(this)),
      capacityLabel_(new QLabel(this)),
      interfaceLabel_(new QLabel(this)),
      deviceIdLabel_(new QLabel(this)),
      systemDiskLabel_(new QLabel(this)),
      removableLabel_(new QLabel(this))
{
    setWindowTitle(QStringLiteral("Storage Device Details"));
    setModal(true);

    resize(520, 360);

    modelLabel_->setText(
        QString::fromStdString(device.getModel())
    );

    serialLabel_->setText(
        QString::fromStdString(device.getSerialNumber())
    );

    capacityLabel_->setText(
        formatCapacity(device.getCapacityBytes())
    );

    interfaceLabel_->setText(
        QString::fromStdString(device.getInterfaceType())
    );

    deviceIdLabel_->setText(
        QString::fromStdString(device.getDeviceId())
    );

    systemDiskLabel_->setText(
        device.isSystemDisk()
            ? QStringLiteral("Yes")
            : QStringLiteral("No")
    );

    removableLabel_->setText(
        device.isRemovable()
            ? QStringLiteral("Yes")
            : QStringLiteral("No")
    );

    auto *formLayout = new QFormLayout;

    formLayout->setLabelAlignment(
        Qt::AlignLeft | Qt::AlignVCenter
    );

    formLayout->addRow(
        QStringLiteral("Model"),
        modelLabel_
    );

    formLayout->addRow(
        QStringLiteral("Serial Number"),
        serialLabel_
    );

    formLayout->addRow(
        QStringLiteral("Capacity"),
        capacityLabel_
    );

    formLayout->addRow(
        QStringLiteral("Interface"),
        interfaceLabel_
    );

    formLayout->addRow(
        QStringLiteral("Device ID"),
        deviceIdLabel_
    );

    formLayout->addRow(
        QStringLiteral("System Disk"),
        systemDiskLabel_
    );

    formLayout->addRow(
        QStringLiteral("Removable"),
        removableLabel_
    );

    auto *buttonBox =
        new QDialogButtonBox(
            QDialogButtonBox::Close,
            this
        );

    connect(
        buttonBox,
        &QDialogButtonBox::rejected,
        this,
        &QDialog::reject
    );

    auto *mainLayout = new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        24,
        24,
        24,
        24
    );

    mainLayout->setSpacing(20);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

QString DeviceDetailsDialog::formatCapacity(
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