cmake_minimum_required(VERSION 3.0)

project (wakeupkcm)
set(PROJECT_VERSION "0.8")

set(QT_MIN_VERSION "5.3.0")
set(KF5_MIN_VERSION "5.2.0")

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS 
    Core    # QCommandLineParser, QStringLiteral
    Widgets # QApplication 
)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons      # KAboutData
    I18n            # KLocalizedString
    Auth
    Config
    ConfigWidgets
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(kcmwakeup_SRCS WakeupKcm.cpp ACPIEntry.cpp USBEntry.cpp)
add_library(kcm_wakeup MODULE ${kcmwakeup_SRCS})

add_executable(wakeuphelper WakeupHelper.cpp)

target_link_libraries(kcm_wakeup
    Qt5::Widgets
    KF5::CoreAddons
    KF5::I18n
    KF5::Auth
    KF5::ConfigCore
    KF5::ConfigWidgets
)

target_compile_definitions(kcm_wakeup PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

target_link_libraries(wakeuphelper KF5::Auth KF5::ConfigCore)

install(TARGETS kcm_wakeup ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

kauth_install_helper_files(wakeuphelper org.kde.wakeupmanager root)
install(TARGETS wakeuphelper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})
kauth_install_actions(org.kde.wakeupmanager wakeupmanager.actions)

install(FILES kcm_wakeup.desktop DESTINATION ${CMAKE_INSTALL_KSERVICES5DIR})


