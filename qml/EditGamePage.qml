import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import org.rakhenmanoa.wineLauncher

Kirigami.Page {
    id: root

    property int gameIndex: -1
    property bool prefixCreating: false

    title: gameIndex >= 0 ? "Modifier le jeu" : "Nouveau jeu"

    actions: [
        Kirigami.Action {
            text: "Enregistrer"
            icon.name: "document-save-symbolic"
            onTriggered: root.save()
        }
    ]

    Component.onCompleted: {
        if (gameIndex >= 0) {
            const g = GameManager.model.get(gameIndex)
            nameField.text = g.name
            exeField.text = g.exe
            argsField.text = g.args
            prefixField.text = g.prefix
            wineField.text = g.wine
            protonField.text = g.proton
            iconField.text = g.icon
            mangohudCheck.checked = g.mangohud
            fullscreenCheck.checked = g.fullscreen
        }
    }

    function save() {
        if (nameField.text.trim() === "" || exeField.text.trim() === "") {
            formError.type = Kirigami.MessageType.Error
            formError.text = "Le nom et le chemin de l'exécutable sont obligatoires."
            formError.visible = true
            return
        }
        if (root.gameIndex >= 0) {
            GameManager.updateGame(root.gameIndex, nameField.text, exeField.text,
                                   argsField.text, prefixField.text, wineField.text,
                                   protonField.text, iconField.text, mangohudCheck.checked,
                                   fullscreenCheck.checked)
        } else {
            GameManager.addGame(nameField.text, exeField.text, argsField.text,
                                prefixField.text, wineField.text, protonField.text,
                                iconField.text, mangohudCheck.checked,
                                fullscreenCheck.checked)
        }
        applicationWindow().pageStack.layers.clear()
    }

    Connections {
        target: GameManager
        function onPrefixCreationFinished(path, success, message) {
            root.prefixCreating = false
            prefixMsg.type = success ? Kirigami.MessageType.Positive : Kirigami.MessageType.Error
            prefixMsg.text = message
            prefixMsg.visible = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            id: formError

            Layout.fillWidth: true
            showCloseButton: true
            visible: false
        }
        Kirigami.InlineMessage {
            id: prefixMsg

            Layout.fillWidth: true
            showCloseButton: true
            visible: false
        }

        Kirigami.FormLayout {
            wideMode: false
            Layout.fillWidth: true

            QQC2.TextField {
                id: nameField

                Kirigami.FormData.label: "Nom du jeu :"
                Layout.fillWidth: true
                placeholderText: "Ex. : Mon jeu Windows"
            }

            RowLayout {
                Kirigami.FormData.label: "Exécutable (.exe) :"
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.TextField {
                    id: exeField

                    Layout.fillWidth: true
                    placeholderText: "/chemin/vers/jeu.exe"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-folder-symbolic"
                    onClicked: exeDialog.open()
                }
            }

            QQC2.TextField {
                id: argsField

                Kirigami.FormData.label: "Arguments :"
                Layout.fillWidth: true
                placeholderText: "optionnels, séparés par des espaces"
            }

            RowLayout {
                Kirigami.FormData.label: "Préfixe Wine :"
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.TextField {
                    id: prefixField

                    Layout.fillWidth: true
                    placeholderText: "vide → préfixe automatique"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-folder-symbolic"
                    onClicked: prefixDialog.open()
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Binaire Wine :"
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.TextField {
                    id: wineField

                    Layout.fillWidth: true
                    placeholderText: GameManager.defaultWineBinary !== ""
                                     ? GameManager.defaultWineBinary : "défaut (wine système)"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-folder-symbolic"
                    onClicked: wineDialog.open()
                }
            }

            RowLayout {
                Kirigami.FormData.label: "GE-Proton :"
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.TextField {
                    id: protonField

                    Layout.fillWidth: true
                    placeholderText: GameManager.defaultProtonDir !== ""
                                     ? GameManager.defaultProtonDir : "aucun (dossier GE-Proton)"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-folder-symbolic"
                    onClicked: protonDialog.open()
                }
            }

            QQC2.CheckBox {
                id: mangohudCheck

                Layout.fillWidth: true
                text: "Overlay MangoHud (FPS, CPU/GPU)"
            }

            QQC2.CheckBox {
                id: fullscreenCheck

                Layout.fillWidth: true
                text: "Démarrer en plein écran (paysage)"
            }

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                opacity: 0.8
                text: "Plein écran paysage : le jeu tourne dans un sway imbriqué (pacman -S sway) dont la sortie est pivotée de 90° — seul le jeu est pivoté, pas l'écran du système. Il démarre en fenêtré, puis le launcher le passe en plein écran via KWin (qdbus6, paquet qt6-tools). Sans sway : bureau virtuel Wine, passé en plein écran de la même façon, sans rotation d'image."
            }

            RowLayout {
                Kirigami.FormData.label: "Icône :"
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                    source: iconField.text !== "" ? iconField.text : "applications-games"
                }
                QQC2.TextField {
                    id: iconField

                    Layout.fillWidth: true
                    placeholderText: "image du jeu (optionnel)"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-folder-symbolic"
                    onClicked: iconDialog.open()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            QQC2.Button {
                text: root.prefixCreating ? "Création du préfixe…" : "Créer / initialiser ce préfixe"
                icon.name: "folder-new-symbolic"
                enabled: prefixField.text.trim() !== "" && !root.prefixCreating
                onClicked: {
                    root.prefixCreating = true
                    prefixMsg.visible = false
                    GameManager.createPrefix(prefixField.text.trim())
                }
            }
            QQC2.BusyIndicator {
                visible: root.prefixCreating
                running: root.prefixCreating
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "Le préfixe Wine est un dossier isolé qui contient tout le « disque C: » virtuel du jeu. Un préfixe par jeu évite les conflits entre applications."
            opacity: 0.8
        }
    }

    FileDialog {
        id: exeDialog

        nameFilters: ["Applications Windows (*.exe *.msi *.bat *.lnk)", "Tous les fichiers (*)"]
        onAccepted: exeField.text = GameManager.urlToPath(selectedFile)
    }
    FolderDialog {
        id: prefixDialog

        onAccepted: prefixField.text = GameManager.urlToPath(selectedFolder)
    }
    FileDialog {
        id: wineDialog

        nameFilters: ["Tous les fichiers (*)"]
        onAccepted: wineField.text = GameManager.urlToPath(selectedFile)
    }
    FolderDialog {
        id: protonDialog

        onAccepted: protonField.text = GameManager.urlToPath(selectedFolder)
    }
    FileDialog {
        id: iconDialog

        nameFilters: ["Images (*.png *.jpg *.jpeg *.svg *.ico *.bmp)", "Tous les fichiers (*)"]
        onAccepted: iconField.text = GameManager.urlToPath(selectedFile)
    }
}
