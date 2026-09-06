#include "mainwindow.h"

#include "AuthManager.h"

#include "controllers/DeviceController.h"
#include "models/DeviceTableModel.h"
#include "pages/DeviceDetailsPage.h"
#include "pages/ForensicPage.h"

#include "ui_mainwindow.h"
#include "services/SanitizationRequestService.h"

#include "../../backend/classification/include/DeviceClassifier.h"
#include "../../backend/classification/include/ClassificationResult.h"
#include "../../backend/sanitization/include/SanitizationCapability.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableView>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "styles/AppTheme.h"


static bool requestMatchesDevice(
    const QString &requestedType,
    const StorageDevice &device)
{
    DeviceClassifier classifier;

    const ClassificationResult classification =
        classifier.classify(device);

    const QString requestType =
        requestedType.trimmed();

    if (requestType == QStringLiteral("SSD"))
    {
        return classification.mediaType ==
               MediaType::SSD;
    }

    if (requestType == QStringLiteral("HDD"))
    {
        return classification.mediaType ==
               MediaType::HDD;
    }

    if (requestType == QStringLiteral("USB Drive"))
    {
        return classification.busType ==
               BusType::USB;
    }

    if (requestType == QStringLiteral("NVMe SSD"))
    {
        return classification.busType ==
                   BusType::NVMe
               &&
               classification.mediaType ==
                   MediaType::SSD;
    }

    /*
     * "Other" must not automatically match a physical
     * device because there is currently no defined
     * sanitization mapping for it.
     */
    return false;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , authManager(new AuthManager(this))
    , sanitizationRequestService(
          new SanitizationRequestService(this))
    , deviceController(new DeviceController(this))
    , deviceTableModel(new DeviceTableModel(this))
    , deviceDetailsPage(nullptr)
    , forensicPage(nullptr)
    , refreshDevicesButton(nullptr)
{
    ui->setupUi(this);


    /*
     * =========================================================
     * Assigned Request Table
     * =========================================================
     */

    ui->recentJobsTable->setHorizontalHeaderLabels({
        QStringLiteral("Request ID"),
        QStringLiteral("Device"),
        QStringLiteral("Method"),
        QStringLiteral("Status")
    });

    ui->recentJobsTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    ui->recentJobsTable->setSelectionMode(
        QAbstractItemView::SingleSelection
    );

    ui->recentJobsTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );


    /*
     * ---------------------------------------------------------
     * Request Selection
     * ---------------------------------------------------------
     *
     * The selected request is the source of truth for:
     *
     *   selectedRequestId
     *   selectedRequestDeviceType
     *   selectedRequestMethod
     *
     * The employee does not choose the sanitization method
     * manually.
     */

    connect(
        ui->recentJobsTable,
        &QTableWidget::itemSelectionChanged,
        this,
        [this]()
        {
            const int row =
                ui->recentJobsTable->currentRow();

            if (row < 0)
            {
                selectedRequestId.clear();
                selectedRequestDeviceType.clear();
                selectedRequestMethod.clear();

                ui->methodComboBox->clear();
                ui->methodComboBox->setEnabled(false);

                ui->deviceComboBox->clear();

                ui->startWipeButton->setEnabled(false);

                return;
            }


            selectedRequestId =
                ui->recentJobsTable->item(row, 0)
                    ? ui->recentJobsTable
                          ->item(row, 0)
                          ->text()
                    : QString();


            selectedRequestDeviceType =
                ui->recentJobsTable->item(row, 1)
                    ? ui->recentJobsTable
                          ->item(row, 1)
                          ->text()
                    : QString();


            selectedRequestMethod =
                ui->recentJobsTable->item(row, 2)
                    ? ui->recentJobsTable
                          ->item(row, 2)
                          ->text()
                    : QString();


            /*
             * The assigned backend method is displayed,
             * but the employee cannot change it.
             */

            ui->methodComboBox->clear();

            if (!selectedRequestMethod.isEmpty())
            {
                ui->methodComboBox->addItem(
                    selectedRequestMethod
                );
            }

            ui->methodComboBox->setCurrentIndex(
                selectedRequestMethod.isEmpty()
                    ? -1
                    : 0
            );

            ui->methodComboBox->setEnabled(
                !selectedRequestMethod.isEmpty()
            );


            /*
             * Refresh physical devices so the Wipe page
             * is filtered against the newly selected request.
             */

            deviceController->refreshDevices();
        }
    );


    /*
     * =========================================================
     * Appearance
     * =========================================================
     */

    /*
     * The .ui file may contain older styles.
     * Clear them so AppTheme controls the application.
     */

    const QList<QWidget *> widgets =
        findChildren<QWidget *>();

    for (QWidget *widget : widgets)
    {
        widget->setStyleSheet("");
    }

    AppTheme::apply(this);


    /*
     * =========================================================
     * Devices Page
     * =========================================================
     */

    setupDevicesPage();

    /*
     * =========================================================
     * Forensics Page
     * =========================================================
     *
     * The existing Reports placeholder is reused so the current
     * desktop shell, navigation spacing and overall UI structure
     * remain unchanged.
     */

    ui->reportsPlaceholderLabel->hide();
    ui->reportsNavButton->setText(QStringLiteral("Forensics"));

    forensicPage =
        new ForensicPage(
            deviceController,
            ui->reportsPage
        );

    if (auto *reportsLayout =
            qobject_cast<QVBoxLayout *>(
                ui->reportsPage->layout()))
    {
        reportsLayout->setContentsMargins(0, 0, 0, 0);
        reportsLayout->setSpacing(0);
        reportsLayout->addWidget(forensicPage);
    }


    /*
     * =========================================================
     * Authentication
     * =========================================================
     */

    connect(
        ui->logoutButton,
        &QPushButton::clicked,
        this,
        &MainWindow::logout
    );


    connect(
        authManager,
        &AuthManager::loginSuccessful,
        this,
        [this]()
        {
            ui->stackedWidget->setCurrentWidget(
                ui->appPage
            );

            ui->contentStack->setCurrentWidget(
                ui->dashboardPage
            );

            setActiveNavButton(
                ui->dashboardNavButton
            );


            /*
             * Fetch requests assigned to the
             * currently logged-in employee.
             */

            sanitizationRequestService
                ->fetchAssignedRequests(
                    authManager->token()
                );


            /*
             * Discover physical storage devices
             * after successful login.
             */

            refreshDevices();
        }
    );


    connect(
        authManager,
        &AuthManager::loginFailed,
        this,
        [this](const QString &message)
        {
            ui->loginErrorLabel->setText(
                message
            );
        }
    );


    connect(
        ui->loginButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            ui->loginErrorLabel->clear();

            const QString email =
                ui->emailLineEdit
                    ->text()
                    .trimmed();


            if (email.isEmpty())
            {
                ui->loginErrorLabel->setText(
                    QStringLiteral(
                        "Email is required."
                    )
                );

                return;
            }


            QRegularExpression emailPattern(
                R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)"
            );


            if (!emailPattern
                    .match(email)
                    .hasMatch())
            {
                ui->loginErrorLabel->setText(
                    QStringLiteral(
                        "Please enter a valid email address."
                    )
                );

                return;
            }


            const QString password =
                ui->passwordLineEdit
                    ->text()
                    .trimmed();


            if (password.isEmpty())
            {
                ui->loginErrorLabel->setText(
                    QStringLiteral(
                        "Password is required."
                    )
                );

                return;
            }


            authManager->login(
                email,
                password
            );
        }
    );


    /*
     * =========================================================
     * Navigation
     * =========================================================
     */

    connect(
        ui->dashboardNavButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            ui->contentStack->setCurrentWidget(
                ui->dashboardPage
            );

            setActiveNavButton(
                ui->dashboardNavButton
            );
        }
    );


    connect(
        ui->devicesNavButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            ui->contentStack->setCurrentWidget(
                ui->devicesPage
            );

            setActiveNavButton(
                ui->devicesNavButton
            );

            /*
             * Refresh whenever the Devices page is opened.
             */

            refreshDevices();
        }
    );


    connect(
        ui->wipeNavButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            ui->contentStack->setCurrentWidget(
                ui->wipePage
            );

            setActiveNavButton(
                ui->wipeNavButton
            );


            if (selectedRequestId.isEmpty())
            {
                ui->statusLabel->setText(
                    QStringLiteral(
                        "Select an assigned request from Dashboard before sanitization."
                    )
                );
            }
            else
            {
                ui->statusLabel->setText(
                    QStringLiteral(
                        "Request: %1 | Device: %2 | Method: %3"
                    )
                        .arg(selectedRequestId)
                        .arg(selectedRequestDeviceType)
                        .arg(selectedRequestMethod)
                );
            }
        }
    );


    connect(
        ui->reportsNavButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            ui->contentStack->setCurrentWidget(
                ui->reportsPage
            );

            setActiveNavButton(
                ui->reportsNavButton
            );
        }
    );


    connect(
        ui->settingsNavButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            ui->contentStack->setCurrentWidget(
                ui->settingsPage
            );

            setActiveNavButton(
                ui->settingsNavButton
            );
        }
    );


    /*
     * =========================================================
     * Device Controller → Frontend
     * =========================================================
     */

    connect(
        deviceController,
        &DeviceController::devicesUpdated,
        this,
        [this]()
        {
            deviceTableModel->setDevices(
                deviceController->devices()
            );


            /*
             * Keep the Wipe page device selector synchronized
             * with the latest physical-device discovery.
             *
             * Only devices matching the selected request type
             * are shown.
             */

            ui->deviceComboBox->clear();

            const auto &devices =
                deviceController->devices();


            for (int deviceIndex = 0;
                 deviceIndex < static_cast<int>(
                     devices.size());
                 ++deviceIndex)
            {
                const StorageDevice &device =
                    devices[deviceIndex];


                if (!selectedRequestDeviceType.isEmpty())
                {
                    if (!requestMatchesDevice(
                            selectedRequestDeviceType,
                            device))
                    {
                        continue;
                    }
                }


                QString label =
                    QString::fromStdString(
                        device.getModel()
                    );


                label +=
                    QStringLiteral(" (");


                label +=
                    QString::fromStdString(
                        device.getDeviceId()
                    );


                label +=
                    QStringLiteral(")");


                /*
                 * Store the ORIGINAL device index.
                 *
                 * This is important because the combo box
                 * may contain only a filtered subset.
                 */

                ui->deviceComboBox->addItem(
                    label,
                    deviceIndex
                );
            }


            /*
             * Do not automatically select a physical device.
             */

            ui->deviceComboBox->setCurrentIndex(-1);

            ui->startWipeButton->setEnabled(false);
        }
    );


    connect(
        deviceController,
        &DeviceController::discoveryFailed,
        this,
        [this](const QString &message)
        {
            QMessageBox::warning(
                this,
                QStringLiteral(
                    "Storage Discovery"
                ),
                message
            );
        }
    );


    connect(
        deviceController,
        &DeviceController::safetyCheckPassed,
        this,
        [this]()
        {
            if (!deviceDetailsPage)
            {
                return;
            }

            deviceDetailsPage->updateSafetyStatus(
                true
            );
        }
    );


    connect(
        deviceController,
        &DeviceController::safetyCheckFailed,
        this,
        [this](const QString &message)
        {
            if (!deviceDetailsPage)
            {
                return;
            }

            deviceDetailsPage->updateSafetyStatus(
                false,
                message
            );
        }
    );


    /*
     * =========================================================
     * Assigned Requests → Dashboard
     * =========================================================
     */

    connect(
        sanitizationRequestService,
        &SanitizationRequestService::assignedRequestsFetched,
        this,
        [this](const QJsonArray &requests)
        {
            int totalCount = requests.size();
            int completedCount = 0;
            int failedCount = 0;
            int inProgressCount = 0;


            ui->recentJobsTable->setRowCount(
                0
            );


            for (const QJsonValue &value : requests)
            {
                if (!value.isObject())
                {
                    continue;
                }


                const QJsonObject request =
                    value.toObject();


                const int row =
                    ui->recentJobsTable->rowCount();


                ui->recentJobsTable->insertRow(
                    row
                );


                const QString requestId =
                    request.value(
                        QStringLiteral("requestId")
                    ).toString();


                const QString deviceType =
                    request.value(
                        QStringLiteral("deviceType")
                    ).toString();


                const QString method =
                    request.value(
                        QStringLiteral(
                            "sanitizationMethod"
                        )
                    ).toString();


                const QString status =
                    request.value(
                        QStringLiteral("status")
                    ).toString();


                if (status ==
                    QStringLiteral("COMPLETED"))
                {
                    completedCount++;
                }
                else if (status ==
                         QStringLiteral("FAILED"))
                {
                    failedCount++;
                }
                else if (status ==
                         QStringLiteral("IN_PROGRESS"))
                {
                    inProgressCount++;
                }


                ui->recentJobsTable->setItem(
                    row,
                    0,
                    new QTableWidgetItem(
                        requestId
                    )
                );


                ui->recentJobsTable->setItem(
                    row,
                    1,
                    new QTableWidgetItem(
                        deviceType
                    )
                );


                ui->recentJobsTable->setItem(
                    row,
                    2,
                    new QTableWidgetItem(
                        method
                    )
                );


                ui->recentJobsTable->setItem(
                    row,
                    3,
                    new QTableWidgetItem(
                        status
                    )
                );
            }


            ui->totalJobsValue->setText(
                QString::number(
                    totalCount
                )
            );


            ui->completedJobsValue->setText(
                QString::number(
                    completedCount
                )
            );


            ui->failedJobsValue->setText(
                QString::number(
                    failedCount
                )
            );


            ui->inProgressValue->setText(
    QString::number(inProgressCount)
);


            ui->recentJobsTable
                ->resizeColumnsToContents();
        }
    );


    connect(
        sanitizationRequestService,
        &SanitizationRequestService::requestFetchFailed,
        this,
        [this](const QString &message)
        {
            ui->recentJobsTable->setRowCount(
                0
            );


            QMessageBox::warning(
                this,
                QStringLiteral(
                    "Assigned Requests"
                ),
                message
            );
        }
    );


    /*
     * =========================================================
     * Wipe Target Selection
     * =========================================================
     *
     * The combo box contains only devices matching the
     * selected request.
     *
     * itemData() stores the original index in the complete
     * DeviceController device list.
     */

    connect(
        ui->deviceComboBox,
        QOverload<int>::of(
            &QComboBox::currentIndexChanged
        ),
        this,
        [this](int index)
        {
            if (index < 0)
            {
                ui->startWipeButton->setEnabled(
                    false
                );

                return;
            }


            bool ok = false;


            const int deviceIndex =
                ui->deviceComboBox
                    ->itemData(index)
                    .toInt(&ok);


            if (!ok)
            {
                ui->statusLabel->setText(
                    QStringLiteral(
                        "Unable to identify selected device."
                    )
                );


                ui->startWipeButton->setEnabled(
                    false
                );

                return;
            }


            if (!deviceController->selectTarget(
                    deviceIndex))
            {
                ui->statusLabel->setText(
                    QStringLiteral(
                        "Unable to select target device."
                    )
                );


                ui->startWipeButton->setEnabled(
                    false
                );

                return;
            }


            /*
             * Capability detection is informational at this
             * stage. It does not authorize sanitization.
             */

            const SanitizationCapability capability =
                deviceController
                    ->detectSelectedTargetCapability();


            QString capabilityText;


            if (capability.nativeSanitizeSupported ==
                NativeSanitizeSupport::SUPPORTED)
            {
                capabilityText =
                    QStringLiteral(
                        "Native sanitization supported."
                    );
            }
            else if (capability.isUsbDevice &&
                     capability.scsiPathAvailable)
            {
                capabilityText =
                    QStringLiteral(
                        "USB/SCSI sanitization path detected."
                    );
            }
            else
            {
                capabilityText =
                    QStringLiteral(
                        "No supported sanitization method detected."
                    );
            }


            ui->statusLabel->setText(
                QStringLiteral(
                    "Target device selected. %1"
                ).arg(capabilityText)
            );


            const bool requestSelected =
                !selectedRequestId.isEmpty();


            const bool deviceSelected =
                ui->deviceComboBox
                    ->currentIndex() >= 0;


            ui->startWipeButton->setEnabled(
                requestSelected &&
                deviceSelected
            );
        }
    );


    /*
     * Never enable Start Sanitization automatically.
     */

    ui->startWipeButton->setEnabled(
        false
    );


    /*
     * =========================================================
     * Start Sanitization
     * =========================================================
     *
     * Current stage:
     *
     *   1. Request must be selected.
     *   2. Physical target must be selected.
     *   3. Target is freshly discovered.
     *   4. Request/device type must match.
     *   5. SafetyEngine must approve the target.
     *
     * Actual sanitization is intentionally NOT invoked yet.
     */

    connect(
        ui->startWipeButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            /*
             * -------------------------------------------------
             * Step 1: Assigned request
             * -------------------------------------------------
             */

            if (selectedRequestId.isEmpty())
            {
                ui->statusLabel->setText(
                    QStringLiteral(
                        "Select an assigned request before sanitization."
                    )
                );

                return;
            }


            /*
             * -------------------------------------------------
             * Step 2: Physical device
             * -------------------------------------------------
             */

            if (!deviceController
                    ->selectedTarget()
                    .has_value())
            {
                ui->statusLabel->setText(
                    QStringLiteral(
                        "Select a physical storage device before sanitization."
                    )
                );

                return;
            }


            /*
             * -------------------------------------------------
             * Step 3: Fresh target validation
             * -------------------------------------------------
             *
             * Rediscover the physical devices and make sure
             * the selected target still has the same identity.
             */

            if (!deviceController
                    ->validateSelectedTarget())
            {
                return;
            }


            const auto &selectedTarget =
                deviceController
                    ->selectedTarget();


            if (!selectedTarget.has_value())
            {
                ui->statusLabel->setText(
                    QStringLiteral(
                        "Validated target is no longer available."
                    )
                );

                return;
            }


            /*
             * -------------------------------------------------
             * Step 4: Request ↔ physical device matching
             * -------------------------------------------------
             */

            if (!requestMatchesDevice(
                    selectedRequestDeviceType,
                    *selectedTarget))
            {
                ui->statusLabel->setText(
                    QStringLiteral(
                        "Selected physical device does not match "
                        "the assigned request."
                    )
                );


                QMessageBox::warning(
                    this,
                    QStringLiteral(
                        "Device Mismatch"
                    ),
                    QStringLiteral(
                        "The selected physical device does not "
                        "match the device type assigned to this "
                        "request.\n\n"
                        "Sanitization has been blocked."
                    )
                );


                return;
            }


            /*
             * -------------------------------------------------
             * Step 5: Safety evaluation
             * -------------------------------------------------
             */

            if (!deviceController
                    ->evaluateSelectedTarget())
            {
                return;
            }


            /*
             * -------------------------------------------------
             * Current stopping point
             * -------------------------------------------------
             *
             * We have not started a destructive operation.
             */

            ui->statusLabel->setText(
                QStringLiteral(
                    "Safety checks passed. Ready for sanitization."
                )
            );


            QMessageBox::information(
                this,
                QStringLiteral(
                    "Safety Check"
                ),
                QStringLiteral(
                    "All safety checks passed.\n\n"
                    "The target device is ready for sanitization."
                )
            );


            /*
             * IMPORTANT:
             *
             * Actual SanitizationEngine::sanitize() is NOT
             * called here yet.
             *
             * Backend request status is also NOT changed here.
             *
             * Final confirmation + structured result +
             * execution + verification will be added next.
             */
        }
    );
}


/*
 * =============================================================
 * Destructor
 * =============================================================
 */

MainWindow::~MainWindow()
{
    delete ui;
}


/*
 * =============================================================
 * Devices Page
 * =============================================================
 */

void MainWindow::setupDevicesPage()
{
    /*
     * The existing UI contains a placeholder label.
     *
     * We keep the .ui file untouched and build the actual
     * device integration UI here.
     */

    ui->devicesPlaceholderLabel->hide();


    /*
     * Main devices layout
     */

    QVBoxLayout *layout =
        qobject_cast<QVBoxLayout *>(
            ui->devicesPage->layout()
        );


    if (!layout)
    {
        layout =
            new QVBoxLayout(
                ui->devicesPage
            );

        ui->devicesPage->setLayout(
            layout
        );
    }


    /*
     * ---------------------------------------------------------
     * Header
     * ---------------------------------------------------------
     */

    QLabel *titleLabel =
        new QLabel(
            QStringLiteral(
                "Storage Devices"
            ),
            ui->devicesPage
        );


    titleLabel->setStyleSheet(
        "QLabel {"
        "color: #172033;"
        "font-size: 24px;"
        "font-weight: 600;"
        "}"
    );


    QLabel *subtitleLabel =
        new QLabel(
            QStringLiteral(
                "Physical storage devices detected by SecureWipe."
            ),
            ui->devicesPage
        );


    subtitleLabel->setStyleSheet(
        "QLabel {"
        "color: #667085;"
        "font-size: 13px;"
        "}"
    );


    /*
     * ---------------------------------------------------------
     * Refresh button
     * ---------------------------------------------------------
     */

    refreshDevicesButton =
        new QPushButton(
            QStringLiteral(
                "Refresh Devices"
            ),
            ui->devicesPage
        );


    refreshDevicesButton->setMinimumHeight(
        38
    );


    refreshDevicesButton->setMinimumWidth(
        150
    );


    refreshDevicesButton->setCursor(
        Qt::PointingHandCursor
    );


    refreshDevicesButton->setStyleSheet(
        "QPushButton {"
        "background-color: #2563EB;"
        "color: white;"
        "border: none;"
        "border-radius: 6px;"
        "padding: 8px 16px;"
        "font-size: 13px;"
        "font-weight: 500;"
        "}"
        ""
        "QPushButton:hover {"
        "background-color: #1D4ED8;"
        "}"
        ""
        "QPushButton:pressed {"
        "background-color: #1E40AF;"
        "}"
        ""
        "QPushButton:disabled {"
        "background-color: #CBD5E1;"
        "color: #64748B;"
        "}"
    );


    connect(
        refreshDevicesButton,
        &QPushButton::clicked,
        this,
        &MainWindow::refreshDevices
    );


    QHBoxLayout *headerLayout =
        new QHBoxLayout();


    headerLayout->addWidget(
        titleLabel
    );


    headerLayout->addStretch();


    headerLayout->addWidget(
        refreshDevicesButton
    );


    /*
     * ---------------------------------------------------------
     * Device table
     * ---------------------------------------------------------
     */

    QTableView *deviceTable =
        new QTableView(
            ui->devicesPage
        );


    deviceTable->setModel(
        deviceTableModel
    );


    deviceTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );


    deviceTable->setSelectionMode(
        QAbstractItemView::SingleSelection
    );


    deviceTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );


    deviceTable->setAlternatingRowColors(
        true
    );


    deviceTable->setShowGrid(
        false
    );


    deviceTable->verticalHeader()
        ->setVisible(false);


    deviceTable->horizontalHeader()
        ->setStretchLastSection(true);


    deviceTable->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::ResizeToContents
        );


    deviceTable->setMinimumHeight(
        300
    );


    deviceTable->setStyleSheet(
        "QTableView {"
        "background-color: #FFFFFF;"
        "alternate-background-color: #F8FAFC;"
        "border: 1px solid #E2E8F0;"
        "border-radius: 8px;"
        "color: #172033;"
        "font-size: 13px;"
        "selection-background-color: #DBEAFE;"
        "selection-color: #172033;"
        "}"
        ""
        "QHeaderView::section {"
        "background-color: #F8FAFC;"
        "color: #475467;"
        "border: none;"
        "border-bottom: 1px solid #E2E8F0;"
        "padding: 10px;"
        "font-size: 12px;"
        "font-weight: 600;"
        "}"
    );


    /*
     * Double-click → device details
     */

    connect(
        deviceTable,
        &QTableView::doubleClicked,
        this,
        [this](const QModelIndex &)
        {
            showSelectedDeviceDetails();
        }
    );


    /*
     * ---------------------------------------------------------
     * Page layout
     * ---------------------------------------------------------
     */

    layout->setContentsMargins(
        28,
        24,
        28,
        24
    );


    layout->setSpacing(
        6
    );


    layout->addLayout(
        headerLayout
    );


    layout->addWidget(
        subtitleLabel
    );


    layout->addSpacing(
        14
    );


    layout->addWidget(
        deviceTable
    );
}


/*
 * =============================================================
 * Refresh Devices
 * =============================================================
 */

void MainWindow::refreshDevices()
{
    if (!refreshDevicesButton)
    {
        return;
    }


    refreshDevicesButton->setEnabled(
        false
    );


    refreshDevicesButton->setText(
        QStringLiteral(
            "Scanning..."
        )
    );


    /*
     * Current backend discovery is synchronous.
     *
     * We will move this to a worker thread later when the
     * sanitization workflow is integrated.
     */

    deviceController->refreshDevices();


    refreshDevicesButton->setText(
        QStringLiteral(
            "Refresh Devices"
        )
    );


    refreshDevicesButton->setEnabled(
        true
    );
}


/*
 * =============================================================
 * Device Details
 * =============================================================
 */

void MainWindow::showSelectedDeviceDetails()
{
    const QList<QTableView *> tables =
        ui->devicesPage
            ->findChildren<QTableView *>();


    if (tables.isEmpty())
    {
        return;
    }


    QTableView *table =
        tables.first();


    const QModelIndex currentIndex =
        table->currentIndex();


    if (!currentIndex.isValid())
    {
        return;
    }


    const StorageDevice *device =
        deviceTableModel->deviceAt(
            currentIndex.row()
        );


    if (!device)
    {
        return;
    }


    showDeviceDetails(
        *device
    );
}


void MainWindow::showDeviceDetails(
    const StorageDevice &device)
{
    /*
     * Remove the old details page if one already exists.
     */

    if (deviceDetailsPage)
    {
        ui->contentStack->removeWidget(
            deviceDetailsPage
        );


        deviceDetailsPage->deleteLater();


        deviceDetailsPage = nullptr;
    }


    /*
     * Create a new details page for the selected device.
     */

    deviceDetailsPage =
        new DeviceDetailsPage(
            device,
            ui->contentStack
        );


    /*
     * Add the page to the application's stacked
     * content area.
     */

    ui->contentStack->addWidget(
        deviceDetailsPage
    );


    /*
     * Show Device Details.
     */

    ui->contentStack->setCurrentWidget(
        deviceDetailsPage
    );


    /*
     * Keep Devices navigation active.
     */

    setActiveNavButton(
        ui->devicesNavButton
    );


    /*
     * Back → Devices
     */

    connect(
        deviceDetailsPage,
        &DeviceDetailsPage::backRequested,
        this,
        &MainWindow::showDevicesPage
    );


    /*
     * Refresh → rediscover devices.
     */

    connect(
        deviceDetailsPage,
        &DeviceDetailsPage::refreshRequested,
        this,
        [this]()
        {
            showDevicesPage();
            refreshDevices();
        }
    );
}


void MainWindow::showDevicesPage()
{
    ui->contentStack->setCurrentWidget(
        ui->devicesPage
    );


    setActiveNavButton(
        ui->devicesNavButton
    );
}


void MainWindow::hideDeviceDetailsPage()
{
    showDevicesPage();
}


/*
 * =============================================================
 * Navigation Styling
 * =============================================================
 */

void MainWindow::setActiveNavButton(
    QPushButton *activeButton)
{
    const QString inactiveStyle =
        "QPushButton {"
        "background-color: transparent;"
        "border: none;"
        "border-radius: 7px;"
        "color: #475467;"
        "font-size: 13px;"
        "font-weight: 500;"
        "text-align: left;"
        "padding: 10px 12px;"
        "}"
        ""
        "QPushButton:hover {"
        "background-color: #F2F4F7;"
        "color: #172033;"
        "}";


    const QString activeStyle =
        "QPushButton {"
        "background-color: #EFF6FF;"
        "border: none;"
        "border-radius: 7px;"
        "color: #1D4ED8;"
        "font-size: 13px;"
        "font-weight: 600;"
        "text-align: left;"
        "padding: 10px 12px;"
        "}"
        ""
        "QPushButton:hover {"
        "background-color: #DBEAFE;"
        "color: #1D4ED8;"
        "}";


    const QList<QPushButton *> navButtons = {
        ui->dashboardNavButton,
        ui->devicesNavButton,
        ui->wipeNavButton,
        ui->reportsNavButton,
        ui->settingsNavButton
    };


    for (QPushButton *button : navButtons)
    {
        if (!button)
        {
            continue;
        }


        button->setStyleSheet(
            button == activeButton
                ? activeStyle
                : inactiveStyle
        );
    }
}


/*
 * =============================================================
 * Logout
 * =============================================================
 */

void MainWindow::logout()
{
    ui->passwordLineEdit->clear();

    ui->loginErrorLabel->clear();

    ui->stackedWidget->setCurrentWidget(
        ui->loginPage
    );
}