import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.rakhenmanoa.wineLauncher

Kirigami.Page {
    id: root

    property int gameIndex: -1
    property var game: null

    title: game !== null && game.name !== "" ? game.name : "Détail"

    Component.onCompleted: refresh()
    onGameIndexChanged: refresh()

    function refresh() {
        root.game = GameManager.model.get(root.gameIndex)
    }

    Connections {
        target: GameManager.model
        function onDataChanged() {
            root.refresh()
        }
    }

    Connections {
        target: GameManager
        function onGameLaunched(row, success, message) {
            if (row !== root.gameIndex)
                return
            if (!success && message !== "") {
                launchMsg.type = Kirigami.MessageType.Error
                launchMsg.text = message
                launchMsg.visible = true
            } else if (success) {
                launchMsg.visible = false
            }
        }
    }

    actions: [
        Kirigami.Action {
            text: "Modifier"
            icon.name: "document-edit-symbolic"
            onTriggered: applicationWindow().pageStack.layers.push(editPageComponent, { gameIndex: root.gameIndex })
        },
        Kirigami.Action {
            text: "Extraire l'icône"
            icon.name: "view-refresh-symbolic"
            onTriggered: GameManager.refreshIcon(root.gameIndex)
        },
        Kirigami.Action {
            text: "Supprimer"
            icon.name: "edit-delete-symbolic"
            onTriggered: confirmDialog.open()
        }
    ]

    Component {
        id: editPageComponent
        EditGamePage {}
    }

    QQC2.Dialog {
        id: confirmDialog

        modal: true
        title: "Supprimer le jeu"
        standardButtons: QQC2.Dialog.Yes | QQC2.Dialog.No
        onAccepted: {
            GameManager.removeGame(root.gameIndex)
            applicationWindow().pageStack.layers.clear()
        }

        contentItem: QQC2.Label {
            text: root.game !== null
                  ? "Supprimer « " + root.game.name + " » de la bibliothèque ?\nLe préfixe Wine et ses fichiers ne sont pas effacés."
                  : ""
            wrapMode: Text.WordWrap
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            id: launchMsg

            Layout.fillWidth: true
            showCloseButton: true
            visible: false
        }

        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Kirigami.Units.iconSizes.large
            Layout.preferredHeight: Kirigami.Units.iconSizes.large
            source: root.game !== null && root.game.icon !== ""
                    ? root.game.icon : "applications-games"
        }

        Kirigami.FormLayout {
            wideMode: false
            Layout.fillWidth: true

            QQC2.Label {
                Kirigami.FormData.label: "Exécutable :"
                Layout.fillWidth: true
                text: root.game !== null ? root.game.exe : ""
                elide: Text.ElideMiddle
            }
            QQC2.Label {
                Kirigami.FormData.label: "Arguments :"
                Layout.fillWidth: true
                text: root.game !== null && root.game.args !== "" ? root.game.args : "—"
                elide: Text.ElideMiddle
            }
            QQC2.Label {
                Kirigami.FormData.label: "Préfixe Wine :"
                Layout.fillWidth: true
                text: root.game !== null && root.game.prefix !== "" ? root.game.prefix : "automatique"
                elide: Text.ElideMiddle
            }
            QQC2.Label {
                Kirigami.FormData.label: "Binaire Wine :"
                Layout.fillWidth: true
                text: root.game !== null && root.game.wine !== "" ? root.game.wine : "défaut"
                elide: Text.ElideMiddle
            }
            QQC2.Label {
                Kirigami.FormData.label: "GE-Proton :"
                Layout.fillWidth: true
                text: root.game !== null && root.game.proton !== "" ? root.game.proton : "défaut"
                elide: Text.ElideMiddle
            }
            QQC2.Label {
                Kirigami.FormData.label: "Dernier lancement :"
                Layout.fillWidth: true
                text: root.game !== null && root.game.lastPlayed > 0
                      ? Qt.formatDateTime(new Date(root.game.lastPlayed), "dd/MM/yyyy HH:mm")
                      : "jamais"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing

            QQC2.Button {
                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * 2.5
                text: "Jouer"
                icon.name: "media-playback-start"
                highlighted: true
                enabled: root.game !== null && !root.game.running
                onClicked: GameManager.launchGame(root.gameIndex)
            }
            QQC2.Button {
                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * 2.5
                text: "Arrêter"
                icon.name: "media-playback-stop"
                enabled: root.game !== null && root.game.running
                onClicked: GameManager.stopGame(root.gameIndex)
            }
        }

        RowLayout {
            Layout.fillWidth: true

            QQC2.Label {
                Layout.fillWidth: true
                text: "Journal Wine"
                font.bold: true
            }
            QQC2.ToolButton {
                icon.name: "edit-clear-all-symbolic"
                onClicked: GameManager.clearLog()
            }
        }

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            QQC2.TextArea {
                readOnly: true
                wrapMode: QQC2.TextArea.WrapAnywhere
                text: GameManager.logOutput
                font.family: "monospace"
            }
        }
    }
}
