import QtQuick
import QtQuick.Window

// Manette à l'écran : fenêtre plein écran transparente (couche overlay) dont seuls
// les boutons reçoivent les touches ; le reste de l'écran traverse l'overlay.
// Codes : constantes du noyau Linux (linux/input-event-codes.h).
Window {
    id: win

    readonly property int btnSouth: 0x130
    readonly property int btnEast: 0x131
    readonly property int btnNorth: 0x133
    readonly property int btnWest: 0x134
    readonly property int btnTL: 0x136
    readonly property int btnTR: 0x137
    readonly property int btnSelect: 0x13a
    readonly property int btnStart: 0x13b
    readonly property int btnMode: 0x13c
    readonly property int btnThumbL: 0x13d
    readonly property int btnThumbR: 0x13e
    readonly property int absX: 0x00
    readonly property int absY: 0x01
    readonly property int absZ: 0x02
    readonly property int absRX: 0x03
    readonly property int absRY: 0x04
    readonly property int absRZ: 0x05
    readonly property int absHatX: 0x10
    readonly property int absHatY: 0x11

    // Les contrôles vivent dans un repère « paysage » tourné comme le jeu
    readonly property bool turned: overlayRotation === 90 || overlayRotation === 270
    // Disposition personnalisée : mode édition (lancé depuis la fiche du jeu) ou jeu
    // overlayEdit : démarre en mode édition ; overlayStandalone : éditeur seul (« Terminer » quitte),
    // sinon « Terminer » passe en mode jeu (édition au lancement du jeu).
    property bool editMode: overlayEdit
    readonly property real unit: Math.max(48, Math.min(content.width, content.height) / 7)
    readonly property var controlList: [dpad, leftStick, rightStick, bA, bB, bX, bY, zl, l, zr, r, selectBtn, homeBtn, startBtn]

    flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus | Qt.WindowStaysOnTopHint
    color: "transparent"
    visible: true
    visibility: Window.FullScreen

    Component.onCompleted: {
        pad.setupOverlay(win)
        content.layout = pad.loadLayout()
        Qt.callLater(updateMask)
    }

    function updateMask() {
        const rects = []
        for (let i = 0; i < content.children.length; ++i) {
            const c = content.children[i]
            if (!c.touchZone)
                continue
            const r = c.mapRectToItem(null, 0, 0, c.width, c.height)
            rects.push({x: r.x, y: r.y, w: r.width, h: r.height})
        }
        pad.setInputMask(win, rects)
    }
    onWidthChanged: Qt.callLater(updateMask)
    onHeightChanged: Qt.callLater(updateMask)

    // La fenêtre layer-shell reçoit sa taille et les positions des contrôles après coup : on
    // recalcule le masque (zone qui capte les touches) quelques fois au démarrage.
    Timer { interval: 300; running: true; onTriggered: win.updateMask() }
    Timer { interval: 1500; running: true; onTriggered: win.updateMask() }

    Rectangle {
        anchors.fill: parent
        visible: win.editMode
        color: "#66000000"
    }

    Item {
        id: content

        anchors.centerIn: parent
        width: win.turned ? win.height : win.width
        height: win.turned ? win.width : win.height
        rotation: overlayRotation
        onWidthChanged: Qt.callLater(win.updateMask)
        onHeightChanged: Qt.callLater(win.updateMask)
        Component.onCompleted: Qt.callLater(win.updateMask)

        // Positions et tailles personnalisées : { <clé>: {x, y, s} } (x, y : centre du
        // contrôle en fraction de l'écran, s : facteur de taille propre au contrôle)
        property var layout: ({})
        property string selected: ""
        function posX(key, w, def) {
            const p = layout[key]
            return p && p.x !== undefined ? p.x * width - w / 2 : def
        }
        function posY(key, h, def) {
            const p = layout[key]
            return p && p.y !== undefined ? p.y * height - h / 2 : def
        }
        function scaleOf(key) {
            const p = layout[key]
            return p && p.s > 0 ? p.s : 1
        }
        function itemFor(key) {
            for (let i = 0; i < win.controlList.length; ++i)
                if (win.controlList[i].key === key)
                    return win.controlList[i]
            return null
        }
        function setCenter(key, cx, cy) {
            const l = Object.assign({}, layout)
            l[key] = Object.assign({}, layout[key] || {}, {
                x: Math.max(0, Math.min(1, cx)), y: Math.max(0, Math.min(1, cy)) })
            layout = l
        }
        // Change la taille du contrôle sélectionné en gardant son centre
        function setScaleOf(key, v) {
            const it = itemFor(key)
            if (!it)
                return
            const l = Object.assign({}, layout)
            const cur = Object.assign({}, l[key] || {})
            if (cur.x === undefined) {
                cur.x = (it.x + it.width / 2) / width
                cur.y = (it.y + it.height / 2) / height
            }
            cur.s = Math.max(0.5, Math.min(2, v))
            l[key] = cur
            layout = l
            commit()
        }
        function commit() {
            pad.saveLayout(layout)
            Qt.callLater(win.updateMask)
        }

        // ----- bouton rond / pastille générique -----
        component PadButton: Rectangle {
            id: btn
            property string label
            property int code
            property string key
            readonly property real k: content.scaleOf(key)
            property bool touchZone: true
            property bool pressed: false
            width: win.unit * 0.9 * k
            height: width
            radius: width / 2
            color: pressed ? "#cc3daee9" : "#66202428"
            border.color: "#aaffffff"
            border.width: 2
            Text {
                anchors.centerIn: parent
                text: btn.label
                color: "white"
                font.bold: true
                font.pixelSize: btn.width * 0.38
            }
            MultiPointTouchArea {
                enabled: !win.editMode
                anchors.fill: parent
                anchors.margins: -win.unit * 0.15
                maximumTouchPoints: 1
                onPressed: { btn.pressed = true; pad.button(btn.code, true) }
                onReleased: { btn.pressed = false; pad.button(btn.code, false) }
                onCanceled: { btn.pressed = false; pad.button(btn.code, false) }
            }
        }

        // Gâchette (ZL/ZR analogiques) ou épaule (L/R numériques)
        component Shoulder: Rectangle {
            id: sh
            property string label
            property int code
            property string key
            property int axisCode: -1
            property bool touchZone: true
            property bool pressed: false
            readonly property real k: content.scaleOf(key)
            width: win.unit * 1.5 * k
            height: win.unit * 0.7 * k
            radius: height / 3
            color: pressed ? "#cc3daee9" : "#66202428"
            border.color: "#aaffffff"
            border.width: 2
            Text {
                anchors.centerIn: parent
                text: sh.label
                color: "white"
                font.bold: true
                font.pixelSize: sh.height * 0.45
            }
            MultiPointTouchArea {
                enabled: !win.editMode
                anchors.fill: parent
                maximumTouchPoints: 1
                function set(on) {
                    sh.pressed = on
                    if (sh.axisCode >= 0)
                        pad.axis(sh.axisCode, on ? 1 : 0)
                    else
                        pad.button(sh.code, on)
                }
                onPressed: set(true)
                onReleased: set(false)
                onCanceled: set(false)
            }
        }

        // Petits boutons Sélection / Accueil / Start
        component SmallButton: Rectangle {
            id: sm
            property string label
            property int code
            property string key
            property bool touchZone: true
            property bool pressed: false
            property bool isHome: false
            readonly property real k: content.scaleOf(key)
            width: win.unit * 0.95 * k
            height: win.unit * 0.5 * k
            radius: height / 2
            color: pressed ? "#cc3daee9" : "#66202428"
            border.color: "#aaffffff"
            border.width: 2
            Text {
                anchors.centerIn: parent
                text: sm.label
                color: "white"
                font.pixelSize: sm.height * 0.45
            }
            MultiPointTouchArea {
                enabled: !win.editMode
                anchors.fill: parent
                anchors.margins: -win.unit * 0.1
                maximumTouchPoints: 1
                onPressed: { sm.pressed = true; if (!sm.isHome) pad.button(sm.code, true) }
                onReleased: { sm.pressed = false; if (sm.isHome) pad.goHome(); else pad.button(sm.code, false) }
                onCanceled: { sm.pressed = false; if (!sm.isHome) pad.button(sm.code, false) }
            }
        }

        // Stick analogique
        component Stick: Rectangle {
            id: stick
            property int axisX
            property int axisY
            property string key
            property int thumbCode
            property bool touchZone: true
            readonly property real k: content.scaleOf(key)
            width: win.unit * 2.4 * k
            height: width
            radius: width / 2
            color: "#44202428"
            border.color: "#88ffffff"
            border.width: 2
            Rectangle {
                id: knob
                width: stick.width * 0.45
                height: width
                radius: width / 2
                color: "#aa3daee9"
                border.color: "white"
                border.width: 2
                x: (stick.width - width) / 2
                y: (stick.height - height) / 2
            }
            MultiPointTouchArea {
                enabled: !win.editMode
                anchors.fill: parent
                maximumTouchPoints: 1
                function move(p) {
                    const half = (stick.width - knob.width) / 2
                    let dx = (p.x - stick.width / 2) / half
                    let dy = (p.y - stick.height / 2) / half
                    const len = Math.hypot(dx, dy)
                    if (len > 1) { dx /= len; dy /= len }
                    knob.x = (stick.width - knob.width) / 2 + dx * half
                    knob.y = (stick.height - knob.height) / 2 + dy * half
                    pad.axis(stick.axisX, dx)
                    pad.axis(stick.axisY, dy)
                }
                function reset() {
                    knob.x = (stick.width - knob.width) / 2
                    knob.y = (stick.height - knob.height) / 2
                    pad.axis(stick.axisX, 0)
                    pad.axis(stick.axisY, 0)
                }
                onPressed: (points) => move(points[0])
                onUpdated: (points) => move(points[0])
                onReleased: reset()
                onCanceled: reset()
            }
        }

        // Croix directionnelle (hat)
        Item {
            id: dpad
            property bool touchZone: true
            readonly property string key: "dpad"
            x: content.posX(key, width, win.unit * 0.4)
            y: content.posY(key, height, content.height - height - win.unit * 0.4)
            readonly property real k: content.scaleOf(key)
            width: win.unit * 2.6 * k
            height: width
            Rectangle {
                anchors.centerIn: parent
                width: parent.width
                height: parent.width * 0.38
                radius: height / 3
                color: "#55202428"
                border.color: "#88ffffff"
                border.width: 2
            }
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.38
                height: parent.width
                radius: width / 3
                color: "#55202428"
                border.color: "#88ffffff"
                border.width: 2
            }
            Rectangle {
                id: dpadMark
                visible: false
                width: parent.width * 0.3
                height: width
                radius: width / 2
                color: "#cc3daee9"
            }
            MultiPointTouchArea {
                enabled: !win.editMode
                anchors.fill: parent
                maximumTouchPoints: 1
                function set(p) {
                    const dx = p.x / dpad.width - 0.5
                    const dy = p.y / dpad.height - 0.5
                    let hx = 0, hy = 0
                    if (Math.hypot(dx, dy) > 0.12) {
                        if (Math.abs(dx) > 0.17) hx = dx > 0 ? 1 : -1
                        if (Math.abs(dy) > 0.17) hy = dy > 0 ? 1 : -1
                    }
                    pad.axis(win.absHatX, hx)
                    pad.axis(win.absHatY, hy)
                    dpadMark.visible = hx !== 0 || hy !== 0
                    dpadMark.x = dpad.width / 2 + hx * dpad.width * 0.3 - dpadMark.width / 2
                    dpadMark.y = dpad.height / 2 + hy * dpad.height * 0.3 - dpadMark.height / 2
                }
                function reset() {
                    pad.axis(win.absHatX, 0)
                    pad.axis(win.absHatY, 0)
                    dpadMark.visible = false
                }
                onPressed: (points) => set(points[0])
                onUpdated: (points) => set(points[0])
                onReleased: reset()
                onCanceled: reset()
            }
        }

        // Épaules / gâchettes (côté gauche : ZL, L ; côté droit : R, ZR)
        Shoulder { id: zl; key: "zl"; label: "ZL"; axisCode: win.absZ; x: content.posX(key, width, win.unit * 0.4); y: content.posY(key, height, win.unit * 0.3) }
        Shoulder { id: l; key: "l"; label: "L"; code: win.btnTL; x: content.posX(key, width, win.unit * 0.4); y: content.posY(key, height, win.unit * 1.15) }
        Shoulder { id: zr; key: "zr"; label: "ZR"; axisCode: win.absRZ; x: content.posX(key, width, content.width - width - win.unit * 0.4); y: content.posY(key, height, win.unit * 0.3) }
        Shoulder { id: r; key: "r"; label: "R"; code: win.btnTR; x: content.posX(key, width, content.width - width - win.unit * 0.4); y: content.posY(key, height, win.unit * 1.15) }

        // Sticks : gauche au-dessus de la croix, droit sous les boutons
        Stick { id: leftStick; key: "leftStick"; axisX: win.absX; axisY: win.absY; x: content.posX(key, width, win.unit * 0.5); y: content.posY(key, height, dpad.y - height - win.unit * 0.3) }
        Stick { id: rightStick; key: "rightStick"; axisX: win.absRX; axisY: win.absRY; x: content.posX(key, width, content.width - width - win.unit * 0.5); y: content.posY(key, height, content.height - height - win.unit * 0.4) }

        // Boutons ABXY : quatre contrôles indépendants (disposition Nintendo : A à droite, B en bas,
        // X en haut, Y à gauche ; codes Xbox : A = SOUTH, B = EAST). Positions par défaut en losange.
        readonly property real faceSize: win.unit * 2.9
        readonly property real faceX: content.width - faceSize - win.unit * 0.4
        readonly property real faceY: content.height - win.unit * 2.4 - win.unit * 0.4 - faceSize - win.unit * 0.2
        PadButton { id: bX; key: "btnX"; label: "X"; code: win.btnNorth
            x: content.posX(key, width, content.faceX + content.faceSize / 2 - width / 2)
            y: content.posY(key, height, content.faceY) }
        PadButton { id: bB; key: "btnB"; label: "B"; code: win.btnEast
            x: content.posX(key, width, content.faceX + content.faceSize / 2 - width / 2)
            y: content.posY(key, height, content.faceY + content.faceSize - win.unit * 0.9) }
        PadButton { id: bY; key: "btnY"; label: "Y"; code: win.btnWest
            x: content.posX(key, width, content.faceX)
            y: content.posY(key, height, content.faceY + content.faceSize / 2 - height / 2) }
        PadButton { id: bA; key: "btnA"; label: "A"; code: win.btnSouth
            x: content.posX(key, width, content.faceX + content.faceSize - win.unit * 0.9)
            y: content.posY(key, height, content.faceY + content.faceSize / 2 - height / 2) }

        // − Home +
        SmallButton { id: selectBtn; key: "select"; label: "−"; code: win.btnSelect; x: content.posX(key, width, content.width / 2 - width * 1.6); y: content.posY(key, height, content.height - height - win.unit * 0.3) }
        SmallButton { id: homeBtn; key: "home"; label: "⌂"; code: win.btnMode; isHome: true; x: content.posX(key, width, content.width / 2 - width / 2); y: content.posY(key, height, content.height - height - win.unit * 0.3) }
        SmallButton { id: startBtn; key: "start"; label: "+"; code: win.btnStart; x: content.posX(key, width, content.width / 2 + width * 0.6); y: content.posY(key, height, content.height - height - win.unit * 0.3) }

        // ----- Mode édition : glisser les contrôles, taille, réinitialiser -----
        Repeater {
            model: win.editMode ? win.controlList : []
            delegate: MultiPointTouchArea {
                id: dragArea
                required property var modelData
                property real offX: 0
                property real offY: 0
                z: 10
                x: modelData.x
                y: modelData.y
                width: modelData.width
                height: modelData.height
                maximumTouchPoints: 1
                function start(p) {
                    const c = mapToItem(content, p.x, p.y)
                    offX = c.x - (modelData.x + modelData.width / 2)
                    offY = c.y - (modelData.y + modelData.height / 2)
                }
                function drag(p) {
                    const c = mapToItem(content, p.x, p.y)
                    content.setCenter(modelData.key, (c.x - offX) / content.width, (c.y - offY) / content.height)
                }
                onPressed: (points) => { content.selected = modelData.key; start(points[0]) }
                onUpdated: (points) => drag(points[0])
                onReleased: content.commit()
                onCanceled: content.commit()
                Rectangle {
                    anchors.fill: parent
                    color: "#223daee9"
                    border.color: content.selected === modelData.key ? "#ffffffff" : "#ffffb300"
                    border.width: content.selected === modelData.key ? 5 : 3
                    radius: 8
                }
            }
        }

        component EditButton: Rectangle {
            id: eb
            property string label
            signal clicked()
            width: Math.max(win.unit * 0.8, ebText.implicitWidth + win.unit * 0.5)
            height: win.unit * 0.6
            radius: height / 2
            color: ebArea.pressed ? "#cc3daee9" : "#cc202428"
            border.color: "#ffffffff"
            border.width: 2
            Text {
                id: ebText
                anchors.centerIn: parent
                text: eb.label
                color: "white"
                font.bold: true
                font.pixelSize: eb.height * 0.45
            }
            MultiPointTouchArea {
                id: ebArea
                property bool pressed: false
                anchors.fill: parent
                maximumTouchPoints: 1
                onPressed: pressed = true
                onReleased: { pressed = false; eb.clicked() }
                onCanceled: pressed = false
            }
        }

        Column {
            id: toolbar
            property bool touchZone: win.editMode
            visible: win.editMode
            z: 20
            spacing: win.unit * 0.15
            x: (content.width - width) / 2
            y: win.unit * 0.3
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                color: "white"
                font.bold: true
                font.pixelSize: win.unit * 0.28
                text: "Glisse un contrôle pour le placer ; touche-le puis − / + pour sa taille"
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: win.unit * 0.15
                EditButton { label: "−"; onClicked: if (content.selected !== "") content.setScaleOf(content.selected, content.scaleOf(content.selected) / 1.1) }
                EditButton { label: "+"; onClicked: if (content.selected !== "") content.setScaleOf(content.selected, content.scaleOf(content.selected) * 1.1) }
                EditButton { label: "Réinitialiser"; onClicked: { content.selected = ""; content.layout = ({}); content.commit() } }
                EditButton { label: "Terminer"; onClicked: { content.selected = ""; if (overlayStandalone) Qt.quit(); else { win.editMode = false; Qt.callLater(win.updateMask) } } }
            }
        }
    }
}
