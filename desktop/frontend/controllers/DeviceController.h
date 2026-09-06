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
        QObject *parent = nullptr);


    const std::vector<StorageDevice> &
    devices() const;


    const std::optional<StorageDevice> &
    selectedTarget() const;


    const SafetyResult &
    lastSafetyResult() const;


    bool selectTarget(
        int index);


    bool validateSelectedTarget();


    bool evaluateSelectedTarget();


    bool sanitizeSelectedTarget();


    SanitizationCapability
    detectSelectedTargetCapability() const;


public slots:

    void refreshDevices();


signals:

    void devicesUpdated();


    void discoveryFailed(
        const QString &message);


    void safetyCheckPassed();


    void safetyCheckFailed(
        const QString &message);


    void sanitizationSucceeded();


    void sanitizationFailed(
        const QString &message);


private:

    StorageService *storageService_;


    std::vector<StorageDevice>
        devices_;


    SafetyEngine safetyEngine_;


    SanitizationEngine
        sanitizationEngine_;


    std::optional<StorageDevice>
        selectedTarget_;


    SafetyResult
        lastSafetyResult_{};
};