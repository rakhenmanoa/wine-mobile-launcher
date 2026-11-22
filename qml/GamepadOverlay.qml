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
    readonly property bool editMode: overlayEdit
    readonly property real userScale: content.layout.scale > 0 ? content.layout.scale : 1
    readonly property real unit: Math.max(48, Math.min(content.width, content.height) / 7) * userScale
    readonly property var controlList: [dpad, leftStick, rightStick, face, zl, l, zr, r, selectBtn, homeBtn, startBtn]

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
            if (!c.touchZone || !c.visible)
                continue
            const r = c.mapRectToItem(null, 0, 0, c.width, c.height)
            rects.push({x: r.x, y: r.y, w: r.width, h: r.height})
        }
        pad.setInputMask(win, rects)
    }
    onWidthChanged: Qt.callLater(updateMask)
    onHeightChanged: Qt.callLater(updateMask)

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

        // Positions personnalisées : { scale, <clé>: {x, y} } (centre du contrôle, 0..1)
        property var layout: ({})
        function posX(key, w, def) {
            const p = layout[key]
            return p ? p.x * width - w / 2 : def
        }
        function posY(key, h, def) {
            const p = layout[key]
            return p ? p.y * height - h / 2 : def
        }
        function setCenter(key, cx, cy) {
            const l = Object.assign({}, layout)
            l[key] = { x: Math.max(0, Math.min(1, cx)), y: Math.max(0, Math.min(1, cy)) }
            layout = l
        }
        function setScale(v) {
            const l = Object.assign({}, layout)
            l.scale = Math.max(0.6, Math.min(1.6, v))
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
            property bool touchZone: true
            property bool pressed: false
            width: win.unit * 0.9
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
            width: win.unit * 1.5
            height: win.unit * 0.7
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
            width: win.unit * 0.95
            height: win.unit * 0.5
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
            width: win.unit * 2.4
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
            width: win.unit * 2.6
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

        // Boutons ABXY (disposition Nintendo : A à droite, B en bas, X en haut, Y à gauche ; codes Xbox : A = SOUTH, B = EAST)
        Item {
            id: face
            width: win.unit * 2.9
            height: width
            readonly property string key: "face"
            property bool touchZone: true
            x: content.posX(key, width, content.width - width - win.unit * 0.4)
            y: content.posY(key, height, rightStick.y - height - win.unit * 0.2)
            readonly property real d: win.unit * 0.95
            PadButton { label: "X"; code: win.btnNorth; x: (face.width - width) / 2; y: 0 }
            PadButton { label: "B"; code: win.btnEast; x: (face.width - width) / 2; y: face.height - height }
            PadButton { label: "Y"; code: win.btnWest; x: 0; y: (face.height - height) / 2 }
            PadButton { label: "A"; code: win.btnSouth; x: face.width - width; y: (face.height - height) / 2 }
        }

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
                onPressed: (points) => start(points[0])
                onUpdated: (points) => drag(points[0])
                onReleased: content.commit()
                onCanceled: content.commit()
                Rectangle {
                    anchors.fill: parent
                    color: "#223daee9"
                    border.color: "#ffffb300"
                    border.width: 3
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
            property bool touchZone: true
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
                text: "Glisse les contrôles pour les placer"
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: win.unit * 0.15
                EditButton { label: "−"; onClicked: content.setScale(win.userScale - 0.1) }
                EditButton { label: "+"; onClicked: content.setScale(win.userScale + 0.1) }
                EditButton { label: "Réinitialiser"; onClicked: { content.layout = ({}); content.commit() } }
                EditButton { label: "Terminer"; onClicked: Qt.quit() }
            }
        }
    }
}
