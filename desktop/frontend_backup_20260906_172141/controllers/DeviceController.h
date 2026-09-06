#pragma once

#include <QObject>
#include <optional>
#include <vector>

#include "StorageDevice.h"
#include "SafetyEngine.h"
#include "SanitizationEngine.h"
#include "../../sanitization/include/SanitizationCapability.h"

class StorageService;

class DeviceController : public QObject
{
    Q_OBJECT

public:
    explicit DeviceController(
        QObject *parent = nullptr
    );

    const std::vector<StorageDevice> &
    devices() const;

    const std::optional<StorageDevice> &
    selectedTarget() const;

    const SafetyResult &
    lastSafetyResult() const;

    // Save the device selected by the user.
    bool selectTarget(int index);

    // Rediscover devices and validate the
    // previously selected target.
    bool validateSelectedTarget();

    // Run SafetyEngine after successful
    // target validation.
    bool evaluateSelectedTarget();

    // Execute sanitization using the already
    // validated target and safety result.
    bool sanitizeSelectedTarget();

    SanitizationCapability detectSelectedTargetCapability() const;

public slots:
    void refreshDevices();

signals:
    void devicesUpdated();

    void discoveryFailed(
        const QString &message
    );

    void safetyCheckPassed();

    void safetyCheckFailed(
        const QString &message
    );

    void sanitizationSucceeded();

    void sanitizationFailed(
        const QString &message
    );

private:
    StorageService *storageService_;

    std::vector<StorageDevice> devices_;

    SafetyEngine safetyEngine_;

    SanitizationEngine sanitizationEngine_;

    // Copy of the device selected by the user.
    std::optional<StorageDevice> selectedTarget_;

    // Result from the most recent safety evaluation.
    SafetyResult lastSafetyResult_{};
};