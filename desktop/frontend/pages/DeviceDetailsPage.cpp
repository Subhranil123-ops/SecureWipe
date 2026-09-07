#include "DeviceDetailsPage.h"

#include "DeviceClassifier.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace
{

QString cardStyle()
{
    return QStringLiteral(
        "QFrame#deviceCard {"
        "background:#FFFFFF;"
        "border:1px solid #E7ECF3;"
        "border-radius:16px;"
        "}");
}

QString fieldLabelStyle()
{
    return QStringLiteral(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:11px;"
        "font-weight:600;"
        "}");
}

QString valueLabelStyle()
{
    return QStringLiteral(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#172033;"
        "font-size:13px;"
        "font-weight:600;"
        "}");
}

QString sectionTitleStyle()
{
    return QStringLiteral(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#101828;"
        "font-size:15px;"
        "font-weight:700;"
        "}");
}

QString secondaryTextStyle()
{
    return QStringLiteral(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:12px;"
        "}");
}

QString pendingBadgeStyle()
{
    return QStringLiteral(
        "QLabel {"
        "background:#FFF7ED;"
        "color:#B45309;"
        "border:1px solid #FED7AA;"
        "border-radius:10px;"
        "padding:6px 10px;"
        "font-size:10px;"
        "font-weight:700;"
        "}");
}

QString successBadgeStyle()
{
    return QStringLiteral(
        "QLabel {"
        "background:#ECFDF3;"
        "color:#027A48;"
        "border:1px solid #ABEFC6;"
        "border-radius:10px;"
        "padding:6px 10px;"
        "font-size:10px;"
        "font-weight:700;"
        "}");
}

QString failedBadgeStyle()
{
    return QStringLiteral(
        "QLabel {"
        "background:#FEF3F2;"
        "color:#B42318;"
        "border:1px solid #FECDCA;"
        "border-radius:10px;"
        "padding:6px 10px;"
        "font-size:10px;"
        "font-weight:700;"
        "}");
}

QString neutralBadgeStyle()
{
    return QStringLiteral(
        "QLabel {"
        "background:#F8FAFC;"
        "color:#475467;"
        "border:1px solid #E4E7EC;"
        "border-radius:10px;"
        "padding:6px 10px;"
        "font-size:10px;"
        "font-weight:700;"
        "}");
}

}

DeviceDetailsPage::DeviceDetailsPage(
    const StorageDevice &device,
    QWidget *parent)
    : QWidget(parent)
    , device_(device)
    , modelLabel_(nullptr)
    , typeBadgeLabel_(nullptr)
    , serialLabel_(nullptr)
    , capacityLabel_(nullptr)
    , interfaceLabel_(nullptr)
    , devicePathLabel_(nullptr)
    , systemDiskValueLabel_(nullptr)
    , removableValueLabel_(nullptr)
    , mediaTypeValueLabel_(nullptr)
    , busTypeValueLabel_(nullptr)
    , deviceTypeValueLabel_(nullptr)
    , classificationStatusLabel_(nullptr)
    , safetyStatusLabel_(nullptr)
    , safetyDescriptionLabel_(nullptr)
{
    setupUi();
}

void DeviceDetailsPage::setupUi()
{
    setObjectName(QStringLiteral("deviceDetailsPage"));

    setStyleSheet(
        QStringLiteral(
            "DeviceDetailsPage {"
            "background:#F5F7FB;"
            "}"
            "DeviceDetailsPage QLabel {"
            "background:transparent;"
            "border:none;"
            "}"
            "DeviceDetailsPage QFrame#deviceCard {"
            "background:#FFFFFF;"
            "border:1px solid #E7ECF3;"
            "border-radius:16px;"
            "}"));

    auto *scroll =
        new QScrollArea(this);

    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);

    auto *page =
        new QWidget;

    auto *rootLayout =
        new QVBoxLayout(page);

    rootLayout->setContentsMargins(
        30,
        26,
        30,
        30);

    rootLayout->setSpacing(16);

    /*
     * Top navigation
     */
    auto *topLayout =
        new QHBoxLayout;

    auto *backButton =
        new QPushButton(
            QStringLiteral("←  Back to Devices"),
            page);

    backButton->setCursor(
        Qt::PointingHandCursor);

    backButton->setMinimumHeight(34);

    backButton->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "background:transparent;"
            "border:none;"
            "color:#2563EB;"
            "padding:4px 2px;"
            "font-size:12px;"
            "font-weight:600;"
            "}"
            "QPushButton:hover {"
            "color:#1D4ED8;"
            "}"));

    auto *refreshButton =
        new QPushButton(
            QStringLiteral("↻  Refresh"),
            page);

    refreshButton->setCursor(
        Qt::PointingHandCursor);

    refreshButton->setMinimumHeight(36);

    refreshButton->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "background:#FFFFFF;"
            "border:1px solid #D0D5DD;"
            "border-radius:9px;"
            "color:#344054;"
            "padding:7px 13px;"
            "font-size:11px;"
            "font-weight:600;"
            "}"
            "QPushButton:hover {"
            "background:#F8FAFC;"
            "border-color:#98A2B3;"
            "}"));

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &DeviceDetailsPage::backRequested);

    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &DeviceDetailsPage::refreshRequested);

    topLayout->addWidget(
        backButton);

    topLayout->addStretch();

    topLayout->addWidget(
        refreshButton);

    rootLayout->addLayout(
        topLayout);

    /*
     * Page heading
     */
    auto *heading =
        new QVBoxLayout;

    heading->setSpacing(4);

    auto *title =
        new QLabel(
            QStringLiteral("Device Details"),
            page);

    title->setStyleSheet(
        QStringLiteral(
            "QLabel {"
            "color:#101828;"
            "font-size:26px;"
            "font-weight:750;"
            "}"));

    auto *subtitle =
        new QLabel(
            QStringLiteral(
                "Identity, classification and safety information for the selected physical storage device."),
            page);

    subtitle->setWordWrap(true);
    subtitle->setStyleSheet(
        QStringLiteral(
            "QLabel {"
            "color:#667085;"
            "font-size:12px;"
            "}"));

    heading->addWidget(title);
    heading->addWidget(subtitle);

    rootLayout->addLayout(
        heading);

    /*
     * Device identity
     */
    rootLayout->addWidget(
        createDeviceHeader());

    /*
     * Classification / system status
     */
    DeviceClassifier classifier;

    const ClassificationResult classification =
        classifier.classify(device_);

    auto *summaryLayout =
        new QHBoxLayout;

    summaryLayout->setSpacing(16);

    summaryLayout->addWidget(
        createClassificationCard(
            classification),
        1);

    summaryLayout->addWidget(
        createSystemStatusCard(
            classification),
        1);

    rootLayout->addLayout(
        summaryLayout);

    /*
     * Safety
     */
    rootLayout->addWidget(
        createSafetyCard());

    rootLayout->addStretch();

    scroll->setWidget(page);

    auto *pageLayout =
        new QVBoxLayout(this);

    pageLayout->setContentsMargins(
        0,
        0,
        0,
        0);

    pageLayout->addWidget(scroll);
}

/*
 * Device header
 */
QWidget *DeviceDetailsPage::createDeviceHeader()
{
    QFrame *card =
        createCard();

    auto *layout =
        new QHBoxLayout(card);

    layout->setContentsMargins(
        22,
        20,
        22,
        20);

    layout->setSpacing(18);

    auto *iconFrame =
        new QFrame(card);

    iconFrame->setFixedSize(
        68,
        68);

    iconFrame->setStyleSheet(
        QStringLiteral(
            "QFrame {"
            "background:#EFF6FF;"
            "border:1px solid #DBEAFE;"
            "border-radius:16px;"
            "}"));

    auto *iconLayout =
        new QVBoxLayout(iconFrame);

    iconLayout->setContentsMargins(
        0,
        0,
        0,
        0);

    auto *icon =
        new QLabel(
            QStringLiteral("SSD"),
            iconFrame);

    icon->setAlignment(
        Qt::AlignCenter);

    icon->setStyleSheet(
        QStringLiteral(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#2563EB;"
            "font-size:13px;"
            "font-weight:800;"
            "}"));

    iconLayout->addWidget(
        icon);

    layout->addWidget(
        iconFrame);

    auto *identityLayout =
        new QVBoxLayout;

    identityLayout->setSpacing(5);

    auto *identityTop =
        new QHBoxLayout;

    modelLabel_ =
        new QLabel(
            QString::fromStdString(
                device_.getModel()),
            card);

    modelLabel_->setStyleSheet(
        QStringLiteral(
            "QLabel {"
            "color:#101828;"
            "font-size:19px;"
            "font-weight:750;"
            "}"));

    modelLabel_->setTextInteractionFlags(
        Qt::TextSelectableByMouse);

    typeBadgeLabel_ =
        new QLabel(
            QStringLiteral("Storage Device"),
            card);

    typeBadgeLabel_->setAlignment(
        Qt::AlignCenter);

    typeBadgeLabel_->setStyleSheet(
        QStringLiteral(
            "QLabel {"
            "background:#F2F4F7;"
            "color:#475467;"
            "border:1px solid #E4E7EC;"
            "border-radius:9px;"
            "padding:5px 9px;"
            "font-size:10px;"
            "font-weight:700;"
            "}"));

    identityTop->addWidget(
        modelLabel_);

    identityTop->addSpacing(
        8);

    identityTop->addWidget(
        typeBadgeLabel_);

    identityTop->addStretch();

    identityLayout->addLayout(
        identityTop);

    auto *detailsGrid =
        new QGridLayout;

    detailsGrid->setContentsMargins(
        0,
        5,
        0,
        0);

    detailsGrid->setHorizontalSpacing(
        22);

    detailsGrid->setVerticalSpacing(
        8);

    serialLabel_ =
        createValueLabel(
            QString::fromStdString(
                device_.getSerialNumber()));

    capacityLabel_ =
        createValueLabel(
            formatCapacity(
                device_.getCapacityBytes()));

    interfaceLabel_ =
        createValueLabel(
            QString::fromStdString(
                device_.getInterfaceType()));

    devicePathLabel_ =
        createValueLabel(
            QString::fromStdString(
                device_.getDeviceId()));

    detailsGrid->addWidget(
        createFieldLabel(
            QStringLiteral("Serial Number")),
        0,
        0);

    detailsGrid->addWidget(
        serialLabel_,
        0,
        1);

    detailsGrid->addWidget(
        createFieldLabel(
            QStringLiteral("Capacity")),
        1,
        0);

    detailsGrid->addWidget(
        capacityLabel_,
        1,
        1);

    detailsGrid->addWidget(
        createFieldLabel(
            QStringLiteral("Interface")),
        0,
        2);

    detailsGrid->addWidget(
        interfaceLabel_,
        0,
        3);

    detailsGrid->addWidget(
        createFieldLabel(
            QStringLiteral("Device ID")),
        1,
        2);

    detailsGrid->addWidget(
        devicePathLabel_,
        1,
        3);

    identityLayout->addLayout(
        detailsGrid);

    layout->addLayout(
        identityLayout,
        1);

    /*
     * Safety status panel
     */
    auto *statusFrame =
        new QFrame(card);

    statusFrame->setFixedWidth(
        165);

    statusFrame->setStyleSheet(
        QStringLiteral(
            "QFrame {"
            "background:#FFF9EB;"
            "border:1px solid #FDE7B0;"
            "border-radius:12px;"
            "}"));

    auto *statusLayout =
        new QVBoxLayout(statusFrame);

    statusLayout->setContentsMargins(
        12,
        12,
        12,
        12);

    statusLayout->setSpacing(
        5);

    auto *shield =
        new QLabel(
            QStringLiteral("◆"),
            statusFrame);

    shield->setAlignment(
        Qt::AlignCenter);

    shield->setStyleSheet(
        QStringLiteral(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#D97706;"
            "font-size:15px;"
            "font-weight:800;"
            "}"));

    safetyStatusLabel_ =
        new QLabel(
            QStringLiteral(
                "Assessment Pending"),
            statusFrame);

    safetyStatusLabel_->setAlignment(
        Qt::AlignCenter);

    safetyStatusLabel_->setStyleSheet(
        QStringLiteral(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#B45309;"
            "font-size:11px;"
            "font-weight:700;"
            "}"));

    auto *statusDescription =
        new QLabel(
            QStringLiteral(
                "Safety assessment required before sanitization."),
            statusFrame);

    statusDescription->setWordWrap(
        true);

    statusDescription->setAlignment(
        Qt::AlignCenter);

    statusDescription->setStyleSheet(
        QStringLiteral(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#92400E;"
            "font-size:9px;"
            "}"));

    statusLayout->addWidget(
        shield);

    statusLayout->addWidget(
        safetyStatusLabel_);

    statusLayout->addWidget(
        statusDescription);

    layout->addWidget(
        statusFrame);

    return card;
}

/*
 * Classification
 */
QWidget *DeviceDetailsPage::createClassificationCard(
    const ClassificationResult &classification)
{
    QFrame *card =
        createCard();

    auto *layout =
        new QVBoxLayout(card);

    layout->setContentsMargins(
        18,
        18,
        18,
        18);

    layout->setSpacing(
        12);

    auto *header =
        new QHBoxLayout;

    header->addWidget(
        createSectionTitle(
            QStringLiteral("Classification")));

    header->addStretch();

    auto *badge =
        new QLabel(
            QStringLiteral("DETECTED"),
            card);

    badge->setAlignment(
        Qt::AlignCenter);

    badge->setStyleSheet(
        successBadgeStyle());

    header->addWidget(
        badge);

    layout->addLayout(
        header);

    auto *grid =
        new QGridLayout;

    grid->setVerticalSpacing(
        13);

    grid->setHorizontalSpacing(
        22);

    mediaTypeValueLabel_ =
        createValueLabel(
            mediaTypeText(
                classification.mediaType));

    busTypeValueLabel_ =
        createValueLabel(
            busTypeText(
                classification.busType));

    deviceTypeValueLabel_ =
        createValueLabel(
            deviceTypeText(
                classification.deviceType));

    classificationStatusLabel_ =
        createValueLabel(
            QStringLiteral("Detected"));

    classificationStatusLabel_->setStyleSheet(
        QStringLiteral(
            "QLabel {"
            "color:#027A48;"
            "font-size:12px;"
            "font-weight:700;"
            "}"));

    grid->addWidget(
        createFieldLabel(
            QStringLiteral("Media Type")),
        0,
        0);

    grid->addWidget(
        mediaTypeValueLabel_,
        0,
        1);

    grid->addWidget(
        createFieldLabel(
            QStringLiteral("Bus Type")),
        1,
        0);

    grid->addWidget(
        busTypeValueLabel_,
        1,
        1);

    grid->addWidget(
        createFieldLabel(
            QStringLiteral("Device Type")),
        2,
        0);

    grid->addWidget(
        deviceTypeValueLabel_,
        2,
        1);

    grid->addWidget(
        createFieldLabel(
            QStringLiteral("Classification")),
        3,
        0);

    grid->addWidget(
        classificationStatusLabel_,
        3,
        1);

    layout->addLayout(
        grid);

    return card;
}

/*
 * System status
 */
QWidget *DeviceDetailsPage::createSystemStatusCard(
    const ClassificationResult &classification)
{
    QFrame *card =
        createCard();

    auto *layout =
        new QVBoxLayout(card);

    layout->setContentsMargins(
        18,
        18,
        18,
        18);

    layout->setSpacing(
        12);

    auto *header =
        new QHBoxLayout;

    header->addWidget(
        createSectionTitle(
            QStringLiteral("System Status")));

    header->addStretch();

    auto *badge =
        new QLabel(
            device_.isSystemDisk()
                ? QStringLiteral("PROTECTED")
                : QStringLiteral("NON-SYSTEM"),
            card);

    badge->setAlignment(
        Qt::AlignCenter);

    badge->setStyleSheet(
        device_.isSystemDisk()
            ? failedBadgeStyle()
            : successBadgeStyle());

    header->addWidget(
        badge);

    layout->addLayout(
        header);

    auto *grid =
        new QGridLayout;

    grid->setVerticalSpacing(
        13);

    grid->setHorizontalSpacing(
        22);

    systemDiskValueLabel_ =
        createValueLabel(
            device_.isSystemDisk()
                ? QStringLiteral("Yes")
                : QStringLiteral("No"));

    removableValueLabel_ =
        createValueLabel(
            device_.isRemovable()
                ? QStringLiteral("Yes")
                : QStringLiteral("No"));

    auto *classificationValue =
        createValueLabel(
            classification.isSystemDisk
                ? QStringLiteral(
                    "System Device")
                : QStringLiteral(
                    "Non-System Device"));

    auto *seekPenaltyValue =
        createValueLabel(
            device_.hasSeekPenalty()
                ? QStringLiteral("Detected")
                : QStringLiteral("Not detected"));

    grid->addWidget(
        createFieldLabel(
            QStringLiteral("System Disk")),
        0,
        0);

    grid->addWidget(
        systemDiskValueLabel_,
        0,
        1);

    grid->addWidget(
        createFieldLabel(
            QStringLiteral("Removable")),
        1,
        0);

    grid->addWidget(
        removableValueLabel_,
        1,
        1);

    grid->addWidget(
        createFieldLabel(
            QStringLiteral("System Classification")),
        2,
        0);

    grid->addWidget(
        classificationValue,
        2,
        1);

    grid->addWidget(
        createFieldLabel(
            QStringLiteral("Seek Penalty")),
        3,
        0);

    grid->addWidget(
        seekPenaltyValue,
        3,
        1);

    layout->addLayout(
        grid);

    return card;
}

/*
 * Safety
 */
QWidget *DeviceDetailsPage::createSafetyCard()
{
    QFrame *card =
        createCard();

    auto *layout =
        new QVBoxLayout(card);

    layout->setContentsMargins(
        18,
        18,
        18,
        18);

    layout->setSpacing(
        12);

    auto *header =
        new QHBoxLayout;

    header->addWidget(
        createSectionTitle(
            QStringLiteral("Safety Status")));

    header->addStretch();

    auto *badge =
        new QLabel(
            QStringLiteral(
                "ASSESSMENT PENDING"),
            card);

    badge->setAlignment(
        Qt::AlignCenter);

    badge->setStyleSheet(
        pendingBadgeStyle());

    header->addWidget(
        badge);

    layout->addLayout(
        header);

    safetyDescriptionLabel_ =
        new QLabel(
            QStringLiteral(
                "The device has been discovered and classified. "
                "Fresh safety validation must pass before this physical target can be sanitized."),
            card);

    safetyDescriptionLabel_->setWordWrap(
        true);

    safetyDescriptionLabel_->setStyleSheet(
        secondaryTextStyle());

    layout->addWidget(
        safetyDescriptionLabel_);

    auto *checksLayout =
        new QHBoxLayout;

    checksLayout->setSpacing(
        10);

    const QStringList checks = {
        QStringLiteral("Device identity"),
        QStringLiteral("System disk"),
        QStringLiteral("Boot dependency"),
        QStringLiteral("Mounted volume")
    };

    for (const QString &check :
         checks)
    {
        auto *checkFrame =
            new QFrame(card);

        checkFrame->setStyleSheet(
            QStringLiteral(
                "QFrame {"
                "background:#F8FAFC;"
                "border:1px solid #EAECF0;"
                "border-radius:9px;"
                "}"));

        auto *checkLayout =
            new QHBoxLayout(
                checkFrame);

        checkLayout->setContentsMargins(
            9,
            7,
            9,
            7);

        checkLayout->setSpacing(
            6);

        auto *dot =
            new QLabel(
                QStringLiteral("•"),
                checkFrame);

        dot->setStyleSheet(
            QStringLiteral(
                "QLabel {"
                "color:#98A2B3;"
                "font-size:13px;"
                "font-weight:800;"
                "}"));

        auto *label =
            new QLabel(
                check,
                checkFrame);

        label->setStyleSheet(
            QStringLiteral(
                "QLabel {"
                "color:#667085;"
                "font-size:10px;"
                "font-weight:600;"
                "}"));

        checkLayout->addWidget(
            dot);

        checkLayout->addWidget(
            label);

        checksLayout->addWidget(
            checkFrame,
            1);
    }

    layout->addLayout(
        checksLayout);

    setSafetyPending();

    return card;
}

/*
 * Helpers
 */
QFrame *DeviceDetailsPage::createCard()
{
    auto *card =
        new QFrame(this);

    card->setObjectName(
        QStringLiteral("deviceCard"));

    card->setStyleSheet(
        cardStyle());

    return card;
}

QLabel *DeviceDetailsPage::createSectionTitle(
    const QString &title)
{
    auto *label =
        new QLabel(
            title,
            this);

    label->setStyleSheet(
        sectionTitleStyle());

    return label;
}

QLabel *DeviceDetailsPage::createFieldLabel(
    const QString &text)
{
    auto *label =
        new QLabel(
            text,
            this);

    label->setStyleSheet(
        fieldLabelStyle());

    return label;
}

QLabel *DeviceDetailsPage::createValueLabel(
    const QString &text)
{
    auto *label =
        new QLabel(
            text,
            this);

    label->setStyleSheet(
        valueLabelStyle());

    label->setTextInteractionFlags(
        Qt::TextSelectableByMouse);

    return label;
}

/*
 * Formatting
 */
QString DeviceDetailsPage::formatCapacity(
    std::uint64_t bytes) const
{
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;
    constexpr double GB = MB * 1024.0;
    constexpr double TB = GB * 1024.0;

    if (bytes >=
        static_cast<std::uint64_t>(TB))
    {
        return QString::number(
                   static_cast<double>(
                       bytes) / TB,
                   'f',
                   2)
            + QStringLiteral(" TB");
    }

    if (bytes >=
        static_cast<std::uint64_t>(GB))
    {
        return QString::number(
                   static_cast<double>(
                       bytes) / GB,
                   'f',
                   1)
            + QStringLiteral(" GB");
    }

    if (bytes >=
        static_cast<std::uint64_t>(MB))
    {
        return QString::number(
                   static_cast<double>(
                       bytes) / MB,
                   'f',
                   1)
            + QStringLiteral(" MB");
    }

    if (bytes >=
        static_cast<std::uint64_t>(KB))
    {
        return QString::number(
                   static_cast<double>(
                       bytes) / KB,
                   'f',
                   1)
            + QStringLiteral(" KB");
    }

    return QString::number(
               bytes)
        + QStringLiteral(" B");
}

QString DeviceDetailsPage::mediaTypeText(
    MediaType type) const
{
    switch (type)
    {
    case MediaType::HDD:
        return QStringLiteral("HDD");

    case MediaType::SSD:
        return QStringLiteral("SSD");

    default:
        return QStringLiteral("Unknown");
    }
}

QString DeviceDetailsPage::busTypeText(
    BusType type) const
{
    switch (type)
    {
    case BusType::SATA:
        return QStringLiteral("SATA");

    case BusType::SAS:
        return QStringLiteral("SAS");

    case BusType::USB:
        return QStringLiteral("USB");

    case BusType::NVMe:
        return QStringLiteral("NVMe");

    default:
        return QStringLiteral("Unknown");
    }
}

QString DeviceDetailsPage::deviceTypeText(
    DeviceType type) const
{
    switch (type)
    {
    case DeviceType::Internal:
        return QStringLiteral("Internal");

    case DeviceType::Removable:
        return QStringLiteral("Removable");

    default:
        return QStringLiteral("Unknown");
    }
}

/*
 * Safety state
 */
void DeviceDetailsPage::setSafetyPending()
{
    if (safetyStatusLabel_)
    {
        safetyStatusLabel_->setText(
            QStringLiteral(
                "Assessment Pending"));

        safetyStatusLabel_->setStyleSheet(
            QStringLiteral(
                "QLabel {"
                "background:transparent;"
                "border:none;"
                "color:#B45309;"
                "font-size:11px;"
                "font-weight:700;"
                "}"));
    }

    if (safetyDescriptionLabel_)
    {
        safetyDescriptionLabel_->setText(
            QStringLiteral(
                "Safety assessment has not been completed yet. "
                "Run fresh safety validation before sanitization."));
    }
}

void DeviceDetailsPage::setSafetyPassed()
{
    if (safetyStatusLabel_)
    {
        safetyStatusLabel_->setText(
            QStringLiteral(
                "Safety Checks Passed"));

        safetyStatusLabel_->setStyleSheet(
            QStringLiteral(
                "QLabel {"
                "background:transparent;"
                "border:none;"
                "color:#027A48;"
                "font-size:11px;"
                "font-weight:700;"
                "}"));
    }

    if (safetyDescriptionLabel_)
    {
        safetyDescriptionLabel_->setText(
            QStringLiteral(
                "The device passed the required safety checks "
                "and is eligible for the next authorized workflow step."));
    }
}

void DeviceDetailsPage::setSafetyFailed(
    const QString &message)
{
    if (safetyStatusLabel_)
    {
        safetyStatusLabel_->setText(
            QStringLiteral(
                "Sanitization Blocked"));

        safetyStatusLabel_->setStyleSheet(
            QStringLiteral(
                "QLabel {"
                "background:transparent;"
                "border:none;"
                "color:#B42318;"
                "font-size:11px;"
                "font-weight:700;"
                "}"));
    }

    if (safetyDescriptionLabel_)
    {
        safetyDescriptionLabel_->setText(
            message.isEmpty()
                ? QStringLiteral(
                    "The device did not pass the required safety checks.")
                : message);
    }
}

void DeviceDetailsPage::updateSafetyStatus(
    bool passed,
    const QString &message)
{
    if (passed)
    {
        setSafetyPassed();
        return;
    }

    setSafetyFailed(message);
}