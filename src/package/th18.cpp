#include "th18.h"
#include "general.h"

TH18Package::TH18Package()
    : Package("th18")
{
    General *momoyo = new General(this, "momoyo$", "hld");
    Q_UNUSED(momoyo);

    General *mike = new General(this, "mike", "hld");
    Q_UNUSED(mike);

    General *takane = new General(this, "takane", "hld");
    Q_UNUSED(takane);

    General *sannyo = new General(this, "sannyo", "hld");
    Q_UNUSED(sannyo);

    General *misumaru = new General(this, "misumaru", "hld");
    Q_UNUSED(misumaru);

    General *tsukasa = new General(this, "tsukasa", "hld");
    Q_UNUSED(tsukasa);

    General *megumu = new General(this, "megumu", "hld");
    Q_UNUSED(megumu);

    General *chimata = new General(this, "chimata", "hld");
    Q_UNUSED(chimata);
}

ADD_PACKAGE(TH18)
