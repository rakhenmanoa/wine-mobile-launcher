import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.rakhenmanoa.wineLauncher

Kirigami.ApplicationWindow {
    id: root

    title: "Wine Mobile Launcher"
    minimumWidth: Kirigami.Units.gridUnit * 16
    minimumHeight: Kirigami.Units.gridUnit * 22

    globalDrawer: Kirigami.GlobalDrawer {
        title: "Wine Mobile Launcher"
        titleIcon: "applications-games"
        isMenu: false

        actions: [
            Kirigami.Action {
                text: "Bibliothèque"
                icon.name: "view-grid-symbolic"
                onTriggered: root.pageStack.layers.clear()
            },
            Kirigami.Action {
                text: "Paramètres"
                icon.name: "settings-configure-symbolic"
                onTriggered: {
                    root.pageStack.layers.clear()
                    root.pageStack.layers.push(settingsPageComponent)
                }
            }
        ]
    }

    Component {
        id: settingsPageComponent
        SettingsPage {}
    }

    pageStack.initialPage: LibraryPage {}
}
