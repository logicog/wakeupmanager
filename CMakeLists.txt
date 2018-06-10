cmake_minimum_required(VERSION 3.0)

project (wakeupkcm)
set(PROJECT_VERSION "0.9")

set(QT_MIN_VERSION "5.3.0")
set(KF5_MIN_VERSION "5.2.0")

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(KDE_INSTALL_USE_QT_SYS_PATHS ON CACHE BOOL "Install mkspecs files, Plugins and Imports to the Qt 5 install dir" FORCE)
#set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons      # KAboutData
    I18n            # KLocalizedString
    Auth
    Config
    ConfigWidgets
)

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

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

install(TARGETS kcm_wakeup DESTINATION ${PLUGIN_INSTALL_DIR})
#install(TARGETS kcm_wakeup DESTINATION ${CMAKE_INSTALL_PLUGINDIR})

kauth_install_helper_files(wakeuphelper org.kde.wakeupmanager root)
install(TARGETS wakeuphelper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})
kauth_install_actions(org.kde.wakeupmanager wakeupmanager.actions)

install(FILES kcm_wakeup.desktop DESTINATION ${CMAKE_INSTALL_KSERVICES5DIR})


