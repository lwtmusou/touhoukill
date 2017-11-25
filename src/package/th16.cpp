#include "th16.h"
#include "general.h"

TH16Package::TH16Package()
    : Package("th16")
{
    General *okina = new General(this, "okina$", "kong");
    Q_UNUSED(okina);

    General *etanity = new General(this, "etanity", "kong");
    Q_UNUSED(etanity);

    General *nemuno = new General(this, "nemuno", "kong");
    Q_UNUSED(nemuno);

    General *aun = new General(this, "aun", "kong");
    Q_UNUSED(aun);

    General *narumi = new General(this, "narumi", "kong");
    Q_UNUSED(narumi);

    General *satono = new General(this, "satono", "kong");
    Q_UNUSED(satono);

    General *mai = new General(this, "mai", "kong");
    Q_UNUSED(mai);
}

ADD_PACKAGE(TH16)
