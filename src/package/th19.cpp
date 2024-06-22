#include "th19.h"
#include "general.h"

TH19Package::TH19Package()
    : Package("th19")
{
    General *biten = new General(this, "biten", "swy");
    Q_UNUSED(biten);

    General *enoko = new General(this, "enoko", "swy");
    Q_UNUSED(enoko);

    General *chiyari = new General(this, "chiyari", "swy");
    Q_UNUSED(chiyari);

    General *hisami = new General(this, "hisami", "swy");
    Q_UNUSED(hisami);

    General *zanmu = new General(this, "zanmu", "swy");
    Q_UNUSED(zanmu);
}

ADD_PACKAGE(TH19)
