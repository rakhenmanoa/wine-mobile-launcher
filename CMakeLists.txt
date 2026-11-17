cmake_minimum_required(VERSION 3.21)

project(wine-mobile-launcher VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup()

# QTP0001 (Qt >= 6.8) : préfixe de ressources :/qt/qml/ — main.cpp charge déjà
# ce chemin (qrc:/qt/qml/...). QTP0004 : qmldir par répertoire de QML_FILES.
if(QT6_VERSION VERSION_GREATER_EQUAL "6.8")
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(wine-mobile-launcher src/main.cpp)

qt_add_qml_module(wine-mobile-launcher
    URI org.rakhenmanoa.wineLauncher
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/LibraryPage.qml
        qml/GameDetailPage.qml
        qml/EditGamePage.qml
        qml/SettingsPage.qml
    SOURCES
        src/gamemodel.h
        src/gamemodel.cpp
        src/gamemanager.h
        src/gamemanager.cpp
)

# Le fichier généré _qmltyperegistrations.cpp inclut les headers de types via
# « #if __has_include(<inputFile>) » : si le chemin n'est trouvé dans aucun
# répertoire d'include de la cible, l'include est silencieusement ignoré et la
# registration échoue (« GameManager » was not declared). Ces trois répertoires
# couvrent toutes les formes de chemin que moc peut écrire dans le JSON
# (gamemodel.h, src/gamemodel.h, ../src/gamemodel.h).
target_include_directories(wine-mobile-launcher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wine-mobile-launcher PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

include(GNUInstallDirs)

install(TARGETS wine-mobile-launcher
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES assets/org.rakhenmanoa.wine-mobile-launcher.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    RENAME org.rakhenmanoa.wine-mobile-launcher.desktop
)

install(FILES assets/org.rakhenmanoa.wine-mobile-launcher.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
    RENAME org.rakhenmanoa.wine-mobile-launcher.metainfo.xml
)

install(FILES assets/icon.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    RENAME org.rakhenmanoa.wine-mobile-launcher.svg
)
