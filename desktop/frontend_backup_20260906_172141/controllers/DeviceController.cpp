#include "DeviceController.h"

#include "../services/StorageService.h"
#include "../../backend/sanitization/include/SanitizationCapability.h"


DeviceController::DeviceController(
    QObject *parent)
    : QObject(parent),
      storageService_(new StorageService()),
      devices_(),
      safetyEngine_(),
      selectedTarget_(std::nullopt)
{
}


const std::vector<StorageDevice> &
DeviceController::devices() const
{
    return devices_;
}


const SafetyResult &
DeviceController::lastSafetyResult() const
{
    return lastSafetyResult_;
}


const std::optional<StorageDevice> &
DeviceController::selectedTarget() const
{
    return selectedTarget_;
}


// ============================================================
// Target Selection
// ============================================================

bool DeviceController::selectTarget(
    int index)
{
    if (index < 0 ||
        index >= static_cast<int>(devices_.size()))
    {
        selectedTarget_.reset();

        return false;
    }

    /*
     * Store a copy of the selected device.
     * This is important because devices_ may be replaced
     * during fresh discovery.
     */
    selectedTarget_ =
        devices_[index];

    /*
     * Save the original identity inside SafetyEngine.
     */
    safetyEngine_.setExpectedTarget(
        *selectedTarget_);

    return true;
}


// ============================================================
// Sanitization Capability
// ============================================================

SanitizationCapability
DeviceController::detectSelectedTargetCapability() const
{
    if (!selectedTarget_.has_value())
    {
        return SanitizationCapability{};
    }

    return detectSanitizationCapability(
        *selectedTarget_);
}


// ============================================================
// Device Discovery
// ============================================================

void DeviceController::refreshDevices()
{
    try
    {
        devices_ =
            storageService_->discoverDevices();

        /*
         * Normal manual refresh starts a new discovery state.
         * The selected target must be selected again from this list.
         */
        selectedTarget_.reset();

        emit devicesUpdated();
    }
    catch (const std::exception &exception)
    {
        emit discoveryFailed(
            QString::fromStdString(
                exception.what()));
    }
    catch (...)
    {
        emit discoveryFailed(
            QStringLiteral(
                "Unknown error occurred while discovering storage devices."));
    }
}


// ============================================================
// Fresh Discovery + Target Validation
// ============================================================

bool DeviceController::validateSelectedTarget()
{
    if (!selectedTarget_.has_value())
    {
        emit safetyCheckFailed(
            QStringLiteral(
                "Please select a target device first."));

        return false;
    }

    /*
     * Perform a completely fresh device discovery.
     */
    try
    {
        devices_ =
            storageService_->discoverDevices();
    }
    catch (const std::exception &exception)
    {
        emit safetyCheckFailed(
            QString::fromStdString(
                exception.what()));

        return false;
    }
    catch (...)
    {
        emit safetyCheckFailed(
            QStringLiteral(
                "Unable to refresh storage devices."));

        return false;
    }

    /*
     * Find the original target inside the fresh discovery.
     *
     * validateTarget() also replaces selectedTarget_ with
     * the freshly discovered matching device.
     */
    if (!safetyEngine_.validateTarget(
            devices_,
            *selectedTarget_))
    {
        selectedTarget_.reset();

        emit safetyCheckFailed(
            QStringLiteral(
                "The selected target could not be validated."));

        return false;
    }

    emit devicesUpdated();

    return true;
}


// ============================================================
// Safety Evaluation
// ============================================================

bool DeviceController::evaluateSelectedTarget()
{
    if (!selectedTarget_.has_value())
    {
        emit safetyCheckFailed(
            QStringLiteral(
                "No validated target is available."));

        return false;
    }

    /*
     * SafetyEngine is called only after target validation.
     */
    if (!safetyEngine_.evaluate(
            *selectedTarget_))
    {
        emit safetyCheckFailed(
            QStringLiteral(
                "Safety checks blocked sanitization."));

        return false;
    }

    emit safetyCheckPassed();

    return true;
}