#include "th18.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "skill.h"

#undef DESCRIPTION

#ifdef DESCRIPTION
["chimata"] = "天弓千亦",
    ["#chimata"] = "无主物之神", ["simao"] = "司贸",
    [":simao"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以令二至四名有牌的角色各弃置一张牌，然后以其中一名角色为起点，这些角色各获得这些弃置的牌中一张牌。",
    ["liuneng"] = "流能", [":liuneng"] = "你可以视为使用一张其他角色于当前回合内因使用、打出或弃置而失去的基本牌或普通锦囊牌，<font color=\"green\"><b>每回合限一次。</b></font>",
    ["shirong"] = "市荣",
    [":shirong"] = "<font color=\"orange\"><b>主公技，</b></font><font "
                   "color=\"green\"><b>其他龙势力角色的出牌阶段限一次，</b></font>若你手牌数小于你的手牌上限，其可以弃置一张牌，然后你摸一张牌。",
    ["shirong_attach"] = "市荣",
    [":shirong_attach"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>若你属于龙势力且拥有主公技“市荣”的角色的手牌数小于其手牌上限，你可以弃置一张牌，然后其摸一张牌。",
    ;
;
#endif

class Cizhao : public TriggerSkill
{
public:
    Cizhao()
        : TriggerSkill("cizhao")
    {
        events = {EventPhaseStart};
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        QList<SkillInvokeDetail> d;

        if (p->getPhase() == Player::Play && p->isAlive()) {
            foreach (ServerPlayer *mike, room->getAllPlayers()) {
                if (mike != p && mike->hasSkill(this) && !mike->isNude())
                    d << SkillInvokeDetail(this, mike, mike, p);
            }
        }

        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        const Card *c
            = room->askForExchange(invoke->invoker, objectName() + QStringLiteral("1"), 1, 1, true, objectName() + "-prompt1:" + invoke->targets.first()->objectName(), true);
        if (c != nullptr) {
            LogMessage l;
            l.type = "#ChoosePlayerWithSkill";
            l.from = invoke->invoker;
            l.to = invoke->targets;
            l.arg = objectName();
            room->sendLog(l);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());

            CardMoveReason m(CardMoveReason::S_REASON_GIVE, invoke->invoker->objectName(), objectName(), {});
            room->obtainCard(invoke->targets.first(), c, m, false);
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *t = invoke->targets.first();

        bool discarded = (!t->isNude()) && room->askForDiscard(t, objectName() + QStringLiteral("2"), 1, 1, true, true, objectName() + "-prompt2:" + invoke->invoker->objectName());

        LogMessage l;
        l.from = t;

        if (!discarded) {
            room->drawCards(t, 1, objectName());
            room->setPlayerFlag(t, objectName() + QStringLiteral("plus2"));
            l.type = "#cizhao-log1";
        } else {
            room->setPlayerFlag(t, objectName() + QStringLiteral("minus1"));
            l.type = "#cizhao-log2";
        }

        room->sendLog(l);

        return false;
    }
};

class CizhaoDistance : public DistanceSkill
{
public:
    CizhaoDistance(const QString &baseSkill)
        : DistanceSkill("#" + baseSkill + "-distance")
        , b(baseSkill)
    {
    }

    int getCorrect(const Player *from, const Player *) const override
    {
        if (from->hasFlag(b + QStringLiteral("plus2")))
            return 2;
        if (from->hasFlag(b + QStringLiteral("minus1")))
            return -1;

        return 0;
    }

private:
    QString b;
};

class DanranVS : public ViewAsSkill
{
public:
    DanranVS(const QString &objectName)
        : ViewAsSkill(objectName)
    {
        response_pattern = "jink";
        response_or_use = true;
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const override
    {
        return !to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() == Self->getHandcardNum()) {
            Card *j = Sanguosha->cloneCard("jink");
            j->addSubcards(cards);
            j->setSkillName(objectName());
            j->setShowSkill(objectName());
            return j;
        }

        return nullptr;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        if (player->isKongcheng())
            return false;

        Card::Suit s = Card::SuitToBeDecided;
        foreach (const Card *c, player->getHandcards()) {
            if (c->isRed()) {
                if (s == Card::SuitToBeDecided)
                    s = Card::NoSuitRed;
                else if (s != Card::NoSuitRed)
                    return false;
            } else if (c->isBlack()) {
                if (s == Card::SuitToBeDecided)
                    s = Card::NoSuitBlack;
                else if (s != Card::NoSuitBlack)
                    return false;
            } else {
                if (s == Card::SuitToBeDecided)
                    s = Card::NoSuit;
                else if (s != Card::NoSuit)
                    return false;
            }
        }

        return ViewAsSkill::isEnabledAtResponse(player, pattern);
    }
};

class Danran : public TriggerSkill
{
public:
    Danran()
        : TriggerSkill("danran")
    {
        view_as_skill = new DanranVS(objectName());
        events = {CardFinished, CardResponded};
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *from = nullptr;
        const Card *card = nullptr;

        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            from = use.from;
            card = use.card;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (!resp.m_isUse) {
                from = resp.m_from;
                card = resp.m_card;
            }
        }

        if (from != nullptr && card != nullptr && card->getSkillName() == objectName() && card->isKindOf("Jink"))
            return {SkillInvokeDetail(this, from, from, from, true, nullptr, false)};

        return {};
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ExNihilo *sheng = new ExNihilo(Card::NoSuit, -1);
        sheng->setSkillName("_" + objectName());
        CardUseStruct use(sheng, invoke->invoker, invoke->invoker, false);
        room->useCard(use);
        return false;
    }
};

#ifdef DESCRIPTION
["takane"] = "山城高岭", ["#takane"] = "深山的经商妖怪", ["yingji"] = "营计",
                         [":yingji"]
    = "一名角色的出牌阶段结束时，若其于此阶段内使用的最后一张牌是普通锦囊牌并在弃牌堆里，你可以将之置于你的人物牌上称为“货”；当一名角色于其出牌阶段内使用第一张牌时，若"
      "此牌是基本牌且你人物牌上有“货”，你可以获得此基本牌，然后其获得一张“货”。",
                         ["zhixiao"] = "滞销", [":zhixiao"] = "<font color=\"blue\"><b>锁定技，</b></font>摸牌阶段开始时，若“货”数量大于你的体力上限，你放弃摸牌并得所有“货”。",
                         ;
;
#endif
#ifdef DESCRIPTION
["sannyo"] = "驹草山如", ["#sannyo"] = "栖息于高地的山女郎", ["boxi"] = "博戏",
                         [":boxi"]
    = "<font "
      "color=\"green\"><b>出牌阶段限一次，</b></"
      "font>"
      "你可以顺时或逆时针令所有有牌的角色逐个展示一张牌，然后你可以再展示一张牌。均展示后，你弃置这些展示的牌中牌数唯一最多的花色牌，令展示牌数最少（可并列）的花色牌的角"
      "色各摸一张牌，最后你可以使用或获得一张因此次弃置而置入弃牌堆的牌。",
                         ;
;
#endif
#ifdef DESCRIPTION
["misumaru"] = "玉造魅须丸", ["#misumaru"] = "真正的勾玉匠人", ["zhuyu"] = "铸玉",
                             [":zhuyu"]
    = "摸牌阶段结束时，你可以展示牌堆底的三张牌，然后可以弃置一张牌并选择一项：依次使用其中与之花色相同的能使用的牌；或将其中与之花色不同的牌置入弃牌堆。选择后若余下的"
      "牌花色相同，你将之当【杀】使用或交给一名角色。",
                             ["shuzhu"] = "戍珠", [":shuzhu"] = "其他角色的弃牌阶段开始时，若其手牌数大于其手牌上限，你可以展示其一张手牌并将之置于牌堆底或弃置之。",
                             ;
;
#endif
#ifdef DESCRIPTION
["tsukasa"] = "菅牧典", ["#tsukasa"] = "耳边低语的邪恶白狐", ["tiaosuo"] = "挑唆",
                        [":tiaosuo"] = "其他角色的出牌阶段开始时，你可以将一张黑色牌交给其并横置或重置其和另一名角色，然后其于此阶段内：使用【杀】的次数+"
                                       "1且无距离限制；使用【杀】或【决斗】不能选择与其人物牌横竖放置状态不同的角色为目标。",
                        ["zuanying"] = "钻营",
                        [":zuanying"] = "结束阶段开始时，你可以令一名处于连环状态的其他角色摸一张牌，然后若其手牌数大于其手牌上限，你回复1点体力或获得其两张牌。",
                        ;
;
#endif
#ifdef DESCRIPTION
["megumu"] = "饭纲丸龙", ["#megumu"] = "鸦天狗的首领", ["fgwlshezheng"] = "涉政",
                         [":fgwlshezheng"]
    = "当你于出牌阶段内使用牌时，你可以将牌堆底的一张牌置入弃牌堆，若两张牌颜色：相同，你于此阶段内使用的本牌和下一张牌不计入使用次数限制；不同，你弃置一张牌。",
                         ["miji"] = "觅机", [":miji"] = "出牌阶段开始时或当你因弃置而失去基本牌后，你可以观看牌堆底的三张牌并获得其中一张当前回合未使用过的类别牌。",
                         ;
;
#endif
#ifdef DESCRIPTION
["momoyo"] = "姬虫百百世", ["#momoyo"] = "漆黑的噬龙者", ["juezhu"] = "掘珠",
                           [":juezhu"] = "<font "
                                         "color=\"green\"><b>出牌阶段限一次，</b></"
                                         "font>你可以选择所有手牌数不小于你的其他角色，令其各选择一项：1.令你摸一张牌；2.摸一张牌，然后你可以终止此流程并视为对其使用【决斗】。",
                           ["zhanyi"] = "战意", [":zhanyi"] = "你可以将X张牌（X为你的体力值与手牌数之差且至少为1）当【杀】使用或打出。",
                           ;
;
;
#endif

TH18Package::TH18Package()
    : Package("th18")
{
    General *chimata = new General(this, "chimata$", "hld");
    chimata->addSkill(new Skill("simao"));
    chimata->addSkill(new Skill("liuneng"));
    chimata->addSkill(new Skill("shirong$"));

    General *mike = new General(this, "mike", "hld", 3);
    mike->addSkill(new Cizhao);
    mike->addSkill(new CizhaoDistance("cizhao"));
    mike->addSkill(new Danran);
    related_skills.insertMulti("cizhao", "#cizhao-distance");

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
