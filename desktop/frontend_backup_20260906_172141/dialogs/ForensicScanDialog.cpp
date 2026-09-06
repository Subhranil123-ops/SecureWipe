#include "ForensicScanDialog.h"

#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QStringList>
#include <QVBoxLayout>

ForensicScanDialog::ForensicScanDialog(
    const QString &source,
    QWidget *parent)
    : QDialog(parent),
      sourceLabel_(new QLabel(this)),
      progressBar_(new QProgressBar(this))
{
    setWindowTitle(
        QStringLiteral(
            "SecureWipe · Forensic acquisition"
        )
    );

    setModal(true);

    setFixedSize(
        590,
        350
    );

    setWindowFlags(
        Qt::Dialog |
        Qt::CustomizeWindowHint |
        Qt::WindowTitleHint
    );

    setStyleSheet(
        "QDialog {"
        "background:#FFFFFF;"
        "}"
    );

    auto *layout =
        new QVBoxLayout(this);

    layout->setContentsMargins(
        32,
        28,
        32,
        26
    );

    layout->setSpacing(
        12
    );

    auto *eyebrow =
        new QLabel(
            QStringLiteral(
                "SECUREWIPE  /  FORENSIC WORKSPACE"
            ),
            this
        );

    eyebrow->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#2563EB;"
        "font-size:10px;"
        "font-weight:700;"
        "letter-spacing:0.5px;"
        "}"
    );

    auto *title =
        new QLabel(
            QStringLiteral(
                "Acquiring digital evidence"
            ),
            this
        );

    title->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#101828;"
        "font-size:23px;"
        "font-weight:700;"
        "}"
    );

    auto *description =
        new QLabel(
            QStringLiteral(
                "SecureWipe is reading the selected source in "
                "read-only mode. Detected JPEG candidates are "
                "carved, validated and cryptographically hashed "
                "before being accepted as evidence."
            ),
            this
        );

    description->setWordWrap(
        true
    );

    description->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:12px;"
        "line-height:1.4;"
        "}"
    );

    auto *sourceTitle =
        new QLabel(
            QStringLiteral("SOURCE"),
            this
        );

    sourceTitle->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#667085;"
        "font-size:10px;"
        "font-weight:700;"
        "}"
    );

    sourceLabel_->setText(
        QFontMetrics(
            sourceLabel_->font()
        ).elidedText(
            source,
            Qt::ElideMiddle,
            510
        )
    );

    sourceLabel_->setToolTip(
        source
    );

    sourceLabel_->setStyleSheet(
        "QLabel {"
        "background:#F8FAFC;"
        "border:1px solid #EAECF0;"
        "border-radius:9px;"
        "color:#344054;"
        "padding:11px 12px;"
        "font-size:11px;"
        "}"
    );

    progressBar_->setRange(
        0,
        0
    );

    progressBar_->setTextVisible(
        false
    );

    progressBar_->setFixedHeight(
        8
    );

    progressBar_->setStyleSheet(
        "QProgressBar {"
        "background:#EEF2F7;"
        "border:none;"
        "border-radius:4px;"
        "}"
        "QProgressBar::chunk {"
        "background:#2563EB;"
        "border-radius:4px;"
        "}"
    );

    auto *stageFrame =
        new QFrame(this);

    stageFrame->setStyleSheet(
        "QFrame {"
        "background:#F8FAFC;"
        "border:1px solid #EAECF0;"
        "border-radius:10px;"
        "}"
    );

    auto *stageLayout =
        new QHBoxLayout(
            stageFrame
        );

    stageLayout->setContentsMargins(
        13,
        10,
        13,
        10
    );

    stageLayout->setSpacing(
        8
    );

    const QStringList stages = {
        QStringLiteral("Read"),
        QStringLiteral("Carve"),
        QStringLiteral("Validate"),
        QStringLiteral("Hash")
    };

    for (int i = 0;
         i < stages.size();
         ++i)
    {
        auto *label =
            new QLabel(
                QStringLiteral(
                    "●  %1"
                ).arg(
                    stages.at(i)
                ),
                stageFrame
            );

        label->setStyleSheet(
            "QLabel {"
            "background:transparent;"
            "border:none;"
            "color:#475467;"
            "font-size:10px;"
            "font-weight:600;"
            "}"
        );

        stageLayout->addWidget(
            label
        );

        if (i <
            stages.size() - 1)
        {
            auto *arrow =
                new QLabel(
                    QStringLiteral("›"),
                    stageFrame
                );

            arrow->setStyleSheet(
                "QLabel {"
                "background:transparent;"
                "border:none;"
                "color:#98A2B3;"
                "font-size:13px;"
                "}"
            );

            stageLayout->addWidget(
                arrow
            );
        }
    }

    auto *note =
        new QLabel(
            QStringLiteral(
                "READ-ONLY ACQUISITION  ·  "
                "No write, erase or sanitization operation "
                "is performed on the selected source."
            ),
            this
        );

    note->setWordWrap(
        true
    );

    note->setStyleSheet(
        "QLabel {"
        "background:transparent;"
        "border:none;"
        "color:#98A2B3;"
        "font-size:10px;"
        "}"
    );

    layout->addWidget(
        eyebrow
    );

    layout->addWidget(
        title
    );

    layout->addWidget(
        description
    );

    layout->addSpacing(
        3
    );

    layout->addWidget(
        sourceTitle
    );

    layout->addWidget(
        sourceLabel_
    );

    layout->addSpacing(
        3
    );

    layout->addWidget(
        progressBar_
    );

    layout->addWidget(
        stageFrame
    );

    layout->addWidget(
        note
    );
}