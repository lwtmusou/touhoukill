
return{ ["th06"] = "红魔乡",
	["hmx"] = "红",
	--hmx001  势力：红 3血
	["remilia"] = "蕾米莉亚",
	["#remilia"] = "红之恶魔",
	["!remilia"] = "蕾米莉亚·斯卡雷特",
	["illustrator:remilia"] = "ke-ta",
	["origin:remilia"] = "p号：18583207，个人ID：3104565",
	["illustrator:remilia_1"] = "ザネリ",
	["origin:remilia_1"] = "p号：20900706，个人ID：739716",
	["illustrator:remilia_2"] = "Capura.L",
	["origin:remilia_2"] = "p号：5273512，个人ID：2475", --(台湾 非商业作品的话，可以自由用)
	["illustrator:remilia_3"] = "ナヅカ",
	["origin:remilia_3"] = "p号：32574830，个人ID：474458",
	["illustrator:remilia_4"] = "こぞう",
	["origin:remilia_4"] = "p号：40467146，个人ID：5626224",
	["illustrator:remilia_5"] = "れい亜",
	["origin:remilia_5"] = "p号：30891009，个人ID：444732",
	["illustrator:remilia_6"] = "晩杯あきら",
	["origin:remilia_6"] = "p号：27496231，个人ID：27452",
	["skltkexue"] = "渴血",
	[":skltkexue"] = "当你处于濒死状态并向其他角色求【桃】时，若该角色的体力值大于1，该角色可以失去1点体力并摸两张牌，然后令你回复1点体力。",
	["skltkexue_attach"]= "渴血出桃",
	[":skltkexue_attach"]= "当拥有“渴血”的角色处于濒死状态并向你求【桃】时，若你的体力值大于1，你可以失去1点体力并摸两张牌，然后令其回复1点体力。",
	["mingyun"] = "命运",
	[":mingyun"] = "当判定开始前，你可以观看牌堆顶的两张牌并获得其中的一张。",
	["mingyun:judge"] = "<font color=\"#00FF00\"><b>%src </b></font> 将要进行 “%dest” 的判定，你可以发动“命运”。",
	["xueyi"] = "血裔",
	[":xueyi"] = "<font color=\"orange\"><b>主公技，</b></font>当其他红势力角色回复体力后，该角色可令你摸一张牌。",
	["@@xueyi"]= "指定拥有主公技“血裔”的角色，该角色摸一张牌。",

--************************************************
	--hmx002  势力：红 3血
	["flandre"] = "芙兰朵露",
	["#flandre"] = "恶魔之妹",
	["!flandre"] = "芙兰朵露·斯卡雷特",
	["illustrator:flandre"] = "大嘘",
	["origin:flandre"] = "个人ID：457541",
	["illustrator:flandre_1"] = "ideolo",
	["origin:flandre_1"] = "p号：32092803，个人ID：61513", --（国人）
	["illustrator:flandre_2"] = "茨乃",
	["origin:flandre_2"] = "p号：752120，个人ID：67388",
	["illustrator:flandre_3"] = "赤りんご",
	["origin:flandre_3"] = "p号：7768620，个人ID：164813", --(貌似可以用的样子。。但其个人网站乱码 确认不能。。。)
	["illustrator:flandre_4"] = "6U",
	["origin:flandre_4"] = "p号：45272630，个人ID：94883",
	["pohuai"] = "破坏",
	[":pohuai"] = "<font color=\"blue\"><b>锁定技，</b></font>准备阶段开始时，你判定，若结果为【杀】，你指定距离不大于1的所有角色，你对这些角色依次造成1点伤害。",
	["yuxue"] = "浴血",
	[":yuxue"] = "当你受到伤害后，你可以使用一张【杀】，此【杀】无距离限制，且造成的伤害+1。",
	["@yuxue"] = "你可以发动“浴血”，使用一张【杀】。",
	["#yuxue_damage"]= "%from的“%arg”效果被触发,%from对%to的伤害+1",
	["shengyan"] = "盛宴",
	[":shengyan"] = "每当你造成1点伤害后，你可以摸一张牌。",

--************************************************
	--hmx003  势力：红 4血
	["sakuya"] = "十六夜咲夜",
	["#sakuya"] = "完美潇洒的从者",
	["illustrator:sakuya"] = "JQ3C273!",
	["origin:sakuya"] = "p号：9734150，个人ID：431033", --(国人)
	["illustrator:sakuya_1"] = "かなりあ",
	["origin:sakuya_1"] = "p号：2862235，个人ID：180460",
	["illustrator:sakuya_2"] = "刃天",
	["origin:sakuya_2"] = "p号：21597876，个人ID：512849", --（国人 可以用）
	["illustrator:sakuya_3"] = "赤りんご",
	["origin:sakuya_3"] = "个人ID：164813", --(貌似可以用的样子。。但其个人网站乱码 确认不能。。。)
	["illustrator:sakuya_4"] = "ジョンディー",
	["origin:sakuya_4"] = "p号：33681713，个人ID：1686747",
	["illustrator:sakuya_5"] = "藤宮ふみや",
	["origin:sakuya_5"] = "p号：37762298，个人ID：4556900",
	["suoding"] = "锁定",
	[":suoding"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以依次将一至三名角色的共计一至三张手牌面朝下置于该角色旁，直到此回合结束或你死亡。",
	["suoding_cards"] = "锁定",
	["suoding_exchange"] = "你对你发动了“锁定”，请选择锁定牌 %src 张。",
	["#suoding_Trigger"]= "%from 的 “%arg” 被触发， %to 收回了 %arg2 张锁定牌。",
	["huisu"] = "回溯",
	[":huisu"] = "当你受到伤害后，你可以判定，若结果为红色2-9，你回复1点体力。当你失去体力后，你可以判定，若结果为<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>2-9，你回复1点体力。",
	["huisu1"] = "回溯（伤害）",
	["huisu2"] = "回溯（体力流失）",

--************************************************
	--hmx004  势力：红 3血
	["patchouli"] = "帕秋莉",
	["#patchouli"] = "不动的大图书馆",
	["!patchouli"] = "帕秋莉·诺蕾姬",
	["illustrator:patchouli"] = "萩原",
	["origin:patchouli_1"] = "个人ID：37062",
	["illustrator:patchouli_1"] = "ryosios",
	["origin:patchouli_1"] = "p号：41382219，个人ID：1508165", --（作者表态可用）
	["illustrator:patchouli_2"] = "パンチ",
	["origin:patchouli_2"] = "p号：5182740，个人ID：48755",
	["illustrator:patchouli_3"] = "miya9",
	["origin:patchouli_3"] = "p号：7603627，个人ID：110090",
	["illustrator:patchouli_4"] = "希",
	["origin:patchouli_4"] = "p号：4780745，个人ID：92364",
	["illustrator:patchouli_5"] = "木子翔",
	["origin:patchouli_5"] = "p号：33195548，个人ID：652180",--（国人？）
	["bolan"] = "博览",
	[":bolan"] = "当其他角色使用的非延时类锦囊牌（【无懈可击】除外）在结算后置入弃牌堆后，你可以将其置于你的人物牌上，称为“曜”。此回合结束时，你须将所有“曜”加入手牌，然后将手牌弃至体力上限的张数。",
	["#bolan_Invoke"]= "%from的“%arg”被触发，%from获得了%arg2张“曜”。",
	["#bolan"] = "博览",
	["yao_mark"] = "曜",
	["bolan:obtain"] = "你可以发动“博览”，将【%src】作为“曜”置于你的人物牌上。",
	["bolan_discard"]= "“博览”：收回“曜”后，请弃置%src张手牌。",
	["qiyao"] = "七曜",
	[":qiyao"] = "你可以于你的回合外将一张非延时类锦囊牌当【桃】使用。",

--************************************************
	--hmx005  势力：红 4血
	["meirin"] = "红美铃",
	["#meirin"] = "华人小姑娘",
	["illustrator:meirin"] = "yudofu",
	["origin:meirin"] = "p号：3804946",
	["illustrator:meirin_1"] = "NEKO",
	["origin:meirin_1"] = "p号：35872256，个人ID：2600911", --（国人）
	["illustrator:meirin_2"] = "ke-ta",
	["origin:meirin_2"] = "p号：18637750，个人ID：3104565",
	["illustrator:meirin_3"] = "もりのほん",
	["origin:meirin_3"] = "p号：1816285，个人ID：258411",
	["illustrator:meirin_4"] = "もねてぃ",
	["origin:meirin_4"] = "p号：35513635，个人ID：3066815",
	["neijin"] = "内劲",
	[":neijin"] = "其他角色的出牌阶段结束时，若你不处于连环状态且你的手牌数小于体力上限，你可以横置并将手牌摸至体力上限的张数，然后将X张手牌交给该角色（X为你以此法摸牌的数量）。",
	["neijin_exchange"] = "请将<font color=\"#00FF00\"><b>%dest </b></font> 张手牌交给当前回合人 <font color=\"#00FF00\"><b>%src </b></font>。",
	["taiji"] = "太极",
	[":taiji"] = "当你受到非属性伤害后，若你处于连环状态，你可以重置，然后对一名其他角色造成1点伤害。",
	["@taiji"] = "你可以发动“太极”，将人物牌重置，对一名其他角色造成一点伤害。",

--************************************************
	--hmx006  势力：红 3血
	["cirno"] = "琪露诺",
	["#cirno"] = "湖上的冰精",
	["illustrator:cirno"] = "ちんちくりん",
	["origin:cirno"] = "p号：7616113，个人ID：3401",
	["illustrator:cirno_1"] = "6U",
	["origin:cirno_1"] = "p号：10269990，个人ID：94883", --(第一条禁止 第三条 允许 好矛盾。。。)
	["illustrator:cirno_2"] = "あくぁまりん",
	["origin:cirno_2"] = "p号：1441901，个人ID：70728",
	["illustrator:cirno_3"] = "しょういん",
	["origin:cirno_3"] = "p号：39005285，个人ID：2148690",
	["illustrator:cirno_4"] = "栗",
	["origin:cirno_4"] = "p号：20779267，个人ID：36168",
	["dongjie"] = "冻结",
	[":dongjie"] = "当你使用【杀】对目标角色造成伤害时，你可以防止此伤害并摸一张牌，然后该角色翻面并摸一张牌。",
	["#Dongjie"] = "%from 发动了“%arg”，防止了对 %to 的伤害",
	["bingpo"] = "冰魄",
	[":bingpo"] = "<font color=\"blue\"><b>锁定技，</b></font>当你受到火焰伤害以外的伤害时，若此伤害多于1点或你的体力值不大于1，防止此伤害。",
	["#bingpolog"] = "%from的“%arg”被触发, %from 防止了%arg2点伤害.",
	["bendan"] = "笨蛋",
	[":bendan"] = "<font color=\"blue\"><b>锁定技，</b></font>你的牌的点数均视为9。",

--************************************************
	--hmx007  势力：红 3血
	["rumia"] = "露米娅",
	["#rumia"] = "宵暗的妖怪",
	["illustrator:rumia"] = "ポップル",
	["origin:rumia"] = "p号：6034882，个人ID：888775", --（貌似本人从p站退了）
	["illustrator:rumia_1"] = "蘇我サクラ",
	["origin:rumia_1"] = "p号：42134846，个人ID：809115",
	["illustrator:rumia_2"] = "晩杯あきら",
	["origin:rumia_2"] = "p号：6272599，个人ID：27452",
	["illustrator:rumia_3"] = "黒夢",
	["origin:rumia_3"] = "p号：30635333，个人ID：1350867",
	["illustrator:rumia_4"] = "黒夢",
	["origin:rumia_4"] = "p号：35541348，个人ID：1350867",
	["zhenye"] = "真夜",
	[":zhenye"] = "结束阶段开始时，你可以翻面，然后令一名其他角色翻面。",
	["@zhenye-select"]= "你可以指定一名其他角色，你和该角色依次翻面。",
	["anyu"] = "暗域",
	[":anyu"] = "<font color=\"blue\"><b>锁定技，</b></font>当你成为黑色【杀】或黑色非延时类锦囊牌的目标时，你选择一项：摸一张牌，或翻面。",
	["anyu:turnover"] = "翻面",
	["anyu:draw"] = "摸一张牌" ,

--************************************************
	--hmx008  势力：红 3血
	["koakuma"] = "小恶魔",
	["#koakuma"] = "图书馆中的使魔",
	["illustrator:koakuma"] = "cercis",
	["illustrator:koakuma_1"] = "ぎん太郎",
	["origin:koakuma_1"] = "p号：8593014，个人ID：95886",
	["illustrator:koakuma_2"] = "犯人はサム",
	["origin:koakuma_2"] = "p号：9470640，个人ID：99026",
	["illustrator:koakuma_3"] = "yamasan",
	["origin:koakuma_3"] = "p号：15952878，个人ID：346855",
	["illustrator:koakuma_4"] = "しろさ",
	["origin:koakuma_4"] = "p号：27795527，个人ID：374262",
	["qiyue"] = "契约",
	[":qiyue"] = "其他角色的准备阶段开始时，你可以先摸一张牌再选择一项：1.失去1点体力；2.减1点体力上限。若如此做，该角色跳过判定阶段和摸牌阶段。",
	["qiyue:maxhp"]= "失去1点体力上限",
	["qiyue:hp"]= "失去1点体力",
	["qiyue:target"] = "你可以发动“契约”，令<font color=\"#00FF00\"><b>%src </b></font>跳过此回合的判定阶段和摸牌阶段。",
	["moxue"] = "魔血",
	[":moxue"] = "<font color=\"blue\"><b>锁定技，</b></font>当你的体力上限扣减至1时，你摸X张牌（X为你的手牌数且最少为1）。",
	["$moxueAnimate"]= "skill=koakuma:moxue",

--************************************************
	--hmx009  势力：红 3血
	["daiyousei"]= "大妖精",
	["#daiyousei"]= "雾之湖畔的妖精",
	["illustrator:daiyousei"] = "住吉眞白",
	["origin:daiyousei"] = "p号：19870726，个人ID：2388526",
	["illustrator:daiyousei_1"] = "伊吹のつ",
	["origin:daiyousei_1"] = "p号：34272295，个人ID：7013",
	["illustrator:daiyousei_2"] = "うぬ",
	["origin:daiyousei_2"] = "p号：42578781，个人ID：444909",
	["juxian"]= "具现",
	[":juxian"]= "当你进入濒死状态时，若你的人物牌正面朝上，你可以翻面，亮出牌堆顶的三张牌，然后选择一种花色并获得与所选花色不同的牌，将其余的牌置入弃牌堆并回复等量的体力。",
	["banyue"]= "半月",
	[":banyue"]= "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以失去1点体力，令一至三名角色依次摸一张牌。",

	--hmx010
	["sakuya_sp"] = "sp咲夜" ,
	["#sakuya_sp"] = "红魔馆的女仆" ,

}




