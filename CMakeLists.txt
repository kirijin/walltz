cmake_minimum_required(VERSION 3.20)
project(walltz)
set(VERSION "0.1.0")

find_package(ECM 6.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMFindQmlModule)
include(ECMQmlModule)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Quick
    Gui
    QuickControls2
    Widgets
)

find_package(KF6 REQUIRED COMPONENTS
    Kirigami
    I18n
    CoreAddons
    IconThemes
)

ecm_find_qmlmodule(QtQuick)
ecm_find_qmlmodule(QtQuick.Layouts)
ecm_find_qmlmodule(QtQuick.Controls)
ecm_find_qmlmodule(org.kde.kirigami)

add_subdirectory(src)

install(FILES org.walltz.walltz.desktop
    DESTINATION ${KDE_INSTALL_APPDIR}
)
