#include "th18.h"
#include "general.h"
#include "skill.h"

TH18Package::TH18Package()
    : Package("th18")
{
    General *chimata = new General(this, "chimata$", "hld");
    chimata->addSkill(new Skill("simao"));
    chimata->addSkill(new Skill("liuneng"));
    chimata->addSkill(new Skill("shirong$"));

    General *mike = new General(this, "mike", "hld", 3);
    mike->addSkill(new Skill("cizhao"));
    mike->addSkill(new Skill("danran"));

    General *takane = new General(this, "takane", "hld");
    takane->addSkill(new Skill("yingji"));
    takane->addSkill(new Skill("zhixiao"));

    General *sannyo = new General(this, "sannyo", "hld");
    sannyo->addSkill(new Skill("boxi"));

    General *misumaru = new General(this, "misumaru", "hld");
    misumaru->addSkill(new Skill("zhuyu"));
    misumaru->addSkill(new Skill("shuzhu"));

    General *tsukasa = new General(this, "tsukasa", "hld", 3);
    tsukasa->addSkill(new Skill("tiaosuo"));
    tsukasa->addSkill(new Skill("zuanying"));

    General *megumu = new General(this, "megumu", "hld");
    megumu->addSkill(new Skill("fgwlshezheng"));
    megumu->addSkill(new Skill("miji"));

    General *momoyo = new General(this, "momoyo", "hld");
    momoyo->addSkill(new Skill("juezhu"));
    momoyo->addSkill(new Skill("zhanyi"));
}

ADD_PACKAGE(TH18)
