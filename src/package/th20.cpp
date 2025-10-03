#include "th20.h"

#include "general.h"

TH20Package::TH20Package()
    : Package("th20")
{
    General *ariya = new General(this, "ariya$", "jsj");
    Q_UNUSED(ariya);

    General *ubame = new General(this, "ubame", "jsj");
    Q_UNUSED(ubame);

    General *chimi = new General(this, "chimi", "jsj");
    Q_UNUSED(chimi);

    General *nareko = new General(this, "nareko", "jsj");
    Q_UNUSED(nareko);

    General *asama = new General(this, "asama", "jsj");
    Q_UNUSED(asama);

    General *nina = new General(this, "nina", "jsj");
    Q_UNUSED(nina);
}

ADD_PACKAGE(TH20)
