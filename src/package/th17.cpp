#include "th17.h"
#include "general.h"

TH17Package::TH17Package()
    : Package("th17")
{
    General *saki = new General(this, "saki$", "gxs");
    Q_UNUSED(saki);

    General *eika = new General(this, "eika", "gxs");
    Q_UNUSED(eika);

    General *urumi = new General(this, "urumi", "gxs");
    Q_UNUSED(urumi);

    General *kutaka = new General(this, "kutaka", "gxs");
    Q_UNUSED(kutaka);

    General *yachie = new General(this, "yachie", "gxs");
    Q_UNUSED(yachie);

    General *mayumi = new General(this, "mayumi", "gxs");
    Q_UNUSED(mayumi);

    General *keiki = new General(this, "keiki", "gxs");
    Q_UNUSED(keiki);
}

ADD_PACKAGE(TH17)
