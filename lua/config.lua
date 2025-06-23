-- this script to store the basic configuration for game program itself
-- and it is a little different from config.ini

config = {
	developers = {
	},

	kingdoms = {
		"zhu",
		"pc98",
		"hmx",
		"yym",
		"yyc",
		"zhan",
		"fsl",
		"dld",
		"xlc",
		"slm",
		"hzc",
		"gzz",
		"tkz",
		"gxs",
		"hld",
		-- "swy",
		"wai",
		"touhougod"
	},
	hegemony_kingdoms = {
	"wei", "qun", "shu", "wu"
	},
	kingdom_colors = {
		pc98 = "#a699cc";
		zhu ="#853dcc",
		hmx = "#cc1414",
		yym = "#3d3dcc",
		yyc = "#999999",
		zhan = "#3dcc3d",
		fsl = "#cc9c3d",
		dld = "#cc3d85",
		xlc = "#3dcccc",
		slm ="#66cc99",
		hzc ="#aacc66",
		gzz = "#005AD7" ,
		tkz = "#48D1CC",
		gxs = "#ff8080",
		hld = "#ff0000",
		swy = "#461187",
		wai = "#cc7014",
		touhougod = "#96943D",

		wei = "#547998",
		shu = "#D0796C",
		wu = "#4DB873",
		qun = "#8A807A",
		god = "#96943D"
	},

	package_names = {
		--卡牌包
		"StandardCard",
		"StandardExCard",
		"WashOut",
		"Maneuvering",
		"HegemonyCard",

		--武将包
		"PeasantsVSLandlord",
		"Protagonist",
		"TH0105",
		"TH06",
		"TH07",
		"TH08",
		"TH09",
		"TH10",
		"TH11",
		"TH12",
		"TH13",
		"TH14",
		"TH15",
		"TH16",
		"TH17",
		"TH18",
		-- "TH19",
		"TH99",
		"THNDJ",
		"TouhouGod",
		"HegemonyGeneral",
		"Standard", --此包内带pattern的定义 不能屏蔽。。。
		"Test",
		"Playground",
	},

	hulao_packages = {
		"standard",
		"wind"
	},

	xmode_packages = {
		"standard",
		"wind",
		"fire",
		"nostal_standard",
		"nostal_wind",
	},

	easy_text = {
		"太慢了，做两个俯卧撑吧！",
		"快点吧，我等的花儿都谢了！",
		"高，实在是高！",
		"好手段，可真不一般啊！",
		"哦，太菜了。水平有待提高。",
		"你会不会玩啊？！",
		"嘿，一般人，我不使这招。",
		"呵，好牌就是这么打地！",
		"杀！神挡杀神！佛挡杀佛！",
		"你也忒坏了吧？！"
	},

	robot_names = {
		"神社的赛钱箱",
		"魔法蘑菇",
		"洋馆的妖精女仆",
		"冻青蛙",
		"无法往生的幽灵" ,
		"白芋头麻薯",
		"居家用人偶",
		"妖怪兔",
		"寺子屋的小孩",
		"向日葵（严禁攀折践踏）",
		"棋艺超群的天狗",
		"河童重工普通职员",
		"地狱鸦",
		"火焰猫" ,
		"“UFO”",
		"吓人用妖怪伞",
		"寻宝用鼠",
		"非想天则",
		"修行不足的狸猫",
		"防腐僵尸",
		"付丧神（暂时）",
		"人间之里的普通人",
		"白泽球" ,
		"毛玉" ,
		"油库里" ,
		"罪袋" ,
	},

	roles_ban = {
	},

	peasantsvslandlord_ban = {
	},

	contest2v2_ban = {
	},

	kof_ban = {
	},

	hulao_ban = {
	},

	xmode_ban = {
	},

	basara_ban = {
	},

	hegemony_ban = {

	},

	pairs_ban = {
	},

	convert_pairs = {
	},



	bgm_convert_pairs = {
		--BGM： 由于开始尝试加入arrange代替原曲，曲目对应关系可能有变，以后转换列表还要重新整理。

		"kosuzu->akyuu",
		"tokiko->rinnosuke",
		"unzan->ichirin",
		"yorihime->toyohime",
		"youki->youmu",
		"myouren->byakuren",
		"leira|lunasa|merlin|lyrica->prismriver",
		"reisen2->tewi",
		"merry->renko",
		"lunar|star->sunny",
		"shanghai->alice",
		"yatsuhashi->benben",
		"mai->satono",
		"lilywhite_sp->lilywhite",
	},

	backdrop_convert_pairs = {
		"yukimai->shinki",
		"lunar|star->sunny",
		"reisen2->tewi",
		"seiran->ringo",
		"seija_sp->seija",
		"rumia_sp->rumia",
		"lunasa|merlin|lyrica->prismriver",
	},
	latest_generals = {
		-- 0.10.5
		"elis", "luize",
		-- 0.10.6
		"eirin_ndj",
		-- 0.10.9
		"ellen", "sara", "futo", "rika", "keiki", "saki", "yorigamis", "mayumi", "eika",
		"yuyuko_sp", "hatate", "nue_slm", "sumireko", "kaguya_god", "konngara", "yuyuko",
		"yachie", "kisume", "benben", "ichirin", "ringo", "mokou_sp", "kutaka", "seiran",
		"junko", "urumi",
	}
}

local f = io.open("audio/bgm/version.txt")
if f then
	config.withBgm = f:read("*L")
	f:close()
else
	config.withBgm = "N/A"
end

f = io.open("image/heroskin/version.txt")
if f then
	config.withHeroSkin = f:read("*L")
	f:close()
else
	config.withHeroSkin = "N/A"
end
