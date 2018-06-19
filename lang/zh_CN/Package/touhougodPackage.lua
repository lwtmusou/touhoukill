return{
	["touhougod"]= "神",
	["#ChooseKingdom"]= "%from 选择了 %arg 势力",

	["zun"] = "ZUN",
	["#zun"] = "创幻神主",
	["designer:zun"] = "辰焰天明",

	["chuanghuan"] = "创幻",
	[":chuanghuan"] = "<font color=\"#808080\"><b>永久技，</b></font>所有角色均选择人物牌后，你随机观看人物牌堆中的X张牌（X为角色数），选择其中的一张替换此人物牌。",


--*************************************
	--shen001  势力：神 4血
	["yukari_god"] = "神 八云紫",
	["&yukari_god"] = "神八云紫",
	["#yukari_god"] = "幻想之界",
	["designer:yukari_god"] = "星野梦美☆",

	["illustrator:yukari_god"] = "NEKO",
	["origin:yukari_god"] = "p号:28123130，个人ID：306422" ,
	["illustrator:yukari_god_1"] = "Hong",
	["origin:yukari_god_1"] = "p号:18527886，个人ID：2600911" ,
	["illustrator:yukari_god_2"] = "鈴蘭",
	["origin:yukari_god_2"] = "p号:26157977，个人ID：1086157" ,
	["illustrator:yukari_god_3"] = "stari ",
	["origin:yukari_god_3"] = "",

	["jiexian"] = "界线",
	[":jiexian"] = "当一名角色受到伤害时，你可以弃置一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，防止此伤害，令其回复1点体力。当一名角色回复体力时，你可以弃置一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，防止此体力回复效果，令其受到无来源的1点伤害。",
	["@jiexiandamage"] = "你可以发动“界线”，弃置一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，防止 <font color=\"#00FF00\"><b>%src </b></font> 受到的伤害，并使其回复1点体力。",
	["@jiexianrecover"] = "你可以发动“界线”，弃置一张<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，防止 <font color=\"#00FF00\"><b>%src </b></font> 的体力回复效果，并令其受到无来源的1点伤害。",
	["#jiexianrecover"]= "由于“%arg”的效果，%from 受到的 %arg2 点体力回复效果被防止。",
	["#jiexiandamage"]= "由于“%arg”的效果，%from 受到的 %arg2 点伤害被防止。",

--*************************************
	--shen002  势力：神 3血
	["remilia_god"] = "神 蕾米莉亚",
	["&remilia_god"] = "神蕾米莉亚",
	["#remilia_god"] = "永远的红之幼月",
	["!remilia_god"] = "神 蕾米莉亚·斯卡蕾特",
	["designer:remilia_god"] = "星野梦美☆",

	["illustrator:remilia_god"] = "とりのあくあ",
	["origin:remilia_god"] = "p号:21061795，个人ID：1960050" ,
	["illustrator:remilia_god_1"] = "ジョンディー",
	["origin:remilia_god_1"] = "p号:33584365，个人ID：1686747" ,
	["illustrator:remilia_god_2"] = "non",
	["origin:remilia_god_2"] = "p号:37458038，个人ID：19068" ,
	["illustrator:remilia_god_2"] = "UGUME",
	["origin:remilia_god_2"] = "p号:53998812，个人ID：1457830" ,

	["zhouye"] = "昼夜",
	[":zhouye"] = "<font color=\"blue\"><b>锁定技，</b></font>准备阶段开始时，你弃所有“夜”标记，然后将牌堆顶的一张牌置入弃牌堆，若为黑色，你获得一枚“夜”标记。若你没有“夜”标记，你不能使用【杀】。",
	["@ye"]= "夜",
	["hongwu"] = "红雾",
	[":hongwu"] = "出牌阶段，若你没有“夜”标记，你可以弃置两张红色牌，获得一枚“夜”标记。",
	["shenqiang"] = "神枪",
	[":shenqiang"] = "出牌阶段，若你有“夜”标记，你可以弃置一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌或武器牌，对一名其他角色造成1点伤害。",
	["yewang"] = "夜王",
	[":yewang"] = "<font color=\"blue\"><b>锁定技，</b></font>当你受到伤害时，若你有“夜”标记，此伤害-1。",
	["#YewangTrigger"]= "%from 的 “%arg” 被触发，减少了 %arg2 点伤害。",

--*************************************
	--shen003  势力：神 9血
	["cirno_god"] = "神 琪露诺",
	["&cirno_god"] = "神琪露诺",
	["#cirno_god"] = "最强",
	["designer:cirno_god"] = "三国有单",

	["illustrator:cirno_god"] = "牛木義隆",
	["origin:cirno_god"] = "个人ID：4544" ,
	["illustrator:cirno_god_1"] = "mmn.",
	["origin:cirno_god_1"] = "p号:23560857，个人ID：1791682" ,
	["illustrator:cirno_god_2"] = "きぃら",
	["origin:cirno_god_2"] = "p号:28253966，个人ID：3725" ,


	["bingfeng"] = "冰封",
	[":bingfeng"] = "<font color=\"blue\"><b>锁定技，</b></font>其他角色因受到你造成的伤害进入濒死状态时，其获得一枚“冰封”标记。你对有“冰封”标记的角色造成伤害时，此伤害-1。回合开始时，若所有其他角色均有“冰封”标记，你的阵营获胜。",
	["wushen"] = "武神",
	[":wushen"] = "<font color=\"blue\"><b>锁定技，</b></font>当装备区里有武器牌的其他角色使用【杀】指定目标后，此【杀】的伤害来源改为你。",
	["@ice"] = "冰封",
	["ice_nature"] = "冰属性",
	["ice_slash"] = "冰杀",
	["#Bingfeng"] = "%from 的“%arg”被触发， 减少了 %arg2 点 对 %to 的伤害",
	["#BingfengWin"] = "%from 的 身份是 “%arg”， 其阵营获胜",
    ["$WushenChange"] = "%from 的“%arg”被触发， 其将成为 %card 的伤害来源。",
	
--*************************************
	--shen004  势力：神 4血
	["utsuho_god"] = "神 灵乌路空",
	["&utsuho_god"] = "神灵乌路空",
	["#utsuho_god"] = "地底的太阳",
	["designer:utsuho_god"] = "星野梦美☆",

	["illustrator:utsuho_god"] = "唯",
	["origin:utsuho_god"] = "p号:8170252，个人ID：230943" ,
	["illustrator:utsuho_god_1"] = "朱シオ",
	["origin:utsuho_god_1"] = "p号:20315372，个人ID：341747" ,
	["illustrator:utsuho_god_2"] = "うき",
	["origin:utsuho_god_2"] = "p号:5189374，个人ID：134944" ,

	["shikong"] = "失控",
	[":shikong"] = "<font color=\"blue\"><b>锁定技，</b></font>你使用【杀】时，目标改为所有合法角色。",
	["#Shikong"] = "%from触发%arg， 使用【杀】的目标改为 %to 。",
	["ronghui"] = "熔毁",
	[":ronghui"] = "<font color=\"blue\"><b>锁定技，</b></font>当你使用【杀】对目标角色造成伤害时，你弃置其装备区里的所有牌。",
	["jubian"] = "聚变",
	[":jubian"] = "<font color=\"blue\"><b>锁定技，</b></font>当你使用牌结算后，若此牌对不少于两名角色造成过伤害，你回复1点体力。",
	["hengxing"] = "恒星",
	[":hengxing"] = "<font color=\"blue\"><b>锁定技，</b></font>结束阶段开始时，你失去1点体力，然后摸三张牌。",
	--[":shikong"] = "<font color=\"blue\"><b>锁定技，</b></font>你于出牌阶段空闲时间点使用【杀】时指定你攻击范围内的所有角色为目标。",
	--[":ronghui"] = "<font color=\"blue\"><b>锁定技，</b></font>当你于出牌阶段内使用【杀】对目标角色造成伤害时，你弃置其装备区里的所有牌。",
	--[":jubian"] = "<font color=\"blue\"><b>锁定技，</b></font>当你于出牌阶段内使用牌结算后，若此牌对不少于两名角色造成过伤害，你须回复1点体力。",

--*************************************
	--shen005  势力：神 0血
	["suika_god"] = "神 伊吹萃香",
	["&suika_god"] = "神伊吹萃香",
	["#suika_god"] = "虚幻的萃聚之梦",
	["designer:suika_god"] = "星野梦美☆",

	["illustrator:suika_god"] = "華々つぼみ",
	["origin:suika_god"] = "p号：4204240，个人ID：225672",
	["illustrator:suika_god_1"] = "Aの子",
	["origin:suika_god_1"] = "p号：13669192，个人ID：778201",
	["illustrator:suika_god_2"] = "チョモラン ティア",
	["origin:suika_god_2"] = "p号：32178545，个人ID：76370",

	["huanmeng"] = "幻梦",
	[":huanmeng"] = "<font color=\"#808080\"><b>永久技，</b></font>你没有体力和体力上限。你跳过摸牌阶段和弃牌阶段。回合开始时，若你没有手牌，你死亡。",
	["cuixiang"] = "萃想",
	[":cuixiang"] = "<font color=\"blue\"><b>锁定技，</b></font>准备阶段开始时，所有其他角色依次弃置一张手牌（若其没有手牌，将牌堆顶的一张牌置入弃牌堆），然后你获得零至两张以此法置入弃牌堆的牌。",
	["cuixiang-exchange"] = "<font color=\"#00FF00\"><b>%src </b></font> 的“%dest”被触发，请弃置一张手牌。",
	["xuying"] = "虚影",
	[":xuying"] = "<font color=\"#808080\"><b>永久技，</b></font>当你抵消【杀】时，你摸一张牌。当【杀】对你生效时，若你有手牌，你弃置一半的手牌（最少为一）。",
	["xuying_discard"]= "“虚影”：你未能抵消【杀】，请弃置 %src 张手牌。",

--*************************************
	--shen006  势力：神 3血
	["flandre_god"]= "神 芙兰朵露",
	["&flandre_god"]= "神芙兰朵露",
	["#flandre_god"]= "绯色月下",
	["!flandre_god"]="神 芙兰朵露·斯卡雷特",
	["designer:flandre_god"] = "星野梦美☆",

	["illustrator:flandre_god"] = "大嘘",
	["origin:flandre_god"] = "个人ID：457541",
	["illustrator:flandre_god_1"] = "6U",
	["origin:flandre_god_1"] = "p号：10349300，个人ID：94883",
	["illustrator:flandre_god_2"] = "6U",
	["origin:flandre_god_2"] = "p号：18638832，个人ID：94883",
	["illustrator:flandre_god_2"] = "6U",
	["origin:flandre_god_2"] = "p号：18638832，个人ID：94883",
	["illustrator:flandre_god_3"] = "6U",
	["origin:flandre_god_3"] = "p号：22217688，个人ID：94883",

	["kuangyan"]= "狂宴",
	[":kuangyan"]= "当你于回合外进入濒死状态时，你可以获得一枚“禁忌”标记并回复体力至体力下限，然后对当前回合角色造成1点伤害。",
	["kuangyan:recover"]= "你可以发动“狂宴”，获得一枚“禁忌”标记并回复体力至体力下限，然后对当前回合的角色 <font color=\"#00FF00\"><b> %src </b></font> 造成1点伤害。",
	["huimie"]= "毁灭",
	[":huimie"]= "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以获得一枚“禁忌”标记并令一名其他角色横置，然后对其造成1点火焰伤害。",
	["jinguo"]= "禁果",
	[":jinguo"]= "<font color=\"blue\"><b>锁定技，</b></font>出牌阶段结束时，你判定，若结果为<font size=\"5\", color=\"#808080\"><b>♠</b></font>，你跳过此回合的弃牌阶段。反之，你选择一项：弃置X张牌（X为“禁忌”标记的数量），或失去X/2点体力。",

	["@kinki"]= "禁忌",
	["@jinguo"]= "“禁果”：请弃置 %src 张牌，否则将失去 %dest 点体力。",

--*************************************
	--shen007  势力：神 3血
	["sakuya_god"] = "神 十六夜咲夜",
	["&sakuya_god"] = "神十六夜咲夜",
	["#sakuya_god"] = "铭刻的月之钟",
	["designer:sakuya_god"] = "星野梦美☆",

	["illustrator:sakuya_god"] = "雪降り",
	["origin:sakuya_god"] = "p号：2074909，个人ID：126233",
	["illustrator:sakuya_god_1"] = "Riv",
	["origin:sakuya_god_1"] = "p号：17018428，个人ID：64821",
	["illustrator:sakuya_god_2"] = "Tro",
	["origin:sakuya_god_2"] = "p号：16104268，个人ID：52449",
	["illustrator:sakuya_god_3"] = " c7肘 ",
	["origin:sakuya_god_3"] = "p号：44808875，个人ID：217707",

	["shicao"] = "时操",
	[":shicao"] = "<font color=\"blue\"><b>锁定技，</b></font>你的非额外回合的准备阶段开始时，你获得一枚“时”标记。",
	["@clock"]= "时",
	["shiting"] = "时停",
	[":shiting"] = "一名角色的回合结束前，你可以弃所有“时”标记。若如此做，此回合结束后，你进行一个额外的回合。",
	["shiting:extraturn"] = "在 <font color=\"#00FF00\"><b>%src </b></font> 的回合开始前,你可以发动“时停”，进行一个额外的回合。",
	["huanzai"] = "幻在",
	[":huanzai"] = "<font color=\"red\"><b>限定技，</b></font>结束阶段开始时，你可以获得一枚“时”标记。",
	["@huanzai"]= "幻在",
	["$huanzaiAnimate"]= "skill=sakuya_god:huanzai",
	["shanghun"] = "伤魂",
	[":shanghun"] = "<font color=\"red\"><b>限定技，</b></font>当你受到伤害后，你可以获得一枚“时”标记。",
	["$shanghunAnimate"]= "skill=sakuya_god:shanghun",
	["@shanghun"]= "伤魂",
	["#touhouExtraTurn"]= "%from 将进行一个额外的回合。",

--*************************************
	--shen008  势力：神 3血+3血
	["youmu_god"] = "神 魂魄妖梦",
	["&youmu_god"] = "神魂魄妖梦",
	["#youmu_god"] = "半生半死、半悟半惘",
	["designer:youmu_god"] = "星野梦美☆",

	["illustrator:youmu_god"] = "伊関",
	["origin:youmu_god"] = "p号：4047009，个人ID：4655",
	["illustrator:youmu_god_1"] = "ゆのまち。",
	["origin:youmu_god_1"] = "p号：34846796，个人ID：825635",
	["illustrator:youmu_god_2"] = "しがらき",
	["origin:youmu_god_2"] = "p号：30393575，个人ID：1004274",
	["illustrator:youmu_god_3"] = "ryosios",
	["origin:youmu_god_3"] = "p号：50353497，个人ID：1508165",

	["banling"] = "半灵",
	--[":banling"] = "<font color=\"#808080\"><b>永久技，</b></font>你拥有人体力和灵体力。你的体力值与其中较少的那种相同。若你的体力或体力上限将发生1点改变，你选择两种体力中的一种进行结算。",
	[":banling"] = "<font color=\"#808080\"><b>永久技，</b></font>你拥有人体力和灵体力。你的体力值为其中的较小值。若你的体力或体力上限将发生1点改变，你选择一种体力进行结算。",
	["#lingtilidamage"] = "%from 的 %arg 被触发， %from 选择了扣减 %arg2 点 “灵”体力。",
	["#rentilidamage"] = "%from 的 %arg 被触发， %from 选择了扣减 %arg2 点 “人”体力。",
	["#rentilirecover"] = "%from 的 %arg 被触发， %from 选择了回复 %arg2 点 “人”体力。",
	["#lingtilirecover"] = "%from 的 %arg 被触发， %from 选择了回复 %arg2 点 “灵”体力。",
	["banling_minus:rentili"] = "人体力(右)",
	["banling_minus:lingtili"] = "灵体力(左)" ,
	["banling_plus:rentili"] = "人体力(右)",
	["banling_plus:lingtili"] = "灵体力(左)" ,
	["@lingtili"] = "灵体力",
	["lingtili"] = "灵体力",
	["@lingtili"] = "灵体力",
	["rentili"] = "人体力",
	["@rentili"] = "人体力",
	["banling_minus"]= "半灵 扣减体力",
	["banling_plus"]= "半灵 回复体力",
	["rengui"] = "人鬼",
	["renguidraw"] = "人鬼(摸牌)",
	["renguidiscard"] = "人鬼(弃牌)",
	[":rengui"] = "准备阶段开始时，你可令一名角色摸X张牌，然后你可以依次弃置一名角色的Y张牌（X，Y分别为你已损失的灵/人体力值且最多为2）。",
	["@rengui-draw"] = "你可令一名角色摸 %src 张牌。",
	["~rengui"] = "指定一名角色→点击“确定”。",
	["@rengui-discard"] = "你可以弃置一名角色的 %src 张牌。" ,

--*************************************
	--shen009  势力：神 4血
	["reisen_god"] = "神 铃仙",
	["&reisen_god"] = "神铃仙",
	["#reisen_god"] = "狂气的赤眼",
	["!reisen_god"] = "神 铃仙·优昙华院·因幡",
	["designer:reisen_god"] = "星野梦美☆",

	["illustrator:reisen_god"] = "べし",
	["origin:reisen_god"] = "p号：6495485，个人ID：4518",
	["illustrator:reisen_god_1"] = "ATOMix",
	["origin:reisen_god_1"] = "p号：20807866，个人ID：1557409",
	["illustrator:reisen_god_2"] = "赤りんご",
	["origin:reisen_god_2"] = "p号：1438991，个人ID：164813",

	["ningshi"] = "凝视",
	[":ningshi"] = "<font color=\"blue\"><b>锁定技，</b></font>当你于出牌阶段内使用【杀】或锦囊牌指定其他角色为唯一目标后，其失去1点体力。",
	["@ningshi"] = "请弃置 %src 张牌，否则将失去 %src 点体力",
	["gaoao"] = "高傲",
	[":gaoao"] = "<font color=\"blue\"><b>锁定技，</b></font>若一张牌于你的回合外将进入你的区域，改为将此牌置入弃牌堆。",

--*************************************
	--shen010  势力：神 4血
	["sanae_god"] = "神 东风谷早苗",
	["&sanae_god"] = "神东风谷早苗",
	["#sanae_god"] = "现代人的现人神",
	["designer:sanae_god"] = "星野梦美☆",

	["illustrator:sanae_god"] = "粗茶",
	["origin:sanae_god"] = "p号：20068975，个人ID：10210",
	["illustrator:sanae_god_1"] = "9時",
	["origin:sanae_god_1"] = "p号：7765131，个人ID：98455",
	["illustrator:sanae_god_2"] = "Riv",
	["origin:sanae_god_2"] = "p号：8271655，个人ID：64821",

	["shenshou"] = "神授",
	[":shenshou"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可展示一张手牌并将其交给一名其他角色。若此牌为你最后的手牌，此牌视为【杀】，花色视为<font size=\"5\", color=\"#808080\"><b>♠</b></font>，点数视为5。<br /> 1. 若此牌为【杀】，你可令其视为对其攻击范围内的由你指定的一名角色使用【杀】。<br />2. 若此牌为<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，你可令其获得其攻击范围内的由你指定的一名角色的一张手牌。 <br />3. 若此牌的点数为5-9，你可以摸一张牌。",
	["shenshou_slash"]= "视为该角色对另一名角色使用【杀】",
	["@shenshou-slash"]= "请指定一名角色，视为 <font color=\"yellow\"><b>%src </b></font> 对其使用一张【杀】。",
	["shenshou_obtain"]= "令该角色获得另一名角色的一张手牌",
	["@shenshou-obtain"]= "请指定一名角色，<font color=\"yellow\"><b>%src </b></font> 获得其一张手牌。",
	["shenshou_draw"]= "你摸一张牌",
	["$ShenshouTurnOver"]= "%from 发动 %arg 展示了最后的手牌，此牌被视为 %card。",

--*************************************
	--shen011  势力：神 4血
	["reimu_god"] = "神 博丽灵梦",
	["&reimu_god"] = "神博丽灵梦",
	["#reimu_god"] = "幻想乡秩序的守护者",
	["designer:reimu_god"] = "三国有单",

	["illustrator:reimu_god"] = "赤りんご",
	["origin:reimu_god"] = "p号：15423619，个人ID：164813",
	["illustrator:reimu_god_1"] = "秋月リア",
	["origin:reimu_god_1"] = "p号：5490342，个人ID：12081",
	["illustrator:reimu_god_2"] = "えふぇ",
	["origin:reimu_god_2"] = "p号：34544357，个人ID：292644",

	["yibian"] = "异变",
	[":yibian"] = "一名角色的准备阶段开始时，若其身份牌暗置，其可以明置其身份牌，然后令一名与其阵营不同的角色摸一张牌；若其身份牌明置，其可以将一张牌交给一名与其阵营相同的其他角色。",
	["#YibianShow"] = "%from 发动了 <font color=\"red\"><b>异变</b></font>，明置了身份牌，为 %arg 。",
	["@yibian"] = "选择一名身份牌明置且与你阵营不同的角色，令其摸一张牌。",
	["yibian:yibian_notice"]= "你身份牌为暗置状态，是否要明置身份牌",
	["yibian_give"] = "【异变】：你可以将一张牌交给身份牌明置的同阵营角色。",
	["fengyin"] = "封印",
	[":fengyin"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以弃置一张<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，令一名身份牌明置的角色翻面并将其身份牌暗置。",
	["#FengyinHide"] = "%from 将 %arg 的身份牌 暗置 。",
	["huanxiang"] = "幻乡",
	[":huanxiang"] = "<font color=\"blue\"><b>锁定技，</b></font>结束阶段开始时，若各现存阵营之间的身份牌明置的角色数均不相同，你弃置一张牌。否则，你摸一张牌。",
	["huanxiang_discard"] = "【幻乡】的效果被触发，你需要弃置一张牌",

--*************************************
	--shen012  势力：神 4血
	["shikieiki_god"] = "神 四季映姬" ,
	["&shikieiki_god"] = "神四季映姬" ,
	["#shikieiki_god"] = "幻想乡的裁判长" ,
	["!shikieiki_god"] = "神 四季映姬·亚玛萨那度" ,
	["designer:shikieiki_god"] = "星野梦美☆",

	["illustrator:shikieiki_god"] = "卜部ミチル",
	["origin:shikieiki_god"] = "p号：20858470",
	["illustrator:shikieiki_god_1"] = "七原冬雪",
	["origin:shikieiki_god_1"] = "p号：32122356，个人ID：286217",
	["illustrator:shikieiki_god_2"] = "memai",
	["origin:shikieiki_god_2"] = "p号：2537720",
	["illustrator:shikieiki_god_3"] = "memai",
	["origin:shikieiki_god_3"] = "p号：2537720",

	["quanjie"] = "劝诫" ,
	[":quanjie"] = "其他角色的出牌阶段开始时，你可令其选择一项：摸一张牌，其于此回合内不能使用【杀】；或弃置一张【杀】。" ,
	["@quanjie-discard"] = "请弃置一张【杀】，否则你摸一张牌并不能使用【杀】直到回合结束。" ,
	["@duanzui-extra"]= "断罪",
	["duanzui"] = "断罪" ,
	[":duanzui"] = "其他角色的回合结束前，若此回合内有角色死亡，你可以翻至正面朝上，在此回合结束后进行一个额外的回合。你于此额外回合内拥有技能“审判”。" ,

--*************************************
	--shen013  势力：神 3血
	["meirin_god"] = "神 红美铃",
	["&meirin_god"] = "神红美铃",
	["#meirin_god"] = "龙神的化身",
	["designer:meirin_god"] = "三国有单",

	["illustrator:meirin_god"] = "shihou",
	["origin:meirin_god"] = "p号：16008904，个人ID：591727",
	["illustrator:meirin_god_1"] = "竹森真太郎",
	["origin:meirin_god_1"] = "p号：14127493，个人ID：293912",
	["illustrator:meirin_god_2"] = "もねてぃ",
	["origin:meirin_god_2"] = "p号：43112047，个人ID：3066815",
	["illustrator:meirin_god_3"] = "純（すなお）",
	["origin:meirin_god_3"] = "p号：58340947，个人ID：209109",

	["huaxiang"] = "华想",
	[":huaxiang"] = "当你需要使用或打出基本牌或【无懈可击】时，你可以声明之（若你的体力上限大于3，不能声明【闪】；若大于2，不能声明【桃】；若大于1，不能声明【无懈可击】），并将一张与你人物牌上的任何一张牌花色均不同的手牌置于人物牌上，称为“虹”，你视为使用或打出了一张你声明的牌。",
	["huaxiang_skill_slash"]= "华想";
	["huaxiang_skill_saveself"]= "华想";
	["rainbow"] = "虹",
	["caiyu"] = "彩雨",
	[":caiyu"] = "一名角色的结束阶段开始时，若“虹”数不少于4，你可以获得所有“虹”并弃置两张手牌，然后你可以失去1点体力上限。",
	["caiyu:discard"] ="你可以其发动“彩雨”，收回所有“虹”，然后将弃置两张手牌。",
	["caiyu_discard"]="“彩雨”：收回“虹”后，请弃置%src张手牌。",
	["caiyu:loseMaxHp"] = "你是否执行“彩雨”效果，扣减1点体力上限。",
	["xuanlan"] = "绚烂",
	[":xuanlan"] = "若你未受伤，你可以跳过弃牌阶段。",

--*************************************
	--shen014  势力：神 4血
	["eirin_god"] = "神 八意永琳",
	["&eirin_god"] = "神八意永琳",
	["#eirin_god"] = "月都大贤者",
	["designer:eirin_god"] = "星野梦美☆",

	["illustrator:eirin_god"] = "NEKO",
	["origin:eirin_god"] = "p号：30537439，个人ID：2600911",
	["illustrator:eirin_god_1"] = "zhu fun",
	["origin:eirin_god_1"] = "p号：38148654，个人ID：942466",
	["illustrator:eirin_god_2"] = "にしもん",
	["origin:eirin_god_2"] = "p号：45011715，个人ID：202286",
	["illustrator:eirin_god_3"] = "minusT",
	["origin:eirin_god_3"] = "p号：57556628，个人ID：15772166",
	["illustrator:eirin_god_4"] = "鏡 Area",
	["origin:eirin_god_4"] = "p号：49697676，个人ID：2623593",

	["qiannian"] = "千年",
	[":qiannian"] = "<font color=\"blue\"><b>锁定技，</b></font>游戏开始时或洗牌后，你获得一枚“岁月”标记。摸牌阶段，你多摸X张牌（X为“岁月”标记的数量）。你的手牌上限增加X的2倍。",
	["@qiannian"]= "岁月",
	["#qiannian_max"] = "千年",
--*************************************
	--shen015  势力：神 4血
	["kanako_god"] = "神 八坂神奈子" ,
	["&kanako_god"] = "神八坂神奈子",
	["#kanako_god"] = "大和神" ,
	["designer:kanako_god"] = "星野梦美☆",

	["illustrator:kanako_god"] = "NEKO",
	["origin:kanako_god"] = "p号：28471296，个人ID：2600911",
	["illustrator:kanako_god_1"] = "嘎哦",
	["origin:kanako_god_1"] = "p号：9510077，个人ID：230434",
	["illustrator:kanako_god_2"] = "ねりま",
	["origin:kanako_god_2"] = "p号：23881315，个人ID：3588500",

	["qinlue"] = "侵略" ,
	[":qinlue"] = "其他角色的出牌阶段开始前，你可以弃置一张【杀】或武器牌，令其选择一项：弃置一张【闪】或防具牌；  或结束回合，然后你翻至正面朝上，进行一个仅有出牌阶段的额外回合。此额外回合开始时与结束时，你与其交换手牌。",
	--[":qinlue"] = "其他角色的出牌阶段开始前，你可以弃置一张【杀】或装备牌，令其选择一项：弃置一张【闪】，或跳过此出牌阶段。若其以此法跳过出牌阶段，此回合结束后，你将所有手牌面朝下置于你的人物牌上，称为“战备”，然后获得其所有手牌，你翻至正面朝上，进行一个仅有出牌阶段的额外回合。此额外回合结束时，你将所有手牌交给其，然后获得所有“战备”。",
	["zhanbei"] = "战备" ,
	["#Qinlue"] = "%from (原来 %arg 手牌) 与 %to (原来 %arg2 手牌) 交换了手牌",
	["@qinlue-discard"]= "你可以对 <font color=\"#00FF00\"><b>%src </b></font> 发动“侵略”。";
	["@qinlue-discard1"]= "<font color=\"#00FF00\"><b>%src </b></font> 发动了“侵略”，请弃置一张【闪】或防具牌，否则你将跳过出牌阶段，其将在回合结束后进行一个仅有出牌阶段的额外阶段。";
    ["#qinlue_effect"] = "侵略(后续)" ,
	
--*************************************
	--shen016  势力：神 4血
	["byakuren_god"] = "神 圣白莲" ,
	["&byakuren_god"] = "神圣白莲" ,
	["#byakuren_god"] = "灭除八苦的尼公" ,
	["designer:byakuren_god"] = "星野梦美☆",

	["illustrator:byakuren_god"] = "NEKO",
	["origin:byakuren_god"] = "p号：31716552，个人ID：2600911",
	["illustrator:byakuren_god_1"] = "青インク",
	["origin:byakuren_god_1"] = "p号：13228444，个人ID：1372893",
	["illustrator:byakuren_god_2"] = "覚醒",
	["origin:byakuren_god_2"] = "p号：36099374，个人ID：838019",
	["illustrator:byakuren_god_3"] = "ゾウノセ",
	["origin:byakuren_god_3"] = "p号：54089405",

	["chaoren"] = "超人" ,
	[":chaoren"] = "牌堆顶的牌对你可见。你可以使用或打出此牌。",
	["$chaorendrawpile"] = "牌堆顶的牌为： %card" ,
    ["$chaoren"] = "%from 发动“%arg” 使用/打出 牌堆顶 的 %card ",
--*************************************
	--shen017  势力：神 3血
	["koishi_god"] = "神 古明地恋",
	["&koishi_god"] = "神古明地恋",
	["#koishi_god"] = "哈特曼的妖怪少女",
	["designer:koishi_god"] = "星野梦美☆",

	["illustrator:koishi_god"] = "茶葉",
	["origin:koishi_god"] = "p号：15934556，个人ID：13233",
	["illustrator:koishi_god_1"] = "Vima",
	["origin:koishi_god_1"] = "p号：45292462，个人ID：546819",
	["illustrator:koishi_god_2"] = "月本葵",
	["origin:koishi_god_2"] = "p号：45610081，个人ID：246176",

	["biaoxiang"] = "表象",
	[":biaoxiang"] = "<font color=\"purple\"><b>觉醒技，</b></font>准备阶段开始时，若你的手牌数小于二，你将体力牌翻面，并获得技能“自我”（出牌阶段，你可以弃置两张手牌，回复1点体力）。",
	["#BiaoxiangWake"]= "%from 的手牌数小于 <font color=\"yellow\"><b>2</b></font>，触发“%arg”觉醒。",
	["$biaoxiangAnimate"]= "skill=koishi_god:biaoxiang",
	["shifang"] = "释放",
	[":shifang"] = "<font color=\"purple\"><b>觉醒技，</b></font>当你失去装备区里的最后的牌时，你增加体力上限至4，并失去所有技能（觉醒技除外），然后获得技能“本我”（每当你受到伤害时，若你已受伤，你可以摸X张牌，然后来源弃置X张牌，X为你已损失的体力值）。",
	["#ShifangWake"]= "%from 失去装备区里的最后的牌，触发“%arg”觉醒。",
	["$shifangAnimate"]= "skill=koishi_god:shifang",
	["yizhi"] = "抑制",
	[":yizhi"] = "<font color=\"purple\"><b>觉醒技，</b></font>当你进入濒死状态时，你回复体力至体力下限，失去体力上限至3，并失去所有技能（觉醒技除外），然后获得技能“超我”（结束阶段开始时，你可以弃置一张手牌，令一名体力上限不小于你的角色摸两张牌，若其的体力上限为3，你摸两张牌）。",
	["#YizhiWake"]= "%from 进入濒死状态，触发“%arg”觉醒。",
	["$yizhiAnimate"]= "skill=koishi_god:yizhi",
	["ziwo"] = "自我",
	[":ziwo"] = "出牌阶段，你可以弃置两张手牌，回复1点体力。",
	["benwo"] = "本我",
	[":benwo"] = "每当你受到伤害时，若你已受伤，你可以摸X张牌，然后来源弃置X张牌，X为你已损失的体力值。",
	["benwo:invoke"] = "你将受到来自 <font color=\"#00FF00\"><b>%src </b></font> 的伤害，你可以发动“本我”，摸 %dest 张牌，然后令 <font color=\"#00FF00\"><b>%src </b></font> 弃置 %dest 张牌。",
	["chaowo"] = "超我",
	[":chaowo"] = "结束阶段开始时，你可以弃置一张手牌，令一名体力上限不小于你的角色摸两张牌，若其的体力上限为3，你摸两张牌。",
	["@chaowo"] = "你可以发动“超我”，令一名体力上限不小于你的角色摸两张牌，若其的体力上限为3，你摸两张牌。",
	["~chaowo"] = "请选择要弃置的手牌并指定“超我”的目标。",
	["@chaowo_chosenplayer"] = "请指定一名角色，令其摸两张牌。",

--*************************************
	--shen018  势力：神 5血
	["suwako_god"] = "神 泄矢诹访子",
	["#suwako_god"] = "名存实亡的神明",
	["&suwako_god"] = "神泄矢诹访子",
	["designer:suwako_god"] = "星野梦美☆",

	["illustrator:suwako_god"] = "ルリア",
	["origin:suwako_god"] = "p号：13338753",
	["illustrator:suwako_god_1"] = "伊吹のつ",
	["origin:suwako_god_1"] = "p号：51609431",
	["illustrator:suwako_god_2"] = "もしよ",
	["origin:suwako_god_2"] = "p号：45182321",
	["illustrator:suwako_god_3"] = " 粟 ",
	["origin:suwako_god_3"] = "p号：44497383",

	["zuosui"] = "作祟",
	[":zuosui"] = "当你对其他角色造成伤害时，你可以防止此伤害并获得一枚“信仰”标记，令其选择一个不大于4的正整数X，然后你选择一项：令其摸X张牌，然后其失去X点体力；或令其将牌弃至X张。",
	["#zuosuichoice"]= "由于“%arg”的效果，%from 选择了一个数字 %arg2",
	["zuosui:losehp"]= "流失体力",
	["zuosui:discard"]= "弃牌",
	["worao"] = "沃饶",
	[":worao"] = "当你成为其他角色使用【杀】或普通锦囊牌的目标后，你可以摸两张牌并获得一枚“信仰”标记，然后将一张手牌交给该角色。",
	["worao:invoke"]= "<font color=\"#00FF00\"><b> %src </b></font>使用【%dest】指定了你为目标，你可以发动“沃饶”。",
	["woraoGive"]= "请将一张手牌交给“沃饶”对象 <font color=\"#00FF00\"><b>%src </b></font>。",
	["@xinyang"] = "信仰",
	["shenhua"] = "神话",
	[":shenhua"] = "<font color=\"blue\"><b>锁定技，</b></font>结束阶段开始时，你选择一项：失去1点体力上限；弃所有“信仰”标记。",
	["shenhua:loseHp"] = "失去体力上限",
	["shenhua:discardMark"] = "弃标记",

--*************************************
	--shen019  势力：神 4血
	["miko_god"] = "神 丰聪耳神子",
	["&miko_god"] = "神丰聪耳神子",
	["#miko_god"] = "日出之国的天子",
	["designer:miko_god"] = "三国有单",

	["illustrator:miko_god"] = "Mik-cis",
	["origin:miko_god"] = "p号：35385868",
	["illustrator:miko_god_1"] = "まくわうに",
	["origin:miko_god_1"] = "p号：42026225",
	["illustrator:miko_god_2"] = "りひと",
	["origin:miko_god_2"] = "p号：53472162",

	["hongfo"]= "弘佛",
	[":hongfo"]= "摸牌阶段摸牌后，你可以指定所有势力与你相同的角色，这些角色依次摸一张牌。然后你令其余角色中的一名角色将其势力改变为你的势力。",
	["#hongfoChangeKingdom"] = "%from 被改变为 %arg 势力",
	["@hongfo"]= "你需要将一名其他角色的势力属性改变为和你相同",
	["junwei"]= "君威",
	[":junwei"]= "<font color=\"blue\"><b>锁定技，</b></font>当你成为【杀】的目标后，若此【杀】的使用者的势力与你不同，其选择一项：弃置一张黑色牌，或令此【杀】对你无效。",
	["@junwei-discard"]= "<font color=\"#00FF00\"><b>%src </b></font> 的“君威”被触发，请弃置一张黑色牌，否则【%dest】对其无效。",
	["gaizong"]= "改宗",
	[":gaizong"]= "<font color=\"purple\"><b>觉醒技，</b></font>结束阶段开始时，若势力数不大于2，你须失去1点体力上限，重新选择势力，失去技能“弘佛”，获得技能“问道”（出牌阶段开始时，你可以指定任意数量的势力各不相同的角色，你依次弃置这些角色的一张手牌。每以此法弃置一张红色牌，你回复1点体力）",
	["#GaizongWake"] = "场上势力数不大于2， %from 触发“%arg”觉醒。",
	["$gaizongAnimate"]= "skill=miko_god:gaizong",
	["wendao"]= "问道",
	[":wendao"]= "出牌阶段开始时，你可以指定任意数量的势力各不相同的角色，你依次弃置这些角色的一张手牌。每以此法弃置一张红色牌，你回复1点体力",
	["@wendao"] = "你可以发动“问道”，指定势力各不相同的任意数量的角色，你依次弃置这些角色的一张手牌。",
	["~wendao"] = "选择角色，然后确定",
	["@wendao-dis"] = "你对自己发动了“问道”，选择问道需弃置的牌",

--*************************************
   --shen020  势力：神 4血
	["kaguya_god"] = "神 蓬莱山辉夜",
	["&kaguya_god"] = "神蓬莱山辉夜",
	["#kaguya_god"] = "永远的公主殿下",
	["designer:kaguya_god"] = "三国有单",

	["illustrator:kaguya_god"] = "しまちょ",
	["origin:kaguya_god"] = "p号：1051695，个人ID：12647",
	["illustrator:kaguya_god_1"] = "yae狼ト",
	["origin:kaguya_god_1"] = "p号：26921148",
	["illustrator:kaguya_god_2"] = "乃絵のえる",
	["origin:kaguya_god_2"] = "p号：51840626",
	["illustrator:kaguya_god_3"] = "鏡 Area",
	["origin:kaguya_god_3"] = "p号：46207117，个人ID：2623593",

	["shenbao"] = "神宝",
	[":shenbao"] = "<font color=\"blue\"><b>锁定技，</b></font>视为你装备着场上的所有装备牌（武器牌的攻击范围取最长，名称不同的装备牌效果均叠加）。",
--[[
	["shenbao_spear"] = "丈八蛇矛",
	["shenbao_pagoda"] = "宝塔",
	["shenbao_jadeSeal"] = "玉玺",
	[":shenbao_spear"] = "若其他角色装备有丈八蛇矛，你可以发动丈八蛇矛",
	["：shenbao_pagoda"] = "若其他角色装备有宝塔，你可以发动宝塔",
	["：shenbao_jadeSeal"] = "若其他角色装备有玉玺，你可以发动玉玺",
]]
	["shenbao_attach"] = "装备",
	[":shenbao_attach"] = "你可以发动其他角色装备牌的技能。",

    ["#shenbao_distance"] = "神宝(距离)",
	["#shenbao"] = "神宝",
--*************************************
	--shen021  势力：神 4血
	["komachi_god"] = "神 小野塚小町",
	["&komachi_god"] = "神小野塚小町",
	["#komachi_god"] = "江户时代气质的死神",
	["designer:komachi_god"] = "三国有单",

	["illustrator:komachi_god"] = "にしもん",
	["origin:komachi_god"] = "p号：29989054，个人ID：202286",
	["illustrator:komachi_god_1"] = "蟻",
	["origin:komachi_god_1"] = "p号：51593223",
	["illustrator:komachi_god_2"] = "藤原",
	["origin:komachi_god_2"] = "个人ID：27517",

	["yindu"] = "引渡",
	[":yindu"] = "当其他角色死亡时，你可以摸三张牌。若如此做，不执行奖惩。",
	["yindu:invoke"] = "你是否发动“引渡”，摸 3 张牌，若如此做，<font color=\"#00FF00\"><b>%src </b></font>的死亡将不会执行奖惩",
	["huanming"] = "换命",
	[":huanming"] = "<font color=\"red\"><b>限定技，</b></font>当你对其他角色造成伤害时，你可以防止此伤害，并与其交换体力。",
	["$huanmingAnimate"]= "skill=komachi_god:huanming",
	["chuanwu"] = "川雾",
	[":chuanwu"] = "<font color=\"blue\"><b>锁定技，</b></font>你与其他角色的距离最多为X（X为其体力值且最少为1）。",

--*************************************
		--shen022  势力：神 1血
	["yuyuko_god"] = "神 西行寺幽幽子" ,
	["&yuyuko_god"] = "神西行寺幽幽子" ,
	["#yuyuko_god"] = "天衣无缝的亡灵",
	["designer:yuyuko_god"] = "三国有单",

	["illustrator:yuyuko_god"] = "NEKO",
	["origin:yuyuko_god"] = "p号：33060621,个人ID:2600911",
	["illustrator:yuyuko_god_1"] = "赤りんご",
	["origin:yuyuko_god_1"] = "p号：32219109,个人ID:164813",
	["illustrator:yuyuko_god_2"] = "RAN",
	["origin:yuyuko_god_2"] = "p号：60095372,个人ID:2957827",

	["fanhun"] = "反魂",
	[":fanhun"] = "<font color=\"#808080\"><b>永久技，</b></font>你进入濒死状态时，增加1点体力上限，将体力回复至体力上限，摸体力上限张数的牌。结束阶段开始时，若你的体力上限大于4，你死亡。",
	["yousi"] = "诱死",
	[":yousi"] = "<font color=\"blue\"><b>锁定技，</b></font>你的回合内，其他角色的体力下限视为x。（x为你的体力值）",
    ["#yousi"] = "诱死",
--*************************************
		--shen023  势力：神 3血
	["satori_god"] = "神 古明地觉",
	["&satori_god"] = "神古明地觉" ,
	["#satori_god"] = "大家的心病",
	["designer:satori_god"] = "三国有单",

	["illustrator:satori_god"] = "ぼんた",
	["origin:satori_god"] = "p号：17281878,个人ID:12945",
	["illustrator:satori_god_1"] = "hitsu ",
	["origin:satori_god_1"] = "p号：47473508,个人ID:671593",
		["illustrator:satori_god_2"] = "皆村",
	["origin:satori_god_2"] = "p号：28695491，个人ID：6098",

	--["xinhua"] = "心花",
	--[":xinhua"] = "当你成为其他角色使用牌的目标后或当你使用牌指定其他角色为目标后，你可以选择你和该角色的各一张手牌，交换之并依次明置之。",
	--["dongcha"] = "洞察",
	--[":dongcha"] = "当其他角色使用牌时，你可以暗置你的一张与此牌类别相同的明置手牌，取消所有目标。若此牌为明置手牌，你可以明置其一张手牌。",
	--["@dongcha"] = "“洞察”： 你可以将一张明置的 %arg 暗置， 取消 %dest 使用的 %src 的所有目标 ",
	--["zhuiyi"] = "追忆",
	--[":zhuiyi"] = "<font color=\"blue\"><b>锁定技，</b></font>当其他角色的明置手牌因弃置而置入弃牌堆后，你获得并明置之。",
	["shown_card"] = "明牌",
	["%shown_card"] = "他人的明牌",
	
	["kuixin"] = "窥心",
	--[":kuixin"] = "当你使用牌指定其他角色为唯一目标后或当你成为其他角色使用牌的唯一目标后，你可观看其手牌，你可以明置其中一张暗置牌。",
	[":kuixin"] = "<font color=\"blue\"><b>锁定技，</b></font>当你使用的牌对其他角色结算后或其他角色使用的牌对你结算后，若目标数为1且该角色没有明置的手牌，你明置其一张手牌。",
	
	["xinhua"] = "心花",
	--[":xinhua"] = "你可以使用或打出其他角色的明置手牌。<font color=\"green\"><b>每阶段限一次。</b></font>",

	[":xinhua"] = "你可以使用或打出其他角色的明置手牌。",
	
	["cuimian"] = "催眠",
	[":cuimian"] = "<font color=\"blue\"><b>锁定技，</b></font>若其他角色的明置手牌数不大于其手牌上限，其于其回合内不能使用、打出或弃置明置手牌。",
	["$xinhua"] = "%from 发动“%arg” 使用/打出 %to 的 %card ",
--*************************************
		--shen024  势力：神 4血
	["aya_god"] = "神 射命丸文" ,
	["&aya_god"] = "神射命丸文" ,
	["#aya_god"] = "风雨之鸦" ,
	["designer:aya_god"] = "三国有单",

	["illustrator:aya_god"] = "リリア",
	["origin:aya_god"] = "p号：10207828，个人ID：997454",
	["illustrator:aya_god_1"] = "风骚华探长",
	["origin:aya_god_1"] = "p号：52206792,个人ID:4122676",
	["illustrator:aya_god_2"] = "夜汽車",
	["origin:aya_god_2"] = "p号：27449018,个人ID:186716",
	["illustrator:aya_god_3"] = "中壱（なかいち）",
	["origin:aya_god_3"] = "p号：54770479,个人ID:2134064",

	["tianqu"] = "天衢",
	[":tianqu"] = "你于出牌阶段空闲时间点使用牌可以无视合法性选择其他角色为目标。你使用牌无次数限制。",
	["fengmi"] = "风靡",
	[":fengmi"] = "你使用【杀】指定其他角色为目标后，你可令其弃置其区域里的所有牌，摸等量的牌。",

--*************************************
	--shen025  势力：神 3血
	["seiga_god"] = "神 霍青娥",
	["&seiga_god"] = "神霍青娥",
	["#seiga_god"] = "古老的邪仙",
	["designer:seiga_god"] = "三国有单",

	["illustrator:seiga_god"] = "白蘇ふぁみ",
	["origin:seiga_god"] = "p号：54003676,个人ID:5860132",
	["illustrator:seiga_god_1"] = "ゾウノセ",
	["origin:seiga_god_1"] = "p号：35834843,个人ID:2622803",

	["huanhun"] = "还魂",
	[":huanhun"] = "<font color=\"#808080\"><b>永久技，</b></font>当其他角色死亡后，若你已死亡且不属于现存人数最多的阵营，你以游戏开始时的状态重新加入游戏。",
	["tongling"] = "通灵",
	[":tongling"] = "<font color=\"red\"><b>限定技，</b></font>回合开始时，你可以获得一名死亡角色的人物牌，并获得其上的一个技能（永久技，觉醒技，主公技及其他角色拥有的技能除外）。",
	["$tonglingAnimate"]= "skill=seiga_god:tongling",
	["rumo"] = "入魔",
	[":rumo"] = "<font color=\"red\"><b>限定技，</b></font>出牌阶段，你可令包括你在内的至多X名角色横置，然后你摸X张牌（X为人数最多的阵营的角色数）。",
	["$rumoAnimate"]= "skill=seiga_god:rumo",



--*************************************
	["nue_god"]= "神 封兽鵺",
	["&nue_god"] = "神封兽鵺",
	["#nue_god"]= "真相不明的未知Ｘ ",
	["designer:nue_god"] = "三国有单",
	
    ["anyun"] = "暗云",
	[":anyun"] = "游戏开始时，你将3张人物牌扣置于你的人物牌旁，称为“X”。若你没有明置的X，你可以公开X并发动及拥有其上的技能，直到失去此X。（永久技，限定技，觉醒技除外）。",
	["benzun"] = "本尊",
	[":benzun"] = "<font color=\"blue\"><b>锁定技，</b></font>任意回合结束时，若你有明置的X，将一张人物牌扣置为X，移除明置的X。然后将体力上限调整为游戏开始时，并移除标记和人物牌上的牌。",
    ["#GetHuashen"] = "%from 获得了  %arg 张 X",
    ["huashencard"] = "X",
    ["#anyunShowStatic"] = "暗云（状态技）",
	["#anyun_prohibit"] = "暗云（非法目标）",
    ["invoke_hidden_compulsory:compulsory"] = "你可以发动隐藏的锁定技 <font color=\"#FF8000\"><b>%src </b></font>。",
    ["#RemoveHiddenGeneral"] = "%from 移除了明置的人物牌  %arg ",
	["#ShowHiddenGeneral"] = "%from 公开了人物牌  %arg ",
    ["#GetHuashenDetail"] = "%from 获得了“化身牌” %arg",
	
	["marisa_god"] = "神 雾雨魔理沙",
	["&marisa_god"] = "神雾雨魔理沙",
	["#marisa_god"] = "奇妙的魔法使",
	["designer:marisa_god"] = "三国有单",
	
	["chongneng"] = "充能",
	[":chongneng"] = "你使用手牌指定目标时，你可选择一项： 取消一名目标并获得一枚“星”标记； 或弃一枚“星”标记，令此牌的一个效果值+1。若此牌为<font size=\"5\", color=\"#808080\"><b>♠</b></font>，你摸一张牌。",
	["@chongneng"] = "你发动了<font color=\"#00FF00\"><b>充能</b></font> 选择你要取消的目标，并确定。<br/>或者直接取消则视为弃“星”标记增加效果值",
	["@star"] = "星",
	["chongneng:1"] = "效果中的第一个的效果基值+1" ,
	["chongneng:2"] = "效果中的最后一个效果值+1" ,
	["#Chongneng"] = "%from 发动了 <font color=\"#00FF00\">充能</font>， 令【%arg】的 第 %arg2 个效果值+1" ,
	["#ChongnengCacel"] = "当其他角色使用牌时，你可以暗置你的一张与此牌类别相同的明置手牌，取消所有目标。若此牌为明置手牌，你可以明置其一张手牌。",
	
	["huixing"] = "彗星",
	[":huixing"] = "你使用基本牌或普通锦囊牌对唯一目标结算完毕后，你可弃一枚“星”标记，视为对其下家使用效果值不变的同名牌。",
    ["huixing:target"]= "你可发动 <font color=\"#00FF00\"><b>%src </b></font>, 视为对<font color=\"#00FF00\"><b>%dest</b></font> 使用<font color=\"#00FF00\"><b> 【%arg】</b></font> 。",

	
	["alice_god"] = "神 爱丽丝",
	["&alice_god"] = "神爱丽丝",
	["#alice_god"] = "少女裁判",
	
	
	["shinmyoumaru_god"] = "神 少名针妙丸",
	["&shinmyoumaru_god"] = "神少名针妙丸",
	["#shinmyoumaru_god"] = "辉光之针的利立浦特",
	
	
	["tenshi_god"] = "神 比那名居天子",
	["&tenshi_god"] = "神比那名居天子",
	["#tenshi_god"] = "有顶天变",
	
	["yuyuko_1v3"] = "西行寺幽幽子",
	["!yuyuko_1v3"] = "神 西行寺幽幽子-1v3",
	["#yuyuko_1v3"] = "天衣无缝的亡灵" ,
	
	
	["kaguya_1v3"] = "蓬莱山辉夜",
	["!kaguya_1v3"] = "神 蓬莱山辉夜-1v3",
	["yueji"] = "月姬",
	[":yueji"] = "场上装备牌数量少于5时，你视为拥有技能“永恒”。",
}
