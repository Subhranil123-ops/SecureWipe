#pragma once

#include <QWidget>

#include "StorageDevice.h"
#include "ClassificationResult.h"

class QLabel;
class QPushButton;
class QFrame;

class DeviceDetailsPage : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceDetailsPage(
        const StorageDevice &device,
        QWidget *parent = nullptr
    );

    void updateSafetyStatus(
    bool passed,
    const QString &message = QString()
    );

signals:
    void backRequested();
    void refreshRequested();

private:
    const StorageDevice &device_;

    QLabel *modelLabel_;
    QLabel *typeBadgeLabel_;
    QLabel *serialLabel_;
    QLabel *capacityLabel_;
    QLabel *interfaceLabel_;
    QLabel *devicePathLabel_;
    QLabel *systemDiskValueLabel_;
    QLabel *removableValueLabel_;
    QLabel *mediaTypeValueLabel_;
    QLabel *busTypeValueLabel_;
    QLabel *deviceTypeValueLabel_;
    QLabel *classificationStatusLabel_;

    QLabel *safetyStatusLabel_;
    QLabel *safetyDescriptionLabel_;

    void setupUi();

    void setSafetyPending();

    void setSafetyPassed();

    void setSafetyFailed(
        const QString &message
    );

    QWidget *createDeviceHeader();
    QWidget *createClassificationCard(
        const ClassificationResult &classification
    );
    QWidget *createSystemStatusCard(
        const ClassificationResult &classification
    );
    QWidget *createSafetyCard();

    QFrame *createCard();

    QLabel *createSectionTitle(
        const QString &title
    );

    QLabel *createFieldLabel(
        const QString &text
    );

    QLabel *createValueLabel(
        const QString &text
    );

    QString formatCapacity(
        std::uint64_t bytes
    ) const;

    QString mediaTypeText(
        MediaType type
    ) const;

    QString busTypeText(
        BusType type
    ) const;

    QString deviceTypeText(
        DeviceType type
    ) const;
};