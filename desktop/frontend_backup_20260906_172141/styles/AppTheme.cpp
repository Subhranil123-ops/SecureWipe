#include "AppTheme.h"

#include <QWidget>

namespace AppTheme
{

void apply(QWidget *root)
{
    if (!root)
    {
        return;
    }

    root->setStyleSheet(

        /*
          
         * MAIN WINDOW
          
         */

        "QMainWindow {"
        "background-color: #F8FAFC;"
        "color: #172033;"
        "}"

        "QWidget {"
        "font-family: 'Segoe UI';"
        "color: #172033;"
        "}"

        /*
          
         * LOGIN PAGE
          
         */

        "QWidget#loginPage {"
        "background-color: #F8FAFC;"
        "}"

        "QLabel#appTitle {"
        "background-color: transparent;"
        "color: #172033;"
        "font-size: 30px;"
        "font-weight: 700;"
        "}"

        "QLabel#appSubtitle {"
        "background-color: transparent;"
        "color: #667085;"
        "font-size: 13px;"
        "}"

        "QLabel#emailLineLabel,"
        "QLabel#passwordLabel {"
        "background-color: transparent;"
        "color: #344054;"
        "font-size: 12px;"
        "font-weight: 600;"
        "}"

        "QLineEdit#emailLineEdit,"
        "QLineEdit#passwordLineEdit {"
        "background-color: #FFFFFF;"
        "color: #172033;"
        "border: 1px solid #D0D5DD;"
        "border-radius: 7px;"
        "padding: 9px 11px;"
        "font-size: 13px;"
        "}"

        "QLineEdit#emailLineEdit:focus,"
        "QLineEdit#passwordLineEdit:focus {"
        "border: 1px solid #2563EB;"
        "}"

        "QLineEdit#emailLineEdit::placeholder,"
        "QLineEdit#passwordLineEdit::placeholder {"
        "color: #98A2B3;"
        "}"

        "QPushButton#loginButton {"
        "background-color: #2563EB;"
        "color: #FFFFFF;"
        "border: none;"
        "border-radius: 7px;"
        "padding: 10px 18px;"
        "font-size: 13px;"
        "font-weight: 600;"
        "}"

        "QPushButton#loginButton:hover {"
        "background-color: #1D4ED8;"
        "}"

        "QPushButton#loginButton:pressed {"
        "background-color: #1E40AF;"
        "}"

        "QPushButton#loginButton:disabled {"
        "background-color: #D0D5DD;"
        "color: #98A2B3;"
        "}"

        "QPushButton#forgotPasswordButton {"
        "background-color: transparent;"
        "color: #2563EB;"
        "border: none;"
        "font-size: 12px;"
        "padding: 4px;"
        "}"

        "QPushButton#forgotPasswordButton:hover {"
        "color: #1D4ED8;"
        "text-decoration: underline;"
        "}"

        "QLabel#loginErrorLabel {"
        "background-color: transparent;"
        "color: #D92D20;"
        "font-size: 12px;"
        "}"

        /*
          
         * APPLICATION SHELL
          
         */

        "QWidget#appPage {"
        "background-color: #F8FAFC;"
        "}"
        "QFrame#sidebarFrame {"
        "background-color: #FFFFFF;"
        "border-right: 1px solid #E4E7EC;"
        "}"
        /*
         * The sidebar is currently represented by the first
         * layout inside appPage rather than a QWidget.
         *
         * We therefore give the navigation controls a clean,
         * consistent appearance.
         */

        "QLabel#sidebarTitle {"
        "background-color: transparent;"
        "color: #172033;"
        "font-size: 17px;"
        "font-weight: 700;"
        "padding: 6px 4px 18px 4px;"
        "}"

        /*
          
         * NAVIGATION
          
         */

        "QPushButton#dashboardNavButton,"
"QPushButton#devicesNavButton,"
"QPushButton#wipeNavButton,"
"QPushButton#reportsNavButton,"
"QPushButton#settingsNavButton {"
"background-color: transparent;"
"color: #475467;"
"border: none;"
"border-radius: 7px;"
"text-align: left;"
"padding: 0px 12px;"
"font-size: 13px;"
"font-weight: 500;"
"}"

"QPushButton#dashboardNavButton:hover,"
"QPushButton#devicesNavButton:hover,"
"QPushButton#wipeNavButton:hover,"
"QPushButton#reportsNavButton:hover,"
"QPushButton#settingsNavButton:hover {"
"background-color: #F2F4F7;"
"color: #172033;"
"}"

"QPushButton#logoutButton {"
"background-color: transparent;"
"color: #D92D20;"
"border: none;"
"border-radius: 7px;"
"text-align: left;"
"padding: 0px 12px;"
"font-size: 13px;"
"font-weight: 500;"
"}"

"QPushButton#logoutButton:hover {"
"background-color: #FEF3F2;"
"}"

        /*
          
         * DASHBOARD
          
         */

        "QLabel#welcomeLabel {"
        "background-color: transparent;"
        "color: #172033;"
        "font-size: 20px;"
        "font-weight: 700;"
        "}"

        "QLabel#roleBadge {"
        "background-color: #EFF6FF;"
        "color: #1D4ED8;"
        "border-radius: 10px;"
        "padding: 4px 10px;"
        "font-size: 11px;"
        "font-weight: 600;"
        "}"

        "QLabel#totalJobsTitle,"
        "QLabel#completedJobsTitle,"
        "QLabel#failedJobsTitle,"
        "QLabel#inProgressTitle {"
        "background-color: transparent;"
        "color: #667085;"
        "font-size: 12px;"
        "}"

        "QLabel#totalJobsValue,"
        "QLabel#completedJobsValue,"
        "QLabel#failedJobsValue,"
        "QLabel#inProgressValue {"
        "background-color: transparent;"
        "font-size: 25px;"
        "font-weight: 700;"
        "}"

        "QLabel#totalJobsValue {"
        "color: #172033;"
        "}"

        "QLabel#completedJobsValue {"
        "color: #039855;"
        "}"

        "QLabel#failedJobsValue {"
        "color: #D92D20;"
        "}"

        "QLabel#inProgressValue {"
        "color: #2563EB;"
        "}"

        "QLabel#recentJobsLabel {"
        "background-color: transparent;"
        "color: #344054;"
        "font-size: 13px;"
        "font-weight: 600;"
        "}"

        "QTableWidget {"
        "background-color: #FFFFFF;"
        "alternate-background-color: #F8FAFC;"
        "color: #172033;"
        "gridline-color: #EAECF0;"
        "border: 1px solid #E4E7EC;"
        "border-radius: 8px;"
        "}"

        "QTableWidget::item {"
        "padding: 7px;"
        "}"

        "QTableWidget::item:selected {"
        "background-color: #EFF6FF;"
        "color: #172033;"
        "}"

        "QHeaderView::section {"
        "background-color: #F8FAFC;"
        "color: #667085;"
        "padding: 8px;"
        "border: none;"
        "border-bottom: 1px solid #E4E7EC;"
        "font-size: 12px;"
        "font-weight: 600;"
        "}"

        /*
          
         * WIPE PAGE
          
         */

        "QLabel#wipePageTitle,"
        "QLabel#operationTitle,"
        "QLabel#statusTitle {"
        "background-color: transparent;"
        "color: #172033;"
        "font-weight: 700;"
        "}"

        "QLabel#deviceLabel,"
        "QLabel#methodLabel {"
        "background-color: transparent;"
        "color: #475467;"
        "font-size: 12px;"
        "}"

        "QComboBox#deviceComboBox,"
        "QComboBox#methodComboBox {"
        "background-color: #FFFFFF;"
        "color: #172033;"
        "border: 1px solid #D0D5DD;"
        "border-radius: 6px;"
        "padding: 7px 9px;"
        "}"

        "QComboBox#deviceComboBox:focus,"
        "QComboBox#methodComboBox:focus {"
        "border: 1px solid #2563EB;"
        "}"

        "QPushButton#startWipeButton {"
        "background-color: #2563EB;"
        "color: #FFFFFF;"
        "border: none;"
        "border-radius: 7px;"
        "padding: 10px;"
        "font-size: 13px;"
        "font-weight: 600;"
        "}"

        "QPushButton#startWipeButton:hover {"
        "background-color: #1D4ED8;"
        "}"

        "QLabel#statusLabel {"
        "background-color: transparent;"
        "color: #667085;"
        "font-size: 13px;"
        "}"

        "QProgressBar {"
        "background-color: #F2F4F7;"
        "color: #344054;"
        "border: 1px solid #D0D5DD;"
        "border-radius: 6px;"
        "text-align: center;"
        "}"

        "QProgressBar::chunk {"
        "background-color: #2563EB;"
        "border-radius: 6px;"
        "}"

        /*
          
         * PLACEHOLDER PAGES
          
         */

        "QLabel#devicesPlaceholderLabel,"
        "QLabel#reportsPlaceholderLabel,"
        "QLabel#settingsPlaceholderLabel {"
        "background-color: transparent;"
        "color: #667085;"
        "font-size: 14px;"
        "}"

        /*
          
         * GENERAL CONTROLS
          
         */

        "QPushButton {"
        "font-family: 'Segoe UI';"
        "}"

        "QScrollArea {"
        "background-color: transparent;"
        "border: none;"
        "}"

        "QToolTip {"
        "background-color: #172033;"
        "color: #FFFFFF;"
        "border: none;"
        "padding: 6px 8px;"
        "}"

    );
}

}