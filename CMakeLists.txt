cmake_minimum_required(VERSION 3.21)

project(wine-mobile-launcher VERSION 0.2.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

# REQUIRES 6.8 : pose QTP0001..QTP0005 en NEW. QTP0001 est crucial : il place
# le module sous /qt/qml/, exactement là où main.cpp charge main.qml
# (qrc:/qt/qml/org/rakhenmanoa/wineLauncher/qml/main.qml — le sous-répertoire
# qml/ est conservé d'après la doc qt_add_qml_module). Sans policy, le
# préfixe par défaut est / et le binaire ne trouve plus son QML au démarrage.
# NB : qt_standard_project_setup() sans REQUIRES ne pose AUCUNE policy, et
# QT6_VERSION n'est pas renseigné par find_package(Qt6) — d'où un garde
# if(QT6_VERSION ...) qui ne se déclenchait jamais.
qt_standard_project_setup(REQUIRES 6.8)

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
