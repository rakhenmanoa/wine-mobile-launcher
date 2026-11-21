import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

// Icône d'un jeu : l'image du jeu si elle existe, sinon une pastille colorée avec
// l'initiale du nom (la couleur dépend du nom, donc chaque jeu garde la sienne).
// Un point vert s'affiche dans le coin quand le jeu tourne.
Item {
    id: root

    property string name: ""
    property string source: ""
    property bool running: false

    implicitWidth: Kirigami.Units.iconSizes.large
    implicitHeight: implicitWidth

    readonly property real hue: {
        let h = 0
        for (let i = 0; i < root.name.length; ++i)
            h = (h * 31 + root.name.charCodeAt(i)) % 360
        return h / 360
    }

    Kirigami.Icon {
        anchors.fill: parent
        visible: root.source !== ""
        source: root.source
    }

    Rectangle {
        anchors.fill: parent
        visible: root.source === ""
        radius: width * 0.22
        color: Qt.hsla(root.hue, 0.45, 0.4, 1)

        QQC2.Label {
            anchors.centerIn: parent
            text: root.name.length > 0 ? root.name.charAt(0).toUpperCase() : "?"
            color: "white"
            font.bold: true
            font.pixelSize: parent.height * 0.5
        }
    }

    Rectangle {
        visible: root.running
        width: Math.max(Kirigami.Units.gridUnit * 0.8, root.width * 0.2)
        height: width
        radius: width / 2
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: Kirigami.Theme.positiveTextColor
        border.width: 2
        border.color: Kirigami.Theme.backgroundColor
    }
}
