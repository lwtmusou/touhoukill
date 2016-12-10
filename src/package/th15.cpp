#include "th15.h"
#include "general.h"

TH15Package::TH15Package()
    : Package("th15")
{
    General *junko = new General(this, "junko$", "gzz", 4, false);
    Q_UNUSED(junko);
    General *seiran = new General(this, "seiran", "gzz", 4, false);
    Q_UNUSED(seiran);
    General *ringo = new General(this, "ringo", "gzz", 4, false);
    Q_UNUSED(ringo);
    General *doremy = new General(this, "doremy", "gzz", 4, false);
    Q_UNUSED(doremy);
    General *sagume = new General(this, "sagume", "gzz", 4, false);
    Q_UNUSED(sagume);
    General *clownpiece = new General(this, "clownpiece", "gzz", 4, false);
    Q_UNUSED(clownpiece);
    General *hecatia = new General(this, "hecatia", "gzz", 4, false);
    Q_UNUSED(hecatia);

}

ADD_PACKAGE(TH15)
