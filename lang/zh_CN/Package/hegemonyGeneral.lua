return
{
	["hegemonyGeneral"] = "国战标准包",
	--奖励规则部分
	["@HalfLife"] = "半血奖励",
	["HalfLife"] = "半血奖励",
	["halflife_attach"] = "半血奖励",
	[":halflife_attach"] = "出牌阶段，你可以结算半血奖励，摸一张牌。 弃牌阶段开始时，你可以结算半血奖励，令本回合你的手牌上限+2",
	["#HalfLife"] = "%from 的武将牌上有单独的阴阳鱼，本回合手牌上限+2",
	
	["@CompanionEffect"] = "珠联璧合奖励",
    ["#DoCompanionEffect"] = "%from 结算珠联璧合奖励",
	["companion_attach"] =  "珠联璧合奖励",
	["@companion_attach"] = "出牌阶段，你可以结算珠联璧合奖励，摸两张牌或回复1点体力。一名角色向你求【桃】时，你可以结算珠联璧合奖励，令其回复1点体力。",
	
	["@Pioneer"] = "先驱",
	["pioneer_attach"] =  "首亮奖励",
	[":pioneer_attach"] =  "出牌阶段，你可执行首亮奖励，将手牌补至四张，并观看一名角色的一张暗置武将牌。",
	["@pioneer_attach"] =  "你执行首亮奖励，观看一名角色的一张暗置武将牌",
	
	["reimu_hegemony"] = "博丽灵梦",
	["tuizhi_hegemony"] = "退治",
	[":tuizhi_hegemony"] = "当你使用<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌时，你可以暗置一名其他角色的一张明置的人物牌。",
	["tuizhi_hegemony:head"] = "暗置主将",
	["tuizhi_hegemony:deputy"] = "暗置副将",
	["@tuizhi_hegemony"] = "你可以选择一名角色，暗置其一张人物牌",
	["tongjie_hegemony"] = "同诘",
	[":tongjie_hegemony"] = "<font color=\"blue\"><b>锁定技，</b></font>当其他角色明置一张武将牌时，你摸一张牌；其他角色不能于你的回合内明置武将牌。",

	["mofa_hegemony"] = "魔法",
	[":mofa_hegemony"] = "<font color=\"blue\"><b>锁定技，</b></font>当你对其他角色造成伤害时，若其有暗置的人物牌，此伤害值+1；当你受到其他角色造成的伤害时，若其有暗置的人物牌，此伤害值-1。",
	["#mofa_damage1"]= "%from 对 %to 的伤害由 %arg2 点减少到 %arg 点。",
	["mofa_hegemony:notice1"] = "魔法： 你即将对 <font color=\"#00FF00\"><b>%src </b></font> 造成 %dest 点伤害，是否亮将增加1点伤害",
	["mofa_hegemony:notice2"] = "魔法： 你即将收到 <font color=\"#00FF00\"><b>%src </b></font> 造成的 %dest点伤害， 是否亮将减少1点伤害",

	["jiezou_hegemony"] = "借走",
	[":jiezou_hegemony"] = "<font color=\"green\"><b>出牌阶段限一次</b></font>，你可以将一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌当无距离限制的【顺手牵羊】使用。",

    
    ["marisa_hegemony"] = "雾雨魔理沙",
	
	
	

--***********************
--春（花）
	["byakuren_hegemony"] = "圣白莲",
	["nue_hegemony"]= "封兽鵺",
	["toramaru_hegemony"] = "寅丸星",
	["murasa_hegemony"] = "村纱水蜜",
	
	["ichirin_hegemony"] = "云居一轮",--国战修改
	["lizhi_hegemony"] = "理智",
	[":lizhi_hegemony"] = "当具有伤害效果的牌结算完毕后，若你为使用者或目标角色，且此牌未造成过伤害，你可以令一名与你阵营相同的角色获得此牌。<font color=\"green\"><b>每阶段限一次。</b></font>",
	["@lizhi"] = "理智： 卡牌 【%src】 并未造成伤害，你可以令一名与你阵营相同的角色获得此牌。 ",
	
	["nazrin_hegemony"] = "娜兹玲",
	
	["miko_hegemony"] = "丰聪耳神子",--国战修改
	["qingting_hegemony"] = "倾听",
	[":qingting_hegemony"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以令有手牌的所有其他角色各将一张手牌交给你，然后你交给这些角色各一张手牌。",
	["chiling_hegemony"] = "敕令",
	[":chiling_hegemony"] = "<font color=\"purple\"><b>主将技，</b></font>此武将牌上单独的阴阳鱼个数-1；当一名其他角色获得你的牌后，你可以令其使用其中的一张牌。",
	["@chiling_hegemony"] = "敕令： 你从 <font color=\"#00FF00\"><b>%src </b></font> 处获得了【%dest】，你可以使用此牌。",
	["shezheng_hegemony"] = "摄政",
	[":shezheng_hegemony"] = "<font color=\"purple\"><b>副将技，</b></font>若你装备区没有武器牌，你视为装备了【雌雄剑（国）】。",
	["shezheng_attach"] = "摄政（出牌阶段亮将）";
	[":shezheng_attach"] = "摄政（出牌阶段亮将）";
	
	
	["mamizou_hegemony"] = "二岩猯藏",
	[":xihua_hegemony"] = "当你需要使用/打出任意基本牌或普通锦囊牌时，若你有手牌，你可以声明之（不能声明于此回合内以此法声明过的牌名）并选择一名其他角色，令其展示你的一张手牌，若此牌与你声明的牌名类别相同，你将此牌当你声明的牌使用/打出，否则你弃置之。",
	
	
	["futo_hegemony"] = "物部布都",
	
	
	["toziko_hegemony"] = "苏我屠自古",--国战修改
	["fenlei_hegemony"] = "忿雷",
	[":fenlei_hegemony"] = "当你脱离濒死状态后，你可以令一名其他角色受到无来源的1点雷电伤害。",
	["@fenlei"] = "你可发动“忿雷”，令一名其他角色受到无来源的1点雷电伤害",
	
	["seiga_hegemony"]= "霍青娥",
	["yoshika_hegemony"]= "宫古芳香",
	["kyouko_hegemony"] = "幽谷响子",
	
	["kogasa_hegemony"] = "多多良小伞",
	["jingxia_hegemony"] = "惊吓" ,--国战削弱
	[":jingxia_hegemony"] = "当你受到1点伤害后，你可以选择一项：依次弃置来源的两张牌；或弃置场上的一张牌。" ,
	["jingxia_hegemony:dismiss"] = "不发动技能" ,
	["jingxia_hegemony:discard"] = "弃置来源的两张牌" ,
	["jingxia_hegemony:discardfield"] = "弃置场上的一张牌" ,
	
	["kokoro_hegemony"] = "秦心",
	["unzan_hegemony"] = "云山",
	["bianhuan_hegemony"] = "变幻",
	[":bianhuan_hegemony"] = "当你受到伤害时，你可以失去1点体力，防止此伤害。",
	["lianda_hegemony"] = "连打",
	[":lianda_hegemony"] = "<font color=\"blue\"><b>锁定技，</b></font>你于出牌阶段使用【杀】的次数+X（X为你已失去的体力值）。",
	["myouren_hegemony"] = "命莲" ,
	["shanshi_hegemony"] = "善逝" ,
	[":shanshi_hegemony"] = "当你于一名其他角色的回合内失去牌后或一名其他角色于你的回合内失去牌后，若失去牌的角色的手牌数小于X（X为其体力值且至少为1），你可以令你与其各摸一张牌。<font color=\"green\"><b>每阶段限一次。</b></font>" ,
	
	["!sunny_hegemony"] = "桑尼·米尔克",
	["sunny_hegemony"] = "桑尼",
	["!lunar_hegemony"] = "露娜·切露德",
	["lunar_hegemony"] = "露娜",
	["!star_hegemony"] = "斯塔·萨菲雅",
	["star_hegemony"] = "斯塔",
	
--***********************
--夏（月）

	["remilia_hegemony"] = "蕾米莉亚",
	["!remilia_hegemony"] = "蕾米莉亚·斯卡蕾特",
	--["skltkexue_hegemony"] = "渴血",
	[":skltkexue_hegemony"] = "你进入濒死状态时，你可以明置此武将牌；当你向其他角色求【桃】时，若其体力值大于其体力下限，其可以失去1点体力，摸一张牌，然后令你回复1点体力。",
	--["skltkexue_hegemony_attach"]= "渴血出桃",
	[":skltkexue_attach_hegemony"]= "当拥有“渴血”的角色处于濒死状态时并向你求【桃】时，若你的体力值大于体力下限，你可以失去1点体力，摸一张牌，然后令其回复1点体力。",
	
	["flandre_hegemony"] = "芙兰朵露",
	["!flandre_hegemony"] = "芙兰朵露·斯卡蕾特",
	["sakuya_hegemony"] = "十六夜咲夜",
	["patchouli_hegemony"] = "帕秋莉",
	["!patchouli_hegemony"] = "帕秋莉·诺蕾姬",
	["bolan_hegemony"] = "博览",
	[":bolan_hegemony"] = "当一名角色使用普通锦囊牌时或成为普通锦囊的目标后，若其与你阵营相同，你可以亮出牌堆顶的两张牌，然后获得其中的锦囊牌和与此牌花色相同的牌。<font color=\"green\"><b>每阶段限一次。</b></font>",
	["hezhou_hegemony"] = "合咒",
	[":hezhou_hegemony"] = "你于回合外可以将两张类别不同的牌当【桃】使用，当以此法使用的牌因结算完毕而置入弃牌堆后，你可以令一名其他角色获得这两张牌中的锦囊牌。",
	["@hezhou_hegemony"] = "你可以发动“合咒” ，令一名其他角色获得 <font color=\"#FF8000\"><b>%src </b></font> ",
	
	
	["meirin_hegemony"] = "红美铃",
	["beishui_hegemony"] = "背水",
	[":beishui_hegemony"] = "你可以将X张牌当任意基本牌使用（X为你的体力值和与你阵营相同的角色数中的较大值）。<font color=\"green\"><b>每阶段限一次。</b></font>",
	
	["koakuma_hegemony"] = "小恶魔",
	["moqi_hegemony"] = "魔契",
	[":moqi_hegemony"] = "当与你阵营相同的角色使用普通锦囊牌时，你可以令此牌的首个效果值+1。<font color=\"green\"><b>每阶段限一次。</b></font>",

	--辉夜 国战削弱
	["kaguya_hegemony"] = "蓬莱山辉夜",
	["ShowShenbao"] = "神宝（出牌阶段亮将）";
	["xuyu_hegemony"] = "须臾",
	[":xuyu_hegemony"] = "<font color=\"purple\"><b>主将技，</b></font><font color=\"blue\"><b>锁定技，</b></font>当你失去最后的手牌后，你移除副将的武将牌，并令此武将牌视为拥有“永恒”。",
	["$xuyuHegemonyAnimate"] = "skill=kaguya:xuyu_hegemony",

	["eirin_hegemony"] = "八意永琳",
	["yaoshi_hegemony"] = "药矢",
	[":yaoshi_hegemony"] = "当你使用牌对一名角色造成伤害时，你可以防止此伤害，令其回复1点体力。",
	["#yaoshi_log"] = "因“%arg”的效果，%from防止了%arg2点伤害.",
	
	["mokou_hegemony"] = "藤原妹红",
	["fengxiang_hegemony"] = "凤翔",
	[":fengxiang_hegemony"] = "你可以将一张红色手牌当【火攻】使用，且此【火攻】效果中的“弃置”改为“重铸”。",
	["kaifeng_hegemony"] = "凯风",
	[":kaifeng_hegemony"] = "当你对一名其他角色造成火焰伤害时或你受到一名其他角色造成的火焰伤害时，若你的体力值小于该角色，你回复1点体力。",
	
	["reisen_hegemony"]="铃仙",
	["!reisen_hegemony"]="铃仙·优昙华院·因幡",
	
	--国战改动
	["keine_hegemony"] = "上白泽慧音",
	["xushi_hegemony"] = "虚史",
	[":xushi_hegemony"] = "当至少两名角色成为牌的目标时，你可以弃置一张手牌取消其中一个目标。",
	["@xushi_hegemony_targetchosen"] = "虚史： 【%src】 有至少两个目标， 你是否弃置一张手牌取消其中一个目标？",
	["~xushi_hegemony"] = "选择一张手牌并选择要取消的目标--》确定",
	["#XushiHegemonySkillAvoid"] = "因为 “%arg”的效果，取消了 【%arg2】 的目标 %from ",
	["xinyue_hegemony"] = "新月",
	[":xinyue_hegemony"] = "当你受到伤害后，你可以令来源将其手牌弃置至X张（X为你的体力值），若如此做，当前回合结束时，你将武将牌“上白泽慧音”替换为“白泽”。",
	["xinyue_hegemony:target"] = "<font color=\"#00FF00\"><b>%src </b></font>对你造成了伤害，你可以发动“新月”，令其将手牌弃至 %arg 张。",

	["keine_sp_hegemony"] = "白泽",
	["wangyue_hegemony"] = "望月",
	[":wangyue_hegemony"] = "当你受到伤害后，你可以将手牌补至X张（X为来源的手牌数且至多为5），若如此做，当前回合结束时，你将武将牌“白泽”替换为“上白泽慧音”。",
	["wangyue_hegemony:target"] = "<font color=\"#00FF00\"><b>%src </b></font>对你造成了伤害，你可以发动“望月”，将手牌补至 %arg 张。",
	
	
	--因幡帝 国战削弱
	["tewi_hegemony"] = "因幡天为",
	["xingyun_hegemony"] = "幸运",
	[":xingyun_hegemony"] = "当你获得牌后，你可以展示其中一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌并选择一项：回复1点体力；或令一名角色摸一张牌。",
	["@xingyun_hegemony"] = "你可以发动“幸运”，展示获得的牌中的一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌。",
	["~xingyun_hegemony"] = "选择此次获得的一张红桃牌 ->确定",
	["xingyun:letdraw"] = "令一名角色摸一张牌" ,
	["xingyun:recover"] = "你回复1点体力" ,
	["@xingyun-select"]= "选择一名角色，令其摸一张牌。",
	

	["toyohime_hegemony"] = "绵月丰姬",
	["yueshi_hegemony"] = "月使",
	[":yueshi_hegemony"] = "<font color=\"purple\"><b>主将技，</b></font>此武将牌上单独的阴阳鱼个数-1；你视为拥有“睿智”（当普通锦囊牌对你结算结束时，若你已受伤，你可以判定，若结果为红色，你回复1点体力）。",
	["yueshi_hegemony:invoke"]= "普通锦囊牌【%src】对你结算结束了，你可以发动“睿智”。",
	
	["yorihime_hegemony"] = "绵月依姬",
	["pingyi_hegemony"] = "凭依",
	[":pingyi_hegemony"] = "当你造成伤害后或受到伤害后，你可以弃置一张牌，令此武将牌视为拥有一个未登场的夏势力人物牌的一个技能（主将技，副将技，限定技除外），直到此人物牌被明置，或你重新发动“凭依”，或人物牌“绵月依姬”被移除或被暗置为止。<font color=\"green\"><b>每回合限一次。</b></font>",
	["@pingyi_hegemony"]= "你是否要弃置一张牌，发动“凭依”",
	
	
	["wriggle_hegemony"] = "莉格露",
	["#wriggle_hegemony"] = "在黑暗中蠢动的光虫",
	["!wriggle_hegemony"] = "莉格露·奈特巴格",
	["yinghuo_hegemony"] = "萤火",
	["chongqun_hegemony"] = "虫群",

	[":yinghuo_hegemony"] = "当你使用基本牌时，若此牌是【闪】或选择的目标只有你，你可以摸一张牌。 ",
	[":chongqun_hegemony"] = "当你因弃置而失去一张基本牌后，你可以弃置一名其他角色的一张手牌。",
	--["@chongqun_target"]= "选择有手牌的一名其他角色 -> 确定",

	["rumia_hegemony"] = "露米娅",
	["zhenye_hegemony"] = "真夜",
	[":zhenye_hegemony"] = "结束阶段开始时，你可以令一名其他角色翻面和令其摸X张牌（X为你的阵营的角色数），然后你翻面。",
	["@zhenye-select-heg"]= "你可以令一名其他角色的人物牌翻面和令其摸X张牌（X为你的阵营的角色数），然后你的人物牌翻面。",
	
	
	["yuka_hegemony"]= "风见幽香",
	
	["!mystia_hegemony"] = "米斯蒂娅·萝蕾拉",
	["mystia_hegemony"]="米斯蒂娅",
	[":yege_hegemony"] = "其他角色的出牌阶段开始时，若场上没有【乐不思蜀】，你可以将一张手牌当【乐不思蜀】对其使用。你以此法使用<font size=\"5\", color=\"#FF0000\"><b>♦</b></font>牌时，你摸一张牌。",
	["shinmyoumaru_hegemony"] = "少名针妙丸",
	["seija_hegemony"] = "鬼人正邪",
	
	
--***********************
--秋（风）	
	["kanako_hegemony"] = "八坂神奈子",
	["qiankun_kanako"] = "乾坤",
	[":qiankun_kanako"] = "<font color=\"blue\"><b>锁定技，</b></font>你的手牌上限+2。",
	
	["suwako_hegemony"] = "洩矢诹访子",
	["qiankun_suwako"] = "乾坤",
	[":qiankun_suwako"] = "<font color=\"blue\"><b>锁定技，</b></font>你的手牌上限+2。",
	["chuancheng_hegemony"] = "传承",
	[":chuancheng_hegemony"] = "当你死亡时，你可以令一名阵营与你相同的其他角色获得“乾坤”和“传承”，然后其获得你区域里的所有牌。",
	["@chuancheng_hegemony"] = "选择一名同阵营的其他角色，令其获得“乾坤”和“传承”以及你区域里的所有牌。",
	
	["sanae_hegemony"] = "东风谷早苗",
	["aya_hegemony"] = "射命丸文",
	["fengsu_attach"] = "风速（出牌阶段亮将）";
	[":fengsu_attach"] = "风速（出牌阶段亮将）";
	
	
	["nitori_hegemony"] = "河城荷取",
	["hina_hegemony"] = "键山雏",
	["momizi_hegemony"] = "犬走椛",
	["minoriko_hegemony"] = "秋穰子",
	["shizuha_hegemony"] = "秋静叶",
	
	--国战改动
	["satori_hegemony"] = "古明地觉",
	["duxin_hegemony"] = "读心",
	[":duxin_hegemony"] = "<font color=\"blue\"><b>锁定技，</b></font>当你选择其他角色的手牌时，其手牌对你可见；当你成为其他角色使用牌的唯一目标时，你观看其暗置的副人物牌。",
	
	--国战改动
	["koishi_hegemony"] = "古明地恋",
	["wunian_hegemony"] = "无念",
    [":wunian_hegemony"] = "<font color=\"blue\"><b>锁定技，</b></font>当你即将造成伤害时，你令一名其他角色成为伤害来源；当你成为其他角色使用锦囊牌的目标时，若你已受伤，你取消自己。",
	["@wunian_transfer"] = "无念：你即将对 %src 造成伤害，你须令一名其他角色成为此伤害来源",

	
	["utsuho_hegemony"] = "灵乌路空",
	["rin_hegemony"] = "火焰猫燐",
	["yugi_hegemony"] = "星熊勇仪",
	["parsee_hegemony"]= "水桥帕露西",
	
	["yamame_hegemony"] = "黑谷山女",
	["chuanran_hegemony"] = "传染",
	[":chuanran_hegemony"] = "当一名角色受到【杀】造成的非属性伤害后，若来源属于你的阵营或处于连环状态，你可令该角色横置。",
	
	["kisume_hegemony"] = "琪斯美",
	
	["diaoping_hegemony"] = "钓瓶",
	[":diaoping_hegemony"] = "当其他角色使用【杀】指定目标后，若目标包含与你阵营相同的角色，你可以与此牌的使用者拼点：当你赢后，其横置且此【杀】无效；当你没赢后，你与其各摸一张牌。",
    ["diaoping_hegemony:slashtarget"]= "<font color=\"#FF8000\"><b>%src </b></font> 使用了【%dest】，你可以发动“钓瓶”。",
	
	["suika_hegemony"] = "伊吹萃香",
	[":zuiyue_hegemony"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>若你于此阶段内使用过非基本牌，你可以视为使用【酒】。",
	[":doujiu_hegemony"] = "<font color=\"purple\"><b>主将技，</b></font>此武将牌上单独的阴阳鱼个数-1；当其他角色使用【桃】或【酒】时，你可以摸一张牌并与其拼点，当你赢后，此牌无效，你回复1点体力。<font color=\"green\"><b>每阶段限一次。</b></font>",
	["cuiji_hegemony"] = "萃集",
	[":cuiji_hegemony"] = "<font color=\"purple\"><b>副将技，</b></font>摸牌阶段，你可以少摸一张牌，声明：基本牌，或非基本牌，或一种花色，然后检索一张符合条件的牌并获得之。",
	["nonbasic"] = "非基本牌",
	
	["kasen_hegemony"] = "茨木華扇",
	["hatate_hegemony"] = "姬海棠果",
	["kuaizhao_hegemony"] = "快照",
	[":kuaizhao_hegemony"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以令一名角色视为对另一名角色使用【知己知彼（国）】；你须执行你使用的【知己知彼（国）】效果里的所有选项。",
	
--***********************
--冬（雪）

	["yuyuko_hegemony"] = "西行寺幽幽子",
	["yukari_hegemony"]= "八云紫",
	["ran_hegemony"]= "八云蓝",
	["shihui_hegemony"] = "式辉",
	[":shihui_hegemony"] = "当你使用【杀】造成伤害后或受到【杀】造成的伤害后，你可以视为使用【以逸待劳（国）】。", 
	--[":shihui_hegemony"] = "当一名角色使用【杀】造成伤害后或受到【杀】造成的伤害后，若其与你阵营相同，你可以令其选择是否将一张装备牌当【无中生有】使用。<font color=\"green\"><b>每阶段限一次。</b></font>", 
	--["shihuiuse_hegemony"]= "“式辉”：你可将一张装备牌当【无中生有】使用",
	--["~shihui_hegemonyVS"]= "选择一张装备牌 -> 确定 -> 转化 或 取消",

	
	["youmu_hegemony"] = "魂魄妖梦",
	--["prismriver"]= "普莉兹姆利巴三姐妹",
	
	["lunasa_hegemony"] =  "露娜萨",
	["!lunasa_hegemony"] =  "露娜萨·普莉兹姆利巴",
	["#lunasa_hegemony"] = "骚灵提琴手",
	["hezou_hegemony"] = "合奏",
	[":hezou_hegemony"] = "你可以将一张非基本牌当【杀】使用或打出，以此法使用或打出牌时，若场上有与之花色相同的牌，你摸一张牌。",
	["xianling_hegemony"] = "弦灵",
	[":xianling_hegemony"] = "<font color=\"blue\"><b>锁定技，</b></font>与你阵营相同的角色的手牌上限+1。",
	
	["merlin_hegemony"] =  "梅露兰",
	["!merlin_hegemony"] =  "梅露兰·普莉兹姆利巴",
	["#merlin_hegemony"] = "骚灵小号手",
	["jizou_hegemony"] = "激奏",
	[":jizou_hegemony"] = "当你对一名其他角色造成伤害后，你可以视为对其使用【过河拆桥】。",
	["guanling_hegemony"] = "管灵",
	[":guanling_hegemony"] = "<font color=\"blue\"><b>锁定技，</b></font>与你阵营相同的角色的出牌阶段使用【杀】的次数+1。",
	
	
	["lyrica_hegemony"] =  "莉莉卡",
	["!lyrica_hegemony"] =  "莉莉卡·普莉兹姆利巴",
	["#lyrica_hegemony"] = "骚灵键盘手",
	["jianling_hegemony"] = "键灵",
	[":jianling_hegemony"] = "<font color=\"blue\"><b>锁定技，</b></font>与你阵营相同的角色于其摸牌阶段多摸一张牌。",
	
	["alice_hegemony"]="爱丽丝",
	["chen_hegemony"] = "橙",
		
	["letty_hegemony"]="蕾蒂",
	["illustrator:letty_hegemony"] = "猫車",
	["origin:letty_hegemony"] = "p号：39410238；个人ID：167342",
	["hanbo_hegemony"] = "寒波",
	[":hanbo_hegemony"] = "<font color=\"blue\"><b>锁定技，</b></font>当你对其他角色造成非属性伤害时，若其没有手牌，此伤害值+1。",
	["#HanboEffect"] = "%from 的【<font color=\"yellow\"><b>寒波</b></font>】效果被触发， %to 没有手牌，伤害从 %arg 增加至 %arg2",
	["dongzhi_hegemony"] = "冬至",
	["dongzhi"] = "冬至",
	[":dongzhi_hegemony"] = "<font color=\"red\"><b>限定技，</b></font>出牌阶段，你可以选择任意名同一阵营的有牌的角色，依次弃置这些角色各X张牌（X为其装备区里的牌数+1）。",
	--["#DongzhiDamage"] = "%from 的 <font color=\"yellow\"><b>冬至</b></font>效果被触发，伤害由 %arg 点增加至 %arg2 点",
	["$dongzhiAnimate"]= "skill=letty:dongzhi",
	
	
	["lilywhite_hegemony"] = "莉莉霍瓦特",
	["baochun_hegemony"]= "报春",
	[":baochun_hegemony"]= "<font color=\"blue\"><b>锁定技，</b></font>当你受到伤害后或回复体力后，与你阵营相同的角色各摸一张牌；当一名同阵营的其他角色受到伤害后，你弃置一张牌。",
	["chunhen_hegemony"]= "春痕",
	[":chunhen_hegemony"]= "当你的一张红色牌因弃置而置入弃牌堆后，你可以将之交给一名其他角色。",
	["#chunhen_temp"]= "春痕",
	["@chunhen_give"] = "春痕： 你可将你被弃置的红色牌交给一名其他角色",
	["~chunhen_hegemony"]= "选择牌和一名其他角色 --> 确定",
	
	--上海国战削弱
	["shanghai_hegemony"] = "上海人形" ,
	--["zhancao_hegemony"] = "战操" ,
	--[":zhancao_hegemony"] = "当你或一名在你攻击范围内的角色成为【杀】的目标后，你可以弃置一张装备区的牌，令此【杀】对其无效。",
	--["@zhancao_hegemony-discard"] = "<font color=\"#00FF00\"><b>%src </b></font>使用【杀】指定了<font color=\"#FF8000\"><b>%dest </b></font>为目标，你可以发动“战操”，弃置一张装备区的牌，使此【杀】对<font color=\"#FF8000\"><b>%dest </b></font>无效" ,
	--["#zhancaoTarget"] = "%from 使用 %arg 的目标是 %to。" ,
	--["mocao_hegemony"] = "魔操" ,
	--[":mocao_hegemony"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以获得一名其他角色装备区里的一张牌，令其摸X张牌（X为其已损失的体力值且至少为1）。",

	["youki_hegemony"] = "魂魄妖忌" ,
	
	["cirno_hegemony"] = "琪露诺",--国战修改
	["dongjie_hegemony"] = "冻结",
	[":dongjie_hegemony"] = "当你使用【杀】对一名角色造成伤害时，你可以令其选择一项：摸一张牌，然后翻面并防止此伤害，或弃置一张手牌。",
	["@dongjie_discard"] =" %src 对你发动了“冻结”， 你可弃置一张手牌，否则将摸一张牌，翻面，防止当前伤害",
	["bingpo_hegemony"]= "冰魄",
	[":bingpo_hegemony"]= "<font color=\"blue\"><b>锁定技，</b></font>当你不因火焰伤害而进入濒死状态时，你回复1点体力。",
	["#bingpo_hegemony_log"] = "%from的“%arg”被触发, %from 回复了 %arg2点体力.",
	
	["daiyousei_hegemony"]= "大妖精",--国战修改
	["banyue_hegemony"]= "半月",
	[":banyue_hegemony"]= "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以选择两名阵营不同的角色并失去1点体力，视为其中一名对另一名角色使用【远交近攻】。",
	["juxian_hegemony"]= "具现",
	[":juxian_hegemony"]= "<font color=\"red\"><b>限定技，</b></font>当你进入濒死状态时，你可以亮出牌堆顶的三张牌并获得之，然后你回复x点体力（x为其中的花色数）。",
	["$juxianAnimate"]= "skill=daiyousei:juxian",
	
	["renko_hegemony"] = "宇佐见莲子",
	["merry_hegemony"] = "玛艾露贝莉",
	["!merry_hegemony"] = "玛艾露贝莉·赫恩",
	
	
	["luanying_hegemony"] = "乱影",
	[":luanying_hegemony"] = "<font color=\"purple\"><b>主将技，</b></font>此武将牌上单独的阴阳鱼个数-1；当其他角色使用基本牌时，你可以将一张与此牌颜色相同的“境界”交给其，令此牌无效。<font color=\"green\"><b>每阶段限一次。</b></font>",
    ["mengxian_hegemony"] = "梦现",
	--[":mengxian_hegemony"] = "<font color=\"purple\"><b>副将技，</b></font>一名角色的弃牌阶段开始时，你可以弃置X张境界，令其的手牌上限于本回合+X。",
	[":mengxian_hegemony"] = "<font color=\"purple\"><b>副将技，</b></font>当与你阵营相同的角色使用【杀】对目标角色造成伤害时，你可以将一张“境界”置入弃牌堆，防止此伤害，依次弃置该角色两张牌。",
	["@mengxian_hegemony"] = "你可以发动“梦现”： 你可以防止 %src 对 %dest  的伤害， 并弃置%dest两张牌",
	["~mengxian_hegemony"] = "选择一张境界 -> 确定",
	
	["rinnosuke_hegemony"] = "森近霖之助",
	["tokiko_hegemony"] = "朱鹭子",
	["mima_hegemony"] = "魅魔" ,
	
------------------------------------------
-- 设计者 @dawda 错了自己改
	["designer:reimu_hegemony"] = "三国有单", -- 推重
	["designer:marisa_hegemony"] = "星野梦美☆",

	["designer:byakuren_hegemony"] = "星野梦美☆",
	["designer:nue_hegemony"] = "星野梦美☆",
	["designer:toramaru_hegemony"] = "三国有单",
	["designer:murasa_hegemony"] = "星野梦美☆",
	["designer:ichirin_hegemony"] = "星野梦美☆",
	["designer:nazrin_hegemony"] = "三国有单",
	["designer:miko_hegemony"] = "星野梦美☆",
	["designer:mamizou_hegemony"] = "星野梦美☆",
	["designer:futo_hegemony"] = "星野梦美☆",
	["designer:toziko_hegemony"] = "星野梦美☆",
	["designer:seiga_hegemony"] = "星野梦美☆",
	["designer:yoshika_hegemony"] = "星野梦美☆",
	["designer:kyouko_hegemony"] = "星野梦美☆",
	["designer:kogasa_hegemony"] = "星野梦美☆",
	["designer:kokoro_hegemony"] = "星野梦美☆",
	["designer:unzan_hegemony"] = "三国有单", -- 这个云山是推重的吗？
	["designer:myouren_hegemony"] = "辰焰天明",
	["designer:sunny_hegemony"] = "星野梦美☆",
	["designer:lunar_hegemony"] = "星野梦美☆",
	["designer:star_hegemony"] = "三国有单",

	["designer:remilia_hegemony"] = "星野梦美☆",
	["designer:flandre_hegemony"] = "星野梦美☆",
	["designer:sakuya_hegemony"] = "星野梦美☆",
	["designer:patchouli_hegemony"] = "bullytou",
	["designer:meirin_hegemony"] = "三国有单",
	["designer:koakuma_hegemony"] = "bullytou",
	["designer:kaguya_hegemony"] = "三国有单", -- 缝合怪
	["designer:eirin_hegemony"] = "星野梦美☆",
	["designer:mokou_hegemony"] = "星野梦美☆",
	["designer:reisen_hegemony"] = "星野梦美☆",
	["designer:keine_hegemony"] = "辰焰天明",
	["designer:keine_sp_hegemony"] = "星野梦美☆",
	["designer:tewi_hegemony"] = "星野梦美☆",
	["designer:toyohime_hegemony"] = "星野梦美☆",
	["designer:yorihime_hegemony"] = "星野梦美☆",
	["designer:wriggle_hegemony"] = "星野梦美☆",
	["designer:rumia_hegemony"] = "星野梦美☆",
	["designer:yuka_hegemony"] = "星野梦美☆",
	["designer:mystia_hegemony"] = "辰焰天明",
	["designer:shinmyoumaru_hegemony"] = "三国有单",
	["designer:seija_hegemony"] = "星野梦美☆",

	["designer:kanako_hegemony"] = "星野梦美☆",
	["designer:suwako_hegemony"] = "星野梦美☆",
	["designer:sanae_hegemony"] = "星野梦美☆",
	["designer:aya_hegemony"] = "星野梦美☆",
	["designer:nitori_hegemony"] = "星野梦美☆",
	["designer:hina_hegemony"] = "三国有单",
	["designer:momizi_hegemony"] = "星野梦美☆",
	["designer:minoriko_hegemony"] = "星野梦美☆",
	["designer:shizuha_hegemony"] = "星野梦美☆",
	["designer:satori_hegemony"] = "星野梦美☆",
	["designer:koishi_hegemony"] = "星野梦美☆",
	["designer:utsuho_hegemony"] = "星野梦美☆",
	["designer:rin_hegemony"] = "星野梦美☆",
	["designer:yugi_hegemony"] = "星野梦美☆",
	["designer:parsee_hegemony"] = "三国有单",
	["designer:yamame_hegemony"] = "三国有单",
	["designer:kisume_hegemony"] = "星野梦美☆",
	["designer:suika_hegemony"] = "星野梦美☆",
	["designer:kasen_hegemony"] = "三国有单",
	["designer:hatate_hegemony"] = "名和行年",

	["designer:yuyuko_hegemony"] = "三国有单",
	["designer:yukari_hegemony"] = "三国有单",
	["designer:ran_hegemony"] = "三国有单",
	["designer:youmu_hegemony"] = "三国有单",
	["designer:lunasa_hegemony"] = "三国有单", -- 国战独占
	["designer:merlin_hegemony"] = "三国有单", -- 国战独占
	["designer:lyrica_hegemony"] = "三国有单", -- 国战独占
	["designer:alice_hegemony"] = "三国有单",
	["designer:chen_hegemony"] = "三国有单",
	["designer:letty_hegemony"] = "名和行年", -- 推重，看到了设计者
	["designer:lilywhite_hegemony"] = "三国有单", -- 推重
	["designer:shanghai_hegemony"] = "星野梦美☆",
	["designer:youki_hegemony"] = "辰焰天明",
	["designer:cirno_hegemony"] = "星野梦美☆",
	["designer:daiyousei_hegemony"] = "星野梦美☆",
	["designer:renko_hegemony"] = "星野梦美☆",
	["designer:merry_hegemony"] = "星野梦美☆",
	["designer:rinnosuke_hegemony"] = "星野梦美☆",
	["designer:tokiko_hegemony"] = "星野梦美☆",
	["designer:mima_hegemony"] = "bullytou",
}
