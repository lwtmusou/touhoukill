-- translation for ManeuveringPackage

return {
	["maneuvering"] = "军争篇",

	["fire_slash"] = "火杀",
	[":fire_slash"] = "基本牌<br />使用时机：出牌阶段限一次。<br />使用目标：你攻击范围内的一名其他角色。<br />作用效果：你对目标角色造成<font color=\"green\"><b>1点</b></font>火焰伤害",

	["thunder_slash"] = "雷杀",
	[":thunder_slash"] = "基本牌<br />使用时机：出牌阶段限一次。<br />使用目标：你攻击范围内的一名其他角色。<br />作用效果：你对目标角色造成<font color=\"green\"><b>1点</b></font>雷电伤害",

	["analeptic"] = "酒",
	[":analeptic"] = "基本牌<br />使用方法①<br />使用时机：出牌阶段，每回合限一次。<br />使用目标：包括你在内的一名角色。<br />作用效果：目标角色于此回合内使用的下一张【杀】的伤害值基数+<font color=\"green\"><b>1</b></font>。<br />◆一名角色使用的【杀】是否会受到【酒】（使用方法①）的效果的影响是根据其声明使用的牌的牌名（【杀】）时是否为其使用【酒】（使用方法①）的那个回合内来判断的。<br />◆不处于濒死状态的角色使用【酒】默认是以使用方法①使用【酒】。<br /> <br />使用方法②<br />使用时机：当你处于濒死状态时。<br />使用目标：你。<br />作用效果：你回复<font color=\"green\"><b>1点</b></font>体力。<br />◆处于濒死状态的角色使用【酒】默认是以使用方法②使用【酒】。",

	["#UnsetDrankEndOfTurn"] = "%from 此阶段获得的 【<font color=\"yellow\"><b>酒</b></font>】的效果消失",

	["Fan"] = "朱雀羽扇",
	[":Fan"] = "装备牌·武器<br />攻击范围：4<br />装备技能：当你使用【杀】指定目标时，若此牌为：非属性【杀】，你可以令之视为火【杀】；火【杀】，你可以摸一张牌。",

	["GudingBlade"] = "古锭刀",
	[":GudingBlade"] = "装备牌·武器<br />攻击范围：2<br />装备技能：<font color=\"blue\"><b>锁定技，</b></font>当你使用【杀】对目标角色造成伤害时，若其没有手牌，你令伤害值+1。",
	["#GudingBladeEffect"] = "%from 的【<font color=\"yellow\"><b>古锭刀</b></font>】效果被触发， %to 没有手牌，伤害从 %arg 增加至 %arg2",

	["Vine"] = "藤甲",
	[":Vine"] = "装备牌·防具<br />装备技能：<font color=\"blue\"><b>锁定技，</b></font>【南蛮入侵】、【万箭齐发】、非属性【杀】对你无效；当你受到火焰伤害时，你令伤害值+1。",
	["#VineDamage"] = "%from 的防具【<font color=\"yellow\"><b>藤甲</b></font>】效果被触发，火焰伤害由 %arg 点增加至 %arg2 点",

	["SilverLion"] = "白银狮子",
	[":SilverLion"] = "装备牌·防具<br />装备技能：<font color=\"blue\"><b>锁定技，</b></font>当你受到大于1点的伤害时，你将伤害值改为1点；当你失去装备区里的【白银狮子】后，你回复1点体力。",
	["#SilverLion"] = "%from 的防具【%arg2】将 %arg 点伤害的伤害值改为了 <font color=\"yellow\"><b>1</b></font> 点",

	["fire_attack"] = "火攻",
	["@fire_attack_show"] = "你因受到【火攻】的作用效果， 请展示 %src 张手牌",
	["@fire-attack"] = "展示的牌的花色为 <font color=\"green\"><b>%src %dest %arg %arg2</b></font>，请弃置一张与其中的一种花色相同的手牌",
	[":fire_attack"] = "锦囊牌<br />使用时机：出牌阶段。<br />使用目标：一名有手牌的角色。<br />作用效果：目标角色展示<font color=\"green\"><b>一张</b></font>手牌，然后你可以弃置一张与其中的一张牌花色相同的手牌，若你如此做，你对其造成<font color=\"green\"><b>1点</b></font>火焰伤害。",
	["fire-attack-card"] = "您可以弃置一张与 %dest 所展示的牌中的一张牌花色相同的牌，对 %dest 造成1点火焰伤害",

	["iron_chain"] = "铁索连环",
	[":iron_chain"] = "锦囊牌·非全体性的普通锦囊牌<br />使用时机：出牌阶段。<br />使用目标：一至两名角色。<br />作用效果：目标角色选择一项：横置；或重置。 <br />重铸时机：出牌阶段。<br />◆重铸：声明一张牌并将之置入处理区，然后将之置入弃牌堆，最后摸一张牌。",

	["supply_shortage"] = "兵粮寸断",
	[":supply_shortage"] = "锦囊牌·一次性的延时类锦囊牌<br />使用时机：出牌阶段。<br />使用目标：距离为1的一名其他角色。<br />作用效果：目标角色判定，若结果不为<font size=\"5\", color=\"#808080\"><b>♠</b></font>，其跳过摸牌阶段。",

	["HuaLiu"] = "骅骝",

	["IronArmor"] = "明光铠",
	[":IronArmor"] = "装备牌·防具<br /><b>装备技能</b>：<font color=\"blue\"><b>锁定技，</b></font>属性【杀】或【火攻】或【铁索连环】对你无效。",
}
