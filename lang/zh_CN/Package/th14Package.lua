return{

    ["th14"] = "辉针城",
    ["hzc"] = "辉",
    
    --hzc001  势力：辉 3血
    ["shinmyoumaru"] = "少名针妙丸",
    ["#shinmyoumaru"] = "小人的末裔",
    ["!shinmyoumaru"] = "少名针妙丸",
	--["designer:shinmyoumaru"] = "星野梦美 | 程序:三国有单",
	["cv:shinmyoumaru"] = "暂无", 
	["illustrator:shinmyoumaru"] = "ミイル p号:38962368",
	["illustrator:shinmyoumaru_1"] = "東天紅 p号:39094589",
	["illustrator:shinmyoumaru_2"] = "BoboMaster p号:38023798",
	
    ["baochui"] = "宝槌",
    [":baochui"] = "一名角色的准备阶段开始时，若该角色的手牌数小于三，你可以弃置一张牌，令该角色将手牌摸至三张。此回合的弃牌阶段开始时，若该角色的手牌数小于三，该角色失去1点体力。",
    ["@baochui"] = "你可以弃置一张牌，对 <font color=\"#FF8000\"><b>%src </b></font> 发动“宝槌”。",
	["#BaochuiBuff"] = "%from 手牌数小于 3 ， “%arg”效果被触发，",
    ["yicun"] = "一寸",
    [":yicun"] = "<font color=\"blue\"><b>锁定技，</b></font>当你成为【杀】的目标时，若此【杀】的使用者的手牌数不小于你，此牌对你无效。",
    ["moyi"] = "末裔",
    [":moyi"] = "<font color=\"orange\"><b>主公技，</b></font>其他辉势力角色的弃牌阶段结束时，该角色可将一张于此阶段内置入弃牌堆的基本牌加入你的手牌。",
    ["@moyi-select"]="你可以指定拥有主公技“末裔”的角色，将一张本阶段置入弃牌堆的基本牌加入其手牌。",

	
--****************************
    --hzc002  势力：辉 4血
    ["raiko"] = "堀川雷鼓",
    ["#raiko"] = "梦幻的打击乐手",
    ["!raiko"] = "堀川雷鼓",
	--["designer:raiko"] = "星野梦美 | 程序:三国有单",
	["cv:raiko"] = "暂无", 
	["illustrator:raiko"] = "伊吹のつ p号:41891311",
	["illustrator:raiko_1"] = "泉水茜 p号:39316689",
	
    ["leiting"] = "雷霆",
    [":leiting"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以指定一名其他角色，然后你摸两张牌，弃置一张手牌。若以此法弃置了<font size=\"5\", color=\"#FF0000\"><b>♥</b></font>牌，该角色须摸一张牌，然后受到1点无来源的雷电伤害。若以此法弃置了<font size=\"5\", color=\"#808080\"><b>♠</b></font>牌，该角色视为对其攻击范围内的由该角色指定的一名角色使用一张雷【杀】。",
    ["@leiting"] ="你发动“雷霆”指定了 <font color=\"#00FF00\"><b>%src </b></font>，请选择要弃置的牌。",
    ["@leiting_chosen"] ="<font color=\"#00FF00\"><b>%src </b></font> 对你发动了“雷霆”，请选择【杀】的目标。",
 
--****************************
 --hzc003  势力：辉 3血
    ["seija"] = "鬼人正邪",
    ["#seija"] = "逆袭的天邪鬼",
    ["!seija"] = "鬼人正邪",
	--["designer:seija"] = "星野梦美 | 程序:三国有单", 
	["cv:seija"] = "暂无", 
	["illustrator:seija"] = "藤宮ふみや p号:37885158",
	["illustrator:seija_1"] = "ふらぺち p号:53327346",
	["illustrator:seija_2"] = "ゾウノセ p号:51838778",
	
    ["nizhuan"] = "逆转",
    [":nizhuan"] ="当一名角色成为【杀】的目标后，若此【杀】只指定了该角色为目标，且该角色已损失的体力值大于此【杀】的使用者，你可以弃置该角色的一张手牌，令此【杀】无效，然后令该角色视为对此【杀】的使用者使用一张【杀】。",
    ["nizhuan:target"] = "<font color=\"#00FF00\"><b>%src </b></font>使用【杀】指定了<font color=\"#FF8000\"><b>%dest </b></font>，你是否发动“逆转”" ,
    ["guizha"]="诡诈", 
    [":guizha"] = "<font color=\"blue\"><b>锁定技，</b></font>当你处于濒死状态并向其他角色求【桃】时，该角色展示所有手牌，若其中有【桃】，该角色须对你使用。",
    ["@guizha"] ="由于“诡诈”的效果，你必须对 <font color=\"#00FF00\"><b>%src </b></font> 使用一张【桃】。",    
 
 --****************************
    --hzc004  势力：辉 3血
    ["benben"] = "九十九弁弁", 
    ["#benben"] = "古旧琵琶的付丧神",
    ["!benben"] = "九十九弁弁",
	--["designer:benben"] = "星野梦美 | 程序:三国有单", 
	["cv:benben"] = "暂无", 
	["illustrator:benben"] = "neko p号:38047797",
	["illustrator:benben_1"] = "会帆 p号:41942420",
	
    ["yuyin"] = "余音",
    [":yuyin"] = "当你受到伤害后，你可以获得一名体力值大于你的角色的一张牌。",
    ["@yuyin"] = "你可以发动“余音”，指定一名体力值大于你的角色，获得其一张牌。",
    ["wuchang"] = "无常",
    [":wuchang"] = "其他角色的弃牌阶段结束时，若该角色于此阶段内没有弃置红色手牌，你可令该角色弃置一张手牌。",
    ["wuchang_discard"]="由于“无常”的效果，请弃置一张手牌。",
    ["wuchang:target"] ="你是否对 <font color=\"#00FF00\"><b>%src </b></font> 发动“无常” ";
	
--****************************
    --hzc005  势力：辉 3血
    ["yatsuhashi"] = "九十九八桥",
    ["#yatsuhashi"] = "古旧的琴的付丧神",
    ["!yatsuhashi"] = "九十九八桥",
	--["designer:yatsuhashi"] = "星野梦美 | 程序:三国有单", 
	["cv:yatsuhashi"] = "暂无",
	["illustrator:yatsuhashi"] = "neko p号:38047797",
	["illustrator:yatsuhashi_1"] = "ウミガラス p号:41847626",
	
    ["canxiang"] = "残响",
    [":canxiang"] ="当你造成伤害后，你可以获得一名体力值大于你的角色的一张牌。",
    ["@canxiang"] = "你可以发动“残响”，指定一名体力值大于你的角色，获得其一张牌。",
    ["juwang"] = "俱亡",
    [":juwang"] ="当你需要于你的回合外使用或打出一张【闪】时，你可令当前回合的角色选择一项：弃置一张红色手牌，或受到你对其造成的1点伤害。",
    ["juwang:throw"] ="你可以对 <font color=\"#00FF00\"><b>%src </b></font> 发动“俱亡”。";
    ["@juwang"]="<font color=\"#00FF00\"><b>%src </b></font>对你发动了“俱亡”，请弃置一张红色手牌，否则其对你造成的1点伤害。",
 
--****************************
  --hzc006  势力：辉 4血
    ["kagerou"] = "今泉影狼",
    ["#kagerou"] = "竹林的人狼",
    ["!kagerou"] = "今泉影狼",
	--["designer:kagerou"] = "星野梦美 | 程序:三国有单", 
	["cv:kagerou"] = "暂无",
	["illustrator:kagerou"] = "myaaco p号:38562704",
	["illustrator:kagerou_1"] = "kirero p号:40434726",
	
    ["langying"] = "狼影",
    [":langying"] ="当你需要使用或打出一张【闪】时，你可以将你装备区里的所有牌（至少一张）返回手牌，视为你使用或打出了一张【闪】。",
    ["yuanfei"] = "远吠",
    [":yuanfei"] ="<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以指定一名其他角色。该角色不能使用或打出牌，直到回合结束。若该角色在你攻击范围内，你须先弃置一张手牌。",
    ["#yuanfei"] ="因为 %arg 的效果，此回合 %from 不能使用或打出牌。",
    ["yuanfeinear"] = "远吠",

--****************************
    --hzc007  势力：辉 4血
    ["sekibanki"] = "赤蛮奇",
    ["#sekibanki"] = "辘轳首的怪奇", 
    ["!sekibanki"] = "赤蛮奇",
	--["designer:sekibanki"] = "星野梦美 | 程序:三国有单", 
	["cv:sekibanki"] = "暂无", 
	["illustrator:sekibanki"] = "kito p号:36127927",
	["illustrator:sekibanki_1"] = "シエロ p号:37343098",
	["illustrator:sekibanki_2"] = "キメラ キライ p号:35991964",
	
    ["feitou"] = "飞头",
    ["#feitou_slash"]= "飞头",
    ["feitou:extra_slash"] = "你可以对 <font color=\"#00FF00\"><b>%src </b></font> 发动“飞头”。",
    ["addfeitou"] = "你可以将一张手牌作为“飞头”置于人物牌上。",
    [":feitou"] ="结束阶段开始时，你可以将一张牌置于你的人物牌上，称为“飞头”。你可以将一张“飞头”当【杀】使用或打出，此【杀】无距离限制，不计入每阶段的使用次数限制。",
    ["@feitou-twoCards"]="选择一张“飞头”当【杀】使用。",
    ["~feitou"]="选择牌→指定目标→点击“确定”。",
 
--****************************
    --hzc008  势力：辉 3血
    ["wakasagihime"] = "若鹭姬",
    ["#wakasagihime"] = "栖息于淡水的人鱼",
    ["!wakasagihime"] = "若鹭姬",
	--["designer:wakasagihime"] = "星野梦美 | 程序:三国有单", 
	["cv:wakasagihime"] = "暂无", 
	
	["illustrator:wakasagihime"] = "neme p号:39196423",
	["illustrator:wakasagihime_1"] = "s-syogo p号:38957820",
	["illustrator:wakasagihime_2"] = "藤宮ふみや p号:37256433",
	["illustrator:wakasagihime_3"] = "Cloudy.R p号:42096555",
	["illustrator:wakasagihime_4"] = "粗茶 p号:45890537",
	["illustrator:wakasagihime_5"] = "villyane p号:38169403",
	
     ["shizhu"] = "拾珠",
    [":shizhu"] ="其他角色的结束阶段开始时，你可以选择一张于此回合置入弃牌堆的【桃】并展示所有手牌，若其中没有【桃】，你获得你选择的牌。",
    ["liange"] = "恋歌",
    [":liange"]="<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以将一张牌置于牌堆顶，然后令一名其他角色观看牌堆顶的两张牌，该角色获得其中的一张，将另一张置入弃牌堆。",

}
