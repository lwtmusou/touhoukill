#include "hegemonyGeneral.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"
//#include "th10.h"
//#include "th08.h"

class YonghengHegemony : public TriggerSkill
{
public:
    YonghengHegemony()
        : TriggerSkill("yongheng_hegemony")
    {
        events << EventPhaseChanging << CardsMoveOneTime;
        frequency = Compulsory;
    }

    static void adjustHandcardNum(ServerPlayer *player, QString reason)
    {
        int card_num = qMax(player->getHp(), 1);
        Room *room = player->getRoom();
        int hc_num = player->getHandcardNum();
        if (card_num != hc_num) {
            room->touhouLogmessage("#TriggerSkill", player, reason);
            room->notifySkillInvoked(player, reason);
        }
        if (card_num > hc_num)
            player->drawCards(card_num - hc_num, reason);
        else if (card_num < hc_num)
            room->askForDiscard(player, reason, hc_num - card_num, hc_num - card_num);
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && change.player->hasSkill(objectName()))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QList<ServerPlayer *> kaguyas;
            ServerPlayer *kaguya1 = qobject_cast<ServerPlayer *>(move.from);
            ServerPlayer *kaguya2 = qobject_cast<ServerPlayer *>(move.to);

            if (kaguya1 && kaguya1->isAlive() && kaguya1->hasSkill(this) && move.from_places.contains(Player::PlaceHand) && kaguya1->getHandcardNum() != qMax(kaguya1->getHp(), 1)
                && kaguya1->getPhase() == Player::NotActive)
                kaguyas << kaguya1;
            if (kaguya2 && kaguya2->isAlive() && kaguya2->hasSkill(this) && move.to_place == Player::PlaceHand && kaguya2->getHandcardNum() != qMax(kaguya2->getHp(), 1)
                && kaguya2->getPhase() == Player::NotActive)
                kaguyas << kaguya2;
            if (kaguyas.length() > 1)
                std::sort(kaguyas.begin(), kaguyas.end(), ServerPlayer::CompareByActionOrder);
            if (!kaguyas.isEmpty()) {
                QList<SkillInvokeDetail> d;
                foreach (ServerPlayer *p, kaguyas)
                    d << SkillInvokeDetail(this, p, p, NULL, true);
                return d;
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard) {
                room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
                room->notifySkillInvoked(invoke->invoker, objectName());
                invoke->invoker->skip(change.to);
                adjustHandcardNum(invoke->invoker, objectName());
            }
        } else
            adjustHandcardNum(invoke->invoker, objectName());
        return false;
    }
};



HegemonyGeneralPackage::HegemonyGeneralPackage()
    : Package("hegemonyGeneral")
{




    General *reimu_hegemony = new General(this, "reimu_hegemony", "zhu", 4);
    reimu_hegemony->addSkill("qixiang");
    reimu_hegemony->addSkill("fengmo");
    reimu_hegemony->addCompanion("marisa_hegemony");
    reimu_hegemony->addCompanion("yukari_hegemony");
    reimu_hegemony->addCompanion("aya_hegemony");

    General *marisa_hegemony = new General(this, "marisa_hegemony", "zhu", 4);
    marisa_hegemony->addSkill("mofa");
    marisa_hegemony->addCompanion("patchouli_hegemony");
    marisa_hegemony->addCompanion("alice_hegemony");
    marisa_hegemony->addCompanion("nitori_hegemony");
    
    
    

//Spring
    General *byakuren_hegemony = new General(this, "byakuren_hegemony", "wu", 4);
    byakuren_hegemony->addSkill("pudu");
    byakuren_hegemony->addSkill("jiushu");
    byakuren_hegemony->addCompanion("toramaru_hegemony");
    byakuren_hegemony->addCompanion("murasa_hegemony");
    byakuren_hegemony->addCompanion("ichirin_hegemony");

    General *nue_hegemony = new General(this, "nue_hegemony", "wu", 3);
    nue_hegemony->addSkill("weizhi");
    nue_hegemony->addSkill("weizhuang");
    nue_hegemony->addCompanion("mamizou_hegemony");

    General *toramaru_hegemony = new General(this, "toramaru_hegemony", "wu", 4);
    toramaru_hegemony->addSkill("jinghua");
    toramaru_hegemony->addSkill("weiguang");
    toramaru_hegemony->addCompanion("nazrin_hegemony");

    General *murasa_hegemony = new General(this, "murasa_hegemony", "wu", 4);
    murasa_hegemony->addSkill("shuinan");
    murasa_hegemony->addSkill("nihuo");

    General *ichirin_hegemony = new General(this, "ichirin_hegemony", "wu", 4);
    ichirin_hegemony->addSkill("lizhi");
    ichirin_hegemony->addSkill("yunshang");

    General *nazrin_hegemony = new General(this, "nazrin_hegemony", "wu", 3);
    nazrin_hegemony->addSkill("xunbao");
    nazrin_hegemony->addSkill("lingbai");

    General *miko_hegemony = new General(this, "miko_hegemony", "wu", 4);
    miko_hegemony->addSkill("shengge");
    miko_hegemony->addSkill("qingting");
    miko_hegemony->addCompanion("futo_hegemony");
    miko_hegemony->addCompanion("toziko_hegemony");
    miko_hegemony->addCompanion("seiga_hegemony");

    General *mamizou_hegemony = new General(this, "mamizou_hegemony", "wu", 4);
    mamizou_hegemony->addSkill("xihua");
    mamizou_hegemony->addSkill("#xihua_clear");

    General *futo_hegemony = new General(this, "futo_hegemony", "wu", 3);
    futo_hegemony->addSkill("shijie");
    futo_hegemony->addSkill("fengshui");
    futo_hegemony->addCompanion("toziko_hegemony");

    General *toziko_hegemony = new General(this, "toziko_hegemony", "wu", 4);
    toziko_hegemony->addSkill("leishi");
    toziko_hegemony->addSkill("fenyuan");

    General *seiga_hegemony = new General(this, "seiga_hegemony", "wu", 3);
    seiga_hegemony->addSkill("xiefa");
    seiga_hegemony->addSkill("chuanbi");
    seiga_hegemony->addCompanion("yoshika_hegemony");

    General *yoshika_hegemony = new General(this, "yoshika_hegemony", "wu", 4);
    yoshika_hegemony->addSkill("duzhua");
    yoshika_hegemony->addSkill("#duzhuaTargetMod");
    yoshika_hegemony->addSkill("taotie");

    General *kyouko_hegemony = new General(this, "kyouko_hegemony", "wu", 3);
    kyouko_hegemony->addSkill("songjing");
    kyouko_hegemony->addSkill("gongzhen");



    General *kogasa_hegemony = new General(this, "kogasa_hegemony", "wu", 3);
    kogasa_hegemony->addSkill("yiwang");
    kogasa_hegemony->addSkill("jingxia");

    General *kokoro_hegemony = new General(this, "kokoro_hegemony", "wu", 4);
    kokoro_hegemony->addSkill("nengwu");
    kokoro_hegemony->addSkill("#nengwu2");
    kokoro_hegemony->addCompanion("miko_hegemony");


//Summer 

    General *remilia_hegemony = new General(this, "remilia_hegemony", "shu", 3);
    remilia_hegemony->addSkill("skltkexue");
    remilia_hegemony->addSkill("mingyun");
    remilia_hegemony->addCompanion("flandre_hegemony");
    remilia_hegemony->addCompanion("sakuya_hegemony");
    remilia_hegemony->addCompanion("patchouli_hegemony");

    General *flandre_hegemony = new General(this, "flandre_hegemony", "shu", 3);
    flandre_hegemony->addSkill("pohuai");
    flandre_hegemony->addSkill("yuxue");
    flandre_hegemony->addSkill("#yuxue-slash-ndl");
    flandre_hegemony->addSkill("shengyan");
    flandre_hegemony->addCompanion("meirin_hegemony");


    General *sakuya_hegemony = new General(this, "sakuya_hegemony", "shu", 4);
    sakuya_hegemony->addSkill("suoding");
    sakuya_hegemony->addSkill("huisu");
    sakuya_hegemony->addCompanion("meirin_hegemony");

    General *patchouli_hegemony = new General(this, "patchouli_hegemony", "shu", 3);
    patchouli_hegemony->addSkill("bolan");
    patchouli_hegemony->addSkill("hezhou");
    patchouli_hegemony->addCompanion("koakuma_hegemony");

    General *meirin_hegemony = new General(this, "meirin_hegemony", "shu", 4);
    meirin_hegemony->addSkill("taiji");
    meirin_hegemony->addSkill("beishui");

    General *koakuma_hegemony = new General(this, "koakuma_hegemony", "shu", 3);
    koakuma_hegemony->addSkill("moqi");
    koakuma_hegemony->addSkill("sishu");

    General *kaguya_hegemony = new General(this, "kaguya_hegemony", "shu", 4);
    kaguya_hegemony->addSkill(new YonghengHegemony);
    kaguya_hegemony->addCompanion("eirin_hegemony");
    kaguya_hegemony->addCompanion("mokou_hegemony");

    General *eirin_hegemony = new General(this, "eirin_hegemony", "shu", 4);
    eirin_hegemony->addSkill("ruizhi");
    eirin_hegemony->addSkill("miyao");

    General *mokou_hegemony = new General(this, "mokou_hegemony", "shu", 4);
    mokou_hegemony->addSkill("kaifeng");
    mokou_hegemony->addSkill("fengxiang");
    mokou_hegemony->addCompanion("keine_hegemony");
    mokou_hegemony->addCompanion("keine_sp_hegemony");

    General *reisen_hegemony = new General(this, "reisen_hegemony", "shu", 4);
    reisen_hegemony->addSkill("kuangzao");
    reisen_hegemony->addSkill("huanshi");
    reisen_hegemony->addCompanion("tewi_hegemony");


    General *keine_hegemony = new General(this, "keine_hegemony", "shu", 3);
    keine_hegemony->addSkill("xushi");
    keine_hegemony->addSkill("xinyue");
    keine_hegemony->addCompanion("keine_sp_hegemony");

    General *tewi_hegemony = new General(this, "tewi_hegemony", "shu", 3);
    tewi_hegemony->addSkill("buxian");
    tewi_hegemony->addSkill("#buxian");
    tewi_hegemony->addSkill("xingyun");

    General *keine_sp_hegemony = new General(this, "keine_sp_hegemony", "shu", 3);
    keine_sp_hegemony->addSkill("chuangshi");
    keine_sp_hegemony->addSkill("wangyue");

    General *toyohime_hegemony = new General(this, "toyohime_hegemony", "shu", 3);
    toyohime_hegemony->addSkill("lianxi");
    toyohime_hegemony->addSkill("yueshi");
    toyohime_hegemony->addCompanion("yorihime_hegemony");

    General *yorihime_hegemony = new General(this, "yorihime_hegemony", "shu", 4);
    yorihime_hegemony->addSkill("pingyi");
    yorihime_hegemony->addSkill("#pingyi_handle");



//Autumn
    General *kanako_hegemony = new General(this, "kanako_hegemony", "qun", 4);
    kanako_hegemony->addSkill("shende");
    kanako_hegemony->addSkill("qiankun");
    kanako_hegemony->addCompanion("suwako_hegemony");
    kanako_hegemony->addCompanion("sanae_hegemony");

    General *suwako_hegemony = new General(this, "suwako_hegemony", "qun", 3);
    suwako_hegemony->addSkill("bushu");
    suwako_hegemony->addSkill("qiankun");
    suwako_hegemony->addSkill("chuancheng");
    suwako_hegemony->addCompanion("sanae_hegemony");

    General *sanae_hegemony = new General(this, "sanae_hegemony", "qun", 3);
    sanae_hegemony->addSkill("dfgzmjiyi");
    sanae_hegemony->addSkill("qiji");

    General *aya_hegemony = new General(this, "aya_hegemony", "qun", 3);
    aya_hegemony->addSkill("fengshen");
    aya_hegemony->addSkill("fengsu");
    aya_hegemony->addSkill("#fengsu-effect");
    aya_hegemony->addCompanion("momizi_hegemony");

    General *nitori_hegemony = new General(this, "nitori_hegemony", "qun", 3);
    nitori_hegemony->addSkill("xinshang");
    nitori_hegemony->addSkill("#xinshang_effect");
    nitori_hegemony->addSkill("micai");

    General *hina_hegemony = new General(this, "hina_hegemony", "qun", 3);
    hina_hegemony->addSkill("jie");
    hina_hegemony->addSkill("liuxing");

    General *momizi_hegemony = new General(this, "momizi_hegemony", "qun", 4);
    momizi_hegemony->addSkill("shouhu");
    momizi_hegemony->addSkill("shaojie");

    General *minoriko_hegemony = new General(this, "minoriko_hegemony", "qun", 4);
    minoriko_hegemony->addSkill("fengrang");
    minoriko_hegemony->addSkill("shouhuo");
    minoriko_hegemony->addCompanion("shizuha_hegemony");

    General *shizuha_hegemony = new General(this, "shizuha_hegemony", "qun", 4);
    shizuha_hegemony->addSkill("jiliao");
    shizuha_hegemony->addSkill("zhongyan");

    General *satori_hegemony = new General(this, "satori_hegemony", "qun", 3);
    satori_hegemony->addSkill("xiangqi");
    satori_hegemony->addSkill("duxin");
    satori_hegemony->addCompanion("koishi_hegemony");

    General *koishi_hegemony = new General(this, "koishi_hegemony", "qun", 3);
    koishi_hegemony->addSkill("maihuo");
    koishi_hegemony->addSkill("wunian");


    General *utsuho_hegemony = new General(this, "utsuho_hegemony", "qun", 4);
    utsuho_hegemony->addSkill("yaoban");
    utsuho_hegemony->addSkill("here");
    utsuho_hegemony->addCompanion("rin_hegemony");

    General *rin_hegemony = new General(this, "rin_hegemony", "qun", 4);
    rin_hegemony->addSkill("yuanling");
    rin_hegemony->addSkill("songzang");

    General *yugi_hegemony = new General(this, "yugi_hegemony", "qun", 4);
    yugi_hegemony->addSkill("guaili");
    yugi_hegemony->addSkill("jiuhao");
    yugi_hegemony->addCompanion("parsee_hegemony");

    General *parsee_hegemony = new General(this, "parsee_hegemony", "qun", 3);
    parsee_hegemony->addSkill("jidu");
    parsee_hegemony->addSkill("gelong");

//Winter
    General *yuyuko_hegemony = new General(this, "yuyuko_hegemony", "wei", 4, false);
    yuyuko_hegemony->addSkill("sidie");
    yuyuko_hegemony->addSkill("huaxu");
    yuyuko_hegemony->addCompanion("yukari_hegemony");
    yuyuko_hegemony->addCompanion("youmu_hegemony");

    General *yukari_hegemony = new General(this, "yukari_hegemony", "wei", 4, false);
    yukari_hegemony->addSkill("shenyin");
    yukari_hegemony->addSkill("xijian");
    yukari_hegemony->addCompanion("ran_hegemony");

    General *ran_hegemony = new General(this, "ran_hegemony", "wei", 3, false);
    ran_hegemony->addSkill("shihui");
    ran_hegemony->addSkill("huanzang");
    ran_hegemony->addSkill("#huanzang");
    ran_hegemony->addCompanion("chen_hegemony");


    General *youmu_hegemony = new General(this, "youmu_hegemony", "wei", 4, false);
    youmu_hegemony->addSkill("shuangren");
    youmu_hegemony->addSkill("zhanwang");

    General *lunasa = new General(this, "lunasa", "wei", 3, false);
    lunasa->addCompanion("merlin");
    lunasa->addCompanion("lyrica");
    General *merlin = new General(this, "merlin", "wei", 3, false);
    merlin->addCompanion("lyrica");
    General *lyrica = new General(this, "lyrica", "wei", 3, false);
    

        

    General *alice_hegemony = new General(this, "alice_hegemony", "wei", 4, false);
    alice_hegemony->addSkill("zhanzhen");
    alice_hegemony->addSkill("renou");
    alice_hegemony->addCompanion("shanghai_hegemony");

    General *chen_hegemony = new General(this, "chen_hegemony", "wei", 3, false);
    chen_hegemony->addSkill("qimen");
    chen_hegemony->addSkill("dunjia");
    chen_hegemony->addSkill("#qimen-dist");
    chen_hegemony->addSkill("#qimen-prohibit");

    General *letty_hegemony = new General(this, "letty_hegemony", "wei", 4);
    letty_hegemony->addSkill("jiyi");
    letty_hegemony->addSkill("chunmian");

    General *lilywhite_hegemony = new General(this, "lilywhite_hegemony", "wei", 3);
    lilywhite_hegemony->addSkill("baochun");
    lilywhite_hegemony->addSkill("chunyi");

    General *shanghai_hegemony = new General(this, "shanghai_hegemony", "wei", 3);
    shanghai_hegemony->addSkill("zhancao");
    shanghai_hegemony->addSkill("mocao");

    General *youki_hegemony = new General(this, "youki_hegemony", "wei", 4, true);
    youki_hegemony->addSkill("shoushu");
    youki_hegemony->addSkill("yujian");
    youki_hegemony->addCompanion("youmu_hegemony");

    General *cirno_hegemony = new General(this, "cirno_hegemony", "wei", 3);
    cirno_hegemony->addSkill("dongjie");
    cirno_hegemony->addSkill("bingpo");
    cirno_hegemony->addCompanion("daiyousei_hegemony");

    General *daiyousei_hegemony = new General(this, "daiyousei_hegemony", "wei", 3);
    daiyousei_hegemony->addSkill("juxian");
    daiyousei_hegemony->addSkill("banyue");


    //skills << new MingmuVS << new YemangRange << new MingmuRange;
}

ADD_PACKAGE(HegemonyGeneral)
