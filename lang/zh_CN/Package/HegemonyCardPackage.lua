-- translation for HegemonyCardPackage

return {
	["hegemony_card"] = "国战标准版",
	
	
	["SixSwords"] = "吴六剑",
	[":SixSwords"] = "装备牌·武器\n\n攻击范围：2<br/>技能：锁定技，与你阵营相同的其他角色的攻击范围+1。",
	
	["befriend_attacking"] = "远交近攻",
	[":befriend_attacking"] = "锦囊牌\n\n使用时机：出牌阶段。\n使用目标：与你阵营不同的一名有明置的武将牌的角色。\n作用效果：目标角色摸一张牌，然后你摸三张牌。",

	["await_exhausted_hegemony"] = "以逸待劳(国)",
	[":await_exhausted_hegemony"] = "锦囊牌\n\n使用时机：出牌阶段。\n使用目标：你和与你阵营相同的所有角色。\n作用效果：每名目标角色摸两张牌，然后每名目标角色弃置两张牌。",

	["heg_nullification"] = "无懈可击·国",
	[":heg_nullification"] = "锦囊牌\n\n使用方法Ⅰ：\n使用时机：一张锦囊牌对一个目标生效前。\n使用目标：一张对一个目标生效前的锦囊牌。\n作用效果：抵消此锦囊牌。"
	.."\n\n使用方法Ⅱ：\n使用时机：一张锦囊牌对一名目标角色生效前。\n使用目标：一张对一名目标角色生效前的锦囊牌。\n作用效果：抵消此牌，然后你选择所有除目标角色外与目标角色阵营相同的角色，令所有角色不能使用【无懈可击】响应对这些角色结算的此牌，若如此做，每当此牌对你选择的这些角色中的一名角色生效前，抵消之。",
	["#heg_nullification"] = "无懈可击·国 对 %log",
	["heg_nullification:single"] = "无懈单个角色",
	["heg_nullification:all"] = "无懈之后同阵营的所有角色" ,
	
	["#HegNullificationDetails"] = "【<font color=\"yellow\"><b>无懈可击·国</b></font>】的目标是 %from 对 %to 的锦囊 【%arg】",
	["#HegNullificationEffect"] = "【<font color=\"yellow\"><b>无懈可击·国</b></font>】生效， 目标是 %from 对 %to 的锦囊 【%arg】",
	["#HegNullificationSelection"] = "%from 选择了该【<font color=\"yellow\"><b>无懈可击·国</b></font>】为 %arg" ,
	["hegnul_single"] = "对单一角色生效" ,
	["hegnul_all"] = "对该阵营的全体剩余角色生效" ,
	
	
	["known_both_hegemony"] = "知己知彼（国）",
	[":known_both_hegemony"] = "锦囊牌·非全体性的普通锦囊牌<br />使用时机：出牌阶段。<br />使用目标：一名有手牌或暗置人物牌的其他角色。<br />作用效果：你选择：<br />1.观看目标角色的一张暗置人物牌及身份牌 <br /> 2.观看目标角色手牌。<br />重铸时机：出牌阶段。<br />◆重铸：声明一张牌并将之置入处理区，然后将之置入弃牌堆，最后摸一张牌。",
	["known_both_hegemony:showhead"] = "主将",
	["known_both_hegemony:showdeputy"] = "副将",
	["known_both_hegemony:showcard"] = "手牌",
	["showhead"] = "主将",
	["showdeputy"] = "副将",
	["showcard"] = "手牌",
	--["known_both_hegemony:showcard"] = "明置手牌",
	["#KnownBothView"] = "%from 观看了 %to 的 %arg" ,
	["$KnownBothViewGeneral"] = "%from 观看了 %to 的 %arg， 阵营为 %arg2" ,
	
	
	["DoubleSwordHegemony"] = "雌雄剑（国）",
	[":DoubleSwordHegemony"] = "装备牌·武器<br />攻击范围：2<br />装备技能： 你使用【杀】指定有暗置武将牌的角色后，你可令其选择一项： 其明置一张武将牌并摸一张牌; 或其弃置一张手牌。",
	["DoubleSwordHegemony:showhead"] = "明置主将",
	["DoubleSwordHegemony:showdeputy"] = "明置副将",
}
