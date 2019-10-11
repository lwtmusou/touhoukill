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
	["heg_nullification:single"] = "为%to无懈此锦囊",
	["heg_nullification:all"] = "为%to及之后的%log阵营角色无懈此锦囊" ,
	
	["#HegNullificationDetails"] = "【<font color=\"yellow\"><b>无懈可击·国</b></font>】的目标是 %from 对 %to 的锦囊 【%arg】",
	["#HegNullificationEffect"] = "【<font color=\"yellow\"><b>无懈可击·国</b></font>】生效， 目标是 %from 对 %to 的锦囊 【%arg】",
	["#HegNullificationSelection"] = "%from 选择了该【<font color=\"yellow\"><b>无懈可击·国</b></font>】为 %arg" ,
	["hegnul_single"] = "对单一角色生效" ,
	["hegnul_all"] = "对该阵营的全体剩余角色生效" ,
}
