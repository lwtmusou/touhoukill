return{
	["thndj"] = "年代记SP",

	--ndj001  势力：主 4血
	["mokou_ndj"] = "年代记SP妹红" ,
	["&mokou_ndj"] = "年代记妹红" ,
	["#mokou_ndj"] = "炎杀姬" ,
	["designer:mokou_ndj"] = "星野梦美☆",

	["illustrator:mokou_ndj"] = "年代记",

	["rexue"]= "热血",
	[":rexue"]= "<font color=\"blue\"><b>锁定技，</b></font>你的非额外回合结束前，若你的体力为1，且此回合内没有角色死亡，你回复1点体力，然后摸一张牌，并在此回合结束后进行一个额外的回合。",
	["sidou"] = "死斗",
	[":sidou"] = "准备阶段开始时，你可以弃置场上的一张非武器牌。若如此做，你摸一张牌，然后对你造成1点火焰伤害。",
	["@sidou_target"]= "你可以指定一名角色，弃置其装备区或判定区里的一张非武器牌。",
	["tymhwuyu"]= "物语",
	[":tymhwuyu"]= "<font color=\"orange\"><b>主公技，</b></font><font color=\"blue\"><b>锁定技，</b></font>其他角色死亡时，视为由你杀死。",

--**********************************
	--ndj002  势力：主 3血
	["kaguya_ndj"] = "年代记SP辉夜" ,
	["&kaguya_ndj"] = "年代记辉夜" ,
	["#kaguya_ndj"] = "贤月公主" ,
	["designer:kaguya_ndj"] = "星野梦美☆",

	["illustrator:kaguya_ndj"] = "年代记",

	["huanyue"]= "幻月",
	[":huanyue"]= "当其他角色受到普通锦囊牌造成的伤害时，你可令其弃置你的一张手牌，若为黑色，此伤害+1。",
	["#huanyue_log"]= "%from 对 %to 的伤害由 %arg 点增加到 %arg2 点。",
	["huanyue:target"]= "<font color=\"#00FF00\"><b>%src </b></font> 受到 <font color=\"#00FF00\"><b>【%dest】 </b></font>的伤害， 你是否发动“幻月”。",
	["sizhai"] = "死宅",
	[":sizhai"] = "一名角色的结束阶段开始时，若其于此回合内没有使用或打出过非装备牌，你可以摸一张牌。",
	["sizhai:draw"] = "此回合 %src 没有使用或打出过非装备牌，你可以发动“死宅”，摸一张牌。",

	--**********************************
	--ndj003 势力：妖 3血
	["yukari_ndj"] = "年代记SP紫" ,
	["&yukari_ndj"] = "年代记紫" ,
	["#yukari_ndj"] = "境界中驻足的妖怪贤者" ,
	["designer:yukari_ndj"] = "辰焰天明",

	["illustrator:yukari_ndj"] = "年代记" ,

	["yuanhu"] = "援护" ,
	--[":yuanhu"] = "其他角色的摸牌阶段，其可以少摸一张牌，令你摸一张牌，若如此做，你可以将至多两张手牌交给该角色" ,
	[":yuanhu"] = "其他角色的摸牌阶段结束时，其可以交给你一张手牌，若如此做，你可以交给其至多两张牌。" ,
	["@yuanhu"] = "你可以发动 <font color=\"#FF8000\"><b>%src </b></font> 的“援护”， 交给其一张手牌。" ,
	["yuanhu:invoke"] = "您想发动 <font color=\"#FF8000\"><b>%src </b></font> 的“援护”吗？" ,
	["@yuanhu-exchange"] = "你可以交给 <font color=\"#FF8000\"><b>%src </b></font> 至多两张手牌。" ,
	["shouxie"] = "守楔" ,
	[":shouxie"] = "<font color=\"blue\"><b>锁定技，</b></font>若你的手牌数不大于七，你跳过弃牌阶段。结束阶段开始时，你将手牌补至手牌上限的张数。" ,

--**********************************
	--ndj004  势力：妖 4血
	["youmu_ndj"] = "年代记SP妖梦" ,
	["&youmu_ndj"] = "年代记妖梦" ,
	["#youmu_ndj"] = "修行的庭师" ,
	["designer:youmu_ndj"] = "星野梦美☆",

	["illustrator:youmu_ndj"] = "年代记",

	["hunpo"]= "魂魄",
	[":hunpo"]= "出牌阶段，若你的体力上限小于4，你可以弃置一张牌，增加1点体力上限。",
	["fanji"] = "反击",
	--[":fanji"] = "当你受到其他角色对你造成的伤害后，你可以对来源造成1点伤害。当你攻击范围内的角色受到另一名角色对其造成的伤害后，若来源不是你且你的体力上限大于1，你可以失去1点体力上限，对来源造成1点伤害。",
	[":fanji"] = "当你或你攻击范围内的角色受到另一名其他角色造成的伤害后，你可以失去1点体力或体力上限，对来源造成1点伤害。",
	["fanji:target"] = "<font color=\"#FF8000\"><b>%src </b></font> 对 <font color=\"#00FF00\"><b>%dest </b></font> 造成了伤害，你可以对 <font color=\"#FF8000\"><b>%src </b></font> 发动“反击”。",
	["fanji:hp"]= "失去体力",
	["fanji:maxhp"]= "扣减体力上限",
--**********************************
	--ndj010  势力：外 3血
	["merry_ndj"] = "年代记SP玛艾露贝莉" ,
	["&merry_ndj"] = "年代记梅莉" ,
	["#merry_ndj"] = "境界探访者" ,
	["designer:merry_ndj"] = "辰焰天明",

	["illustrator:merry_ndj"] = "年代记",

	["zaiwu"]= "载物",
	[":zaiwu"]= "其他角色的摸牌阶段：若你的体力值大于该角色，你可令其多摸一张牌；若你的体力值为1且不大于其，你可令其少摸一张牌。",
	["zaiwu:minus"]= "你可以发动“载物”，令<font color=\"#FF8000\"><b>%src </b></font> 少摸一张牌",
	["zaiwu:plus"]= "你可以发动“载物”， 令<font color=\"#FF8000\"><b>%src </b></font> 多摸一张牌",
	["liexi"] = "裂隙",
	[":liexi"]= "出牌阶段结束时，你可以视为使用【杀】（无距离限制）。当你以此法使用【杀】指定目标时，你令一名非目标的其他角色选择是否摸一张牌并成为目标，若其选择否，此牌无效。",
	["@liexi"]= "你可以发动“裂隙”， 视为使用【杀】",
	["~liexi"]= "选择“裂隙”使用【杀】的目标 -> 确定",
	["@liexi_extra"]= "你发动“裂隙”使用了【杀】， 你须指定一名非此杀目标的角色",
	["liexi_extra:target"]= "你可以响应 <font color=\"#00FF00\"><b>%dest </b></font> 的“裂隙”，成为 <font color=\"#FF8000\"><b>%src </b></font> 使用的【%arg】的额外目标。",
	["#liexi_extra"]= "%to 成为 %from 使用的 %arg 的额外目标。",
	["#liexi"] = "裂隙",
	
	["mengwei"] = "梦违",
	[":mengwei"] = "当你成为【杀】的目标时，你可以令一名非目标的其他角色选择是否摸一张牌并成为目标，若其选择是，当此【杀】对一名目标角色结算完毕后未造成伤害，此牌对其他目标角色无效。",
	["@mengwei"] = "梦违",
	["@mengwei"]= "<font color=\"#00FF00\"><b>%src </b></font>使用【杀】指定了你，你可以发动“梦违”，指定一名不为此杀目标的角色。",
	["mengwei_extra:target"]= "你可以响应 <font color=\"#00FF00\"><b>%dest </b></font> 的“梦违”，成为 <font color=\"#FF8000\"><b>%src </b></font> 使用的【%arg】的额外目标。",
	["#mengwei_extra"]= "%to 成为 %from 使用的 %arg 的额外目标。",
	["#MengweiNullified"] = "由于【%arg】的效果，%from 对 %to 使用的【%arg2】无效",
	--["mengwei"] = "梦违",
	--[":mengwei"] = "若你已受伤且有手牌，其他角色可以跳过其出牌阶段，令你回复1点体力，若如此做，你将一张牌交给该角色。",
	["mengwei_give"]= "<font color=\"#FF8000\"><b>%src </b></font> 发动了你的 “梦违”， 请交给其一张手牌",
     

--**********************************
	--ndj011  势力：外 3血
	["renko_ndj"] = "年代记SP莲子" ,
	["&renko_ndj"] = "年代记莲子",
	["#renko_ndj"] = "科学少女" ,
	["designer:renko_ndj"] = "星野梦美☆",

	["illustrator:renko_ndj"] = "年代记",

	["liangzi"]= "量子",
	[":liangzi"]= "<font color=\"blue\"><b>锁定技，</b></font>当你使用或打出基本牌后，你选择一项：横置，或重置。",
	["kexue"] = "科学",
	[":kexue"] = "若你处于连环状态，你于出牌阶段空闲时间点使用【杀】时无距离限制，且可以额外指定任意数量的其他角色为目标。",
    ["#kexue-effect"] = "科学",

--**********************************
	--ndj013  势力：风 4血
	["sanae_ndj"] = "年代记SP早苗" ,
	["&sanae_ndj"] = "年代记早苗",
	["#sanae_ndj"] = "风月战姬" ,
	["designer:sanae_ndj"] = "辰焰天明",

	["illustrator:sanae_ndj"] = "年代记",
	["xiubu"]= "修补",
	[":xiubu"]= "当一名角色于其出牌阶段内使用第一张牌时，若其为你或在你的攻击范围内，你可以弃置其一张手牌，令其于此阶段内使用最后一张手牌时无距离限制且不计入次数限制。若如此做，此阶段结束时，若其于此阶段内使用过不少于两张牌，其摸两张牌。",
	["@xiubu-self"] = "你可以发动<font color=\"#FF8000\"><b> 修补</b></font> 弃置一张手牌。",
    ["#xiubu-mod"]= "修补",
	
--**********************************	
	--ndj018 势力：风 3血
	["aya_ndj"] = "年代记SP文" ,
	["&aya_ndj"] = "年代记文",
	["#aya_ndj"] = "神风记者" ,
	["designer:aya_ndj"] = "辰焰天明",

	["illustrator:aya_ndj"] = "年代记",
	["jineng"]= "机能",
	[":jineng"]= "准备阶段开始时，或当你造成伤害后，你可判定，若结果的花色与人物牌上的“机”均不同，你将之置于人物牌上，称为“机”；你可将一张“机”按下列规则使用或打出：黑桃当【杀】（无距离限制），红桃当【闪】，梅花当【知己知彼】，方片当【酒】。",
	["jinengPile"]= "机",
	["kuaibao"]= "快报",
	[":kuaibao"]= "其他角色的准备阶段开始时，若“机”数大于你的体力值，你可获得所有“机”，对其造成1点伤害。",
	["$Kuaibao"] = "%from 发动 “%arg” ， 获得  %card ",

}
