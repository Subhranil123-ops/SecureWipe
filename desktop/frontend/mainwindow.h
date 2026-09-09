#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QMainWindow>
#include <QStackedWidget>
#include <QString>

#include "StorageDevice.h"
#include "../../backend/sanitization/include/SanitizationMethod.h"
#include "../../backend/sanitization/include/SanitizationPipeline.h"
#include "../../backend/sanitization/include/SanitizationResult.h"

class AuthManager;
class DeviceController;
class DeviceTableModel;
class SanitizationRequestService;
class SanitizationResultService;

class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;
class QTableWidget;
class QProgressBar;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    AuthManager *authManager_;
    SanitizationRequestService *requestService_;
    SanitizationResultService *resultService_;

    DeviceController *deviceController_;
    DeviceTableModel *deviceTableModel_;

    QWidget *deviceDetailsPage_;
    QWidget *forensicPage_;

    QWidget *root_;
    QStackedWidget *rootStack_;
    QWidget *loginPage_;
    QWidget *appPage_;
    QStackedWidget *contentStack_;

    QLineEdit *emailEdit_;
    QLineEdit *passwordEdit_;
    QLabel *loginErrorLabel_;
    QPushButton *loginButton_;

    QLabel *operatorNameLabel_;
    QLabel *operatorRoleLabel_;
    QLabel *connectionBadgeLabel_;

    QPushButton *dashboardNavButton_;
    QPushButton *devicesNavButton_;
    QPushButton *jobsNavButton_;
    QPushButton *forensicsNavButton_;
    QPushButton *settingsNavButton_;
    QPushButton *logoutButton_;

    QWidget *dashboardPage_;
    QWidget *devicesPage_;
    QWidget *jobsPage_;
    QWidget *forensicsPage_;
    QWidget *settingsPage_;

    QTableWidget *dashboardJobsTable_;
    QTableWidget *assignedJobsTable_;
    QTableWidget *deviceTable_;
    QTableWidget *devicesInventoryTable_;
    QLabel *devicesStatusLabel_;

    QLabel *totalJobsValue_;
    QLabel *activeJobsValue_;
    QLabel *completedJobsValue_;
    QLabel *failedJobsValue_;

    QComboBox *jobComboBox_;

    QLabel *jobRequestIdValue_;
    QLabel *jobDeviceTypeValue_;
    QLabel *jobRequestedMethodValue_;
    QLabel *jobAssetValue_;
    QLabel *jobWorkstationValue_;

    QLabel *targetModelValue_;
    QLabel *targetSerialValue_;
    QLabel *targetCapacityValue_;
    QLabel *targetInterfaceValue_;
    QLabel *targetPathValue_;

    QLabel *targetSafetyBadge_;
    QLabel *targetSafetyText_;

    QLabel *capabilityValue_;
    QLabel *selectedMethodValue_;

    QLabel *pipelineStatusValue_;
    QLabel *verificationValue_;
    QLabel *operationValue_;
    QLabel *bytesProcessedValue_;
    QLabel *bytesVerifiedValue_;
    QLabel *samplesValue_;
    QLabel *certificateValue_;
    QLabel *evidenceValue_;

    QLabel *jobMessageLabel_;
    QProgressBar *operationProgress_;

    QPushButton *refreshJobsButton_;
    QPushButton *refreshDevicesButton_;
    QPushButton *validateTargetButton_;
    QPushButton *startSanitizationButton_;

    QJsonArray assignedRequests_;

    QString selectedRequestId_;
    QString selectedRequestDeviceType_;
    QString selectedRequestMethod_;
    QString selectedRequestSerialNumber_;
    QString selectedWorkstationId_;

    bool operationRunning_ = false;
    bool workstationIdentityVerified_ = false;
    QString verifiedWorkstationId_;

    // Cached outcome of the last completed runTargetSafetyCheck() SAFE
    // branch (i.e. core SafetyResult::isOverallSafe was true along with
    // every serial/type/capability/method gate). This lets the Start
    // button be re-evaluated cheaply (see updateStartSanitizationReadiness())
    // whenever workstation-verification or request state changes on its
    // own, without re-running hardware target rediscovery. It is reset to
    // false any time the selected target/request changes or a fresh
    // safety check has not yet passed.
    bool lastSafetyCheckPassed_ = false;

    void buildUi();
    void buildLoginPage();
    void buildAppShell();
    void buildDashboardPage();
    void buildJobsPage();
    void buildDevicesPage();
    void buildForensicsPage();
    void buildSettingsPage();

    void applyTheme();

    void setActiveNav(QPushButton *button);

    void setConnectionState(
        bool connected,
        const QString &text = QString());

    void showPage(
        QWidget *page,
        QPushButton *navButton);

    void refreshAssignedRequests();

    void handleAssignedRequests(
        const QJsonArray &requests);

    QJsonObject selectedRequestObject() const;

    void selectRequestFromJobs(int index);
    void populateJobDetails();

    void refreshPhysicalDevices();
    void populateDeviceTable();

    bool requestMatchesDevice(
        const QString &requestedType,
        const StorageDevice &device) const;

    void selectTargetDevice(int row);

    void resetTargetPanel();

    void runTargetSafetyCheck();

    // Single source of truth for Start Sanitization button enablement.
    // Recomputes requestReady / workstationReady from current state and
    // combines them with the cached lastSafetyCheckPassed_ flag. This is
    // a cheap, side-effect-free readout (no hardware access), so it is
    // safe to call from every site that can change any of those three
    // inputs (safety check completion, workstation verification events,
    // assigned-job refresh reconciliation, target/request selection).
    void updateStartSanitizationReadiness();

    void startSanitization();

    void finishSanitization(
        const SecureWipe::SanitizationPipelineResult &pipelineResult);

    void submitPipelineResult(
        const SecureWipe::SanitizationPipelineResult &pipelineResult);

    void updateTargetPanelFromDevice(
        const StorageDevice &device);

    void updatePipelineUiForResult(
        const SecureWipe::SanitizationPipelineResult &pipelineResult);

    void showSelectedDeviceDetails();

    void showDeviceDetails(
        const StorageDevice &device);

    void logout();

    QString sanitizationMethodName(
        SanitizationMethod method) const;

    QString formatCapacity(
        std::uint64_t bytes) const;

    QString formatBytes(
        std::uint64_t bytes) const;

    QString formatDuration(
        std::uint64_t milliseconds) const;

    QString statusText(
        SecureWipe::SanitizationStatus status) const;

    QString verificationText(
        SecureWipe::VerificationStatus status) const;
};