
// final is not recognized by swig so use macro define to remove it.
// DO NOT USE final AS IDENTIFIER!!!

#define final

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

#undef final
