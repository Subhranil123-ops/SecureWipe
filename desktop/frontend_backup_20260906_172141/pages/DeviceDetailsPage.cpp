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
    return
        "QFrame#deviceCard {"
        "background-color: #FFFFFF;"
        "border: 1px solid #E2E8F0;"
        "border-radius: 10px;"
        "}";
}


QString secondaryTextStyle()
{
    return
        "color: #667085;"
        "font-size: 12px;";
}


QString valueTextStyle()
{
    return
        "color: #172033;"
        "font-size: 13px;"
        "font-weight: 500;";
}


QString sectionTitleStyle()
{
    return
        "color: #172033;"
        "font-size: 14px;"
        "font-weight: 600;";
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


/*
 *  
 * Main UI
 *  
 */

void DeviceDetailsPage::setupUi()
{
    setStyleSheet(
    "DeviceDetailsPage {"
    "background-color: #F8FAFC;"
    "font-family: 'Segoe UI';"
    "}"
    ""
    "DeviceDetailsPage QLabel {"
    "background-color: transparent;"
    "border: none;"
    "}"
    ""
    "DeviceDetailsPage QFrame#deviceCard {"
    "background-color: #FFFFFF;"
    "border: 1px solid #E2E8F0;"
    "border-radius: 10px;"
    "}"
    ""
    "DeviceDetailsPage QPushButton {"
    "font-family: 'Segoe UI';"
    "}"
);


    auto *rootLayout =
        new QVBoxLayout(this);

    rootLayout->setContentsMargins(
        28,
        22,
        28,
        26
    );

    rootLayout->setSpacing(12);


    /*
     * ---------------------------------------------------------
     * Top navigation
     * ---------------------------------------------------------
     */

    auto *topLayout =
        new QHBoxLayout();

    auto *backButton =
        new QPushButton(
            "←  Back to Devices",
            this
        );

    backButton->setCursor(
        Qt::PointingHandCursor
    );

    backButton->setStyleSheet(
        "QPushButton {"
        "background: transparent;"
        "border: none;"
        "color: #2563EB;"
        "font-size: 12px;"
        "font-weight: 500;"
        "padding: 4px;"
        "}"
        ""
        "QPushButton:hover {"
        "color: #1D4ED8;"
        "}"
    );


    auto *refreshButton =
        new QPushButton(
            "↻  Refresh Details",
            this
        );

    refreshButton->setMinimumHeight(34);
    refreshButton->setCursor(
        Qt::PointingHandCursor
    );

    refreshButton->setStyleSheet(
        "QPushButton {"
        "background-color: #FFFFFF;"
        "border: 1px solid #D0D5DD;"
        "border-radius: 6px;"
        "color: #344054;"
        "padding: 7px 13px;"
        "font-size: 12px;"
        "font-weight: 500;"
        "}"
        ""
        "QPushButton:hover {"
        "background-color: #F8FAFC;"
        "border-color: #98A2B3;"
        "}"
    );


    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &DeviceDetailsPage::backRequested
    );


    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &DeviceDetailsPage::refreshRequested
    );


    topLayout->addWidget(
        backButton
    );

    topLayout->addStretch();

    topLayout->addWidget(
        refreshButton
    );


    rootLayout->addLayout(
        topLayout
    );


    /*
     * ---------------------------------------------------------
     * Page title
     * ---------------------------------------------------------
     */

    auto *title =
        new QLabel(
            "Device Details",
            this
        );

    title->setStyleSheet(
    "QLabel {"
    "background-color: transparent;"
    "border: none;"
    "color: #101828;"
    "font-size: 25px;"
    "font-weight: 700;"
    "padding: 0px;"
    "}"
);


    auto *subtitle =
        new QLabel(
            "Detailed information and classification of the selected device.",
            this
        );

    subtitle->setStyleSheet(
    "QLabel {"
    "background-color: transparent;"
    "border: none;"
    "color: #667085;"
    "font-size: 12px;"
    "padding: 0px;"
    "}"
    );


    rootLayout->addWidget(title);
    rootLayout->addWidget(subtitle);


    /*
     * ---------------------------------------------------------
     * Device header card
     * ---------------------------------------------------------
     */

    rootLayout->addWidget(
        createDeviceHeader()
    );


    /*
     * ---------------------------------------------------------
     * Classification + System Status
     * ---------------------------------------------------------
     */

    DeviceClassifier classifier;

    ClassificationResult classification =
        classifier.classify(device_);


    auto *cardsLayout =
        new QHBoxLayout();

    cardsLayout->setSpacing(14);


    QWidget *classificationCard =
        createClassificationCard(
            classification
        );

    QWidget *systemStatusCard =
        createSystemStatusCard(
            classification
        );


    cardsLayout->addWidget(
        classificationCard,
        1
    );

    cardsLayout->addWidget(
        systemStatusCard,
        1
    );


    rootLayout->addLayout(
        cardsLayout
    );


    /*
     * ---------------------------------------------------------
     * Safety
     * ---------------------------------------------------------
     */

    rootLayout->addWidget(
        createSafetyCard()
    );


    rootLayout->addStretch();
}


/*
 *  
 * Device Header
 *  
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
        20
    );

    layout->setSpacing(22);


    /*
     * Device icon placeholder
     *
     * We intentionally keep this simple for now.
     * Later we can use proper SSD/HDD icons.
     */

    QLabel *deviceIcon =
        new QLabel(
            "SSD",
            card
        );

    deviceIcon->setFixedSize(
        80,
        80
    );

    deviceIcon->setAlignment(
        Qt::AlignCenter
    );

    deviceIcon->setStyleSheet(
    "QLabel {"
    "background-color: #F8FAFC;"
    "border: 1px solid #D0D5DD;"
    "border-radius: 10px;"
    "color: #344054;"
    "font-size: 13px;"
    "font-weight: 700;"
    "}"
);


    /*
     * Main identity information
     */

    auto *identityLayout =
        new QVBoxLayout();

    identityLayout->setSpacing(
        5
    );


    auto *identityTop =
        new QHBoxLayout();


    modelLabel_ =
        new QLabel(
            QString::fromStdString(
                device_.getModel()
            ),
            card
        );

    modelLabel_->setStyleSheet(
    "color: #101828;"
    "font-size: 19px;"
    "font-weight: 700;"
);


    typeBadgeLabel_ =
        new QLabel(
            "Storage Device",
            card
        );

    typeBadgeLabel_->setAlignment(
        Qt::AlignCenter
    );

    typeBadgeLabel_->setStyleSheet(
        "QLabel {"
        "background-color: #EFF6FF;"
        "color: #2563EB;"
        "border-radius: 10px;"
        "padding: 4px 9px;"
        "font-size: 11px;"
        "font-weight: 600;"
        "}"
    );


    identityTop->addWidget(
        modelLabel_
    );

    identityTop->addSpacing(
        8
    );

    identityTop->addWidget(
        typeBadgeLabel_
    );

    identityTop->addStretch();


    identityLayout->addLayout(
        identityTop
    );


    /*
     * Field grid
     */

    auto *detailsGrid =
        new QGridLayout();

    detailsGrid->setHorizontalSpacing(
        28
    );

    detailsGrid->setVerticalSpacing(
        6
    );


    serialLabel_ =
        createValueLabel(
            QString::fromStdString(
                device_.getSerialNumber()
            )
        );

    capacityLabel_ =
        createValueLabel(
            formatCapacity(
                device_.getCapacityBytes()
            )
        );

    interfaceLabel_ =
        createValueLabel(
            QString::fromStdString(
                device_.getInterfaceType()
            )
        );

    devicePathLabel_ =
        createValueLabel(
            QString::fromStdString(
                device_.getDeviceId()
            )
        );


    detailsGrid->addWidget(
        createFieldLabel("Serial Number"),
        0,
        0
    );

    detailsGrid->addWidget(
        serialLabel_,
        0,
        1
    );

    detailsGrid->addWidget(
        createFieldLabel("Capacity"),
        1,
        0
    );

    detailsGrid->addWidget(
        capacityLabel_,
        1,
        1
    );

    detailsGrid->addWidget(
        createFieldLabel("Interface"),
        0,
        2
    );

    detailsGrid->addWidget(
        interfaceLabel_,
        0,
        3
    );

    detailsGrid->addWidget(
        createFieldLabel("Device Path"),
        1,
        2
    );

    detailsGrid->addWidget(
        devicePathLabel_,
        1,
        3
    );


    identityLayout->addSpacing(
        6
    );

    identityLayout->addLayout(
        detailsGrid
    );


    layout->addWidget(
        deviceIcon
    );

    layout->addLayout(
        identityLayout,
        1
    );


    /*
     * Right-side status
     */

    QFrame *statusFrame =
        new QFrame(card);

    statusFrame->setFixedWidth(
        150
    );

    statusFrame->setStyleSheet(
    "QFrame {"
    "background-color: #FFFBEB;"
    "border: 1px solid #FDE68A;"
    "border-radius: 8px;"
    "}"
    );


    auto *statusLayout =
        new QVBoxLayout(statusFrame);

    statusLayout->setContentsMargins(
        12,
        10,
        12,
        10
    );


    QLabel *shield =
    new QLabel(
        "!",
        statusFrame
    );

    shield->setAlignment(
        Qt::AlignCenter
    );

    shield->setStyleSheet(
    "color: #D97706;"
    "font-size: 20px;"
    "font-weight: 700;"
    );


    safetyStatusLabel_ =
    new QLabel(
        "Assessment Pending",
        statusFrame
    );

    safetyStatusLabel_->setAlignment(
        Qt::AlignCenter
    );

    safetyStatusLabel_->setStyleSheet(
        "color: #15803D;"
        "font-size: 12px;"
        "font-weight: 600;"
    );


    QLabel *statusDescription =
        new QLabel(
            "Device information available",
            statusFrame
        );

    statusDescription->setWordWrap(
        true
    );

    statusDescription->setAlignment(
        Qt::AlignCenter
    );

    statusDescription->setStyleSheet(
    "color: #92400E;"
    "font-size: 10px;"
    );


    statusLayout->addWidget(
        shield
    );

    statusLayout->addWidget(
        safetyStatusLabel_
    );

    statusLayout->addWidget(
        statusDescription
    );


    layout->addWidget(
        statusFrame
    );


    return card;
}


/*
 *  
 * Classification Card
 *  
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
        17,
        18,
        17
    );


    layout->addWidget(
        createSectionTitle(
            "Classification"
        )
    );


    auto *grid =
        new QGridLayout();

    grid->setVerticalSpacing(
        12
    );

    grid->setHorizontalSpacing(
        24
    );


    mediaTypeValueLabel_ =
        createValueLabel(
            mediaTypeText(
                classification.mediaType
            )
        );

    busTypeValueLabel_ =
        createValueLabel(
            busTypeText(
                classification.busType
            )
        );

    deviceTypeValueLabel_ =
        createValueLabel(
            deviceTypeText(
                classification.deviceType
            )
        );


    grid->addWidget(
        createFieldLabel("Media Type"),
        0,
        0
    );

    grid->addWidget(
        mediaTypeValueLabel_,
        0,
        1
    );


    grid->addWidget(
        createFieldLabel("Bus Type"),
        1,
        0
    );

    grid->addWidget(
        busTypeValueLabel_,
        1,
        1
    );


    grid->addWidget(
        createFieldLabel("Device Type"),
        2,
        0
    );

    grid->addWidget(
        deviceTypeValueLabel_,
        2,
        1
    );


    classificationStatusLabel_ =
        createValueLabel(
            "Detected"
        );

    classificationStatusLabel_->setStyleSheet(
        "color: #15803D;"
        "font-size: 12px;"
        "font-weight: 600;"
    );


    grid->addWidget(
        createFieldLabel("Status"),
        3,
        0
    );

    grid->addWidget(
        classificationStatusLabel_,
        3,
        1
    );


    layout->addLayout(
        grid
    );


    return card;
}


/*
 *  
 * System Status Card
 *  
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
        17,
        18,
        17
    );


    layout->addWidget(
        createSectionTitle(
            "System Status"
        )
    );


    auto *grid =
        new QGridLayout();

    grid->setVerticalSpacing(
        12
    );


    systemDiskValueLabel_ =
        createValueLabel(
            device_.isSystemDisk()
                ? "Yes"
                : "No"
        );


    removableValueLabel_ =
        createValueLabel(
            device_.isRemovable()
                ? "Yes"
                : "No"
        );


    QLabel *systemStatus =
        createValueLabel(
            classification.isSystemDisk
                ? "System Device"
                : "Non-System Device"
        );


    QLabel *seekPenalty =
        createValueLabel(
            device_.hasSeekPenalty()
                ? "Detected"
                : "Not Detected"
        );


    grid->addWidget(
        createFieldLabel("System Disk"),
        0,
        0
    );

    grid->addWidget(
        systemDiskValueLabel_,
        0,
        1
    );


    grid->addWidget(
        createFieldLabel("Removable"),
        1,
        0
    );

    grid->addWidget(
        removableValueLabel_,
        1,
        1
    );


    grid->addWidget(
        createFieldLabel("System Classification"),
        2,
        0
    );

    grid->addWidget(
        systemStatus,
        2,
        1
    );


    grid->addWidget(
        createFieldLabel("Seek Penalty"),
        3,
        0
    );

    grid->addWidget(
        seekPenalty,
        3,
        1
    );


    layout->addLayout(
        grid
    );


    return card;
}


/*
 *  
 * Safety Card
 *  
 */

QWidget *DeviceDetailsPage::createSafetyCard()
{
    QFrame *card =
        createCard();


    auto *layout =
        new QVBoxLayout(card);

    layout->setContentsMargins(
        16,
        15,
        16,
        15
    );


    auto *header =
        new QHBoxLayout();


    header->addWidget(
        createSectionTitle(
            "Safety Status"
        )
    );

    header->addStretch();


    QLabel *pendingBadge =
        new QLabel(
            "Assessment pending",
            card
        );

    pendingBadge->setStyleSheet(
        "QLabel {"
        "background-color: #FFF7ED;"
        "color: #C2410C;"
        "border-radius: 10px;"
        "padding: 4px 9px;"
        "font-size: 10px;"
        "font-weight: 600;"
        "}"
    );


    header->addWidget(
        pendingBadge
    );


    layout->addLayout(
        header
    );


    safetyDescriptionLabel_ =
        new QLabel(
            "The device has been discovered and classified. "
            "Detailed safety checks will be connected to the "
            "SafetyEngine before any sanitization operation.",
            card
        );

    safetyDescriptionLabel_->setWordWrap(
        true
    );

    safetyDescriptionLabel_->setStyleSheet(
        "color: #667085;"
        "font-size: 12px;"
    );


    layout->addWidget(
        safetyDescriptionLabel_
    );


    auto *checksLayout =
        new QHBoxLayout();

    checksLayout->setSpacing(
        12
    );


    const QStringList checks = {
        "Device identity",
        "System disk",
        "Boot dependency",
        "Mounted volume"
    };


    for (const QString &check : checks)
    {
        QLabel *checkLabel =
            new QLabel(
                "•  " + check,
                card
            );

        checkLabel->setStyleSheet(
            "color: #667085;"
            "font-size: 11px;"
        );

        checksLayout->addWidget(
            checkLabel
        );
    }


    layout->addLayout(
        checksLayout
    );

    setSafetyPending();
    return card;
}


/*
 *  
 * Helpers
 *  
 */

QFrame *DeviceDetailsPage::createCard()
{
    QFrame *card =
        new QFrame(this);

    card->setObjectName(
        "deviceCard"
    );

    card->setStyleSheet(
        cardStyle()
    );

    return card;
}


QLabel *DeviceDetailsPage::createSectionTitle(
    const QString &title)
{
    QLabel *label =
        new QLabel(
            title,
            this
        );

    label->setStyleSheet(
        sectionTitleStyle()
    );

    return label;
}


QLabel *DeviceDetailsPage::createFieldLabel(
    const QString &text)
{
    QLabel *label =
        new QLabel(
            text,
            this
        );

    label->setStyleSheet(
        secondaryTextStyle()
    );

    return label;
}


QLabel *DeviceDetailsPage::createValueLabel(
    const QString &text)
{
    QLabel *label =
        new QLabel(
            text,
            this
        );

    label->setStyleSheet(
        valueTextStyle()
    );

    label->setTextInteractionFlags(
        Qt::TextSelectableByMouse
    );

    return label;
}


/*
 *  
 * Formatting
 *  
 */

QString DeviceDetailsPage::formatCapacity(
    std::uint64_t bytes) const
{
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;
    constexpr double GB = MB * 1024.0;
    constexpr double TB = GB * 1024.0;


    if (bytes >= static_cast<std::uint64_t>(TB))
    {
        return QString::number(
            static_cast<double>(bytes) / TB,
            'f',
            2
        ) + " TB";
    }


    if (bytes >= static_cast<std::uint64_t>(GB))
    {
        return QString::number(
            static_cast<double>(bytes) / GB,
            'f',
            1
        ) + " GB";
    }


    if (bytes >= static_cast<std::uint64_t>(MB))
    {
        return QString::number(
            static_cast<double>(bytes) / MB,
            'f',
            1
        ) + " MB";
    }


    if (bytes >= static_cast<std::uint64_t>(KB))
    {
        return QString::number(
            static_cast<double>(bytes) / KB,
            'f',
            1
        ) + " KB";
    }


    return QString::number(bytes) + " B";
}


QString DeviceDetailsPage::mediaTypeText(
    MediaType type) const
{
    switch (type)
    {
        case MediaType::HDD:
            return "HDD";

        case MediaType::SSD:
            return "SSD";

        default:
            return "Unknown";
    }
}


QString DeviceDetailsPage::busTypeText(
    BusType type) const
{
    switch (type)
    {
        case BusType::SATA:
            return "SATA";

        case BusType::SAS:
            return "SAS";

        case BusType::USB:
            return "USB";

        case BusType::NVMe:
            return "NVMe";

        default:
            return "Unknown";
    }
}


QString DeviceDetailsPage::deviceTypeText(
    DeviceType type) const
{
    switch (type)
    {
        case DeviceType::Internal:
            return "Internal";

        case DeviceType::Removable:
            return "Removable";

        default:
            return "Unknown";
    }
}

void DeviceDetailsPage::setSafetyPending()
{
    if (!safetyStatusLabel_)
    {
        return;
    }

    safetyStatusLabel_->setText(
        "Assessment Pending"
    );

    safetyStatusLabel_->setStyleSheet(
        "QLabel {"
        "color: #B45309;"
        "font-size: 12px;"
        "font-weight: 600;"
        "}"
    );


    if (safetyDescriptionLabel_)
    {
        safetyDescriptionLabel_->setText(
            "Safety assessment has not been completed yet."
        );
    }
}


void DeviceDetailsPage::setSafetyPassed()
{
    if (!safetyStatusLabel_)
    {
        return;
    }

    safetyStatusLabel_->setText(
        "Safety Checks Passed"
    );

    safetyStatusLabel_->setStyleSheet(
        "QLabel {"
        "color: #15803D;"
        "font-size: 12px;"
        "font-weight: 600;"
        "}"
    );


    if (safetyDescriptionLabel_)
    {
        safetyDescriptionLabel_->setText(
            "The device passed the required safety checks."
        );
    }
}


void DeviceDetailsPage::setSafetyFailed(
    const QString &message)
{
    if (!safetyStatusLabel_)
    {
        return;
    }

    safetyStatusLabel_->setText(
        "Sanitization Blocked"
    );

    safetyStatusLabel_->setStyleSheet(
        "QLabel {"
        "color: #B42318;"
        "font-size: 12px;"
        "font-weight: 600;"
        "}"
    );


    if (safetyDescriptionLabel_)
    {
        safetyDescriptionLabel_->setText(
            message.isEmpty()
                ? "The device did not pass the required safety checks."
                : message
        );
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