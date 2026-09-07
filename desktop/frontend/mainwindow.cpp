#include "mainwindow.h"

#include "AuthManager.h"
#include "controllers/DeviceController.h"
#include "models/DeviceTableModel.h"
#include "services/SanitizationRequestService.h"
#include "services/SanitizationResultService.h"
#include "pages/ForensicPage.h"
#include "pages/DeviceDetailsPage.h"

#include "../../backend/classification/include/ClassificationResult.h"
#include "../../backend/classification/include/DeviceClassifier.h"
#include "../../backend/safety/include/SafetyResult.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QIcon>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QToolButton>
#include <QBrush>
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

#include <QtConcurrent/QtConcurrentRun>

#include <exception>
#include <cstddef>

namespace
{

QFrame *makeCard(QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("surfaceCard"));
    card->setStyleSheet(
        "QFrame#surfaceCard {"
        "background:#FFFFFF;"
        "border:1px solid #E4E7EC;"
        "border-radius:16px;"
        "}");
    return card;
}

QLabel *makeTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#101828;"
        "font-size:26px;"
        "font-weight:700;"
        "}");
    return label;
}

QLabel *makeSubtitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:13px;"
        "font-weight:400;"
        "}");
    return label;
}

QLabel *makeCaption(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:10px;"
        "font-weight:700;"
        "}");
    return label;
}

QLabel *makeValue(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setTextInteractionFlags(
        Qt::TextSelectableByMouse);
    label->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#172033;"
        "font-size:13px;"
        "font-weight:600;"
        "}");
    return label;
}

QLabel *makeFeatureTitle(
    const QString &text,
    QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#172033;"
        "font-size:12px;"
        "font-weight:700;"
        "}");
    return label;
}

QLabel *makeFeatureText(
    const QString &text,
    QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:11px;"
        "line-height:17px;"
        "}");
    return label;
}

QPushButton *makePrimaryButton(
    const QString &text,
    QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(44);
    button->setStyleSheet(
        "QPushButton {"
        "background:#2563EB;"
        "color:#FFFFFF;"
        "border:none;"
        "border-radius:10px;"
        "padding:10px 18px;"
        "font-size:12px;"
        "font-weight:700;"
        "}"
        "QPushButton:hover {"
        "background:#1D4ED8;"
        "}"
        "QPushButton:pressed {"
        "background:#1E40AF;"
        "}"
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
    auto *button = new QPushButton(text, parent);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(40);
    button->setStyleSheet(
        "QPushButton {"
        "background:#FFFFFF;"
        "color:#344054;"
        "border:1px solid #D0D5DD;"
        "border-radius:10px;"
        "padding:9px 15px;"
        "font-size:12px;"
        "font-weight:600;"
        "}"
        "QPushButton:hover {"
        "background:#F9FAFB;"
        "border-color:#98A2B3;"
        "}"
        "QPushButton:pressed {"
        "background:#F2F4F7;"
        "}"
        "QPushButton:disabled {"
        "background:#F2F4F7;"
        "color:#98A2B3;"
        "}");
    return button;
}

QPushButton *makeNavButton(
    const QString &text,
    QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(44);
    button->setStyleSheet(
        "QPushButton {"
        "background:transparent;"
        "color:#475467;"
        "border:none;"
        "border-radius:10px;"
        "text-align:left;"
        "padding:0 14px;"
        "font-size:12px;"
        "font-weight:600;"
        "}"
        "QPushButton:hover {"
        "background:#F2F4F7;"
        "}");
    return button;
}

QLabel *makeBadge(
    const QString &text,
    QWidget *parent)
{
    auto *label =
        new QLabel(
            text,
            parent);

    label->setAlignment(
        Qt::AlignCenter);

    label->setMinimumHeight(
        28);

    label->setMinimumWidth(
        96);

    return label;
}

QString badgeStyle(
    const QString &status)
{
    const QString value =
        status.trimmed().toUpper();

    if (value == QStringLiteral("SAFE") ||
        value == QStringLiteral("COMPLETED") ||
        value == QStringLiteral("PASSED") ||
        value == QStringLiteral("CONNECTED"))
    {
        return
            "QLabel {"
            "background:#ECFDF3;"
            "color:#027A48;"
            "border:1px solid #ABEFC6;"
            "border-radius:8px;"
            "padding:5px 10px;"
            "font-size:10px;"
            "font-weight:700;"
            "}";
    }

    if (value == QStringLiteral("FAILED") ||
        value == QStringLiteral("BLOCKED") ||
        value == QStringLiteral("DISCONNECTED") ||
        value == QStringLiteral("UNSUPPORTED"))
    {
        return
            "QLabel {"
            "background:#FEF3F2;"
            "color:#B42318;"
            "border:1px solid #FECDCA;"
            "border-radius:8px;"
            "padding:5px 10px;"
            "font-size:10px;"
            "font-weight:700;"
            "}";
    }

    return
        "QLabel {"
        "background:#EFF6FF;"
        "color:#175CD3;"
        "border:1px solid #B2CCFF;"
        "border-radius:8px;"
        "padding:5px 10px;"
        "font-size:10px;"
        "font-weight:700;"
        "}";
}

QFrame *makeFeatureCard(
    const QString &iconText,
    const QString &title,
    const QString &description,
    QWidget *parent)
{
    auto *card =
        new QFrame(parent);

    card->setObjectName(
        QStringLiteral("loginFeatureCard"));

    card->setStyleSheet(
        "QFrame#loginFeatureCard {"
        "background:#FFFFFF;"
        "border:1px solid #E4E7EC;"
        "border-radius:14px;"
        "}");

    auto *layout =
        new QHBoxLayout(card);

    layout->setContentsMargins(
        14,
        13,
        14,
        13);

    layout->setSpacing(
        12);

    auto *icon =
        new QLabel(
            iconText,
            card);

    icon->setAlignment(
        Qt::AlignCenter);

    icon->setFixedSize(
        38,
        38);

    icon->setStyleSheet(
        "QLabel {"
        "background:#EFF6FF;"
        "border:1px solid #BFDBFE;"
        "border-radius:11px;"
        "color:#2563EB;"
        "font-size:17px;"
        "font-weight:700;"
        "}");

    layout->addWidget(
        icon);

    auto *textLayout =
        new QVBoxLayout;

    textLayout->setSpacing(
        3);

    textLayout->addWidget(
        makeFeatureTitle(
            title,
            card));

    textLayout->addWidget(
        makeFeatureText(
            description,
            card));

    layout->addLayout(
        textLayout,
        1);

    return card;
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
            QStringLiteral("SSD"),
            Qt::CaseInsensitive) == 0)
    {
        return classification.mediaType ==
               MediaType::SSD;
    }

    if (type.compare(
            QStringLiteral("HDD"),
            Qt::CaseInsensitive) == 0)
    {
        return classification.mediaType ==
               MediaType::HDD;
    }

    if (type.compare(
            QStringLiteral("USB Drive"),
            Qt::CaseInsensitive) == 0)
    {
        return classification.busType ==
               BusType::USB;
    }

    if (type.compare(
            QStringLiteral("NVMe SSD"),
            Qt::CaseInsensitive) == 0)
    {
        return classification.busType ==
                   BusType::NVMe &&
               classification.mediaType ==
                   MediaType::SSD;
    }

    return false;
}

void configureTable(
    QTableWidget *table)
{
    if (!table)
        return;

    table->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    table->setSelectionMode(
        QAbstractItemView::SingleSelection);

    table->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->setWordWrap(false);

    table->verticalHeader()->setVisible(
        false);

    table->verticalHeader()
        ->setDefaultSectionSize(46);

    table->horizontalHeader()
        ->setHighlightSections(false);

    table->setFocusPolicy(
        Qt::NoFocus);
}

void addStatCard(
    QHBoxLayout *layout,
    const QString &caption,
    QLabel **value)
{
    auto *card =
        makeCard(
            layout->parentWidget());

    auto *box =
        new QVBoxLayout(card);

    box->setContentsMargins(
        18,
        16,
        18,
        16);

    box->setSpacing(
        6);

    box->addWidget(
        makeCaption(
            caption,
            card));

    *value =
        new QLabel(
            QStringLiteral("0"),
            card);

    (*value)->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#101828;"
        "font-size:25px;"
        "font-weight:700;"
        "}");

    box->addWidget(
        *value);

    layout->addWidget(
        card);
}

}

MainWindow::MainWindow(
    QWidget *parent)
    : QMainWindow(parent)
    , authManager_(
          new AuthManager(this))
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
    , devicesInventoryTable_(nullptr)
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

    resize(
        1420,
        900);

    setMinimumSize(
        1180,
        760);

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

            operatorNameLabel_->setText(
                emailEdit_->text().trimmed());

            QString operatorRole =
                authManager_->role();
            if (operatorRole.isEmpty())
            {
                operatorRoleLabel_->setText(
                    QStringLiteral("UNKNOWN ROLE"));
            }
            else
            {
                operatorRole.replace(
                    QStringLiteral("_"),
                    QStringLiteral(" "));
                operatorRoleLabel_->setText(
                    operatorRole);
            }

            rootStack_->setCurrentWidget(
                appPage_);

            contentStack_->setCurrentWidget(
                dashboardPage_);

            setActiveNav(
                dashboardNavButton_);

            setConnectionState(
                true,
                QStringLiteral("Connected"));

            if (authManager_->role() ==
                QStringLiteral("WORKSTATION_EMPLOYEE"))
            {
                refreshAssignedRequests();
            }
            else
            {
                assignedRequests_ = QJsonArray();
                populateJobDetails();
            }

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
                QStringLiteral(
                    "Authentication failed"));
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

            if (!pattern.match(
                    email).hasMatch())
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

            loginButton_->setEnabled(
                false);

            loginButton_->setText(
                QStringLiteral(
                    "Authenticating..."));

            authManager_->login(
                email,
                password);
        });

    connect(
        requestService_,
        &SanitizationRequestService::
            assignedRequestsFetched,
        this,
        &MainWindow::handleAssignedRequests);

    connect(
        requestService_,
        &SanitizationRequestService::
            requestFetchFailed,
        this,
        [this](const QString &message)
        {
            jobMessageLabel_->setText(
                QStringLiteral(
                    "Unable to load assigned jobs: %1")
                    .arg(message));

            jobMessageLabel_->setStyleSheet(
                "QLabel {"
                "background:transparent;"
                "border:none;"
                "color:#B42318;"
                "font-size:12px;"
                "font-weight:600;"
                "}");

            const bool accessDenied =
                message.contains(
                    QStringLiteral("HTTP 403"),
                    Qt::CaseInsensitive) ||
                message.contains(
                    QStringLiteral("Forbidden"),
                    Qt::CaseInsensitive);

            setConnectionState(
                false,
                accessDenied
                    ? QStringLiteral("API access denied")
                    : QStringLiteral("API unavailable"));
        });

    connect(
        requestService_,
        &SanitizationRequestService::
            requestStatusUpdated,
        this,
        [this](
            const QString &requestId,
            const QString &status)
        {
            if (requestId ==
                selectedRequestId_)
            {
                jobMessageLabel_->setText(
                    QStringLiteral(
                        "Request %1 moved to %2.")
                        .arg(
                            requestId,
                            status));

                jobMessageLabel_->setStyleSheet(
                    "QLabel {"
                    "background:transparent;"
                    "border:none;"
                    "color:#027A48;"
                    "font-size:12px;"
                    "font-weight:600;"
                    "}");
            }

            refreshAssignedRequests();
        });

    connect(
        requestService_,
        &SanitizationRequestService::
            requestStatusUpdateFailed,
        this,
        [this](const QString &message)
        {
            jobMessageLabel_->setText(
                QStringLiteral(
                    "Request status update failed: %1")
                    .arg(message));

            jobMessageLabel_->setStyleSheet(
                "QLabel {"
                "background:transparent;"
                "border:none;"
                "color:#B42318;"
                "font-size:12px;"
                "font-weight:600;"
                "}");
        });

    connect(
        resultService_,
        &SanitizationResultService::
            resultSubmitted,
        this,
        [this](const QString &requestId)
        {
            if (requestId !=
                selectedRequestId_)
            {
                return;
            }

            pipelineStatusValue_->setText(
                QStringLiteral(
                    "VERIFYING"));

            pipelineStatusValue_->setStyleSheet(
                badgeStyle(
                    "VERIFYING"));

            jobMessageLabel_->setText(
                QStringLiteral(
                    "Sanitization result uploaded. Synchronizing certificate..."));
        });

    connect(
        resultService_,
        &SanitizationResultService::
            resultSubmissionFailed,
        this,
        [this](const QString &message)
        {
            jobMessageLabel_->setText(
                QStringLiteral(
                    "Result upload failed: %1")
                    .arg(message));

            jobMessageLabel_->setStyleSheet(
                "QLabel {"
                "background:transparent;"
                "border:none;"
                "color:#B42318;"
                "font-size:12px;"
                "font-weight:600;"
                "}");
        });

    connect(
        resultService_,
        &SanitizationResultService::
            certificateSubmitted,
        this,
        [this](
            const QString &requestId,
            const QString &certificateId)
        {
            if (requestId !=
                selectedRequestId_)
            {
                return;
            }

            certificateValue_->setText(
                certificateId.isEmpty()
                    ? QStringLiteral(
                          "Generated")
                    : certificateId);

            pipelineStatusValue_->setText(
                QStringLiteral(
                    "COMPLETED"));

            pipelineStatusValue_->setStyleSheet(
                badgeStyle(
                    "COMPLETED"));

            jobMessageLabel_->setText(
                QStringLiteral(
                    "Certificate synchronized successfully. Sanitization request completed."));

            jobMessageLabel_->setStyleSheet(
                "QLabel {"
                "background:transparent;"
                "border:none;"
                "color:#027A48;"
                "font-size:12px;"
                "font-weight:700;"
                "}");

            refreshAssignedRequests();
        });

    connect(
        resultService_,
        &SanitizationResultService::
            certificateSubmissionFailed,
        this,
        [this](const QString &message)
        {
            jobMessageLabel_->setText(
                QStringLiteral(
                    "Certificate upload failed: %1")
                    .arg(message));

            jobMessageLabel_->setStyleSheet(
                "QLabel {"
                "background:transparent;"
                "border:none;"
                "color:#B42318;"
                "font-size:12px;"
                "font-weight:600;"
                "}");
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
                "QLabel {"
                "background:transparent;"
                "border:none;"
                "color:#B42318;"
                "font-size:12px;"
                "font-weight:600;"
                "}");
        });

    connect(
        jobComboBox_,
        QOverload<int>::of(
            &QComboBox::currentIndexChanged),
        this,
        &MainWindow::selectRequestFromJobs);

    connect(
        assignedJobsTable_,
        &QTableWidget::
            itemSelectionChanged,
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
            {
                jobComboBox_->setCurrentIndex(
                    index);
            }
        });

    connect(
        deviceTable_,
        &QTableWidget::
            itemSelectionChanged,
        this,
        [this]()
        {
            selectTargetDevice(
                deviceTable_->currentRow());
        });

    connect(
        devicesInventoryTable_,
        &QTableWidget::
            cellDoubleClicked,
        this,
        [this](
            int row,
            int column)
        {
            Q_UNUSED(column);

            if (!devicesInventoryTable_ ||
                row < 0 ||
                row >= devicesInventoryTable_->rowCount())
            {
                return;
            }

            devicesInventoryTable_->selectRow(row);
            showSelectedDeviceDetails();
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
    root_ =
        new QWidget(this);

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
        0,
        0,
        0,
        0);

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
        new QHBoxLayout(
            loginPage_);

    outer->setContentsMargins(
        64,
        50,
        64,
        50);

    outer->setSpacing(
        54);

    auto *left =
        new QWidget(
            loginPage_);

    auto *leftLayout =
        new QVBoxLayout(
            left);

    leftLayout->setContentsMargins(
        18,
        22,
        12,
        22);

    leftLayout->setSpacing(
        16);

    auto *brandRow =
        new QHBoxLayout;

    brandRow->setSpacing(
        12);

    auto *logoBox =
        new QFrame(left);

    logoBox->setFixedSize(
        56,
        56);

    logoBox->setObjectName(
        QStringLiteral(
            "brandLogo"));

    logoBox->setStyleSheet(
        "QFrame#brandLogo {"
        "background:#EFF6FF;"
        "border:1px solid #BFDBFE;"
        "border-radius:16px;"
        "}");

    auto *logoLayout =
        new QVBoxLayout(
            logoBox);

    logoLayout->setContentsMargins(
        0,
        0,
        0,
        0);

    auto *logo =
        new QLabel(
            QStringLiteral(
                "◈"),
            logoBox);

    logo->setAlignment(
        Qt::AlignCenter);

    logo->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#2563EB;"
        "font-size:25px;"
        "font-weight:700;"
        "}");

    logoLayout->addWidget(
        logo);

    brandRow->addWidget(
        logoBox);

    auto *brandBlock =
        new QVBoxLayout;

    brandBlock->setSpacing(
        2);

    auto *brand =
        new QLabel(
            QStringLiteral(
                "SecureWipe"),
            left);

    brand->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#101828;"
        "font-size:27px;"
        "font-weight:700;"
        "}");

    auto *brandSub =
        new QLabel(
            QStringLiteral(
                "Secure storage lifecycle"),
            left);

    brandSub->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:11px;"
        "font-weight:600;"
        "}");

    brandBlock->addWidget(
        brand);

    brandBlock->addWidget(
        brandSub);

    brandRow->addLayout(
        brandBlock);

    brandRow->addStretch();

    leftLayout->addLayout(
        brandRow);

    auto *headline =
        new QLabel(
            QStringLiteral(
                "Secure sanitization\nand forensic recovery."),
            left);

    headline->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#101828;"
        "font-size:31px;"
        "font-weight:700;"
        "}");

    leftLayout->addWidget(
        headline);

    auto *description =
        new QLabel(
            QStringLiteral(
                "A controlled workstation console for physical storage sanitization, forensic acquisition, evidence recovery and auditable results."),
            left);

    description->setWordWrap(
        true);

    description->setMaximumWidth(
        610);

    description->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:13px;"
        "line-height:20px;"
        "}");

    leftLayout->addWidget(
        description);

    leftLayout->addSpacing(
        6);

    leftLayout->addWidget(
        makeFeatureCard(
            QStringLiteral("✓"),
            QStringLiteral(
                "Secure Sanitization"),
            QStringLiteral(
                "NVMe Sanitize, ATA Sanitize and controlled host overwrite with target validation and verification."),
            left));

    leftLayout->addWidget(
        makeFeatureCard(
            QStringLiteral("⌕"),
            QStringLiteral(
                "Forensic Recovery"),
            QStringLiteral(
                "Read-only physical or forensic-image acquisition with validated artifact recovery and evidence inspection."),
            left));

    leftLayout->addWidget(
        makeFeatureCard(
            QStringLiteral("#"),
            QStringLiteral(
                "Evidence & Audit"),
            QStringLiteral(
                "Certificates, hashes, operation records and audit evidence remain tied to the executed pipeline."),
            left));

    auto *workflow =
        makeCard(left);

    auto *workflowLayout =
        new QVBoxLayout(
            workflow);

    workflowLayout->setContentsMargins(
        16,
        14,
        16,
        14);

    workflowLayout->setSpacing(
        7);

    workflowLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "CONTROLLED WORKFLOW"),
            workflow));

    auto *workflowText =
        new QLabel(
            QStringLiteral(
                "Authorize → acquire / sanitize → verify → preserve evidence"),
            workflow);

    workflowText->setWordWrap(
        true);

    workflowText->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#344054;"
        "font-size:11px;"
        "font-weight:600;"
        "}");

    workflowLayout->addWidget(
        workflowText);

    leftLayout->addWidget(
        workflow);

    leftLayout->addStretch();

    auto *right =
        new QFrame(
            loginPage_);

    right->setObjectName(
        QStringLiteral(
            "loginCard"));

    right->setMinimumWidth(
        390);

    right->setMaximumWidth(
        430);

    right->setSizePolicy(
        QSizePolicy::Preferred,
        QSizePolicy::Fixed);

    right->setStyleSheet(
        "QFrame#loginCard {"
        "background:#FFFFFF;"
        "border:1px solid #DCE3EC;"
        "border-radius:22px;"
        "}");

    auto *cardLayout =
        new QVBoxLayout(
            right);

    cardLayout->setContentsMargins(
        34,
        32,
        34,
        30);

    cardLayout->setSpacing(
        13);

    auto *secureBadge =
        new QLabel(
            QStringLiteral(
                "WORKSTATION AUTHENTICATION"),
            right);

    secureBadge->setAlignment(
        Qt::AlignCenter);

    secureBadge->setStyleSheet(
        "QLabel {"
        "background:#EFF6FF;"
        "border:1px solid #BFDBFE;"
        "border-radius:7px;"
        "padding:6px 9px;"
        "color:#175CD3;"
        "font-size:9px;"
        "font-weight:700;"
        "}");

    cardLayout->addWidget(
        secureBadge,
        0,
        Qt::AlignLeft);

    auto *title =
        new QLabel(
            QStringLiteral(
                "Welcome back"),
            right);

    title->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#101828;"
        "font-size:24px;"
        "font-weight:700;"
        "}");

    cardLayout->addWidget(
        title);

    auto *sub =
        new QLabel(
            QStringLiteral(
                "Sign in to access your authorized SecureWipe workstation."),
            right);

    sub->setWordWrap(
        true);

    sub->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:12px;"
        "}");

    cardLayout->addWidget(
        sub);

    cardLayout->addSpacing(
        8);

    auto *emailCaption =
        makeCaption(
            QStringLiteral(
                "WORK EMAIL"),
            right);

    emailCaption->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#475467;"
        "font-size:10px;"
        "font-weight:700;"
        "}");

    cardLayout->addWidget(
        emailCaption);

    emailEdit_ =
        new QLineEdit(
            right);

    emailEdit_->setPlaceholderText(
        QStringLiteral(
            "name@securewipe.com"));

    emailEdit_->setMinimumHeight(
        48);

    emailEdit_->setStyleSheet(
        "QLineEdit {"
        "background:#FFFFFF;"
        "border:1px solid #D0D5DD;"
        "border-radius:10px;"
        "padding:11px 13px;"
        "color:#172033;"
        "font-size:12px;"
        "}"
        "QLineEdit:focus {"
        "border:1px solid #2563EB;"
        "background:#FFFFFF;"
        "}");

    cardLayout->addWidget(
        emailEdit_);

    auto *passwordCaption =
        makeCaption(
            QStringLiteral(
                "PASSWORD"),
            right);

    passwordCaption->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#475467;"
        "font-size:10px;"
        "font-weight:700;"
        "}");

    cardLayout->addWidget(
        passwordCaption);

    auto *passwordRow =
        new QWidget(
            right);

    auto *passwordLayout =
        new QHBoxLayout(
            passwordRow);

    passwordLayout->setContentsMargins(
        0,
        0,
        0,
        0);

    passwordLayout->setSpacing(
        0);

    passwordEdit_ =
        new QLineEdit(
            passwordRow);

    passwordEdit_->setEchoMode(
        QLineEdit::Password);

    passwordEdit_->setPlaceholderText(
        QStringLiteral(
            "Enter your password"));

    passwordEdit_->setMinimumHeight(
        48);

    passwordEdit_->setStyleSheet(
        "QLineEdit {"
        "background:#FFFFFF;"
        "border:1px solid #D0D5DD;"
        "border-right:none;"
        "border-radius:10px 0 0 10px;"
        "padding:11px 13px;"
        "color:#172033;"
        "font-size:12px;"
        "}"
        "QLineEdit:focus {"
        "border:1px solid #2563EB;"
        "border-right:none;"
        "background:#FFFFFF;"
        "}");

    auto *passwordVisibilityButton =
        new QToolButton(
            passwordRow);

    passwordVisibilityButton->setFixedSize(
        48,
        48);

    passwordVisibilityButton->setCursor(
        Qt::PointingHandCursor);

    passwordVisibilityButton->setToolTip(
        QStringLiteral(
            "Show password"));

    passwordVisibilityButton->setStyleSheet(
        "QToolButton {"
        "background:#FFFFFF;"
        "border:1px solid #D0D5DD;"
        "border-left:none;"
        "border-radius:0 10px 10px 0;"
        "padding:0;"
        "}"
        "QToolButton:hover {"
        "background:#F8FAFC;"
        "}");

    const auto makeEyeIcon = [](
        bool visible)
    {
        QPixmap pixmap(
            22,
            22);
        pixmap.fill(
            Qt::transparent);

        QPainter painter(
            &pixmap);
        painter.setRenderHint(
            QPainter::Antialiasing,
            true);

        QPen pen(
            QColor(
                "#667085"));
        pen.setWidth(2);
        pen.setCapStyle(
            Qt::RoundCap);
        painter.setPen(
            pen);
        painter.setBrush(
            Qt::NoBrush);

        QRectF eye(
            2.0,
            6.0,
            18.0,
            10.0);
        painter.drawEllipse(
            eye);

        painter.setBrush(
            QColor(
                "#667085"));
        painter.drawEllipse(
            QRectF(
                8.0,
                9.0,
                6.0,
                6.0));

        if (!visible)
        {
            painter.setPen(
                pen);
            painter.drawLine(
                QPointF(
                    3.0,
                    3.0),
                QPointF(
                    19.0,
                    19.0));
        }

        return QIcon(
            pixmap);
    };

    passwordVisibilityButton->setIcon(
        makeEyeIcon(
            false));

    passwordVisibilityButton->setIconSize(
        QSize(22, 22));

    connect(
        passwordVisibilityButton,
        &QToolButton::clicked,
        right,
        [this, passwordVisibilityButton, makeEyeIcon]() mutable
        {
            const bool showPassword =
                passwordEdit_->echoMode() ==
                QLineEdit::Password;

            passwordEdit_->setEchoMode(
                showPassword
                    ? QLineEdit::Normal
                    : QLineEdit::Password);

            passwordVisibilityButton->setIcon(
                makeEyeIcon(
                    showPassword));

            passwordVisibilityButton->setToolTip(
                showPassword
                    ? QStringLiteral(
                          "Hide password")
                    : QStringLiteral(
                          "Show password"));

            passwordEdit_->setFocus();
        });

    passwordLayout->addWidget(
        passwordEdit_,
        1);

    passwordLayout->addWidget(
        passwordVisibilityButton);

    cardLayout->addWidget(
        passwordRow);

    loginErrorLabel_ =
        new QLabel(
            right);

    loginErrorLabel_->setWordWrap(
        true);

    loginErrorLabel_->setMinimumHeight(
        22);

    loginErrorLabel_->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#B42318;"
        "font-size:11px;"
        "font-weight:600;"
        "}");

    cardLayout->addWidget(
        loginErrorLabel_);

    loginButton_ =
        makePrimaryButton(
            QStringLiteral(
                "Sign in"),
            right);

    loginButton_->setMinimumHeight(
        50);

    cardLayout->addWidget(
        loginButton_);

    cardLayout->addSpacing(
        4);

    auto *securityRow =
        new QHBoxLayout;

    securityRow->setSpacing(
        8);

    auto *dot =
        new QLabel(
            QStringLiteral(
                "●"),
            right);

    dot->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#12B76A;"
        "font-size:8px;"
        "}");

    auto *securityText =
        new QLabel(
            QStringLiteral(
                "Protected workstation • Authorized operators only"),
            right);

    securityText->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#98A2B3;"
        "font-size:10px;"
        "}");

    securityRow->addWidget(
        dot,
        0,
        Qt::AlignVCenter);

    securityRow->addWidget(
        securityText);

    securityRow->addStretch();

    cardLayout->addLayout(
        securityRow);

    outer->addWidget(
        left,
        1);

    outer->addWidget(
        right,
        0,
        Qt::AlignVCenter);
}

void MainWindow::buildAppShell()
{
    appPage_ =
        new QWidget;

    auto *mainLayout =
        new QHBoxLayout(
            appPage_);

    mainLayout->setContentsMargins(
        0,
        0,
        0,
        0);

    mainLayout->setSpacing(
        0);

    auto *sidebar =
        new QFrame(
            appPage_);

    sidebar->setFixedWidth(
        248);

    sidebar->setStyleSheet(
        "QFrame {"
        "background:#FFFFFF;"
        "border-right:1px solid #EAECF0;"
        "}");

    auto *sidebarLayout =
        new QVBoxLayout(
            sidebar);

    sidebarLayout->setContentsMargins(
        18,
        22,
        18,
        18);

    sidebarLayout->setSpacing(
        7);

    auto *brandRow =
        new QHBoxLayout;

    brandRow->setSpacing(
        10);

    auto *brandIcon =
        new QLabel(
            QStringLiteral("◈"),
            sidebar);

    brandIcon->setAlignment(
        Qt::AlignCenter);

    brandIcon->setFixedSize(
        34,
        34);

    brandIcon->setStyleSheet(
        "QLabel {"
        "background:#EFF6FF;"
        "color:#2563EB;"
        "border:1px solid #BFDBFE;"
        "border-radius:10px;"
        "font-size:18px;"
        "font-weight:700;"
        "}");

    auto *brand =
        new QLabel(
            QStringLiteral(
                "SecureWipe"),
            sidebar);

    brand->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#101828;"
        "font-size:17px;"
        "font-weight:700;"
        "}");

    brandRow->addWidget(
        brandIcon);

    brandRow->addWidget(
        brand);

    brandRow->addStretch();

    sidebarLayout->addLayout(
        brandRow);

    sidebarLayout->addSpacing(
        18);

    dashboardNavButton_ =
        makeNavButton(
            QStringLiteral(
                "Overview"),
            sidebar);

    jobsNavButton_ =
        makeNavButton(
            QStringLiteral(
                "Assigned Jobs"),
            sidebar);

    devicesNavButton_ =
        makeNavButton(
            QStringLiteral(
                "Physical Devices"),
            sidebar);

    forensicsNavButton_ =
        makeNavButton(
            QStringLiteral(
                "Forensic Recovery"),
            sidebar);

    settingsNavButton_ =
        makeNavButton(
            QStringLiteral(
                "Settings"),
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
        13,
        13,
        13,
        13);

    operatorLayout->setSpacing(
        4);

    operatorLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "CURRENT OPERATOR"),
            operatorCard));

    operatorNameLabel_ =
        new QLabel(
            QStringLiteral(
                "Operator"),
            operatorCard);

    operatorNameLabel_->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#172033;"
        "font-size:12px;"
        "font-weight:700;"
        "}");

    operatorRoleLabel_ =
        new QLabel(
            QStringLiteral(
                "NOT SIGNED IN"),
            operatorCard);

    operatorRoleLabel_->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:10px;"
        "}");

    operatorLayout->addWidget(
        operatorNameLabel_);

    operatorLayout->addWidget(
        operatorRoleLabel_);

    sidebarLayout->addWidget(
        operatorCard);

    sidebarLayout->addSpacing(
        9);

    logoutButton_ =
        new QPushButton(
            QStringLiteral(
                "Sign out"),
            sidebar);

    logoutButton_->setCursor(
        Qt::PointingHandCursor);

    logoutButton_->setMinimumHeight(
        40);

    logoutButton_->setStyleSheet(
        "QPushButton {"
        "background:#FFF5F5;"
        "color:#B42318;"
        "border:1px solid #FECACA;"
        "border-radius:10px;"
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
        new QWidget(
            appPage_);

    auto *rightLayout =
        new QVBoxLayout(
            right);

    rightLayout->setContentsMargins(
        0,
        0,
        0,
        0);

    rightLayout->setSpacing(
        0);

    auto *topbar =
        new QFrame(
            right);

    topbar->setFixedHeight(
        72);

    topbar->setStyleSheet(
        "QFrame {"
        "background:#FFFFFF;"
        "border-bottom:1px solid #EAECF0;"
        "}");

    auto *topLayout =
        new QHBoxLayout(
            topbar);

    topLayout->setContentsMargins(
        28,
        0,
        28,
        0);

    auto *console =
        new QLabel(
            QStringLiteral(
                "Workstation Console"),
            topbar);

    console->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#344054;"
        "font-size:13px;"
        "font-weight:600;"
        "}");

    topLayout->addWidget(
        console);

    topLayout->addStretch();

    connectionBadgeLabel_ =
        makeBadge(
            QStringLiteral(
                "Disconnected"),
            topbar);

    connectionBadgeLabel_->setStyleSheet(
        badgeStyle(
            "DISCONNECTED"));

    topLayout->addWidget(
        connectionBadgeLabel_);

    rightLayout->addWidget(
        topbar);

    contentStack_ =
        new QStackedWidget(
            right);

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

    scroll->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);

    auto *page =
        new QWidget;

    auto *root =
        new QVBoxLayout(page);

    root->setContentsMargins(
        30,
        28,
        30,
        30);

    root->setSpacing(
        18);

    root->addWidget(
        makeTitle(
            QStringLiteral(
                "Overview"),
            page));

    root->addWidget(
        makeSubtitle(
            QStringLiteral(
                "A live view of assigned sanitization work, physical target readiness and forensic capabilities."),
            page));

    auto *metrics =
        new QHBoxLayout;

    metrics->setSpacing(
        14);

    addStatCard(
        metrics,
        QStringLiteral(
            "TOTAL JOBS"),
        &totalJobsValue_);

    addStatCard(
        metrics,
        QStringLiteral(
            "ACTIVE"),
        &activeJobsValue_);

    addStatCard(
        metrics,
        QStringLiteral(
            "COMPLETED"),
        &completedJobsValue_);

    addStatCard(
        metrics,
        QStringLiteral(
            "FAILED"),
        &failedJobsValue_);

    root->addLayout(
        metrics);

    auto *capabilityRow =
        new QHBoxLayout;

    capabilityRow->setSpacing(
        14);

    auto *sanitizeCard =
        makeCard(page);

    auto *sanitizeLayout =
        new QVBoxLayout(
            sanitizeCard);

    sanitizeLayout->setContentsMargins(
        18,
        18,
        18,
        18);

    sanitizeLayout->setSpacing(
        7);

    sanitizeLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "SANITIZATION"),
            sanitizeCard));

    auto *sanitizeTitle =
        new QLabel(
            QStringLiteral(
                "Physical storage sanitization"),
            sanitizeCard);

    sanitizeTitle->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#172033;"
        "font-size:15px;"
        "font-weight:700;"
        "}");

    sanitizeLayout->addWidget(
        sanitizeTitle);

    auto *sanitizeText =
        new QLabel(
            QStringLiteral(
                "Assigned work orders are executed against an exact physical target only after fresh validation and safety checks."),
            sanitizeCard);

    sanitizeText->setWordWrap(
        true);

    sanitizeText->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:11px;"
        "}");

    sanitizeLayout->addWidget(
        sanitizeText);

    capabilityRow->addWidget(
        sanitizeCard,
        1);

    auto *forensicCard =
        makeCard(page);

    auto *forensicLayout =
        new QVBoxLayout(
            forensicCard);

    forensicLayout->setContentsMargins(
        18,
        18,
        18,
        18);

    forensicLayout->setSpacing(
        7);

    forensicLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "FORENSIC RECOVERY"),
            forensicCard));

    auto *forensicTitle =
        new QLabel(
            QStringLiteral(
                "Read-only acquisition and recovery"),
            forensicCard);

    forensicTitle->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#172033;"
        "font-size:15px;"
        "font-weight:700;"
        "}");

    forensicLayout->addWidget(
        forensicTitle);

    auto *forensicText =
        new QLabel(
            QStringLiteral(
                "Acquire from a physical storage device or forensic image and inspect validated recovered artifacts."),
            forensicCard);

    forensicText->setWordWrap(
        true);

    forensicText->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:11px;"
        "}");

    forensicLayout->addWidget(
        forensicText);

    capabilityRow->addWidget(
        forensicCard,
        1);

    root->addLayout(
        capabilityRow);

    auto *jobsCard =
        makeCard(page);

    auto *jobsLayout =
        new QVBoxLayout(
            jobsCard);

    jobsLayout->setContentsMargins(
        18,
        18,
        18,
        18);

    jobsLayout->setSpacing(
        10);

    auto *jobsHeader =
        new QHBoxLayout;

    jobsHeader->addWidget(
        makeCaption(
            QStringLiteral(
                "RECENT ASSIGNED JOBS"),
            jobsCard));

    jobsHeader->addStretch();

    auto *openJobs =
        makeSecondaryButton(
            QStringLiteral(
                "Open assigned jobs"),
            jobsCard);

    connect(
        openJobs,
        &QPushButton::clicked,
        this,
        [this]()
        {
            showPage(
                jobsPage_,
                jobsNavButton_);
        });

    jobsHeader->addWidget(
        openJobs);

    jobsLayout->addLayout(
        jobsHeader);

    dashboardJobsTable_ =
        new QTableWidget(
            jobsCard);

    dashboardJobsTable_->setColumnCount(
        4);

    dashboardJobsTable_->setHorizontalHeaderLabels(
        {
            QStringLiteral(
                "Request"),
            QStringLiteral(
                "Device"),
            QStringLiteral(
                "Method"),
            QStringLiteral(
                "Status")
        });

    configureTable(
        dashboardJobsTable_);

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
        0,
        0,
        0,
        0);

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

    scroll->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);

    auto *page =
        new QWidget;

    auto *root =
        new QVBoxLayout(page);

    root->setContentsMargins(
        30,
        28,
        30,
        30);

    root->setSpacing(
        16);

    auto *header =
        new QHBoxLayout;

    auto *titleBlock =
        new QVBoxLayout;

    titleBlock->setSpacing(
        4);

    titleBlock->addWidget(
        makeTitle(
            QStringLiteral(
                "Assigned Sanitization Jobs"),
            page));

    titleBlock->addWidget(
        makeSubtitle(
            QStringLiteral(
                "Requests authorized for physical execution at this workstation."),
            page));

    header->addLayout(
        titleBlock);

    header->addStretch();

    refreshJobsButton_ =
        makeSecondaryButton(
            QStringLiteral(
                "Refresh jobs"),
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
        16,
        16,
        16,
        16);

    assignedJobsTable_ =
        new QTableWidget(
            tableCard);

    assignedJobsTable_->setColumnCount(
        5);

    assignedJobsTable_->setHorizontalHeaderLabels(
        {
            QStringLiteral(
                "Request ID"),
            QStringLiteral(
                "Device Type"),
            QStringLiteral(
                "Method"),
            QStringLiteral(
                "Asset"),
            QStringLiteral(
                "Status")
        });

    configureTable(
        assignedJobsTable_);

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
        18,
        18,
        18,
        18);

    detailLayout->setSpacing(
        12);

    detailLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "SELECTED WORK ORDER"),
            detailCard));

    jobComboBox_ =
        new QComboBox(
            detailCard);

    jobComboBox_->setMinimumHeight(
        42);

    detailLayout->addWidget(
        jobComboBox_);

    auto *jobGrid =
        new QGridLayout;

    jobGrid->setHorizontalSpacing(
        22);

    jobGrid->setVerticalSpacing(
        12);

    jobRequestIdValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            detailCard);

    jobDeviceTypeValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            detailCard);

    jobRequestedMethodValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            detailCard);

    jobAssetValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            detailCard);

    jobWorkstationValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            detailCard);

    jobGrid->addWidget(
        makeCaption(
            QStringLiteral(
                "Request ID"),
            detailCard),
        0,
        0);

    jobGrid->addWidget(
        jobRequestIdValue_,
        1,
        0);

    jobGrid->addWidget(
        makeCaption(
            QStringLiteral(
                "Device type"),
            detailCard),
        0,
        1);

    jobGrid->addWidget(
        jobDeviceTypeValue_,
        1,
        1);

    jobGrid->addWidget(
        makeCaption(
            QStringLiteral(
                "Assigned method"),
            detailCard),
        0,
        2);

    jobGrid->addWidget(
        jobRequestedMethodValue_,
        1,
        2);

    jobGrid->addWidget(
        makeCaption(
            QStringLiteral(
                "Asset"),
            detailCard),
        2,
        0);

    jobGrid->addWidget(
        jobAssetValue_,
        3,
        0);

    jobGrid->addWidget(
        makeCaption(
            QStringLiteral(
                "Workstation"),
            detailCard),
        2,
        1);

    jobGrid->addWidget(
        jobWorkstationValue_,
        3,
        1);

    detailLayout->addLayout(
        jobGrid);

    root->addWidget(
        detailCard);

    auto *targetCard =
        makeCard(page);

    auto *targetLayout =
        new QVBoxLayout(
            targetCard);

    targetLayout->setContentsMargins(
        18,
        18,
        18,
        18);

    targetLayout->setSpacing(
        11);

    auto *targetHeader =
        new QHBoxLayout;

    auto *targetTitleBlock =
        new QVBoxLayout;

    targetTitleBlock->setSpacing(
        3);

    targetTitleBlock->addWidget(
        makeCaption(
            QStringLiteral(
                "PHYSICAL TARGET"),
            targetCard));

    targetTitleBlock->addWidget(
        makeSubtitle(
            QStringLiteral(
                "Only compatible physical devices are shown for the selected request."),
            targetCard));

    targetHeader->addLayout(
        targetTitleBlock);

    targetHeader->addStretch();

    refreshDevicesButton_ =
        makeSecondaryButton(
            QStringLiteral(
                "Refresh devices"),
            targetCard);

    targetHeader->addWidget(
        refreshDevicesButton_);

    targetLayout->addLayout(
        targetHeader);

    deviceTable_ =
        new QTableWidget(
            targetCard);

    deviceTable_->setColumnCount(
        6);

    deviceTable_->setHorizontalHeaderLabels(
        {
            QStringLiteral(
                "Model"),
            QStringLiteral(
                "Serial"),
            QStringLiteral(
                "Interface"),
            QStringLiteral(
                "Capacity"),
            QStringLiteral(
                "System"),
            QStringLiteral(
                "Device ID")
        });

    configureTable(
        deviceTable_);

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
        18,
        18,
        18,
        18);

    safetyLayout->setSpacing(
        9);

    auto *safetyHeader =
        new QHBoxLayout;

    auto *safetyBlock =
        new QVBoxLayout;

    safetyBlock->setSpacing(
        3);

    safetyBlock->addWidget(
        makeCaption(
            QStringLiteral(
                "TARGET SAFETY"),
            safetyCard));

    safetyBlock->addWidget(
        makeSubtitle(
            QStringLiteral(
                "Fresh validation is required before a destructive operation."),
            safetyCard));

    safetyHeader->addLayout(
        safetyBlock);

    safetyHeader->addStretch();

    targetSafetyBadge_ =
        makeBadge(
            QStringLiteral(
                "NOT CHECKED"),
            safetyCard);

    targetSafetyBadge_->setStyleSheet(
        badgeStyle(
            "NOT_CHECKED"));

    safetyHeader->addWidget(
        targetSafetyBadge_);

    safetyLayout->addLayout(
        safetyHeader);

    targetSafetyText_ =
        new QLabel(
            QStringLiteral(
                "Select an exact physical target."),
            safetyCard);

    targetSafetyText_->setWordWrap(
        true);

    targetSafetyText_->setMinimumHeight(
        56);

    targetSafetyText_->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:11px;"
        "}");

    safetyLayout->addWidget(
        targetSafetyText_);

    auto *targetGrid =
        new QGridLayout;

    targetGrid->setHorizontalSpacing(
        18);

    targetGrid->setVerticalSpacing(
        9);

    targetModelValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            safetyCard);

    targetSerialValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            safetyCard);

    targetCapacityValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            safetyCard);

    targetInterfaceValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            safetyCard);

    targetPathValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            safetyCard);

    targetGrid->addWidget(
        makeCaption(
            QStringLiteral(
                "Model"),
            safetyCard),
        0,
        0);

    targetGrid->addWidget(
        targetModelValue_,
        1,
        0);

    targetGrid->addWidget(
        makeCaption(
            QStringLiteral(
                "Serial"),
            safetyCard),
        0,
        1);

    targetGrid->addWidget(
        targetSerialValue_,
        1,
        1);

    targetGrid->addWidget(
        makeCaption(
            QStringLiteral(
                "Capacity"),
            safetyCard),
        2,
        0);

    targetGrid->addWidget(
        targetCapacityValue_,
        3,
        0);

    targetGrid->addWidget(
        makeCaption(
            QStringLiteral(
                "Interface"),
            safetyCard),
        2,
        1);

    targetGrid->addWidget(
        targetInterfaceValue_,
        3,
        1);

    targetGrid->addWidget(
        makeCaption(
            QStringLiteral(
                "Physical device"),
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
            QStringLiteral(
                "—"),
            safetyCard);

    selectedMethodValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            safetyCard);

    safetyLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Capability"),
            safetyCard));

    safetyLayout->addWidget(
        capabilityValue_);

    safetyLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Detected method"),
            safetyCard));

    safetyLayout->addWidget(
        selectedMethodValue_);

    workspace->addWidget(
        safetyCard,
        1);

    auto *executionCard =
        makeCard(page);

    auto *executionLayout =
        new QVBoxLayout(
            executionCard);

    executionLayout->setContentsMargins(
        18,
        18,
        18,
        18);

    executionLayout->setSpacing(
        8);

    auto *executionHeader =
        new QHBoxLayout;

    auto *executionBlock =
        new QVBoxLayout;

    executionBlock->setSpacing(
        3);

    executionBlock->addWidget(
        makeCaption(
            QStringLiteral(
                "SANITIZATION PIPELINE"),
            executionCard));

    executionBlock->addWidget(
        makeSubtitle(
            QStringLiteral(
                "Physical execution, verification and evidence state."),
            executionCard));

    executionHeader->addLayout(
        executionBlock);

    executionHeader->addStretch();

    pipelineStatusValue_ =
        makeBadge(
            QStringLiteral(
                "NOT STARTED"),
            executionCard);

    pipelineStatusValue_->setStyleSheet(
        badgeStyle(
            "NOT_STARTED"));

    executionHeader->addWidget(
        pipelineStatusValue_);

    executionLayout->addLayout(
        executionHeader);

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
            QStringLiteral(
                "—"),
            executionCard);

    operationValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            executionCard);

    bytesProcessedValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            executionCard);

    bytesVerifiedValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            executionCard);

    samplesValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            executionCard);

    certificateValue_ =
        makeValue(
            QStringLiteral(
                "—"),
            executionCard);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Verification"),
            executionCard));

    executionLayout->addWidget(
        verificationValue_);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Operation ID"),
            executionCard));

    executionLayout->addWidget(
        operationValue_);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Bytes processed"),
            executionCard));

    executionLayout->addWidget(
        bytesProcessedValue_);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Bytes verified"),
            executionCard));

    executionLayout->addWidget(
        bytesVerifiedValue_);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Samples"),
            executionCard));

    executionLayout->addWidget(
        samplesValue_);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "Certificate"),
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
        18,
        16,
        18,
        16);

    actionLayout->setSpacing(
        12);

    jobMessageLabel_ =
        new QLabel(
            QStringLiteral(
                "Select an assigned request to begin."),
            actionCard);

    jobMessageLabel_->setWordWrap(
        true);

    jobMessageLabel_->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:12px;"
        "font-weight:600;"
        "}");

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

    auto *outer =
        new QVBoxLayout(
            jobsPage_);

    outer->setContentsMargins(
        0,
        0,
        0,
        0);

    outer->addWidget(
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

    scroll->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);

    auto *page =
        new QWidget;

    auto *root =
        new QVBoxLayout(
            page);

    root->setContentsMargins(
        30,
        28,
        30,
        30);

    root->setSpacing(
        16);

    auto *header =
        new QHBoxLayout;

    auto *titleBlock =
        new QVBoxLayout;

    titleBlock->setSpacing(
        4);

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
            QStringLiteral(
                "Refresh"),
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

    auto *intro =
        makeCard(page);

    auto *introLayout =
        new QHBoxLayout(
            intro);

    introLayout->setContentsMargins(
        18,
        16,
        18,
        16);

    introLayout->setSpacing(
        14);

    auto *icon =
        new QLabel(
            QStringLiteral(
                "◉"),
            intro);

    icon->setAlignment(
        Qt::AlignCenter);

    icon->setFixedSize(
        42,
        42);

    icon->setStyleSheet(
        "QLabel {"
        "background:#EFF6FF;"
        "color:#2563EB;"
        "border:1px solid #BFDBFE;"
        "border-radius:12px;"
        "font-size:18px;"
        "font-weight:700;"
        "}");

    introLayout->addWidget(
        icon);

    auto *introText =
        new QVBoxLayout;

    introText->setSpacing(
        3);

    introText->addWidget(
        makeFeatureTitle(
            QStringLiteral(
                "Physical inventory"),
            intro));

    introText->addWidget(
        makeFeatureText(
            QStringLiteral(
                "Every physical storage device returned by the desktop discovery layer is shown here. System disks remain visibly blocked."),
            intro));

    introLayout->addLayout(
        introText,
        1);

    root->addWidget(
        intro);

    auto *tableCard =
        makeCard(page);

    auto *layout =
        new QVBoxLayout(
            tableCard);

    layout->setContentsMargins(
        16,
        16,
        16,
        16);

    devicesInventoryTable_ =
        new QTableWidget(
            tableCard);

    devicesInventoryTable_->setColumnCount(
        6);

    devicesInventoryTable_->setHorizontalHeaderLabels(
        {
            QStringLiteral(
                "Model"),
            QStringLiteral(
                "Serial"),
            QStringLiteral(
                "Interface"),
            QStringLiteral(
                "Capacity"),
            QStringLiteral(
                "System"),
            QStringLiteral(
                "Device ID")
        });

    configureTable(
        devicesInventoryTable_);

    devicesInventoryTable_->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::Stretch);

    devicesInventoryTable_->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::Stretch);

    devicesInventoryTable_->horizontalHeader()
        ->setSectionResizeMode(
            2,
            QHeaderView::ResizeToContents);

    devicesInventoryTable_->horizontalHeader()
        ->setSectionResizeMode(
            3,
            QHeaderView::ResizeToContents);

    devicesInventoryTable_->horizontalHeader()
        ->setSectionResizeMode(
            4,
            QHeaderView::ResizeToContents);

    devicesInventoryTable_->horizontalHeader()
        ->setSectionResizeMode(
            5,
            QHeaderView::Stretch);

    layout->addWidget(
        devicesInventoryTable_);

    root->addWidget(
        tableCard,
        1);

    scroll->setWidget(
        page);

    auto *outer =
        new QVBoxLayout(
            devicesPage_);

    outer->setContentsMargins(
        0,
        0,
        0,
        0);

    outer->addWidget(
        scroll);
}

void MainWindow::buildForensicsPage()
{
    forensicsPage_ =
        new QWidget;

    auto *root =
        new QVBoxLayout(
            forensicsPage_);

    root->setContentsMargins(
        0,
        0,
        0,
        0);

    root->setSpacing(
        0);

    auto *scroll =
        new QScrollArea(
            forensicsPage_);

    scroll->setWidgetResizable(
        true);

    scroll->setFrameShape(
        QFrame::NoFrame);

    scroll->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);

    auto *page =
        new QWidget;

    auto *pageLayout =
        new QVBoxLayout(
            page);

    pageLayout->setContentsMargins(
        30,
        28,
        30,
        30);

    pageLayout->setSpacing(
        16);

    auto *header =
        new QHBoxLayout;

    auto *titleBlock =
        new QVBoxLayout;

    titleBlock->setSpacing(
        4);

    titleBlock->addWidget(
        makeTitle(
            QStringLiteral(
                "Forensic Recovery"),
            page));

    titleBlock->addWidget(
        makeSubtitle(
            QStringLiteral(
                "Read-only acquisition, artifact recovery, validation and evidence inspection."),
            page));

    header->addLayout(
        titleBlock);

    header->addStretch();

    auto *forensicBadge =
        makeBadge(
            QStringLiteral(
                "READ ONLY"),
            page);

    forensicBadge->setStyleSheet(
        badgeStyle(
            "SAFE"));

    header->addWidget(
        forensicBadge);

    pageLayout->addLayout(
        header);

    auto *infoCard =
        makeCard(page);

    auto *infoLayout =
        new QHBoxLayout(
            infoCard);

    infoLayout->setContentsMargins(
        18,
        15,
        18,
        15);

    infoLayout->setSpacing(
        12);

    auto *infoIcon =
        new QLabel(
            QStringLiteral(
                "⌕"),
            infoCard);

    infoIcon->setAlignment(
        Qt::AlignCenter);

    infoIcon->setFixedSize(
        40,
        40);

    infoIcon->setStyleSheet(
        "QLabel {"
        "background:#F5F3FF;"
        "border:1px solid #DDD6FE;"
        "border-radius:11px;"
        "color:#6941C6;"
        "font-size:18px;"
        "font-weight:700;"
        "}");

    infoLayout->addWidget(
        infoIcon);

    auto *infoText =
        new QVBoxLayout;

    infoText->setSpacing(
        3);

    infoText->addWidget(
        makeFeatureTitle(
            QStringLiteral(
                "Forensic acquisition"),
            infoCard));

    infoText->addWidget(
        makeFeatureText(
            QStringLiteral(
                "The recovery engine can acquire from a physical storage device or a forensic image and present validated recovered artifacts for inspection."),
            infoCard));

    infoLayout->addLayout(
        infoText,
        1);

    pageLayout->addWidget(
        infoCard);

    forensicPage_ =
        new ForensicPage(
            deviceController_,
            page);

    forensicPage_->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding);

    pageLayout->addWidget(
        forensicPage_,
        1);

    auto *evidenceCard =
        makeCard(page);

    auto *evidenceLayout =
        new QVBoxLayout(
            evidenceCard);

    evidenceLayout->setContentsMargins(
        18,
        16,
        18,
        16);

    evidenceLayout->setSpacing(
        7);

    evidenceLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "LATEST SANITIZATION EVIDENCE"),
            evidenceCard));

    evidenceValue_ =
        makeValue(
            QStringLiteral(
                "No sanitization pipeline evidence generated yet."),
            evidenceCard);

    evidenceLayout->addWidget(
        evidenceValue_);

    pageLayout->addWidget(
        evidenceCard);

    scroll->setWidget(
        page);

    root->addWidget(
        scroll);
}

void MainWindow::buildSettingsPage()
{
    settingsPage_ =
        new QWidget;

    auto *scroll =
        new QScrollArea(
            settingsPage_);

    scroll->setWidgetResizable(
        true);

    scroll->setFrameShape(
        QFrame::NoFrame);

    scroll->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);

    auto *page =
        new QWidget;

    auto *root =
        new QVBoxLayout(
            page);

    root->setContentsMargins(
        30,
        28,
        30,
        30);

    root->setSpacing(
        16);

    root->addWidget(
        makeTitle(
            QStringLiteral(
                "Settings"),
            page));

    root->addWidget(
        makeSubtitle(
            QStringLiteral(
                "Workstation configuration, backend connectivity and execution policy."),
            page));

    auto *apiCard =
        makeCard(page);

    auto *apiLayout =
        new QVBoxLayout(
            apiCard);

    apiLayout->setContentsMargins(
        18,
        18,
        18,
        18);

    apiLayout->setSpacing(
        7);

    apiLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "BACKEND API"),
            apiCard));

    apiLayout->addWidget(
        makeValue(
            QStringLiteral(
                "http://localhost:5000"),
            apiCard));

    root->addWidget(
        apiCard);

    auto *executionCard =
        makeCard(page);

    auto *executionLayout =
        new QVBoxLayout(
            executionCard);

    executionLayout->setContentsMargins(
        18,
        18,
        18,
        18);

    executionLayout->setSpacing(
        8);

    executionLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "DESTRUCTIVE-OPERATION POLICY"),
            executionCard));

    auto *policyTitle =
        new QLabel(
            QStringLiteral(
                "Target identity is always checked before execution."),
            executionCard);

    policyTitle->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#172033;"
        "font-size:14px;"
        "font-weight:700;"
        "}");

    executionLayout->addWidget(
        policyTitle);

    auto *policyText =
        new QLabel(
            QStringLiteral(
                "Never authorize an operating-system disk or an unintended storage device. The exact target must pass identity validation and safety checks before physical sanitization."),
            executionCard);

    policyText->setWordWrap(
        true);

    policyText->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:11px;"
        "}");

    executionLayout->addWidget(
        policyText);

    root->addWidget(
        executionCard);

    auto *workflowCard =
        makeCard(page);

    auto *workflowLayout =
        new QVBoxLayout(
            workflowCard);

    workflowLayout->setContentsMargins(
        18,
        18,
        18,
        18);

    workflowLayout->setSpacing(
        7);

    workflowLayout->addWidget(
        makeCaption(
            QStringLiteral(
                "SECUREWIPE WORKFLOW"),
            workflowCard));

    workflowLayout->addWidget(
        makeValue(
            QStringLiteral(
                "Sanitization: assigned request → physical target → safety → sanitize → verify → certificate → audit evidence"),
            workflowCard));

    workflowLayout->addWidget(
        makeValue(
            QStringLiteral(
                "Forensics: read-only source → acquisition → carving → validation → recovered evidence"),
            workflowCard));

    root->addWidget(
        workflowCard);

    root->addStretch();

    scroll->setWidget(
        page);

    auto *outer =
        new QVBoxLayout(
            settingsPage_);

    outer->setContentsMargins(
        0,
        0,
        0,
        0);

    outer->addWidget(
        scroll);
}

void MainWindow::applyTheme()
{
    qApp->setStyleSheet(
        "QWidget {"
        "font-family:'Segoe UI';"
        "color:#172033;"
        "}"
        "QMainWindow {"
        "background:#F5F7FB;"
        "}"
        "QScrollArea {"
        "background:#F5F7FB;"
        "border:none;"
        "}"
        "QScrollArea > QWidget > QWidget {"
        "background:#F5F7FB;"
        "}"
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "}"
        "QLineEdit {"
        "background:#FFFFFF;"
        "color:#172033;"
        "border:1px solid #D0D5DD;"
        "border-radius:10px;"
        "padding:10px 12px;"
        "font-size:12px;"
        "}"
        "QLineEdit:focus {"
        "border:1px solid #2563EB;"
        "}"
        "QComboBox {"
        "background:#FFFFFF;"
        "color:#172033;"
        "border:1px solid #D0D5DD;"
        "border-radius:10px;"
        "padding:9px 12px;"
        "font-size:12px;"
        "}"
        "QComboBox:focus {"
        "border:1px solid #2563EB;"
        "}"
        "QComboBox QAbstractItemView {"
        "background:#FFFFFF;"
        "color:#172033;"
        "border:1px solid #D0D5DD;"
        "selection-background-color:#EFF6FF;"
        "selection-color:#172033;"
        "padding:4px;"
        "}"
        "QTableWidget {"
        "background:#FFFFFF;"
        "alternate-background-color:#FAFBFC;"
        "color:#172033;"
        "border:1px solid #E4E7EC;"
        "border-radius:12px;"
        "gridline-color:transparent;"
        "outline:none;"
        "font-size:12px;"
        "}"
        "QTableWidget::item {"
        "padding:9px;"
        "border:none;"
        "}"
        "QTableWidget::item:selected {"
        "background:#EFF6FF;"
        "color:#172033;"
        "}"
        "QHeaderView::section {"
        "background:#F8FAFC;"
        "color:#667085;"
        "border:none;"
        "border-bottom:1px solid #E4E7EC;"
        "padding:10px 9px;"
        "font-size:10px;"
        "font-weight:700;"
        "}"
        "QProgressBar {"
        "background:#E9EEF5;"
        "border:none;"
        "border-radius:5px;"
        "min-height:8px;"
        "max-height:8px;"
        "}"
        "QProgressBar::chunk {"
        "background:#2563EB;"
        "border-radius:5px;"
        "}"
        "QScrollBar:vertical {"
        "background:#F2F4F7;"
        "width:9px;"
        "border:none;"
        "}"
        "QScrollBar::handle:vertical {"
        "background:#CBD5E1;"
        "min-height:35px;"
        "border-radius:4px;"
        "}"
        "QScrollBar::add-line:vertical,"
        "QScrollBar::sub-line:vertical {"
        "height:0;"
        "}"
        "QToolTip {"
        "background:#172033;"
        "color:#FFFFFF;"
        "border:none;"
        "padding:7px 9px;"
        "border-radius:6px;"
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

    for (QPushButton *item :
         buttons)
    {
        if (!item)
            continue;

        if (item == button)
        {
            item->setStyleSheet(
                "QPushButton {"
                "background:#EFF6FF;"
                "color:#1D4ED8;"
                "border:none;"
                "border-radius:10px;"
                "text-align:left;"
                "padding:0 14px;"
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
                "border-radius:10px;"
                "text-align:left;"
                "padding:0 14px;"
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
    if (!connectionBadgeLabel_)
        return;

    const QString display =
        text.isEmpty()
            ? (connected
                   ? QStringLiteral(
                         "Connected")
                   : QStringLiteral(
                         "Disconnected"))
            : text;

    connectionBadgeLabel_->setText(
        display);

    connectionBadgeLabel_->setStyleSheet(
        badgeStyle(
            connected
                ? QStringLiteral(
                      "CONNECTED")
                : QStringLiteral(
                      "DISCONNECTED")));
}

void MainWindow::showPage(
    QWidget *page,
    QPushButton *navButton)
{
    if (!page ||
        !contentStack_)
    {
        return;
    }

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
    dashboardJobsTable_->setRowCount(
        0);

    assignedJobsTable_->clearContents();
    assignedJobsTable_->setRowCount(
        0);

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
                QStringLiteral(
                    "requestId"))
                .toString();

        const QString deviceType =
            request.value(
                QStringLiteral(
                    "deviceType"))
                .toString();

        const QString method =
            request.value(
                QStringLiteral(
                    "sanitizationMethod"))
                .toString();

        const QString status =
            request.value(
                QStringLiteral(
                    "status"))
                .toString();

        const QString asset =
            request.value(
                QStringLiteral(
                    "asset"))
                .toString(
                    request.value(
                        QStringLiteral(
                            "customer"))
                        .toString());

        if (requestId.isEmpty())
            continue;

        ++total;

        if (status ==
                QStringLiteral(
                    "ASSIGNED") ||
            status ==
                QStringLiteral(
                    "IN_PROGRESS") ||
            status ==
                QStringLiteral(
                    "VERIFYING"))
        {
            ++active;
        }

        if (status ==
            QStringLiteral(
                "COMPLETED"))
        {
            ++completed;
        }

        if (status ==
            QStringLiteral(
                "FAILED"))
        {
            ++failed;
        }

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
                        ? QStringLiteral(
                              "—")
                        : method));

            auto *statusItem =
                new QTableWidgetItem(
                    status);

            statusItem->setForeground(
                status ==
                        QStringLiteral(
                            "FAILED")
                    ? QBrush(
                          QColor(
                              "#B42318"))
                    : QBrush(
                          QColor(
                              "#344054")));

            dashboardJobsTable_->setItem(
                row,
                3,
                statusItem);
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
                    ? QStringLiteral(
                          "—")
                    : method));

        assignedJobsTable_->setItem(
            row,
            3,
            new QTableWidgetItem(
                asset.isEmpty()
                    ? QStringLiteral(
                          "—")
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

        const int index =
            jobComboBox_->count();

        jobComboBox_->addItem(
            display);

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
            requestId ==
                selectedRequestId_)
        {
            jobComboBox_->setCurrentIndex(
                index);
        }
    }

    totalJobsValue_->setText(
        QString::number(
            total));

    activeJobsValue_->setText(
        QString::number(
            active));

    completedJobsValue_->setText(
        QString::number(
            completed));

    failedJobsValue_->setText(
        QString::number(
            failed));

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
                QStringLiteral(
                    "requestId"))
                .toString() ==
            selectedRequestId_)
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
            Qt::UserRole)
            .toString();

    selectedRequestDeviceType_ =
        jobComboBox_->itemData(
            index,
            Qt::UserRole + 1)
            .toString();

    selectedRequestMethod_ =
        jobComboBox_->itemData(
            index,
            Qt::UserRole + 2)
            .toString();

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
            QStringLiteral(
                "requestId"))
            .toString(
                QStringLiteral(
                    "—")));

    jobDeviceTypeValue_->setText(
        request.value(
            QStringLiteral(
                "deviceType"))
            .toString(
                QStringLiteral(
                    "—")));

    jobRequestedMethodValue_->setText(
        request.value(
            QStringLiteral(
                "sanitizationMethod"))
            .toString(
                QStringLiteral(
                    "—")));

    jobAssetValue_->setText(
        request.value(
            QStringLiteral(
                "asset"))
            .toString(
                request.value(
                    QStringLiteral(
                        "customer"))
                    .toString(
                        QStringLiteral(
                            "—"))));

    jobWorkstationValue_->setText(
        request.value(
            QStringLiteral(
                "workstationCenter"))
            .toString(
                request.value(
                    QStringLiteral(
                        "center"))
                    .toString(
                        QStringLiteral(
                            "—"))));

    const QString status =
        request.value(
            QStringLiteral(
                "status"))
            .toString();

    if (status ==
        QStringLiteral(
            "ASSIGNED"))
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "This request is ready for physical workstation execution."));

        jobMessageLabel_->setStyleSheet(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#667085;"
            "font-size:12px;"
            "font-weight:600;"
            "}");
    }
    else if (status ==
             QStringLiteral(
                 "IN_PROGRESS"))
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "This request is currently in physical sanitization."));
    }
    else if (status ==
             QStringLiteral(
                 "VERIFYING"))
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "The sanitization result is in verification/evidence state."));
    }
    else if (status ==
             QStringLiteral(
                 "COMPLETED"))
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "This sanitization request is completed."));
    }
    else if (status ==
             QStringLiteral(
                 "FAILED"))
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
    const auto &devices =
        deviceController_->devices();

    if (devicesInventoryTable_)
    {
        devicesInventoryTable_->blockSignals(
            true);

        devicesInventoryTable_->clearContents();

        devicesInventoryTable_->setRowCount(
            0);

        for (int i = 0;
             i < static_cast<int>(devices.size());
             ++i)
        {
            const StorageDevice &device =
                devices[static_cast<std::size_t>(i)];

            const int row =
                devicesInventoryTable_->rowCount();

            devicesInventoryTable_->insertRow(
                row);

            auto *modelItem =
                new QTableWidgetItem(
                    QString::fromStdString(
                        device.getModel()));

            modelItem->setData(
                Qt::UserRole,
                i);

            devicesInventoryTable_->setItem(
                row,
                0,
                modelItem);

            devicesInventoryTable_->setItem(
                row,
                1,
                new QTableWidgetItem(
                    QString::fromStdString(
                        device.getSerialNumber())));

            devicesInventoryTable_->setItem(
                row,
                2,
                new QTableWidgetItem(
                    QString::fromStdString(
                        device.getInterfaceType())));

            devicesInventoryTable_->setItem(
                row,
                3,
                new QTableWidgetItem(
                    formatCapacity(
                        device.getCapacityBytes())));

            auto *systemItem =
                new QTableWidgetItem(
                    device.isSystemDisk()
                        ? QStringLiteral("BLOCKED")
                        : QStringLiteral("Available"));

            systemItem->setForeground(
                device.isSystemDisk()
                    ? QBrush(QColor("#B42318"))
                    : QBrush(QColor("#027A48")));

            devicesInventoryTable_->setItem(
                row,
                4,
                systemItem);

            devicesInventoryTable_->setItem(
                row,
                5,
                new QTableWidgetItem(
                    QString::fromStdString(
                        device.getDeviceId())));
        }

        devicesInventoryTable_->blockSignals(
            false);
    }

    if (!deviceTable_)
        return;

    deviceTable_->blockSignals(
        true);

    deviceTable_->clearContents();

    deviceTable_->setRowCount(
        0);

    for (int i = 0;
         i < static_cast<int>(devices.size());
         ++i)
    {
        const StorageDevice &device =
            devices[static_cast<std::size_t>(i)];

        if (!selectedRequestDeviceType_.isEmpty() &&
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

        auto *modelItem =
            new QTableWidgetItem(
                QString::fromStdString(
                    device.getModel()));

        modelItem->setData(
            Qt::UserRole,
            i);

        deviceTable_->setItem(
            row,
            0,
            modelItem);

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
                    ? QStringLiteral(
                          "BLOCKED")
                    : QStringLiteral(
                          "Available"));

        systemItem->setForeground(
            device.isSystemDisk()
                ? QBrush(
                      QColor(
                          "#B42318"))
                : QBrush(
                      QColor(
                          "#027A48")));

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
        method ==
                SanitizationMethod::Unsupported
            ? QStringLiteral(
                  "Unsupported")
            : QStringLiteral(
                  "Available"));

    selectedMethodValue_->setText(
        sanitizationMethodName(
            method));

    targetSafetyBadge_->setText(
        QStringLiteral(
            "NOT CHECKED"));

    targetSafetyBadge_->setStyleSheet(
        badgeStyle(
            "NOT_CHECKED"));

    targetSafetyText_->setText(
        QStringLiteral(
            "Exact target selected. Run a fresh safety check before sanitization."));

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
        QStringLiteral(
            "NOT CHECKED"));

    targetSafetyBadge_->setStyleSheet(
        badgeStyle(
            "NOT_CHECKED"));

    targetSafetyText_->setText(
        QStringLiteral(
            "Select an exact physical target."));

    pipelineStatusValue_->setText(
        QStringLiteral(
            "NOT STARTED"));

    pipelineStatusValue_->setStyleSheet(
        badgeStyle(
            "NOT_STARTED"));

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

    evidenceValue_->setText(
        QStringLiteral(
            "No sanitization pipeline evidence generated yet."));

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

    if (!deviceController_->selectedTarget().has_value())
    {
        targetSafetyBadge_->setText(QStringLiteral("BLOCKED"));
        targetSafetyBadge_->setStyleSheet(badgeStyle("BLOCKED"));
        targetSafetyText_->setText(
            QStringLiteral("Select an exact physical target before running the safety gate."));
        startSanitizationButton_->setEnabled(false);
        return;
    }

    validateTargetButton_->setEnabled(false);
    startSanitizationButton_->setEnabled(false);

    targetSafetyBadge_->setText(QStringLiteral("CHECKING"));
    targetSafetyBadge_->setStyleSheet(badgeStyle("CHECKING"));
    targetSafetyText_->setText(
        QStringLiteral("Fresh target rediscovery and core safety evaluation are in progress..."));
    jobMessageLabel_->setText(QStringLiteral("Running target safety gate. No destructive operation has started."));
    jobMessageLabel_->setStyleSheet(
        "QLabel { background:transparent; border:none; color:#667085; font-size:12px; font-weight:600; }");

    QApplication::processEvents();

    if (!deviceController_->validateSelectedTarget())
    {
        const QString message = QStringLiteral(
            "Fresh target validation failed. The previously selected device is no longer trusted for destructive use.\n"
            "Select the physical device again and rerun the safety check.");

        targetSafetyBadge_->setText(QStringLiteral("BLOCKED"));
        targetSafetyBadge_->setStyleSheet(badgeStyle("BLOCKED"));
        targetSafetyText_->setText(message);
        validateTargetButton_->setEnabled(true);

        jobMessageLabel_->setText(QStringLiteral("Safety gate blocked the operation: %1").arg(message));
        jobMessageLabel_->setStyleSheet(
            "QLabel { background:transparent; border:none; color:#B42318; font-size:12px; font-weight:600; }");
        return;
    }

    const auto validatedTarget = deviceController_->selectedTarget();
    if (!validatedTarget.has_value())
    {
        targetSafetyBadge_->setText(QStringLiteral("BLOCKED"));
        targetSafetyBadge_->setStyleSheet(badgeStyle("BLOCKED"));
        targetSafetyText_->setText(QStringLiteral("Validation completed without a trusted target. Operation blocked."));
        validateTargetButton_->setEnabled(true);
        return;
    }

    updateTargetPanelFromDevice(*validatedTarget);

    if (!deviceController_->evaluateSelectedTarget())
    {
        const SafetyResult &result = deviceController_->lastSafetyResult();

        targetSafetyBadge_->setText(QStringLiteral("BLOCKED"));
        targetSafetyBadge_->setStyleSheet(badgeStyle("BLOCKED"));

        QString details = result.summary.empty()
            ? QStringLiteral("The core SafetyEngine did not authorize this target.")
            : QString::fromStdString(result.summary);

        for (const SafetyCheckResult &check : result.checks)
        {
            if (!details.isEmpty())
                details += QStringLiteral("\n");

            const QString state = check.passed
                ? QStringLiteral("✓")
                : QStringLiteral("✗");
            const QString name = QString::fromStdString(check.checkName);
            const QString message = QString::fromStdString(check.message);

            details += QStringLiteral("%1 %2").arg(state, name);
            if (!message.trimmed().isEmpty())
                details += QStringLiteral(" — %1").arg(message.trimmed());
        }

        targetSafetyText_->setText(details);
        validateTargetButton_->setEnabled(true);

        jobMessageLabel_->setText(QStringLiteral("Sanitization blocked by the core safety gate."));
        jobMessageLabel_->setStyleSheet(
            "QLabel { background:transparent; border:none; color:#B42318; font-size:12px; font-weight:700; }");
        return;
    }

    const auto target = deviceController_->selectedTarget();
    if (!target.has_value())
    {
        targetSafetyBadge_->setText(QStringLiteral("BLOCKED"));
        targetSafetyBadge_->setStyleSheet(badgeStyle("BLOCKED"));
        targetSafetyText_->setText(QStringLiteral("The safety check completed without a trusted target."));
        validateTargetButton_->setEnabled(true);
        return;
    }

    updateTargetPanelFromDevice(*target);

    const SanitizationCapability capability =
        deviceController_->detectSelectedTargetCapability();
    const SanitizationMethod method =
        deviceController_->detectSelectedTargetMethod();

    QString capabilityText = QStringLiteral("Capability could not be confirmed.");
    if (capability.nativeSanitizeSupported == NativeSanitizeSupport::SUPPORTED)
    {
        capabilityText = QStringLiteral("Native sanitization capability confirmed.");
    }
    else if (capability.isUsbDevice && capability.scsiPathAvailable)
    {
        capabilityText = QStringLiteral("USB/SCSI sanitization path confirmed.");
    }

    capabilityValue_->setText(capabilityText);
    selectedMethodValue_->setText(sanitizationMethodName(method));

    if (method == SanitizationMethod::Unsupported)
    {
        targetSafetyBadge_->setText(QStringLiteral("UNSUPPORTED"));
        targetSafetyBadge_->setStyleSheet(badgeStyle("UNSUPPORTED"));
        targetSafetyText_->setText(
            QStringLiteral("Safety checks passed, but the core could not select a supported sanitization method."));
        validateTargetButton_->setEnabled(true);
        startSanitizationButton_->setEnabled(false);

        jobMessageLabel_->setText(
            QStringLiteral("Target is safe, but no supported sanitization method is available."));
        jobMessageLabel_->setStyleSheet(
            "QLabel { background:transparent; border:none; color:#B54708; font-size:12px; font-weight:700; }");
        return;
    }

    QString detectedMethod;
    switch (method)
    {
    case SanitizationMethod::NvmeSanitize:
        detectedMethod = QStringLiteral("NVME_SANITIZE");
        break;
    case SanitizationMethod::AtaSanitize:
        detectedMethod = QStringLiteral("ATA_SANITIZE");
        break;
    case SanitizationMethod::HostOverwrite:
        detectedMethod = QStringLiteral("HOST_OVERWRITE");
        break;
    default:
        detectedMethod = QStringLiteral("UNSUPPORTED");
        break;
    }

    QString assignedMethod = selectedRequestMethod_.trimmed().toUpper();
    assignedMethod.replace(QStringLiteral(" "), QStringLiteral("_"));
    assignedMethod.replace(QStringLiteral("-"), QStringLiteral("_"));

    if (assignedMethod == QStringLiteral("NVME"))
        assignedMethod = QStringLiteral("NVME_SANITIZE");
    if (assignedMethod == QStringLiteral("ATA"))
        assignedMethod = QStringLiteral("ATA_SANITIZE");

    if (!assignedMethod.isEmpty() &&
        assignedMethod != QStringLiteral("UNSUPPORTED") &&
        assignedMethod != detectedMethod)
    {
        targetSafetyBadge_->setText(QStringLiteral("METHOD MISMATCH"));
        targetSafetyBadge_->setStyleSheet(badgeStyle("BLOCKED"));
        targetSafetyText_->setText(
            QStringLiteral("The assigned request method (%1) does not match the method selected by the sanitization core (%2).")
                .arg(assignedMethod, detectedMethod));
        validateTargetButton_->setEnabled(true);
        startSanitizationButton_->setEnabled(false);
        return;
    }

    const SafetyResult &result = deviceController_->lastSafetyResult();

    targetSafetyBadge_->setText(QStringLiteral("SAFE"));
    targetSafetyBadge_->setStyleSheet(badgeStyle("SAFE"));

    QString details = result.summary.empty()
        ? QStringLiteral("Core safety gate passed. This target is authorized to proceed to final confirmation.")
        : QString::fromStdString(result.summary);

    for (const SafetyCheckResult &check : result.checks)
    {
        if (!details.isEmpty())
            details += QStringLiteral("\n");

        const QString state = check.passed
            ? QStringLiteral("✓")
            : QStringLiteral("✗");
        const QString name = QString::fromStdString(check.checkName);
        const QString message = QString::fromStdString(check.message);

        details += QStringLiteral("%1 %2").arg(state, name);
        if (!message.trimmed().isEmpty())
            details += QStringLiteral(" — %1").arg(message.trimmed());
    }

    details += QStringLiteral("\n\nSelected method: %1").arg(sanitizationMethodName(method));
    details += QStringLiteral("\nCapability: %1").arg(capabilityText);
    targetSafetyText_->setText(details);

    const QJsonObject request = selectedRequestObject();
    const QString status = request.value(QStringLiteral("status")).toString();
    const bool requestReady = status == QStringLiteral("ASSIGNED");

    startSanitizationButton_->setEnabled(requestReady && result.isOverallSafe);
    validateTargetButton_->setEnabled(true);

    if (requestReady)
    {
        jobMessageLabel_->setText(
            QStringLiteral("Safety gate passed. Final target confirmation is available."));
        jobMessageLabel_->setStyleSheet(
            "QLabel { background:transparent; border:none; color:#027A48; font-size:12px; font-weight:700; }");
    }
    else
    {
        jobMessageLabel_->setText(
            QStringLiteral("Safety gate passed, but the selected request is not ASSIGNED. Destructive execution remains disabled."));
        jobMessageLabel_->setStyleSheet(
            "QLabel { background:transparent; border:none; color:#B54708; font-size:12px; font-weight:700; }");
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
            QStringLiteral(
                "No request"),
            QStringLiteral(
                "Select an assigned sanitization request first."));

        return;
    }

    const QJsonObject request =
        selectedRequestObject();

    if (request.value(
            QStringLiteral(
                "status"))
            .toString() !=
        QStringLiteral(
            "ASSIGNED"))
    {
        QMessageBox::warning(
            this,
            QStringLiteral(
                "Request unavailable"),
            QStringLiteral(
                "Only an ASSIGNED request can start physical execution."));

        return;
    }

    auto target =
        deviceController_->selectedTarget();

    if (!target.has_value())
    {
        QMessageBox::warning(
            this,
            QStringLiteral(
                "No target"),
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

    const auto validatedTarget =
        deviceController_->selectedTarget();

    if (!validatedTarget.has_value())
    {
        QMessageBox::critical(
            this,
            QStringLiteral("Target validation"),
            QStringLiteral("The selected physical target is no longer available after fresh validation. Sanitization was blocked."));
        runTargetSafetyCheck();
        return;
    }

    target = validatedTarget;

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
                "Capacity: %5\n"
                "Physical device: %6\n"
                "Method: %7\n\n"
                "This operation is destructive and cannot be undone.\n"
                "Continue only when this exact physical target is intentionally authorized.")
                .arg(
                    selectedRequestId_,
                    selectedRequestDeviceType_,
                    model,
                    serial,
                    formatCapacity(target->getCapacityBytes()),
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

        jobMessageLabel_->setStyleSheet(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#667085;"
            "font-size:12px;"
            "font-weight:600;"
            "}");

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
        QStringLiteral(
            "STARTING"));

    pipelineStatusValue_->setStyleSheet(
        badgeStyle(
            "IN_PROGRESS"));

    operationValue_->setText(
        QStringLiteral(
            "Waiting for request synchronization"));

    verificationValue_->setText(
        QStringLiteral(
            "Pending"));

    jobMessageLabel_->setText(
        QStringLiteral(
            "Moving assigned request to IN_PROGRESS..."));

    requestService_->updateRequestStatus(
        token,
        selectedRequestId_,
        QStringLiteral(
            "IN_PROGRESS"));

    connect(
        requestService_,
        &SanitizationRequestService::
            requestStatusUpdated,
        this,
        [this, targetCopy](
            const QString &requestId,
            const QString &status)
        {
            if (requestId !=
                    selectedRequestId_ ||
                status !=
                    QStringLiteral(
                        "IN_PROGRESS"))
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
                    SecureWipe::SanitizationPipelineResult>::
                    finished,
                this,
                [this, watcher]()
                {
                    const auto result =
                        watcher->result();

                    watcher->deleteLater();

                    finishSanitization(
                        result);
                });

            pipelineStatusValue_->setText(
                QStringLiteral(
                    "SANITIZING"));

            pipelineStatusValue_->setStyleSheet(
                badgeStyle(
                    "IN_PROGRESS"));

            jobMessageLabel_->setText(
                QStringLiteral(
                    "Physical sanitization is running. Do not disconnect the target device."));

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
                        catch (
                            const std::exception &exception)
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
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#B42318;"
            "font-size:12px;"
            "font-weight:600;"
            "}");

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

        jobMessageLabel_->setStyleSheet(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#027A48;"
            "font-size:12px;"
            "font-weight:700;"
            "}");
    }
    else
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "Physical sanitization failed. Uploading failure result..."));

        jobMessageLabel_->setStyleSheet(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#B42318;"
            "font-size:12px;"
            "font-weight:600;"
            "}");
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
                ? QStringLiteral(
                      "COMPLETED")
                : result.status ==
                          SecureWipe::SanitizationStatus::FAILED
                      ? QStringLiteral(
                            "FAILED")
                      : QStringLiteral(
                            "IN_PROGRESS")));

    verificationValue_->setText(
        verificationText(
            result.verificationStatus));

    operationValue_->setText(
        result.operationId.empty()
            ? QStringLiteral(
                  "—")
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
             .certificateId
             .empty())
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

    if (!pipelineResult
             .auditLogPath
             .empty())
    {
        evidence +=
            QStringLiteral(
                "Audit: ");

        evidence +=
            QString::fromStdString(
                pipelineResult
                    .auditLogPath);
    }

    if (!pipelineResult
             .operationLogPath
             .empty())
    {
        if (!evidence.isEmpty())
            evidence += '\n';

        evidence +=
            QStringLiteral(
                "Operation log: ");

        evidence +=
            QString::fromStdString(
                pipelineResult
                    .operationLogPath);
    }

    if (!pipelineResult
             .certificatePath
             .empty())
    {
        if (!evidence.isEmpty())
            evidence += '\n';

        evidence +=
            QStringLiteral(
                "Certificate: ");

        evidence +=
            QString::fromStdString(
                pipelineResult
                    .certificatePath);
    }

    evidenceValue_->setText(
        evidence.isEmpty()
            ? QStringLiteral(
                  "No sanitization evidence path reported.")
            : evidence);

    if (result.isSuccess())
    {
        jobMessageLabel_->setText(
            QStringLiteral(
                "Sanitization and verification completed successfully."));

        jobMessageLabel_->setStyleSheet(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#027A48;"
            "font-size:12px;"
            "font-weight:700;"
            "}");
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
            if (!message.isEmpty())
                message += '\n';

            message +=
                QString::fromStdString(
                    result.errorMessage);
        }

        jobMessageLabel_->setText(
            message);

        jobMessageLabel_->setStyleSheet(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#B42318;"
            "font-size:12px;"
            "font-weight:600;"
            "}");
    }
}

void MainWindow::showSelectedDeviceDetails()
{
    if (!devicesInventoryTable_)
        return;

    const int row =
        devicesInventoryTable_->currentRow();

    if (row < 0 ||
        row >= devicesInventoryTable_->rowCount())
    {
        return;
    }

    QTableWidgetItem *item =
        devicesInventoryTable_->item(
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

    if (!ok ||
        deviceIndex < 0 ||
        deviceIndex >=
            static_cast<int>(
                deviceController_->devices().size()))
    {
        QMessageBox::warning(
            this,
            QStringLiteral(
                "Device details"),
            QStringLiteral(
                "The selected physical device is no longer available in the current discovery result."));
        return;
    }

    showDeviceDetails(
        deviceController_->devices()
            .at(
                static_cast<std::size_t>(
                    deviceIndex)));
}

void MainWindow::showDeviceDetails(
    const StorageDevice &device)
{
    if (!contentStack_)
        return;

    if (deviceDetailsPage_)
    {
        contentStack_->removeWidget(
            deviceDetailsPage_);

        deviceDetailsPage_->deleteLater();

        deviceDetailsPage_ =
            nullptr;
    }

    auto *detailsPage =
        new DeviceDetailsPage(
            device,
            contentStack_);

    deviceDetailsPage_ =
        detailsPage;

    connect(
        detailsPage,
        &DeviceDetailsPage::backRequested,
        this,
        [this]()
        {
            if (deviceDetailsPage_)
            {
                contentStack_->removeWidget(
                    deviceDetailsPage_);

                deviceDetailsPage_->deleteLater();

                deviceDetailsPage_ =
                    nullptr;
            }

            contentStack_->setCurrentWidget(
                devicesPage_);

            setActiveNav(
                devicesNavButton_);

            refreshPhysicalDevices();
        });

    connect(
        detailsPage,
        &DeviceDetailsPage::refreshRequested,
        this,
        [this]()
        {
            if (operationRunning_)
                return;

            if (deviceDetailsPage_)
            {
                contentStack_->removeWidget(
                    deviceDetailsPage_);

                deviceDetailsPage_->deleteLater();

                deviceDetailsPage_ =
                    nullptr;
            }

            contentStack_->setCurrentWidget(
                devicesPage_);

            setActiveNav(
                devicesNavButton_);

            refreshPhysicalDevices();
        });

    contentStack_->addWidget(
        detailsPage);

    contentStack_->setCurrentWidget(
        detailsPage);

    setActiveNav(
        devicesNavButton_);
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

    operatorNameLabel_->setText(
        QStringLiteral(
            "Operator"));

    selectedRequestId_.clear();
    selectedRequestDeviceType_.clear();
    selectedRequestMethod_.clear();

    assignedRequests_ =
        QJsonArray();

    resetTargetPanel();

    setConnectionState(
        false,
        QStringLiteral(
            "Signed out"));

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

    constexpr double gb =
        1024.0 *
        1024.0 *
        1024.0;

    constexpr double tb =
        1024.0 *
        1024.0 *
        1024.0 *
        1024.0;

    const double value =
        static_cast<double>(
            bytes);

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

    constexpr double kb =
        1024.0;

    constexpr double mb =
        1024.0 *
        1024.0;

    constexpr double gb =
        1024.0 *
        1024.0 *
        1024.0;

    const double value =
        static_cast<double>(
            bytes);

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
            .arg(
                milliseconds);
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
            .arg(
                hours)
            .arg(
                minutes % 60)
            .arg(
                seconds % 60);
    }

    if (minutes > 0)
    {
        return QStringLiteral(
            "%1m %2s")
            .arg(
                minutes)
            .arg(
                seconds % 60);
    }

    return QStringLiteral(
        "%1.%2 s")
        .arg(
            seconds)
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