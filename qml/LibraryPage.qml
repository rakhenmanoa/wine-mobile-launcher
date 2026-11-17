import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.rakhenmanoa.wineLauncher

Kirigami.Page {
    id: root

    title: "Bibliothèque"

    actions: [
        Kirigami.Action {
            text: "Ajouter un jeu"
            icon.name: "list-add-symbolic"
            onTriggered: applicationWindow().pageStack.layers.push(editPageComponent, { gameIndex: -1 })
        }
    ]

    Component {
        id: editPageComponent
        EditGamePage {}
    }
    Component {
        id: detailPageComponent
        GameDetailPage {}
    }

    ListView {
        id: listView

        anchors.fill: parent
        clip: true
        spacing: Kirigami.Units.smallSpacing * 2
        topMargin: Kirigami.Units.smallSpacing * 2
        bottomMargin: Kirigami.Units.smallSpacing * 2
        model: GameManager.model

        delegate: Kirigami.AbstractCard {
            width: ListView.view.width

            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                    source: model.icon !== "" ? model.icon : "applications-games"
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: model.name
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: model.exe
                        elide: Text.ElideMiddle
                        opacity: 0.7
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: model.running
                        text: "● En cours d'exécution"
                        color: Kirigami.Theme.positiveTextColor
                        font.bold: true
                    }
                }

                QQC2.ToolButton {
                    icon.name: model.running ? "media-playback-stop" : "media-playback-start"
                    onClicked: model.running ? GameManager.stopGame(model.index)
                                             : GameManager.launchGame(model.index)
                }
            }

            onClicked: applicationWindow().pageStack.layers.push(detailPageComponent, { gameIndex: model.index })
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: listView.count === 0
            icon.name: "applications-games-symbolic"
            text: "Aucun jeu dans la bibliothèque"
            explanation: "Ajoutez une application Windows (.exe) et son préfixe Wine pour commencer."
            helpfulAction: Kirigami.Action {
                text: "Ajouter un jeu"
                icon.name: "list-add-symbolic"
                onTriggered: applicationWindow().pageStack.layers.push(editPageComponent, { gameIndex: -1 })
            }
        }
    }
}
