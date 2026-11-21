import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import org.rakhenmanoa.wineLauncher

Kirigami.ScrollablePage {
    id: root

    title: "Paramètres"

    Component.onCompleted: {
        wineField.text = GameManager.defaultWineBinary
        protonField.text = GameManager.defaultProtonDir
        prefixField.text = GameManager.defaultPrefixDir
        verboseCheck.checked = GameManager.verboseTerminal
    }

    function save() {
        GameManager.defaultWineBinary = wineField.text.trim()
        GameManager.defaultProtonDir = protonField.text.trim()
        GameManager.defaultPrefixDir = prefixField.text.trim()
        savedMsg.type = Kirigami.MessageType.Positive
        savedMsg.text = "Paramètres enregistrés."
        savedMsg.visible = true
    }

    function detect() {
        const resolved = GameManager.detectWine(wineField.text.trim(), protonField.text.trim())
        wineMsg.visible = true
        if (resolved === "") {
            wineMsg.type = Kirigami.MessageType.Error
            wineMsg.text = "Wine introuvable. Installez le paquet wine ou indiquez un binaire Wine personnalisé, ou le dossier d'un GE-Proton (contenant le script « proton »)."
        } else {
            wineMsg.type = Kirigami.MessageType.Positive
            wineMsg.text = "Runner détecté : " + resolved
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
        spacing: Kirigami.Units.largeSpacing

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
                spacing: Kirigami.Units.smallSpacing

                QQC2.TextField {
                    id: wineField

                    Layout.fillWidth: true
                    placeholderText: "wine (système)"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-folder-symbolic"
                    onClicked: wineDialog.open()
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Dossier GE-Proton par défaut :"
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.TextField {
                    id: protonField

                    Layout.fillWidth: true
                    placeholderText: "ex. ~/GE-Proton-aarch64 (prioritaire sur Wine)"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-folder-symbolic"
                    onClicked: protonDialog.open()
                }
                QQC2.ToolButton {
                    icon.name: "view-refresh-symbolic"
                    onClicked: root.detect()
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Dossier des préfixes :"
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

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

            QQC2.CheckBox {
                id: verboseCheck

                Layout.fillWidth: true
                text: "Journal des jeux dans le terminal (verbose)"
                onToggled: GameManager.verboseTerminal = checked
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "GE-Proton (y compris les builds aarch64) : renseignez le dossier extrait (celui qui contient le script « proton ») dans « Dossier GE-Proton par défaut » ; s'il est défini, il est utilisé à la place de Wine. Chaque jeu peut aussi avoir son propre GE-Proton. Le jeu est alors lancé via « proton run » ; le préfixe Wine se trouve dans le sous-dossier « pfx » du dossier de préfixe du jeu."
            opacity: 0.8
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "Chaque jeu possède son propre préfixe : un « C: » virtuel isolé. Sans préfixe défini pour un jeu, un dossier est créé automatiquement ici, nommé d'après le jeu."
            opacity: 0.8
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "« Journal dans le terminal » : quand l'application est lancée depuis un terminal, tout le journal (lancements, sortie Wine/Proton, erreurs) y est recopié en direct — pratique pour diagnostiquer umu-run/GE-Proton sans quitter l'appli. Ce mode laisse aussi Wine afficher ses messages d'erreur (au lieu de WINEDEBUG=-all). Le journal est en plus enregistré dans le fichier : " + GameManager.logFilePath()
            opacity: 0.8
        }
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
    FolderDialog {
        id: prefixDialog

        onAccepted: prefixField.text = GameManager.urlToPath(selectedFolder)
    }
}
