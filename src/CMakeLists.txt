add_executable(walltz)

ecm_add_qml_module(walltz
    URI org.walltz.walltz
)

target_sources(walltz PRIVATE
    main.cpp
    WallpaperProcessor.h
    WallpaperProcessor.cpp
)

ecm_target_qml_sources(walltz SOURCES
    Main.qml
)

target_link_libraries(walltz PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Gui
    Qt6::QuickControls2
    Qt6::Widgets
    KF6::Kirigami
    KF6::I18n
    KF6::I18nQml
    KF6::CoreAddons
    KF6::IconThemes
)

install(TARGETS walltz ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
