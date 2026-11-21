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
    readonly property real unit: Math.max(48, Math.min(content.width, content.height) / 7)

    flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus | Qt.WindowStaysOnTopHint
    color: "transparent"
    visible: true
    visibility: Window.FullScreen

    Component.onCompleted: pad.setupOverlay(win)

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

    Item {
        id: content

        anchors.centerIn: parent
        width: win.turned ? win.height : win.width
        height: win.turned ? win.width : win.height
        rotation: overlayRotation
        onWidthChanged: Qt.callLater(win.updateMask)
        onHeightChanged: Qt.callLater(win.updateMask)
        Component.onCompleted: Qt.callLater(win.updateMask)

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
            property bool touchZone: true
            property bool pressed: false
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
                anchors.fill: parent
                anchors.margins: -win.unit * 0.1
                maximumTouchPoints: 1
                onPressed: { sm.pressed = true; pad.button(sm.code, true) }
                onReleased: { sm.pressed = false; pad.button(sm.code, false) }
                onCanceled: { sm.pressed = false; pad.button(sm.code, false) }
            }
        }

        // Stick analogique
        component Stick: Rectangle {
            id: stick
            property int axisX
            property int axisY
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
            x: win.unit * 0.4
            y: content.height - height - win.unit * 0.4
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
        Shoulder { id: zl; label: "ZL"; axisCode: win.absZ; x: win.unit * 0.4; y: win.unit * 0.3 }
        Shoulder { id: l; label: "L"; code: win.btnTL; x: win.unit * 0.4; y: win.unit * 1.15 }
        Shoulder { id: zr; label: "ZR"; axisCode: win.absRZ; x: content.width - width - win.unit * 0.4; y: win.unit * 0.3 }
        Shoulder { id: r; label: "R"; code: win.btnTR; x: content.width - width - win.unit * 0.4; y: win.unit * 1.15 }

        // Sticks : gauche au-dessus de la croix, droit sous les boutons
        Stick { id: leftStick; axisX: win.absX; axisY: win.absY; x: win.unit * 0.5; y: dpad.y - height - win.unit * 0.3 }
        Stick { id: rightStick; axisX: win.absRX; axisY: win.absRY; x: content.width - width - win.unit * 0.5; y: content.height - height - win.unit * 0.4 }

        // Boutons ABXY (disposition Nintendo : A à droite, B en bas, X en haut, Y à gauche)
        Item {
            id: face
            width: win.unit * 2.9
            height: width
            x: content.width - width - win.unit * 0.4
            y: rightStick.y - height - win.unit * 0.2
            readonly property real d: win.unit * 0.95
            PadButton { label: "X"; code: win.btnNorth; x: (face.width - width) / 2; y: 0 }
            PadButton { label: "B"; code: win.btnSouth; x: (face.width - width) / 2; y: face.height - height }
            PadButton { label: "Y"; code: win.btnWest; x: 0; y: (face.height - height) / 2 }
            PadButton { label: "A"; code: win.btnEast; x: face.width - width; y: (face.height - height) / 2 }
        }

        // − Home +
        SmallButton { label: "−"; code: win.btnSelect; x: content.width / 2 - width * 1.6; y: content.height - height - win.unit * 0.3 }
        SmallButton { label: "⌂"; code: win.btnMode; x: content.width / 2 - width / 2; y: content.height - height - win.unit * 0.3 }
        SmallButton { label: "+"; code: win.btnStart; x: content.width / 2 + width * 0.6; y: content.height - height - win.unit * 0.3 }
    }
}
