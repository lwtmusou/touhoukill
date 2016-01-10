#include "th01-05.h"
#include "general.h"

TH0105Package::TH0105Package()
    : Package("th0105")
{
    General *shinki = new General(this, "shinki", "pc98", 4, false);
    Q_UNUSED(shinki);
    General *alice_old = new General(this, "alice_old", "pc98", 4, false);
    Q_UNUSED(alice_old);
    General *yumemi = new General(this, "yumemi", "pc98", 4, false);
    Q_UNUSED(yumemi);
    General *chiyuri = new General(this, "chiyuri", "pc98", 4, false);
    Q_UNUSED(chiyuri);
    General *rikako = new General(this, "rikako", "pc98", 4, false);
    Q_UNUSED(rikako);
    General *kana = new General(this, "kana", "pc98", 4, false);
    Q_UNUSED(kana);
    General *mima = new General(this, "mima", "pc98", 4, false);
    Q_UNUSED(mima);
    General *gengetsumugetsu = new General(this, "gengetsumugetsu", "pc98", 4, false);
    Q_UNUSED(gengetsumugetsu);
    General *elly = new General(this, "elly", "pc98", 4, false);
    Q_UNUSED(elly);
}

ADD_PACKAGE(TH0105)
