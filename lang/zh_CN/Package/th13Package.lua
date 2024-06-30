return{

	["th13"] = "神灵庙",
	["slm"] = "灵",

	--slm001  势力：灵 4血
	["miko"] = "丰聪耳神子",
	["#miko"] = "圣德道士",
	["designer:miko"] = "星野梦美☆",

	["illustrator:miko"] = "NAbyssor",
	["origin:miko"] = "p号：21176402，个人id：467511",
	["illustrator:miko_1"] = "カズ",
	["origin:miko_1"] = "p号：36122574，个人id：137496",
	["illustrator:miko_2"] = "泉水茜",
	["origin:miko_2"] = "p号：21607040，个人id：786736",
	["illustrator:miko_3"] = "詩韻",
	["origin:miko_3"] = "p号：21141746，个人id：44784",
	["illustrator:miko_4"] = "七瀬尚",
	["origin:miko_4"] = "p号：23917375，个人id：312852",

	["shengge"] = "圣格",
	[":shengge"] = "<font color=\"purple\"><b>觉醒技，</b></font>准备阶段开始时，若你没有手牌或手牌数唯一最少，你减1点体力上限，摸三张牌。",
	["$shenggeAnimate"]= "skill=miko:shengge",
	["#ShenggeWake"] = "%from 没有手牌，触发“%arg”觉醒",
	["qingting"] = "倾听",
	[":qingting"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以令有手牌的所有其他角色各将一张手牌交给你（“圣格”发动后，改为你获得这些角色各一张手牌），然后你交给这些角色各一张手牌。",
	["qingtingGive"]= "<font color=\"#00FF00\"><b>%src </b></font> 发动了“倾听”，请将一张手牌交给 <font color=\"#00FF00\"><b>%src </b></font>。",
	["qingtingReturn"]= "请将一张手牌交给被“倾听”的角色 <font color=\"#00FF00\"><b>%src </b></font>。",
	["chiling"] = "敕令",
	[":chiling"] = "<font color=\"orange\"><b>主公技，</b></font>当你成为其他角色使用的【杀】的唯一目标时，你可以令体力值大于你的其他灵势力角色选择是否将此牌目标转移给其。",
	["chiling_target:384318315"] = "敕令你可以将目标为主公的【杀】转移给你",

--*********************************************
	--slm002  势力：灵 4血
	["mamizou"] = "二岩猯藏",
	["#mamizou"] = "狸猫怪的十大变化",
	["designer:mamizou"] = "星野梦美☆",

	["illustrator:mamizou"] = "茹でピー",
	["origin:mamizou"] = "p号：33663902，个人id：264281",

	["illustrator:mamizou_1"] = "ふーえん",
	["origin:mamizou_1"] = "p号：36315798，个人id：131669",
	["illustrator:mamizou_2"] = "まくわうに",
	["origin:mamizou_2"] = "p号：33687206，个人id：941624",
	["illustrator:mamizou_3"] = "ideolo",
	["origin:mamizou_3"] = "p号：27366760，个人id：61513",
	["illustrator:mamizou_4"] = "7",
	["origin:mamizou_4"] = "p号：46198301，个人id：547647",

	["xihua"] = "戏画",
	[":xihua"] = "当你需要使用/打出任意基本牌或普通锦囊牌时，你可以声明之（此回合内以此法声明过的牌名除外）并令一名其他角色展示你的一张手牌，若此牌与声明的牌类别相同或此牌的点数为J~K，你将之当声明的牌使用/打出，否则你弃置之。",
	["#Xihua_failed"] = "%from 发动 “%arg” 声明【%arg2】失败。",
	["~xihua"]= "选择“戏画牌”的使用目标",
	["@xihua_chosen"]= "为“戏画牌” “%src” 选择一名要展示你的牌的角色。",
	["xihua_vs"]= "戏画",
	["xihua_skill_slash"]= "戏画",
	["xihua_skill_saveself"]= "戏画",
	["#Xihua"] = "%from 发动了“%arg2”，声明此牌为 【%arg】，选择的目标为 %to",
	["#XihuaNoTarget"] = "%from 发动了“%arg2”，声明此牌为 【%arg】",
	["#xihua_clear"]= "戏画(后续)",
--*********************************************
	--slm003  势力：灵 3血
	["futo"] = "物部布都",
	["#futo"] = "古代日本的尸解仙",
	["designer:futo"] = "星野梦美☆",

	["illustrator:futo"] = "とりのあくあ",
	["origin:futo"] = "p号：22092678，个人id：1960050",
	["illustrator:futo_1"] = "Mik-cis",
	["origin:futo_1"] = "p号：33480707，个人id：1006311",
	["illustrator:futo_2"] = "ななしな",
	["origin:futo_2"] = "p号：23798022，个人id：274594",
	["illustrator:futo_3"] = "キタユキ",
	["origin:futo_3"] = "p号：32655390，个人id：199411",
	["illustrator:futo_4"] = "まくわうに",
	["origin:futo_4"] = "p号：33451843，个人id：941624",
	["illustrator:futo_5"] = "みや",
	["origin:futo_5"] = "p号：22338210，个人id：41977",

	["shijie"] = "尸解",
	[":shijie"] = "当一名角色向你求【桃】时，你可以将其装备区里的一张牌当【桃】对其使用。",
	["#shijie"] = "尸解",
	["fengshui"] = "风水",
	[":fengshui"] = "当判定开始前，你可以观看牌堆顶和底部各一张牌并可以交换之。",
	["#fengshuiResult"] = "%from 的“<font color=\"yellow\"><b>风水</b></font>”结果：%arg 上，%arg2 下。",
	["xiangdi"] = "相地",
	[":xiangdi"] = "当你于出牌阶段外因使用、打出或弃置而失去红色牌后，你可以判定，当判定牌生效后，你令一名角色获得之，然后可以将场上的一张与之花色相同的牌置入一名角色的装备区或判定区里。每回合限一次。",
	["@xiangdi-giveCard"] = "相地你可以令一名角色获得判定牌",
	["@xiangdi-movefrom"] = "相地你可以选择一名角色，移动一张装备或判定区内的%arg牌",
	["@xiangdi-self"] = "相地你选择一张你装备或判定区内的%arg牌",
	["@xiangdi-moveto"] = "相地你可以将该牌移动至另一名角色的%arg",

--*********************************************
	--slm004  势力：灵 4血
	["toziko"] = "苏我屠自古",
	["#toziko"] = "神明后裔的亡灵",
	["designer:toziko"] = "星野梦美☆",

	["illustrator:toziko"] = "まぐろいど",
	["origin:toziko"] = "p号：22946343，个人id：2096991",
	["illustrator:toziko_1"] = "遠坂あさぎ",
	["origin:toziko_1"] = "p号：26376620，个人id：3302692",
	["illustrator:toziko_2"] = "ぬぬっこ",
	["origin:toziko_2"] = "p号：22618790，个人id：1030312",

	["leishi"] = "雷矢",
	[":leishi"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以视为对一名有手牌的其他角色使用不计入使用次数限制的雷【杀】，若如此做，当此牌被【闪】抵消时，你失去1点体力。",
	["fenyuan"] = "愤怨",
	[":fenyuan"] = "当你于回合外进入濒死状态时，你可以死亡（视为没有角色杀死过你）并令当前回合角色受到无来源的2点雷电伤害。",
	["fenyuan:invoke"] = "你可以发动“愤怨”立即死亡（无来源）并让当前回合的角色 <font color=\"#00FF00\"><b>%src </b></font> 受到无来源的2点雷电伤害。",

--*********************************************
	--slm005  势力：灵 3血
	["seiga"]= "霍青娥",
	["#seiga"]= "穿壁之邪仙人",
	["designer:seiga"] = "星野梦美☆",

	["illustrator:seiga"] = "桐葉",
	["origin:seiga"] = "p号：24756353，个人id：260108",
	["illustrator:seiga_1"] = "ぬぬっこ",
	["origin:seiga_1"] = "p号：22465190，个人id：1030312",
	["illustrator:seiga_2"] = "みぃこ",
	["origin:seiga_2"] = "p号：42397248，个人id：5044827",
	["illustrator:seiga_3"] = "まぐろいど",
	["origin:seiga_3"] = "p号：22021510，个人id：2096991",
	["illustrator:seiga_4"] = "みゅーと",
	["origin:seiga_4"] = "p号：27261606，个人id：1203504",

	["xiefa"]= "邪法",
	[":xiefa"]= "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以将一张手牌交给一名其他角色并选择其攻击范围内的一名角色，令其视为对该角色使用【杀】，若如此做：当此【杀】被【闪】抵消时，其失去1点体力；当目标角色受到此【杀】造成的伤害后，你摸一张牌。",
	["chuanbi"]= "穿壁",
	[":chuanbi"]= "当你需要使用/打出【闪】时，若当前回合角色的装备区里没有武器牌，你可以视为使用/打出【闪】。",

--*********************************************
	--slm006  势力：灵 4血
	["yoshika"]= "宫古芳香",
	["#yoshika"]= "忠诚的尸体",
	["designer:yoshika"] = "星野梦美☆",

	["illustrator:yoshika"] = "ハシコ",
	["origin:yoshika"] = "p号：18277982，个人id：74348",
	["illustrator:yoshika_1"] = "KS",
	["origin:yoshika_1"] = "p号：18416435，个人id：649747",
	["illustrator:yoshika_2"] = "Cetera",
	["origin:yoshika_2"] = "p号：35882272，个人id：192311",
	["illustrator:yoshika_3"] = "tucana",
	["origin:yoshika_3"] = "p号：18499354，个人id：1593245",
	["illustrator:yoshika_4"] = " 二酸化炭素 ",
	["origin:yoshika_4"] = "p号：56385422",

	["duzhua"]= "毒爪",
	[":duzhua"]= "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以视为使用不计入使用次数限制的【杀】，使用时你须选择弃置一张红色手牌或失去1点体力。",
	["#duzhuaTargetMod"]= "毒爪",
	["taotie"]= "饕餮",
	[":taotie"]= "当其他角色使用【闪】时，若你已受伤，你可以判定，若结果为黑色，你回复1点体力。",

--*********************************************
	--slm007  势力：灵 3血
	["kyouko"] = "幽谷响子",
	["#kyouko"] = "念经的山灵",
	["designer:kyouko"] = "星野梦美☆",

	["illustrator:kyouko"] = "砂雲",
	["origin:kyouko"] = "p号：19958781，个人id：295604",
	["illustrator:kyouko_1"] = "潜男",
	["origin:kyouko_1"] = "p号：18253434，个人id：1220170",
	["illustrator:kyouko_2"] = "ideolo",
	["origin:kyouko_2"] = "个人id：61513",

	["songjing"] = "诵经",
	[":songjing"] = "当一名角色使用延时类锦囊牌时，你可以摸两张牌。",
	["songjing:use"] = "<font color=\"#00FF00\"><b>%src </b></font>使用了延时类锦囊【%dest】，你可以发动“诵经”",
	["gongzhen"] = "共振",
	[":gongzhen"] = "当一名在你的攻击范围内的角色受到【杀】造成的伤害后，你可以弃置一张与此【杀】花色相同的手牌，对其造成1点伤害。",
	["@gongzhen"] = "你可以发动 “共振” 弃置一张%arg 花色的 手牌，对 %src 造成1点伤害",
--*********************************************
	--slm008  势力：灵 3血
	["yuyuko_slm"] = "神灵庙SP幽幽子" ,
	["&yuyuko_slm"] = "神灵庙幽幽子" ,
	["#yuyuko_slm"] = "不再彷徨的亡灵" ,
	["designer:yuyuko_slm"] = "星野梦美☆",

	["illustrator:yuyuko_slm"] = "みや",
	["origin:yuyuko_slm"] = "p号：22766094，个人id：41977",
	["illustrator:yuyuko_slm_1"] = ".SIN",
	["origin:yuyuko_slm_1"] = "p号：72973111；个人id：1763655",
	["illustrator:yuyuko_slm_2"] = "一葉モカ",
	["origin:yuyuko_slm_2"] = "p号：17232914，个人id：464525",
	["illustrator:yuyuko_slm_3"] = "はいばね",
	["origin:yuyuko_slm_3"] = "p号：22145913",

	["chuixue"] = "吹雪" ,
	[":chuixue"] = "弃牌阶段结束时，若你于此阶段内弃置过你的至少一张手牌，你可以令一名其他角色选择一项：弃置与这些牌花色均不同的一张手牌，或失去1点体力。" ,
	["@chuixue-select"] = "请选择要“吹雪”的角色。" ,
	["@chuixue-discard"] = "请弃置一张手牌且此牌的花色不能与此阶段内 <font color=\"#00FF00\"><b>%src </b></font> 弃置的任意一张牌相同，否则你将失去1点体力。" ,
	["wushou"] = "无寿" ,
	[":wushou"] = "当你成为【杀】或【决斗】的目标后，你可以摸X张牌（X为你的体力上限），然后弃置Y张手牌（Y为你的体力值）。" ,
	["wushou_discard"]= "“无寿”：请弃置 %src 张手牌。",

--*********************************************
	--slm009  势力：灵 3血
	["nue_slm"] = "神灵庙SP鵺",
	["&nue_slm"] = "神灵庙鵺",
	["#nue_slm"] = "古代妖怪之一",
	["designer:nue_slm"] = "星野梦美☆",

	["illustrator:nue_slm"] = "赤シオ",
	["origin:nue_slm"] = "p号：6203096，个人id：341747",
	["illustrator:nue_slm_1"] = "赤シオ",
	["origin:nue_slm_1"] = "p号：9055771，个人id：341747",
	["illustrator:nue_slm_2"] = "なまもななせ",
	["origin:nue_slm_2"] = "p号：9785493，个人id：1167548",
	["illustrator:nue_slm_3"] = "赤シオ",
	["origin:nue_slm_3"] = "p号：26650042，个人id：341747",

	["buming"] = "不明",
	[":buming"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以弃置一张牌并选择一名在你攻击范围内的角色，其须声明【杀】或【决斗】，然后你视为对其使用其声明的牌。",
	["slash_buming"] = "视为你被使用【杀】",
	["duel_buming"] = "视为你被使用【决斗】",
	["#buming_choose"]= "%from 选择了 “%arg”。",
	["zhengti"] = "正体",
	[":zhengti"] = "当你受到伤害后，你可以获得伤害来源装备区里的一张牌；当你受到伤害时，你可以将一张装备牌置入其他角色的装备区，将此伤害转移给其。",
	["@zhengti-rob"] = "正体你可以发动“正体”抢走 %src 一张装备区的牌",
	["@zhengti-redirect"] = "正体你可以将一张装备牌置入一名角色的装备区，将伤害转移给其",
	["~zhengti"] = "选择装备牌 -> 选择角色 -> “确定”",

--*********************************************
	--slm010  势力：灵 3血
	["kogasa_slm"] = "神灵庙SP小伞" ,
	["&kogasa_slm"] = "神灵庙小伞" ,
	["#kogasa_slm"] = "为难的遗忘之物" ,
	["designer:kogasa_slm"] = "星野梦美☆",

	["illustrator:kogasa_slm"] = "にろ",
	["origin:kogasa_slm"] = "p号：26300308，个人id：194231",
	["illustrator:kogasa_slm_1"] = "竜",
	["origin:kogasa_slm_1"] = "p号：11868952，个人id：612224",
	["illustrator:kogasa_slm_2"] = "村上４時",
	["origin:kogasa_slm_2"] = "p号：27202944，个人id：309850",
	["illustrator:kogasa_slm_3"] = "プリンプリン",
	["origin:kogasa_slm_3"] = "p号：9172954，个人id：4179",

	["qingyu"] = "晴雨",
	[":qingyu"] = "当你受到伤害后，你可以将一张手牌当【兵粮寸断】置入你的判定区并选择所有体力值不小于你的其他角色，令这些角色各选择一项：弃置一张牌，或令你摸一张牌。" ,
	["@qingyu"] = "你可以发动“晴雨”，将一张手牌当做【兵粮寸断】置入你的判定区",
	["@qingyu-discard"] = "请弃置一张牌，否则 <font color=\"#00FF00\"><b>%src </b></font> 将摸一张牌。" ,
	["guoke"] = "过客",
	[":guoke"] = "当一张牌移出你的判定区后，你可以选择一项：摸一张牌，或回复1点体力。" ,
	["guoke:recover"] = "回复1点体力" ,
	["guoke:draw"] = "摸一张牌" ,
}
