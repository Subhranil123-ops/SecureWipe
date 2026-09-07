#include "mainwindow.h"

#include "AuthManager.h"
#include "controllers/DeviceController.h"
#include "models/DeviceTableModel.h"
#include "pages/DeviceDetailsPage.h"
#include "pages/ForensicPage.h"
#include "services/SanitizationRequestService.h"
#include "ui_mainwindow.h"
#include "styles/AppTheme.h"

#include "../../backend/classification/include/ClassificationResult.h"
#include "../../backend/classification/include/DeviceClassifier.h"
#include "../../backend/sanitization/include/SanitizationCapability.h"
#include "../../backend/sanitization/include/SanitizationEngine.h"
#include "../../backend/safety/include/SafetyResult.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QFrame>
#include <QFutureWatcher>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

static bool requestMatchesDevice(const QString &requestedType, const StorageDevice &device)
{
    DeviceClassifier classifier;
    const ClassificationResult classification = classifier.classify(device);
    const QString requestType = requestedType.trimmed();
    if (requestType == QStringLiteral("SSD"))
        return classification.mediaType == MediaType::SSD;
    if (requestType == QStringLiteral("HDD"))
        return classification.mediaType == MediaType::HDD;
    if (requestType == QStringLiteral("USB Drive"))
        return classification.busType == BusType::USB;
    if (requestType == QStringLiteral("NVMe SSD"))
        return classification.busType == BusType::NVMe && classification.mediaType == MediaType::SSD;
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
            const int row = ui->recentJobsTable->currentRow();
            if (row < 0)
            {
                selectedRequestId.clear();
                selectedRequestDeviceType.clear();
                selectedRequestMethod.clear();
                if (wipeRequestComboBox)
                    wipeRequestComboBox->setCurrentIndex(-1);
                populateWipeDevices();
                return;
            }

            selectedRequestId = ui->recentJobsTable->item(row, 0)
                ? ui->recentJobsTable->item(row, 0)->text() : QString();
            selectedRequestDeviceType = ui->recentJobsTable->item(row, 1)
                ? ui->recentJobsTable->item(row, 1)->text() : QString();
            selectedRequestMethod = ui->recentJobsTable->item(row, 2)
                ? ui->recentJobsTable->item(row, 2)->text() : QString();

            if (wipeRequestComboBox)
            {
                const int requestIndex = wipeRequestComboBox->findData(selectedRequestId, Qt::UserRole);
                if (requestIndex >= 0)
                    wipeRequestComboBox->setCurrentIndex(requestIndex);
            }
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

    setupWipePage();

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
            ui->contentStack->setCurrentWidget(ui->wipePage);
            setActiveNavButton(ui->wipeNavButton);

            if (wipeRequestComboBox)
            {
                int requestIndex = wipeRequestComboBox->findData(selectedRequestId, Qt::UserRole);
                if (requestIndex < 0 && wipeRequestComboBox->count() > 0)
                    requestIndex = 0;
                if (requestIndex >= 0)
                    wipeRequestComboBox->setCurrentIndex(requestIndex);
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
            deviceTableModel->setDevices(deviceController->devices());
            populateWipeDevices();
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

            ui->recentJobsTable->setRowCount(0);

            if (wipeRequestComboBox)
            {
                wipeRequestComboBox->blockSignals(true);
                wipeRequestComboBox->clear();
            }

            for (const QJsonValue &value : requests)
            {
                if (!value.isObject())
                    continue;

                const QJsonObject request = value.toObject();
                const QString requestId = request.value(QStringLiteral("requestId")).toString();
                const QString deviceType = request.value(QStringLiteral("deviceType")).toString();
                const QString method = request.value(QStringLiteral("sanitizationMethod")).toString();
                const QString status = request.value(QStringLiteral("status")).toString();

                if (status == QStringLiteral("COMPLETED"))
                    ++completedCount;
                else if (status == QStringLiteral("FAILED"))
                    ++failedCount;
                else if (status == QStringLiteral("IN_PROGRESS"))
                    ++inProgressCount;

                const int row = ui->recentJobsTable->rowCount();
                ui->recentJobsTable->insertRow(row);
                ui->recentJobsTable->setItem(row, 0, new QTableWidgetItem(requestId));
                ui->recentJobsTable->setItem(row, 1, new QTableWidgetItem(deviceType));
                ui->recentJobsTable->setItem(row, 2, new QTableWidgetItem(method));
                ui->recentJobsTable->setItem(row, 3, new QTableWidgetItem(status));

                if (wipeRequestComboBox)
                {
                    const QString display = QStringLiteral("%1  •  %2  •  %3")
                        .arg(requestId,
                             deviceType,
                             method.isEmpty() ? QStringLiteral("Method not specified") : method);
                    wipeRequestComboBox->addItem(display);
                    const int requestIndex = wipeRequestComboBox->count() - 1;
                    wipeRequestComboBox->setItemData(requestIndex, requestId, Qt::UserRole);
                    wipeRequestComboBox->setItemData(requestIndex, deviceType, Qt::UserRole + 1);
                    wipeRequestComboBox->setItemData(requestIndex, method, Qt::UserRole + 2);
                }
            }

            ui->totalJobsValue->setText(QString::number(totalCount));
            ui->completedJobsValue->setText(QString::number(completedCount));
            ui->failedJobsValue->setText(QString::number(failedCount));
            ui->inProgressValue->setText(QString::number(inProgressCount));
            ui->recentJobsTable->resizeColumnsToContents();

            if (wipeRequestComboBox)
            {
                int requestIndex = wipeRequestComboBox->findData(selectedRequestId, Qt::UserRole);
                if (requestIndex < 0 && wipeRequestComboBox->count() > 0)
                    requestIndex = 0;
                wipeRequestComboBox->blockSignals(false);
                wipeRequestComboBox->setCurrentIndex(requestIndex);
            }
        }
    );

    connect(
        sanitizationRequestService,
        &SanitizationRequestService::requestFetchFailed,
        this,
        [this](const QString &message)
        {
            ui->recentJobsTable->setRowCount(0);
            if (wipeRequestComboBox)
            {
                wipeRequestComboBox->blockSignals(true);
                wipeRequestComboBox->clear();
                wipeRequestComboBox->blockSignals(false);
            }
            wipeStatusLabel->setText(QStringLiteral("Could not load assigned requests: %1").arg(message));
            wipeStatusLabel->setStyleSheet("color:#B42318; font-size:12px; font-weight:600;");
            QMessageBox::warning(this, QStringLiteral("Assigned Requests"), message);
        }
    );

    connect(
        sanitizationRequestService,
        &SanitizationRequestService::requestStatusUpdated,
        this,
        [this](const QString &requestId, const QString &status)
        {
            wipeStatusLabel->setText(QStringLiteral("Sanitization finished. Request %1 is now %2.").arg(requestId, status));
        }
    );

    connect(
        sanitizationRequestService,
        &SanitizationRequestService::requestStatusUpdateFailed,
        this,
        [this](const QString &message)
        {
            wipeStatusLabel->setText(QStringLiteral("Sanitization finished locally, but request sync failed: %1").arg(message));
            wipeStatusLabel->setStyleSheet("color:#B54708; font-size:12px; font-weight:600;");
        }
    );

    /*
     * The Wipe workspace is built programmatically in setupWipePage().
     * The legacy Designer controls remain hidden and are not used.
     */
}



namespace
{
void clearLayoutItems(QLayout *layout)
{
    if (!layout)
        return;

    while (QLayoutItem *item = layout->takeAt(0))
    {
        if (QLayout *childLayout = item->layout())
        {
            clearLayoutItems(childLayout);
            delete childLayout;
        }

        if (QWidget *widget = item->widget())
            widget->hide();

        delete item;
    }
}

QLabel *makeSectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setStyleSheet("color:#172033; font-size:15px; font-weight:700;");
    return label;
}

QLabel *makeFieldCaption(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setStyleSheet("color:#667085; font-size:11px; font-weight:600;");
    return label;
}

QLabel *makeFieldValue(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setStyleSheet("color:#172033; font-size:13px; font-weight:600;");
    return label;
}

QFrame *makeCard(QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setFrameShape(QFrame::StyledPanel);
    card->setStyleSheet("QFrame { background:#FFFFFF; border:1px solid #E4E7EC; border-radius:12px; }");
    return card;
}
}

void MainWindow::setupWipePage()
{
    if (!ui || !ui->wipePage)
        return;

    QVBoxLayout *root = qobject_cast<QVBoxLayout *>(ui->wipePage->layout());
    if (!root)
    {
        root = new QVBoxLayout(ui->wipePage);
        ui->wipePage->setLayout(root);
    }

    clearLayoutItems(root);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(14);
    ui->wipePage->setStyleSheet("QWidget#wipePage { background:#F6F8FB; }");

    auto *title = new QLabel(QStringLiteral("Sanitize Storage Device"), ui->wipePage);
    title->setStyleSheet("color:#101828; font-size:25px; font-weight:700;");
    auto *subtitle = new QLabel(
        QStringLiteral("Follow the four visible steps below. SecureWipe will block the operation when the target is unsafe."),
        ui->wipePage);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet("color:#667085; font-size:13px;");
    root->addWidget(title);
    root->addWidget(subtitle);

    auto *steps = new QHBoxLayout;
    steps->setSpacing(8);
    const QStringList stepTexts = {QStringLiteral("1  Request"), QStringLiteral("2  Target"), QStringLiteral("3  Safety"), QStringLiteral("4  Sanitize")};
    for (int i = 0; i < stepTexts.size(); ++i)
    {
        auto *step = new QLabel(stepTexts.at(i), ui->wipePage);
        step->setAlignment(Qt::AlignCenter);
        step->setMinimumHeight(30);
        step->setStyleSheet(i == 0
            ? "QLabel { background:#EAF2FF; color:#175CD3; border:1px solid #B2CCFF; border-radius:8px; padding:5px 10px; font-size:11px; font-weight:700; }"
            : "QLabel { background:#FFFFFF; color:#667085; border:1px solid #E4E7EC; border-radius:8px; padding:5px 10px; font-size:11px; font-weight:600; }");
        steps->addWidget(step, 1);
    }
    root->addLayout(steps);

    QFrame *requestCard = makeCard(ui->wipePage);
    auto *requestLayout = new QVBoxLayout(requestCard);
    requestLayout->setContentsMargins(16, 14, 16, 14);
    requestLayout->setSpacing(8);
    requestLayout->addWidget(makeSectionTitle(QStringLiteral("1. Assigned Request"), requestCard));

    auto *requestCaption = makeFieldCaption(QStringLiteral("Select the work order you are authorized to execute"), requestCard);
    requestLayout->addWidget(requestCaption);

    wipeRequestComboBox = new QComboBox(requestCard);
    wipeRequestComboBox->setMinimumHeight(42);
    wipeRequestComboBox->setCursor(Qt::PointingHandCursor);
    wipeRequestComboBox->setStyleSheet(
        "QComboBox { background:#FFFFFF; color:#172033; border:1px solid #D0D5DD; border-radius:8px; padding:8px 12px; font-size:13px; }"
        "QComboBox:hover { border:1px solid #98A2B3; }"
        "QComboBox:focus { border:1px solid #2563EB; }"
        "QComboBox::drop-down { width:32px; border-left:1px solid #EAECF0; }"
        "QComboBox QAbstractItemView { background:#FFFFFF; color:#172033; border:1px solid #D0D5DD; selection-background-color:#EAF2FF; selection-color:#172033; padding:4px; }"
    );
    requestLayout->addWidget(wipeRequestComboBox);

    wipeRequestSummaryLabel = new QLabel(QStringLiteral("No assigned request selected."), requestCard);
    wipeRequestSummaryLabel->setWordWrap(true);
    wipeRequestSummaryLabel->setStyleSheet("color:#667085; font-size:12px;");
    requestLayout->addWidget(wipeRequestSummaryLabel);
    root->addWidget(requestCard);

    auto *workspace = new QHBoxLayout;
    workspace->setSpacing(14);

    QFrame *targetCard = makeCard(ui->wipePage);
    auto *targetLayout = new QVBoxLayout(targetCard);
    targetLayout->setContentsMargins(16, 14, 16, 14);
    targetLayout->setSpacing(8);

    auto *targetHeader = new QHBoxLayout;
    targetHeader->addWidget(makeSectionTitle(QStringLiteral("2. Target Device"), targetCard));
    targetHeader->addStretch();
    wipeRefreshButton = new QPushButton(QStringLiteral("Refresh"), targetCard);
    wipeRefreshButton->setMinimumHeight(34);
    wipeRefreshButton->setCursor(Qt::PointingHandCursor);
    wipeRefreshButton->setStyleSheet("QPushButton { background:#FFFFFF; color:#344054; border:1px solid #D0D5DD; border-radius:8px; padding:7px 12px; font-size:12px; font-weight:600; } QPushButton:hover { background:#F9FAFB; border-color:#98A2B3; }");
    targetHeader->addWidget(wipeRefreshButton);
    targetLayout->addLayout(targetHeader);

    auto *targetHint = new QLabel(QStringLiteral("Only devices matching the selected request are shown."), targetCard);
    targetHint->setStyleSheet("color:#667085; font-size:11px;");
    targetLayout->addWidget(targetHint);

    wipeDeviceTable = new QTableWidget(targetCard);
    wipeDeviceTable->setColumnCount(6);
    wipeDeviceTable->setHorizontalHeaderLabels({QStringLiteral("Model"), QStringLiteral("Interface"), QStringLiteral("Capacity"), QStringLiteral("Device"), QStringLiteral("System"), QStringLiteral("Type")});
    wipeDeviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    wipeDeviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    wipeDeviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    wipeDeviceTable->setFocusPolicy(Qt::StrongFocus);
    wipeDeviceTable->setShowGrid(false);
    wipeDeviceTable->verticalHeader()->setVisible(false);
    wipeDeviceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    wipeDeviceTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    wipeDeviceTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    wipeDeviceTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    wipeDeviceTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    wipeDeviceTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    wipeDeviceTable->setMinimumHeight(280);
    wipeDeviceTable->setStyleSheet(
        "QTableWidget { background:#FFFFFF; color:#172033; border:1px solid #E4E7EC; border-radius:8px; gridline-color:transparent; selection-background-color:#EAF2FF; selection-color:#172033; font-size:12px; }"
        "QTableWidget::item { padding:7px; border-bottom:1px solid #F2F4F7; }"
        "QHeaderView::section { background:#F9FAFB; color:#667085; border:none; border-bottom:1px solid #E4E7EC; padding:8px; font-size:10px; font-weight:700; }"
    );
    targetLayout->addWidget(wipeDeviceTable, 1);
    workspace->addWidget(targetCard, 3);

    QFrame *statusCard = makeCard(ui->wipePage);
    auto *statusLayout = new QVBoxLayout(statusCard);
    statusLayout->setContentsMargins(18, 14, 18, 14);
    statusLayout->setSpacing(10);
    statusLayout->addWidget(makeSectionTitle(QStringLiteral("3. Pre-flight Safety"), statusCard));

    wipeSafetyBadgeLabel = new QLabel(QStringLiteral("NOT CHECKED"), statusCard);
    wipeSafetyBadgeLabel->setAlignment(Qt::AlignCenter);
    wipeSafetyBadgeLabel->setMinimumHeight(30);
    wipeSafetyBadgeLabel->setStyleSheet("QLabel { background:#FFF7E8; color:#B54708; border:1px solid #FAD7A0; border-radius:8px; padding:5px 10px; font-size:11px; font-weight:700; }");
    statusLayout->addWidget(wipeSafetyBadgeLabel);

    auto *targetCaption = makeFieldCaption(QStringLiteral("Selected target"), statusCard);
    statusLayout->addWidget(targetCaption);
    wipeTargetValueLabel = makeFieldValue(QStringLiteral("None"), statusCard);
    statusLayout->addWidget(wipeTargetValueLabel);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(8);
    grid->addWidget(makeFieldCaption(QStringLiteral("Capacity"), statusCard), 0, 0);
    grid->addWidget(makeFieldCaption(QStringLiteral("Interface"), statusCard), 0, 1);
    wipeCapacityValueLabel = makeFieldValue(QStringLiteral("—"), statusCard);
    wipeInterfaceValueLabel = makeFieldValue(QStringLiteral("—"), statusCard);
    grid->addWidget(wipeCapacityValueLabel, 1, 0);
    grid->addWidget(wipeInterfaceValueLabel, 1, 1);
    statusLayout->addLayout(grid);

    statusLayout->addWidget(makeFieldCaption(QStringLiteral("Execution method"), statusCard));
    wipeMethodValueLabel = makeFieldValue(QStringLiteral("Select a target"), statusCard);
    statusLayout->addWidget(wipeMethodValueLabel);

    wipeSafetyChecksLabel = new QLabel(QStringLiteral("Safety checks have not been run."), statusCard);
    wipeSafetyChecksLabel->setWordWrap(true);
    wipeSafetyChecksLabel->setStyleSheet("color:#667085; font-size:11px; line-height:150%;");
    statusLayout->addWidget(wipeSafetyChecksLabel, 1);
    workspace->addWidget(statusCard, 2);

    root->addLayout(workspace, 1);

    QFrame *actionCard = makeCard(ui->wipePage);
    auto *actionLayout = new QVBoxLayout(actionCard);
    actionLayout->setContentsMargins(16, 14, 16, 14);
    actionLayout->setSpacing(8);

    auto *actionHeader = new QHBoxLayout;
    actionHeader->addWidget(makeSectionTitle(QStringLiteral("4. Sanitization"), actionCard));
    actionHeader->addStretch();
    wipeStatusLabel = new QLabel(QStringLiteral("Choose a request and target device."), actionCard);
    wipeStatusLabel->setStyleSheet("color:#667085; font-size:12px; font-weight:600;");
    actionHeader->addWidget(wipeStatusLabel);
    actionLayout->addLayout(actionHeader);

    wipeProgressBar = new QProgressBar(actionCard);
    wipeProgressBar->setRange(0, 100);
    wipeProgressBar->setValue(0);
    wipeProgressBar->setTextVisible(true);
    wipeProgressBar->setMinimumHeight(10);
    wipeProgressBar->setStyleSheet("QProgressBar { background:#F2F4F7; border:1px solid #D0D5DD; border-radius:5px; text-align:center; color:#475467; } QProgressBar::chunk { background:#2563EB; border-radius:5px; }");
    actionLayout->addWidget(wipeProgressBar);

    wipeVerificationLabel = new QLabel(QStringLiteral("Verification: —"), actionCard);
    wipeVerificationLabel->setStyleSheet("color:#667085; font-size:11px;");
    actionLayout->addWidget(wipeVerificationLabel);

    auto *actionButtons = new QHBoxLayout;
    actionButtons->addStretch();
    wipeSafetyButton = new QPushButton(QStringLiteral("Run Safety Check"), actionCard);
    wipeSafetyButton->setMinimumSize(160, 40);
    wipeSafetyButton->setCursor(Qt::PointingHandCursor);
    wipeSafetyButton->setStyleSheet("QPushButton { background:#FFFFFF; color:#344054; border:1px solid #D0D5DD; border-radius:8px; padding:9px 14px; font-size:12px; font-weight:700; } QPushButton:hover { background:#F9FAFB; border-color:#98A2B3; } QPushButton:disabled { color:#98A2B3; background:#F2F4F7; }");
    actionButtons->addWidget(wipeSafetyButton);

    wipeStartButton = new QPushButton(QStringLiteral("Start Sanitization"), actionCard);
    wipeStartButton->setMinimumSize(180, 40);
    wipeStartButton->setCursor(Qt::PointingHandCursor);
    wipeStartButton->setEnabled(false);
    wipeStartButton->setStyleSheet("QPushButton { background:#2563EB; color:#FFFFFF; border:none; border-radius:8px; padding:9px 14px; font-size:12px; font-weight:700; } QPushButton:hover { background:#1D4ED8; } QPushButton:pressed { background:#1E40AF; } QPushButton:disabled { background:#CBD5E1; color:#64748B; }");
    actionButtons->addWidget(wipeStartButton);
    actionLayout->addLayout(actionButtons);
    root->addWidget(actionCard);

    connect(wipeRequestComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index)
    {
        selectedRequestId.clear();
        selectedRequestDeviceType.clear();
        selectedRequestMethod.clear();
        selectedWipeDeviceId.clear();
        wipeSafetyApproved = false;
        if (index >= 0)
        {
            selectedRequestId = wipeRequestComboBox->itemData(index, Qt::UserRole).toString();
            selectedRequestDeviceType = wipeRequestComboBox->itemData(index, Qt::UserRole + 1).toString();
            selectedRequestMethod = wipeRequestComboBox->itemData(index, Qt::UserRole + 2).toString();
            wipeRequestSummaryLabel->setText(QStringLiteral("Request %1  •  Device type: %2  •  Assigned method: %3").arg(selectedRequestId, selectedRequestDeviceType, selectedRequestMethod.isEmpty() ? QStringLiteral("Not specified") : selectedRequestMethod));
        }
        else
        {
            wipeRequestSummaryLabel->setText(QStringLiteral("No assigned request selected."));
        }
        wipeStatusLabel->setText(index >= 0 ? QStringLiteral("Select a matching physical target device.") : QStringLiteral("Choose a request to begin."));
        wipeStatusLabel->setStyleSheet("color:#667085; font-size:12px; font-weight:600;");
        wipeSafetyBadgeLabel->setText(QStringLiteral("NOT CHECKED"));
        wipeSafetyBadgeLabel->setStyleSheet("QLabel { background:#FFF7E8; color:#B54708; border:1px solid #FAD7A0; border-radius:8px; padding:5px 10px; font-size:11px; font-weight:700; }");
        wipeSafetyChecksLabel->setText(QStringLiteral("Safety checks have not been run."));
        wipeMethodValueLabel->setText(QStringLiteral("Select a target"));
        wipeTargetValueLabel->setText(QStringLiteral("None"));
        wipeCapacityValueLabel->setText(QStringLiteral("—"));
        wipeInterfaceValueLabel->setText(QStringLiteral("—"));
        wipeVerificationLabel->setText(QStringLiteral("Verification: —"));
        wipeProgressBar->setRange(0, 100);
        wipeProgressBar->setValue(0);
        wipeStartButton->setEnabled(false);
        wipeSafetyButton->setEnabled(false);
        deviceController->refreshDevices();
    });

    connect(wipeRefreshButton, &QPushButton::clicked, this, [this]()
    {
        selectedWipeDeviceId.clear();
        wipeSafetyApproved = false;
        wipeSafetyButton->setEnabled(false);
        wipeStartButton->setEnabled(false);
        wipeStatusLabel->setText(QStringLiteral("Refreshing physical storage devices..."));
        deviceController->refreshDevices();
    });

    connect(wipeDeviceTable, &QTableWidget::itemSelectionChanged, this, [this]()
    {
        const int row = wipeDeviceTable->currentRow();
        wipeSafetyApproved = false;
        wipeStartButton->setEnabled(false);
        wipeSafetyButton->setEnabled(row >= 0 && !sanitizationOperationRunning);
        if (row < 0)
        {
            selectedWipeDeviceId.clear();
            wipeTargetValueLabel->setText(QStringLiteral("None"));
            wipeMethodValueLabel->setText(QStringLiteral("Select a target"));
            wipeCapacityValueLabel->setText(QStringLiteral("—"));
            wipeInterfaceValueLabel->setText(QStringLiteral("—"));
            return;
        }

        bool ok = false;
        const int deviceIndex = wipeDeviceTable->item(row, 0)->data(Qt::UserRole).toInt(&ok);
        if (!ok || !deviceController->selectTarget(deviceIndex))
        {
            wipeStatusLabel->setText(QStringLiteral("Unable to select this physical device."));
            wipeSafetyButton->setEnabled(false);
            return;
        }

        const StorageDevice &device = deviceController->devices().at(deviceIndex);
        selectedWipeDeviceId = QString::fromStdString(device.getDeviceId());
        wipeTargetValueLabel->setText(QString::fromStdString(device.getModel()) + QStringLiteral("  •  ") + QString::fromStdString(device.getSerialNumber()));
        wipeCapacityValueLabel->setText(QString::number(static_cast<double>(device.getCapacityBytes()) / (1024.0 * 1024.0 * 1024.0), 'f', 1) + QStringLiteral(" GB"));
        wipeInterfaceValueLabel->setText(QString::fromStdString(device.getInterfaceType()));

        const SanitizationMethod method = deviceController->detectSelectedTargetMethod();
        wipeMethodValueLabel->setText(sanitizationMethodName(method));
        wipeVerificationLabel->setText(QStringLiteral("Verification: not run yet"));
        wipeSafetyChecksLabel->setText(QStringLiteral("Target selected. Run the safety check before authorization."));
        wipeSafetyBadgeLabel->setText(QStringLiteral("NOT CHECKED"));
        wipeSafetyBadgeLabel->setStyleSheet("QLabel { background:#FFF7E8; color:#B54708; border:1px solid #FAD7A0; border-radius:8px; padding:5px 10px; font-size:11px; font-weight:700; }");
        wipeStatusLabel->setText(method == SanitizationMethod::Unsupported
            ? QStringLiteral("Target selected, but no supported execution method was detected.")
            : QStringLiteral("Target selected. Run the safety check to unlock sanitization."));
        wipeStatusLabel->setStyleSheet(method == SanitizationMethod::Unsupported
            ? "color:#B42318; font-size:12px; font-weight:600;"
            : "color:#667085; font-size:12px; font-weight:600;");
    });

    connect(wipeSafetyButton, &QPushButton::clicked, this, &MainWindow::runWipeSafetyCheck);
    connect(wipeStartButton, &QPushButton::clicked, this, &MainWindow::startSanitization);

    wipeRefreshButton->setToolTip(QStringLiteral("Rediscover physical storage devices"));
    wipeSafetyButton->setToolTip(QStringLiteral("Re-check target identity, system disk, boot dependency and mounted volumes"));
    wipeStartButton->setToolTip(QStringLiteral("Starts the destructive sanitization operation only after safety approval"));

    wipeSafetyButton->setEnabled(false);
    wipeStartButton->setEnabled(false);
}

void MainWindow::populateWipeDevices()
{
    if (!wipeDeviceTable)
        return;

    const QString preserveId = selectedWipeDeviceId;
    wipeDeviceTable->blockSignals(true);
    wipeDeviceTable->clearContents();
    wipeDeviceTable->setRowCount(0);

    const auto &devices = deviceController->devices();
    for (int i = 0; i < static_cast<int>(devices.size()); ++i)
    {
        const StorageDevice &device = devices[static_cast<std::size_t>(i)];
        if (selectedRequestDeviceType.isEmpty() || !requestMatchesDevice(selectedRequestDeviceType, device))
            continue;

        const int row = wipeDeviceTable->rowCount();
        wipeDeviceTable->insertRow(row);
        const QStringList values = {
            QString::fromStdString(device.getModel()),
            QString::fromStdString(device.getInterfaceType()),
            QString::number(static_cast<double>(device.getCapacityBytes()) / (1024.0 * 1024.0 * 1024.0), 'f', 1) + QStringLiteral(" GB"),
            QString::fromStdString(device.getDeviceId()),
            device.isSystemDisk() ? QStringLiteral("YES") : QStringLiteral("NO"),
            device.isRemovable() ? QStringLiteral("Removable") : QStringLiteral("Internal")
        };
        for (int column = 0; column < values.size(); ++column)
        {
            auto *item = new QTableWidgetItem(values.at(column));
            if (column == 0)
                item->setData(Qt::UserRole, i);
            wipeDeviceTable->setItem(row, column, item);
        }

        if (preserveId == QString::fromStdString(device.getDeviceId()))
            wipeDeviceTable->selectRow(row);
    }

    wipeDeviceTable->blockSignals(false);

    if (wipeDeviceTable->rowCount() == 0)
    {
        wipeStatusLabel->setText(selectedRequestId.isEmpty()
            ? QStringLiteral("Choose an assigned request first.")
            : QStringLiteral("No physical device matches this request type."));
        wipeStatusLabel->setStyleSheet("color:#667085; font-size:12px; font-weight:600;");
        wipeSafetyButton->setEnabled(false);
        wipeStartButton->setEnabled(false);
        return;
    }

    if (!preserveId.isEmpty())
    {
        for (int row = 0; row < wipeDeviceTable->rowCount(); ++row)
        {
            const int deviceIndex = wipeDeviceTable->item(row, 0)->data(Qt::UserRole).toInt();
            if (deviceIndex >= 0 && deviceIndex < static_cast<int>(devices.size()) && QString::fromStdString(devices[static_cast<std::size_t>(deviceIndex)].getDeviceId()) == preserveId)
            {
                wipeDeviceTable->selectRow(row);
                break;
            }
        }
    }
}

void MainWindow::updateWipeSelectionState()
{
    const auto &target = deviceController->selectedTarget();
    if (!target.has_value())
    {
        wipeStartButton->setEnabled(false);
        return;
    }

    const SanitizationMethod method = deviceController->detectSelectedTargetMethod();
    wipeMethodValueLabel->setText(sanitizationMethodName(method));
    wipeStartButton->setEnabled(wipeSafetyApproved && method != SanitizationMethod::Unsupported && !sanitizationOperationRunning);
}

void MainWindow::runWipeSafetyCheck()
{
    if (sanitizationOperationRunning)
        return;

    if (!deviceController->selectedTarget().has_value())
    {
        wipeStatusLabel->setText(QStringLiteral("Select a physical target device first."));
        return;
    }

    wipeSafetyApproved = false;
    wipeStartButton->setEnabled(false);
    wipeSafetyButton->setEnabled(false);
    wipeSafetyBadgeLabel->setText(QStringLiteral("CHECKING..."));
    wipeSafetyBadgeLabel->setStyleSheet("QLabel { background:#EAF2FF; color:#175CD3; border:1px solid #B2CCFF; border-radius:8px; padding:5px 10px; font-size:11px; font-weight:700; }");
    wipeStatusLabel->setText(QStringLiteral("Running fresh target validation and safety checks..."));
    wipeStatusLabel->setStyleSheet("color:#175CD3; font-size:12px; font-weight:600;");
    qApp->processEvents();

    if (!deviceController->validateSelectedTarget())
    {
        wipeSafetyBadgeLabel->setText(QStringLiteral("BLOCKED"));
        wipeSafetyBadgeLabel->setStyleSheet("QLabel { background:#FEF3F2; color:#B42318; border:1px solid #FECDCA; border-radius:8px; padding:5px 10px; font-size:11px; font-weight:700; }");
        wipeSafetyChecksLabel->setText(QStringLiteral("Target validation failed. Re-select the physical device and try again."));
        wipeStatusLabel->setText(QStringLiteral("Safety check blocked sanitization."));
        wipeStatusLabel->setStyleSheet("color:#B42318; font-size:12px; font-weight:600;");
        wipeSafetyButton->setEnabled(wipeDeviceTable->currentRow() >= 0);
        return;
    }

    if (!deviceController->evaluateSelectedTarget())
    {
        const SafetyResult &result = deviceController->lastSafetyResult();
        QString html = QStringLiteral("<b>%1</b><br>").arg(QString::fromStdString(result.summary).toHtmlEscaped());
        for (const SafetyCheckResult &check : result.checks)
        {
            html += QStringLiteral("%1 %2<br>").arg(check.passed ? QStringLiteral("✓") : QStringLiteral("✗"), QString::fromStdString(check.checkName).toHtmlEscaped());
        }
        wipeSafetyChecksLabel->setText(html);
        wipeSafetyBadgeLabel->setText(QStringLiteral("BLOCKED"));
        wipeSafetyBadgeLabel->setStyleSheet("QLabel { background:#FEF3F2; color:#B42318; border:1px solid #FECDCA; border-radius:8px; padding:5px 10px; font-size:11px; font-weight:700; }");
        wipeStatusLabel->setText(QString::fromStdString(result.summary));
        wipeStatusLabel->setStyleSheet("color:#B42318; font-size:12px; font-weight:600;");
        wipeSafetyButton->setEnabled(wipeDeviceTable->currentRow() >= 0);
        return;
    }

    const SafetyResult &result = deviceController->lastSafetyResult();
    QString html = QStringLiteral("<b>All required checks passed.</b><br>");
    for (const SafetyCheckResult &check : result.checks)
    {
        html += QStringLiteral("✓ %1<br>").arg(QString::fromStdString(check.checkName).toHtmlEscaped());
    }
    wipeSafetyChecksLabel->setText(html);
    wipeSafetyBadgeLabel->setText(QStringLiteral("SAFE"));
    wipeSafetyBadgeLabel->setStyleSheet("QLabel { background:#ECFDF3; color:#027A48; border:1px solid #ABEFC6; border-radius:8px; padding:5px 10px; font-size:11px; font-weight:700; }");

    const SanitizationMethod method = deviceController->detectSelectedTargetMethod();
    wipeMethodValueLabel->setText(sanitizationMethodName(method));
    wipeSafetyApproved = true;

    if (method == SanitizationMethod::Unsupported)
    {
        wipeStatusLabel->setText(QStringLiteral("Safety passed, but SecureWipe has no supported execution method for this device."));
        wipeStatusLabel->setStyleSheet("color:#B42318; font-size:12px; font-weight:600;");
        wipeStartButton->setEnabled(false);
    }
    else
    {
        wipeStatusLabel->setText(QStringLiteral("Safety passed. The target is ready for final destructive confirmation."));
        wipeStatusLabel->setStyleSheet("color:#027A48; font-size:12px; font-weight:600;");
        updateWipeSelectionState();
    }

    wipeSafetyButton->setEnabled(true);
}

void MainWindow::startSanitization()
{
    if (sanitizationOperationRunning)
        return;

    const auto selectedTarget = deviceController->selectedTarget();
    if (!selectedTarget.has_value())
    {
        wipeStatusLabel->setText(QStringLiteral("Select a target device first."));
        return;
    }

    waitingForSanitizationStart = true;
    if (!deviceController->validateSelectedTarget() || !deviceController->evaluateSelectedTarget())
    {
        waitingForSanitizationStart = false;
        runWipeSafetyCheck();
        return;
    }

    const auto validatedTarget = deviceController->selectedTarget();
    if (!validatedTarget.has_value())
    {
        waitingForSanitizationStart = false;
        wipeStatusLabel->setText(QStringLiteral("Validated target is no longer available."));
        return;
    }

    if (!requestMatchesDevice(selectedRequestDeviceType, *validatedTarget))
    {
        waitingForSanitizationStart = false;
        wipeStatusLabel->setText(QStringLiteral("The selected device does not match the assigned request."));
        wipeStatusLabel->setStyleSheet("color:#B42318; font-size:12px; font-weight:600;");
        return;
    }

    const SanitizationMethod method = deviceController->detectSelectedTargetMethod();
    if (method == SanitizationMethod::Unsupported)
    {
        waitingForSanitizationStart = false;
        wipeStatusLabel->setText(QStringLiteral("No supported sanitization method is available for this target."));
        wipeStatusLabel->setStyleSheet("color:#B42318; font-size:12px; font-weight:600;");
        return;
    }

    const QString model = QString::fromStdString(validatedTarget->getModel());
    const QString serial = QString::fromStdString(validatedTarget->getSerialNumber());
    const QString deviceId = QString::fromStdString(validatedTarget->getDeviceId());
    const QString methodName = sanitizationMethodName(method);

    const auto confirmation = QMessageBox::warning(
        this,
        QStringLiteral("Final destructive confirmation"),
        QStringLiteral("You are about to permanently sanitize this physical device.\n\nRequest: %1\nModel: %2\nSerial: %3\nDevice: %4\nMethod: %5\n\nThis can destroy all data on the target. Continue only when the identity is correct and the device is intentionally authorized.")
            .arg(selectedRequestId, model, serial, deviceId, methodName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    waitingForSanitizationStart = false;
    if (confirmation != QMessageBox::Yes)
    {
        wipeStatusLabel->setText(QStringLiteral("Sanitization cancelled. No destructive operation was started."));
        wipeStatusLabel->setStyleSheet("color:#667085; font-size:12px; font-weight:600;");
        return;
    }

    const StorageDevice targetCopy = *validatedTarget;
    const SafetyResult safetyCopy = deviceController->lastSafetyResult();

    sanitizationOperationRunning = true;
    wipeSafetyButton->setEnabled(false);
    wipeStartButton->setEnabled(false);
    wipeRefreshButton->setEnabled(false);
    wipeRequestComboBox->setEnabled(false);
    wipeDeviceTable->setEnabled(false);
    wipeProgressBar->setRange(0, 0);
    wipeVerificationLabel->setText(QStringLiteral("Verification: waiting for sanitization result..."));
    wipeStatusLabel->setText(QStringLiteral("Sanitizing %1. Do not disconnect the device.").arg(model));
    wipeStatusLabel->setStyleSheet("color:#175CD3; font-size:12px; font-weight:700;");

    auto *watcher = new QFutureWatcher<SecureWipe::SanitizationResult>(this);
    connect(watcher, &QFutureWatcher<SecureWipe::SanitizationResult>::finished, this, [this, watcher]()
    {
        const SecureWipe::SanitizationResult result = watcher->result();
        watcher->deleteLater();
        if (QApplication::overrideCursor())
            QApplication::restoreOverrideCursor();

        sanitizationOperationRunning = false;
        wipeRefreshButton->setEnabled(true);
        wipeRequestComboBox->setEnabled(true);
        wipeDeviceTable->setEnabled(true);
        showSanitizationResult(result);

        if (!selectedRequestId.isEmpty() && authManager && !authManager->token().isEmpty())
        {
            sanitizationRequestService->updateRequestStatus(
                authManager->token(),
                selectedRequestId,
                result.status == SecureWipe::SanitizationStatus::COMPLETED ? QStringLiteral("COMPLETED") : QStringLiteral("FAILED"));
        }
    });

    QApplication::setOverrideCursor(Qt::WaitCursor);

    watcher->setFuture(QtConcurrent::run([targetCopy, safetyCopy]() -> SecureWipe::SanitizationResult
    {
        SanitizationEngine engine;
        return engine.sanitize(targetCopy, safetyCopy);
    }));
}

void MainWindow::showSanitizationResult(const SecureWipe::SanitizationResult &result)
{
    const QString method = sanitizationMethodName(result.method);
    QString status;
    switch (result.status)
    {
    case SecureWipe::SanitizationStatus::COMPLETED: status = QStringLiteral("COMPLETED"); break;
    case SecureWipe::SanitizationStatus::FAILED: status = QStringLiteral("FAILED"); break;
    case SecureWipe::SanitizationStatus::IN_PROGRESS: status = QStringLiteral("IN PROGRESS"); break;
    case SecureWipe::SanitizationStatus::ABORTED: status = QStringLiteral("ABORTED"); break;
    default: status = QStringLiteral("NOT STARTED"); break;
    }

    QString verification;
    switch (result.verificationStatus)
    {
    case SecureWipe::VerificationStatus::PASSED: verification = QStringLiteral("PASSED"); break;
    case SecureWipe::VerificationStatus::FAILED: verification = QStringLiteral("FAILED"); break;
    case SecureWipe::VerificationStatus::IN_PROGRESS: verification = QStringLiteral("IN PROGRESS"); break;
    default: verification = QStringLiteral("NOT PERFORMED"); break;
    }

    if (result.status == SecureWipe::SanitizationStatus::COMPLETED)
    {
        wipeProgressBar->setRange(0, 100);
        wipeProgressBar->setValue(100);
        wipeSafetyBadgeLabel->setText(QStringLiteral("COMPLETED"));
        wipeSafetyBadgeLabel->setStyleSheet("QLabel { background:#ECFDF3; color:#027A48; border:1px solid #ABEFC6; border-radius:8px; padding:5px 10px; font-size:11px; font-weight:700; }");
        wipeStatusLabel->setText(QStringLiteral("Sanitization completed. Verification: %1.").arg(verification));
        wipeStatusLabel->setStyleSheet("color:#027A48; font-size:12px; font-weight:700;");
    }
    else
    {
        wipeProgressBar->setRange(0, 100);
        wipeProgressBar->setValue(0);
        wipeSafetyBadgeLabel->setText(status);
        wipeSafetyBadgeLabel->setStyleSheet("QLabel { background:#FEF3F2; color:#B42318; border:1px solid #FECDCA; border-radius:8px; padding:5px 10px; font-size:11px; font-weight:700; }");
        wipeStatusLabel->setText(QStringLiteral("Sanitization %1.").arg(status.toLower()));
        wipeStatusLabel->setStyleSheet("color:#B42318; font-size:12px; font-weight:700;");
    }

    wipeTargetValueLabel->setText(QString::fromStdString(result.model) + QStringLiteral("  •  ") + QString::fromStdString(result.serialNumber));
    wipeMethodValueLabel->setText(method);
    wipeCapacityValueLabel->setText(QString::number(static_cast<double>(result.capacityBytes) / (1024.0 * 1024.0 * 1024.0), 'f', 1) + QStringLiteral(" GB"));
    wipeInterfaceValueLabel->setText(QString::fromStdString(result.interfaceType));
    wipeVerificationLabel->setText(QStringLiteral("Verification: %1  •  Bytes verified: %2  •  Samples: %3")
        .arg(verification, QString::number(result.bytesVerified), QString::number(result.verificationSamples)));

    QString html = QStringLiteral("<b>%1</b><br>").arg(QStringLiteral("Operation result: %1").arg(status).toHtmlEscaped());
    if (!result.message.empty())
        html += QStringLiteral("%1<br>").arg(QString::fromStdString(result.message).toHtmlEscaped());
    if (!result.errorMessage.empty())
        html += QStringLiteral("<span style='color:#B42318;'>%1</span><br>").arg(QString::fromStdString(result.errorMessage).toHtmlEscaped());
    html += QStringLiteral("Operation ID: %1<br>Bytes processed: %2<br>Duration: %3 ms")
        .arg(QString::fromStdString(result.operationId).toHtmlEscaped(), QString::number(result.bytesProcessed), QString::number(result.operationDurationMs));
    if (!result.verificationMessage.empty())
        html += QStringLiteral("<br>Verification detail: %1").arg(QString::fromStdString(result.verificationMessage).toHtmlEscaped());
    wipeSafetyChecksLabel->setText(html);

    wipeSafetyApproved = false;
    wipeSafetyButton->setEnabled(wipeDeviceTable->currentRow() >= 0);
    wipeStartButton->setEnabled(false);
}

QString MainWindow::sanitizationMethodName(SanitizationMethod method) const
{
    switch (method)
    {
    case SanitizationMethod::NvmeSanitize: return QStringLiteral("NVMe Sanitize");
    case SanitizationMethod::AtaSanitize: return QStringLiteral("ATA Sanitize");
    case SanitizationMethod::HostOverwrite: return QStringLiteral("Host Overwrite");
    case SanitizationMethod::Unsupported: default: return QStringLiteral("Unsupported");
    }
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

    selectedWipeDeviceId.clear();
    wipeSafetyApproved = false;
    if (wipeSafetyButton)
        wipeSafetyButton->setEnabled(false);
    if (wipeStartButton)
        wipeStartButton->setEnabled(false);

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