
// final is not recognized by swig earlier than 4.1 so use macro define to remove it on that version.
// DO NOT USE final AS IDENTIFIER!!!

#if SWIG_VERSION < 0x040100
#define final
#endif

%include "global.h"
%rename(sameTrigger) TriggerDetail::operator ==(const TriggerDetail &arg2) const;
%rename(lessThan) TriggerDetail::operator <(const TriggerDetail &arg2) const;
%include "structs.h"
%include "engine.h"
%include "CardFace.h"
%include "card.h"
%include "general.h"
%include "package.h"
%include "player.h"
%include "skill.h"
%include "trigger.h"
%include "RoomObject.h"
%include "game-logic.h"
%include "mode.h"

#if SWIG_VERSION < 0x040100
#undef final
#endif
