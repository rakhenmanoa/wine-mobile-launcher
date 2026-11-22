import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import org.rakhenmanoa.wineLauncher

Kirigami.ScrollablePage {
    id: root

    property int gameIndex: -1
    property bool prefixCreating: false

    title: gameIndex >= 0 ? "Modifier le jeu" : "Nouveau jeu"

    // Chargement des champs : pas de sauvegarde automatique tant que c'est en cours
    property bool loaded: false

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
            gamepadCheck.checked = g.gamepad
            zinkCheck.checked = g.zink
            waylandCheck.checked = g.wayland
            touchMouseCheck.checked = g.touchMouse
        }
        loaded = true
    }

    // Jeu existant : enregistrement automatique (légèrement différé pendant la frappe)
    function scheduleSave() {
        if (loaded && gameIndex >= 0)
            saveTimer.restart()
    }

    // Quitter la page : on enregistre ce qui est en attente
    Component.onDestruction: {
        if (saveTimer.running) {
            saveTimer.stop()
            root.save(false)
        }
    }

    Timer {
        id: saveTimer
        interval: 700
        onTriggered: root.save(false)
    }

    function save(closePage) {
        if (nameField.text.trim() === "" || exeField.text.trim() === "") {
            // Édition auto : on attend que les champs obligatoires soient remplis
            if (closePage) {
                formError.type = Kirigami.MessageType.Error
                formError.text = "Le nom et le chemin de l'exécutable sont obligatoires."
                formError.visible = true
            }
            return
        }
        formError.visible = false
        if (root.gameIndex >= 0) {
            GameManager.updateGame(root.gameIndex, nameField.text, exeField.text,
                                   argsField.text, prefixField.text, wineField.text,
                                   protonField.text, iconField.text, mangohudCheck.checked,
                                   fullscreenCheck.checked, gamepadCheck.checked, zinkCheck.checked,
                                   waylandCheck.checked, touchMouseCheck.checked)
        } else {
            GameManager.addGame(nameField.text, exeField.text, argsField.text,
                                prefixField.text, wineField.text, protonField.text,
                                iconField.text, mangohudCheck.checked,
                                fullscreenCheck.checked, gamepadCheck.checked, zinkCheck.checked,
                                   waylandCheck.checked, touchMouseCheck.checked)
        }
        if (closePage)
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

        QQC2.TabBar {
            id: tabs

            Layout.fillWidth: true
            QQC2.TabButton { text: "Général" }
            QQC2.TabButton { text: "Affichage et graphisme" }
        }

        Kirigami.FormLayout {
            visible: tabs.currentIndex === 0
            wideMode: false
            Layout.fillWidth: true

            QQC2.TextField {
                id: nameField
                onTextEdited: root.scheduleSave()

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
                    onTextEdited: root.scheduleSave()

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
                onTextEdited: root.scheduleSave()

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
                    onTextEdited: root.scheduleSave()

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
                    onTextEdited: root.scheduleSave()

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
                    onTextEdited: root.scheduleSave()

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
                onToggled: root.scheduleSave()

                Layout.fillWidth: true
                text: "Overlay MangoHud (FPS, CPU/GPU)"
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
                    onTextEdited: root.scheduleSave()

                    Layout.fillWidth: true
                    placeholderText: "image du jeu (optionnel)"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-folder-symbolic"
                    onClicked: iconDialog.open()
                }
            }
        }

        Kirigami.FormLayout {
            id: displayForm

            visible: tabs.currentIndex === 1
            wideMode: false
            Layout.fillWidth: true

            QQC2.CheckBox {
                id: fullscreenCheck
                onToggled: root.scheduleSave()

                Layout.fillWidth: true
                text: "Démarrer en plein écran (paysage)"
            }

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                opacity: 0.8
                text: "Plein écran paysage : par défaut le jeu tourne dans un sway imbriqué (pacman -S sway) dont la sortie est pivotée de 90° — seul le jeu est pivoté, pas l'écran du système. Avec le pilote Wayland ci-dessous, la rotation est faite par Wine lui-même, sans sway. Sans sway ni Wayland : bureau virtuel Wine, sans rotation d'image. Le jeu démarre en fenêtré, puis le launcher le passe en plein écran via KWin (qdbus6, paquet qt6-tools)."
            }

            QQC2.CheckBox {
                id: waylandCheck
                onToggled: root.scheduleSave()

                Layout.fillWidth: true
                text: "Pilote Wayland de Wine (sans Xwayland)"
            }

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                opacity: 0.8
                text: "Active PROTON_ENABLE_WAYLAND. Avec le plein écran, l'image est pivotée par Wine (WINE_WAYLAND_ROTATE) : nécessite le winewayland.so modifié dans GE-Proton ; sinon pas de rotation."
            }

            QQC2.CheckBox {
                id: touchMouseCheck
                onToggled: root.scheduleSave()

                enabled: waylandCheck.checked
                Layout.fillWidth: true
                text: "Tactile émulé en souris"
            }

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                opacity: 0.8
                text: "Le premier doigt déplace le curseur et clique (WINE_WAYLAND_TOUCH_MOUSE). Utile pour les jeux qui n'écoutent que la souris. Nécessite le pilote Wayland."
            }

            QQC2.CheckBox {
                id: zinkCheck

                Layout.fillWidth: true
                text: "OpenGL via Zink (Vulkan)"
                onToggled: root.scheduleSave()
            }

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                opacity: 0.8
                text: "Pour les jeux OpenGL : Mesa traduit OpenGL en Vulkan (MESA_LOADER_DRIVER_OVERRIDE=zink). Les jeux D3D passent déjà par DXVK/vkd3d (Vulkan) ; utile avec le pilote Wayland, qui ne gère pas OpenGL en direct."
            }

            QQC2.CheckBox {
                id: gamepadCheck

                enabled: fullscreenCheck.checked
                Layout.fillWidth: true
                text: "Manette à l'écran"
                onToggled: root.scheduleSave()
            }

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                opacity: 0.8
                text: "Disponible avec le plein écran. Superpose une manette tactile (sticks, croix, ABXY, L/R/ZL/ZR, −/+) au jeu, vue par Wine comme une manette Xbox 360 (uinput) ; le bouton ⌂ ramène à l'accueil de Plasma. Nécessite l'accès à /dev/uinput (règle udev du paquet, puis redémarrage) ; layer-shell-qt est recommandé."
            }
        }

        QQC2.Button {
            Layout.fillWidth: true
            visible: root.gameIndex < 0
            highlighted: true
            text: "Ajouter le jeu"
            icon.name: "list-add-symbolic"
            onClicked: root.save(true)
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
        onAccepted: { exeField.text = GameManager.urlToPath(selectedFile); root.scheduleSave() }
    }
    FolderDialog {
        id: prefixDialog

        onAccepted: { prefixField.text = GameManager.urlToPath(selectedFolder); root.scheduleSave() }
    }
    FileDialog {
        id: wineDialog

        nameFilters: ["Tous les fichiers (*)"]
        onAccepted: { wineField.text = GameManager.urlToPath(selectedFile); root.scheduleSave() }
    }
    FolderDialog {
        id: protonDialog

        onAccepted: { protonField.text = GameManager.urlToPath(selectedFolder); root.scheduleSave() }
    }
    FileDialog {
        id: iconDialog

        nameFilters: ["Images (*.png *.jpg *.jpeg *.svg *.ico *.bmp)", "Tous les fichiers (*)"]
        onAccepted: { iconField.text = GameManager.urlToPath(selectedFile); root.scheduleSave() }
    }
}
