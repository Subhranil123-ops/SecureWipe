#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>

#include "StorageDevice.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class AuthManager;
class DeviceController;
class DeviceTableModel;
class DeviceDetailsPage;
class ForensicPage;
class SanitizationRequestService;

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

    bool waitingForSanitizationStart = false;
    bool sanitizationOperationRunning = false;

    Ui::MainWindow *ui;

    AuthManager *authManager;
    SanitizationRequestService *sanitizationRequestService;

    DeviceController *deviceController;
    DeviceTableModel *deviceTableModel;

    DeviceDetailsPage *deviceDetailsPage;
    ForensicPage *forensicPage;

    QPushButton *refreshDevicesButton;

    void setActiveNavButton(QPushButton *activeButton);

    void setupDevicesPage();

    void refreshDevices();

    void showSelectedDeviceDetails();

    void showDeviceDetails(
        const StorageDevice &device
    );

    void showDevicesPage();

    void hideDeviceDetailsPage();

    void logout();
};

#endif // MAINWINDOW_H