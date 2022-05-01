
#define QSGS_CORE_NODEPRECATED
#define Q_DECL_DEPRECATED
#define QSGS_CORE_EXPORT
#define Q_DECLARE_METATYPE(...)
#define Q_GADGET
#define Q_OBJECT
#define Q_DECLARE_FLAGS(Flags, Enum) \
    typedef QFlags<Enum> Flags;
#define Q_DECLARE_OPERATORS_FOR_FLAGS(...)
#define Q_ENUM(...)
#define Q_ALWAYS_INLINE
#define final

%include "global.h"
%rename(sameTrigger) TriggerDetail::operator ==(const TriggerDetail &arg2) const;
%rename(lessThan) TriggerDetail::operator <(const TriggerDetail &arg2) const;
%include "structs.h"
%include "engine.h"
%include "CardFace.h"
%include "card.h"
%include "general.h"
%rename(insertGeneral) Package::operator <<(const General *);
%rename(insertCard) Package::operator <<(const CardDescriptor &);
%include "package.h"
%include "player.h"
%include "skill.h"
%include "trigger.h"
%include "RoomObject.h"

#undef final
