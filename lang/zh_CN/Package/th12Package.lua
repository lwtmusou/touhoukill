return{
	
	["th12"] = "星莲船",
	["xlc"] = "星",
	
	
	
	
["xlc001"] = "圣白莲",["#xlc001"] = "被封印的大魔法师",
         
		["pudu"] = "普度",
        [":pudu"] = "出牌阶段,你可令一名已受伤的其他角色恢复一点体力,若如此做,你失去1点体力。<font color=\"green\"><b>每阶段限一次</b></font>",
       
	   ["jiushu"] = "救赎",
        [":jiushu"] = "回合结束阶段开始时,你可以摸x张牌(x为你已损失的体力值).",
        
		["fahua"] = "法华",
        [":fahua"] = "<font color=\"orange\"><b>主公技，</b></font>当你成为其他角色使用的锦囊牌的目标时,你可以请求其他星势力的角色(不能是此牌的使用者或目标)将此牌转移给该角色。",
		["fahua_change"]="法华",
		["fahua_change:tricktarget"]="你是否响应<font color=\"#00FF00\"><b>%dest </b></font>的主公技“法华”,代替<font color=\"#00FF00\"><b>%dest </b></font>成为<font color=\"#FF8000\"><b>%src </b></font>使用的【%arg】的目标",
		["#fahua_change"]="%to 代替其成为 %from使用的%arg的目标",

 ["xlc002"]="封兽鵺",["#xlc002"]="未确认幻想飞行少女",
       
	   ["weizhi"]="未知",
        [":weizhi"]="出牌阶段，你可以弃置任意数量的非锦囊牌，然后摸X+1张牌（X为你以此法弃置的牌的数量）。<font color=\"green\"><b>每阶段限一次，</b></font>",
        
		["weizhuang"]="伪装",
        [":weizhuang"]="<font color=\"blue\"><b>锁定技，</b></font>当你成为其他角色使用的非延时类锦囊牌的目标时，该牌的使用者须弃置一张基本牌，否则此牌对你无效。",
        ["@weizhuang-discard"]="<font color=\"#00FF00\"><b>%src </b></font> 的“伪装”被触发，请弃置一张基本牌，否则【%dest】对其无效",

 ["xlc003"] = "寅丸星",["#xlc003"] = "毘沙门天的弟子",
          
		["jinghua"] = "净化",
        [":jinghua"] = "一名角色的回合开始阶段开始时，你可以将场上的一张延时类锦囊牌置于牌堆底。 若此回合不是你的回合，你须失去一点体力",
        ["@targetchoose"]="你可以选择一个角色，将其判定区的一张牌置于牌堆底。",

		["zhengyi"] = "正义",
        [":zhengyi"] = "<font color=\"blue\"><b>锁定技，</b></font>若你的装备区有防具牌，弃置之。你手牌中的防具牌均视为【无懈可击】。每当你成为黑色【杀】或黑色非延时类锦囊牌的目标时，你须选择一项：摸一张牌，或令此牌对你无效。",
        ["zhengyi:drawcard"] = "<font color=\"#00FF00\"><b>%src </b></font>对你使用了【%dest】。你的技能“正义”被触发, 点击确定，摸一张牌；点击取消令 【%dest】 对你无效",
		["#zhengyi2"]="【正义】被触发，此装备牌装备后会被弃置",		
		["#ZhengyiUninstall"]="%from 的“%arg”被触发，防具不能置于 %from 的 装备区",
	
["xlc004"] = "村纱水蜜", ["#xlc004"] = "水难事故的念缚灵",
 
 ["chuannan"] = "船难", 
 [":chuannan"] = "每当你对一名其他角色造成一次伤害后，你可以将一张手牌当【兵粮寸断】对该角色使用。每当你受到其他角色对你造成的一次伤害后，你可以摸一张牌，然后将一张手牌当【兵粮寸断】对伤害来源使用。", 
["@chuannan"] ="你可以将一张手牌作为【兵粮寸断】对 <font color=\"#00FF00\"><b>%src </b></font> 使用。",

  ["xlc005"] = "云居一轮",["#xlc005"] = "守护与被守护的大轮",
        
		["lizhi"] = "理智",
        [":lizhi"] = "每当你造成一次伤害时,你可以防止此伤害,并摸两张牌",
        
		["yunshang"] = "云上",
        [":yunshang"] = "<font color=\"blue\"><b>锁定技，</b></font>当你成为其他角色的非延迟性锦囊牌的目标时,若你不在其攻击范围内,此牌对你无效",

 ["xlc006"] = "娜兹玲",["#xlc006"] = "探宝的小小大将",
         
		["souji"] = "搜集",
        [":souji"] = "你的回合内，若其他角色失去的牌置入弃牌堆，你可以获得它",
        
		["tansuo"] = "探索",
        [":tansuo"] = "若你于弃牌阶段弃置了不少于X张手牌（X为你当前体力值），则于此阶段结束时，你可以获得牌堆底的两张牌。",
		
 ["xlc007"] = "多多良小伞" ,["#xlc007"] = "愉快的遗忘之伞" ,
          
	 ["ddlxsyiwang"] = "遗忘" ,
        [":ddlxsyiwang"] = "每当你失去装备区里的一张牌时，你可选择一项：回复1点体力，或令一名其他角色回复1点体力，然后你摸1张牌。" ,
        ["@ddlxsyiwang-recover"] = "请选择回复体力的角色。" ,
        
		["ddlxsjingxia"] = "惊吓" ,
        [":ddlxsjingxia"] = "每当你受到1点伤害后，你可以选择一项，依次弃置伤害来源的两张牌，或依次弃置场上的1-2张牌。" ,
        ["ddlxsjingxia:dismiss"] = "不发动" ,
        ["ddlxsjingxia:discard"] = "弃置来源" ,
        ["ddlxsjingxia:discardfield"] = "弃置场上" ,
        ["@ddlxsjingxia-discardfield"] = "请选择要弃置牌的第1名角色。" ,
        ["@ddlxsjingxia-discardfield2"] = "请选择要弃置牌的第2名角色。" ,
        ["ddlxsjingxia-discard"] = "惊吓" ,
        ["ddlxsjingxia-discardfield"] = "惊吓" ,
		["#jingxia1"]="%from发动了“%arg”",

["xlc008"] = "云山", ["#xlc008"] = "脱离时代的顽固老爹",

 ["bianhuan"] = "变幻", 
 [":bianhuan"] = "每当你受到一次伤害时，你可以失去1点体力上限，防止该伤害", 
 
 ["nuhuo"] = "怒火", 
 [":nuhuo"] ="出牌阶段，你可令一名其他角色对你造成1点伤害，然后视为你对你攻击范围内的由该角色指定的一名角色（不能是你）使用一张【杀】，此【杀】不计入每阶段的使用限制。<font color=\"green\"><b>每阶段限一次</b></font>",
 ["@nuhuo"] = "“怒火”：选择 <font color=\"#00FF00\"><b>%src </b></font> 攻击范围内一名角色，视为 <font color=\"#00FF00\"><b>%src </b></font> 对其出【杀】",
 }
