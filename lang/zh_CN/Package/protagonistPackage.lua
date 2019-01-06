
return {

	["protagonist"] = "主角",
	["zhu"] = "主",

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

	["qixiang"] = "绮想",
	--[":qixiang"] = "当一名角色的判定牌生效后，若其手牌数小于你的体力上限，你可以令其摸一张牌。",
	[":qixiang"] = "当一名角色的红色判定牌生效后，你可以令其回复1点体力。",
	["qixiang:target"] = "<font color=\"#00FF00\"><b>%src </b></font> 的 “%dest” 的红色判定牌已生效，你可以对其发动“绮想”，令其回复1点体力。",

	["fengmo"] = "封魔",
	--[":fengmo"] = "当一名角色使用【闪】或【无懈可击】时，你可以令一名角色判定，若结果为红色，其于此回合内不能使用或打出除<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌外的牌。<font color=\"green\"><b>每阶段限一次。</b></font>",
	[":fengmo"] = "当一名角色于其回合外使用【闪】或【无懈可击】时，你可以弃置一张手牌，判定，若结果花色与你弃置的牌不同，当前回合角色选择一项：于此回合内不能使用非<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>的牌；或受到前者造成1点伤害。",
	["@fengmo"] = "你可以弃置一张手牌，发动“封魔”",
    ["fengmo:card"] = "使用牌将受到限制",
	["fengmo:damage"] = "受到使用者造成的伤害",

	
	["boli"] = "博丽",
	--[":boli"] = "<font color=\"orange\"><b>主公技，</b></font>当非<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>的判定牌生效前，你可以令其他角色选择是否打出<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌代替之。",
	["@boli-retrial"]= "你可以响应 <font color=\"#00FF00\"><b>%dest </b></font> 的主公技“博丽”，打出一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌，代替 <font color=\"#FF8000\"><b>%src </b></font> 的 “%arg” 判定牌。",
	["boli:judge"] = "<font color=\"#00FF00\"><b>%src </b></font> 的 “%dest” 判定结果不为<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>，你可以发动主公技“博丽”，令其他角色选择是否打出一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌代替之。",
	[":boli"] = "<font color=\"orange\"><b>主公技，</b></font>当非<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>的判定牌生效前，你可以令其他角色选择是否打出<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌替换之。",


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
	[":mofa"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以弃置一张手牌，令你于此回合内使用的【杀】或普通锦囊牌的伤害值基数+1，然后若此牌花色为<font size=\"5\", color=\"#808080\"><b>♠</b></font>，你摸一张牌。",
	["#mofa_notice"]= "由于 %arg 的效果，此回合内 %from 使用的【杀】或普通锦囊牌的伤害值基数+1。",
	["#mofa_damage"]= "%from 对 %to 的伤害由 %arg2 点增加到 %arg 点。",
	["#TouhouBuff"]= "%from 的 %arg 效果被触发。",
	["wuyu"] = "雾雨",
	[":wuyu"] = "<font color=\"orange\"><b>主公技，</b></font><font color=\"green\"><b>其他角色的出牌阶段限一次，</b></font>其可以将一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌交给你。",
	[":wuyu_attach"]= "出牌阶段，你可以将一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌交给拥有主公技“雾雨”的角色。每阶段限一次。",
	["wuyu_attach"]= "雾雨送牌",

--************************************************
	--zhu003  势力：主 4血
	["reimu_sp"] = "SP无节操灵梦",
	["#reimu_sp"] = "十万巫女",
	["&reimu_sp"] = "无节操灵梦",
	["designer:reimu_sp"] = "星野梦美☆",

	["illustrator:reimu_sp"] = "唯",
	["origin:reimu_sp"] = "p号：35756906，个人ID：230943",
	["illustrator:reimu_sp_1"] = "あめろ",
	["origin:reimu_sp_1"] = "p号：31824198，个人ID：37336",
	["illustrator:reimu_sp_2"] = "しがらき",
	["origin:reimu_sp_2"] = "p号：19597658，个人ID：1004274",


	["saiqian"] = "赛钱",
	[":saiqian"] = "<font color=\"green\"><b>其他角色的出牌阶段限一次，</b></font>其可以将一至三张手牌交给你，若如此做，你可以选择一至两项：1.弃置一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌；2.失去1点体力。每当你选择一项后，其回复1点体力。",
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
	[":jiezou"] = "出牌阶段，你可以获得一名其他角色区域里的一张牌，若如此做，你选择一项：弃置一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌；或失去1点体力并结束此阶段。",
	["@jiezou_spadecard"]= "请弃置一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，否则你将失去1点体力并结束出牌阶段。",
	["jiezou_skip"]= "出牌阶段",
	["#jiezou_skip"]= "由于 %arg2 的效果，%from 被强制结束了 %arg。",
	["shoucang"] = "收藏",
	[":shoucang"] = "弃牌阶段开始时，你可以展示至少一张花色各不相同的手牌，令你的手牌上限于此回合内+X（X为你以此法展示的牌数）。",
	["#shoucang_max"]= "%from的手牌上限于此回合内+%arg2",
	["@shoucang"] = "你可以发动“收藏”，展示至少一张花色各不相同的手牌。",
	["~shoucang"]= "选择要展示的手牌 -> 确定。",
	["#shoucang"] = "收藏（手牌上限）",

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
	[":baoyi"] = "准备阶段开始时，你可以弃置任意数量的装备牌和判定区里的所有牌（至少弃置一张牌，但判定区里的牌数可以为零），你每以此法弃置一张牌，你可以视为对一名其他角色使用【杀】，最后若你以此法弃置过<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，你摸两张牌。",
	["@baoyi"] = "你可以发动“爆衣”，弃置任意数量的装备牌并弃置判定区里的所有牌（至少弃置一张牌；即判定区里可以没有牌可弃置，但要弃置装备牌，反之可以不弃置装备牌，弃置判定区里的牌）。",
	["~baoyi"] = "一共弃置的牌数等于之后你可以依次视为使用的【杀】的数量。",
	["@@baoyi_chosen"] = "你可以视为使用【杀】。<br />提示：你还能再视为使用 <font color=\"#00FF00\"><b>%src </b></font> 张【杀】。",
	["baoyi:drawcard"] = "由于你发动“爆衣”时，弃置过<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，你摸两张牌。" ,

--************************************************
  --zhu006  势力：主 4血
	["reimu_yym"]= "妖妖梦SP灵梦",
	["&reimu_yym"]= "妖妖梦灵梦",
	["#reimu_yym"]= "春巫女",
	["designer:reimu_yym"] = "星野梦美☆",

	["illustrator:reimu_yym"] = "萩原",
	["origin:reimu_yym"] = "p号：16215731，个人ID：37062",
	["illustrator:reimu_yym_1"] = "みゃけ",
	["origin:reimu_yym_1"] = "p号：9705615，个人ID：60103",
	["illustrator:reimu_yym_2"] = "かがよ",
	["origin:reimu_yym_2"] = "p号：9056340，个人ID：693172",

	["zhize"]= "职责",
	[":zhize"]= "<font color=\"blue\"><b>锁定技，</b></font>摸牌阶段开始时，你选择一项：失去1点体力并跳过出牌阶段；或放弃摸牌并观看有手牌的一名其他角色的手牌并可以获得其中的一张牌。",
	["@@zhize"]= "请选择有手牌的一名其他角色，观看其手牌并可以获得其中的一张牌，否则你将失去1点体力并跳过此回合内的出牌阶段。",
	["chunxi"]= "春息",
	[":chunxi"]= "当你获得牌后，你可以展示其中至少一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，你每以此法展示一张牌，你可以获得一名其他角色的一张手牌。",
	["@chunxi"]= "你可以发动“春息”，展示你获得的至少一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌",
	["~chunxi"]= "选择此次获得的至少一张红桃牌 -> 确定",
	["chunxi-target"]= "此次“春息”展示了 %dest 张牌。你可以选择一名有手牌的其他角色并获得其一张手牌。<br />此次选择是其中的第 %src 次。",
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
	[":bllmwuyu"] = "准备阶段开始时，你可以将“欲”标记补至X枚（X为你已损失的体力值+1）；你可以弃1枚“欲”标记或弃置一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，发动下列中的一项技能：<br /><font color=\"orange\"><b>“名欲”</b></font>（你跳过判定阶段）。<br /><font color=\"orange\"><b>“财欲”</b></font>（摸牌阶段开始时，你摸一张牌）。<br /><font color=\"orange\"><b>“色欲”</b></font>（出牌阶段，你于此回合内使用【杀】的额外次数上限+1）。<br /><font color=\"orange\"><b>“睡欲”</b></font>（弃牌阶段开始时，你的手牌上限于此阶段内视为4）。<br /><font color=\"orange\"><b>“食欲”</b></font>（你可以将一张手牌当【酒】使用）。" ,
	["bllmcaiyu"] = "财欲" ,
	["#bllmcaiyu"] = "财欲" ,
	["bllmseyu"] = "色欲" ,
	["#bllmseyu"] = "色欲" ,
	["bllmmingyu"] = "名欲" ,
	["#bllmmingyu"] = "名欲" ,
	["bllmshiyu"] = "食欲" ,
	["#bllmshiyu"] = "食欲" ,
	["bllmshuiyu"] = "睡欲" ,
	["#bllmshuiyu"] = "睡欲" ,
	["#bllmshuiyu2"] = "睡欲" ,
	["@bllm-discard"] = "你可以弃置一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，发动“%src”。" ,
	["bllmwuyu:useyu"]= "你可以弃 1 枚“欲”标记，发动“%src”。" ,
	["bllmwuyu:dismiss"] = "取消" ,
	["@bllmshiyu-basics"] = "“食欲”：你可以将一张手牌当【酒】使用。" ,
	["~bllmwuyu"] = "选择一张手牌 -> 确定" ,
	["@yu"] = "欲" ,

--************************************************
	--zhu008  势力：主 3血
	["marisa_slm"] = "神灵庙SP魔理沙",
	["&marisa_slm"] = "神灵庙魔理沙",
	["#marisa_slm"] = "强欲的魔法使",
	["designer:marisa_slm"] = "风的呓语",

	["illustrator:marisa_slm"] = "まるかた",
	["origin:marisa_slm"] = "p号：8831829，个人ID：6359",
	["illustrator:marisa_slm_1"] = "まるかた",
	["origin:marisa_slm_1"] = "p号：3168979，个人ID：6359",
	["illustrator:marisa_slm_2"] = "れい亜",
	["origin:marisa_slm_2"] = "p号：36286762，个人ID：444732",

	["qiangyu"] = "强欲",
	[":qiangyu"] = "当你从牌堆顶摸牌时，你可以多摸两张牌，然后选择一项：弃置两张手牌；或弃置一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>手牌。",
	["qiangyu-discard"]= "请弃置一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>手牌，否则请弃置两张手牌",
	["~qiangyu"]= "选择一张黑桃手牌 或 两张手牌 -> 确定",
	["mokai"] = "魔开",
	[":mokai"] = "当你于出牌阶段内使用黑色锦囊牌时，你可以弃置一张装备牌，摸两张牌。每阶段限X次（X为你的体力值且至少为1）。",
	--[":mokai"] = "当你使用一张锦囊牌时，你可以将一张装备牌置于人物牌上，称为“天仪”。然后若“天仪”数大于你的体力值，你可以将一张“天仪”置入弃牌堆。",
	--[":mokai"] = "你于出牌阶段使用锦囊牌时，你可以横置装备区的一张牌并跳过结束阶段。你失去装备区的横置牌时，你摸两张牌。",

	["@mokai"] = "魔开：你可以弃置一张装备牌，摸两张牌",
	["#mokai_count"] = "本阶段 %from 第 %arg2 次发动 %arg",

--************************************************
	--zhu009  势力：主 4血
	["sanae_slm"] = "神灵庙SP早苗",
	["&sanae_slm"] = "神灵庙早苗",
	["#sanae_slm"] = "私欲的巫女",
	["designer:sanae_slm"] = "星野梦美☆",

	["illustrator:sanae_slm"] = "An2A",
	["origin:sanae_slm"] = "p号：27724528，个人ID：173876",
	["illustrator:sanae_slm_1"] = "小強",
	["origin:sanae_slm_1"] = "p号：19757581，个人ID：1754781",
	["illustrator:sanae_slm_2"] = "木shiyo",
	["origin:sanae_slm_2"] = "p号：34431982，个人ID：40222",

	["yuwang_list"] = "欲望",
	["dfgzmsiyu"] = "私欲",
	[":dfgzmsiyu"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以将一张手牌交给一名其他角色。若如此做，此回合结束时，你观看其手牌并获得其中的一张牌。",
	["qishu"] = "奇术",
	[":qishu"] = "你使用最后的手牌无距离限制；出牌阶段内，若你使用的【杀】或普通锦囊牌是最后的手牌，你使用此牌的额外目标数无上限。",
	["~qishu"] = "第一个目标为持有武器的角色，第二个目标为出杀的目标。",
	["@qishu-add"] = "请选择【%arg】的额外目标",
	["#QishuAdd"] = "%from 发动了“%arg2”为【%arg】增加了额外目标 %to",
	["#qishu-mod"] = "奇术",

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
	[":hpymsiyu"] = "<font color=\"blue\"><b>锁定技，</b></font>当你于其他角色回合内首次进入濒死状态前，你将人物牌翻至正面朝上，然后将你判定区里的所有牌置入弃牌堆，终止一切结算，结束当前回合，获得一个额外的回合。此额外回合结束时，若你的体力值小于体力下限，你进入濒死状态。<br /><br />" ..
					"♦此技能效果可以突破无法多重执行额外回合效果的限制进行额外回合。",
	["juhe"] = "居合",
	[":juhe"] = "摸牌阶段，你可以多摸三张牌，然后你弃置X张手牌（X为你的体力值）。",
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
	[":yinyang"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以选择有手牌的一名其他角色，令其弃置一张手牌，然后你弃置一张手牌，最后若这两张牌的颜色：不同，其摸一张牌，然后其回复1点体力；相同，你摸两张牌，然后失去1点体力。" ,
	["@yinyang_discard"] = "因“阴阳”的效果，你须弃置一张手牌",
	["lingji"] = "灵击" ,
	[":lingji"] = "结束阶段开始时，若你于此回合内因使用或打出或弃置而失去过<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，你可以对一名其他角色造成1点伤害。" ,
	["@lingji"] = "你可以发动“灵击”，对一名其他角色造成1点伤害",


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
	[":toushi"] = "其他角色的出牌阶段结束时，若其于此阶段内使用的最后的牌为【杀】或普通锦囊牌，你可以将一张基本牌或<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌当与之牌名、属性、效果均相同的牌使用。" ,
	["@toushi"]= "你可以发动“偷师”，将一张 基本牌 或 <font size=\"5\", color=\"#808080\"><b>♠</b></font> 牌当做【%src】使用。",
	["~toushi"]= "选择基本牌或黑桃牌并选择此转化牌的目标 -> 确定",
	["moli"] = "魔力" ,
	[":moli"] = "一名角色的结束阶段开始时，若你于此回合内造成与受到过的伤害值之和大于1点，你可以令一名角色回复1点体力。",
	["@moli"]= "你可以发动“魔力”，令一名角色回复1点体力",


--***********************************
	--zhu013  势力：主 4血
	["reisen_gzz"] = "绀珠传SP铃仙",
	["&reisen_gzz"] = "绀珠传铃仙",
	["#reisen_gzz"] = "地上的月兔",
	["designer:reisen_gzz"] = "蔚海幽浮",
	
	["illustrator:reisen_gzz"] = "まさる.jp",
	["origin:reisen_gzz"] = "p号：59883222，个人ID：6547201",
	
	["bodong"] = "波动" ,
	[":bodong"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以弃置一张手牌并选择场上一至三张装备牌，依次横置之。",
	["huanlong"] = "幻胧",
	[":huanlong"] = "当一名角色受到或造成伤害后，你可以重置其装备区里的一张横牌，摸一张牌。",

}
