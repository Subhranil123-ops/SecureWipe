#include "mainwindow.h"

#include "AuthManager.h"
#include "controllers/DeviceController.h"
#include "models/DeviceTableModel.h"
#include "services/SanitizationRequestService.h"
#include "services/SanitizationResultService.h"

#include "../../backend/classification/include/ClassificationResult.h"
#include "../../backend/classification/include/DeviceClassifier.h"
#include "../../backend/safety/include/SafetyResult.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QFutureWatcher>
#include <QMetaType>

#include <QtConcurrent/QtConcurrentRun>

namespace
{

QFrame *makeCard(QWidget *parent)
{
    auto *card = new QFrame(parent);

    card->setStyleSheet(
        "QFrame {"
        "background:#FFFFFF;"
        "border:1px solid #E4E7EC;"
        "border-radius:14px;"
        "}");

    return card;
}

QLabel *makeTitle(
    const QString &text,
    QWidget *parent)
{
    auto *label = new QLabel(text, parent);

    label->setStyleSheet(
        "QLabel {"
        "color:#101828;"
        "font-size:25px;"
        "font-weight:700;"
        "}");

    return label;
}

QLabel *makeSubtitle(
    const QString &text,
    QWidget *parent)
{
    auto *label = new QLabel(text, parent);

    label->setWordWrap(true);

    label->setStyleSheet(
        "QLabel {"
        "color:#667085;"
        "font-size:13px;"
        "}");

    return label;
}

QLabel *makeCaption(
    const QString &text,
    QWidget *parent)
{
    auto *label = new QLabel(text, parent);

    label->setStyleSheet(
        "QLabel {"
        "color:#667085;"
        "font-size:11px;"
        "font-weight:600;"
        "}");

    return label;
}

QLabel *makeValue(
    const QString &text,
    QWidget *parent)
{
    auto *label = new QLabel(text, parent);

    label->setWordWrap(true);
    label->setTextInteractionFlags(
        Qt::TextSelectableByMouse);

    label->setStyleSheet(
        "QLabel {"
        "color:#172033;"
        "font-size:13px;"
        "font-weight:600;"
        "}");

    return label;
}

QPushButton *makePrimaryButton(
    const QString &text,
    QWidget *parent)
{
    auto *button =
        new QPushButton(text, parent);

    button->setCursor(
        Qt::PointingHandCursor);

    button->setMinimumHeight(40);

    button->setStyleSheet(
        "QPushButton {"
        "background:#2563EB;"
        "color:#FFFFFF;"
        "border:none;"
        "border-radius:9px;"
        "padding:9px 16px;"
        "font-size:12px;"
        "font-weight:700;"
        "}"
        "QPushButton:hover {background:#1D4ED8;}"
        "QPushButton:pressed {background:#1E40AF;}"
        "QPushButton:disabled {"
        "background:#CBD5E1;"
        "color:#64748B;"
        "}");

    return button;
}

QPushButton *makeSecondaryButton(
    const QString &text,
    QWidget *parent)
{
    auto *button =
        new QPushButton(text, parent);

    button->setCursor(
        Qt::PointingHandCursor);

    button->setMinimumHeight(38);

    button->setStyleSheet(
        "QPushButton {"
        "background:#FFFFFF;"
        "color:#344054;"
        "border:1px solid #D0D5DD;"
        "border-radius:9px;"
        "padding:8px 14px;"
        "font-size:12px;"
        "font-weight:600;"
        "}"
        "QPushButton:hover {"
        "background:#F9FAFB;"
        "border-color:#98A2B3;"
        "}"
        "QPushButton:disabled {"
        "background:#F2F4F7;"
        "color:#98A2B3;"
        "}");

    return button;
}

QString badgeStyle(
    const QString &status)
{
    const QString value =
        status.trimmed().toUpper();

    if (value == "SAFE" ||
        value == "COMPLETED" ||
        value == "PASSED")
    {
        return
            "QLabel {"
            "background:#ECFDF3;"
            "color:#027A48;"
            "border:1px solid #ABEFC6;"
            "border-radius:9px;"
            "padding:5px 10px;"
            "font-size:11px;"
            "font-weight:700;"
            "}";
    }

    if (value == "FAILED" ||
        value == "BLOCKED")
    {
        return
            "QLabel {"
            "background:#FEF3F2;"
            "color:#B42318;"
            "border:1px solid #FECDCA;"
            "border-radius:9px;"
            "padding:5px 10px;"
            "font-size:11px;"
            "font-weight:700;"
            "}";
    }

    return
        "QLabel {"
        "background:#EFF6FF;"
        "color:#175CD3;"
        "border:1px solid #B2CCFF;"
        "border-radius:9px;"
        "padding:5px 10px;"
        "font-size:11px;"
        "font-weight:700;"
        "}";
}

bool requestMatchesDeviceImpl(
    const QString &requestedType,
    const StorageDevice &device)
{
    DeviceClassifier classifier;

    const ClassificationResult classification =
        classifier.classify(device);

    const QString type =
        requestedType.trimmed();

    if (type.compare(
            "SSD",
            Qt::CaseInsensitive) == 0)
    {
        return classification.mediaType ==
               MediaType::SSD;
    }

    if (type.compare(
            "HDD",
            Qt::CaseInsensitive) == 0)
    {
        return classification.mediaType ==
               MediaType::HDD;
    }

    if (type.compare(
            "USB Drive",
            Qt::CaseInsensitive) == 0)
    {
        return classification.busType ==
               BusType::USB;
    }

    if (type.compare(
            "NVMe SSD",
            Qt::CaseInsensitive) == 0)
    {
        return classification.busType ==
                   BusType::NVMe &&
               classification.mediaType ==
                   MediaType::SSD;
    }

    return false;
}

}

MainWindow::MainWindow(
    QWidget *parent)
    : QMainWindow(parent)
    , authManager_(new AuthManager(this))
    , requestService_(
          new SanitizationRequestService(this))
    , resultService_(
          new SanitizationResultService(this))
    , deviceController_(
          new DeviceController(this))
    , deviceTableModel_(
          new DeviceTableModel(this))
    , deviceDetailsPage_(nullptr)
    , forensicPage_(nullptr)
    , root_(nullptr)
    , rootStack_(nullptr)
    , loginPage_(nullptr)
    , appPage_(nullptr)
    , contentStack_(nullptr)
    , emailEdit_(nullptr)
    , passwordEdit_(nullptr)
    , loginErrorLabel_(nullptr)
    , loginButton_(nullptr)
    , operatorNameLabel_(nullptr)
    , operatorRoleLabel_(nullptr)
    , connectionBadgeLabel_(nullptr)
    , dashboardNavButton_(nullptr)
    , devicesNavButton_(nullptr)
    , jobsNavButton_(nullptr)
    , forensicsNavButton_(nullptr)
    , settingsNavButton_(nullptr)
    , logoutButton_(nullptr)
    , dashboardPage_(nullptr)
    , devicesPage_(nullptr)
    , jobsPage_(nullptr)
    , forensicsPage_(nullptr)
    , settingsPage_(nullptr)
    , dashboardJobsTable_(nullptr)
    , assignedJobsTable_(nullptr)
    , deviceTable_(nullptr)
    , totalJobsValue_(nullptr)
    , activeJobsValue_(nullptr)
    , completedJobsValue_(nullptr)
    , failedJobsValue_(nullptr)
    , jobComboBox_(nullptr)
    , jobRequestIdValue_(nullptr)
    , jobDeviceTypeValue_(nullptr)
    , jobRequestedMethodValue_(nullptr)
    , jobAssetValue_(nullptr)
    , jobWorkstationValue_(nullptr)
    , targetModelValue_(nullptr)
    , targetSerialValue_(nullptr)
    , targetCapacityValue_(nullptr)
    , targetInterfaceValue_(nullptr)
    , targetPathValue_(nullptr)
    , targetSafetyBadge_(nullptr)
    , targetSafetyText_(nullptr)
    , capabilityValue_(nullptr)
    , selectedMethodValue_(nullptr)
    , pipelineStatusValue_(nullptr)
    , verificationValue_(nullptr)
    , operationValue_(nullptr)
    , bytesProcessedValue_(nullptr)
    , bytesVerifiedValue_(nullptr)
    , samplesValue_(nullptr)
    , certificateValue_(nullptr)
    , evidenceValue_(nullptr)
    , jobMessageLabel_(nullptr)
    , operationProgress_(nullptr)
    , refreshJobsButton_(nullptr)
    , refreshDevicesButton_(nullptr)
    , validateTargetButton_(nullptr)
    , startSanitizationButton_(nullptr)
{
    setWindowTitle(
        QStringLiteral("SecureWipe"));

    resize(1420, 900);
    setMinimumSize(1180, 760);

    buildUi();
    applyTheme();

    connect(
        authManager_,
        &AuthManager::loginSuccessful,
        this,
        [this]()
        {
            loginButton_->setEnabled(true);
            loginButton_->setText(
                QStringLiteral("Sign in"));

            rootStack_->setCurrentWidget(
                appPage_);

            contentStack_->setCurrentWidget(
                dashboardPage_);

            setActiveNav(
                dashboardNavButton_);

            setConnectionState(
                true,
                QStringLiteral("Connected"));

            refreshAssignedRequests();
            refreshPhysicalDevices();
        });

    connect(
        authManager_,
        &AuthManager::loginFailed,
        this,
        [this](const QString &message)
        {
            loginButton_->setEnabled(true);
            loginButton_->setText(
                QStringLiteral("Sign in"));

            loginErrorLabel_->setText(
                message);

            setConnectionState(
                false,
                QStringLiteral("Authentication failed"));
        });

    connect(
        loginButton_,
        &QPushButton::clicked,
        this,
        [this]()
        {
            loginErrorLabel_->clear();

            const QString email =
                emailEdit_->text().trimmed();

            const QString password =
                passwordEdit_->text();

            if (email.isEmpty())
            {
                loginErrorLabel_->setText(
                    QStringLiteral(
                        "Email is required."));
                return;
            }

            const QRegularExpression pattern(
                QStringLiteral(
                    R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)"));

            if (!pattern.match(email).hasMatch())
            {
                loginErrorLabel_->setText(
                    QStringLiteral(
                        "Please enter a valid email address."));
                return;
            }

            if (password.isEmpty())
            {
                loginErrorLabel_->setText(
                    QStringLiteral(
                        "Password is required."));
                return;
            }

            loginButton_->setEnabled(false);
            loginButton_->setText(
                QStringLiteral("Signing in..."));

            authManager_->login(
                email,
                password);
        });

    connect(
        requestService_,
        &SanitizationRequestService::assignedRequestsFetched,
        this,
        &MainWindow::handleAssignedRequests);

    connect(
        requestService_,
        &SanitizationRequestService::requestFetchFailed,
        this,
        [this](const QString &message)
        {
            jobMessageLabel_->setText(
                QStringLiteral(
                    "Unable to load assigned jobs: %1")
                    .arg(message));

            jobMessageLabel_->setStyleSheet(
                "color:#B42318;"
                "font-size:12px;"
                "font-weight:600;");

            setConnectionState(
                false,
                QStringLiteral("API unavailable"));
        });

    connect(
        requestService_,
        &SanitizationRequestService::requestStatusUpdated,
        this,
        [this](
            const QString &requestId,
            const QString &status)
        {
            if (requestId == selectedRequestId_)
            {
                jobMessageLabel_->setText(
                    QStringLiteral(
                        "Request %1 moved to %2.")
                        .arg(
                            requestId,
                            status));

                jobMessageLabel_->setStyleSheet(
                    "color:#027A48;"
                    "font-size:12px;"
                    "font-weight:600;");
            }

            refreshAssignedRequests();
        });

    connect(
        requestService_,
        &SanitizationRequestService::requestStatusUpdateFailed,
        this,
        [this](const QString &message)
        {
            jobMessageLabel_->setText(
                QStringLiteral(
                    "Request status update failed: %1")
                    .arg(message));

            jobMessageLabel_->setStyleSheet(
                "color:#B42318;"
                "font-size:12px;"
                "font-weight:600;");
        });

    connect(
        resultService_,
        &SanitizationResultService::resultSubmitted,
        this,
        [this](const QString &requestId)
        {
            if (requestId != selectedRequestId_)
                return;

            pipelineStatusValue_->setText(
                QStringLiteral("VERIFYING"));

            pipelineStatusValue_->setStyleSheet(
                badgeStyle("VERIFYING"));

            jobMessageLabel_->setText(
                QStringLiteral(
                    "Sanitization result uploaded. Synchronizing certificate..."));
        });

    connect(
        resultService_,
        &SanitizationResultService::resultSubmissionFailed,
        this,
        [this](const QString &message)
        {
            jobMessageLabel_->setText(
                QStringLiteral(
                    "Result upload failed: %1")
                    .arg(message));

            jobMessageLabel_->setStyleSheet(
                "color:#B42318;"
                "font-size:12px;"
                "font-weight:600;");
        });

    connect(
        resultService_,
        &SanitizationResultService::certificateSubmitted,
        this,
        [this](
            const QString &requestId,
            const QString &certificateId)
        {
            if (requestId != selectedRequestId_)
                return;

            certificateValue_->setText(
                certificateId.isEmpty()
                    ? QStringLiteral("Generated")
                    : certificateId);

            pipelineStatusValue_->setText(
                QStringLiteral("COMPLETED"));

            pipelineStatusValue_->setStyleSheet(
                badgeStyle("COMPLETED"));

            jobMessageLabel_->setText(
                QStringLiteral(
                    "Certificate synchronized successfully. Sanitization request completed."));

            jobMessageLabel_->setStyleSheet(
                "color:#027A48;"
                "font-size:12px;"
                "font-weight:700;");

            refreshAssignedRequests();
        });

    connect(
        resultService_,
        &SanitizationResultService::certificateSubmissionFailed,
        this,
        [this](const QString &message)
        {
            jobMessageLabel_->setText(
                QStringLiteral(
                    "Certificate upload failed: %1")
                    .arg(message));

            jobMessageLabel_->setStyleSheet(
                "color:#B42318;"
                "font-size:12px;"
                "font-weight:600;");
        });

    connect(
        deviceController_,
        &DeviceController::devicesUpdated,
        this,
        [this]()
        {
            deviceTableModel_->setDevices(
                deviceController_->devices());

            populateDeviceTable();
        });

    connect(
        deviceController_,
        &DeviceController::discoveryFailed,
        this,
        [this](const QString &message)
        {
            jobMessageLabel_->setText(
                QStringLiteral(
                    "Device discovery failed: %1")
                    .arg(message));

            jobMessageLabel_->setStyleSheet(
                "color:#B42318;"
                "font-size:12px;"
                "font-weight:600;");
        });

    connect(
        jobComboBox_,
        QOverload<int>::of(
            &QComboBox::currentIndexChanged),
        this,
        &MainWindow::selectRequestFromJobs);

    connect(
        assignedJobsTable_,
        &QTableWidget::itemSelectionChanged,
        this,
        [this]()
        {
            const int row =
                assignedJobsTable_->currentRow();

            if (row < 0)
                return;

            const QTableWidgetItem *item =
                assignedJobsTable_->item(
                    row,
                    0);

            if (!item)
                return;

            const QString requestId =
                item->text();

            const int index =
                jobComboBox_->findData(
                    requestId,
                    Qt::UserRole);

            if (index >= 0)
                jobComboBox_->setCurrentIndex(index);
        });

    connect(
        deviceTable_,
        &QTableWidget::itemSelectionChanged,
        this,
        [this]()
        {
            selectTargetDevice(
                deviceTable_->currentRow());
        });

    connect(
        refreshJobsButton_,
        &QPushButton::clicked,
        this,
        &MainWindow::refreshAssignedRequests);

    connect(
        refreshDevicesButton_,
        &QPushButton::clicked,
        this,
        &MainWindow::refreshPhysicalDevices);

    connect(
        validateTargetButton_,
        &QPushButton::clicked,
        this,
        &MainWindow::runTargetSafetyCheck);

    connect(
        startSanitizationButton_,
        &QPushButton::clicked,
        this,
        &MainWindow::startSanitization);

    connect(
        dashboardNavButton_,
        &QPushButton::clicked,
        this,
        [this]()
        {
            showPage(
                dashboardPage_,
                dashboardNavButton_);
        });

    connect(
        jobsNavButton_,
        &QPushButton::clicked,
        this,
        [this]()
        {
            showPage(
                jobsPage_,
                jobsNavButton_);

            refreshAssignedRequests();
        });

    connect(
        devicesNavButton_,
        &QPushButton::clicked,
        this,
        [this]()
        {
            showPage(
                devicesPage_,
                devicesNavButton_);

            refreshPhysicalDevices();
        });

    connect(
        forensicsNavButton_,
        &QPushButton::clicked,
        this,
        [this]()
        {
            showPage(
                forensicsPage_,
                forensicsNavButton_);
        });

    connect(
        settingsNavButton_,
        &QPushButton::clicked,
        this,
        [this]()
        {
            showPage(
                settingsPage_,
                settingsNavButton_);
        });

    connect(
        logoutButton_,
        &QPushButton::clicked,
        this,
        &MainWindow::logout);

    resetTargetPanel();

    rootStack_->setCurrentWidget(
        loginPage_);
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    root_ = new QWidget(this);

    rootStack_ =
        new QStackedWidget(root_);

    buildLoginPage();
    buildAppShell();

    rootStack_->addWidget(
        loginPage_);

    rootStack_->addWidget(
        appPage_);

    auto *layout =
        new QVBoxLayout(root_);

    layout->setContentsMargins(
        0, 0, 0, 0);

    layout->addWidget(
        rootStack_);

    setCentralWidget(
        root_);
}

void MainWindow::buildLoginPage()
{
    loginPage_ =
        new QWidget;

    auto *outer =
        new QVBoxLayout(
            loginPage_);

    outer->setContentsMargins(
        40, 40, 40, 40);

    outer->addStretch();

    auto *card =
        makeCard(loginPage_);

    card->setMaximumWidth(
        470);

    auto *layout =
        new QVBoxLayout(card);

    layout->setContentsMargins(
        34, 34, 34, 34);

    layout->setSpacing(
        13);

    auto *logo =
        new QLabel(
            QStringLiteral("◈"),
            card);

    logo->setAlignment(
        Qt::AlignCenter);

    logo->setFixedSize(
        58,
        58);

    logo->setStyleSheet(
        "QLabel {"
        "background:#EFF6FF;"
        "color:#2563EB;"
        "border:1px solid #BFDBFE;"
        "border-radius:16px;"
        "font-size:28px;"
        "font-weight:700;"
        "}");

    layout->addWidget(
        logo,
        0,
        Qt::AlignHCenter);

    auto *brand =
        new QLabel(
            QStringLiteral("SecureWipe"),
            card);

    brand->setAlignment(
        Qt::AlignCenter);

    brand->setStyleSheet(
        "color:#101828;"
        "font-size:28px;"
        "font-weight:750;");

    layout->addWidget(
        brand);

    auto *subtitle =
        makeSubtitle(
            QStringLiteral(
                "Authorized workstation console for physical storage sanitization"),
            card);

    subtitle->setAlignment(
        Qt::AlignCenter);

    layout->addWidget(
        subtitle);

    layout->addSpacing(
        10);

    layout->addWidget(
        makeCaption(
            QStringLiteral("Work email"),
            card));

    emailEdit_ =
        new QLineEdit(card);

    emailEdit_->setPlaceholderText(
        QStringLiteral(
            "employee@example.com"));

    emailEdit_->setMinimumHeight(
        44);

    layout->addWidget(
        emailEdit_);

    layout->addWidget(
        makeCaption(
            QStringLiteral("Password"),
            card));

    passwordEdit_ =
        new QLineEdit(card);

    passwordEdit_->setEchoMode(
        QLineEdit::Password);

    passwordEdit_->setMinimumHeight(
        44);

    layout->addWidget(
        passwordEdit_);

    loginErrorLabel_ =
        new QLabel(card);

    loginErrorLabel_->setWordWrap(
        true);

    loginErrorLabel_->setStyleSheet(
        "color:#B42318;"
        "font-size:12px;"
        "font-weight:600;");

    layout->addWidget(
        loginErrorLabel_);

    loginButton_ =
        makePrimaryButton(
            QStringLiteral("Sign in"),
            card);

    loginButton_->setMinimumHeight(
        46);

    layout->addWidget(
        loginButton_);

    auto *notice =
        new QLabel(
            QStringLiteral(
                "Only authorized workstation roles can access this console."),
            card);

    notice->setAlignment(
        Qt::AlignCenter);

    notice->setStyleSheet(
        "color:#98A2B3;"
        "font-size:10px;");

    notice->setWordWrap(
        true);

    layout->addWidget(
        notice);

    outer->addWidget(
        card,
        0,
        Qt::AlignHCenter);

    outer->addStretch();
}

void MainWindow::buildAppShell()
{
    appPage_ =
        new QWidget;

    auto *mainLayout =
        new QHBoxLayout(
            appPage_);

    mainLayout->setContentsMargins(
        0, 0, 0, 0);

    mainLayout->setSpacing(
        0);

    auto *sidebar =
        new QFrame(appPage_);

    sidebar->setFixedWidth(
        238);

    sidebar->setStyleSheet(
        "QFrame {"
        "background:#FFFFFF;"
        "border-right:1px solid #EAECF0;"
        "}");

    auto *sidebarLayout =
        new QVBoxLayout(sidebar);

    sidebarLayout->setContentsMargins(
        18, 20, 18, 18);

    sidebarLayout->setSpacing(
        6);

    auto *brand =
        new QLabel(
            QStringLiteral("◈  SecureWipe"),
            sidebar);

    brand->setStyleSheet(
        "color:#101828;"
        "font-size:18px;"
        "font-weight:750;"
        "padding-bottom:18px;");

    sidebarLayout->addWidget(
        brand);

    dashboardNavButton_ =
        new QPushButton(
            QStringLiteral("Overview"),
            sidebar);

    jobsNavButton_ =
        new QPushButton(
            QStringLiteral("Assigned Jobs"),
            sidebar);

    devicesNavButton_ =
        new QPushButton(
            QStringLiteral("Devices"),
            sidebar);

    forensicsNavButton_ =
        new QPushButton(
            QStringLiteral("Evidence & Forensics"),
            sidebar);

    settingsNavButton_ =
        new QPushButton(
            QStringLiteral("Settings"),
            sidebar);

    sidebarLayout->addWidget(
        dashboardNavButton_);

    sidebarLayout->addWidget(
        jobsNavButton_);

    sidebarLayout->addWidget(
        devicesNavButton_);

    sidebarLayout->addWidget(
        forensicsNavButton_);

    sidebarLayout->addWidget(
        settingsNavButton_);

    sidebarLayout->addStretch();

    auto *operatorCard =
        makeCard(sidebar);

    auto *operatorLayout =
        new QVBoxLayout(
            operatorCard);

    operatorLayout->setContentsMargins(
        12, 12, 12, 12);

    operatorNameLabel_ =
        new QLabel(
            QStringLiteral("Operator"),
            operatorCard);

    operatorNameLabel_->setStyleSheet(
        "color:#172033;"
        "font-size:12px;"
        "font-weight:700;");

    operatorRoleLabel_ =
        new QLabel(
            QStringLiteral(
                "WORKSTATION EMPLOYEE"),
            operatorCard);

    operatorRoleLabel_->setStyleSheet(
        "color:#667085;"
        "font-size:10px;");

    operatorLayout->addWidget(
        operatorNameLabel_);

    operatorLayout->addWidget(
        operatorRoleLabel_);

    sidebarLayout->addWidget(
        operatorCard);

    sidebarLayout->addSpacing(
        8);

    logoutButton_ =
        new QPushButton(
            QStringLiteral("Sign out"),
            sidebar);

    logoutButton_->setMinimumHeight(
        38);

    logoutButton_->setStyleSheet(
        "QPushButton {"
        "background:#FFF5F5;"
        "color:#B42318;"
        "border:1px solid #FECACA;"
        "border-radius:9px;"
        "font-size:12px;"
        "font-weight:600;"
        "}"
        "QPushButton:hover {"
        "background:#FEF2F2;"
        "}");

    sidebarLayout->addWidget(
        logoutButton_);

    mainLayout->addWidget(
        sidebar);

    auto *right =
        new QWidget(appPage_);

    auto *rightLayout =
        new QVBoxLayout(right);

    rightLayout->setContentsMargins(
        0, 0, 0, 0);

    rightLayout->setSpacing(
        0);

    auto *topbar =
        new QFrame(right);

    topbar->setFixedHeight(
        68);

    topbar->setStyleSheet(
        "QFrame {"
        "background:#FFFFFF;"
        "border-bottom:1px solid #EAECF0;"
        "}");

    auto *topLayout =
        new QHBoxLayout(topbar);

    topLayout->setContentsMargins(
        24, 0, 24, 0);

    auto *consoleLabel =
        new QLabel(
            QStringLiteral("Workstation Console"),
            topbar);

    consoleLabel->setStyleSheet(
        "color:#667085;"
        "font-size:12px;");

    topLayout->addWidget(
        consoleLabel);

    topLayout->addStretch();

    connectionBadgeLabel_ =
        new QLabel(
            QStringLiteral("Disconnected"),
            topbar);

    connectionBadgeLabel_->setAlignment(
        Qt::AlignCenter);

    connectionBadgeLabel_->setMinimumWidth(
        110);

    topLayout->addWidget(
        connectionBadgeLabel_);

    rightLayout->addWidget(
        topbar);

    contentStack_ =
        new QStackedWidget(right);

    rightLayout->addWidget(
        contentStack_);

    buildDashboardPage();
    buildJobsPage();
    buildDevicesPage();
    buildForensicsPage();
    buildSettingsPage();

    contentStack_->addWidget(
        dashboardPage_);

    contentStack_->addWidget(
        jobsPage_);

    contentStack_->addWidget(
        devicesPage_);

    contentStack_->addWidget(
        forensicsPage_);

    contentStack_->addWidget(
        settingsPage_);

    mainLayout->addWidget(
        right,
        1);
}

void MainWindow::buildDashboardPage()
{
    dashboardPage_ =
        new QWidget;

    auto *scroll =
        new QScrollArea(
            dashboardPage_);

    scroll->setWidgetResizable(
        true);

    scroll->setFrameShape(
        QFrame::NoFrame);

    auto *page =
        new QWidget;

    auto *root =
        new QVBoxLayout(page);

    root->setContentsMargins(
        28, 26, 28, 28);

    root->setSpacing(
        16);

    root->addWidget(
        makeTitle(
            QStringLiteral("Overview"),
            page));

    root->addWidget(
        makeSubtitle(
            QStringLiteral(
                "Monitor assigned jobs, physical target readiness and evidence."),
            page));

    auto *metrics =
        new QHBoxLayout;

    metrics->setSpacing(
        12);

    auto createMetric =
        [&](const QString &caption,
            QLabel **value)
    {
        auto *card =
            makeCard(page);

        auto *layout =
            new QVBoxLayout(card);

        layout->setContentsMargins(
            16, 14, 16, 14);

        layout->addWidget(
            makeCaption(
                caption,
                card));

        *value =
            new QLabel(
                QStringLiteral("0"),
                card);

        (*value)->setStyleSheet(
            "color:#101828;"
            "font-size:25px;"
            "font-weight:750;");

        layout->addWidget(
            *value);

        metrics->addWidget(
            card);
    };

    createMetric(
        QStringLiteral("Total jobs"),
        &totalJobsValue_);

    createMetric(
        QStringLiteral("Active"),
        &activeJobsValue_);

    createMetric(
        QStringLiteral("Completed"),
        &completedJobsValue_);

    createMetric(
        QStringLiteral("Failed"),
        &failedJobsValue_);

    root->addLayout(
        metrics);

    auto *infoCard =
        makeCard(page);

    auto *infoLayout =
        new QVBoxLayout(
            infoCard);

    infoLayout->setContentsMargins(
        18, 18, 18, 18);

    infoLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "SecureWipe execution policy"),
            infoCard));

    auto *info =
        new QLabel(
            QStringLiteral(
                "The operator follows the assigned work order. "
                "The exact physical target is validated before sanitization, "
                "and the backend sanitization pipeline produces the operation, verification, certificate and audit evidence."),
            infoCard);

    info->setWordWrap(
        true);

    info->setStyleSheet(
        "color:#667085;"
        "font-size:12px;");

    infoLayout->addWidget(
        info);

    root->addWidget(
        infoCard);

    auto *jobsCard =
        makeCard(page);

    auto *jobsLayout =
        new QVBoxLayout(
            jobsCard);

    jobsLayout->setContentsMargins(
        16, 16, 16, 16);

    jobsLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Recent assigned jobs"),
            jobsCard));

    dashboardJobsTable_ =
        new QTableWidget(jobsCard);

    dashboardJobsTable_->setColumnCount(
        4);

    dashboardJobsTable_->setHorizontalHeaderLabels(
        {
            QStringLiteral("Request"),
            QStringLiteral("Device"),
            QStringLiteral("Method"),
            QStringLiteral("Status")
        });

    dashboardJobsTable_->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    dashboardJobsTable_->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    dashboardJobsTable_->setShowGrid(
        false);

    dashboardJobsTable_->verticalHeader()
        ->setVisible(false);

    dashboardJobsTable_->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::Stretch);

    dashboardJobsTable_->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::Stretch);

    dashboardJobsTable_->horizontalHeader()
        ->setSectionResizeMode(
            2,
            QHeaderView::ResizeToContents);

    dashboardJobsTable_->horizontalHeader()
        ->setSectionResizeMode(
            3,
            QHeaderView::ResizeToContents);

    jobsLayout->addWidget(
        dashboardJobsTable_);

    root->addWidget(
        jobsCard,
        1);

    scroll->setWidget(
        page);

    auto *outer =
        new QVBoxLayout(
            dashboardPage_);

    outer->setContentsMargins(
        0, 0, 0, 0);

    outer->addWidget(
        scroll);
}

void MainWindow::buildJobsPage()
{
    jobsPage_ =
        new QWidget;

    auto *scroll =
        new QScrollArea(
            jobsPage_);

    scroll->setWidgetResizable(
        true);

    scroll->setFrameShape(
        QFrame::NoFrame);

    auto *page =
        new QWidget;

    auto *root =
        new QVBoxLayout(page);

    root->setContentsMargins(
        28, 26, 28, 28);

    root->setSpacing(
        16);

    auto *header =
        new QHBoxLayout;

    auto *block =
        new QVBoxLayout;

    block->addWidget(
        makeTitle(
            QStringLiteral(
                "Assigned Sanitization Jobs"),
            page));

    block->addWidget(
        makeSubtitle(
            QStringLiteral(
                "Requests authorized for this workstation employee."),
            page));

    header->addLayout(
        block);

    header->addStretch();

    refreshJobsButton_ =
        makeSecondaryButton(
            QStringLiteral("Refresh jobs"),
            page);

    header->addWidget(
        refreshJobsButton_);

    root->addLayout(
        header);

    auto *tableCard =
        makeCard(page);

    auto *tableLayout =
        new QVBoxLayout(
            tableCard);

    tableLayout->setContentsMargins(
        16, 16, 16, 16);

    assignedJobsTable_ =
        new QTableWidget(tableCard);

    assignedJobsTable_->setColumnCount(
        5);

    assignedJobsTable_->setHorizontalHeaderLabels(
        {
            QStringLiteral("Request ID"),
            QStringLiteral("Device Type"),
            QStringLiteral("Method"),
            QStringLiteral("Asset"),
            QStringLiteral("Status")
        });

    assignedJobsTable_->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    assignedJobsTable_->setSelectionMode(
        QAbstractItemView::SingleSelection);

    assignedJobsTable_->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    assignedJobsTable_->setShowGrid(
        false);

    assignedJobsTable_->verticalHeader()
        ->setVisible(false);

    assignedJobsTable_->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::Stretch);

    assignedJobsTable_->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::ResizeToContents);

    assignedJobsTable_->horizontalHeader()
        ->setSectionResizeMode(
            2,
            QHeaderView::ResizeToContents);

    assignedJobsTable_->horizontalHeader()
        ->setSectionResizeMode(
            3,
            QHeaderView::Stretch);

    assignedJobsTable_->horizontalHeader()
        ->setSectionResizeMode(
            4,
            QHeaderView::ResizeToContents);

    tableLayout->addWidget(
        assignedJobsTable_);

    root->addWidget(
        tableCard);

    auto *detailCard =
        makeCard(page);

    auto *detailLayout =
        new QVBoxLayout(
            detailCard);

    detailLayout->setContentsMargins(
        18, 18, 18, 18);

    detailLayout->setSpacing(
        10);

    detailLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Selected work order"),
            detailCard));

    jobComboBox_ =
        new QComboBox(detailCard);

    jobComboBox_->setMinimumHeight(
        42);

    detailLayout->addWidget(
        jobComboBox_);

    auto *grid =
        new QGridLayout;

    grid->setHorizontalSpacing(
        24);

    grid->setVerticalSpacing(
        12);

    jobRequestIdValue_ =
        makeValue(
            QStringLiteral("—"),
            detailCard);

    jobDeviceTypeValue_ =
        makeValue(
            QStringLiteral("—"),
            detailCard);

    jobRequestedMethodValue_ =
        makeValue(
            QStringLiteral("—"),
            detailCard);

    jobAssetValue_ =
        makeValue(
            QStringLiteral("—"),
            detailCard);

    jobWorkstationValue_ =
        makeValue(
            QStringLiteral("—"),
            detailCard);

    grid->addWidget(
        makeCaption(
            QStringLiteral("Request ID"),
            detailCard),
        0,
        0);

    grid->addWidget(
        jobRequestIdValue_,
        1,
        0);

    grid->addWidget(
        makeCaption(
            QStringLiteral("Device type"),
            detailCard),
        0,
        1);

    grid->addWidget(
        jobDeviceTypeValue_,
        1,
        1);

    grid->addWidget(
        makeCaption(
            QStringLiteral("Assigned method"),
            detailCard),
        0,
        2);

    grid->addWidget(
        jobRequestedMethodValue_,
        1,
        2);

    grid->addWidget(
        makeCaption(
            QStringLiteral("Asset"),
            detailCard),
        2,
        0);

    grid->addWidget(
        jobAssetValue_,
        3,
        0);

    grid->addWidget(
        makeCaption(
            QStringLiteral("Workstation"),
            detailCard),
        2,
        1);

    grid->addWidget(
        jobWorkstationValue_,
        3,
        1);

    detailLayout->addLayout(
        grid);

    root->addWidget(
        detailCard);

    auto *targetCard =
        makeCard(page);

    auto *targetLayout =
        new QVBoxLayout(
            targetCard);

    targetLayout->setContentsMargins(
        18, 18, 18, 18);

    targetLayout->setSpacing(
        10);

    auto *targetHeader =
        new QHBoxLayout;

    targetHeader->addWidget(
        makeCaption(
            QStringLiteral("Physical target"),
            targetCard));

    targetHeader->addStretch();

    refreshDevicesButton_ =
        makeSecondaryButton(
            QStringLiteral("Refresh devices"),
            targetCard);

    targetHeader->addWidget(
        refreshDevicesButton_);

    targetLayout->addLayout(
        targetHeader);

    deviceTable_ =
        new QTableWidget(targetCard);

    deviceTable_->setColumnCount(
        6);

    deviceTable_->setHorizontalHeaderLabels(
        {
            QStringLiteral("Model"),
            QStringLiteral("Serial"),
            QStringLiteral("Interface"),
            QStringLiteral("Capacity"),
            QStringLiteral("System"),
            QStringLiteral("Device ID")
        });

    deviceTable_->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    deviceTable_->setSelectionMode(
        QAbstractItemView::SingleSelection);

    deviceTable_->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    deviceTable_->setShowGrid(
        false);

    deviceTable_->verticalHeader()
        ->setVisible(false);

    deviceTable_->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::Stretch);

    deviceTable_->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::Stretch);

    deviceTable_->horizontalHeader()
        ->setSectionResizeMode(
            2,
            QHeaderView::ResizeToContents);

    deviceTable_->horizontalHeader()
        ->setSectionResizeMode(
            3,
            QHeaderView::ResizeToContents);

    deviceTable_->horizontalHeader()
        ->setSectionResizeMode(
            4,
            QHeaderView::ResizeToContents);

    deviceTable_->horizontalHeader()
        ->setSectionResizeMode(
            5,
            QHeaderView::Stretch);

    targetLayout->addWidget(
        deviceTable_);

    root->addWidget(
        targetCard);

    auto *workspace =
        new QHBoxLayout;

    workspace->setSpacing(
        16);

    auto *safetyCard =
        makeCard(page);

    auto *safetyLayout =
        new QVBoxLayout(
            safetyCard);

    safetyLayout->setContentsMargins(
        18, 18, 18, 18);

    safetyLayout->setSpacing(
        9);

    safetyLayout->addWidget(
        makeCaption(
            QStringLiteral("Target safety"),
            safetyCard));

    targetSafetyBadge_ =
        new QLabel(
            QStringLiteral("NOT CHECKED"),
            safetyCard);

    targetSafetyBadge_->setAlignment(
        Qt::AlignCenter);

    targetSafetyBadge_->setStyleSheet(
        badgeStyle("NOT_CHECKED"));

    safetyLayout->addWidget(
        targetSafetyBadge_);

    targetSafetyText_ =
        new QLabel(
            QStringLiteral(
                "Select a physical target and run a fresh safety check."),
            safetyCard);

    targetSafetyText_->setWordWrap(
        true);

    targetSafetyText_->setStyleSheet(
        "color:#667085;"
        "font-size:11px;");

    safetyLayout->addWidget(
        targetSafetyText_);

    auto *targetGrid =
        new QGridLayout;

    targetGrid->setHorizontalSpacing(
        18);

    targetGrid->setVerticalSpacing(
        8);

    targetModelValue_ =
        makeValue(
            QStringLiteral("—"),
            safetyCard);

    targetSerialValue_ =
        makeValue(
            QStringLiteral("—"),
            safetyCard);

    targetCapacityValue_ =
        makeValue(
            QStringLiteral("—"),
            safetyCard);

    targetInterfaceValue_ =
        makeValue(
            QStringLiteral("—"),
            safetyCard);

    targetPathValue_ =
        makeValue(
            QStringLiteral("—"),
            safetyCard);

    targetGrid->addWidget(
        makeCaption(
            QStringLiteral("Model"),
            safetyCard),
        0,
        0);

    targetGrid->addWidget(
        targetModelValue_,
        1,
        0);

    targetGrid->addWidget(
        makeCaption(
            QStringLiteral("Serial"),
            safetyCard),
        0,
        1);

    targetGrid->addWidget(
        targetSerialValue_,
        1,
        1);

    targetGrid->addWidget(
        makeCaption(
            QStringLiteral("Capacity"),
            safetyCard),
        2,
        0);

    targetGrid->addWidget(
        targetCapacityValue_,
        3,
        0);

    targetGrid->addWidget(
        makeCaption(
            QStringLiteral("Interface"),
            safetyCard),
        2,
        1);

    targetGrid->addWidget(
        targetInterfaceValue_,
        3,
        1);

    targetGrid->addWidget(
        makeCaption(
            QStringLiteral("Physical device"),
            safetyCard),
        4,
        0,
        1,
        2);

    targetGrid->addWidget(
        targetPathValue_,
        5,
        0,
        1,
        2);

    safetyLayout->addLayout(
        targetGrid);

    capabilityValue_ =
        makeValue(
            QStringLiteral("—"),
            safetyCard);

    selectedMethodValue_ =
        makeValue(
            QStringLiteral("—"),
            safetyCard);

    safetyLayout->addWidget(
        makeCaption(
            QStringLiteral("Capability"),
            safetyCard));

    safetyLayout->addWidget(
        capabilityValue_);

    safetyLayout->addWidget(
        makeCaption(
            QStringLiteral("Detected method"),
            safetyCard));

    safetyLayout->addWidget(
        selectedMethodValue_);

    workspace->addWidget(
        safetyCard,
        2);

    auto *executionCard =
        makeCard(page);

    auto *executionLayout =
        new QVBoxLayout(
            executionCard);

    executionLayout->setContentsMargins(
        18, 18, 18, 18);

    executionLayout->setSpacing(
        9);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Sanitization pipeline"),
            executionCard));

    pipelineStatusValue_ =
        new QLabel(
            QStringLiteral("NOT STARTED"),
            executionCard);

    pipelineStatusValue_->setAlignment(
        Qt::AlignCenter);

    pipelineStatusValue_->setStyleSheet(
        badgeStyle("NOT_STARTED"));

    executionLayout->addWidget(
        pipelineStatusValue_);

    operationProgress_ =
        new QProgressBar(
            executionCard);

    operationProgress_->setRange(
        0,
        100);

    operationProgress_->setValue(
        0);

    operationProgress_->setTextVisible(
        false);

    operationProgress_->setFixedHeight(
        8);

    executionLayout->addWidget(
        operationProgress_);

    verificationValue_ =
        makeValue(
            QStringLiteral("—"),
            executionCard);

    operationValue_ =
        makeValue(
            QStringLiteral("—"),
            executionCard);

    bytesProcessedValue_ =
        makeValue(
            QStringLiteral("—"),
            executionCard);

    bytesVerifiedValue_ =
        makeValue(
            QStringLiteral("—"),
            executionCard);

    samplesValue_ =
        makeValue(
            QStringLiteral("—"),
            executionCard);

    certificateValue_ =
        makeValue(
            QStringLiteral("—"),
            executionCard);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral("Verification"),
            executionCard));

    executionLayout->addWidget(
        verificationValue_);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral("Operation ID"),
            executionCard));

    executionLayout->addWidget(
        operationValue_);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral("Bytes processed"),
            executionCard));

    executionLayout->addWidget(
        bytesProcessedValue_);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral("Bytes verified"),
            executionCard));

    executionLayout->addWidget(
        bytesVerifiedValue_);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral("Samples"),
            executionCard));

    executionLayout->addWidget(
        samplesValue_);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral("Certificate"),
            executionCard));

    executionLayout->addWidget(
        certificateValue_);

    workspace->addWidget(
        executionCard,
        1);

    root->addLayout(
        workspace);

    auto *actionCard =
        makeCard(page);

    auto *actionLayout =
        new QVBoxLayout(
            actionCard);

    actionLayout->setContentsMargins(
        18, 16, 18, 16);

    jobMessageLabel_ =
        new QLabel(
            QStringLiteral(
                "Select an assigned request to begin."),
            actionCard);

    jobMessageLabel_->setWordWrap(
        true);

    jobMessageLabel_->setStyleSheet(
        "color:#667085;"
        "font-size:12px;"
        "font-weight:600;");

    actionLayout->addWidget(
        jobMessageLabel_);

    auto *buttons =
        new QHBoxLayout;

    buttons->addStretch();

    validateTargetButton_ =
        makeSecondaryButton(
            QStringLiteral(
                "Run safety check"),
            actionCard);

    validateTargetButton_->setEnabled(
        false);

    buttons->addWidget(
        validateTargetButton_);

    startSanitizationButton_ =
        makePrimaryButton(
            QStringLiteral(
                "Start sanitization"),
            actionCard);

    startSanitizationButton_->setEnabled(
        false);

    buttons->addWidget(
        startSanitizationButton_);

    actionLayout->addLayout(
        buttons);

    root->addWidget(
        actionCard);

    scroll->setWidget(
        page);

    auto *layout =
        new QVBoxLayout(
            jobsPage_);

    layout->setContentsMargins(
        0, 0, 0, 0);

    layout->addWidget(
        scroll);
}

void MainWindow::buildDevicesPage()
{
    devicesPage_ =
        new QWidget;

    auto *scroll =
        new QScrollArea(
            devicesPage_);

    scroll->setWidgetResizable(
        true);

    scroll->setFrameShape(
        QFrame::NoFrame);

    auto *page =
        new QWidget;

    auto *root =
        new QVBoxLayout(page);

    root->setContentsMargins(
        28, 26, 28, 28);

    root->setSpacing(
        16);

    auto *header =
        new QHBoxLayout;

    auto *titleBlock =
        new QVBoxLayout;

    titleBlock->addWidget(
        makeTitle(
            QStringLiteral(
                "Physical Storage Devices"),
            page));

    titleBlock->addWidget(
        makeSubtitle(
            QStringLiteral(
                "Storage targets currently discovered by SecureWipe."),
            page));

    header->addLayout(
        titleBlock);

    header->addStretch();

    auto *refreshButton =
        makeSecondaryButton(
            QStringLiteral("Refresh"),
            page);

    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &MainWindow::refreshPhysicalDevices);

    header->addWidget(
        refreshButton);

    root->addLayout(
        header);

    auto *card =
        makeCard(page);

    auto *layout =
        new QVBoxLayout(card);

    layout->setContentsMargins(
        16, 16, 16, 16);

    auto *table =
        new QTableWidget(card);

    table->setColumnCount(
        5);

    table->setHorizontalHeaderLabels(
        {
            QStringLiteral("Model"),
            QStringLiteral("Serial"),
            QStringLiteral("Interface"),
            QStringLiteral("Capacity"),
            QStringLiteral("Device ID")
        });

    table->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    table->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    table->setShowGrid(
        false);

    table->verticalHeader()
        ->setVisible(false);

    table->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::Stretch);

    table->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::Stretch);

    table->horizontalHeader()
        ->setSectionResizeMode(
            2,
            QHeaderView::ResizeToContents);

    table->horizontalHeader()
        ->setSectionResizeMode(
            3,
            QHeaderView::ResizeToContents);

    table->horizontalHeader()
        ->setSectionResizeMode(
            4,
            QHeaderView::Stretch);

    layout->addWidget(
        table);

    root->addWidget(
        card);

    scroll->setWidget(
        page);

    auto *outer =
        new QVBoxLayout(
            devicesPage_);

    outer->setContentsMargins(
        0, 0, 0, 0);

    outer->addWidget(
        scroll);
}

void MainWindow::buildForensicsPage()
{
    forensicsPage_ =
        new QWidget;

    forensicPage_ =
        forensicsPage_;

    auto *root =
        new QVBoxLayout(
            forensicsPage_);

    root->setContentsMargins(
        28, 26, 28, 28);

    root->setSpacing(
        16);

    root->addWidget(
        makeTitle(
            QStringLiteral(
                "Evidence & Forensics"),
            forensicsPage_));

    root->addWidget(
        makeSubtitle(
            QStringLiteral(
                "Audit, operation and certificate evidence produced by the sanitization pipeline."),
            forensicsPage_));

    auto *card =
        makeCard(forensicsPage_);

    auto *layout =
        new QVBoxLayout(card);

    layout->setContentsMargins(
        18, 18, 18, 18);

    layout->addWidget(
        makeCaption(
            QStringLiteral(
                "Latest evidence"),
            card));

    evidenceValue_ =
        makeValue(
            QStringLiteral(
                "No pipeline evidence generated yet."),
            card);

    layout->addWidget(
        evidenceValue_);

    auto *info =
        new QLabel(
            QStringLiteral(
                "Evidence is generated by the sanitization pipeline and should not be modified manually from the workstation UI."),
            card);

    info->setWordWrap(
        true);

    info->setStyleSheet(
        "color:#667085;"
        "font-size:11px;");

    layout->addWidget(
        info);

    root->addWidget(
        card);

    root->addStretch();
}

void MainWindow::buildSettingsPage()
{
    settingsPage_ =
        new QWidget;

    auto *root =
        new QVBoxLayout(
            settingsPage_);

    root->setContentsMargins(
        28, 26, 28, 28);

    root->setSpacing(
        16);

    root->addWidget(
        makeTitle(
            QStringLiteral("Settings"),
            settingsPage_));

    root->addWidget(
        makeSubtitle(
            QStringLiteral(
                "Workstation configuration and execution policy."),
            settingsPage_));

    auto *apiCard =
        makeCard(settingsPage_);

    auto *apiLayout =
        new QVBoxLayout(
            apiCard);

    apiLayout->setContentsMargins(
        18, 18, 18, 18);

    apiLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Backend API"),
            apiCard));

    apiLayout->addWidget(
        makeValue(
            QStringLiteral(
                "http://localhost:5000"),
            apiCard));

    root->addWidget(
        apiCard);

    auto *safetyCard =
        makeCard(settingsPage_);

    auto *safetyLayout =
        new QVBoxLayout(
            safetyCard);

    safetyLayout->setContentsMargins(
        18, 18, 18, 18);

    safetyLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Destructive-operation policy"),
            safetyCard));

    auto *safety =
        new QLabel(
            QStringLiteral(
                "Never authorize an operating-system disk or an unintended storage device. "
                "The exact target must pass identity validation and safety checks before physical sanitization."),
            safetyCard);

    safety->setWordWrap(
        true);

    safety->setStyleSheet(
        "color:#667085;"
        "font-size:12px;");

    safetyLayout->addWidget(
        safety);

    root->addWidget(
        safetyCard);

    root->addStretch();
}

void MainWindow::applyTheme()
{
    qApp->setStyleSheet(
        "QWidget {font-family:'Segoe UI';}"
        "QMainWindow {background:#F6F8FB;}"
        "QLineEdit {"
        "background:#FFFFFF;"
        "color:#172033;"
        "border:1px solid #D0D5DD;"
        "border-radius:9px;"
        "padding:8px 12px;"
        "font-size:12px;"
        "}"
        "QLineEdit:focus {border-color:#2563EB;}"
        "QComboBox {"
        "background:#FFFFFF;"
        "color:#172033;"
        "border:1px solid #D0D5DD;"
        "border-radius:9px;"
        "padding:8px 12px;"
        "font-size:12px;"
        "}"
        "QComboBox:focus {border-color:#2563EB;}"
        "QTableWidget {"
        "background:#FFFFFF;"
        "color:#172033;"
        "border:1px solid #E4E7EC;"
        "border-radius:10px;"
        "gridline-color:transparent;"
        "selection-background-color:#EFF6FF;"
        "selection-color:#172033;"
        "font-size:12px;"
        "}"
        "QTableWidget::item {padding:8px;}"
        "QHeaderView::section {"
        "background:#F8FAFC;"
        "color:#667085;"
        "border:none;"
        "border-bottom:1px solid #E4E7EC;"
        "padding:9px;"
        "font-size:10px;"
        "font-weight:700;"
        "}"
        "QProgressBar {"
        "background:#F2F4F7;"
        "border:none;"
        "border-radius:4px;"
        "}"
        "QProgressBar::chunk {"
        "background:#2563EB;"
        "border-radius:4px;"
        "}"
        "QScrollArea {"
        "background:#F6F8FB;"
        "border:none;"
        "}");
}

void MainWindow::setActiveNav(
    QPushButton *button)
{
    const QList<QPushButton *> buttons = {
        dashboardNavButton_,
        jobsNavButton_,
        devicesNavButton_,
        forensicsNavButton_,
        settingsNavButton_
    };

    for (QPushButton *item : buttons)
    {
        if (!item)
            continue;

        item->setMinimumHeight(
            40);

        if (item == button)
        {
            item->setStyleSheet(
                "QPushButton {"
                "background:#EFF6FF;"
                "color:#1D4ED8;"
                "border:none;"
                "border-radius:9px;"
                "text-align:left;"
                "padding:9px 12px;"
                "font-size:12px;"
                "font-weight:700;"
                "}"
                "QPushButton:hover {"
                "background:#DBEAFE;"
                "}");
        }
        else
        {
            item->setStyleSheet(
                "QPushButton {"
                "background:transparent;"
                "color:#475467;"
                "border:none;"
                "border-radius:9px;"
                "text-align:left;"
                "padding:9px 12px;"
                "font-size:12px;"
                "font-weight:600;"
                "}"
                "QPushButton:hover {"
                "background:#F2F4F7;"
                "}");
        }
    }
}

void MainWindow::setConnectionState(
    bool connected,
    const QString &text)
{
    connectionBadgeLabel_->setText(
        text.isEmpty()
            ? (connected
                   ? QStringLiteral("Connected")
                   : QStringLiteral("Disconnected"))
            : text);

    connectionBadgeLabel_->setStyleSheet(
        badgeStyle(
            connected
                ? QStringLiteral("SAFE")
                : QStringLiteral("FAILED")));
}

void MainWindow::showPage(
    QWidget *page,
    QPushButton *navButton)
{
    if (!page)
        return;

    contentStack_->setCurrentWidget(
        page);

    setActiveNav(
        navButton);
}

void MainWindow::refreshAssignedRequests()
{
    if (!authManager_)
        return;

    const QString token =
        authManager_->token();

    if (token.isEmpty())
        return;

    requestService_->fetchAssignedRequests(
        token);
}

void MainWindow::handleAssignedRequests(
    const QJsonArray &requests)
{
    assignedRequests_ =
        requests;

    int total = 0;
    int active = 0;
    int completed = 0;
    int failed = 0;

    dashboardJobsTable_->clearContents();
    dashboardJobsTable_->setRowCount(0);

    assignedJobsTable_->clearContents();
    assignedJobsTable_->setRowCount(0);

    jobComboBox_->blockSignals(
        true);

    jobComboBox_->clear();

    for (const QJsonValue &value :
         requests)
    {
        if (!value.isObject())
            continue;

        const QJsonObject request =
            value.toObject();

        const QString requestId =
            request.value(
                QStringLiteral("requestId"))
                .toString();

        const QString deviceType =
            request.value(
                QStringLiteral("deviceType"))
                .toString();

        const QString method =
            request.value(
                QStringLiteral("sanitizationMethod"))
                .toString();

        const QString status =
            request.value(
                QStringLiteral("status"))
                .toString();

        const QString asset =
            request.value(
                QStringLiteral("asset"))
                .toString(
                    request.value(
                        QStringLiteral("customer"))
                        .toString());

        const QString workstation =
            request.value(
                QStringLiteral("workstationCenter"))
                .toString(
                    request.value(
                        QStringLiteral("center"))
                        .toString());

        if (requestId.isEmpty())
            continue;

        ++total;

        if (status == "ASSIGNED" ||
            status == "IN_PROGRESS" ||
            status == "VERIFYING")
        {
            ++active;
        }

        if (status == "COMPLETED")
            ++completed;

        if (status == "FAILED")
            ++failed;

        if (dashboardJobsTable_->rowCount() <
            8)
        {
            const int row =
                dashboardJobsTable_->rowCount();

            dashboardJobsTable_->insertRow(
                row);

            dashboardJobsTable_->setItem(
                row,
                0,
                new QTableWidgetItem(
                    requestId));

            dashboardJobsTable_->setItem(
                row,
                1,
                new QTableWidgetItem(
                    deviceType));

            dashboardJobsTable_->setItem(
                row,
                2,
                new QTableWidgetItem(
                    method.isEmpty()
                        ? QStringLiteral("—")
                        : method));

            dashboardJobsTable_->setItem(
                row,
                3,
                new QTableWidgetItem(
                    status));
        }

        const int row =
            assignedJobsTable_->rowCount();

        assignedJobsTable_->insertRow(
            row);

        assignedJobsTable_->setItem(
            row,
            0,
            new QTableWidgetItem(
                requestId));

        assignedJobsTable_->setItem(
            row,
            1,
            new QTableWidgetItem(
                deviceType));

        assignedJobsTable_->setItem(
            row,
            2,
            new QTableWidgetItem(
                method.isEmpty()
                    ? QStringLiteral("—")
                    : method));

        assignedJobsTable_->setItem(
            row,
            3,
            new QTableWidgetItem(
                asset.isEmpty()
                    ? QStringLiteral("—")
                    : asset));

        assignedJobsTable_->setItem(
            row,
            4,
            new QTableWidgetItem(
                status));

        const QString display =
            QStringLiteral(
                "%1  •  %2  •  %3")
                .arg(
                    requestId,
                    deviceType,
                    method.isEmpty()
                        ? QStringLiteral(
                            "Method not specified")
                        : method);

        jobComboBox_->addItem(
            display);

        const int index =
            jobComboBox_->count() - 1;

        jobComboBox_->setItemData(
            index,
            requestId,
            Qt::UserRole);

        jobComboBox_->setItemData(
            index,
            deviceType,
            Qt::UserRole + 1);

        jobComboBox_->setItemData(
            index,
            method,
            Qt::UserRole + 2);

        if (!selectedRequestId_.isEmpty() &&
            requestId == selectedRequestId_)
        {
            jobComboBox_->setCurrentIndex(
                index);
        }
    }

    totalJobsValue_->setText(
        QString::number(total));

    activeJobsValue_->setText(
        QString::number(active));

    completedJobsValue_->setText(
        QString::number(completed));

    failedJobsValue_->setText(
        QString::number(failed));

    jobComboBox_->blockSignals(
        false);

    if (jobComboBox_->currentIndex() < 0 &&
        jobComboBox_->count() > 0)
    {
        jobComboBox_->setCurrentIndex(
            0);
    }

    populateJobDetails();
    populateDeviceTable();
}

QJsonObject MainWindow::selectedRequestObject() const
{
    for (const QJsonValue &value :
         assignedRequests_)
    {
        if (!value.isObject())
            continue;

        const QJsonObject request =
            value.toObject();

        if (request.value(
                QStringLiteral("requestId"))
            .toString()
            == selectedRequestId_)
        {
            return request;
        }
    }

    return {};
}

void MainWindow::selectRequestFromJobs(
    int index)
{
    selectedRequestId_.clear();
    selectedRequestDeviceType_.clear();
    selectedRequestMethod_.clear();

    if (index < 0)
    {
        populateJobDetails();
        resetTargetPanel();
        return;
    }

    selectedRequestId_ =
        jobComboBox_->itemData(
            index,
            Qt::UserRole).toString();

    selectedRequestDeviceType_ =
        jobComboBox_->itemData(
            index,
            Qt::UserRole + 1).toString();

    selectedRequestMethod_ =
        jobComboBox_->itemData(
            index,
            Qt::UserRole + 2).toString();

    populateJobDetails();
    resetTargetPanel();
    populateDeviceTable();
}

void MainWindow::populateJobDetails()
{
    const QJsonObject request =
        selectedRequestObject();

    jobRequestIdValue_->setText(
        request.value(
            QStringLiteral("requestId"))
            .toString(
                QStringLiteral("—")));

    jobDeviceTypeValue_->setText(
        request.value(
            QStringLiteral("deviceType"))
            .toString(
                QStringLiteral("—")));

    jobRequestedMethodValue_->setText(
        request.value(
            QStringLiteral("sanitizationMethod"))
            .toString(
                QStringLiteral("—")));

    jobAssetValue_->setText(
        request.value(
            QStringLiteral("asset"))
            .toString(
                request.value(
                    QStringLiteral("customer"))
                    .toString(
                        QStringLiteral("—"))));

    jobWorkstationValue_->setText(
        request.value(
            QStringLiteral("workstationCenter"))
            .toString(
                request.value(
                    QStringLiteral("center"))
                    .toString(
                        QStringLiteral("—"))));

    const QString status =
        request.value(
            QStringLiteral("status"))
            .toString();

    if (status == "ASSIGNED")
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "This request is ready for physical workstation execution."));
    }
    else if (status == "IN_PROGRESS")
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "This request is currently in physical sanitization."));
    }
    else if (status == "VERIFYING")
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "The sanitization result is in verification/evidence state."));
    }
    else if (status == "COMPLETED")
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "This sanitization request is completed."));
    }
    else if (status == "FAILED")
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "This sanitization request is marked failed."));
    }
    else
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "Select an assigned request to begin."));
    }
}

void MainWindow::refreshPhysicalDevices()
{
    if (operationRunning_)
        return;

    jobMessageLabel_->setText(
        QStringLiteral(
            "Refreshing physical storage devices..."));

    deviceController_->refreshDevices();
}

void MainWindow::populateDeviceTable()
{
    if (!deviceTable_)
        return;

    deviceTable_->blockSignals(
        true);

    deviceTable_->clearContents();
    deviceTable_->setRowCount(
        0);

    const auto &devices =
        deviceController_->devices();

    for (int i = 0;
         i < static_cast<int>(
             devices.size());
         ++i)
    {
        const StorageDevice &device =
            devices[
                static_cast<std::size_t>(
                    i)];

        if (!selectedRequestDeviceType_
                 .isEmpty() &&
            !requestMatchesDevice(
                selectedRequestDeviceType_,
                device))
        {
            continue;
        }

        const int row =
            deviceTable_->rowCount();

        deviceTable_->insertRow(
            row);

        auto *model =
            new QTableWidgetItem(
                QString::fromStdString(
                    device.getModel()));

        model->setData(
            Qt::UserRole,
            i);

        deviceTable_->setItem(
            row,
            0,
            model);

        deviceTable_->setItem(
            row,
            1,
            new QTableWidgetItem(
                QString::fromStdString(
                    device.getSerialNumber())));

        deviceTable_->setItem(
            row,
            2,
            new QTableWidgetItem(
                QString::fromStdString(
                    device.getInterfaceType())));

        deviceTable_->setItem(
            row,
            3,
            new QTableWidgetItem(
                formatCapacity(
                    device.getCapacityBytes())));

        auto *systemItem =
            new QTableWidgetItem(
                device.isSystemDisk()
                    ? QStringLiteral("BLOCKED")
                    : QStringLiteral("No"));

        if (device.isSystemDisk())
        {
            systemItem->setForeground(
                QColor("#B42318"));
        }

        deviceTable_->setItem(
            row,
            4,
            systemItem);

        deviceTable_->setItem(
            row,
            5,
            new QTableWidgetItem(
                QString::fromStdString(
                    device.getDeviceId())));
    }

    deviceTable_->blockSignals(
        false);
}

bool MainWindow::requestMatchesDevice(
    const QString &requestedType,
    const StorageDevice &device) const
{
    return requestMatchesDeviceImpl(
        requestedType,
        device);
}

void MainWindow::selectTargetDevice(
    int row)
{
    if (operationRunning_)
        return;

    if (row < 0)
    {
        resetTargetPanel();
        return;
    }

    auto *item =
        deviceTable_->item(
            row,
            0);

    if (!item)
        return;

    bool ok = false;

    const int deviceIndex =
        item->data(
            Qt::UserRole)
            .toInt(
                &ok);

    if (!ok)
        return;

    if (!deviceController_->selectTarget(
            deviceIndex))
    {
        targetSafetyText_->setText(
            QStringLiteral(
                "Unable to select the physical device."));
        return;
    }

    const StorageDevice &device =
        deviceController_->devices().at(
            static_cast<std::size_t>(
                deviceIndex));

    updateTargetPanelFromDevice(
        device);

    const SanitizationMethod method =
        deviceController_->detectSelectedTargetMethod();

    capabilityValue_->setText(
        method == SanitizationMethod::Unsupported
            ? QStringLiteral("Unsupported")
            : QStringLiteral("Available"));

    selectedMethodValue_->setText(
        sanitizationMethodName(
            method));

    targetSafetyBadge_->setText(
        QStringLiteral("NOT CHECKED"));

    targetSafetyBadge_->setStyleSheet(
        badgeStyle("NOT_CHECKED"));

    targetSafetyText_->setText(
        QStringLiteral(
            "Exact target selected. Run fresh target validation and safety checks before sanitization."));

    validateTargetButton_->setEnabled(
        !selectedRequestId_.isEmpty());

    startSanitizationButton_->setEnabled(
        false);
}

void MainWindow::resetTargetPanel()
{
    if (deviceTable_)
    {
        deviceTable_->clearSelection();
    }

    targetModelValue_->setText(
        QStringLiteral("—"));

    targetSerialValue_->setText(
        QStringLiteral("—"));

    targetCapacityValue_->setText(
        QStringLiteral("—"));

    targetInterfaceValue_->setText(
        QStringLiteral("—"));

    targetPathValue_->setText(
        QStringLiteral("—"));

    capabilityValue_->setText(
        QStringLiteral("—"));

    selectedMethodValue_->setText(
        QStringLiteral("—"));

    targetSafetyBadge_->setText(
        QStringLiteral("NOT CHECKED"));

    targetSafetyBadge_->setStyleSheet(
        badgeStyle("NOT_CHECKED"));

    targetSafetyText_->setText(
        QStringLiteral(
            "Select an exact physical target."));

    pipelineStatusValue_->setText(
        QStringLiteral("NOT STARTED"));

    pipelineStatusValue_->setStyleSheet(
        badgeStyle("NOT_STARTED"));

    verificationValue_->setText(
        QStringLiteral("—"));

    operationValue_->setText(
        QStringLiteral("—"));

    bytesProcessedValue_->setText(
        QStringLiteral("—"));

    bytesVerifiedValue_->setText(
        QStringLiteral("—"));

    samplesValue_->setText(
        QStringLiteral("—"));

    certificateValue_->setText(
        QStringLiteral("—"));

    operationProgress_->setRange(
        0,
        100);

    operationProgress_->setValue(
        0);

    validateTargetButton_->setEnabled(
        false);

    startSanitizationButton_->setEnabled(
        false);
}

void MainWindow::runTargetSafetyCheck()
{
    if (operationRunning_)
        return;

    if (!deviceController_->selectedTarget()
             .has_value())
    {
        targetSafetyBadge_->setText(
            QStringLiteral("BLOCKED"));

        targetSafetyBadge_->setStyleSheet(
            badgeStyle("BLOCKED"));

        targetSafetyText_->setText(
            QStringLiteral(
                "Select a physical target device first."));

        return;
    }

    validateTargetButton_->setEnabled(
        false);

    startSanitizationButton_->setEnabled(
        false);

    targetSafetyBadge_->setText(
        QStringLiteral("CHECKING"));

    targetSafetyBadge_->setStyleSheet(
        badgeStyle("CHECKING"));

    targetSafetyText_->setText(
        QStringLiteral(
            "Running fresh target identity validation and safety checks..."));

    QApplication::processEvents();

    if (!deviceController_->validateSelectedTarget())
    {
        targetSafetyBadge_->setText(
            QStringLiteral("BLOCKED"));

        targetSafetyBadge_->setStyleSheet(
            badgeStyle("BLOCKED"));

        targetSafetyText_->setText(
            QStringLiteral(
                "Target validation failed. Re-select the physical target."));
        
        validateTargetButton_->setEnabled(
            true);

        return;
    }

    if (!deviceController_->evaluateSelectedTarget())
    {
        const SafetyResult &result =
            deviceController_->lastSafetyResult();

        targetSafetyBadge_->setText(
            QStringLiteral("BLOCKED"));

        targetSafetyBadge_->setStyleSheet(
            badgeStyle("BLOCKED"));

        QString details =
            result.summary.empty()
                ? QStringLiteral(
                      "Safety checks did not authorize this target.")
                : QString::fromStdString(
                      result.summary);

        for (const SafetyCheckResult &check :
             result.checks)
        {
            if (!details.isEmpty())
                details += '\n';

            details +=
                QStringLiteral(
                    "%1 %2")
                    .arg(
                        check.passed
                            ? QStringLiteral("✓")
                            : QStringLiteral("✗"),
                        QString::fromStdString(
                            check.checkName));
        }

        targetSafetyText_->setText(
            details);

        validateTargetButton_->setEnabled(
            true);

        return;
    }

    const auto target =
        deviceController_->selectedTarget();

    if (!target.has_value())
    {
        validateTargetButton_->setEnabled(
            true);
        return;
    }

    updateTargetPanelFromDevice(
        *target);

    const SanitizationMethod method =
        deviceController_->detectSelectedTargetMethod();

    if (method ==
        SanitizationMethod::Unsupported)
    {
        targetSafetyBadge_->setText(
            QStringLiteral(
                "UNSUPPORTED"));

        targetSafetyBadge_->setStyleSheet(
            badgeStyle("BLOCKED"));

        targetSafetyText_->setText(
            QStringLiteral(
                "Safety checks passed, but no supported sanitization method is available for this target."));

        validateTargetButton_->setEnabled(
            true);

        return;
    }

    QString detectedMethod;

    switch (method)
    {
    case SanitizationMethod::NvmeSanitize:
        detectedMethod =
            QStringLiteral("NVME_SANITIZE");
        break;

    case SanitizationMethod::AtaSanitize:
        detectedMethod =
            QStringLiteral("ATA_SANITIZE");
        break;

    case SanitizationMethod::HostOverwrite:
        detectedMethod =
            QStringLiteral("HOST_OVERWRITE");
        break;

    default:
        detectedMethod =
            QStringLiteral("UNSUPPORTED");
        break;
    }

    QString assignedMethod =
        selectedRequestMethod_
            .trimmed()
            .toUpper();

    assignedMethod.replace(
        QStringLiteral(" "),
        QStringLiteral("_"));

    assignedMethod.replace(
        QStringLiteral("-"),
        QStringLiteral("_"));

    if (assignedMethod == "NVME")
        assignedMethod =
            QStringLiteral("NVME_SANITIZE");

    if (assignedMethod == "ATA")
        assignedMethod =
            QStringLiteral("ATA_SANITIZE");

    if (!assignedMethod.isEmpty() &&
        assignedMethod != "UNSUPPORTED" &&
        assignedMethod != detectedMethod)
    {
        targetSafetyBadge_->setText(
            QStringLiteral(
                "METHOD MISMATCH"));

        targetSafetyBadge_->setStyleSheet(
            badgeStyle("BLOCKED"));

        targetSafetyText_->setText(
            QStringLiteral(
                "The method assigned to this request does not match the method detected for the physical target."));

        validateTargetButton_->setEnabled(
            true);

        return;
    }

    const SafetyResult &result =
        deviceController_->lastSafetyResult();

    targetSafetyBadge_->setText(
        QStringLiteral("SAFE"));

    targetSafetyBadge_->setStyleSheet(
        badgeStyle("SAFE"));

    QString details =
        result.summary.empty()
            ? QStringLiteral(
                  "All required safety checks passed.")
            : QString::fromStdString(
                  result.summary);

    for (const SafetyCheckResult &check :
         result.checks)
    {
        if (!details.isEmpty())
            details += '\n';

        details +=
            QStringLiteral(
                "%1 %2")
                .arg(
                    check.passed
                        ? QStringLiteral("✓")
                        : QStringLiteral("✗"),
                    QString::fromStdString(
                        check.checkName));
    }

    targetSafetyText_->setText(
        details);

    const QJsonObject request =
        selectedRequestObject();

    const QString status =
        request.value(
            QStringLiteral("status"))
            .toString();

    startSanitizationButton_->setEnabled(
        status == QStringLiteral("ASSIGNED"));

    validateTargetButton_->setEnabled(
        true);

    if (status == QStringLiteral("ASSIGNED"))
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "Safety checks passed. Final destructive confirmation is available."));
    }
}

void MainWindow::startSanitization()
{
    if (operationRunning_)
        return;

    if (selectedRequestId_.isEmpty())
    {
        QMessageBox::warning(
            this,
            QStringLiteral("No request"),
            QStringLiteral(
                "Select an assigned sanitization request first."));
        return;
    }

    const QJsonObject request =
        selectedRequestObject();

    if (request.value(
            QStringLiteral("status"))
            .toString()
        != QStringLiteral("ASSIGNED"))
    {
        QMessageBox::warning(
            this,
            QStringLiteral(
                "Request unavailable"),
            QStringLiteral(
                "Only an ASSIGNED request can start physical execution."));
        return;
    }

    const auto target =
        deviceController_->selectedTarget();

    if (!target.has_value())
    {
        QMessageBox::warning(
            this,
            QStringLiteral("No target"),
            QStringLiteral(
                "Select a physical target device."));
        return;
    }

    if (!deviceController_->validateSelectedTarget() ||
        !deviceController_->evaluateSelectedTarget())
    {
        runTargetSafetyCheck();
        return;
    }

    if (!requestMatchesDevice(
            selectedRequestDeviceType_,
            *target))
    {
        QMessageBox::critical(
            this,
            QStringLiteral(
                "Target mismatch"),
            QStringLiteral(
                "The selected physical target does not match the assigned request device type."));
        return;
    }

    const SanitizationMethod method =
        deviceController_->detectSelectedTargetMethod();

    if (method ==
        SanitizationMethod::Unsupported)
    {
        QMessageBox::critical(
            this,
            QStringLiteral(
                "Unsupported target"),
            QStringLiteral(
                "No supported sanitization method is available for this target."));
        return;
    }

    const QString model =
        QString::fromStdString(
            target->getModel());

    const QString serial =
        QString::fromStdString(
            target->getSerialNumber());

    const QString deviceId =
        QString::fromStdString(
            target->getDeviceId());

    const QMessageBox::StandardButton answer =
        QMessageBox::warning(
            this,
            QStringLiteral(
                "Final destructive confirmation"),
            QStringLiteral(
                "You are about to permanently sanitize this physical device.\n\n"
                "Request: %1\n"
                "Device type: %2\n"
                "Model: %3\n"
                "Serial: %4\n"
                "Physical device: %5\n"
                "Method: %6\n\n"
                "This operation is destructive and cannot be undone.\n"
                "Continue only when this exact physical target is intentionally authorized.")
                .arg(
                    selectedRequestId_,
                    selectedRequestDeviceType_,
                    model,
                    serial,
                    deviceId,
                    sanitizationMethodName(
                        method)),
            QMessageBox::Yes |
                QMessageBox::No,
            QMessageBox::No);

    if (answer !=
        QMessageBox::Yes)
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "Sanitization cancelled. No destructive operation was started."));
        return;
    }

    const QString token =
        authManager_->token();

    if (token.isEmpty())
    {
        QMessageBox::critical(
            this,
            QStringLiteral(
                "Authentication"),
            QStringLiteral(
                "Authentication token is missing."));
        return;
    }

    const StorageDevice targetCopy =
        *target;

    operationRunning_ =
        true;

    validateTargetButton_->setEnabled(
        false);

    startSanitizationButton_->setEnabled(
        false);

    refreshDevicesButton_->setEnabled(
        false);

    refreshJobsButton_->setEnabled(
        false);

    jobComboBox_->setEnabled(
        false);

    deviceTable_->setEnabled(
        false);

    operationProgress_->setRange(
        0,
        0);

    pipelineStatusValue_->setText(
        QStringLiteral("STARTING"));

    pipelineStatusValue_->setStyleSheet(
        badgeStyle("IN_PROGRESS"));

    operationValue_->setText(
        QStringLiteral(
            "Waiting for request synchronization"));

    verificationValue_->setText(
        QStringLiteral("Pending"));

    jobMessageLabel_->setText(
        QStringLiteral(
            "Moving assigned request to IN_PROGRESS..."));

    requestService_->updateRequestStatus(
        token,
        selectedRequestId_,
        QStringLiteral("IN_PROGRESS"));

    connect(
        requestService_,
        &SanitizationRequestService::requestStatusUpdated,
        this,
        [this, targetCopy](
            const QString &requestId,
            const QString &status)
        {
            if (requestId !=
                    selectedRequestId_ ||
                status !=
                    QStringLiteral("IN_PROGRESS"))
            {
                return;
            }

            auto *watcher =
                new QFutureWatcher<
                    SecureWipe::SanitizationPipelineResult>(
                    this);

            connect(
                watcher,
                &QFutureWatcher<
                    SecureWipe::SanitizationPipelineResult>
                    ::finished,
                this,
                [this, watcher]()
                {
                    const auto result =
                        watcher->result();

                    watcher->deleteLater();

                    finishSanitization(
                        result);
                });

            jobMessageLabel_->setText(
                QStringLiteral(
                    "Physical sanitization is running. Do not disconnect the target device."));

            pipelineStatusValue_->setText(
                QStringLiteral("SANITIZING"));

            pipelineStatusValue_->setStyleSheet(
                badgeStyle("IN_PROGRESS"));

            watcher->setFuture(
                QtConcurrent::run(
                    [targetCopy, requestId]()
                    -> SecureWipe::SanitizationPipelineResult
                    {
                        try
                        {
                            SanitizationPipeline pipeline;

                            return pipeline.execute(
                                targetCopy,
                                requestId.toStdString(),
                                std::string());
                        }
                        catch (const std::exception &exception)
                        {
                            SecureWipe::SanitizationPipelineResult result;

                            result.sanitization.deviceId =
                                targetCopy.getDeviceId();

                            result.sanitization.model =
                                targetCopy.getModel();

                            result.sanitization.serialNumber =
                                targetCopy.getSerialNumber();

                            result.sanitization.interfaceType =
                                targetCopy.getInterfaceType();

                            result.sanitization.capacityBytes =
                                targetCopy.getCapacityBytes();

                            result.sanitization.status =
                                SecureWipe::SanitizationStatus::FAILED;

                            result.sanitization.error =
                                SecureWipe::SanitizationErrorCode::
                                    SANITIZATION_EXECUTION_FAILED;

                            result.sanitization.message =
                                exception.what();

                            result.sanitization.errorMessage =
                                exception.what();

                            result.pipelineMessage =
                                exception.what();

                            return result;
                        }
                        catch (...)
                        {
                            SecureWipe::SanitizationPipelineResult result;

                            result.sanitization.deviceId =
                                targetCopy.getDeviceId();

                            result.sanitization.model =
                                targetCopy.getModel();

                            result.sanitization.serialNumber =
                                targetCopy.getSerialNumber();

                            result.sanitization.interfaceType =
                                targetCopy.getInterfaceType();

                            result.sanitization.capacityBytes =
                                targetCopy.getCapacityBytes();

                            result.sanitization.status =
                                SecureWipe::SanitizationStatus::FAILED;

                            result.sanitization.error =
                                SecureWipe::SanitizationErrorCode::
                                    SANITIZATION_EXECUTION_FAILED;

                            result.sanitization.message =
                                QStringLiteral(
                                    "Unknown sanitization pipeline exception.")
                                    .toStdString();

                            result.sanitization.errorMessage =
                                result.sanitization.message;

                            result.pipelineMessage =
                                result.sanitization.message;

                            return result;
                        }
                    }));
        },
        Qt::SingleShotConnection);
}

void MainWindow::finishSanitization(
    const SecureWipe::SanitizationPipelineResult
        &pipelineResult)
{
    operationRunning_ =
        false;

    refreshDevicesButton_->setEnabled(
        true);

    refreshJobsButton_->setEnabled(
        true);

    jobComboBox_->setEnabled(
        true);

    deviceTable_->setEnabled(
        true);

    validateTargetButton_->setEnabled(
        true);

    operationProgress_->setRange(
        0,
        100);

    operationProgress_->setValue(
        pipelineResult.sanitization.isSuccess()
            ? 100
            : 0);

    updatePipelineUiForResult(
        pipelineResult);

    submitPipelineResult(
        pipelineResult);
}

void MainWindow::submitPipelineResult(
    const SecureWipe::SanitizationPipelineResult
        &pipelineResult)
{
    const QString token =
        authManager_->token();

    if (token.isEmpty())
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "Local pipeline finished, but API authentication is unavailable."));
        jobMessageLabel_->setStyleSheet(
            "color:#B42318;"
            "font-size:12px;"
            "font-weight:600;");
        return;
    }

    resultService_->submitResult(
        token,
        selectedRequestId_,
        pipelineResult);

    if (pipelineResult.sanitization.isSuccess())
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "Physical sanitization and verification succeeded. Uploading evidence..."));
    }
    else
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "Physical sanitization failed. Uploading failure result..."));
    }
}

void MainWindow::updateTargetPanelFromDevice(
    const StorageDevice &device)
{
    targetModelValue_->setText(
        QString::fromStdString(
            device.getModel()));

    targetSerialValue_->setText(
        QString::fromStdString(
            device.getSerialNumber()));

    targetCapacityValue_->setText(
        formatCapacity(
            device.getCapacityBytes()));

    targetInterfaceValue_->setText(
        QString::fromStdString(
            device.getInterfaceType()));

    targetPathValue_->setText(
        QString::fromStdString(
            device.getDeviceId()));

    selectedMethodValue_->setText(
        sanitizationMethodName(
            deviceController_
                ->detectSelectedTargetMethod()));
}

void MainWindow::updatePipelineUiForResult(
    const SecureWipe::SanitizationPipelineResult
        &pipelineResult)
{
    const SecureWipe::SanitizationResult &result =
        pipelineResult.sanitization;

    pipelineStatusValue_->setText(
        statusText(
            result.status));

    pipelineStatusValue_->setStyleSheet(
        badgeStyle(
            result.status ==
                    SecureWipe::SanitizationStatus::COMPLETED
                ? QStringLiteral("COMPLETED")
                : QStringLiteral("FAILED")));

    verificationValue_->setText(
        verificationText(
            result.verificationStatus));

    operationValue_->setText(
        result.operationId.empty()
            ? QStringLiteral("—")
            : QString::fromStdString(
                result.operationId));

    bytesProcessedValue_->setText(
        formatBytes(
            result.bytesProcessed));

    bytesVerifiedValue_->setText(
        formatBytes(
            result.bytesVerified));

    samplesValue_->setText(
        QString::number(
            result.verificationSamples));

    if (!pipelineResult
             .certificate
             .certificateId.empty())
    {
        certificateValue_->setText(
            QString::fromStdString(
                pipelineResult
                    .certificate
                    .certificateId));
    }
    else
    {
        certificateValue_->setText(
            QStringLiteral(
                "Not generated"));
    }

    QString evidence;

    if (!pipelineResult.auditLogPath.empty())
    {
        evidence +=
            QStringLiteral("Audit: ");

        evidence +=
            QString::fromStdString(
                pipelineResult.auditLogPath);
    }

    if (!pipelineResult.operationLogPath.empty())
    {
        if (!evidence.isEmpty())
            evidence += '\n';

        evidence +=
            QStringLiteral(
                "Operation log: ");

        evidence +=
            QString::fromStdString(
                pipelineResult.operationLogPath);
    }

    if (!pipelineResult.certificatePath.empty())
    {
        if (!evidence.isEmpty())
            evidence += '\n';

        evidence +=
            QStringLiteral(
                "Certificate: ");

        evidence +=
            QString::fromStdString(
                pipelineResult.certificatePath);
    }

    evidenceValue_->setText(
        evidence.isEmpty()
            ? QStringLiteral(
                "No evidence path reported.")
            : evidence);

    if (result.isSuccess())
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "Sanitization and verification completed successfully."));
        
        jobMessageLabel_->setStyleSheet(
            "color:#027A48;"
            "font-size:12px;"
            "font-weight:700;");
    }
    else
    {
        QString message =
            result.message.empty()
                ? QStringLiteral(
                      "Sanitization failed.")
                : QString::fromStdString(
                      result.message);

        if (!result.errorMessage.empty())
        {
            message += '\n';

            message +=
                QString::fromStdString(
                    result.errorMessage);
        }

        jobMessageLabel_->setText(
            message);

        jobMessageLabel_->setStyleSheet(
            "color:#B42318;"
            "font-size:12px;"
            "font-weight:600;");
    }
}

void MainWindow::showSelectedDeviceDetails()
{
}

void MainWindow::showDeviceDetails(
    const StorageDevice &device)
{
    Q_UNUSED(device);
}

void MainWindow::logout()
{
    if (operationRunning_)
    {
        QMessageBox::warning(
            this,
            QStringLiteral(
                "Operation in progress"),
            QStringLiteral(
                "You cannot sign out while physical sanitization is running."));
        return;
    }

    emailEdit_->clear();
    passwordEdit_->clear();
    loginErrorLabel_->clear();

    selectedRequestId_.clear();
    selectedRequestDeviceType_.clear();
    selectedRequestMethod_.clear();

    assignedRequests_ =
        QJsonArray();

    resetTargetPanel();

    setConnectionState(
        false,
        QStringLiteral("Signed out"));

    rootStack_->setCurrentWidget(
        loginPage_);
}

QString MainWindow::sanitizationMethodName(
    SanitizationMethod method) const
{
    switch (method)
    {
    case SanitizationMethod::NvmeSanitize:
        return QStringLiteral(
            "NVMe Sanitize");

    case SanitizationMethod::AtaSanitize:
        return QStringLiteral(
            "ATA Sanitize");

    case SanitizationMethod::HostOverwrite:
        return QStringLiteral(
            "Host Overwrite");

    case SanitizationMethod::Unsupported:
    default:
        return QStringLiteral(
            "Unsupported");
    }
}

QString MainWindow::formatCapacity(
    std::uint64_t bytes) const
{
    if (bytes == 0)
        return QStringLiteral(
            "Unknown");

    const double value =
        static_cast<double>(
            bytes);

    const double tb =
        1024.0 *
        1024.0 *
        1024.0 *
        1024.0;

    const double gb =
        1024.0 *
        1024.0 *
        1024.0;

    if (value >= tb)
    {
        return QStringLiteral(
            "%1 TB")
            .arg(
                value / tb,
                0,
                'f',
                2);
    }

    return QStringLiteral(
        "%1 GB")
        .arg(
            value / gb,
            0,
            'f',
            1);
}

QString MainWindow::formatBytes(
    std::uint64_t bytes) const
{
    if (bytes < 1024)
    {
        return QStringLiteral(
            "%1 B")
            .arg(bytes);
    }

    const double value =
        static_cast<double>(
            bytes);

    const double kb =
        1024.0;

    const double mb =
        1024.0 *
        1024.0;

    const double gb =
        1024.0 *
        1024.0 *
        1024.0;

    if (value >= gb)
    {
        return QStringLiteral(
            "%1 GB")
            .arg(
                value / gb,
                0,
                'f',
                2);
    }

    if (value >= mb)
    {
        return QStringLiteral(
            "%1 MB")
            .arg(
                value / mb,
                0,
                'f',
                2);
    }

    return QStringLiteral(
        "%1 KB")
        .arg(
            value / kb,
            0,
            'f',
            2);
}

QString MainWindow::formatDuration(
    std::uint64_t milliseconds) const
{
    if (milliseconds < 1000)
    {
        return QStringLiteral(
            "%1 ms")
            .arg(milliseconds);
    }

    const std::uint64_t seconds =
        milliseconds / 1000;

    const std::uint64_t minutes =
        seconds / 60;

    const std::uint64_t hours =
        minutes / 60;

    if (hours > 0)
    {
        return QStringLiteral(
            "%1h %2m %3s")
            .arg(hours)
            .arg(minutes % 60)
            .arg(seconds % 60);
    }

    if (minutes > 0)
    {
        return QStringLiteral(
            "%1m %2s")
            .arg(minutes)
            .arg(seconds % 60);
    }

    return QStringLiteral(
        "%1.%2 s")
        .arg(seconds)
        .arg(
            (milliseconds % 1000) / 100);
}

QString MainWindow::statusText(
    SecureWipe::SanitizationStatus status) const
{
    switch (status)
    {
    case SecureWipe::SanitizationStatus::IN_PROGRESS:
        return QStringLiteral(
            "IN PROGRESS");

    case SecureWipe::SanitizationStatus::COMPLETED:
        return QStringLiteral(
            "COMPLETED");

    case SecureWipe::SanitizationStatus::FAILED:
        return QStringLiteral(
            "FAILED");

    case SecureWipe::SanitizationStatus::ABORTED:
        return QStringLiteral(
            "ABORTED");

    case SecureWipe::SanitizationStatus::NOT_STARTED:
    default:
        return QStringLiteral(
            "NOT STARTED");
    }
}

QString MainWindow::verificationText(
    SecureWipe::VerificationStatus status) const
{
    switch (status)
    {
    case SecureWipe::VerificationStatus::IN_PROGRESS:
        return QStringLiteral(
            "IN PROGRESS");

    case SecureWipe::VerificationStatus::PASSED:
        return QStringLiteral(
            "PASSED");

    case SecureWipe::VerificationStatus::FAILED:
        return QStringLiteral(
            "FAILED");

    case SecureWipe::VerificationStatus::NOT_PERFORMED:
    default:
        return QStringLiteral(
            "NOT PERFORMED");
    }
}