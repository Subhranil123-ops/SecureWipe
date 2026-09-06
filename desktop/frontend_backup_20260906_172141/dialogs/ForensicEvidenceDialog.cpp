#include "ForensicEvidenceDialog.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

ForensicEvidenceDialog::ForensicEvidenceDialog(
    const EvidenceItem &evidence,
    QWidget *parent
)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Forensic Evidence Details"));
    setModal(true);
    setMinimumSize(650, 560);
    resize(720, 620);
    setObjectName(QStringLiteral("forensicEvidenceDialog"));

    QLabel *titleLabel = new QLabel(
        QStringLiteral("Evidence Artifact"),
        this
    );
    titleLabel->setObjectName(QStringLiteral("dialogTitle"));

    QLabel *subtitleLabel = new QLabel(
        QStringLiteral(
            "Detailed forensic metadata, validation results and integrity information."
        ),
        this
    );
    subtitleLabel->setObjectName(QStringLiteral("dialogSubtitle"));
    subtitleLabel->setWordWrap(true);

    QFrame *summaryCard = new QFrame(this);
    summaryCard->setObjectName(QStringLiteral("summaryCard"));

    QVBoxLayout *summaryLayout = new QVBoxLayout(summaryCard);
    summaryLayout->setContentsMargins(18, 16, 18, 16);
    summaryLayout->setSpacing(8);

    QLabel *artifactLabel = new QLabel(
        QString::fromStdString(evidence.artifactId),
        this
    );
    artifactLabel->setObjectName(QStringLiteral("artifactId"));

    QLabel *confidenceLabel = new QLabel(
        QStringLiteral("%1 (%2)")
            .arg(evidence.confidenceScore)
            .arg(QString::fromStdString(evidence.getConfidenceString())),
        this
    );
    confidenceLabel->setObjectName(QStringLiteral("confidenceValue"));

    QHBoxLayout *summaryHeader = new QHBoxLayout();
    summaryHeader->addWidget(artifactLabel);
    summaryHeader->addStretch();
    summaryHeader->addWidget(confidenceLabel);

    summaryLayout->addLayout(summaryHeader);

    QGridLayout *detailsLayout = new QGridLayout();
    detailsLayout->setHorizontalSpacing(20);
    detailsLayout->setVerticalSpacing(10);

    addDetailRow(
        detailsLayout,
        0,
        QStringLiteral("Source"),
        QString::fromStdString(evidence.source)
    );

    addDetailRow(
        detailsLayout,
        1,
        QStringLiteral("File Name"),
        QString::fromStdString(evidence.fileName)
    );

    addDetailRow(
        detailsLayout,
        2,
        QStringLiteral("File Type"),
        QString::fromStdString(evidence.fileType)
    );

    addDetailRow(
        detailsLayout,
        3,
        QStringLiteral("Offset"),
        QStringLiteral("0x%1")
            .arg(
                static_cast<qulonglong>(evidence.offset),
                0,
                16
            )
            .toUpper()
    );

    addDetailRow(
        detailsLayout,
        4,
        QStringLiteral("Size"),
        QStringLiteral("%1 bytes")
            .arg(static_cast<qulonglong>(evidence.size))
    );

    addDetailRow(
        detailsLayout,
        5,
        QStringLiteral("Recovered Path"),
        QString::fromStdString(evidence.recoveredPath)
    );

    summaryLayout->addLayout(detailsLayout);

    QFrame *validationCard = new QFrame(this);
    validationCard->setObjectName(QStringLiteral("validationCard"));

    QVBoxLayout *validationLayout = new QVBoxLayout(validationCard);
    validationLayout->setContentsMargins(18, 16, 18, 16);
    validationLayout->setSpacing(10);

    QLabel *validationTitle = new QLabel(
        QStringLiteral("Validation Results"),
        this
    );
    validationTitle->setObjectName(QStringLiteral("sectionTitle"));

    validationLayout->addWidget(validationTitle);

    QGridLayout *validationGrid = new QGridLayout();
    validationGrid->setHorizontalSpacing(28);
    validationGrid->setVerticalSpacing(10);

    validationGrid->addWidget(
        new QLabel(QStringLiteral("Header"), this),
        0,
        0
    );
    validationGrid->addWidget(
        createStatusLabel(evidence.headerValid),
        0,
        1
    );

    validationGrid->addWidget(
        new QLabel(QStringLiteral("Footer"), this),
        0,
        2
    );
    validationGrid->addWidget(
        createStatusLabel(evidence.footerValid),
        0,
        3
    );

    validationGrid->addWidget(
        new QLabel(QStringLiteral("Structure"), this),
        1,
        0
    );
    validationGrid->addWidget(
        createStatusLabel(evidence.structureValid),
        1,
        1
    );

    validationGrid->addWidget(
        new QLabel(QStringLiteral("Size"), this),
        1,
        2
    );
    validationGrid->addWidget(
        createStatusLabel(evidence.sizeValid),
        1,
        3
    );

    validationGrid->addWidget(
        new QLabel(QStringLiteral("Decodable"), this),
        2,
        0
    );
    validationGrid->addWidget(
        createStatusLabel(evidence.decodable),
        2,
        1
    );

    validationGrid->addWidget(
        new QLabel(QStringLiteral("Validated"), this),
        2,
        2
    );
    validationGrid->addWidget(
        createStatusLabel(evidence.validated),
        2,
        3
    );

    validationLayout->addLayout(validationGrid);

    QFrame *integrityCard = new QFrame(this);
    integrityCard->setObjectName(QStringLiteral("integrityCard"));

    QVBoxLayout *integrityLayout = new QVBoxLayout(integrityCard);
    integrityLayout->setContentsMargins(18, 16, 18, 16);
    integrityLayout->setSpacing(8);

    QLabel *integrityTitle = new QLabel(
        QStringLiteral("Integrity"),
        this
    );
    integrityTitle->setObjectName(QStringLiteral("sectionTitle"));

    QLabel *hashCaption = new QLabel(
        QStringLiteral("SHA-256"),
        this
    );
    hashCaption->setObjectName(QStringLiteral("fieldCaption"));

    QLabel *hashLabel = new QLabel(
        evidence.sha256.empty()
            ? QStringLiteral("Not available")
            : QString::fromStdString(evidence.sha256),
        this
    );
    hashLabel->setObjectName(QStringLiteral("hashValue"));
    hashLabel->setWordWrap(true);
    hashLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    integrityLayout->addWidget(integrityTitle);
    integrityLayout->addWidget(hashCaption);
    integrityLayout->addWidget(hashLabel);

    QFrame *confidenceCard = new QFrame(this);
    confidenceCard->setObjectName(QStringLiteral("confidenceCard"));

    QVBoxLayout *confidenceLayout = new QVBoxLayout(confidenceCard);
    confidenceLayout->setContentsMargins(18, 16, 18, 16);
    confidenceLayout->setSpacing(8);

    QLabel *confidenceTitle = new QLabel(
        QStringLiteral("Confidence Assessment"),
        this
    );
    confidenceTitle->setObjectName(QStringLiteral("sectionTitle"));

    QLabel *confidenceScore = new QLabel(
        QStringLiteral("Score: %1")
            .arg(evidence.confidenceScore),
        this
    );
    confidenceScore->setObjectName(QStringLiteral("scoreValue"));

    confidenceLayout->addWidget(confidenceTitle);
    confidenceLayout->addWidget(confidenceScore);

    if (!evidence.confidenceReasons.empty())
    {
        for (const std::string &reason : evidence.confidenceReasons)
        {
            QLabel *reasonLabel = new QLabel(
                QStringLiteral("• %1")
                    .arg(QString::fromStdString(reason)),
                this
            );
            reasonLabel->setObjectName(QStringLiteral("reasonLabel"));
            reasonLabel->setWordWrap(true);
            confidenceLayout->addWidget(reasonLabel);
        }
    }
    else
    {
        QLabel *reasonLabel = new QLabel(
            QStringLiteral("No additional confidence reasons were recorded."),
            this
        );
        reasonLabel->setObjectName(QStringLiteral("reasonLabel"));
        reasonLabel->setWordWrap(true);
        confidenceLayout->addWidget(reasonLabel);
    }

    QPushButton *closeButton = new QPushButton(
        QStringLiteral("Close"),
        this
    );
    closeButton->setObjectName(QStringLiteral("primaryButton"));
    closeButton->setMinimumHeight(36);
    closeButton->setMinimumWidth(90);

    connect(
        closeButton,
        &QPushButton::clicked,
        this,
        &QDialog::accept
    );

    QVBoxLayout *contentLayout = new QVBoxLayout();
    contentLayout->setContentsMargins(22, 20, 22, 20);
    contentLayout->setSpacing(14);

    contentLayout->addWidget(titleLabel);
    contentLayout->addWidget(subtitleLabel);
    contentLayout->addWidget(summaryCard);
    contentLayout->addWidget(validationCard);
    contentLayout->addWidget(integrityCard);
    contentLayout->addWidget(confidenceCard);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    contentLayout->addLayout(buttonLayout);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(new QWidget());

    QWidget *scrollContent = scrollArea->widget();
    scrollContent->setLayout(contentLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(scrollArea);

    setLayout(mainLayout);

    setStyleSheet(QStringLiteral(
        "QDialog#forensicEvidenceDialog {"
        "    background: #f8fafc;"
        "}"
        "QScrollArea {"
        "    background: transparent;"
        "}"
        "QWidget {"
        "    font-family: 'Segoe UI';"
        "}"
        "QLabel#dialogTitle {"
        "    color: #0f172a;"
        "    font-size: 21px;"
        "    font-weight: 700;"
        "}"
        "QLabel#dialogSubtitle {"
        "    color: #64748b;"
        "    font-size: 12px;"
        "}"
        "QFrame#summaryCard,"
        "QFrame#validationCard,"
        "QFrame#integrityCard,"
        "QFrame#confidenceCard {"
        "    background: #ffffff;"
        "    border: 1px solid #e2e8f0;"
        "    border-radius: 12px;"
        "}"
        "QLabel#artifactId {"
        "    color: #0f172a;"
        "    font-size: 15px;"
        "    font-weight: 700;"
        "}"
        "QLabel#confidenceValue {"
        "    color: #2563eb;"
        "    font-size: 13px;"
        "    font-weight: 700;"
        "}"
        "QLabel#sectionTitle {"
        "    color: #1e293b;"
        "    font-size: 13px;"
        "    font-weight: 700;"
        "}"
        "QLabel#fieldCaption {"
        "    color: #64748b;"
        "    font-size: 11px;"
        "    font-weight: 600;"
        "}"
        "QLabel#hashValue {"
        "    color: #334155;"
        "    background: #f8fafc;"
        "    border: 1px solid #e2e8f0;"
        "    border-radius: 7px;"
        "    padding: 9px;"
        "    font-family: Consolas;"
        "    font-size: 11px;"
        "}"
        "QLabel#scoreValue {"
        "    color: #2563eb;"
        "    font-size: 15px;"
        "    font-weight: 700;"
        "}"
        "QLabel#reasonLabel {"
        "    color: #475569;"
        "    font-size: 12px;"
        "}"
        "QLabel#validationValid {"
        "    color: #15803d;"
        "    font-size: 12px;"
        "    font-weight: 600;"
        "}"
        "QLabel#validationInvalid {"
        "    color: #dc2626;"
        "    font-size: 12px;"
        "    font-weight: 600;"
        "}"
        "QPushButton#primaryButton {"
        "    color: #ffffff;"
        "    background: #2563eb;"
        "    border: 1px solid #2563eb;"
        "    border-radius: 7px;"
        "    padding: 0 16px;"
        "    font-size: 12px;"
        "    font-weight: 600;"
        "}"
        "QPushButton#primaryButton:hover {"
        "    background: #1d4ed8;"
        "}"
    ));
}

QLabel *ForensicEvidenceDialog::createValueLabel(const QString &value)
{
    QLabel *label = new QLabel(value, this);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setStyleSheet(
        QStringLiteral(
            "color: #334155; font-size: 12px;"
        )
    );
    return label;
}

QLabel *ForensicEvidenceDialog::createStatusLabel(bool valid)
{
    QLabel *label = new QLabel(
        valid
            ? QStringLiteral("PASS")
            : QStringLiteral("FAIL"),
        this
    );

    label->setObjectName(
        valid
            ? QStringLiteral("validationValid")
            : QStringLiteral("validationInvalid")
    );

    return label;
}

void ForensicEvidenceDialog::addDetailRow(
    QGridLayout *layout,
    int row,
    const QString &label,
    const QString &value
)
{
    QLabel *caption = new QLabel(label, this);
    caption->setStyleSheet(
        QStringLiteral(
            "color: #64748b; font-size: 11px; font-weight: 600;"
        )
    );

    QLabel *valueLabel = createValueLabel(value);

    layout->addWidget(caption, row, 0);
    layout->addWidget(valueLabel, row, 1);
}