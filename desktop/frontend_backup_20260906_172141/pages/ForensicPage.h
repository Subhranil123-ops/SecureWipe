#pragma once

#include <QWidget>

#include "StorageDevice.h"

class DeviceController;
class ForensicService;

class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;

class ForensicPage : public QWidget
{
    Q_OBJECT

public:
    explicit ForensicPage(
        DeviceController *deviceController,
        QWidget *parent = nullptr
    );

private:
    DeviceController *deviceController_;
    ForensicService *forensicService_;

    QComboBox *sourceTypeCombo_;
    QStackedWidget *sourceStack_;

    QComboBox *deviceCombo_;
    QLineEdit *imagePathEdit_;

    QLabel *deviceInfoLabel_;
    QLabel *systemDiskWarningLabel_;
    QLabel *sourceStatusLabel_;
    QLabel *sourceBadgeLabel_;

    QPushButton *refreshButton_;
    QPushButton *browseButton_;
    QPushButton *scanButton_;

    QLabel *recoveredValue_;
    QLabel *validatedValue_;
    QLabel *highConfidenceValue_;
    QLabel *recoveredBytesValue_;
    QLabel *candidatesValue_;

    QLabel *scanStateLabel_;
    QLabel *resultsSummaryLabel_;

    QLabel *emptyStateIconLabel_;
    QLabel *emptyStateTitleLabel_;
    QLabel *emptyStateBodyLabel_;

    QTableWidget *resultsTable_;

    void buildUi();

    void refreshDeviceList();
    void updateDeviceSelection();
    void updateSourceState();

    void startScan();
    void renderResults();

    void showEvidenceDetails(
        int row
    );

    QString selectedSource() const;

    static QString formatBytes(
        std::uint64_t bytes
    );

    static QString formatOffset(
        std::uint64_t offset
    );

    static QLabel *makeBadge(
        const QString &text,
        const QString &background,
        const QString &foreground,
        QWidget *parent
    );

    static QFrame *makeCard(
        QWidget *parent
    );

    static QLabel *makeMetricValue(
        QWidget *parent
    );

    static void setMetric(
        QLabel *valueLabel,
        const QString &value
    );

    void setScanState(
        const QString &text,
        bool success
    );

    void setEmptyState(
        const QString &title,
        const QString &body,
        const QString &icon
    );
};