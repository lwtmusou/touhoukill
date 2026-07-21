import QtQuick 6.5

import rocks.touhousatsu 1.0

// Skill button for the self player's skill dock. Multi-state image button driven by
// skillType (proactive/frequent/compulsory/awaken/oneoff/array/attachedlord) and
// buttonWidth (1=wide, 2=medium, 3=narrow, mirrors old SkillButtonWidth via image
// filename prefix 1/2/3). Images at image/system/button/skill/<type>/<w>-<state>.png.
//
// Toggle-style skills (proactive/oneoff/array/attachedlord) flip checked on click and
// emit skillActivated/skillDeactivated (mirrors old setSkill TOGGLE + emit flags).
// Non-toggle skills (compulsory/awaken/frequent) only display; clicking has no effect.
// Full pending/mutex/chooseSkillButton logic is a later subtask (see plan.md).
Item {
    id: root

    // 1 = wide (1 button per row), 2 = medium, 3 = narrow (3 per row).
    property int buttonWidth: 1
    property bool checked: false
    property string description: ""
    property string skillName
    property string skillType
    // Only TOGGLE + emit skills respond to clicks (mirror old setSkill).
    readonly property bool toggleable: skillType === "proactive" || skillType === "oneoff" || skillType === "array" || skillType === "attachedlord"
    property string translatedName

    signal skillActivated
    signal skillDeactivated

    function stateImage(state: string): url {
        return G.getAssetUrl("image/system/button/skill/" + skillType + "/" + buttonWidth + "-" + state + ".png");
    }

    implicitHeight: bg.implicitHeight
    implicitWidth: bg.implicitWidth

    Image {
        id: bg

        source: {
            if (root.toggleable && root.checked)
                return root.stateImage("down");
            if (mouseArea.containsMouse)
                return root.stateImage("hover");
            return root.stateImage("normal");
        }
    }

    Text {
        anchors.centerIn: bg
        color: "white"
        font.family: G.SkillButtonFontFace
        style: Text.Outline
        styleColor: "black"
        // Narrow/medium buttons show first 2 chars of the translated skill name
        // (mirror old _repaint: skillName.left(2) when width != WIDE).
        text: root.buttonWidth === 1 ? root.translatedName : root.translatedName.substring(0, 2)
    }

    MouseArea {
        id: mouseArea

        anchors.fill: bg
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onClicked: {
            if (root.toggleable) {
                root.checked = !root.checked;
                if (root.checked)
                    root.skillActivated();
                else
                    root.skillDeactivated();
            }
        }
    }
}
