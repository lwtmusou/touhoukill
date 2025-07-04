#include "th20.h"

#include "general.h"

TH20Package::TH20Package()
    : Package("th20")
{
    General *ubame = new General(this, "ubame", "jsj");
    Q_UNUSED(ubame);

    General *chimi = new General(this, "chimi", "jsj");
    Q_UNUSED(chimi);

    General *nareko = new General(this, "nareko", "jsj");
    Q_UNUSED(nareko);
}

ADD_PACKAGE(TH20)
