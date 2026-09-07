#include "AppTheme.h"

#include <QApplication>
#include <QPalette>
#include <QWidget>

namespace AppTheme
{

void apply(QWidget *root)
{
    if (!root)
        return;

    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#F5F7FB"));
    palette.setColor(QPalette::WindowText, QColor("#172033"));
    palette.setColor(QPalette::Base, QColor("#FFFFFF"));
    palette.setColor(QPalette::AlternateBase, QColor("#F8FAFC"));
    palette.setColor(QPalette::Text, QColor("#172033"));
    palette.setColor(QPalette::Button, QColor("#FFFFFF"));
    palette.setColor(QPalette::ButtonText, QColor("#344054"));
    palette.setColor(QPalette::BrightText, QColor("#FFFFFF"));
    palette.setColor(QPalette::Highlight, QColor("#DBEAFE"));
    palette.setColor(QPalette::HighlightedText, QColor("#172033"));
    palette.setColor(QPalette::PlaceholderText, QColor("#98A2B3"));

    QApplication::setPalette(palette);

    root->setStyleSheet(
        QStringLiteral(
            "QMainWindow {"
            "background:#F5F7FB;"
            "color:#172033;"
            "}"

            "QWidget {"
            "font-family:'Segoe UI';"
            "color:#172033;"
            "}"

            "QWidget#loginPage,"
            "QWidget#appPage,"
            "QWidget#dashboardPage,"
            "QWidget#jobsPage,"
            "QWidget#devicesPage,"
            "QWidget#forensicsPage,"
            "QWidget#settingsPage {"
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
            "border-radius:9px;"
            "padding:10px 12px;"
            "font-size:12px;"
            "selection-background-color:#DBEAFE;"
            "}"

            "QLineEdit:focus {"
            "border:1px solid #2563EB;"
            "}"

            "QLineEdit:disabled {"
            "background:#F2F4F7;"
            "color:#98A2B3;"
            "}"

            "QComboBox {"
            "background:#FFFFFF;"
            "color:#172033;"
            "border:1px solid #D0D5DD;"
            "border-radius:9px;"
            "padding:9px 12px;"
            "font-size:12px;"
            "min-height:18px;"
            "}"

            "QComboBox:hover {"
            "border:1px solid #98A2B3;"
            "}"

            "QComboBox:focus {"
            "border:1px solid #2563EB;"
            "}"

            "QComboBox::drop-down {"
            "border:none;"
            "width:28px;"
            "}"

            "QComboBox QAbstractItemView {"
            "background:#FFFFFF;"
            "color:#172033;"
            "border:1px solid #D0D5DD;"
            "selection-background-color:#EFF6FF;"
            "selection-color:#172033;"
            "padding:4px;"
            "}"

            "QPushButton {"
            "font-family:'Segoe UI';"
            "font-size:12px;"
            "font-weight:600;"
            "border:none;"
            "border-radius:9px;"
            "}"

            "QPushButton:disabled {"
            "color:#98A2B3;"
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
            "text-align:center;"
            "}"

            "QProgressBar::chunk {"
            "background:#2563EB;"
            "border-radius:5px;"
            "}"

            "QScrollBar:vertical {"
            "background:#F2F4F7;"
            "width:10px;"
            "border:none;"
            "margin:0;"
            "}"

            "QScrollBar::handle:vertical {"
            "background:#CBD5E1;"
            "min-height:30px;"
            "border-radius:5px;"
            "}"

            "QScrollBar::handle:vertical:hover {"
            "background:#98A2B3;"
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
            "}"
        )
    );
}

}