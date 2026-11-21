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

    function addGame() {
        applicationWindow().pageStack.layers.push(editPageComponent, { gameIndex: -1 })
    }

    function lastPlayedText(ms) {
        if (ms <= 0)
            return "Jamais lancé"
        const d = (Date.now() - ms) / 1000
        if (d < 60)
            return "Joué à l'instant"
        if (d < 3600)
            return "Joué il y a " + Math.floor(d / 60) + " min"
        if (d < 86400)
            return "Joué il y a " + Math.floor(d / 3600) + " h"
        return "Joué il y a " + Math.floor(d / 86400) + " j"
    }

    actions: [
        Kirigami.Action {
            text: "Ajouter un jeu"
            icon.name: "list-add-symbolic"
            onTriggered: root.addGame()
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
            bottomMargin: Kirigami.Units.gridUnit * 4
            model: GameManager.model

            delegate: Kirigami.AbstractCard {
                id: listCard

                width: ListView.view.width - Kirigami.Units.largeSpacing * 2
                x: Kirigami.Units.largeSpacing
                // zone tactile confortable
                implicitHeight: Math.max(contentItem.implicitHeight, Kirigami.Units.gridUnit * 4) + topPadding + bottomPadding

                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing

                    GameIcon {
                        Layout.preferredWidth: Kirigami.Units.iconSizes.large
                        Layout.preferredHeight: Kirigami.Units.iconSizes.large
                        name: model.name
                        source: model.icon
                        running: model.running
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: model.name
                            font.bold: true
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1
                            elide: Text.ElideRight
                        }
                        QQC2.Label {
                            Layout.fillWidth: true
                            visible: !model.running
                            text: root.lastPlayedText(model.lastPlayed)
                            elide: Text.ElideRight
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

                    QQC2.Button {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 3
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 3
                        icon.name: model.running ? "media-playback-stop" : "media-playback-start"
                        highlighted: !model.running
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
                    onTriggered: root.addGame()
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
            bottomMargin: Kirigami.Units.gridUnit * 4
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
                id: tile

                width: gridView.cellWidth
                height: gridView.cellHeight

                Rectangle {
                    id: tileCard

                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.smallSpacing
                    radius: Kirigami.Units.largeSpacing
                    color: Qt.tint(Kirigami.Theme.backgroundColor,
                                   Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g,
                                           Kirigami.Theme.textColor.b, tapHandler.pressed ? 0.18 : 0.06))
                    border.width: model.running ? 2 : 1
                    border.color: model.running ? Kirigami.Theme.positiveTextColor
                                                : Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g,
                                                          Kirigami.Theme.textColor.b, 0.12)
                    scale: tapHandler.pressed ? 0.97 : 1
                    Behavior on scale { NumberAnimation { duration: 80 } }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.smallSpacing
                        spacing: Kirigami.Units.smallSpacing

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            GameIcon {
                                anchors.centerIn: parent
                                width: Math.min(parent.width - Kirigami.Units.gridUnit * 2, parent.height,
                                                GameManager.viewMode === "large"
                                                    ? Kirigami.Units.iconSizes.enormous
                                                    : Kirigami.Units.iconSizes.huge)
                                height: width
                                name: model.name
                                source: model.icon
                                running: model.running
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
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text: model.running ? "● En cours" : root.lastPlayedText(model.lastPlayed)
                            color: model.running ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
                            font.bold: model.running
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: model.running ? 1 : 0.65
                            elide: Text.ElideRight
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
                        id: tapHandler

                        onTapped: GameManager.launchGame(model.index)
                        onLongPressed: root.openGame(model.index)
                    }
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
                    onTriggered: root.addGame()
                }
            }
        }
    }

    // Bouton flottant « ajouter un jeu », à portée du pouce
    QQC2.RoundButton {
        id: addButton

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Kirigami.Units.gridUnit
        width: Kirigami.Units.gridUnit * 3.5
        height: width
        icon.name: "list-add-symbolic"
        highlighted: true
        visible: (GameManager.viewMode === "list" ? listView.count : gridView.count) > 0
        z: 10
        onClicked: root.addGame()

        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.text: "Ajouter un jeu"
    }
}
