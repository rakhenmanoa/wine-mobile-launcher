import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.rakhenmanoa.wineLauncher

Kirigami.Page {
    id: root

    title: "Bibliothèque"

    function openGame(index) {
        applicationWindow().pageStack.layers.push(detailPageComponent, { gameIndex: index })
    }

    actions: [
        Kirigami.Action {
            text: "Ajouter un jeu"
            icon.name: "list-add-symbolic"
            onTriggered: applicationWindow().pageStack.layers.push(editPageComponent, { gameIndex: -1 })
        },
        Kirigami.Action {
            id: viewModeAction

            text: GameManager.viewMode === "list" ? "Affichage : liste"
                : GameManager.viewMode === "medium" ? "Affichage : icônes moyennes"
                : "Affichage : grandes icônes"
            icon.name: GameManager.viewMode === "list" ? "view-list-symbolic"
                     : GameManager.viewMode === "medium" ? "view-grid-symbolic"
                     : "view-preview-symbolic"
            // Appui simple : faire défiler les modes. Menu : choix direct.
            onTriggered: {
                GameManager.viewMode = GameManager.viewMode === "list" ? "medium"
                                     : GameManager.viewMode === "medium" ? "large" : "list"
            }
            children: [
                Kirigami.Action {
                    text: "Liste"
                    checkable: true
                    checked: GameManager.viewMode === "list"
                    onTriggered: GameManager.viewMode = "list"
                },
                Kirigami.Action {
                    text: "Icônes moyennes"
                    checkable: true
                    checked: GameManager.viewMode === "medium"
                    onTriggered: GameManager.viewMode = "medium"
                },
                Kirigami.Action {
                    text: "Grandes icônes"
                    checkable: true
                    checked: GameManager.viewMode === "large"
                    onTriggered: GameManager.viewMode = "large"
                }
            ]
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

    Connections {
        target: GameManager
        function onGameLaunched(row, success, message) {
            if (success) {
                launchMsg.visible = false
                return
            }
            launchMsg.type = Kirigami.MessageType.Error
            launchMsg.text = message !== "" ? message : "Lancement impossible."
            launchMsg.visible = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Kirigami.InlineMessage {
            id: launchMsg

            Layout.fillWidth: true
            showCloseButton: true
            visible: false
        }

        // Mode « liste » : cartes détaillées (exécutable visible, boutons).
        ListView {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: GameManager.viewMode === "list"
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

                onClicked: root.openGame(model.index)
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

        // Modes « icônes moyennes » et « grandes icônes » : grille de tuiles.
        // Appui sur une tuile : lancer le jeu. Appui long : fiche du jeu.
        // Coins de tuile : arrêt (jeu lancé) et modification du jeu.
        GridView {
            id: gridView

            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: GameManager.viewMode !== "list"
            clip: true
            topMargin: Kirigami.Units.smallSpacing * 2
            bottomMargin: Kirigami.Units.smallSpacing * 2
            model: GameManager.model

            // Largeur cible d'une tuile ; les colonnes se recalculent pour
            // remplir la largeur sans marges perdues.
            property int targetCell: GameManager.viewMode === "large"
                                     ? Kirigami.Units.gridUnit * 12
                                     : Kirigami.Units.gridUnit * 8
            property int columns: Math.max(2, Math.floor(width / targetCell))
            cellWidth: Math.floor(width / columns)
            cellHeight: cellWidth + Kirigami.Units.gridUnit * 2.5

            delegate: Item {
                width: gridView.cellWidth
                height: gridView.cellHeight

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.smallSpacing
                    spacing: Kirigami.Units.smallSpacing

                    Item {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Kirigami.Icon {
                            anchors.centerIn: parent
                            width: Math.min(parent.width - Kirigami.Units.gridUnit,
                                            GameManager.viewMode === "large"
                                                ? Kirigami.Units.iconSizes.enormous
                                                : Kirigami.Units.iconSizes.huge)
                            height: width
                            source: model.icon !== "" ? model.icon : "applications-games"
                        }
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: model.name
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    QQC2.Label {
                        Layout.alignment: Qt.AlignHCenter
                        visible: model.running
                        text: "● En cours"
                        color: Kirigami.Theme.positiveTextColor
                        font.bold: true
                    }
                }

                Row {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: Kirigami.Units.smallSpacing

                    QQC2.ToolButton {
                        visible: model.running
                        icon.name: "media-playback-stop"
                        onClicked: GameManager.stopGame(model.index)
                    }
                    QQC2.ToolButton {
                        icon.name: "document-edit-symbolic"
                        onClicked: applicationWindow().pageStack.layers.push(editPageComponent, { gameIndex: model.index })
                    }
                }

                TapHandler {
                    onTapped: GameManager.launchGame(model.index)
                    onLongPressed: root.openGame(model.index)
                }
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: gridView.count === 0
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
}
