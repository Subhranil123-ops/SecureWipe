cmake_minimum_required(VERSION 3.20)

project(SecureWipe
    VERSION 1.0
    LANGUAGES CXX
)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Concurrent)

qt_standard_project_setup()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(backend)

qt_add_executable(SecureWipe
    WIN32
    MACOSX_BUNDLE

    frontend/main.cpp

    frontend/mainwindow.cpp
    frontend/mainwindow.h
    frontend/mainwindow.ui

    frontend/AuthManager.cpp
    frontend/AuthManager.h

    frontend/controllers/DeviceController.cpp
    frontend/controllers/DeviceController.h

    frontend/models/DeviceTableModel.cpp
    frontend/models/DeviceTableModel.h

    frontend/services/StorageService.cpp
    frontend/services/StorageService.h

    frontend/services/ForensicService.cpp
    frontend/services/ForensicService.h

    frontend/services/SanitizationRequestService.cpp
    frontend/services/SanitizationRequestService.h

    frontend/dialogs/DeviceDetailsDialog.cpp
    frontend/dialogs/DeviceDetailsDialog.h

    frontend/dialogs/ForensicEvidenceDialog.cpp
    frontend/dialogs/ForensicEvidenceDialog.h

    frontend/dialogs/ForensicScanDialog.cpp
    frontend/dialogs/ForensicScanDialog.h

    frontend/pages/DeviceDetailsPage.cpp
    frontend/pages/DeviceDetailsPage.h

    frontend/pages/ForensicPage.cpp
    frontend/pages/ForensicPage.h

    frontend/styles/AppTheme.cpp
    frontend/styles/AppTheme.h

    frontend/resources/resources.qrc
)

target_include_directories(SecureWipe
    PRIVATE
        frontend
        frontend/controllers
        frontend/models
        frontend/services
        frontend/dialogs
        frontend/pages
)

target_link_libraries(SecureWipe
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Concurrent
        SecureWipeStorage
)

install(TARGETS SecureWipe
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET SecureWipe
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})