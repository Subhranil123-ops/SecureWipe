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
    lastSafetyResult_ = SafetyResult{};

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
        lastSafetyResult_ = SafetyResult{};

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
        lastSafetyResult_ = SafetyResult{};

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
    lastSafetyResult_ =
        safetyEngine_.evaluateWithResult(
            *selectedTarget_);

    if (!lastSafetyResult_.isOverallSafe)
    {
        emit safetyCheckFailed(
            QString::fromStdString(
                lastSafetyResult_.summary));

        return false;
    }

    emit safetyCheckPassed();

    return true;
}

// ============================================================
// Sanitization Method
// ============================================================

SanitizationMethod DeviceController::detectSelectedTargetMethod() const
{
    if (!selectedTarget_.has_value())
    {
        return SanitizationMethod::Unsupported;
    }

    const SanitizationCapability capability =
        detectSelectedTargetCapability();

    return sanitizationEngine_.selectMethod(
        *selectedTarget_,
        capability
    );
}


// ============================================================
// Sanitization Execution
// ============================================================

SecureWipe::SanitizationResult
DeviceController::sanitizeSelectedTarget()
{
    SecureWipe::SanitizationResult result;

    if (!selectedTarget_.has_value())
    {
        result.status = SecureWipe::SanitizationStatus::FAILED;
        result.error = SecureWipe::SanitizationErrorCode::SAFETY_VALIDATION_FAILED;
        result.message = "No validated sanitization target is available.";
        result.errorMessage = result.message;
        emit sanitizationFailed(QString::fromStdString(result.message));
        return result;
    }

    if (!lastSafetyResult_.isOverallSafe)
    {
        result.status = SecureWipe::SanitizationStatus::FAILED;
        result.error = SecureWipe::SanitizationErrorCode::SAFETY_VALIDATION_FAILED;
        result.message = "Safety checks did not approve the target.";
        result.errorMessage = result.message;
        emit sanitizationFailed(QString::fromStdString(result.message));
        return result;
    }

    try
    {
        result = sanitizationEngine_.sanitize(
            *selectedTarget_,
            lastSafetyResult_
        );
    }
    catch (const std::exception &exception)
    {
        result.status = SecureWipe::SanitizationStatus::FAILED;
        result.error = SecureWipe::SanitizationErrorCode::SANITIZATION_EXECUTION_FAILED;
        result.message = exception.what();
        result.errorMessage = result.message;
    }
    catch (...)
    {
        result.status = SecureWipe::SanitizationStatus::FAILED;
        result.error = SecureWipe::SanitizationErrorCode::SANITIZATION_EXECUTION_FAILED;
        result.message = "Unknown error occurred during sanitization.";
        result.errorMessage = result.message;
    }

    if (result.status == SecureWipe::SanitizationStatus::COMPLETED)
    {
        emit sanitizationSucceeded();
    }
    else
    {
        emit sanitizationFailed(QString::fromStdString(result.message));
    }

    return result;
}
