#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFuture>
#include <QMainWindow>
#include <QPushButton>
#include <QString>

#include "StorageDevice.h"
#include "../../backend/sanitization/include/SanitizationMethod.h"
#include "../../backend/sanitization/include/SanitizationResult.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class AuthManager;
class DeviceController;
class DeviceTableModel;
class DeviceDetailsPage;
class ForensicPage;
class SanitizationRequestService;
class QComboBox;
class QTableWidget;
class QLabel;
class QProgressBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    QString selectedRequestId;
    QString selectedRequestDeviceType;
    QString selectedRequestMethod;
    QString selectedWipeDeviceId;

    bool waitingForSanitizationStart = false;
    bool sanitizationOperationRunning = false;
    bool wipeSafetyApproved = false;

    Ui::MainWindow *ui;
    AuthManager *authManager;
    SanitizationRequestService *sanitizationRequestService;
    DeviceController *deviceController;
    DeviceTableModel *deviceTableModel;
    DeviceDetailsPage *deviceDetailsPage;
    ForensicPage *forensicPage;
    QPushButton *refreshDevicesButton;

    // Rebuilt, user-facing sanitization workspace.
    QComboBox *wipeRequestComboBox = nullptr;
    QTableWidget *wipeDeviceTable = nullptr;
    QPushButton *wipeRefreshButton = nullptr;
    QPushButton *wipeSafetyButton = nullptr;
    QPushButton *wipeStartButton = nullptr;
    QProgressBar *wipeProgressBar = nullptr;
    QLabel *wipeRequestSummaryLabel = nullptr;
    QLabel *wipeTargetSummaryLabel = nullptr;
    QLabel *wipeMethodValueLabel = nullptr;
    QLabel *wipeTargetValueLabel = nullptr;
    QLabel *wipeCapacityValueLabel = nullptr;
    QLabel *wipeInterfaceValueLabel = nullptr;
    QLabel *wipeSafetyBadgeLabel = nullptr;
    QLabel *wipeStatusLabel = nullptr;
    QLabel *wipeVerificationLabel = nullptr;
    QLabel *wipeSafetyChecksLabel = nullptr;

    void setActiveNavButton(QPushButton *activeButton);
    void setupDevicesPage();
    void setupWipePage();
    void refreshDevices();
    void populateWipeDevices();
    void updateWipeSelectionState();
    void runWipeSafetyCheck();
    void showSelectedDeviceDetails();
    void showDeviceDetails(const StorageDevice &device);
    void showDevicesPage();
    void hideDeviceDetailsPage();
    void logout();
    void startSanitization();
    void showSanitizationResult(const SecureWipe::SanitizationResult &result);
    QString sanitizationMethodName(SanitizationMethod method) const;
};

#endif // MAINWINDOW_H
