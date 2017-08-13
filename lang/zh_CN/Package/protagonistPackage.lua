
return {

	["protagonist"] = "主角",
	["zhu"] = "主",
	--["lord_declaration"]= "主公登场",

	--命名范例
	--["reimu"] 人物名 可能是略称
	--["#reimu"] 称号
	--["&reimu"] 人物头像的显示名
	--["!reimu"] 人物全名


	--zhu001 势力：主 4血
	["reimu"] = "博丽灵梦",
	["#reimu"] = "乐园的美妙巫女",
	["designer:reimu"] = "星野梦美☆",

	["illustrator:reimu"] = "tearfish",
	["origin:reimu"] = "p号：5228650",
	["illustrator:reimu_1"] = "蒼空キズナ",
	["origin:reimu_1"] = "p号：4510083",
	["illustrator:reimu_2"] = "ファルまろ",
	["origin:reimu_2"] = "p号：34912115",
	["illustrator:reimu_3"] = "みや",
	["origin:reimu_3"] = "p号：22911671",
	["illustrator:reimu_4"] = "pico",
	["origin:reimu_4"] = "p号：20132493",

	["lingqi"] = "灵气",
	[":lingqi"] = "当你成为【杀】或普通锦囊牌的目标后，你可以判定，若结果为<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>，此牌对你无效。",
	["lingqi:target"] = " <font color=\"#00FF00\"><b>%src </b></font> 使用【%dest】指定了你为目标，你可以发动“灵气”。",
	["~lingqi"]= "选择一张牌→指定任意数量的角色→点击“确定” 或者取消发动",
	["#LingqiAvoid"] = "“%arg2”效果被触发，【%arg】 对 %from 无效",
	["qixiang"] = "绮想",
	[":qixiang"] = "当一名角色的判定牌生效后，若其手牌数小于你的体力上限，你可令其摸一张牌。",
	["qixiang:target"] = "<font color=\"#00FF00\"><b>%src </b></font> 的 “%dest” 的判定牌已生效，你可以对其发动“绮想”，令其摸一牌。",
	["boli"] = "博丽",
	[":boli"] = "<font color=\"orange\"><b>主公技，</b></font>当非<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>的判定牌生效前，你可令其他角色选择是否打出<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌代替之。",
	["@boli-retrial"]= "你可以响应 <font color=\"#00FF00\"><b>%dest </b></font> 的主公技“博丽”，打出一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌，代替 <font color=\"#FF8000\"><b>%src </b></font> 的 “%arg” 判定。",
	["boli:judge"] = "<font color=\"#00FF00\"><b>%src </b></font> 的 “%dest” 判定结果不为<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>，你可以发动主公技“博丽”令其他角色选择是否打出一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌代替之。",


--************************************************
	--zhu002 势力：主 4血
	["marisa"] = "雾雨魔理沙",
	["#marisa"] = "普通的魔法使",
	["designer:marisa"] = "星野梦美☆",

	["illustrator:marisa"] = "An2A",
	["origin:marisa"] = "个人ID：173876",
	["illustrator:marisa_1"] = "えふぇ",
	["origin:marisa_1"] = "p号：34689497，个人ID：292644",
	["illustrator:marisa_2"] = "zen",
	["origin:marisa_2"] = "p号：40523568，个人ID：31564",
	["illustrator:marisa_3"] = "An2A",
	["origin:marisa_3"] = "个人ID：173876",
	["illustrator:marisa_4"] = "shinia",
	["origin:marisa_4"] = "p号：20443348，个人ID：44778",

	["mofa"] = "魔法",
	[":mofa"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以弃置一张手牌，若如此做，你于此回合内使用的【杀】或普通锦囊牌造成的伤害+1。若以此法弃置了<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，你摸一张牌。",
	["#mofa_notice"]= "由于 %arg 的效果，此回合内 %from 使用的牌造成的伤害+1。",
	["#mofa_damage"]= "%from 对 %to 的伤害由 %arg2 点增加到 %arg 点。",
	["#TouhouBuff"]= "%from 的 %arg 效果被触发。",
	["wuyu"] = "雾雨",
	[":wuyu"] = "<font color=\"orange\"><b>主公技，</b></font><font color=\"green\"><b>其他角色的出牌阶段限一次，</b></font>其可以将一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>手牌交给你。",
	[":wuyu_attach"]= "出牌阶段，你可以将一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>手牌交给拥有主公技“雾雨”的角色。每阶段限一次。",
	["wuyu_attach"]= "雾雨送牌",

--************************************************
	--zhu003  势力：主 4血
	["reimu_sp"] = "SP无节操灵梦",
	["#reimu_sp"] = "十万巫女",
	["&reimu_sp"] = "无节操灵梦",
	["designer:reimu_sp"] = "星野梦美☆",

	["illustrator:reimu_sp"] = "しがらき",
	["origin:reimu_sp"] = "p号：19597658，个人ID：1004274",
	["illustrator:reimu_sp_1"] = "あめろ",
	["origin:reimu_sp_1"] = "p号：31824198，个人ID：37336",
	["illustrator:reimu_sp_2"] = "唯",
	["origin:reimu_sp_2"] = "p号：35756906，个人ID：230943",

	["saiqian"] = "赛钱",
	[":saiqian"] = "<font color=\"green\"><b>其他角色的出牌阶段限一次，</b></font>其可以将一至三张手牌交给你，然后你可以选择至多两项：弃置一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌，或失去1点体力。你每选择一项，该角色回复1点体力。",
	["saiqian_attach"] = "赛钱送牌",
	[":saiqian_attach"] = "你可以将一至三张手牌交给拥有“赛钱”的角色。",
	["losehp_saiqian"]= "失去1点体力，该角色回复1点体力",
	["discard_saiqian"]= "弃置一张红桃手牌，该角色回复1点体力",
	["cancel_saiqian"]= "什么都不做",
	["#saiqian_lose"]= "%from 执行了 “%arg” 的效果",
	["@saiqian-discard"]= "你可以弃置一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌，令 <font color=\"#00FF00\"><b>%src </b></font> 回复1点体力。",

--************************************************
	--zhu004  势力：主 3血
	["marisa_sp"] = "SP大盗魔理沙",
	["&marisa_sp"] = "大盗魔理沙",
	["#marisa_sp"] = "大盗",
	["designer:marisa_sp"] = "星野梦美☆",

	["illustrator:marisa_sp"] = "seeker",
	["origin:marisa_sp"] = "p号：13164043，个人ID：694807",
	["illustrator:marisa_sp_1"] = "NEKO",
	["origin:marisa_sp_1"] = "p号：42655610，个人ID：2600911",
	["illustrator:marisa_sp_2"] = "夢職人",
	["origin:marisa_sp_2"] = "p号：15273460，个人ID：274333",

	["jiezou"] = "借走",
	[":jiezou"] = "出牌阶段，你可以获得其他角色区域里的一张牌，然后选择一项：弃置一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，或失去1点体力并结束此阶段。",
	["@jiezou_spadecard"]= "请弃置一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，否则你将失去1点体力并结束出牌阶段。",
	["jiezou_skip"]= "出牌阶段",
	["#jiezou_skip"]= "由于 %arg2 的效果，%from 被强制结束了 %arg。",
	["shoucang"] = "收藏",
	[":shoucang"] = "弃牌阶段开始时，你可以展示X张花色各不相同的手牌，若如此做，你的手牌上限于此回合内+X。",
	["#shoucang_max"]= "本回合，%from的手牌上限+%arg2",
	["@shoucang"] = "弃牌阶段，你可以发动“收藏”，展示任意数量的花色各不相同的手牌。",
	["~shoucang"]= "选择展示的手牌→点击“确定”。",

--************************************************
 --zhu005  势力：主 4血
	["marisa_sp2"] = "SP超魔理沙",
	["&marisa_sp2"] = "超魔理沙",
	["#marisa_sp2"] = "超·恋之魔女",
	["designer:marisa_sp2"] = "星野梦美☆",

	["illustrator:marisa_sp2"] = "AUER",
	["origin:marisa_sp2"] = "p号：1150651，个人ID：178301",
	["illustrator:marisa_sp2_1"] = "御月ユウヤ",
	["origin:marisa_sp2_1"] = "p号：13678531，个人ID：4971",
	["illustrator:marisa_sp2_2"] = "御月ユウヤ",
	["origin:marisa_sp2_2"] = "p号：750367，个人ID：4971",

	["baoyi"] = "爆衣",
	[":baoyi"] = "准备阶段开始时，你可以弃置任意数量的装备牌和你判定区里的所有牌（总计至少一张）。每以此法弃置一张牌，你可以视为对一名角色使用【杀】。然后若以此法弃置过<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，你摸两张牌。",
	["@baoyi"] = "你可以发动“爆衣”，弃置任意数量的装备牌并弃置判定区里的所有牌。",
	["~baoyi"] = "弃置X张可以视为使用X张【杀】。",
	["@@baoyi_chosen"] = "你可以指定一名其他角色，视为对其使用【杀】。提示：你还可以视为使用 <font color=\"#00FF00\"><b>%src </b></font> 张【杀】。",
	["baoyi:drawcard"] = "由于你发动“爆衣”时弃置了<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，你可以摸两张牌。" ,

--************************************************
  --zhu006  势力：主 4血
	["reimu_yym"]= "妖妖梦SP灵梦",
	["&reimu_yym"]= "妖妖梦灵梦",
	["#reimu_yym"]= "春巫女",
	["designer:reimu_yym"] = "星野梦美☆",

	["illustrator:reimu_yym"] = "みゃけ",
	["origin:reimu_yym"] = "p号：9705615，个人ID：60103",
	["illustrator:reimu_yym_1"] = "萩原",
	["origin:reimu_yym_1"] = "p号：16215731，个人ID：37062",
	["illustrator:reimu_yym_2"] = "かがよ",
	["origin:reimu_yym_2"] = "p号：9056340，个人ID：693172",

	["zhize"]= "职责",
	[":zhize"]= "<font color=\"blue\"><b>锁定技，</b></font>摸牌阶段开始时，你选择一项：失去1点体力并跳过此回合的出牌阶段；或放弃摸牌，改为观看一名有手牌的其他角色的手牌，你可以获得其中的一张。",
	["@@zhize"]= "请指定一名其他角色，观看其手牌并可以获得其中一张，否则你将失去1点体力并跳过此回合的出牌阶段。",
	["chunxi"]= "春息",
	[":chunxi"]= "当你获得牌后，你可以展示其中至少一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌。每以此法展示一张牌，你可以获得一名其他角色的一张手牌。",
	["@chunxi"]= "你可以，发动“春息”，展示你获得的至少一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌",
	["~chunxi"]= "选择任意张此次获得的红桃牌 -> 确定",
	["chunxi-target"]= "本次春息展示了 %dest 张牌。现在你可以选择一名角色获得其一张手牌。 本次选择是其中的第 %src 次。",
--************************************************
	--zhu007  势力：主 4血
	["reimu_slm"] = "神灵庙SP灵梦" ,
	["#reimu_slm"] = "五欲的巫女" ,
	["&reimu_slm"] = "神灵庙灵梦" ,
	["designer:reimu_slm"] = "星野梦美☆",

	["illustrator:reimu_slm"] = "藤原",
	["origin:reimu_slm"] = "p号：51160511，个人ID：27517",
	["illustrator:reimu_slm_1"] = "ぬぬっこ",
	["origin:reimu_slm_1"] = "p号：24429823，个人ID：1030312",
	["illustrator:reimu_slm_2"] = "青芝クレハ_",
	["origin:reimu_slm_2"] = "p号：13295146，个人ID：49165",
	["illustrator:reimu_slm_3"] = "きんたろ",
	["origin:reimu_slm_3"] = "p号：18697741，个人ID：10009",

	["bllmwuyu"] = "五欲" ,
	[":bllmwuyu"] = "准备阶段开始时，你可以将“欲”标记补至X+1枚（X为你已损失的体力值）。你可以弃一枚“欲”标记或一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，发动下列技能中的一个：<font color=\"blue\"><b>“名欲”</b></font>（你可以跳过判定阶段）<font color=\"blue\"><b>“财欲”</b></font>（摸牌阶段开始时，你可以摸一张牌）<font color=\"blue\"><b>“色欲”</b></font>（出牌阶段，你使用【杀】的额外次数上限+1）<font color=\"blue\"><b>“睡欲”</b></font>（弃牌阶段，你的手牌上限视为4）<font color=\"blue\"><b>“食欲”</b></font>（你可以将一张手牌当【酒】使用）" ,
	["bllmcaiyu"] = "财欲" ,
	["bllmseyu"] = "色欲" ,
	["bllmmingyu"] = "名欲" ,
	["bllmshiyu"] = "食欲" ,
	["bllmshuiyu"] = "睡欲" ,
	["#bllmshuiyu"] = "睡欲" ,
	["@bllm-discard"] = "你可以弃置一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，发动“%src”。" ,
	["bllmwuyu:useyu"]= "你可以弃一枚“欲”标记，发动“%src”。" ,
	["bllmwuyu:dismiss"] = "取消" ,
	["@bllmshiyu-basics"] = "“食欲”：你可以将一张手牌当【酒】使用。" ,
	["~bllmwuyu"] = "选择一张手牌→点击“确定”。" ,
	["@yu"] = "欲" ,

--************************************************
	--zhu008  势力：主 3血
	["marisa_slm"] = "神灵庙SP魔理沙",
	["&marisa_slm"] = "神灵庙魔理沙",
	["#marisa_slm"] = "强欲的魔法使",
	["designer:marisa_slm"] = "星野梦美☆",

	["illustrator:marisa_slm"] = "まるかた",
	["origin:marisa_slm"] = "p号：8831829，个人ID：6359",
	["illustrator:marisa_slm_1"] = "まるかた",
	["origin:marisa_slm_1"] = "p号：3168979，个人ID：6359",
	["illustrator:marisa_slm_2"] = "れい亜",
	["origin:marisa_slm_2"] = "p号：36286762，个人ID：444732",

	["qiangyu"] = "强欲",
	[":qiangyu"] = "当你从牌堆顶摸牌时，你可以多摸两张牌，然后选择一项：弃置两张手牌，或弃置一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>手牌。",
	["qiangyu-discard"]= "请弃置一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>手牌，否则弃置两张手牌",
	["~qiangyu"]= "选择一张黑桃牌 或 两张手牌 -> 确定",
	["mokai"] = "魔开",
	[":mokai"] = "当你使用一张锦囊牌时，你可以将一张装备牌置于人物牌上，称为“天仪”。然后若“天仪”数大于你的体力值，你可以将一张“天仪”置入弃牌堆。",
	["tianyi"]= "天仪",
	["@mokai"] = "你可以将一张装备牌作为“天仪”置于人物牌上",
	["@mokai-dis"] = "你可以将一张“天仪”置入弃牌堆",
	["~mokai"] = "选择一张“天仪” -> 确定" ,
	["guangji"] = "光击",
	[":guangji"] = "当你成为【杀】的目标后，你可以将一张“天仪”置入弃牌堆，令此【杀】对你无效。",
	["@guangji-invoke"] = "<font color=\"#00FF00\"><b>%src </b></font>使用【杀】指定了你为目标，你可以发动“光击”。" ,
	["~guangji"] = "选择一张“天仪” -> 确定" ,
	["xinghui"] = "星辉",
	[":xinghui"] = "当一张“天仪”从你的人物牌上离开时，你可以摸一张牌。",



--************************************************
	--zhu009  势力：主 4血
	["sanae_slm"] = "神灵庙SP早苗",
	["&sanae_slm"] = "神灵庙早苗",
	["#sanae_slm"] = "私欲的巫女",
	["designer:sanae_slm"] = "星野梦美☆",

	["illustrator:sanae_slm"] = "小強",
	["origin:sanae_slm"] = "p号：19757581，个人ID：1754781",
	["illustrator:sanae_slm_1"] = "An2A",
	["origin:sanae_slm_1"] = "p号：27724528，个人ID：173876",
	["illustrator:sanae_slm_2"] = "木shiyo",
	["origin:sanae_slm_2"] = "p号：34431982，个人ID：40222",

	["yuwang_list"] = "欲望",
	["dfgzmsiyu"] = "私欲",
	[":dfgzmsiyu"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以将一张手牌交给一名其他角色。此回合结束时，你观看其手牌并获得其中的一张。",
	["qishu"] = "奇术",
	[":qishu"] = "你使用最后的手牌时无距离限制。出牌阶段，当你对其他角色使用【杀】或普通锦囊牌时，若此牌是你最后的手牌，你可以额外指定任意数量的其他角色为目标。",
	["~qishu"] = "第一个目标为持有武器的角色，第二个目标为出杀的目标。",
	["@qishu-add"] = "请选择【%arg】的额外目标",
	["#QishuAdd"] = "%from 发动了“%arg2”为【%arg】增加了额外目标 %to",

--***********************************
	--zhu010  势力：主 2血
	["youmu_slm"] = "神灵庙SP妖梦",
	["&youmu_slm"] = "神灵庙妖梦",
	["#youmu_slm"] = "死欲的半灵",
	["designer:youmu_slm"] = "星野梦美☆",

	["illustrator:youmu_slm"] = "alcd",
	["origin:youmu_slm"] = "p号：14362406，个人ID：2334059",
	["illustrator:youmu_slm_1"] = "竜徹",
	["origin:youmu_slm_1"] = "p号：5976425，个人ID：63354",
	["illustrator:youmu_slm_2"] = "さざなみみぉ",
	["origin:youmu_slm_2"] = "p号：13635722，个人ID：1092517",
	["illustrator:youmu_slm_3"] = " 60枚 ",
	["origin:youmu_slm_3"] = "p号：54120760，个人ID：3322006",

	["hpymsiyu"] = "死欲",
	[":hpymsiyu"] = "<font color=\"blue\"><b>锁定技，</b></font>当你于回合外进入濒死状态前，你翻至正面朝上，将你判定区里的牌置入弃牌堆，终止一切结算，当前回合结束，然后你进行一个额外回合。此额外回合结束时，若你的体力值小于体力下限，你进入濒死状态。<br /><br />" ..
					"♦此技能可以突破无法多重执行额外回合效果的限制进行额外回合。",
	["juhe"] = "居合",
	[":juhe"] = "摸牌阶段摸牌时，你可以多摸三张牌，然后弃置X张手牌（X为你的体力值）。",
	["juhe_discard"]= "“居合”：请弃置 %src 张手牌",

--***********************************
	--zhu011  势力：主 4血
	["reimu_old"] = "旧作SP灵梦" ,
	["#reimu_old"] = "维护梦与传统的巫女" ,
	["&reimu_old"] = "旧作灵梦" ,
	["designer:reimu_old"] = "圆神狂热信徒",

	["illustrator:reimu_old"] = "november",
	["origin:reimu_old"] = "p号：31984422，个人ID：1979063",
	["illustrator:reimu_old_1"] = "くろぬこネーロ",
	["origin:reimu_old_1"] = "p号：32920520，个人ID：335493",


	["yinyang"] = "阴阳" ,
	[":yinyang"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可令一名其他角色弃置一张手牌，然后你弃置一张手牌。若以此法弃置了两张颜色不同的牌，其摸一张牌然后回复1点体力。若以此法弃置了两张颜色相同的牌，你摸两张牌然后失去1点体力。" ,
	["@yinyang_discard"] = "因“阴阳”效果， 你须弃置一张手牌",
	["lingji"] = "灵击" ,
	[":lingji"] = "结束阶段开始时，若你于本回合内使用、打出或因弃置过<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，你可以对一名其他角色造成一点伤害。" ,
	["@lingji"] = "你可以发动“灵击”对一名其他角色造成一点伤害",


--***********************************
	--zhu012  势力：主 4血
	["marisa_old"] = "旧作SP魔理沙" ,
	["#marisa_old"] = "魔法与红梦化成的存在" ,
	["&marisa_old"] = "旧作魔理沙" ,
	["designer:marisa_old"] = "圆神狂热信徒",

	["illustrator:marisa_old"] = "fancybetty",
	["origin:marisa_old"] = "p号：，个人ID：",
	["illustrator:marisa_old_1"] = "カタケイ",
	["origin:marisa_old_1"] = "p号：50400667，个人ID：90042",

	["toushi"] = "偷师" ,
	[":toushi"] = "其他角色的出牌阶段结束时，若其于此阶段内使用的最后的牌为【杀】或普通锦囊牌，你可以将一张基本牌或<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌当与此牌名称相同且属性相同的牌使用。。" ,
	["@toushi"]= "你可以发动“偷师”，视为你使用将一张 基本牌 或<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌当做 【%src】使用。",
	["~toushi"]= "若此牌需要指定目标，你先指定目标，再确定。",
	["moli"] = "魔力" ,
	[":moli"] = "一名角色的结束阶段开始时，若你于此回合内造成和受到过的伤害之和不少于2点，你可令一名角色回复1点体力。",
	["@moli"]= "你可以发动“魔力”，令一名角色回复一点体力",
	
	
--***********************************
	--zhu013  势力：主 4血
	["reisen_gzz"] = "绀珠传SP铃仙",
	["&reisen_gzz"] = "绀珠传铃仙",
	["#reisen_gzz"] = "地上的月兔",
	["designer:reisen_gzz"] = "蔚海幽浮",
	["bodong"] = "波动" ,
	[":bodong"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以弃置一张手牌，横置场上至多三张装备牌。",
    ["huanlong"] = "幻胧",
	[":huanlong"] = "当装备区有横置牌的角色受到或造成伤害后，你可以重置其一张装备牌，然后弃置其一张手牌。",
	
 }