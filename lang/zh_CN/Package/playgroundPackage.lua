return {
	["playground"] = "游乐场包",

	-- removed skills
	["fsu0413gepi"] = "嗝屁",
	[":fsu0413gepi"] = "一名角色的准备阶段开始时，你可以令其弃置你的一张牌，若如此做，你令其一个技能（主公技、觉醒技和永久技除外）于此回合内无效，若以此法无效描述中不带“出牌阶段”的技能或不能无效技能时，其摸三张牌。" ,
	["$Fsu0413GepiNullify"] = "%from 发动 <font color=\"yellow\"><b>嗝屁</b></font> 令 %to 的技能 %arg 于本回合无效",
	["$Fsu0413GepiReset"] = "%from 的回合结束，<font color=\"yellow\"><b>嗝屁</b></font> 的效果消失，%arg 恢复有效",
	["@fsu0413gepi-discard"] = "你可以弃置 1 涨牌发动 “嗝屁”。",
	["fsu0413gepiPlay"] = "出牌阶段",

	["fsu0413sile"] = "死了",
	[":fsu0413sile"] = "。",

--************************************************
	-- GG GGG 势力：神 5血
	["#fsu0413"] = "咕咕咕开发者",
	["fsu0413"] = "咕咕Fs",
	["designer:fsu0413"] = "Fs",
	["illustrator:fsu0413"] = "Fs",
	["origin:fsu0413"] = "帅气自拍",

	["fsu0413gainian"] = "概念",
	[":fsu0413gainian"] = "你可以将一张延时类锦囊牌当任意延时类锦囊牌使用（无距离限制）。",

	["fsu0413lese"] = "了塞",
	[":fsu0413lese"] = "<font color=blue><b>锁定技，</b></font>摸牌阶段，你随机获得弃牌堆中的一张延时类锦囊牌。",

--************************************************
	-- BM WHFY 势力：神 5血
	["#benmao"] = "笨猫",
	["benmao"] = "神 蔚海幽浮",
	["designer:benmao"] = "吉八弔千慧",
	["illustrator:benmao"] = "猫星人",
	["origin:benmao"] = "原形毕露",

	["cv:benmao"] = "Tencent ZhiYan ft. Cat",
	["&benmao"] = "神笨猫",
	["origin:benmao"] = "猫星人",
	["bmmaoji"] = "猫击",
	[":bmmaoji"] = "<font color=blue><b>锁定技，</b></font>你的手牌视为幻【杀】；当你使用【杀】对一个目标生效时，你将其抵消此【杀】的方式改为依次打出两张【杀】；当你成为一名未拥有“猫击”角色使用【杀】的目标后，你令其获得其装备区里的所有牌，然后令其获得“猫击”。",
	["@bmmaoji-slash-start"] = "%src 对你使用了【杀】并触发了“猫击”，你需依次打出 %arg 张【杀】来抵消。",
	["@bmmaoji-slash"] = "%src 触发了“猫击”，你还需要依次打出 %arg 张【杀】。",
	["#bmmaoji-conflictingskill"] = "%from 拥有的技能 <font color=\"yellow\"><b>%arg</b></font> 与 <font color=\"yellow\"><b>猫击</b></font> 冲突，因此将失去技能<font color=\"yellow\"><b>%arg</b></font>。",

	["bmbenti"] = "本体",
	[":bmbenti"] = "<font color=blue><b>锁定技，</b></font>你的攻击范围为X（X为拥有技能“猫击”的角色个数）",

	["$bmmaoji1"] = "喵",
	["$bmmaoji2"] = "喵",
	["$bmmaoji3"] = "笨猫笨猫笨",
	["~benmao"] = "喵",

--************************************************
	-- FS FZ 势力：神 5血
	["otaku"] = "肥宅",
	["#otaku"] = "死肥宅",
	["designer:otaku"] = "F宅·s胖子",
	["origin:otaku"] = "精通二次元",

	["fsu0413fei2zhai"] = "肥宅",
	[":fsu0413fei2zhai"] = "<font color=\"#808080\"><b>永久技，</b></font>当一张【桃】或仙【桃】不因使用而置入弃牌堆后，你获得之；你跳过弃牌阶段。",
	["fsu0413fei4zhai"] = "废宅",
	[":fsu0413fei4zhai"] = "<font color=\"#808080\"><b>永久技，</b></font>当你因【桃】或仙【桃】的效果回复体力时，若你不处于濒死状态，你防止此回复体力。",

--************************************************
	-- BD WMSSJS 势力：神 9血
	["god9"] = "⑨",
	["#god9"] = "算数天才",
	["designer:god9"] = "飞天明",
	["illustrator:god9"] = "DomotoLain",
	["origin:god9"] = "p号：6160883；个人id：38674" ,

	["ftmsuanshu"] = "算数",
	[":ftmsuanshu"] = "<font color=blue><b>锁定技，</b></font>若你的手牌数为⑨，你所有的手牌视为【无中生有】。",

--************************************************
	-- ZJ FT 势力：神 4血
	["flyingskybright"] = "辰焰天明",
	["#flyingskybright"] = "飞天明",
	["designer:flyingskybright"] = "飞天明",

	["illustrator:flyingskybright"] = "谁",
	["origin:flyingskybright"] = "多少" ,

	["ftmfeitian"] = "飞天",
	[":ftmfeitian"] = "<font color=blue><b>锁定技，</b></font>当你使用黑色牌时，你获得X+1枚“飞”标记；当你使用红色牌时，你获得X枚“飞”标记。结束阶段开始时，若X为：0，你摸两张牌；一名角色的座次，你选择令其回复或失去1点体力；牌堆牌数，你获得牌堆。回合结束时，你弃所有“飞”标记。（X为“飞”标记数）",
	["ftmfeitian_x:recover-or-losehp"] = "飞天：点击“确定”令 %src 失去 1 点体力，或者点击“取消”令 %src 回复 1 点体力。",
	["@flying"] = "飞",

--************************************************
	-- （编号未定） 势力：神 2血
	["tailormokou"] = "神 藤原妹红",
	["&tailormokou"] = "神藤原妹红",
	["designer:tailormokou"] = "裁月镌云",

	["illustrator:tailormokou"] = "谁",
	["origin:tailormokou"] = "多少",

	["tailorfuzhong"] = "覆踵",
	[":tailorfuzhong"] = "<font color=\"#808080\"><b>永久技，</b></font>你不进入濒死状态；你死亡时，你可以选择一个非永久技的技能，令你的人物牌失去之，若如此做，你于死亡后以游戏开始时的状态重新加入游戏。",
	["tailorchenglu"] = "澄路",
	[":tailorchenglu"] = "<font color=\"#808080\"><b>永久技，</b></font><font color=\"purple\"><b>觉醒技</b></font>，你首次重新加入游戏时，若你没有【泯心】，你令你的人物牌获得【护卫】并令你的人物牌失去【覆踵】；否则你令你的人物牌获得【凯风】。",
	["$tailorchengluAnimate"] = "skill=tailormokou:tailorchenglu",
	["tailorminxin"] = "泯心",
	[":tailorminxin"] = "<font color=\"blue\"><b>锁定技，</b></font>你的体力值扣减至0以下后，体力下限+1，对一名角色造成1点火焰伤害。",
	["tailorminxin-select"] = "泯心你选择一名角色对齐造成 1 点火焰伤害",
	["tailormiezui"] = "灭罪",
	[":tailormiezui"] = "<font color=\"green\"><b>对每名角色限一次，</b></font>当你对一名角色造成伤害后，若此伤害令其进入濒死状态，且其曾造成过令一名角色进入濒死状态的伤害，你可以选择不大于2的非负整数，令你的体力下限 -X（X为你此次选择的数），若你因此法体力下限变为1，你可以死亡。",
	["tailormiezui:dismiss"] = "取消",
	["tailormiezui:death"] = "灭罪你可以死亡",
}
