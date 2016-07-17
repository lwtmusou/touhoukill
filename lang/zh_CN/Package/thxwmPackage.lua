return{
	["thxwm"] = "线外萌SP" ,

	--xwm001  势力：地 3血
	["satori_xwm"] = "线外萌SP觉",
	["&satori_xwm"] = "线外萌觉",
	["#satori_xwm"] = "第一届线外萌萌王",
	["illustrator:satori_xwm"] = "ぎん太郎",
	["origin:satori_xwm"] = "p号：16388840，个人ID：95886",
	["illustrator:satori_xwm_1"] = "皆村",
	["origin:satori_xwm_1"] = "p号：28695491，个人ID：6098",
	["kongpiao"] = "控票",
	[":kongpiao"] = "<font color=\"blue\"><b>锁定技，</b></font>其他角色的出牌阶段开始时，若该角色的手牌数多于你且你的手牌数小于五，你摸一张牌。",
	["shouhuivs"] = "手绘",
	[":shouhui"] = "出牌阶段或当你处于濒死状态并向你求【桃】时，你可以弃置X张装备牌，然后摸X张牌。",
	["@LuaShouhuiD"] = "请选择任意数量的装备牌。",
	["shouhui"] = "手绘",
	["woyu"] = "我域",
	["woyu"] = "我域",
	["@Woyu"] = "我域",
	[":woyu"] = "<font color=\"orange\"><b>主公技，</b></font><font color=\"red\"><b>限定技，</b></font>出牌阶段，你可令一名身份牌暗置的角色将其身份牌明置：若为忠臣，你弃置所有牌；若为反贼，你摸三张牌。",
	["#WoyuAnnounce"] = "%from 发动了 <font color=\"red\"><b>我域</b></font>，展示了 %arg2 的身份牌，为 %arg 。",
	["$woyuAnimate"]= "skill=satori_xwm:woyu",

--*********************************
	--xwm002  势力：妖 4血
	["yuyuko_xwm"] = "线外萌SP幽幽子",
	["&yuyuko_xwm"] = "线外萌幽幽子",
	["#yuyuko_xwm"] = "第一届线外萌准萌",
	["illustrator:yuyuko_xwm"] = "藤原",
	["origin:yuyuko_xwm"] = "p号：17184292，个人ID：27517",
	["illustrator:yuyuko_xwm_1"] = "はいばね",
	["origin:yuyuko_xwm_1"] = "p号：22145913",
	["beisha"] = "倍杀",
	[":beisha"] = "准备阶段开始时，若你有手牌，你可以选择一项：视为你对一名手牌数不大于你的其他角色使用一张【杀】，或令一名手牌数不大于你的一半的其他角色失去1点体力。",
	["@beisha"] = "请指定“倍杀”的目标。",
	["beisha:useslash"] = "视为对其使用一张【杀】",
	["beisha:losehp"] = "令其失去1点体力",

--*********************************
	--xwm003  势力：主 4血
	["marisa_xwm"] = "线外萌SP魔理沙",
	["&marisa_xwm"] = "线外萌魔理沙",
	["#marisa_xwm"] = "第一届线外萌四强",
	["illustrator:marisa_xwm"] = "甘党",
	["origin:marisa_xwm"] = "p号：8937985，个人ID：298982",
	["illustrator:marisa_xwm_1"] = "かがよ",
	["origin:marisa_xwm_1"] = "p号：32950706，个人ID：693172",
	["xisan"] = "吸散",
	[":xisan"] = "一名角色的准备阶段开始时，你可选择一项：将手牌摸至三张；或弃置任意数量的手牌，然后将手牌摸至两张。",
	["@xisan-discard"] = "弃置任意数量的手牌，然后将手牌摸至两张" ,
	["xisan:a"]= "将手牌摸至三张",
	["xisan:b"]= "先弃置任意数量手牌，再将手牌摸至两张。",

--*********************************
	--xwm004  势力：战 3血
	["aya_xwm"] = "线外萌SP文",
	["&aya_xwm"] = "线外萌文",
	["#aya_xwm"] = "第一届线外萌四强",
	["illustrator:aya_xwm"] = "AUER",
	["origin:aya_xwm"] = "p号：12802720，个人ID：178301", --（国人）
	["illustrator:aya_xwm_1"] = "RiE",
	["origin:aya_xwm_1"] = "p号：27409141，个人ID：400684",
	["jubao"] = "举报", -- 菊爆？
	[":jubao"] = "当你受到伤害后，你可以弃置一名角色的一张手牌，然后弃置该角色的装备区里的所有与此牌颜色相同的牌。",
	["@jubao-select"] = "你可以发动“举报”，指定一名角色，弃置其一张手牌。",
	["haidi"] = "海底",
	[":haidi"] = "当你失去最后的手牌时，你可以回复1点体力。",

--*********************************
	--xwm005  势力：永 3血
	["mokou_xwm"] = "线外萌SP妹红",
	["&mokou_xwm"] = "线外萌妹红",
	["#mokou_xwm"] = "第二届线外萌萌王",
	["illustrator:mokou_xwm"] = "Lowlight.K",
	["origin:mokou_xwm"] = "p号：10738620，个人ID：904187",--（国人？）
	["illustrator:mokou_xwm_1"] = "saku",
	["origin:mokou_xwm_1"] = "p号：2318512，个人ID：19937",
	["shanji"] = "闪击",
	[":shanji"] = "分发起始手牌时，共发你十张牌，选四张作为手牌，将其余的牌面朝下置于你的人物牌上，称为“票”。你可将“票”视为你手牌般使用或打出。",
	["piao"]= "票",
	["@shanji"]= "你可以把“票”视为你手牌般使用或打出。",
	["~shanji"]= "请选择使用的“票”和目标。",
	["#shanji"] = "%from 使用/打出了 %arg 张 %arg2 牌。",
	["yazhi"] = "压制",
	[":yazhi"]= "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以将一张手牌作为“票”面朝下置于你的人物牌上。",
	["tianxiang"] = "天翔",
	[":tianxiang"] = "<font color=\"orange\"><b>主公技，</b></font><font color=\"blue\"><b>锁定技，</b></font>游戏开始时，将反贼的胜利条件改为：主公、忠臣、内奸全部死亡。将内奸的胜利条件改为：其他角色全部死亡。",

--*********************************
	--xwm006  势力：红 3血
	["remilia_xwm"] = "线外萌SP蕾米莉亚",
	["&remilia_xwm"] = "线外萌蕾米",
	["#remilia_xwm"] = "第二届线外萌准萌",
	["illustrator:remilia_xwm"] = "とりのあくあ",
	["origin:remilia_xwm"] = "p号：21693827，个人ID：1960050",
	["illustrator:remilia_xwm_1"] = "甘党",
	["origin:remilia_xwm_1"] = "p号：29099056，个人ID：298982",
	["qingcang"] = "倾仓",
	[":qingcang"] = "摸牌阶段，若你的判定区里没有延时类锦囊牌，你可以多摸四张牌，然后将一张手牌当【兵粮寸断】置于你的判定区里。",
	["@qingcang-card"] = "选择一张手牌,将其当【兵粮寸断】置于你的判定区里。",
	["changqing"] = "常青",
	[":changqing"] = "<font color=\"blue\"><b>锁定技，</b></font>当你进入濒死状态时，若角色数不小于5，你回复1点体力。",
}
