return{
	["thndj"] = "年代记SP",

	--ndj001  势力：主 4血
	["mokou_ndj"] = "年代记SP妹红" ,
	["&mokou_ndj"] = "年代记妹红" ,
	["#mokou_ndj"] = "炎杀姬" ,
	["designer:mokou_ndj"] = "星野梦美☆",

	["illustrator:mokou_ndj"] = "年代记",

	["rexue"]= "热血",
	[":rexue"]= "<font color=\"blue\"><b>锁定技，</b></font>非额外的回合结束时，若你的体力值为1且没有角色于此回合内死亡，你回复1点体力，然后摸一张牌，获得一个额外回合。",
	["sidou"] = "死斗",
	[":sidou"] = "准备阶段开始时，你可以弃置场上的一张非武器牌。摸一张牌，然后对你造成1点火焰伤害。",
	["@sidou_target"]= "你可以选择一名装备区里或判定区里有非武器牌的角色，弃置其装备区或判定区里的一张非武器牌。",
	["tymhwuyu"]= "物语",
	[":tymhwuyu"]= "<font color=\"orange\"><b>主公技，</b></font><font color=\"blue\"><b>锁定技，</b></font>当其他角色死亡时，视为由你杀死。",

--**********************************
	--ndj002  势力：主 3血
	["kaguya_ndj"] = "年代记SP辉夜" ,
	["&kaguya_ndj"] = "年代记辉夜" ,
	["#kaguya_ndj"] = "贤月公主" ,
	["designer:kaguya_ndj"] = "辰焰天明",

	["illustrator:kaguya_ndj"] = "年代记",

	["huanyue"]= "幻月",
	[":huanyue"]= "当你使用牌造成或受到牌造成的伤害后，你可以将此牌置于人物牌上，称为“符”，然后选择一张“符”，将除此“符”外的“符”置入弃牌堆；当其他角色受到牌造成的伤害时，你可以将一张与此牌类别不同的“符”置入弃牌堆，令伤害值+1。",
	["#huanyue_log"]= "%from 对 %to 的伤害由 %arg 点增加到 %arg2 点。",
	["huanyue:target"]= "<font color=\"#00FF00\"><b>%src </b></font> 受到 <font color=\"#00FF00\"><b>【%dest】 </b></font>的造成的伤害， 你是否发动“幻月”。",
	["huanyue_pile"]="符",
	["@huanyue"]= "<font color=\"#00FF00\"><b>%src </b></font> 将受到 <font color=\"#00FF00\"><b>【%dest】 </b></font>的伤害， 你是否发动“幻月”。";
	["~huanyue"]= "选择一张符 -> 确定",

	["wanggou"] = "网购",
	[":wanggou"] = "出牌阶段开始时，你可以检索一张具有伤害效果的牌并获得之，然后你可以弃置以此法获得的普【杀】，重复此流程。",
	["wanggou:retry"]= "网购：你可以弃置以此法获得的普【杀】，重复此流程。",

--**********************************
	--ndj003 势力：妖 3血
	["yukari_ndj"] = "年代记SP紫" ,
	["&yukari_ndj"] = "年代记紫" ,
	["#yukari_ndj"] = "境界中驻足的妖怪贤者" ,
	["designer:yukari_ndj"] = "辰焰天明",

	["illustrator:yukari_ndj"] = "年代记" ,

	["yuanhu"] = "援护" ,
	[":yuanhu"] = "其他角色的摸牌阶段结束时，其可以将一张手牌交给你，若如此做，你可以将一至两张手牌交给其。" ,
	["@yuanhu"] = "你可以发动 <font color=\"#FF8000\"><b>%src </b></font> 的“援护”， 交给其一张手牌。" ,
	["yuanhu:invoke"] = "您想发动 <font color=\"#FF8000\"><b>%src </b></font> 的“援护”吗？" ,
	["@yuanhu-exchange"] = "你可以将一至两张手牌交给 <font color=\"#FF8000\"><b>%src </b></font> 。" ,
	["shouxie"] = "守楔" ,
	[":shouxie"] = "<font color=\"blue\"><b>锁定技，</b></font>若你的手牌数不大于7，你跳过弃牌阶段；结束阶段开始时，你将手牌补至X张（X为你的手牌上限）。" ,

--**********************************
	--ndj004  势力：妖 4血
	["youmu_ndj"] = "年代记SP妖梦" ,
	["&youmu_ndj"] = "年代记妖梦" ,
	["#youmu_ndj"] = "修行的庭师" ,
	["designer:youmu_ndj"] = "星野梦美☆",

	["illustrator:youmu_ndj"] = "年代记",

	["hunpo"]= "魂魄",
	[":hunpo"]= "出牌阶段，若你的体力上限小于4，你可以弃置一张牌，加1点体力上限。",
	["fanji"] = "反击",
	[":fanji"] = "当你或你攻击范围内的角色受到另一名其他角色造成的伤害后，你可以失去1点体力或减1点体力上限，对来源造成1点伤害。",
	["fanji:target"] = "<font color=\"#FF8000\"><b>%src </b></font> 对 <font color=\"#00FF00\"><b>%dest </b></font> 造成了伤害，你可以对 <font color=\"#FF8000\"><b>%src </b></font> 发动“反击”。",
	["fanji:hp"]= "失去体力",
	["fanji:maxhp"]= "减体力上限",
--**********************************
	--ndj010  势力：外 3血
	["merry_ndj"] = "年代记SP玛艾露贝莉" ,
	["&merry_ndj"] = "年代记梅莉" ,
	["#merry_ndj"] = "境界探访者" ,
	["designer:merry_ndj"] = "辰焰天明",

	["illustrator:merry_ndj"] = "年代记",

	["liexi"] = "裂隙",
	[":liexi"]= "出牌阶段结束时，你可以视为使用【杀】（无距离限制），若如此做，当你使用此【杀】指定目标时，你令一名不为此牌目标的其他角色选择是否摸一张牌并成为此牌的目标，若其选择否，此牌无效。",
	["@liexi"]= "你可以发动“裂隙”， 视为使用【杀】（无距离限制）",
	["~liexi"]= "选择【杀】的合法目标 -> 确定",
	["@liexi_extra"]= "你发动“裂隙”使用了【杀】， 你须指定一名不为此【杀】的目标且为合法目标的其他角色",
	["liexi_extra:target"]= "你可以响应 <font color=\"#00FF00\"><b>%dest </b></font> 的“裂隙”，成为 <font color=\"#FF8000\"><b>%src </b></font> 使用的【%arg】的额外目标。",
	["#liexi_extra"]= "%to 成为 %from 使用的 %arg 的额外目标。",
	["#liexi"] = "裂隙",

	["mengwei"] = "梦违",
	[":mengwei"] = "当你成为【杀】的目标时，你可以令一名不为此牌目标的其他角色选择是否摸一张牌并成为目标，若其选择是，当此【杀】对一名目标角色结算结束后，若其未受到此牌造成的伤害，此牌对所有目标角色无效。",
	["@mengwei"] = "梦违",
	["@mengwei"]= "<font color=\"#00FF00\"><b>%src </b></font>使用【杀】指定了你，你可以发动“梦违”，指定一名不为此牌目标的的其他角色。",
	["mengwei_extra:target"]= "你可以响应 <font color=\"#00FF00\"><b>%dest </b></font> 的“梦违”，成为 <font color=\"#FF8000\"><b>%src </b></font> 使用的【%arg】的额外目标。",
	["#mengwei_extra"]= "%to 成为 %from 使用的 %arg 的额外目标。",
	["#MengweiNullified"] = "由于【%arg】的效果，%from 对 %to 使用的【%arg2】无效",

--**********************************
	--ndj011  势力：外 3血
	["renko_ndj"] = "年代记SP莲子" ,
	["&renko_ndj"] = "年代记莲子",
	["#renko_ndj"] = "科学少女" ,
	["designer:renko_ndj"] = "星野梦美☆",

	["illustrator:renko_ndj"] = "年代记",

	["liangzi"]= "量子",
	[":liangzi"]= "<font color=\"blue\"><b>锁定技，</b></font>当你使用或打出基本牌时，你选择一项：横置；或重置。",
	["kexue"] = "科学",
	[":kexue"] = "若你处于连环状态，你于出牌阶段的空闲时间点使用的【杀】无距离限制且额外目标数无上限。",
	["#kexue-effect"] = "科学",

--**********************************
	--ndj013  势力：风 4血
	["sanae_ndj"] = "年代记SP早苗" ,
	["&sanae_ndj"] = "年代记早苗",
	["#sanae_ndj"] = "风月战姬" ,
	["designer:sanae_ndj"] = "辰焰天明",

	["illustrator:sanae_ndj"] = "年代记",
	["xiubu"]= "修补",
	[":xiubu"]= "当一名角色于其出牌阶段内使用第一张牌时，若其为你或在你的攻击范围内，你可以弃置其一张手牌，若如此做，其于此阶段内使用最后的一张手牌无距离限制且不计入限制的使用次数，且当此阶段结束时，若其于此阶段内使用过的牌数大于1，其摸两张牌。",
	["@xiubu-self"] = "你可以发动<font color=\"#FF8000\"><b> “修补”</b></font> 弃置一张手牌。",
	["#xiubu-mod"]= "修补",

--**********************************
	--ndj018 势力：风 3血
	["aya_ndj"] = "年代记SP文" ,
	["&aya_ndj"] = "年代记文",
	["#aya_ndj"] = "神风记者" ,
	["designer:aya_ndj"] = "辰焰天明",

	["illustrator:aya_ndj"] = "年代记",
	["jineng"]= "机能",
	[":jineng"]= "准备阶段开始时或当你造成伤害后，你可以判定，若结果与你人物牌上的所有“机”花色均不同，你将之置于人物牌上，称为“机”；你可以将一张“机”按下列规则使用（无距离限制）或打出：<font size=\"5\", color=\"#808080\"><b>♠</b></font>当【杀】，<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>当【闪】，<font size=\"5\", color=\"#808080\"><b>♣</b></font>当【知己知彼】，<font size=\"5\", color=\"#FF0000\"><b>♦</b></font>当【酒】。",
	["jinengPile"]= "机",
	["kuaibao"]= "快报",
	[":kuaibao"]= "其他角色的准备阶段开始时，若“机”数大于你的体力值，你可以获得所有“机”，对其造成1点伤害。",
	["$Kuaibao"] = "%from 发动 “%arg” ， 获得  %card ",

	
	
	["tenshi_ndj"] = "年代记SP天子" ,
	["&tenshi_ndj"] = "年代记天子",
	["#tenshi_ndj"] = "勇者代行者" ,
	["designer:tenshi_ndj"] = "辰焰天明",
	
	["illustrator:tenshi_ndj"] = "年代记",
	
	["youle"]= "忧乐",
	[":youle"]= "一名角色的非额外回合结束时，若你于此回合内造成或受到过伤害，你可以选择一名角色的一个有牌区域，令其弃置其中的X张牌（X为其中的牌数且至多为5），然后其获得一个额外的回合。",
	["@youle"]= "你可以选择区域里有牌的一名角色作为“忧乐”的目标。",
	["@youle-discard"]= "你因“忧乐”的效果须弃置 五 张手牌",
    ["youle:j"]= "判定区",
	["youle:h"]= "手牌区",
	["youle:e"]= "装备区",
	
	["eirin_ndj"] = "年代记SP永琳",
	["&eirin_ndj"] = "年代记永琳",
	["#eirin_ndj"] = "什么称号",
	["designer:eirin_ndj"] = "风的呓语",
	
	["yaoli"] = "药理",
	[":yaoli"] = "<font color=\"green\"><b>每名角色的出牌阶段限一次，</b></font>其可以弃置一张牌，若如此做，你可以摸一张牌，然后弃置一张牌，若这弃置的两张牌类别相同，其获得对应类别的效果直到此阶段结束：<br /> 基本：其使用的基本牌的第一个效果值+1。<br /> 锦囊：其使用的普通锦囊牌（以此法使用时除外）结算完毕后，其可以视为使用同名牌。<br /> 装备：其使用装备牌时，其可以摸X张或弃置场上至多X张牌（若为武器牌，X等于此牌的攻击范围，否则X为1）。",
	["yaoliattach"] = "药理",
	[":yaoliattach"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以弃置一张牌，然后有“药理”技能的角色可以摸一张牌再弃置一张牌，若这弃置的两张牌类别相同，你获得对应类别的效果直到此阶段结束：<br /> 基本：你使用的基本牌的第一个效果值+1。<br /> 锦囊：你使用的普通锦囊牌（以此法使用时除外）结算完毕后，你可以视为使用同名牌。<br /> 装备：你使用装备牌时，你可以摸X张或弃置场上至多X张牌（若为武器牌，X等于此牌的攻击范围，否则X为1）。",
	["yaolibasic"] = "药理（基本牌）",
	[":yaolibasic"] = "其使用的基本牌的第一个效果值+1。",
	["yaoliequip"] = "药理（装备牌）",
	[":yaoliequip"] = "其使用装备牌时，其可摸X张或弃置场上至多X张牌（若为武器牌，X等于此牌的攻击范围，否则X为1）。",
	["yaolitrick"] = "药理（锦囊牌）",
	[":yaolitrick"] = "其使用的普通锦囊牌（以此法使用时除外）结算完毕后，其可以视为使用同名牌。",
	
	["#yaolistart"] = "%from 获得了 <font color=\"#FF8000\"><b>“药理”</b></font>（%arg） 的效果。",
	["#yaolifinish"] = "%from 的出牌阶段结束，<font color=\"#FF8000\"><b>“药理”</b></font>（%arg） 的效果消失。",
	["yaoli-draw:hahahahahaha"] = "%from 对你发动“药理”，你可以摸 1 张牌然后弃置 1 张牌。" ,
	["@yaoli-discard"] = "请弃置 1 张牌，包括装备" ,
	["#yaolibasic"] = "%from 执行 <font color=\"#FF8000\"><b>“药理”</b></font>（基本牌） 的效果，此次使用的 %arg 的第一个效果值 <font color=green><b>+1</b></font>。",
	["@yaoliequipi"] = "请选择 1 名角色，弃置其区域内的 1 张牌，或者点击“取消”摸 %arg 张牌。<br />您还可以弃置场上 %arg 张牌。",
	["@yaoliequip"] = "请选择 1 名角色，弃置其区域内的 1 张牌。<br />您还可以弃置场上 %arg 张牌。",
	["#yaoliequipi"] = "%from 执行 <font color=\"#FF8000\"><b>“药理”</b></font>（装备牌） 的效果",
	["#yaoliequip0"] = "%from 执行 <font color=\"#FF8000\"><b>“药理”</b></font>（装备牌） 的效果",
	["@yaolitrick"] = "您可以执行“药理”（锦囊牌）的效果，视为使用 %arg。",
	["~yaoliattach"] = "（如果有的话）选择目标->点击“确定”",
	
}
