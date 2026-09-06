#include "ForensicPage.h"

#include "../controllers/DeviceController.h"
#include "../dialogs/ForensicEvidenceDialog.h"
#include "../dialogs/ForensicScanDialog.h"
#include "../services/ForensicService.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCoreApplication>
#include <QEventLoop>
#include <QColor>
#include <QBrush>
#include <QFont>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QTimer>

namespace
{

    QLabel *makeTextLabel(
        const QString &text,
        QWidget *parent,
        int size,
        const QString &color,
        int weight = 400)
    {
        auto *label =
            new QLabel(
                text,
                parent);

        label->setStyleSheet(
            QStringLiteral(
                "QLabel {"
                "background: transparent;"
                "border: none;"
                "color: %1;"
                "font-size: %2px;"
                "font-weight: %3;"
                "}")
                .arg(color)
                .arg(size)
                .arg(weight));

        return label;
    }

    QLabel *makeFieldLabel(
        const QString &text,
        QWidget *parent)
    {
        return makeTextLabel(
            text.toUpper(),
            parent,
            10,
            QStringLiteral("#667085"),
            700);
    }

    QLabel *makeDescription(
        const QString &text,
        QWidget *parent)
    {
        auto *label =
            makeTextLabel(
                text,
                parent,
                12,
                QStringLiteral("#667085"));

        label->setWordWrap(
            true);

        return label;
    }

    QFrame *makeDivider(
        QWidget *parent)
    {
        auto *line =
            new QFrame(parent);

        line->setFrameShape(
            QFrame::HLine);

        line->setFixedHeight(
            1);

        line->setStyleSheet(
            "QFrame {"
            "background:#EAECF0;"
            "border:none;"
            "}");

        return line;
    }

}

ForensicPage::ForensicPage(
    DeviceController *deviceController,
    QWidget *parent)
    : QWidget(parent),
      deviceController_(deviceController),
      forensicService_(
          new ForensicService(this)),
      sourceTypeCombo_(nullptr),
      sourceStack_(nullptr),
      deviceCombo_(nullptr),
      imagePathEdit_(nullptr),
      deviceInfoLabel_(nullptr),
      systemDiskWarningLabel_(nullptr),
      sourceStatusLabel_(nullptr),
      sourceBadgeLabel_(nullptr),
      refreshButton_(nullptr),
      browseButton_(nullptr),
      scanButton_(nullptr),
      recoveredValue_(nullptr),
      validatedValue_(nullptr),
      highConfidenceValue_(nullptr),
      recoveredBytesValue_(nullptr),
      candidatesValue_(nullptr),
      scanStateLabel_(nullptr),
      resultsSummaryLabel_(nullptr),
      emptyStateIconLabel_(nullptr),
      emptyStateTitleLabel_(nullptr),
      emptyStateBodyLabel_(nullptr),
      resultsTable_(nullptr)
{
    buildUi();

    connect(
        forensicService_,
        &ForensicService::scanFinished,
        this,
        [this]()
        {
            renderResults();

            scanButton_->setEnabled(
                true);
        });

    connect(
        forensicService_,
        &ForensicService::scanFailed,
        this,
        [this](const QString &message)
        {
            scanButton_->setEnabled(
                true);

            setScanState(
                QStringLiteral(
                    "Scan failed · %1")
                    .arg(message),
                false);

            setEmptyState(
                QStringLiteral(
                    "Forensic scan could not be completed"),
                message,
                QStringLiteral("!"));
        });

    if (deviceController_)
    {
        connect(
            deviceController_,
            &DeviceController::devicesUpdated,
            this,
            &ForensicPage::refreshDeviceList);

        connect(
            deviceController_,
            &DeviceController::discoveryFailed,
            this,
            [this](const QString &message)
            {
                sourceStatusLabel_->setText(
                    message);

                sourceStatusLabel_->setStyleSheet(
                    "QLabel {"
                    "background:transparent;"
                    "border:none;"
                    "color:#B42318;"
                    "font-size:11px;"
                    "font-weight:600;"
                    "}");
            });
    }

    refreshDeviceList();

    updateSourceState();
}

void ForensicPage::buildUi()
{
    setObjectName(
        QStringLiteral(
            "forensicRoot"));

    setStyleSheet(
        "QWidget#forensicRoot {"
        "background:#F5F7FA;"
        "}"

        "QLabel {"
        "background:transparent;"
        "border:none;"
        "}"

        "QComboBox {"
        "background:#FFFFFF;"
        "color:#172033;"
        "border:1px solid #D0D5DD;"
        "border-radius:8px;"
        "padding:8px 11px;"
        "font-size:12px;"
        "min-height:20px;"
        "}"

        "QComboBox:hover {"
        "border-color:#98A2B3;"
        "}"

        "QComboBox:focus {"
        "border-color:#84ADFF;"
        "}"

        "QComboBox::drop-down {"
        "border:none;"
        "width:26px;"
        "}"

        "QLineEdit {"
        "background:#FFFFFF;"
        "color:#172033;"
        "border:1px solid #D0D5DD;"
        "border-radius:8px;"
        "padding:8px 11px;"
        "font-size:12px;"
        "}"

        "QLineEdit:hover {"
        "border-color:#98A2B3;"
        "}"

        "QLineEdit:focus {"
        "border-color:#84ADFF;"
        "}"

        "QPushButton#forensicPrimaryButton {"
        "background:#2563EB;"
        "color:#FFFFFF;"
        "border:none;"
        "border-radius:8px;"
        "padding:10px 16px;"
        "font-size:12px;"
        "font-weight:700;"
        "}"

        "QPushButton#forensicPrimaryButton:hover {"
        "background:#1D4ED8;"
        "}"

        "QPushButton#forensicPrimaryButton:pressed {"
        "background:#1E40AF;"
        "}"

        "QPushButton#forensicPrimaryButton:disabled {"
        "background:#D0D5DD;"
        "color:#98A2B3;"
        "}"

        "QPushButton#forensicSecondaryButton {"
        "background:#FFFFFF;"
        "color:#344054;"
        "border:1px solid #D0D5DD;"
        "border-radius:8px;"
        "padding:9px 12px;"
        "font-size:11px;"
        "font-weight:600;"
        "}"

        "QPushButton#forensicSecondaryButton:hover {"
        "background:#F8FAFC;"
        "border-color:#98A2B3;"
        "}"

        "QScrollArea {"
        "border:none;"
        "background:transparent;"
        "}"

        "QTableWidget {"
        "background:#FFFFFF;"
        "color:#172033;"
        "border:1px solid #E4E7EC;"
        "border-radius:10px;"
        "gridline-color:#F2F4F7;"
        "font-size:11px;"
        "}"

        "QTableWidget::item {"
        "padding:8px 9px;"
        "border-bottom:1px solid #F2F4F7;"
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

        "QScrollBar:vertical {"
        "background:transparent;"
        "width:9px;"
        "margin:4px;"
        "}"

        "QScrollBar::handle:vertical {"
        "background:#CBD5E1;"
        "border-radius:4px;"
        "min-height:35px;"
        "}"

        "QScrollBar::add-line:vertical,"
        "QScrollBar::sub-line:vertical {"
        "height:0px;"
        "}");

    auto *outer =
        new QVBoxLayout(this);

    outer->setContentsMargins(
        0,
        0,
        0,
        0);

    auto *scroll =
        new QScrollArea(this);

    scroll->setWidgetResizable(
        true);

    scroll->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);

    scroll->setFrameShape(
        QFrame::NoFrame);

    auto *content =
        new QWidget();

    content->setStyleSheet(
        "QWidget {"
        "background:#F5F7FA;"
        "}");

    auto *layout =
        new QVBoxLayout(
            content);

    layout->setContentsMargins(
        30,
        26,
        30,
        32);

    layout->setSpacing(
        16);

    // ---------------------------------------------------------
    // Header
    // ---------------------------------------------------------

    auto *header =
        new QHBoxLayout();

    header->setSpacing(
        16);

    auto *titleBlock =
        new QVBoxLayout();

    titleBlock->setSpacing(
        4);

    auto *eyebrow =
        makeTextLabel(
            QStringLiteral(
                "SECUREWIPE  /  FORENSIC WORKSPACE"),
            content,
            10,
            QStringLiteral("#2563EB"),
            700);

    auto *title =
        makeTextLabel(
            QStringLiteral(
                "Forensic recovery"),
            content,
            26,
            QStringLiteral("#101828"),
            700);

    auto *subtitle =
        makeDescription(
            QStringLiteral(
                "Recover and validate deleted JPEG evidence "
                "from a physical device or forensic image. "
                "Acquisition stays read-only throughout the workflow."),
            content);

    subtitle->setMaximumWidth(
        760);

    titleBlock->addWidget(
        eyebrow);

    titleBlock->addWidget(
        title);

    titleBlock->addWidget(
        subtitle);

    header->addLayout(
        titleBlock,
        1);

    sourceBadgeLabel_ =
        makeBadge(
            QStringLiteral(
                "READ-ONLY"),
            QStringLiteral(
                "#EFF6FF"),
            QStringLiteral(
                "#1D4ED8"),
            content);

    sourceBadgeLabel_->setMinimumHeight(
        28);

    header->addWidget(
        sourceBadgeLabel_,
        0,
        Qt::AlignTop);

    layout->addLayout(
        header);

    // ---------------------------------------------------------
    // Source card
    // ---------------------------------------------------------

    auto *sourceCard =
        makeCard(content);

    auto *sourceLayout =
        new QVBoxLayout(
            sourceCard);

    sourceLayout->setContentsMargins(
        20,
        18,
        20,
        18);

    sourceLayout->setSpacing(
        14);

    auto *sourceHeader =
        new QHBoxLayout();

    auto *sourceTitleBlock =
        new QVBoxLayout();

    sourceTitleBlock->setSpacing(
        3);

    sourceTitleBlock->addWidget(
        makeTextLabel(
            QStringLiteral(
                "Evidence source"),
            sourceCard,
            15,
            QStringLiteral("#101828"),
            700));

    sourceTitleBlock->addWidget(
        makeDescription(
            QStringLiteral(
                "Choose the exact source to examine. "
                "No sanitization or write operation is performed "
                "by forensic acquisition."),
            sourceCard));

    sourceHeader->addLayout(
        sourceTitleBlock,
        1);

    sourceLayout->addLayout(
        sourceHeader);

    sourceLayout->addWidget(
        makeDivider(
            sourceCard));

    auto *typeGrid =
        new QGridLayout();

    typeGrid->setHorizontalSpacing(
        14);

    typeGrid->setVerticalSpacing(
        7);

    typeGrid->addWidget(
        makeFieldLabel(
            QStringLiteral(
                "SOURCE TYPE"),
            sourceCard),
        0,
        0);

    sourceTypeCombo_ =
        new QComboBox(
            sourceCard);

    sourceTypeCombo_->addItems({QStringLiteral(
                                    "Physical device"),
                                QStringLiteral(
                                    "Forensic image / raw source")});

    sourceTypeCombo_->setMinimumHeight(
        38);

    sourceTypeCombo_->setMinimumWidth(
        250);

    typeGrid->addWidget(
        sourceTypeCombo_,
        1,
        0);

    typeGrid->setColumnStretch(
        0,
        1);

    sourceLayout->addLayout(
        typeGrid);

    sourceStack_ =
        new QStackedWidget(
            sourceCard);

    // ---------------------------------------------------------
    // Physical device source
    // ---------------------------------------------------------

    auto *deviceSource =
        new QWidget(
            sourceStack_);

    auto *deviceLayout =
        new QVBoxLayout(
            deviceSource);

    deviceLayout->setContentsMargins(
        0,
        0,
        0,
        0);

    deviceLayout->setSpacing(
        8);

    auto *deviceRow =
        new QHBoxLayout();

    deviceRow->setSpacing(
        8);

    deviceCombo_ =
        new QComboBox(
            deviceSource);

    deviceCombo_->setMinimumHeight(
        40);

    deviceCombo_->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    refreshButton_ =
        new QPushButton(
            QStringLiteral(
                "Refresh"),
            deviceSource);

    refreshButton_->setObjectName(
        QStringLiteral(
            "forensicSecondaryButton"));

    refreshButton_->setCursor(
        Qt::PointingHandCursor);

    refreshButton_->setMinimumHeight(
        40);

    deviceRow->addWidget(
        deviceCombo_,
        1);

    deviceRow->addWidget(
        refreshButton_);

    deviceLayout->addLayout(
        deviceRow);

    deviceInfoLabel_ =
        makeDescription(
            QStringLiteral(
                "No device selected."),
            deviceSource);

    deviceInfoLabel_->setTextInteractionFlags(
        Qt::TextSelectableByMouse);

    deviceInfoLabel_->setStyleSheet(
        "QLabel {"
        "background:#F8FAFC;"
        "border:1px solid #EAECF0;"
        "border-radius:7px;"
        "color:#667085;"
        "font-size:11px;"
        "padding:8px 10px;"
        "}");

    deviceLayout->addWidget(
        deviceInfoLabel_);

    systemDiskWarningLabel_ =
        new QLabel(
            deviceSource);

    systemDiskWarningLabel_->setWordWrap(
        true);

    systemDiskWarningLabel_->setStyleSheet(
        "QLabel {"
        "background:#FFFAEB;"
        "border:1px solid #FEDF89;"
        "border-radius:8px;"
        "color:#B54708;"
        "font-size:11px;"
        "padding:9px 10px;"
        "}");

    systemDiskWarningLabel_->setText(
        QStringLiteral(
            "System disk selected. Acquisition remains "
            "read-only, but scanning may take longer and "
            "can inspect sensitive deleted data."));

    systemDiskWarningLabel_->hide();

    deviceLayout->addWidget(
        systemDiskWarningLabel_);

    sourceStack_->addWidget(
        deviceSource);

    // ---------------------------------------------------------
    // File / image source
    // ---------------------------------------------------------

    auto *fileSource =
        new QWidget(
            sourceStack_);

    auto *fileLayout =
        new QVBoxLayout(
            fileSource);

    fileLayout->setContentsMargins(
        0,
        0,
        0,
        0);

    fileLayout->setSpacing(
        8);

    auto *fileRow =
        new QHBoxLayout();

    fileRow->setSpacing(
        8);

    imagePathEdit_ =
        new QLineEdit(
            fileSource);

    imagePathEdit_->setPlaceholderText(
        QStringLiteral(
            "Select a forensic image or raw source..."));

    imagePathEdit_->setMinimumHeight(
        40);

    imagePathEdit_->setReadOnly(
        true);

    browseButton_ =
        new QPushButton(
            QStringLiteral(
                "Browse"),
            fileSource);

    browseButton_->setObjectName(
        QStringLiteral(
            "forensicSecondaryButton"));

    browseButton_->setCursor(
        Qt::PointingHandCursor);

    browseButton_->setMinimumHeight(
        40);

    fileRow->addWidget(
        imagePathEdit_,
        1);

    fileRow->addWidget(
        browseButton_);

    fileLayout->addLayout(
        fileRow);

    auto *fileInfo =
        makeDescription(
            QStringLiteral(
                "Forensic image files are opened read-only. "
                "The current collector primarily searches for JPEG artifacts."),
            fileSource);

    fileLayout->addWidget(
        fileInfo);

    sourceStack_->addWidget(
        fileSource);

    sourceLayout->addWidget(
        sourceStack_);

    // ---------------------------------------------------------
    // Source status
    // ---------------------------------------------------------

    sourceStatusLabel_ =
        makeDescription(
            QStringLiteral(
                "Select a source to begin."),
            sourceCard);

    sourceStatusLabel_->setTextInteractionFlags(
        Qt::TextSelectableByMouse);

    sourceLayout->addWidget(
        sourceStatusLabel_);

    auto *scanRow =
        new QHBoxLayout();

    scanRow->setSpacing(
        10);

    auto *readOnlyInfo =
        makeTextLabel(
            QStringLiteral(
                "🔒  READ-ONLY ACQUISITION"),
            sourceCard,
            10,
            QStringLiteral("#027A48"),
            700);

    scanRow->addWidget(
        readOnlyInfo);

    scanRow->addStretch();

    scanButton_ =
        new QPushButton(
            QStringLiteral(
                "Start forensic scan  →"),
            sourceCard);

    scanButton_->setObjectName(
        QStringLiteral(
            "forensicPrimaryButton"));

    scanButton_->setCursor(
        Qt::PointingHandCursor);

    scanButton_->setMinimumHeight(
        42);

    scanButton_->setMinimumWidth(
        180);

    scanRow->addWidget(
        scanButton_);

    sourceLayout->addLayout(
        scanRow);

    layout->addWidget(
        sourceCard);

    // ---------------------------------------------------------
    // Metrics
    // ---------------------------------------------------------

    auto *metrics =
        new QGridLayout();

    metrics->setHorizontalSpacing(
        12);

    metrics->setVerticalSpacing(
        12);

    auto addMetric =
        [this, metrics](
            int column,
            const QString &label,
            QLabel **value,
            const QString &caption)
    {
        auto *card =
            makeCard(
                this);

        auto *cardLayout =
            new QVBoxLayout(
                card);

        cardLayout->setContentsMargins(
            16,
            14,
            16,
            14);

        cardLayout->setSpacing(
            4);

        cardLayout->addWidget(
            makeTextLabel(
                label,
                card,
                10,
                QStringLiteral("#667085"),
                600));

        *value =
            makeMetricValue(
                card);

        cardLayout->addWidget(
            *value);

        cardLayout->addWidget(
            makeTextLabel(
                caption,
                card,
                10,
                QStringLiteral("#98A2B3")));

        metrics->addWidget(
            card,
            0,
            column);
    };

    addMetric(
        0,
        QStringLiteral("RECOVERED"),
        &recoveredValue_,
        QStringLiteral("accepted artifacts"));

    addMetric(
        1,
        QStringLiteral("VALIDATED"),
        &validatedValue_,
        QStringLiteral("passed validation"));

    addMetric(
        2,
        QStringLiteral("HIGH CONFIDENCE"),
        &highConfidenceValue_,
        QStringLiteral("confidence score ≥ 80"));

    addMetric(
        3,
        QStringLiteral("RECOVERED BYTES"),
        &recoveredBytesValue_,
        QStringLiteral("total accepted artifact size"));

    addMetric(
        4,
        QStringLiteral("CANDIDATES"),
        &candidatesValue_,
        QStringLiteral("JPEG candidates detected"));

    

    layout->addLayout(
        metrics);

    // ---------------------------------------------------------
    // Scan status card
    // ---------------------------------------------------------

    auto *statusCard =
        makeCard(content);

    auto *statusLayout =
        new QHBoxLayout(
            statusCard);

    statusLayout->setContentsMargins(
        16,
        13,
        16,
        13);

    statusLayout->setSpacing(
        10);

    scanStateLabel_ =
        makeTextLabel(
            QStringLiteral(
                "Ready for forensic acquisition"),
            statusCard,
            11,
            QStringLiteral("#475467"),
            600);

    statusLayout->addWidget(
        scanStateLabel_,
        1);

    resultsSummaryLabel_ =
        makeTextLabel(
            QStringLiteral(
                "No scan performed"),
            statusCard,
            10,
            QStringLiteral("#98A2B3"),
            500);

    resultsSummaryLabel_->setAlignment(
        Qt::AlignRight |
        Qt::AlignVCenter);

    statusLayout->addWidget(
        resultsSummaryLabel_);

    layout->addWidget(
        statusCard);

    // ---------------------------------------------------------
    // Evidence results
    // ---------------------------------------------------------

    auto *resultsCard =
        makeCard(content);

    auto *resultsLayout =
        new QVBoxLayout(
            resultsCard);

    resultsLayout->setContentsMargins(
        18,
        17,
        18,
        18);

    resultsLayout->setSpacing(
        12);

    auto *resultsHeader =
        new QHBoxLayout();

    auto *resultsTitleBlock =
        new QVBoxLayout();

    resultsTitleBlock->setSpacing(
        3);

    resultsTitleBlock->addWidget(
        makeTextLabel(
            QStringLiteral(
                "Evidence results"),
            resultsCard,
            15,
            QStringLiteral("#101828"),
            700));

    resultsTitleBlock->addWidget(
        makeDescription(
            QStringLiteral(
                "Only artifacts that pass the configured "
                "validation pipeline are accepted into this evidence set."),
            resultsCard));

    resultsHeader->addLayout(
        resultsTitleBlock,
        1);

    resultsHeader->addWidget(
        makeBadge(
            QStringLiteral(
                "VALIDATED + HASHED"),
            QStringLiteral("#ECFDF3"),
            QStringLiteral("#027A48"),
            resultsCard),
        0,
        Qt::AlignTop);

    resultsLayout->addLayout(
        resultsHeader);

    resultsTable_ =
        new QTableWidget(
            resultsCard);

    resultsTable_->setColumnCount(
        7);

    resultsTable_->setHorizontalHeaderLabels({QStringLiteral("ARTIFACT"),
                                              QStringLiteral("TYPE"),
                                              QStringLiteral("OFFSET"),
                                              QStringLiteral("SIZE"),
                                              QStringLiteral("CONFIDENCE"),
                                              QStringLiteral("SHA-256"),
                                              QStringLiteral("ACTION")});

    resultsTable_->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    resultsTable_->setSelectionMode(
        QAbstractItemView::SingleSelection);

    resultsTable_->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    resultsTable_->setFocusPolicy(
        Qt::NoFocus);

    resultsTable_->setAlternatingRowColors(
        false);

    resultsTable_->setShowGrid(
        false);

    resultsTable_->verticalHeader()
        ->setVisible(false);

    resultsTable_->horizontalHeader()
        ->setStretchLastSection(false);

    resultsTable_->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::ResizeToContents);

    resultsTable_->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::ResizeToContents);

    resultsTable_->horizontalHeader()
        ->setSectionResizeMode(
            2,
            QHeaderView::ResizeToContents);

    resultsTable_->horizontalHeader()
        ->setSectionResizeMode(
            3,
            QHeaderView::ResizeToContents);

    resultsTable_->horizontalHeader()
        ->setSectionResizeMode(
            4,
            QHeaderView::ResizeToContents);

    resultsTable_->horizontalHeader()
        ->setSectionResizeMode(
            5,
            QHeaderView::Stretch);

    resultsTable_->horizontalHeader()
        ->setSectionResizeMode(
            6,
            QHeaderView::ResizeToContents);

    resultsTable_->setMinimumHeight(
        210);

    resultsLayout->addWidget(
        resultsTable_);

    auto *emptyState =
        new QFrame(
            resultsCard);

    emptyState->setObjectName(
        QStringLiteral(
            "forensicEmptyState"));

    emptyState->setStyleSheet(
        "QFrame#forensicEmptyState {"
        "background:#F8FAFC;"
        "border:1px dashed #D0D5DD;"
        "border-radius:10px;"
        "}");

    auto *emptyLayout =
        new QVBoxLayout(
            emptyState);

    emptyLayout->setContentsMargins(
        24,
        25,
        24,
        25);

    emptyLayout->setSpacing(
        7);

    emptyStateIconLabel_ =
        makeTextLabel(
            QStringLiteral(
                "◌"),
            emptyState,
            28,
            QStringLiteral("#98A2B3"),
            400);

    emptyStateIconLabel_->setAlignment(
        Qt::AlignCenter);

    emptyStateTitleLabel_ =
        makeTextLabel(
            QStringLiteral(
                "No evidence yet"),
            emptyState,
            14,
            QStringLiteral("#344054"),
            700);

    emptyStateTitleLabel_->setAlignment(
        Qt::AlignCenter);

    emptyStateBodyLabel_ =
        makeDescription(
            QStringLiteral(
                "Select a source and start a forensic scan "
                "to populate this workspace."),
            emptyState);

    emptyStateBodyLabel_->setAlignment(
        Qt::AlignCenter);

    emptyStateBodyLabel_->setMaximumWidth(
        650);

    emptyLayout->addWidget(
        emptyStateIconLabel_);

    emptyLayout->addWidget(
        emptyStateTitleLabel_);

    emptyLayout->addWidget(
        emptyStateBodyLabel_);

    resultsLayout->addWidget(
        emptyState);

    layout->addWidget(
        resultsCard);

    auto *footer =
        makeTextLabel(
            QStringLiteral(
                "Forensic acquisition is read-only. "
                "Evidence remains in the current desktop session "
                "and is not automatically persisted as a formal report."),
            content,
            10,
            QStringLiteral("#98A2B3"));

    footer->setWordWrap(
        true);

    layout->addWidget(
        footer);

    layout->addStretch();

    scroll->setWidget(
        content);

    outer->addWidget(
        scroll);

    // ---------------------------------------------------------
    // Connections
    // ---------------------------------------------------------

    connect(
        sourceTypeCombo_,
        QOverload<int>::of(
            &QComboBox::currentIndexChanged),
        this,
        [this](int index)
        {
            sourceStack_->setCurrentIndex(
                index);

            updateSourceState();
        });

    connect(
        deviceCombo_,
        QOverload<int>::of(
            &QComboBox::currentIndexChanged),
        this,
        [this](int index)
        {
            Q_UNUSED(index);
            updateDeviceSelection();
            updateSourceState();
        });

    connect(
        refreshButton_,
        &QPushButton::clicked,
        this,
        [this]()
        {
            if (deviceController_)
            {
                deviceController_->refreshDevices();
            }
            else
            {
                refreshDeviceList();
            }
        });

    connect(
        browseButton_,
        &QPushButton::clicked,
        this,
        [this]()
        {
            const QString path =
                QFileDialog::getOpenFileName(
                    this,
                    QStringLiteral(
                        "Select forensic image or raw source"),
                    QString(),
                    QStringLiteral(
                        "Forensic images (*.dd *.raw *.img *.001 *.e01);;"
                        "All files (*.*)"));

            if (
                path.isEmpty())
            {
                return;
            }

            imagePathEdit_->setText(
                path);

            updateSourceState();
        });

    connect(
        scanButton_,
        &QPushButton::clicked,
        this,
        &ForensicPage::startScan);

    connect(
        resultsTable_,
        &QTableWidget::cellDoubleClicked,
        this,
        &ForensicPage::showEvidenceDetails);
}

void ForensicPage::refreshDeviceList()
{
    if (!deviceCombo_)
    {
        return;
    }

    const QString previousDeviceId =
        deviceCombo_->currentData().toString();

    deviceCombo_->blockSignals(
        true);

    deviceCombo_->clear();

    if (!deviceController_)
    {
        deviceCombo_->addItem(
            QStringLiteral(
                "Device discovery unavailable"));

        deviceCombo_->setEnabled(
            false);

        deviceCombo_->blockSignals(
            false);

        updateDeviceSelection();

        return;
    }

    const std::vector<StorageDevice> &devices =
        deviceController_->devices();

    int restoredIndex =
        -1;

    for (
        int index = 0;
        index <
        static_cast<int>(
            devices.size());
        ++index)
    {
        const StorageDevice &device =
            devices.at(
                static_cast<std::size_t>(
                    index));

        const QString model =
            QString::fromStdString(
                device.getModel())
                .trimmed();

        const QString deviceId =
            QString::fromStdString(
                device.getDeviceId())
                .trimmed();

        const QString serial =
            QString::fromStdString(
                device.getSerialNumber())
                .trimmed();

        const QString interfaceType =
            QString::fromStdString(
                device.getInterfaceType())
                .trimmed();

        const QString capacity =
            formatBytes(
                device.getCapacityBytes());

        QString displayName;

        if (!model.isEmpty())
        {
            displayName =
                model;
        }
        else
        {
            displayName =
                QStringLiteral(
                    "Storage device %1")
                    .arg(
                        index + 1);
        }

        QStringList metadata;

        if (!interfaceType.isEmpty())
        {
            metadata.append(
                interfaceType);
        }

        if (!capacity.isEmpty())
        {
            metadata.append(
                capacity);
        }

        if (device.isRemovable())
        {
            metadata.append(
                QStringLiteral(
                    "Removable"));
        }
        else
        {
            metadata.append(
                QStringLiteral(
                    "Fixed"));
        }

        if (device.isSystemDisk())
        {
            metadata.append(
                QStringLiteral(
                    "System"));
        }

        if (!metadata.isEmpty())
        {
            displayName +=
                QStringLiteral(
                    "  ·  %1")
                    .arg(
                        metadata.join(
                            QStringLiteral(
                                "  ·  ")));
        }

        deviceCombo_->addItem(
            displayName,
            deviceId);

        const int addedIndex =
            deviceCombo_->count() - 1;

        deviceCombo_->setItemData(
            addedIndex,
            serial,
            Qt::ToolTipRole);

        if (
            !previousDeviceId.isEmpty() &&
            deviceId ==
                previousDeviceId)
        {
            restoredIndex =
                addedIndex;
        }
    }

    if (
        deviceCombo_->count() == 0)
    {
        deviceCombo_->addItem(
            QStringLiteral(
                "No physical storage devices found"));

        deviceCombo_->setEnabled(
            false);
    }
    else
    {
        deviceCombo_->setEnabled(
            true);

        if (
            restoredIndex >= 0)
        {
            deviceCombo_->setCurrentIndex(
                restoredIndex);
        }
        else
        {
            deviceCombo_->setCurrentIndex(
                0);
        }
    }

    deviceCombo_->blockSignals(
        false);

    updateDeviceSelection();
    updateSourceState();
}

void ForensicPage::updateDeviceSelection()
{
    if (
        !deviceController_ ||
        !deviceCombo_)
    {
        return;
    }

    const int selectedIndex =
        deviceCombo_->currentIndex();

    const std::vector<StorageDevice> &devices =
        deviceController_->devices();

    if (
        selectedIndex < 0 ||
        selectedIndex >=
            static_cast<int>(
                devices.size()))
    {
        deviceInfoLabel_->setText(
            QStringLiteral(
                "No physical device selected."));

        deviceInfoLabel_->setStyleSheet(
            "QLabel {"
            "background:#F8FAFC;"
            "border:1px solid #EAECF0;"
            "border-radius:7px;"
            "color:#98A2B3;"
            "font-size:11px;"
            "padding:8px 10px;"
            "}");

        systemDiskWarningLabel_->hide();

        return;
    }

    const StorageDevice &device =
        devices.at(
            static_cast<std::size_t>(
                selectedIndex));

    const QString model =
        QString::fromStdString(
            device.getModel())
            .trimmed();

    const QString deviceId =
        QString::fromStdString(
            device.getDeviceId())
            .trimmed();

    const QString serial =
        QString::fromStdString(
            device.getSerialNumber())
            .trimmed();

    const QString interfaceType =
        QString::fromStdString(
            device.getInterfaceType())
            .trimmed();

    QStringList details;

    if (!interfaceType.isEmpty())
    {
        details.append(
            interfaceType);
    }

    details.append(
        formatBytes(
            device.getCapacityBytes()));

    details.append(
        device.isRemovable()
            ? QStringLiteral(
                  "Removable")
            : QStringLiteral(
                  "Fixed"));

    if (
        device.hasSeekPenalty())
    {
        details.append(
            QStringLiteral(
                "Seek penalty"));
    }

    QString text =
        QStringLiteral(
            "<b>%1</b><br>"
            "%2")
            .arg(
                model.isEmpty()
                    ? QStringLiteral(
                          "Storage device")
                    : model,
                details.join(
                    QStringLiteral(
                        "  ·  ")));

    if (
        !deviceId.isEmpty())
    {
        text +=
            QStringLiteral(
                "<br><span style='color:#98A2B3;'>"
                "Path: %1"
                "</span>")
                .arg(
                    deviceId.toHtmlEscaped());
    }

    if (
        !serial.isEmpty())
    {
        text +=
            QStringLiteral(
                "<br><span style='color:#98A2B3;'>"
                "Serial: %1"
                "</span>")
                .arg(
                    serial.toHtmlEscaped());
    }

    deviceInfoLabel_->setText(
        text);

    deviceInfoLabel_->setStyleSheet(
        "QLabel {"
        "background:#F8FAFC;"
        "border:1px solid #EAECF0;"
        "border-radius:7px;"
        "color:#344054;"
        "font-size:11px;"
        "padding:9px 10px;"
        "}");

    if (
        device.isSystemDisk())
    {
        systemDiskWarningLabel_->show();
    }
    else
    {
        systemDiskWarningLabel_->hide();
    }
}

void ForensicPage::updateSourceState()
{
    if (
        !sourceTypeCombo_ ||
        !scanButton_)
    {
        return;
    }

    const int sourceType =
        sourceTypeCombo_->currentIndex();

    bool valid =
        false;

    QString status;

    if (
        sourceType == 0)
    {
        const int index =
            deviceCombo_
                ? deviceCombo_->currentIndex()
                : -1;

        if (
            deviceController_ &&
            index >= 0)
        {
            const auto &devices =
                deviceController_->devices();

            if (
                index <
                static_cast<int>(
                    devices.size()))
            {
                const StorageDevice &device =
                    devices.at(
                        static_cast<std::size_t>(
                            index));

                const QString deviceId =
                    QString::fromStdString(
                        device.getDeviceId())
                        .trimmed();

                valid =
                    !deviceId.isEmpty();

                if (valid)
                {
                    status =
                        QStringLiteral(
                            "Physical device ready for read-only acquisition.");
                }
            }
        }

        if (!valid)
        {
            status =
                QStringLiteral(
                    "Select a physical storage device to continue.");
        }
    }
    else
    {
        const QString path =
            imagePathEdit_
                ? imagePathEdit_->text().trimmed()
                : QString();

        if (
            !path.isEmpty())
        {
            QFileInfo fileInfo(
                path);

            if (
                fileInfo.exists() &&
                fileInfo.isFile() &&
                fileInfo.isReadable())
            {
                valid =
                    true;

                status =
                    QStringLiteral(
                        "Forensic image is ready for read-only acquisition.");
            }
            else
            {
                status =
                    QStringLiteral(
                        "The selected source cannot be opened for reading.");
            }
        }
        else
        {
            status =
                QStringLiteral(
                    "Select a forensic image or raw source to continue.");
        }
    }

    scanButton_->setEnabled(
        valid &&
        !forensicService_->isRunning());

    if (
        !forensicService_->isRunning())
    {
        sourceStatusLabel_->setText(
            status);

        if (valid)
        {
            sourceStatusLabel_->setStyleSheet(
                "QLabel {"
                "background:transparent;"
                "border:none;"
                "color:#027A48;"
                "font-size:11px;"
                "font-weight:600;"
                "}");
        }
        else
        {
            sourceStatusLabel_->setStyleSheet(
                "QLabel {"
                "background:transparent;"
                "border:none;"
                "color:#667085;"
                "font-size:11px;"
                "}");
        }
    }
}

QString ForensicPage::selectedSource() const
{
    if (
        !sourceTypeCombo_)
    {
        return {};
    }

    if (
        sourceTypeCombo_->currentIndex() == 0)
    {
        if (
            !deviceController_ ||
            !deviceCombo_)
        {
            return {};
        }

        const int index =
            deviceCombo_->currentIndex();

        const auto &devices =
            deviceController_->devices();

        if (
            index < 0 ||
            index >=
                static_cast<int>(
                    devices.size()))
        {
            return {};
        }

        return QString::fromStdString(
                   devices.at(
                              static_cast<std::size_t>(
                                  index))
                       .getDeviceId())
            .trimmed();
    }

    if (
        imagePathEdit_)
    {
        return imagePathEdit_
            ->text()
            .trimmed();
    }

    return {};
}

void ForensicPage::startScan()
{
    if (
        !forensicService_ ||
        forensicService_->isRunning())
    {
        return;
    }

    const QString source =
        selectedSource();

    if (
        source.isEmpty())
    {
        QMessageBox::warning(
            this,
            QStringLiteral(
                "No forensic source"),
            QStringLiteral(
                "Please select a physical device or "
                "forensic image before starting acquisition."));

        return;
    }

    const bool physicalDevice =
        sourceTypeCombo_ &&
        sourceTypeCombo_->currentIndex() == 0;

    if (
        physicalDevice &&
        deviceController_ &&
        deviceCombo_)
    {
        const int index =
            deviceCombo_->currentIndex();

        const auto &devices =
            deviceController_->devices();

        if (
            index >= 0 &&
            index <
                static_cast<int>(
                    devices.size()))
        {
            const StorageDevice &device =
                devices.at(
                    static_cast<std::size_t>(
                        index));

            if (
                device.isSystemDisk())
            {
                const QMessageBox::StandardButton
                    answer =
                        QMessageBox::warning(
                            this,
                            QStringLiteral(
                                "System disk selected"),
                            QStringLiteral(
                                "You are about to perform a forensic "
                                "read-only acquisition of the system disk.\n\n"
                                "SecureWipe will not write, erase or sanitize "
                                "the source, but the scan may inspect sensitive "
                                "deleted data and can take significant time.\n\n"
                                "Do you want to continue?"),
                            QMessageBox::Cancel |
                                QMessageBox::Yes,
                            QMessageBox::Cancel);

                if (
                    answer !=
                    QMessageBox::Yes)
                {
                    return;
                }
            }
        }
    }

    if (
        !physicalDevice)
    {
        QFileInfo fileInfo(
            source);

        if (
            !fileInfo.exists() ||
            !fileInfo.isFile() ||
            !fileInfo.isReadable())
        {
            QMessageBox::warning(
                this,
                QStringLiteral(
                    "Invalid source"),
                QStringLiteral(
                    "The selected forensic source does not exist "
                    "or cannot be read."));

            return;
        }
    }

    // Clear the previous result set before a new acquisition.
    resultsTable_->setRowCount(
        0);

    setMetric(
        recoveredValue_,
        QStringLiteral("0"));

    setMetric(
        validatedValue_,
        QStringLiteral("0"));

    setMetric(
        highConfidenceValue_,
        QStringLiteral("0"));

    setMetric(
        recoveredBytesValue_,
        QStringLiteral("0 B"));

    setMetric(
        candidatesValue_,
        QStringLiteral("0"));

    setScanState(
        QStringLiteral(
            "Acquisition in progress…"),
        true);

    resultsSummaryLabel_->setText(
        QStringLiteral(
            "Reading source"));

    setEmptyState(
        QStringLiteral(
            "Acquiring forensic evidence"),
        QStringLiteral(
            "The source is being read in a separate worker. "
            "Please keep the application open until acquisition completes."),
        QStringLiteral(
            "…"));

    scanButton_->setEnabled(
        false);

    sourceStatusLabel_->setText(
        QStringLiteral(
            "Acquisition in progress. Source is being accessed read-only."));

    sourceStatusLabel_->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#2563EB;"
        "font-size:11px;"
        "font-weight:600;"
        "}");

    ForensicScanDialog dialog(
        source,
        window());

    forensicService_->scan(
        source);

    QTimer::singleShot(
        0,
        &dialog,
        [&dialog]()
        {
            if (
                dialog.isVisible())
            {
                return;
            }

            dialog.show();
        });

    while (
        forensicService_->isRunning())
    {
        dialog.repaint();

        QCoreApplication::processEvents(
            QEventLoop::AllEvents,
            50);
    }

    if (
        dialog.isVisible())
    {
        dialog.close();
    }

    // The service has completed by this point.
    // The connected scanFinished/scanFailed signal
    // updates the workspace state.
    QCoreApplication::processEvents(
        QEventLoop::AllEvents);
}

void ForensicPage::renderResults()
{
    if (
        !forensicService_)
    {
        return;
    }

    const ForensicScanSummary &summary =
        forensicService_->summary();

    const QVector<EvidenceItem> &results =
        forensicService_->results();

    setMetric(
        recoveredValue_,
        QString::number(
            static_cast<qulonglong>(
                summary.recoveredArtifacts)));

    setMetric(
        validatedValue_,
        QString::number(
            static_cast<qulonglong>(
                summary.validatedArtifacts)));

    setMetric(
        highConfidenceValue_,
        QString::number(
            static_cast<qulonglong>(
                summary.highConfidenceArtifacts)));

    setMetric(
        recoveredBytesValue_,
        formatBytes(
            summary.recoveredBytes));

    setMetric(
        candidatesValue_,
        QString::number(
            static_cast<qulonglong>(
                summary.candidatesFound)));

    resultsTable_->setRowCount(
        0);

    for (
        int index = 0;
        index < results.size();
        ++index)
    {
        const EvidenceItem &item =
            results.at(
                index);

        const int row =
            resultsTable_->rowCount();

        resultsTable_->insertRow(
            row);

        auto *artifact =
            new QTableWidgetItem(
                QString::fromStdString(
                    item.artifactId));

        auto *type =
            new QTableWidgetItem(
                QString::fromStdString(
                    item.fileType));

        auto *offset =
            new QTableWidgetItem(
                formatOffset(
                    item.offset));

        auto *size =
            new QTableWidgetItem(
                formatBytes(
                    item.size));

        auto *confidence =
            new QTableWidgetItem(
                QStringLiteral(
                    "%1 / 100")
                    .arg(
                        item.confidenceScore));

        auto *hash =
            new QTableWidgetItem(
                QString::fromStdString(
                    item.sha256));

        artifact->setTextAlignment(
            Qt::AlignLeft |
            Qt::AlignVCenter);

        type->setTextAlignment(
            Qt::AlignCenter);

        offset->setTextAlignment(
            Qt::AlignRight |
            Qt::AlignVCenter);

        size->setTextAlignment(
            Qt::AlignRight |
            Qt::AlignVCenter);

        confidence->setTextAlignment(
            Qt::AlignCenter);

        hash->setTextAlignment(
            Qt::AlignLeft |
            Qt::AlignVCenter);

        resultsTable_->setItem(
            row,
            0,
            artifact);

        resultsTable_->setItem(
            row,
            1,
            type);

        resultsTable_->setItem(
            row,
            2,
            offset);

        resultsTable_->setItem(
            row,
            3,
            size);

        resultsTable_->setItem(
            row,
            4,
            confidence);

        resultsTable_->setItem(
            row,
            5,
            hash);

        auto *viewButton =
            new QPushButton(
                QStringLiteral(
                    "View"),
                resultsTable_);

        viewButton->setObjectName(
            QStringLiteral(
                "forensicSecondaryButton"));

        viewButton->setCursor(
            Qt::PointingHandCursor);

        viewButton->setMinimumHeight(
            29);

        viewButton->setMinimumWidth(
            58);

        resultsTable_->setCellWidget(
            row,
            6,
            viewButton);

        connect(
            viewButton,
            &QPushButton::clicked,
            this,
            [this, row]()
            {
                showEvidenceDetails(
                    row);
            });

        if (
            item.confidenceScore >= 80)
        {
            confidence->setForeground(
                QBrush(
                    QColor(
                        "#027A48")));

            confidence->setFont(
                QFont(
                    confidence->font().family(),
                    confidence->font().pointSize(),
                    QFont::Bold));
        }
        else if (
            item.confidenceScore >= 50)
        {
            confidence->setForeground(
                QBrush(
                    QColor(
                        "#B54708")));
        }
        else
        {
            confidence->setForeground(
                QBrush(
                    QColor(
                        "#B42318")));
        }
    }

    if (
        !summary.sourceOpened)
    {
        setScanState(
            QStringLiteral(
                "Scan failed · source could not be opened"),
            false);

        resultsSummaryLabel_->setText(
            QStringLiteral(
                "Source unavailable"));

        setEmptyState(
            QStringLiteral(
                "The forensic source could not be opened"),
            QStringLiteral(
                "Windows did not allow SecureWipe to acquire the selected "
                "source. Check that the device exists, is accessible, and "
                "that the application has the required permissions."),
            QStringLiteral(
                "!"));

        scanButton_->setEnabled(
            true);

        updateSourceState();

        return;
    }

    if (
        !summary.completed)
    {
        setScanState(
            QStringLiteral(
                "Scan stopped before completion"),
            false);

        resultsSummaryLabel_->setText(
            QStringLiteral(
                "%1 scanned")
                .arg(
                    formatBytes(
                        summary.bytesScanned)));

        setEmptyState(
            QStringLiteral(
                "Acquisition stopped before completion"),
            QStringLiteral(
                "%1 bytes were read before the source returned a read error. "
                "The accepted evidence set may therefore be incomplete.")
                .arg(
                    formatBytes(
                        summary.bytesScanned)),
            QStringLiteral(
                "!"));

        scanButton_->setEnabled(
            true);

        updateSourceState();

        return;
    }

    setScanState(
        QStringLiteral(
            "Scan completed successfully"),
        true);

    resultsSummaryLabel_->setText(
        QStringLiteral(
            "%1 scanned  ·  %2 candidates  ·  %3 accepted")
            .arg(
                formatBytes(
                    summary.bytesScanned))
            .arg(
                static_cast<qulonglong>(
                    summary.candidatesFound))
            .arg(
                static_cast<qulonglong>(
                    results.size())));

    if (
        results.isEmpty())
    {
        QString title;
        QString body;
        QString icon;

        if (
            summary.candidatesFound == 0)
        {
            title =
                QStringLiteral(
                    "No recoverable JPEG candidates found");

            body =
                QStringLiteral(
                    "%1 was scanned successfully, but no JPEG signatures "
                    "were detected by the current acquisition engine. "
                    "The current collector is configured primarily for JPEG carving.")
                    .arg(
                        formatBytes(
                            summary.bytesScanned));

            icon =
                QStringLiteral(
                    "✓");
        }
        else
        {
            title =
                QStringLiteral(
                    "JPEG candidates did not pass validation");

            body =
                QStringLiteral(
                    "%1 JPEG candidate(s) were detected, but none passed "
                    "the configured validation pipeline. Rejected candidates "
                    "are not included in the final evidence set.")
                    .arg(
                        static_cast<qulonglong>(
                            summary.candidatesFound));

            icon =
                QStringLiteral(
                    "!");
        }

        setEmptyState(
            title,
            body,
            icon);
    }
    else
    {
        setEmptyState(
            QStringLiteral(
                "Evidence recovered"),
            QStringLiteral(
                "%1 validated artifact(s) were accepted into the evidence set. "
                "Select an artifact or double-click a row to inspect validation "
                "details, integrity hash and recovery path.")
                .arg(
                    static_cast<qulonglong>(
                        results.size())),
            QStringLiteral(
                "✓"));
    }

    updateSourceState();
}

void ForensicPage::showEvidenceDetails(
    int row)
{
    if (
        !forensicService_ ||
        row < 0 ||
        row >=
            forensicService_->results().size())
    {
        return;
    }

    const EvidenceItem &item =
        forensicService_->results().at(
            row);

    ForensicEvidenceDialog dialog(
        item,
        window());

    dialog.exec();
}

QString ForensicPage::formatBytes(
    std::uint64_t bytes)
{
    constexpr double KB =
        1024.0;

    constexpr double MB =
        KB * 1024.0;

    constexpr double GB =
        MB * 1024.0;

    constexpr double TB =
        GB * 1024.0;

    if (
        bytes >=
        static_cast<std::uint64_t>(
            TB))
    {
        return QStringLiteral(
                   "%1 TB")
            .arg(
                bytes / TB,
                0,
                'f',
                2);
    }

    if (
        bytes >=
        static_cast<std::uint64_t>(
            GB))
    {
        return QStringLiteral(
                   "%1 GB")
            .arg(
                bytes / GB,
                0,
                'f',
                2);
    }

    if (
        bytes >=
        static_cast<std::uint64_t>(
            MB))
    {
        return QStringLiteral(
                   "%1 MB")
            .arg(
                bytes / MB,
                0,
                'f',
                1);
    }

    if (
        bytes >=
        static_cast<std::uint64_t>(
            KB))
    {
        return QStringLiteral(
                   "%1 KB")
            .arg(
                bytes / KB,
                0,
                'f',
                1);
    }

    return QStringLiteral(
               "%1 B")
        .arg(
            static_cast<qulonglong>(
                bytes));
}

QString ForensicPage::formatOffset(
    std::uint64_t offset)
{
    return QStringLiteral(
               "0x%1")
        .arg(
            QString::number(
                static_cast<qulonglong>(
                    offset),
                16)
                .toUpper());
}

QLabel *ForensicPage::makeBadge(
    const QString &text,
    const QString &background,
    const QString &foreground,
    QWidget *parent)
{
    auto *label =
        new QLabel(
            text,
            parent);

    label->setAlignment(
        Qt::AlignCenter);

    label->setMinimumWidth(
        72);

    label->setStyleSheet(
        QStringLiteral(
            "QLabel {"
            "background:%1;"
            "color:%2;"
            "border:1px solid %2;"
            "border-radius:7px;"
            "padding:5px 8px;"
            "font-size:9px;"
            "font-weight:700;"
            "}")
            .arg(
                background,
                foreground));

    return label;
}

QFrame *ForensicPage::makeCard(
    QWidget *parent)
{
    auto *card =
        new QFrame(
            parent);

    card->setObjectName(
        QStringLiteral(
            "forensicCard"));

    card->setStyleSheet(
        "QFrame#forensicCard {"
        "background:#FFFFFF;"
        "border:1px solid #E4E7EC;"
        "border-radius:12px;"
        "}");

    return card;
}

void ForensicPage::setMetric(
    QLabel *valueLabel,
    const QString &value)
{
    if (!valueLabel)
    {
        return;
    }

    valueLabel->setText(
        value);
}

QLabel *ForensicPage::makeMetricValue(
    QWidget *parent)
{
    auto *label =
        new QLabel(
            QStringLiteral(
                "0"),
            parent);

    label->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#101828;"
        "font-size:23px;"
        "font-weight:700;"
        "}");

    label->setMinimumHeight(
        30);

    return label;
}

void ForensicPage::setScanState(
    const QString &text,
    bool success)
{
    if (
        !scanStateLabel_)
    {
        return;
    }

    scanStateLabel_->setText(
        text);

    scanStateLabel_->setStyleSheet(
        success
            ? "QLabel {"
              "background:transparent;"
              "border:none;"
              "color:#027A48;"
              "font-size:11px;"
              "font-weight:600;"
              "}"
            : "QLabel {"
              "background:transparent;"
              "border:none;"
              "color:#B42318;"
              "font-size:11px;"
              "font-weight:600;"
              "}");
}

void ForensicPage::setEmptyState(
    const QString &title,
    const QString &body,
    const QString &icon)
{
    if (
        !emptyStateTitleLabel_ ||
        !emptyStateBodyLabel_ ||
        !emptyStateIconLabel_)
    {
        return;
    }

    emptyStateTitleLabel_->setText(
        title);

    emptyStateBodyLabel_->setText(
        body);

    emptyStateIconLabel_->setText(
        icon);

    if (
        icon == QStringLiteral("✓"))
    {
        emptyStateIconLabel_->setStyleSheet(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#12B76A;"
            "font-size:28px;"
            "}");
    }
    else if (
        icon == QStringLiteral("!"))
    {
        emptyStateIconLabel_->setStyleSheet(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#F79009;"
            "font-size:28px;"
            "}");
    }
    else
    {
        emptyStateIconLabel_->setStyleSheet(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#98A2B3;"
            "font-size:28px;"
            "}");
    }
}