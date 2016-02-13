
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
	--["designer:reimu"] = "星野梦美 | 程序:三国有单",
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
	[":lingqi"] = "当你成为【杀】或非延时类锦囊牌的目标时，你可以判定，若结果为<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>，此牌对你无效。",
	["lingqi:target"] = " <font color=\"#00FF00\"><b>%src </b></font> 使用【%dest】指定了你为目标，你可以发动“灵气”。",
	["~lingqi"]= "选择一张牌→指定任意数量的角色→点击“确定” 或者取消发动",
	["#LingqiAvoid"] = "“%arg2”效果被触发，【%arg】 对 %from 无效",
	["qixiang"] = "绮想",
	[":qixiang"] = "当一名角色的判定牌生效后，若该角色的手牌数小于你的体力上限，你可令该角色摸一张牌。",
	["qixiang:target"] = "<font color=\"#00FF00\"><b>%src </b></font> 的 “%dest” 的判定牌已生效，你可以对其发动“绮想”，令其摸一牌。",
	["boli"] = "博丽",
	[":boli"] = "<font color=\"orange\"><b>主公技，</b></font>当非<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>的判定牌生效前，你可令其他角色选择是否打出<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌代替之。",
	["@boli-retrial"]= "你可以响应 <font color=\"#00FF00\"><b>%dest </b></font> 的主公技“博丽”，打出一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌，代替 <font color=\"#FF8000\"><b>%src </b></font> 的 “%arg” 判定。",
	["boli:judge"] = "<font color=\"#00FF00\"><b>%src </b></font> 的 “%dest” 判定结果不为<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>，你可以发动主公技“博丽”令其他角色选择是否打出一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌代替之。",


--************************************************
	--zhu002 势力：主 4血
	["marisa"] = "雾雨魔理沙",
	["#marisa"] = "普通的魔法使",
	["illustrator:marisa"] = "An2A",
	["origin:marisa"] = "个人ID：173876",
	["illustrator:marisa_1"] = "えふぇ",
	["origin:marisa_1"] = "p号：34689497，个人ID：292644", --（作者说明非合同绘 非商业可用）
	["illustrator:marisa_2"] = "zen",
	["origin:marisa_2"] = "p号：40523568，个人ID：31564",
	["illustrator:marisa_3"] = "An2A",
	["origin:marisa_3"] = "个人ID：173876",
	["illustrator:marisa_4"] = "shinia",
	["origin:marisa_4"] = "p号：20443348，个人ID：44778",
		["mofa"] = "魔法",
	[":mofa"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以弃置一张手牌，若如此做，你于此回合内使用的牌造成的伤害+1。若以此法弃置了<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，你摸一张牌。",
	["#mofa_notice"]= "由于 %arg 的效果，此回合内 %from 使用的牌造成的伤害+1。",
	["#mofa_damage"]= "%from 对 %to 的伤害由 %arg2 点增加到 %arg 点。",
	["#TouhouBuff"]= "%from 的 %arg 效果被触发。",
	["wuyu"] = "雾雨",
	[":wuyu"] = "<font color=\"orange\"><b>主公技，</b></font><font color=\"green\"><b>其他角色的出牌阶段限一次，</b></font>该角色可以将一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>手牌交给你。",
	[":wuyu_attach"]= "出牌阶段，你可以将一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>手牌交给拥有主公技“雾雨”的角色。每阶段限一次。",
	["wuyu_attach"]= "雾雨送牌",

--************************************************
	--zhu003  势力：主 4血
    ["reimu_sp"] = "SP无节操灵梦", 
	["#reimu_sp"] = "十万巫女",
    ["&reimu_sp"] = "无节操灵梦",
	["illustrator:reimu_sp"] = "しがらき",
	["origin:reimu_sp"] = "p号：19597658，个人ID：1004274",
	["illustrator:reimu_sp_1"] = "あめろ",
	["origin:reimu_sp_1"] = "p号：31824198，个人ID：37336",
	["illustrator:reimu_sp_2"] = "唯",
	["origin:reimu_sp_2"] = "p号：35756906，个人ID：230943",
		["saiqian"] = "赛钱",
	[":saiqian"] = "<font color=\"green\"><b>其他角色的出牌阶段限一次，</b></font>该角色可以将一至三张手牌交给你，然后你可以选择一至两项：弃置一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>手牌，或失去1点体力。你每选择一项，该角色回复1点体力。",
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
	["illustrator:marisa_sp"] = "seeker",
	["origin:marisa_sp"] = "p号：13164043，个人ID：694807", --（国人）
	["illustrator:marisa_sp_1"] = "NEKO",
	["origin:marisa_sp_1"] = "p号：42655610，个人ID：2600911", --（国人）
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
	["illustrator:marisa_sp2"] = "AUER",
	["origin:marisa_sp2"] = "p号：1150651，个人ID：178301", --（国人）
	["illustrator:marisa_sp2_1"] = "御月ユウヤ",
	["origin:marisa_sp2_1"] = "p号：13678531，个人ID：4971",
	["illustrator:marisa_sp2_2"] = "御月ユウヤ",
	["origin:marisa_sp2_2"] = "p号：750367，个人ID：4971",
		["baoyi"] = "爆衣",
	[":baoyi"] = "准备阶段开始时，你可以弃置任意数量的装备牌并弃置判定区里的所有牌。每以此法弃置一张牌，你可以视为对一名其他角色使用一张【杀】。若以此法弃置了不少于一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，此阶段结束时，你摸两张牌。",
	["@baoyi"] = "你可以发动“爆衣”，弃置任意数量的装备牌并弃置判定区里的所有牌。",
	["~baoyi"] = "弃置X张可以视为使用X张【杀】。",
	["@@baoyi_chosen"] = "由于“爆衣”的效果，你可以指定一名其他角色，视为对其使用一张【杀】。你还能视为使用 <font color=\"#00FF00\"><b>%src </b></font> 张【杀】。",
	["baoyi:drawcard"] = "由于你发动“爆衣”时弃置了<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，你可以摸两张牌。" ,

--************************************************
  --zhu006  势力：主 4血
	["reimu_yym"]= "妖妖梦sp灵梦",
	["&reimu_yym"]= "妖妖梦灵梦",
	["#reimu_yym"]= "春巫女",
	["illustrator:reimu_yym"] = "みゃけ",
	["origin:reimu_yym"] = "p号：9705615，个人ID：60103",
	["illustrator:reimu_yym_1"] = "萩原",
	["origin:reimu_yym_1"] = "p号：16215731，个人ID：37062",
	["illustrator:reimu_yym_2"] = "かがよ",
	["origin:reimu_yym_2"] = "p号：9056340，个人ID：693172",
		["zhize"]= "职责",
	[":zhize"]= "<font color=\"blue\"><b>锁定技，</b></font>摸牌阶段开始时，你选择一项：失去1点体力并跳过此回合的出牌阶段；或放弃摸牌，改为观看一名有手牌的其他角色的手牌，你可以获得其中的一张。",
	["@@zhize"]= "请指定一名其他角色，观看其手牌并获得其中一张，否则你将失去1点体力并跳过此回合的出牌阶段。",
	["chunxi"]= "春息",
	[":chunxi"]= "当你获得牌后，你可以展示其中任意张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌。每以此法展示一张牌，你可以获得一名其他角色的一张手牌。",
	["@@chunxi"]= "你可以展示你获得的<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，然后获得一名其他角色的一张手牌。",

--************************************************
	--zhu007  势力：主 4血
	["reimu_slm"] = "神灵庙sp灵梦" ,
	["#reimu_slm"] = "五欲的巫女" ,
	["&reimu_slm"] = "神灵庙灵梦" ,
	["illustrator:reimu_slm"] = "きんたろ",
	["origin:reimu_slm"] = "p号：18697741，个人ID：10009",
	["illustrator:reimu_slm_1"] = "ぬぬっこ",
	["origin:reimu_slm_1"] = "p号：24429823，个人ID：1030312",--（作者态度可以用）
	["illustrator:reimu_slm_2"] = "青芝クレハ_",
	["origin:reimu_slm_2"] = "p号：13295146，个人ID：49165",
		["bllmwuyu"] = "五欲" ,
	[":bllmwuyu"] = "准备阶段开始时，你可以将“欲”标记补至X+1枚（X为你已损失的体力值）。你可以弃一枚“欲”标记或一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，发动下列技能中的一个：<font color=\"blue\"><b>“名欲”</b></font>（你可以跳过判定阶段）<font color=\"blue\"><b>“财欲”</b></font>（摸牌阶段开始时，你可以摸一张牌）<font color=\"blue\"><b>“色欲”</b></font>（出牌阶段，你可以额外使用一张【杀】）<font color=\"blue\"><b>“睡欲”</b></font>（弃牌阶段，你的手牌上限视为4）<font color=\"blue\"><b>“食欲”</b></font>（你可以将一张手牌当【酒】使用）" ,
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
	["marisa_slm"] = "神灵庙sp魔理沙",
	["&marisa_slm"] = "神灵庙魔理沙",
	["#marisa_slm"] = "强欲的魔法使",
	["illustrator:marisa_slm"] = "まるかた",
	["origin:marisa_slm"] = "p号：8831829，个人ID：6359",
	["illustrator:marisa_slm_1"] = "まるかた",
	["origin:marisa_slm_1"] = "p号：3168979，个人ID：6359",
	["illustrator:marisa_slm_2"] = "れい亜",
	["origin:marisa_slm_2"] = "p号：36286762，个人ID：444732",
		["qiangyu"] = "强欲",
	[":qiangyu"] = "当你从牌堆顶摸牌时，你可以多摸两张牌，然后选择一项：弃置两张手牌，或弃置一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>手牌。",
	["qiangyu_spadecard"]= "请弃置一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>手牌，否则弃置两张手牌",
	["mokai"] = "魔开",
	[":mokai"] = "当你使用一张锦囊牌时，你可以将一张装备牌置于你的人物牌上，称为“天仪”。然后若你的人物牌上有大于X张“天仪”（X为你当前的体力值），你可以将一张“天仪”置入弃牌堆。",
	["tianyi"]= "天仪",
	["@mokai"] = "你可以将一张装备牌作为“天仪”置于人物牌上",
	["guangji"] = "光击",
	[":guangji"] = "当你成为【杀】的目标时，你可以将一张“天仪”置入弃牌堆，令此【杀】对你无效。",
	["guangji:invoke"] = "<font color=\"#00FF00\"><b>%src </b></font>使用【杀】指定了你为目标，你可以发动“光击”。" ,
	["xinghui"] = "星辉",
	[":xinghui"] = "当一张“天仪”从你的人物牌上离开时，你可以摸一张牌。",
	
	--zhu009
	["reimu_old"] = "旧作sp灵梦" ,
	["#reimu_old"] = "维护梦与传统的巫女" ,
	["&reimu_old"] = "旧作灵梦" ,
	["yinyang"] = "阴阳" ,
	[":yinyang"] = "出牌阶段限一次，你可令一名角色弃置一张手牌，然后你弃置一张手牌。若以此法弃置了两张颜色不同的牌，该角色摸一张牌然后回复1点体力。若以此法弃置了两张颜色相同的牌，你摸两张牌然后失去1点体力。" ,
	["lingji"] = "灵击" ,
	[":lingji"] = "结束阶段开始时，若你于本回合内使用、打出或因弃置而失去过红桃牌，你可以对一名其他角色造成一点伤害。" ,
	
	--zhu010
	["marisa_old"] = "旧作sp魔理沙" ,
	["#marisa_old"] = "源于魔法与红梦的存在" ,
	["&marisa_old"] = "旧作魔理沙" ,
	["shicong"] = "师从" ,
	[":shicong"] = "出牌阶段限一次，你可以令一名其他角色展示一张手牌。若为锦囊牌，你可以将任意一张手牌当此牌使用，直到你以此法使用了一张非黑桃牌；若为非锦囊牌，你可以将此牌置于牌堆顶。" ,
	["lianse"] = "恋色" ,
	[":lianse"] = "结束阶段开始时，你可以摸一张牌并展示之，若为黑桃，你可以令一名其他角色摸一张牌。" ,
 }