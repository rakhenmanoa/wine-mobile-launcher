import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import org.rakhenmanoa.wineLauncher

Kirigami.Page {
    id: root

    title: "Paramètres"

    Component.onCompleted: {
        wineField.text = GameManager.defaultWineBinary
        prefixField.text = GameManager.defaultPrefixDir
    }

    function save() {
        GameManager.defaultWineBinary = wineField.text.trim()
        GameManager.defaultPrefixDir = prefixField.text.trim()
        savedMsg.type = Kirigami.MessageType.Positive
        savedMsg.text = "Paramètres enregistrés."
        savedMsg.visible = true
    }

    function detect() {
        const resolved = GameManager.detectWine(wineField.text.trim())
        wineMsg.visible = true
        if (resolved === "") {
            wineMsg.type = Kirigami.MessageType.Error
            wineMsg.text = "Wine introuvable. Installez le paquet wine ou indiquez un binaire personnalisé (ex. : Wine-GE, Proton)."
        } else {
            wineMsg.type = Kirigami.MessageType.Positive
            wineMsg.text = "Wine détecté : " + resolved
        }
    }

    actions: [
        Kirigami.Action {
            text: "Enregistrer"
            icon.name: "document-save-symbolic"
            onTriggered: root.save()
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.units.largeSpacing
        spacing: Kirigami.units.largeSpacing

        Kirigami.InlineMessage {
            id: savedMsg

            Layout.fillWidth: true
            showCloseButton: true
            visible: false
        }
        Kirigami.InlineMessage {
            id: wineMsg

            Layout.fillWidth: true
            showCloseButton: true
            visible: false
        }

        Kirigami.FormLayout {
            wideMode: false
            Layout.fillWidth: true

            RowLayout {
                Kirigami.FormData.label: "Binaire Wine par défaut :"
                Layout.fillWidth: true
                spacing: Kirigami.units.smallSpacing

                QQC2.TextField {
                    id: wineField

                    Layout.fillWidth: true
                    placeholderText: "wine (système)"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-folder-symbolic"
                    onClicked: wineDialog.open()
                }
                QQC2.ToolButton {
                    icon.name: "view-refresh-symbolic"
                    onClicked: root.detect()
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Dossier des préfixes :"
                Layout.fillWidth: true
                spacing: Kirigami.units.smallSpacing

                QQC2.TextField {
                    id: prefixField

                    Layout.fillWidth: true
                }
                QQC2.ToolButton {
                    icon.name: "document-open-folder-symbolic"
                    onClicked: prefixDialog.open()
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    visible: prefixField.text.trim() !== ""
                    onClicked: GameManager.openPath(prefixField.text.trim())
                }
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "Chaque jeu possède son propre préfixe : un « C: » virtuel isolé. Sans préfixe défini pour un jeu, un dossier est créé automatiquement ici, nommé d'après le jeu."
            opacity: 0.8
        }
    }

    FileDialog {
        id: wineDialog

        nameFilters: ["Tous les fichiers (*)"]
        onAccepted: wineField.text = GameManager.urlToPath(selectedFile)
    }
    FolderDialog {
        id: prefixDialog

        onAccepted: prefixField.text = GameManager.urlToPath(selectedFolder)
    }
}
