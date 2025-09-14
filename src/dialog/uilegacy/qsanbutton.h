#ifndef THKILL_QSAN_BUTTON_H
#define THKILL_QSAN_BUTTON_H

#include <QString>

namespace QSanButton {
enum ButtonState
{
    S_STATE_UP,
    S_STATE_HOVER,
    S_STATE_DOWN,
    S_STATE_CANPRESHOW,
    S_STATE_DISABLED,
    S_NUM_BUTTON_STATES
};
enum ButtonStyle
{
    S_STYLE_PUSH,
    S_STYLE_TOGGLE
};
} // namespace QSanButton

namespace QSanSkillButton {
enum SkillType
{
    S_SKILL_ATTACHEDLORD,
    S_SKILL_PROACTIVE,
    S_SKILL_FREQUENT,
    S_SKILL_COMPULSORY,
    S_SKILL_AWAKEN,
    S_SKILL_ONEOFF_SPELL,
    S_SKILL_ARRAY,
    S_NUM_SKILL_TYPES
};

inline QString getSkillTypeString(SkillType type)
{
    QString arg1;
    if (type == QSanSkillButton::S_SKILL_AWAKEN)
        arg1 = "awaken";
    else if (type == QSanSkillButton::S_SKILL_ARRAY)
        arg1 = "array";
    else if (type == QSanSkillButton::S_SKILL_COMPULSORY)
        arg1 = "compulsory";
    else if (type == QSanSkillButton::S_SKILL_FREQUENT)
        arg1 = "frequent";
    else if (type == QSanSkillButton::S_SKILL_ONEOFF_SPELL)
        arg1 = "oneoff";
    else if (type == QSanSkillButton::S_SKILL_PROACTIVE)
        arg1 = "proactive";
    else if (type == QSanSkillButton::S_SKILL_ATTACHEDLORD)
        arg1 = "attachedlord";
    return arg1;
}

} // namespace QSanSkillButton

namespace QSanInvokeSkillButton {
enum SkillButtonWidth
{
    S_WIDTH_WIDE,
    S_WIDTH_MED,
    S_WIDTH_NARROW,
    S_NUM_BUTTON_WIDTHS
};
}

#endif
