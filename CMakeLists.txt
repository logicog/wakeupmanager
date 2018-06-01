cmake_minimum_required(VERSION 3.0)

project (wakeupmanager)

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
    XmlGui          # KXmlGuiWindow
    Auth
    Config
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
    
set(wakeupmanager main.cpp WakeupManager.cpp ACPIEntry.cpp USBEntry.cpp)

add_executable(wakeupmanager ${wakeupmanager})

add_executable(wakeuphelper WakeupHelper.cpp)

target_link_libraries(wakeupmanager
    Qt5::Widgets
    KF5::CoreAddons
    KF5::I18n
    KF5::XmlGui
    KF5::ConfigCore
)

target_link_libraries(wakeuphelper KF5::Auth KF5::ConfigCore)

install(TARGETS wakeupmanager  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

kauth_install_helper_files(wakeuphelper org.kde.wakeupmanager root)
install(TARGETS wakeuphelper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})
kauth_install_actions(org.kde.wakeupmanager wakeupmanager.actions)

install(FILES wakeupmanagerui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/wakeupmanager)


