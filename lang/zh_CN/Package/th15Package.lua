return {
	["gzz"] = "绀",
	["th15"] = "绀珠传",

--***********************************
	--15001 纯狐 4hp
	["junko"] = "纯狐" ,
	["#junko"] = "无名之恨" ,
	["designer:junko"] = "三国有单",
	
	["illustrator:junko"] = "ルリア",
	["origin:junko"] = "p号：58277323， 个人ID：997454",
	["illustrator:junko_1"] = "久賀フーナ",
	["origin:junko_1"] = "p号：54259284， 个人ID：2636363",
	
	
	["xiahui"] = "瑕秽",
	[":xiahui"] = "当你造成或受到伤害后，你可以明置来源和受到伤害的角色中一至两名角色各的一张手牌。",
	["@xiahui"] = "你可以发动“瑕秽”，明置 <font color=\"#FFA500\"><b>受伤者</b></font> 和 <font color=\"#FFA500\"><b>来源</b></font> 中一名或两名角色的 各一张手牌（明置顺序：1.当前回合角色；2.受到伤害的角色。若先明置后者，择不明置前者）",
	["chunhua"] = "纯化",
	[":chunhua"] = "当一名角色使用明置的基本牌或明置的普通锦囊指定角色为目标后，你可以根据其一张明置手牌或此牌的颜色改变此牌的效果：黑色“使用者对目标角色造成1点伤害”；红色“目标角色回复1点体力”。<font color=\"green\"><b>每阶段限一次。</b></font>",
	["#Chunhua"] = "【%arg】 对 目标 %from 产生 纯化 效果",
	["#CancelChunhua"] = "【%arg】 的 纯化 效果 对 目标 %from 无效",
	["chunhua:black"] = "黑色“使用者对目标造成1点伤害”" ,
	["chunhua:red"] = "红色“目标回复1点体力”" ,
	["$ChunhuaRed"] = "%from 对 %to 使用的【%arg】的效果改为 “目标角色回复1点体力”",
	["$ChunhuaBlack"] = "%from 对 %to 使用的【%arg】的效果改为 “使用者对目标角色造成1点伤害”",

	["shayi"] = "杀意",
	[":shayi"] = "<font color=\"orange\"><b>主公技，</b></font>当其他绀势力角色使用具有伤害效果的牌时，其可以令你选择是否成为此牌的使用者。",

	["shayi_change"] = "杀意（主公技）",
	["$Shayi"] = "%from 发动 %arg 成为了 %card 的使用者",
	["#shayi_temp"] = "杀意",
	["@shayi-use"] = "你可以发动“杀意”，使用一张于此阶段内置入弃牌堆的【杀】。",
	["~shayiuse"] = "选择目标 -> 确定",


--***********************************
	--15002 赫卡提亚·拉碧斯拉祖利 4hp
	["hecatia"] = "赫卡提亚" ,
	["!hecatia"] = "赫卡提亚·拉碧斯拉祖利" ,
	["#hecatia"] = "地狱的女神" ,
	["designer:hecatia"] = "辰焰天明",
	
	["illustrator:hecatia"] = "もりもり",
	["origin:hecatia"] = "p号：56930159，　个人ID：3099070",
	["illustrator:hecatia_1"] = "ひよすけ",
	["origin:hecatia_1"] = "p号：52369022",
	
	["santi"] = "三体",
	[":santi"] = "<font color=\"blue\"><b>锁定技，</b></font>非额外回合开始时，你令此回合执行阶段为“准备-判定-（摸牌-出牌-弃牌） x3 -结束”；当你于此回合的出牌阶段内使用牌时，你令此阶段内/余下的出牌阶段内不能使用与之类型不同/相同的牌。",
	--[":santi"] = "<font color=\"blue\"><b>锁定技，</b></font>非额外回合开始时，你令你于此回合内的判定阶段结束后和非额外的弃牌阶段结束后各依次执行一个额外的摸牌阶段、出牌阶段和弃牌阶段；你于回合内的第一/二/三个出牌阶段内，只能使用基本牌/装备牌/锦囊牌。",
	["#santi"] = "三体",

--***********************************
	--15003 克劳恩皮丝 3hp
	["clownpiece"] = "克劳恩皮丝" ,
	["#clownpiece"] = "地狱的妖精" ,
	["designer:clownpiece"] = "三国有单",
	
	["illustrator:clownpiece"] = "ルリア",
	["origin:clownpiece"] = "个人ID：997454",
	["illustrator:clownpiece_1"] = "さな",
	["origin:clownpiece_1"] = "个人ID：58171953",	
	
	["kuangluan"] = "狂乱",
	["#kuangluan1"] = "狂乱",
	["#kuangluan2"] = "狂乱",
	["kuangluan1"] = "狂乱(明置手牌)",
	["kuangluan2"] = "狂乱(技能无效)",
	[":kuangluan"] = "当没有明置手牌的其他角色于其摸牌阶段外获得牌后，你可以令其明置之；当其他角色失去其所有明置手牌后，你可以令其技能（除永久技外）于此回合内无效。",
	["#kuangluan_invalidity"]= " %from的 所有技能 因“%arg”效果于此回合失效（除永久技外）。",

	["yuyi"] = "狱意",
	[":yuyi"] = "当你受到【杀】造成的伤害时，你可以弃置一张手牌，弃置来源的一张手牌，若以此法弃置了两张颜色不同的牌，伤害值-1。",
	["@yuyi_discard"] = "你受到【杀】造成的伤害，你是否弃置牌发动“狱意”",
	["#YuyiTrigger"]= "%from 触发“%arg” ，减少了 %arg2 点伤害。",

--***********************************
	--15004 稀神探女 4hp
	["sagume"] = "稀神探女" ,
	["#sagume"] = "招来口舌之祸的女神" ,
	["designer:sagume"] = "三国有单",
	
	["illustrator:sagume"] = "菊月",
	["origin:sagume"] = "p号：58197052，个人ID：429883",
	["illustrator:sagume_1"] = " りひと",
	["origin:sagume_1"] = "p号：58254093",
	
	
	["shehuo"] = "舌祸",
	[":shehuo"] = "当一名角色使用【杀】或普通锦囊牌指定另一名角色为唯一目标时，若你为两者之一，你可以令目标角色选择一项：取消其并对此牌的使用者使用与之类别不同的牌（无距离限制），或令此牌不能被所有角色使用牌响应。",
	["@shehuo_use"] = "舌祸: <font color=\"#00FF00\"><b>%src </b></font>对你使用了【%dest】, 你是否对<font color=\"#00FF00\"><b>%src </b></font>使用类别不同的一张牌，并取消你（即你不再是此牌的目标，不会继续对你进行结算）。",
	["shehuo:target"]= "<font color=\"#FF8000\"><b>%src </b></font> 使用【%arg】 指定了 <font color=\"#00FF00\"><b>%dest </b></font> ，你是否发动“舌祸”？（其需对你使用类别不同的牌，否则不能用牌响应此牌）",

	["#shehuo"] = "舌祸",
	["shenyan"] = "慎言",
	[":shenyan"] = "其他角色的弃牌阶段开始时，若其于此回合内未造成过伤害，你可以视为使用【以逸待劳】（目标为其与你）。",

--***********************************

--15005 哆来咪·苏伊特 3hp
	["doremy"] = "哆来咪·苏伊特" ,
	["#doremy"] = "梦之支配者" ,
	["&doremy"] = "哆来咪",
	["designer:doremy"] = "三国有单",
	
	["illustrator:doremy"] = "あぶそる",
	["origin:doremy"] = "p号：52317687，个人ID：3202270",
	 
	["illustrator:doremy_1"] = "CUBY",
	["origin:doremy_1"] = "p号：50318152",
	["illustrator:doremy_2"] = "HeikoKuru1224",
	["origin:doremy_2"] = "p号：77932528；个人ID：35452386",
	["illustrator:doremy_3"] = "あいに*",
	["origin:doremy_3"] = "p号：76225193；个人ID：1513922",
	["illustrator:doremy_4"] = "カタケイ",
	["origin:doremy_4"] = "p号：74406452；个人ID：90042",
	["illustrator:doremy_5"] = "here /ヘレ@お仕事募集中",
	["origin:doremy_5"] = "p号：84307074；个人ID：5061948",
	
	["bumeng"] = "捕梦",
	[":bumeng"] = "当其他角色的牌因其弃置而进入弃牌堆后，你可以令其获得其中的一张牌，明置之，然后你可以获得剩余的一张牌，明置之。<font color=\"green\"><b>每回合限一次。</b></font>",
	["rumeng"] = "入梦",
	[":rumeng"] = "当一名角色于其他角色的出牌阶段内因使用或打出而失去所有的明置手牌后，你可以结束此阶段。",
	["rumeng_skip"]= "出牌阶段",
	["#rumeng_skip"]= "由于 %arg2 的效果，%from 结束 %arg。",

--***********************************
	 --15006 铃瑚
	["ringo"] = "铃瑚" ,
	["#ringo"] = "橘色的月兔" ,
	["designer:ringo"] = "三国有单",
	
	["illustrator:ringo"] = "りひと",
	["origin:ringo"] = "p号：56703160，个人ID：1463626",
	["illustrator:ringo_1"] = "CUBY",
	["origin:ringo_1"] = "p号：50411353",
	
	["yuejian"] = "月见";
	[":yuejian"] = "<font color=\"green\"><b>出牌段限一次，</b></font>你可以与一名角色拼点：当你赢后，你视为使用【以逸待劳】；当你没赢后，你将你的拼点牌置于人物牌上，称为“团子”。";

	--[":yuejian"] = "出牌阶段，你可以与一名你未于此阶段内对其发动过此技能的角色拼点：当你没赢后，你视为使用果【酒】；当你赢后，你视为使用【以逸待劳】且你于此阶段内不能再发动此技能。";
	["@yuejian1"] = "你需发动“月见”，视为使用【以逸待劳】";
	--["@yuejian2"] = "你需发动“月见”，视为使用果【酒】";
	["~yuejian"] = "选择目标 -> 确定";

	["jiangguo"] = "浆果";
    [":jiangguo"] = "你可以将一张“团子”当果【酒】使用。";
    ["dango"] = "团子";
	
	
--***********************************
	--15007 清兰 4hp
	["seiran"] = "清兰" ,
	["#seiran"] = "浅葱色的月兔",
	["designer:seiran"] = "辰焰天明",
	
	["illustrator:seiran"] = "minusT",
	["origin:seiran"] = "p号：56472774，个人ID：15772166",
	
	["illustrator:seiran_1"] = "りひと",
	["origin:seiran_1"] = "p号：57754046",
	
	
	["yidan"] = "异弹",
	[":yidan"] = "<font color=\"green\"><b>出牌段限一次，</b></font>你可以将一张手牌当不计入限制的使用次数的的秽【杀】对一名有手牌且无明置手牌的角色使用。",
	["#yidan"] = "异弹",
	["xuechu"] = "血杵",
	[":xuechu"] = "<font color=\"blue\"><b>锁定技，</b></font>当你对其他角色造成伤害后，你令其选择是否弃置一张【闪】，若其选是，其回复1点体力；当你使用指定【杀】目标后，你令所有目标不能使用【闪】响应此牌。",
	["@xuechu"] = "因“血杵”的效果，你可以弃置一张【闪】来回复1点体力",
}
