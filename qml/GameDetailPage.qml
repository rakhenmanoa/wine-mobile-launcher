import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.rakhenmanoa.wineLauncher

Kirigami.ScrollablePage {
    id: root

    property bool logExpanded: false
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
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            id: launchMsg

            Layout.fillWidth: true
            showCloseButton: true
            visible: false
        }

        // En-tête : icône, nom et état
        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            GameIcon {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                Layout.preferredHeight: Kirigami.Units.iconSizes.huge
                name: root.game !== null ? root.game.name : ""
                source: root.game !== null ? root.game.icon : ""
                running: root.game !== null && root.game.running
            }
            Kirigami.Heading {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                level: 2
                wrapMode: Text.Wrap
                text: root.game !== null ? root.game.name : ""
            }
            QQC2.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                font.bold: root.game !== null && root.game.running
                color: root.game !== null && root.game.running ? Kirigami.Theme.positiveTextColor
                                                               : Kirigami.Theme.textColor
                opacity: root.game !== null && root.game.running ? 1 : 0.7
                text: root.game === null ? ""
                    : root.game.running ? "● En cours d'exécution"
                    : root.game.lastPlayed > 0
                      ? "Dernier lancement : " + Qt.formatDateTime(new Date(root.game.lastPlayed), "dd/MM/yyyy HH:mm")
                      : "Jamais lancé"
            }
        }

        // Un seul gros bouton : Jouer, ou Arrêter quand le jeu tourne
        QQC2.Button {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 3
            readonly property bool running: root.game !== null && root.game.running
            text: running ? "Arrêter" : "Jouer"
            icon.name: running ? "media-playback-stop" : "media-playback-start"
            highlighted: !running
            onClicked: running ? GameManager.stopGame(root.gameIndex) : GameManager.launchGame(root.gameIndex)
        }

        // Réglages du jeu
        Kirigami.AbstractCard {
            Layout.fillWidth: true

            contentItem: Kirigami.FormLayout {
                wideMode: false

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
            }
        }

        // Journal : replié par défaut pour laisser la place aux infos du jeu
        RowLayout {
            Layout.fillWidth: true

            QQC2.Button {
                Layout.fillWidth: true
                flat: true
                text: (root.logExpanded ? "▾ " : "▸ ") + "Journal Wine"
                onClicked: root.logExpanded = !root.logExpanded
                contentItem: QQC2.Label {
                    text: parent.text
                    font.bold: true
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
            }
            QQC2.ToolButton {
                visible: root.logExpanded
                icon.name: "edit-clear-all-symbolic"
                onClicked: GameManager.clearLog()
            }
        }

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 14
            visible: root.logExpanded

            QQC2.TextArea {
                readOnly: true
                wrapMode: QQC2.TextArea.WrapAnywhere
                text: GameManager.logOutput
                font.family: "monospace"
            }
        }
    }
}
