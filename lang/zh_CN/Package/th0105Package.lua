return {
	["th0105"] = "旧作",
	["pc98"] = "旧" ,


	--pc98001 魅魔 势力：旧 4血
	["mima"] = "魅魔" ,
	["#mima"] = "复仇的幽灵",
	["designer:mima"] = "bullytou",

	["illustrator:mima"] = "ん",
	["origin:mima"] = "个人ID：134827",
	["illustrator:mima_1"] = "Cro",
	["origin:mima_1"] = "p号：4117188，个人ID：185077",
	["illustrator:mima_2"] = "カタケイ",
	["origin:mima_2"] = "p号：43376152, 个人ID：90042",

	["meiling"] = "魅灵",
	[":meiling"] = "一名角色受到其他角色造成的伤害后，若其存活，你可获得造成伤害的牌。若你与其距离大于X（X为你已损失的体力值），来源对你造成1点伤害。",
	["meiling:distance"]= "你可发动“魅灵”，获得【%arg】。 你与 受伤者<font color=\"#00FF00\"><b>%src </b></font> 的距离为 <font color=\"#FF8000\"><b>%dest </b></font>。",
	["fuchou"] = "复仇",
	[":fuchou"] = "<font color=\"orange\"><b>主公技，</b></font><font color=\"blue\"><b>锁定技，</b></font>你使用【杀】指定其他旧势力角色的攻击范围内且体力值大于其的目标后，此【杀】不计入次数限制。",
	["#fuchou"] = "%from 的 主公技 %arg 被触发， %from 使用的【杀】不计入次数限制。",


--************************************************
	--pc98002 冈崎梦美  势力：旧 4血
	["yumemi"] = "冈崎梦美",
	["#yumemi"] = "梦幻传说",
	["designer:yumemi"] = "三国有单",

	["illustrator:yumemi"] = "poppy",
	["origin:yumemi"] = "个人ID:3596054",
	["illustrator:yumemi_1"] = "Yukian",
	["origin:yumemi_1"] = "p号： 42368069,个人ID:4440",
	["illustrator:yumemi_2"] = "qyx",
	["origin:yumemi_2"] = "个人ID:31564",
	["illustrator:yumemi_3"] = "隂宅",
	["origin:yumemi_3"] = "个人ID:979179",
	["illustrator:yumemi_4"] = " えに ",
	["origin:yumemi_4"] = "p号： 54506014,个人ID:3217912",


	["ciyuan"] = "次元",
	[":ciyuan"] = "回合开始时，你可交换此回合两个阶段的顺序。",
	["#ciyuan"] = "%from 将 %arg 和 %arg2 的执行顺序互换",
	["ciyuan:start"] = "准备阶段",
	["ciyuan:judge"] = "判定阶段",
	["ciyuan:draw"] = "摸牌阶段",
	["ciyuan:play"] = "出牌阶段",
	["ciyuan:discard"] = "弃牌阶段",
	["ciyuan:finish"] = "结束阶段",
	["shigui"] = "时轨",
	[":shigui"] = "当摸牌阶段或出牌阶段结束时，你可将手牌调整至X张（X为此回合已执行阶段数）。若以此法获得牌，你失去1点体力；若以此法失去牌，你回复1点体力。<font color=\"green\"><b>每回合限一次。</b></font>",


	["shigui_draw"] = "时轨(摸牌)",
	["shigui_play"] = "时轨(弃牌)",
	["shigui_discard"] = "你发动“时轨(弃牌)”，请弃置%src张手牌。",
	["shigui:draw_notice"]= "你可发动技能 “时轨(摸牌)”， 目前进行的阶段数为<font color=\"#FF8000\"><b>%src </b></font> 。",
	["shigui:play_notice"]= "你可发动技能 “时轨(弃牌)”， 目前进行的阶段数为<font color=\"#FF8000\"><b>%src </b></font> 。",

	["chongdong"] = "虫洞",
	[":chongdong"] = "<font color=\"orange\"><b>主公技，</b></font>当你于其他角色的出牌阶段内受到伤害后，你可令其他旧势力角色选择是否弃置一张红色手牌，结束此回合。",
	["#chongdong"]= "%from 将结束回合。",
	["@chongdong"]= "你是否响应“虫洞”，弃置一张红色手牌，结束当前回合。",


--************************************************
	--pc98003 北白河千百合 势力：旧 4血
	["chiyuri"] = "北白河千百合" ,
	["#chiyuri"] = "超越时空的梦幻居民",
	["designer:chiyuri"] = "三国有单",

	["illustrator:chiyuri"] = "・・・（mitei）",
	["origin:chiyuri"] = "p号： 50573343,个人ID:4752685",
	["illustrator:chiyuri_1"] = "伊吹のつ",
	["origin:chiyuri_1"] = "p号： 51921215,个人ID:7013",
	["illustrator:chiyuri_2"] = "miya (tsumazukanai)",
	["origin:chiyuri_2"] = "p号：,个人ID:",
	["illustrator:chiyuri_3"] = " 朧月カケル ",
	["origin:chiyuri_3"] = "p号：1969063,个人ID:47896",
	["illustrator:chiyuri_4"] = "ワダンテ",
	["origin:chiyuri_4"] = "p号：51430579 ,个人ID:3811457",

	["zhence"] = "侦测",
	[":zhence"] = "摸牌阶段或出牌阶段开始前，你可视为使用【火攻】。当你使用此牌造成伤害后，你摸一张牌并跳过该阶段。",
	["@zhence"]= "你可发动“侦测”， 视为使用【火攻】。若你每以此法使用牌造成一次伤害后，你摸一牌并跳过<font color=\"#FF8000\"><b>%src </b></font>阶段",
	["~zhence"]= "选择一名角色，确认",
	["shiqu"] = "时驱",
	[":shiqu"] = "一名角色的结束阶段开始时，你可声明一个此回合未执行的阶段并弃置一张牌，令你或该角色于此回合结束后进行一个仅有你声明的阶段的额外回合。",
	["shiqu_start"] = "准备阶段",
	["shiqu_judge"] = "判定阶段",
	["shiqu_draw"] = "摸牌阶段",
	["shiqu_play"] = "出牌阶段",
	["shiqu_discard"] = "弃牌阶段",
	["@shiqu-discard"] = "你可发动“时驱”，弃置一张牌， 然后令你自己 或 当前回合人 <font color=\"#00FF00\"><b>%dest </b></font> 进行 一个<font color=\"#FF8000\"><b>%src </b></font>",
	["#shiqu"] = "%from 发动 “%arg” 令 %to 将会于此回合后进行一个仅含 %arg2 的额外回合",
	["~shiqu"] = "选择牌和目标->确定",


--************************************************
	--pc98004 朝仓理香子 势力：旧 4血
	["rikako"] = "朝仓理香子" ,
	["#rikako"] = "寻找梦想的科学",
	["designer:rikako"] = "辰焰天明",

	["illustrator:rikako"] = "べる",
	["origin:rikako"] = "p号：7874458 ,个人ID:125888",
	["zhenli"] = "真理",
	[":zhenli"] = "<font color=\"blue\"><b>锁定技，</b></font>分发起始手牌后，你摸二十四张牌；当你从牌堆顶摸牌后，你将之扣置于人物牌上，称为“真理”。",
	["qiusuo"] = "求索",
	[":qiusuo"] = "准备阶段开始时，你可展示并获得至少一张点数均相同的“真理”；结束阶段开始时，若你于本回合内造成过伤害，你可展示并获得至少一张点数均相同的“真理”。",
	["@qiusuo"] = "你可展示并获得 任意张点数均相同 的“真理”牌";
	["~qiusuo"] = "选择牌-->确认"; 

--************************************************
	--pc98005 卡纳  势力：旧 3血
	["kana"] = "卡娜" ,
	["!kana"] = "卡娜•安娜贝拉尔" ,
	["#kana"] = "失去梦的少女骚灵" ,
	["designer:kana"] = "三国有单",

	["illustrator:kana"] = " c7肘 ",
	["origin:kana"] = "p号：44923233,个人ID:217707",
	["illustrator:kana_1"] = "Magician",
	["origin:kana_1"] = "p号：51366112,个人ID:5100338",
	["illustrator:kana_2"] = "Culter",
	["origin:kana_2"] = "p号：37805413,个人ID:542147",

	["mengxiao"] = "梦消",
	[":mengxiao"] = "结束阶段开始时，你可将一张手牌当任意一次性延时锦囊牌置入判定区里；其他角色的准备阶段开始时，你可将你判定区里的一张牌置入其判定区。",
	["$Mengxiao"] = "%from 发动 “%arg”, 将 %card 当做 %arg2 置入判定区。",
	["@mengxiao"]= "你可将一张手牌当做 【%src】 置入你的判定区。",

    ["lubiao"] = "路标",
	[":lubiao"] = "<font color=\"blue\"><b>锁定技，</b></font>当你受到牌造成的伤害时，若场上有与之颜色不同的延时锦囊牌，此伤害值-1。",
	["#lubiao"] = "场上有延时锦囊牌， %from 的技能 “%arg” 被触发, %from 防止了 %arg2 点伤害。",
--************************************************
	--pc98006 幽香 势力：旧 4血
	["!yuka_old"]= "旧作幽香",
	["yuka_old"]= "幽香",
	["#yuka_old"] = "妖怪小姐" ,
	["designer:yuka_old"] = "辰焰天明",

	["illustrator:yuka_old"] = "ヨークタウンCV-5",
	["origin:yuka_old"] = "p号：57810973,个人ID:4602524",
	["illustrator:yuka_old_1"] = "fancybetty ",
	["origin:yuka_old_1"] = ":",
	["illustrator:yuka_old_2"] = "くろぬこネーロ",
	["origin:yuka_old_2"] = "p号：32809829,个人ID:335493",

	["yeyan"] = "夜魇",
	[":yeyan"] = "<font color=\"blue\"><b>锁定技，</b></font>当你使用【杀】或普通锦囊牌指定目标后，除你外的目标角色各弃置一张手牌并令你选择一项：展示一张与之类别相同手牌，或此牌对其无效。",
	["#yeyan"] = "%from 对 %to 使用%arg2，并触发了“%arg”效果 ",
	["yeyan-discard"] = "%dest 对你使用 %src 并触发了“夜魇”， 请弃置一张手牌",
	["yeyan-show"] = "“夜魇”： 你可展示一张 %arg ， 令 %src 对 %dest 生效；或令 %src 对 %dest 无效",
	["youyue"] = "幽月",
	[":youyue"] = "出牌阶段结束时，你可摸X张牌（X为没有手牌的角色数）。",
	["menghuan"] = "梦幻",
	[":menghuan"] = "<font color=\"orange\"><b>主公技，</b></font><font color=\"blue\"><b>锁定技，</b></font>你的手牌上限+Y（Y为其他旧势力角色最大的体力值）。",


--************************************************
	--pc98007 幻月/梦月 势力：旧 3血
	["gengetsumugetsu"] = "梦月 ＆ 幻月" ,
	["&gengetsumugetsu"] = "梦月幻月" ,
	["#gengetsumugetsu"] = "女仆 ＆ 恶魔" ,
	["designer:gengetsumugetsu"] = "辰焰天明",

	["illustrator:gengetsumugetsu"] = " tengu ",
	["origin:gengetsumugetsu"] = "p号：55411659,个人ID:4767631",
	["illustrator:gengetsumugetsu_1"] = "二酸化炭素",
	["origin:gengetsumugetsu_1"] = "p号：55591943,个人ID:9149093",
	["illustrator:gengetsumugetsu_2"] = "カタケイ",
	["origin:gengetsumugetsu_2"] = "p号：59852466,个人ID:90042",
	["illustrator:gengetsumugetsu_3"] = "べらぼう",
	["origin:gengetsumugetsu_3"] = "p号：54503236,个人ID:6259229",
	
	["xuxiang"] = "虚像",
	[":xuxiang"] = "当其他角色使用【杀】对目标角色造成伤害时，你可与其拼点：当你赢后，若拼点牌中有【闪】，你可令伤害值+1。若拼点牌中有【杀】，你可令伤害值-1。<font color=\"green\"><b>每阶段限一次。</b></font>",
	["xuxiang:target"]= "<font color=\"#FF8000\"><b>%src </b></font> 使用【%arg】对  %dest 造成了 %arg2 点伤害。 你可发动“虚像”与 <font color=\"#FF8000\"><b>%src </b></font> 拼点。",
	["#Xuxiang_minus"]= "%from 受到的伤害因 %arg效果 减少 %arg2 点。",
	["#Xuxiang_plus"]= "%from 受到的伤害因 %arg效果 增加 %arg2 点。",
	["#xuxiang_plus"] = "虚像加伤",
	["#xuxiang_minus"] = "虚像减伤",
	["#xuxiang"] = "虚像(伤害修正)",
	["huanjue"] = "幻觉",
	[":huanjue"] = "当你需要选择拼点牌时，若对方在你的攻击范围内，你可观看牌堆顶的两张牌并将其中一张作为你的拼点牌。若如此做，对方选择拼点牌时可将另一张作为其拼点牌。",



--************************************************
	--pc98008 艾丽 势力：旧 4血
	["elly"] = "艾丽" ,
	["#elly"] = "馆的门卫" ,
	["designer:elly"] = "工藤",

	["illustrator:elly"] = "ミルキャラ",
	["origin:elly"] = "p号：23409861,个人ID:19359",
	["illustrator:elly_1"] = "sheya",
	["origin:elly_1"] = "p号：59479020,个人ID:11764388",
	["illustrator:elly_2"] = "shouen kigashi ",
	["origin:elly_2"] = "",
	["illustrator:elly_3"] = "菊月",
	["origin:elly_3"] = "p号：43923582,个人ID:429883",

	["lianmu"] = "镰幕",
	[":lianmu"] = "当你使用【杀】结算结束后，若此牌未造成过伤害，你可视为使用【杀】，无距离限制且可以额外指定一名目标。<font color=\"green\"><b>每阶段限一次。</b></font>",
	["@lianmu"] = "你可发动“镰幕”视为使用【杀】（无距离限制）",
	["~lianmu"] = "选择杀的目标 ->确定",
	["#lianmu_mod"] = "镰幕",
	["huanwei"] = "幻卫",
	[":huanwei"] = "<font color=\"blue\"><b>锁定技，</b></font>回合外，你手牌里的黑桃【杀】视为【闪】；当你于你回合内使用黑桃【杀】对目标角色造成伤害时，伤害值-1。",
	["#HuanweiTrigger"]= "%from 的 “%arg” 被触发，对 %to 的伤害减少了 %arg2 点伤害。",
	["#huanwei"] = "幻卫",

--************************************************
	--pc98009 神绮 势力：旧 4血
	["shinki"] = "神绮" ,
	["#shinki"] = "魔界之神" ,
	["designer:shinki"] = "辰焰天明",


	["illustrator:shinki"] = "音無空太",
	["origin:shinki"] = "p号：50941526,个人ID:6273920",
	["illustrator:shinki_1"] = "Vetina",
	["origin:shinki_1"] = "p号：13059449,个人ID:516980",
	["illustrator:shinki_2"] = "Hysteria",
	["origin:shinki_2"] = "p号：,个人ID:1777704",


	["sqchuangshi"] = "创世",
	[":sqchuangshi"] = "出牌阶段开始时，你可选择任意名角色，令这些角色各可使用一张牌。",
	["@sqchuangshi"] = "“创世”： 你可选择任意名角色，令这些角色各可使用一张牌";
	["~sqchuangshi"] = "选择至少一名目标 --> 确定";
	["@sqchuangshi_use"] = "“创世”： 你可使用一张牌";
	["yuanfa"] = "源法",
	[":yuanfa"] = "结束阶段开始时，你可选择所有于本回合内使用过牌的角色，令这些角色各摸一张牌。",
	["shenwei"] = "神威",
	[":shenwei"] = "<font color=\"orange\"><b>主公技，</b></font><font color=\"blue\"><b>锁定技，</b></font>当其他角色于你的回合内使用<font size=\"5\", color=\"#FF0000\"><b>♦</b></font>【闪】时，若其在不少于两名旧势力角色的攻击范围内，此牌无效。",
	["#shenwei"]= "由于“%arg”的效果，%from使用的【%arg2】无效。",


--************************************************
	--pc98010 萝莉丝 势力：旧 3血
	["alice_old"] = "旧作爱丽丝",
	["#alice_old"] = "死之少女" ,
	["&alice_old"] = "爱丽丝" ,
	["designer:alice_old"] = "辰焰天明",

	["illustrator:alice_old"] = " みや ",
	["origin:alice_old"] = "",
	["illustrator:alice_old_1"] = " cierra (ra-bit) ",
	["origin:alice_old_1"] = "",
	["illustrator:alice_old_2"] = " ミーモク",
	["origin:alice_old_2"] = "p号：29830485,个人ID:1021390",
	["illustrator:alice_old_3"] = " MAKI",
	["origin:alice_old_3"] = "p号：15034012,个人ID:1251",

		["modian_attach"] = "魔典给牌",
	["：modian_attach"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以将一张黑色手牌置于魔典技能人的人物牌上。",
	["@modian"] = "“魔典”牌数已经大于你的体力值，你须将一张“魔典”牌置入弃牌堆",
	["~modian"] = "选择一张魔典牌-->确定",
	["modian:draw"] = "摸牌",
	["modian:recover"] = "回复体力",


	["modian"] = "魔典",
	[":modian"] = "<font color=\"green\"><b>每名角色的出牌阶段限一次，</b></font>其可将一张【杀】或普通锦囊牌并置于你人物牌上，称为“魔典”，然后：若“魔典”中没有与之牌名相同的牌，其摸一张牌；若“魔典”数大于你的体力值，你将一张“魔典”置入弃牌堆并回复1点体力。",
	["guaiqi"] = "怪绮",
	[":guaiqi"] = "你可将“魔典”中的一张锦囊牌当【杀】使用；你可将“魔典”中的一张【杀】当“魔典”中的任意锦囊牌使用。",
	
--************************************************
	--pc98011 萨丽艾尔 势力：旧 4血
	["sariel"] = "萨丽艾尔",
	["#sariel"] = "死之天使" ,
	["designer:sariel"] = "辰焰天明",
	["baosi"] = "报死",
	[":baosi"] = "当你使用【杀】或黑色普通锦囊牌指定目标时，你可以选择任意名不是此牌目标且体力值不大于其体力下限的其他角色，这些角色成为此牌的额外目标。",
	["@baosi"] = "你可发动“报死”， 选择 任意 体力值不大于其体力下限的其他角色 为 你使用的 【<font color=\"#FF8000\"><b>%src </b></font>】 的 额外目标",
	["~baosi"] = "选择目标 --> 确定",
	["#baosi-dist"] = "报死（距离）",
	
	["ezhao"] = "噩兆",
	[":ezhao"] = "<font color=\"red\"><b>限定技，</b></font>出牌阶段，你可选择至少一名已受伤的角色，这些角色各回复1点体力并加1点体力下限。",
	["$ezhaoAnimate"]= "skill=sariel:ezhao",
	["moyan"] = "魔眼",
	[":moyan"] = "<font color=\"red\"><b>限定技，</b></font>出牌阶段，你可以令至多X名其他角色加1点体力下限（X为你已损失的体力值）。",
	["$moyanAnimate"]= "skill=sariel:moyan",
	
	
--************************************************
	--pc98012 矜羯罗 势力：旧 4血
	["konngara"] = "矜羯罗",
	["#konngara"] = "星幽剑士" ,
	["designer:konngara"] = "辰焰天明",

	["zongjiu"] = "纵酒",
	[":zongjiu"] = "当一名角色使用【桃】时，你可明置一张手牌；当你需要使用【酒】时，你可暗置一张手牌，视为使用【酒】。",
	["@zongjiu"] = "你可发动“纵酒”， 明置一张（手）牌",
	["xingyou"] = "星幽",
	["#xingyou"] = "星幽",
	--[":xingyou"] = "当你使用【杀】时，若你有与此【杀】花色相同的明置手牌，你可摸一张牌；当你使用的【杀】被目标角色使用的【闪】抵消时，若你有与此【闪】花色相同的明置手牌，你可弃置其一张牌。",
	[":xingyou"] = "<font color=\"blue\"><b>锁定技，</b></font>当你使用与你明置手牌花色相同的【杀】时，你摸一张牌；你的回合内，其他角色不能使用与你明置手牌花色相同的【闪】。",

	
--************************************************	
	--pc98013 梦子 势力：旧 4血
	["yumeko"] = "梦子",
	["#yumeko"] = "魔界女仆" ,
	["designer:yumeko"] = "名和行年",

	["huanshu"] = "幻术",
	[":huanshu"] = "<font color=\"blue\"><b>锁定技，</b></font>当其他角色使用的基本牌或普通锦囊牌对你结算结束后，若你明置手牌数小于你的体力值，你获得此牌并明置之。",
	["qiren"] = "奇刃",
	[":qiren"] = "你使用非转化的暗置手牌（装备牌和延时锦囊牌除外）可如使用你的一张明置手牌般地选择目标角色。若如此做，你暗置该明牌。<font color=\"green\"><b>每阶段限一次。</b></font>",
	["#Qiren"] = "%from 发动“%arg2”，将如同使用 【%arg】一般地 使用暗置手牌（即使用目标和合法性检测视为此 【%arg】 ）。",
	["@qiren"] = "你发动“奇刃”对 %src 使用【借刀杀人】，请选择 %src 出杀的目标",
	
--************************************************	
	["yukimai"] = "雪 ＆ 舞",
	["&yukimai"] = "雪舞",
	["#yukimai"] = "黑白双子" ,
	["designer:yukimai"] = "dawda",

	["jinduan"] = "禁断",
	[":jinduan"] = "当一名角色于其回合外使用基本牌时，你可横置其装备区里所有竖牌并令其获得此基本牌。",
	["liuzhuan"] = "流转",
	[":liuzhuan"] = "当你或你攻击范围内的一名角色区域里的牌被明置或暗置、横置或竖置后，你可令其重铸其中任意张牌。",
	["liuzhuanuse"]= "“流转”：你可以重铸此次放置方式已改变后的任意张牌。",
	["~liuzhuanVS"]= "选择此次放置方式已改变后的任意张牌→点击“确定”。",
	
	["xunlun"] = "循轮",
	--[":xunlun"] = "当一名角色于其回合外使用基本牌时，你可明置其一张手牌，若与此基本牌颜色不同，其摸一张牌。",
	[":xunlun"] = "当一名角色于其出牌阶段外/出牌阶段内使用基本牌时，你可明置/暗置其一张手牌，若与之颜色相同/不同，其摸一张牌。",
	["chenjue"] = "尘绝",
	--[":chenjue"] = "其他角色的结束阶段开始时，若其手牌数等于你，其可令你与其各选择一张手牌，然后交给对方。以此法失去明牌的角色摸一张牌。",
	[":chenjue"] = "其他角色的弃牌阶段开始时，若其手牌数大于你，你可令其与你先各选择一张牌再同时交给对方，然后你与其各可使用非锦囊牌。",
    ["@chenjue"] = "请选择因“尘绝”效果 需要交给 %src 的手牌",
	["@chenjue_use"] = "因“尘绝”效果,你可以使用一张非锦囊牌",
}