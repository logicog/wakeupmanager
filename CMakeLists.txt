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

add_executable(wakeuphelper WakeupHelper.cpp WakeupConfig.cpp)

add_executable(configurewakeup configurewakeup.cpp WakeupConfig.cpp)

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
target_link_libraries(configurewakeup KF5::ConfigCore)

install(TARGETS kcm_wakeup DESTINATION ${PLUGIN_INSTALL_DIR})
#install(TARGETS kcm_wakeup DESTINATION ${CMAKE_INSTALL_PLUGINDIR})

kauth_install_helper_files(wakeuphelper org.kde.wakeupmanager root)
install(TARGETS wakeuphelper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})

install(TARGETS configurewakeup ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

kauth_install_actions(org.kde.wakeupmanager wakeupmanager.actions)

install(FILES kcm_wakeup.desktop DESTINATION ${CMAKE_INSTALL_KSERVICES5DIR})

install(FILES wakeupmanager.sh DESTINATION /lib/systemd/system-sleep
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                    GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)



if(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
include(InstallRequiredSystemLibraries)
 
set(CPACK_SET_DESTDIR "on")
set(CPACK_PACKAGING_INSTALL_PREFIX "/tmp")
set(CPACK_GENERATOR "DEB")
 
set(CPACK_PACKAGE_DESCRIPTION "KCM configuration module for wakeup from sleep")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Module for the KDE system setting allowing you to configure which devices will wake your computer up from Sleep or Hibernation")
set(CPACK_PACKAGE_VENDOR "bkoblitz")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "bkoblitz")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "0")

set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${PROJECT_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${PROJECT_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")

set(CPACK_DEBIAN_PACKAGE_DEPENDS " plasma-desktop ")
 
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SECTION "kde")
set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
 
set(CPACK_COMPONENTS_ALL Libraries ApplicationData)
include(CPack)
 
endif(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
