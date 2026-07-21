import QtQuick 6.5

import rocks.touhousatsu 1.0

// Self skill button dock: single dock above selfPhoto (QML Dashboard dropped the
// old right-frame avatar area, so the dock sits above selfPhoto). Rebuilt from
// roomScene.getPlayerSkillButtons(roomScene.Self) on skill add/remove/acquire/
// invalidity notify and on skill-related game events (dispatched by RoomScene via
// handleSkillEvent). Pending/mutex/chooseSkillButton TODO (plan.md).
Item {
    id: root

    property var buttonWidthsArray: []
    property var roomScene
    property var skills: []

    // Per-button width (1=wide, 2=medium, 3=narrow) mirroring old dock.update() row
    // layout + tail balancing (qsanbutton.cpp:508-610). Rows of up to 3; a lone tail
    // button with a full 3-row above is balanced to 2+2; 3 buttons become 2+1.
    function buttonWidths(count) {
        if (count <= 0)
            return [];
        if (count === 1)
            return [1];
        if (count === 2)
            return [2, 2];
        if (count === 3)
            return [2, 2, 1];
        let arr = [];
        const r = count % 3;
        if (r === 0) {
            for (let i = 0; i < count; ++i)
                arr.push(3);
        } else if (r === 1) {
            const n = (count - 1) / 3;
            const threes = (n - 1) * 3;
            for (let i = 0; i < threes; ++i)
                arr.push(3);
            for (let i = 0; i < 4; ++i)
                arr.push(2);
        } else {
            const n = (count - 2) / 3;
            const threes = n * 3;
            for (let i = 0; i < threes; ++i)
                arr.push(3);
            for (let i = 0; i < 2; ++i)
                arr.push(2);
        }
        return arr;
    }

    // Dispatch skill-related game events (args = [eventType, ...]).
    // ADD/LOSE/ACQUIRE/DETACH/PREPARE/UPDATE SKILL -> rebuild the dock.
    // SKILL_INVOKED -> TODO: skill invoke countdown (mirror old TimedProgressBar).
    function handleSkillEvent(args) {
        if (!args || args.length < 1)
            return;
        const eventType = args[0];
        const T = QSanProtocol;
        if (eventType === T.S_GAME_EVENT_ADD_SKILL || eventType === T.S_GAME_EVENT_LOSE_SKILL || eventType === T.S_GAME_EVENT_ACQUIRE_SKILL || eventType
                === T.S_GAME_EVENT_DETACH_SKILL || eventType === T.S_GAME_EVENT_PREPARE_SKILL || eventType === T.S_GAME_EVENT_UPDATE_SKILL) {
            rebuild();
        } else if (eventType === T.S_GAME_EVENT_SKILL_INVOKED) {
            // TODO: skill invoke countdown.
        }
    }

    function rebuild() {
        skills = roomScene.getPlayerSkillButtons(roomScene.Self);
        buttonWidthsArray = buttonWidths(skills.length);
    }

    implicitHeight: skillFlow.implicitHeight

    Component.onCompleted: rebuild()

    // Skill-specific notifies (NOT global game events; game events arrive via
    // RoomScene's onNotifyEventReceived -> handleSkillEvent).
    Connections {
        function onNotifySkillAcquired() {
            root.rebuild();
        }

        function onNotifySkillAttached() {
            root.rebuild();
        }

        function onNotifySkillDetached() {
            root.rebuild();
        }

        function onNotifySkillInvalidityChanged() {
            root.rebuild();
        }

        target: roomScene
    }

    Flow {
        id: skillFlow

        anchors.fill: parent
        spacing: 2

        Repeater {
            model: root.skills

            QSanSkillButton {
                buttonWidth: root.buttonWidthsArray[index] || 3
                description: modelData.description
                skillName: modelData.skillName
                skillType: modelData.skillType
                translatedName: modelData.translatedName

                onSkillActivated: {
                    // TODO: startPending(viewAsSkill) + mutex + chooseSkillButton (plan.md).
                    console.log("[qml] skill activated", modelData.skillName);
                }
                onSkillDeactivated: {
                    // TODO: stopPending.
                    console.log("[qml] skill deactivated", modelData.skillName);
                }
            }
        }
    }
}
