import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property string symbol: "play"
    property color color: "#d8dee9"

    // Fixed vector geometry keeps icons sharp without a system icon theme.
    readonly property string glyphPath: {
        switch (symbol) {
        case "pause": return "M4 3 H10 V21 H4 Z M14 3 H20 V21 H14 Z"
        case "previous": return "M2 6 H5 V18 H2 Z M5 12 L13 6 V18 Z M13 12 L23 6 V18 Z"
        case "next": return "M19 6 H22 V18 H19 Z M19 12 L11 6 V18 Z M11 12 L1 6 V18 Z"
        case "volume":
        case "mute": return "M2 9 H6 L11 5 V19 L6 15 H2 Z"
        default: return "M6 3 L21 12 L6 21 Z"
        }
    }

    Shape {
        width: 24
        height: 24
        anchors.centerIn: parent
        preferredRendererType: Shape.CurveRenderer
        transform: Scale {
            origin.x: 12
            origin.y: 12
            xScale: root.width / 24
            yScale: root.height / 24
        }

        ShapePath {
            fillColor: root.symbol === "volume" || root.symbol === "mute" ? "transparent" : root.color
            strokeColor: root.color
            strokeWidth: root.symbol === "volume" || root.symbol === "mute" ? 2.2 : -1
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: root.glyphPath }
        }

        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: 2.2
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg {
                path: root.symbol === "volume"
                      ? "M15 8 Q18 12 15 16 M18 4 Q24 12 18 20"
                      : root.symbol === "mute" ? "M16 9 L22 15 M22 9 L16 15" : ""
            }
        }
    }
}
