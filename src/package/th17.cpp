#include "th17.h"
#include "general.h"

TH17Package::TH17Package()
    : Package("th17")
{
    General *saki = new General(this, "saki$", "gui");
    Q_UNUSED(saki);

    General *eika = new General(this, "eika", "gui");
    Q_UNUSED(eika);

    General *urumi = new General(this, "urumi", "gui");
    Q_UNUSED(urumi);

    General *kutaka = new General(this, "kutaka", "gui");
    Q_UNUSED(kutaka);

    General *yachie = new General(this, "yachie", "gui");
    Q_UNUSED(yachie);

    General *mayumi = new General(this, "mayumi", "gui");
    Q_UNUSED(mayumi);

    General *keiki = new General(this, "keiki", "gui");
    Q_UNUSED(keiki);
}

ADD_PACKAGE(TH17)
