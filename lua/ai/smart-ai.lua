-- This is the Smart AI, and it should be loaded and run at the server side

-- "middleclass" is the Lua OOP library written by kikito
-- more information see: https://github.com/kikito/middleclass
require "middleclass"

-- initialize the random seed for later use
math.randomseed(os.time())

-- SmartAI is the base class for all other specialized AI classes
SmartAI = (require "middleclass").class("SmartAI")

version = "TouhouSatsu AI 20180809"



--- this function is only function that exposed to the host program
--- and it clones an AI instance by general name
-- @param player The ServerPlayer object that want to create the AI object
-- @return The AI object
function CloneAI(player)
	return SmartAI(player).lua_ai
end

sgs.ais = {}
sgs.ai_card_intention =     {}
sgs.ai_playerchosen_intention = {}
sgs.ai_Yiji_intention = {}
sgs.ai_Rende_intention = {}
sgs.role_evaluation =       {}
sgs.ai_role =               {}
sgs.ai_keep_value =         {}
sgs.ai_use_value =          {}
sgs.ai_use_priority =       {}
sgs.ai_suit_priority =      {}
sgs.ai_chaofeng =           {}
sgs.ai_global_flags =       {}
sgs.ai_skill_invoke =       {}
sgs.ai_skill_suit =         {}
sgs.ai_skill_cardask =      {}
sgs.ai_skill_choice =       {}
sgs.ai_skill_askforag =     {}
sgs.ai_skill_askforyiji =   {}
sgs.ai_skill_pindian =      {}
sgs.ai_filterskill_filter = {}
sgs.ai_skill_playerchosen = {}
sgs.ai_skill_discard =      {}
sgs.ai_cardshow =           {}
sgs.ai_skill_cardchosen =   {}
sgs.ai_skill_use =          {}
sgs.ai_cardneed =           {}
sgs.ai_skill_use_func =     {}
sgs.ai_skills =             {}
sgs.ai_slash_weaponfilter = {}
sgs.ai_slash_prohibit =     {}
sgs.ai_trick_prohibit =     {}
sgs.ai_view_as = {}
sgs.ai_cardsview = {}
sgs.ai_cardsview_valuable = {}
sgs.dynamic_value =         {
	damage_card =           {},
	control_usecard =       {},
	control_card =          {},
	lucky_chance =          {},
	benefit =               {}
}
sgs.ai_choicemade_filter =  {
	cardUsed =              {},
	cardResponded =         {},
	skillInvoke =           {},
	skillChoice =           {},
	Nullification =         {},
	playerChosen =          {},
	cardChosen =            {},
	Yiji =                  {},
	Rende =                 {},
	viewCards =             {},
	pindian =               {},
	cardExchange =          {}
}

sgs.card_lack =             {}
sgs.ai_need_damaged =       {}
sgs.ai_debug_func =         {}
sgs.ai_chat_func =          {}
sgs.ai_event_callback =     {}
sgs.explicit_renegade =     false
sgs.ai_NeedPeach =          {}
sgs.ai_defense =            {}


for i=sgs.NonTrigger, sgs.NumOfEvents, 1 do
	sgs.ai_debug_func[i]    ={}
	sgs.ai_chat_func[i]     ={}
	sgs.ai_event_callback[i]={}
end

--东方杀新增列表
sgs.ai_1v1 =            {}
sgs.ai_damageCaused=   {}
sgs.ai_damageInflicted= {}
sgs.ai_needToWake= {}
sgs.ai_no_playerchosen_intention = {}
sgs.ai_damage_prohibit =        {}
sgs.ai_need_bear =      {}
sgs.ai_benefitBySlashed ={}
sgs.ai_DamagedBenefit ={}
sgs.siling_lack =               {}
sgs.attackRange_skill = {}
sgs.ai_skillProperty = {}
sgs.ai_judge_model ={}
sgs.ai_skill_property = {  } --技能属性

sgs.fake_loyalist =     false
sgs.fake_rebel = false
sgs.fake_loyalist_players = {}
sgs.fake_rebel_players = {}
--东方杀相关各类列表
--没做定义的【红白】【黑白】【赛钱】【收藏】【五欲】【渴血】【血裔】
--          【盛宴】【博览】【斗魂】【战意】【冻结】【冰魄】【笨蛋】
--           【真夜】【暗域】【契约】【魔血】
--lose_equip_skill【魔开】【遗忘】
--need_kongcheng【迷彩】
--masochism_skill延时卖血不算？【浴血】【】
--wizard_skill wizard_harm_skill【命运】【绯想】【风水】【博丽】
--priority_skill【借走】【强欲】【锁定】
--save_skill【七曜】
--exclusive_skill
--cardneed_skill【魔法】【借走】
--drawpeach_skill【职责】【春息】
--recover_skill【具现】【半月】
--use_lion_skill【爆衣】
--need_equip_skill【爆衣】【魔开】
--judge_reason【灵气】【破坏】
--no_intention_damage
function setInitialTables()
	sgs.current_mode_players = { lord = 0, loyalist = 0, rebel = 0, renegade = 0 }
	sgs.ai_type_name =          {"Skill", "Basic", "Trick", "Equip"}
	sgs.lose_equip_skill = "kofxiaoji|xiaoji|xuanfeng|nosxuanfeng|mokai|yiwang"
	sgs.need_kongcheng = "lianying|kongcheng|sijian|micai"
	sgs.masochism_skill =       "guixin|yiji|fankui|jieming|xuehen|neoganglie|ganglie|vsganglie|enyuan|fangzhu|nosenyuan|langgu|quanji|" ..
						"zhiyu|renjie|tanlan|tongxin|huashen"..
						"|baochun|jingxia|qingyu"
	sgs.wizard_skill =      "guicai|guidao|jilve|tiandu|luoying|noszhenlie|feixiang|mingyun|fengshui"--boli --huanshi
	sgs.wizard_harm_skill =     "guicai|guidao|jilve|feixiang|mingyun|fengshui" --boli
	--急火优先 包养优先?
	sgs.priority_skill =        "dimeng|haoshi|qingnang|nosjizhi|jizhi|guzheng|qixi|jieyin|guose|duanliang|jujian|fanjian|neofanjian|lijian|" ..
						"noslijian|manjuan|tuxi|qiaobian|yongsi|zhiheng|luoshen|nosrende|rende|mingce|wansha|gongxin|jilve|anxu|" ..
						"qice|yinling|qingcheng|houyuan|zhaoxin|shuangren"..
						"|saiqian|qiangyu|miyao|maihuo|jiushu|weizhi|qingting|shenshou"
	sgs.save_skill =        "jijiu|buyi|nosjiefan|chunlao|hezhou"
	--攻击除外 --其实只有查询雷击目标用到了
	sgs.exclusive_skill =       "huilei|duanchang|wuhun|buqu|dushi"
	sgs.cardneed_skill =        "paoxiao|tianyi|xianzhen|shuangxiong|nosjizhi|jizhi|guose|duanliang|qixi|qingnang|yinling|luoyi|guhuo|nosguhuo|kanpo|" ..
						"jieyin|renjie|zhiheng|nosrende|rende|nosjujian|guicai|guidao|qiaobian|beige|jieyuan|" ..
						"mingce|nosfuhun|lirang|longluo|xuanfeng|xinzhan|dangxian|xiaoguo|neoluoyi|fuhun"..
						"|mofa|jiezou"
	sgs.drawpeach_skill =       "tuxi|qiaobian|zhize|chunxi|toupai"
	sgs.recover_skill =     "nosrende|rende|kofkuanggu|kuanggu|zaiqi|jieyin|qingnang|yinghun|shenzhi|nosmiji|zishou|ganlu|xueji|shangshi|" ..
						"nosshangshi|ytchengxiang|buqu|miji"..
						"|juxian"
	sgs.use_lion_skill =         "duanliang|qixi|guidao|noslijian|lijian|jujian|nosjujian|zhiheng|mingce|yongsi|fenxun|gongqi|" ..
						"yinling|jilve|qingcheng|neoluoyi|diyyicong"..
						"|baoyi|zhanzhen|zhancao|chuanran|weizhi|buming|pingyi|shouhui|hunpo|luanwu|qinlue"
	sgs.need_equip_skill =      "shensu|mingce|jujian|beige|yuanhu|huyuan|gongqi|nosgongqi|yanzheng|qingcheng|neoluoyi|shuijian"..
							"|baoyi|mokai|zhanzhen|zhancao|chuanran|weizhi|buming|pingyi|shouhui|hunpo|luanwu|qinlue|zhancao|wunian|yiwang"
	sgs.judge_reason =      "bazhen|EightDiagram|Blade|wuhun|supply_shortage|tuntian|nosqianxi|nosmiji|indulgence|lightning|baonue"..
									"|nosleiji|leiji|caizhaoji_hujia|tieji|luoshen|ganglie|neoganglie|vsganglie|kofkuanggu"..
							"|lingqi|pohuai|huisu"
	sgs.no_intention_damage = "nuhuo|pohuai|zhuonong|meiling"
	sgs.attackRange_skill = "nengwu|bushu|xiangqi|fanji|xiubu|huantong"

	sgs.Friend_All = 0
	sgs.Friend_Draw = 1
	sgs.Friend_Male = 2
	sgs.Friend_Female = 3
	sgs.Friend_Wounded = 4
	sgs.Friend_MaleWounded = 5
	sgs.Friend_FemaleWounded = 6

	for _, aplayer in sgs.qlist(global_room:getAllPlayers()) do
		table.insert(sgs.role_evaluation, aplayer:objectName())
		table.insert(sgs.ai_role, aplayer:objectName())
		if aplayer:isLord() then
			sgs.role_evaluation[aplayer:objectName()] = {lord = 99999, rebel = 0, loyalist = 99999, renegade = 0}
			sgs.ai_role[aplayer:objectName()] = "loyalist"
		else
			sgs.role_evaluation[aplayer:objectName()] = {rebel = 0, loyalist = 0, renegade = 0}
			sgs.ai_role[aplayer:objectName()] = "neutral"
		end
		sgs.fake_loyalist_players[aplayer:objectName()] = false
		sgs.fake_rebel_players[aplayer:objectName()] = false
	end
end

function SmartAI:initialize(player)
	self.player = player
	self.room = player:getRoom()
	self.role  = player:getRole()
	self.lua_ai = sgs.LuaAI(player)
	self.lua_ai.callback = function(full_method_name, ...)
		--The __FUNCTION__ macro is defined as CLASS_NAME::SUBCLASS_NAME::FUNCTION_NAME
		--in MSVC, while in gcc only FUNCTION_NAME is in place.
		local method_name_start = 1
		while true do
			local found = string.find(full_method_name, "::", method_name_start)
			if found ~= nil then
				method_name_start = found + 2
			else
				break
			end
		end
		local method_name = string.sub(full_method_name, method_name_start)
		local method = self[method_name]
		if method then
			local success, result1, result2
			success, result1, result2 = pcall(method, self, ...)
			if not success then
				self.room:writeToConsole(result1)
				self.room:writeToConsole(method_name)
				self.room:writeToConsole(debug.traceback())
				self.room:outputEventStack()
			else
				return result1, result2
			end
		end
	end

	self.retain = 2
	self.keepValue = {}
	self.kept = {}
	self.keepdata = {}
	self.predictedRange = 1
	self.slashAvail = 1
	if not sgs.initialized then
		sgs.initialized = true
		sgs.ais = {}
		sgs.turncount = 0
		sgs.debugmode = false
		global_room = self.room
		global_room:writeToConsole(version .. ", Powered by " .. _VERSION)

		setInitialTables()
		if sgs.isRolePredictable() then
			for _, aplayer in sgs.qlist(global_room:getAllPlayers()) do
				if aplayer:getRole() == "renegade" then sgs.explicit_renegade = true end
				if aplayer:getRole() ~= "lord" then
					sgs.role_evaluation[aplayer:objectName()][aplayer:getRole()] = 65535
					sgs.ai_role[aplayer:objectName()] = aplayer:getRole()
				end
			end
		end
	end

	sgs.ais[player:objectName()] = self

	sgs.card_lack[player:objectName()] = {}
	sgs.card_lack[player:objectName()]["Slash"] = 0
	sgs.card_lack[player:objectName()]["Jink"] = 0
	sgs.card_lack[player:objectName()]["Peach"] = 0
	sgs.ai_NeedPeach[player:objectName()] = 0
	--死灵的判断
	sgs.siling_lack[player:objectName()] = {}
	sgs.siling_lack[player:objectName()]["Black"] = 0
	sgs.siling_lack[player:objectName()]["Red"] = 0

	if self.player:isLord() and not sgs.GetConfig("EnableHegemony", false) then
		if (sgs.ai_chaofeng[self.player:getGeneralName()] or 0) < 3 then
			sgs.ai_chaofeng[self.player:getGeneralName()] = 3
		end
	end

	self:updateAlivePlayerRoles()
	self:updatePlayers()
end

function sgs.cloneCard(name, suit, number)
	suit = suit or sgs.Card_SuitToBeDecided
	number = number or -1
	local card = sgs.Sanguosha:cloneCard(name, suit, number)
	card:deleteLater()
	return card
end

function sgs.getCardNumAtCertainPlace(card, player, place)
	if not card:isVirtualCard() and place == sgs.Player_PlaceHand then return 1
	elseif card:subcardsLength() == 0 then return 0
	else
		local num = 0
		for _, id in sgs.qlist(card:getSubcards()) do
			if place == sgs.Player_PlaceHand then
				if player:handCards():contains(id) then num = num + 1 end
			elseif place == sgs.Player_PlaceEquip then
				if player:hasEquip(sgs.Sanguosha:getCard(id)) then num = num + 1 end
			end
		end
		return num
	end
end

function sgs.getValue(player)
	if not player then global_room:writeToConsole(debug.traceback()) end
	return player:getHp() * 2 + player:getHandcardNum()
end

--东方杀相关
--【红白】
function sgs.getDefense(player, gameProcess)
	if not player then return 0 end
	if not sgs.ai_updateDefense and global_room:getCurrent() and not sgs.GetConfig("EnableHegemony", false) then
		return sgs.ai_defense[player:objectName()]
	end
	--local defense = math.min(sgs.getValue(player), player:getHp() * 3)
	local handcard = player:getHandcardNum()
	if player:hasSkill("shanji") then
		handcard = handcard+player:getPile("piao"):length()
	end

	if sgs.touhouCanWoodenOx(player) then
		handcard = handcard+player:getPile("wooden_ox"):length()
	end
	local defense = math.min(player:getHp() * 2 + handcard , player:getHp() * 3)

	local attacker = global_room:getCurrent()
	local hasEightDiagram = false
	if player:hasArmorEffect("EightDiagram") or player:hasArmorEffect("bazhen") then
		hasEightDiagram = true
	end

	if player:getArmor() then defense = defense + 2 end
	if not player:getArmor() and player:hasSkill("yizhong") then defense = defense + 2 end

	if hasEightDiagram then
		defense = defense + 1.3
		if player:hasSkill("tiandu") then defense = defense + 0.6 end
		if player:hasSkill("leiji") then defense = defense + 0.4 end
		if player:hasSkill("nosleiji") then defense = defense + 0.4 end
	end

	if player:hasSkills("tuntian+zaoxian") then defense = defense + player:getHandcardNum() * 0.4 end
	if player:hasSkill("aocai") and player:getPhase() == sgs.Player_NotActive then defense = defense + 0.3 end
	if attacker or gameProcess then
		local m = sgs.masochism_skill:split("|")
		for _, masochism in ipairs(m) do
			if player:hasSkill(masochism) and sgs.isGoodHp(player) then
				defense = defense + 1
			end
		end
		if player:getMark("@tied") > 0 then defense = defense + 1 end
		if player:hasSkill("jieming") then defense = defense + 3 end
		if player:hasSkill("yiji") then defense = defense + 3 end
		if player:hasSkill("guixin") then defense = defense + 4 end
	end

	if not gameProcess and not sgs.isGoodTarget(player) then defense = defense + 10 end
	if player:hasSkills("rende|nosrende") and player:getHp() > 2 then defense = defense + 1 end
	if player:hasSkill("kuanggu") and player:getHp() > 1 then defense = defense + 0.2 end
	if player:hasSkill("kofkuanggu") and player:getHp() > 1 then defense = defense + 0.25 end
	if player:hasSkill("zaiqi") and player:getHp() > 1 then defense = defense + 0.35 end
	if player:hasSkill("tianming") then defense = defense + 0.1 end

	if player:getHp() > getBestHp(player) then defense = defense + 0.8 end
	if player:getHp() <= 2 then defense = defense - 0.4 end

	--if player:hasSkill("tianxiang") then defense = defense + player:getHandcardNum() * 0.5 end

	if not gameProcess and player:getHandcardNum() == 0 then
		if player:getHp() <= 1 then defense = defense - 2.5 end
		if player:getHp() == 2 then defense = defense - 1.5 end
		if not hasEightDiagram then defense = defense - 2 end
	end

	if isLord(player) then
		defense = defense - 0.4
		if sgs.isLordInDanger() then defense = defense - 0.7 end
	end

	if not gameProcess and (sgs.ai_chaofeng[player:getGeneralName()] or 0) >= 3 then
		defense = defense - math.max(6, (sgs.ai_chaofeng[player:getGeneralName()] or 0)) * 0.035
	end

	if not player:faceUp() then defense = defense - 0.35 end
	if player:containsTrick("indulgence") and not player:containsTrick("YanxiaoCard") then defense = defense - 0.15 end
	if player:containsTrick("supply_shortage") and not player:containsTrick("YanxiaoCard") then defense = defense - 0.15 end

	if not gameProcess and not hasEightDiagram then
		if player:hasSkill("jijiu") then defense = defense - 3 end
		if player:hasSkill("dimeng") then defense = defense - 2.5 end
		if player:hasSkill("guzheng") and getKnownCard(player, attacker, "Jink", true) == 0 then defense = defense - 2.5 end
		if player:hasSkill("qiaobian") then defense = defense - 2.4 end
		if player:hasSkill("jieyin") then defense = defense - 2.3 end
		if player:hasSkills("noslijian|lijian") then defense = defense - 2.2 end
		if player:hasSkill("nosmiji") and player:isWounded() then defense = defense - 1.5 end
		if player:hasSkill("xiliang") and getKnownCard(player, attacker, "Jink", true) == 0 then defense = defense - 2 end
		--if player:hasSkill("shouye") then defense = defense - 2 end
	end

	defense = defense + player:getHandcardNum() * 0.25

	sgs.ai_defense[player:objectName()] = defense
	return defense
end

function SmartAI:assignKeep(num, start)
	num = self.player:getHandcardNum()
	if num <= 0 then return end
	if start then
		self.keepValue = {}
		self.kept = {}
		--[[
			通常的保留顺序
			"peach-1" = 7,
			"peach-2" = 5.8, "jink-1" = 5.2,
			"peach-3" = 4.5, "analeptic-1" = 4.1,
			"jink-2" = 4.0, "ExNihilo-1" = 3.9, "nullification-1" = 3.8, "thunderslash-1" = 3.66 "fireslash-1" = 3.63
			"slash-1" = 3.6 indulgence-1 = 3.5 SupplyShortage-1 = 3.48 snatch-1 = 3.46 Dismantlement-1 = 3.44 Duel-1 = 3.42
			Collateral-1 = 3.40 ArcheryAttack-1 = 3.38 SavageAssault-1 = 3.36 IronChain = 3.34 GodSalvation-1 = 3.32, Fireattack-1 = 3.3 "peach-4" = 3.1
			"analeptic-2" = 2.9, "jink-3" = 2.7 ExNihilo-2 = 2.7 nullification-2 = 2.6 thunderslash-2 = 2.46 fireslash-2 = 2.43 slash-2 = 2.4
			...
			Weapon-1 = 2.08 Armor-1 = 2.06 DefensiveHorse-1 = 2.04 OffensiveHorse-1 = 2
			...
			AmazingGrace-1 = -9 Lightning-1 = -10
		]]
		self.keepdata = {}
		for k, v in pairs(sgs.ai_keep_value) do
			self.keepdata[k] = v
		end

		if not self:isWeak() or num >= 4 then
			for _, friend in ipairs(self.friends_noself) do
				if self:willSkipDrawPhase(friend) or self:willSkipPlayPhase(friend) then
					self.keepdata.Nullification = 5.5
					break
				end
			end
		end

	if not self:isWeak() then
		local needDamaged = false
		if self.player:getHp() > getBestHp(self.player) then needDamaged = true end
		if not needDamaged and not sgs.isGoodTarget(self.player, self.friends, self) then needDamaged = true end
		if not needDamaged then
			for _, skill in sgs.qlist(self.player:getVisibleSkillList()) do
				local callback = sgs.ai_need_damaged[skill:objectName()]
				if type(callback) == "function" and callback(self, nil, self.player) then
					needDamaged = true
					break
				end
			end
		end
		if needDamaged then
			self.keepdata.ThunderSlash = 5.2
			self.keepdata.FireSlash = 5.1
			self.keepdata.Slash = 5
			self.keepdata.Jink = 4.5
		end
	end

		if self:isWeak() then
			for _, ap in sgs.qlist(self.room:getAlivePlayers()) do
				if ap:hasSkill("buyi") and self:isFriend(ap) then
					self.keepdata = { Peach = 10, TrickCard = 8, EquipCard = 7.9 }
					break
				end
			end
		end

		for _, askill in sgs.qlist(self.player:getVisibleSkillList()) do
			local skilltable = sgs[askill:objectName() .. "_keep_value"]
			if skilltable then
				for k, v in pairs(skilltable) do
					self.keepdata[k] = v
				end
			end
		end
	end

	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards, true, self.kept, true)

	local resetCards = function(allcards)
		local result = {}
		for _, a in ipairs(allcards) do
			local found
			for _, b in ipairs(self.kept) do
				if a:getEffectiveId() == b:getEffectiveId() then
					found = true
					break
				end
			end
			if not found then table.insert(result, a) end
		end
		return result
	end

	for i = 1 , num do
		for _, card in ipairs(cards) do
			self.keepValue[card:getId()] = self:getKeepValue(card, self.kept)
			table.insert(self.kept, card)
			break
		end
		cards = resetCards(cards)
	end
end

function SmartAI:getKeepValue(card, kept, Write)
	if not kept then
		local CardPlace = self.room:getCardPlace(card:getEffectiveId())
		if CardPlace == sgs.Player_PlaceHand then
			local v = self.keepValue[card:getId()]
			if not v then
				-- self.room:writeToConsole(debug.traceback())
				v = 0
			end
			return v
		else
			local at_play = self.player:getPhase() == sgs.Player_Play
			if card:isKindOf("SilverLion") and self.player:isWounded() then return -10
			elseif self:hasSkills(sgs.lose_equip_skill) then
				if card:isKindOf("OffensiveHorse") then return -10
				elseif card:isKindOf("Weapon") then return -9.9
				elseif card:isKindOf("OffensiveHorse") then return -9.8
				else return -9.7
				end
			elseif self.player:hasSkills("bazhen|yizhong") and card:isKindOf("Armor") then return -8
			elseif self:needKongcheng() then return 5.0
			elseif card:isKindOf("Armor") then return self:isWeak() and 5.2 or 3.2
			elseif card:isKindOf("DefensiveHorse") then return self:isWeak() and 4.3 or 3.19
			elseif card:isKindOf("Weapon") then return at_play and self:slashIsAvailable() and self:getCardsNum("Slash") > 0 and 3.39 or 3.2
			else return 3.19
			end
		end
	end

	local value_suit, value_number, newvalue = 0, 0, 0

	if Write then
		local class_name = card:getClassName()
		local suit_string = card:getSuitString()
		local number = card:getNumber()
		local i = 0

		for _, askill in sgs.qlist(self.player:getVisibleSkillList()) do
			if sgs[askill:objectName() .. "_suit_value"] then
				local v = sgs[askill:objectName() .. "_suit_value"][suit_string]
				if v then
					i = i + 1
					value_suit = value_suit + v
				end
			end
		end
		if i > 0 then value_suit = value_suit / i end

		i = 0
		for _, askill in sgs.qlist(self.player:getVisibleSkillList()) do
			if sgs[askill:objectName() .. "_number_value"] then
				local v = sgs[askill:objectName() .. "_number_value"][tostring(number)]
				if v then
					i = i + 1
					value_number = value_number + v
				end
			end
		end
		if i > 0 then value_number = value_number / i end
	end

	local maxvalue, mostvaluable_class = -10, card:getClassName()
	for k, v in pairs(self.keepdata) do
		if isCard(k, card, self.player) and v > maxvalue then
			maxvalue = v
			mostvaluable_class = k
		end
	end

	newvalue = maxvalue + value_suit + value_number

	if Write then return newvalue end

	newvalue = self.keepValue[card:getId()] or 0
	local dec = 0
	for _, acard in ipairs(kept) do
		if isCard(mostvaluable_class, acard, self.player) and isCard(mostvaluable_class, card, self.player) then
			newvalue = newvalue - 1.2 - dec
			dec = dec + 0.1
		elseif acard:isKindOf("Slash") and card:isKindOf("Slash") then
			newvalue = newvalue - 1.2 - dec
			dec = dec + 0.1
		end
	end
	return newvalue
end

function SmartAI:getUseValue(card)
	local class_name = card:getClassName()
	local v = sgs.ai_use_value[class_name] or 0
	if class_name == "LuaSkillCard" and card:isKindOf("LuaSkillCard") then
		v = sgs.ai_use_value[card:objectName()] or 0
	end

	if card:isKindOf("GuhuoCard") or card:isKindOf("NosGuhuoCard") then
		local userstring = card:toString()
		userstring = (userstring:split(":"))[3]
		local guhuocard = sgs.cloneCard(userstring, card:getSuit(), card:getNumber())
		local usevalue = self:getUseValue(guhuocard) + #self.enemies * 0.3
		if sgs.Sanguosha:getCard(card:getSubcards():first()):objectName() == userstring
			and (card:isKindOf("GuhuoCard") or card:getSuit() == sgs.Card_Heart) then usevalue = usevalue + 3 end
		return usevalue
	end

	if card:getTypeId() == sgs.Card_TypeEquip then
		if self.player:hasEquip(card) then
			if card:isKindOf("OffensiveHorse") and self.player:getAttackRange() > 2 then return 5.5 end
			if card:isKindOf("DefensiveHorse") and self:hasEightDiagramEffect() then return 5.5 end
			return 9
		end
		if not self:getSameEquip(card) then v = 6.7 end
		if self.weaponUsed and card:isKindOf("Weapon") then v = 2 end
		if self.player:hasSkills("qiangxi|taichen|zhulou") and card:isKindOf("Weapon") then v = 2 end
		if self.player:hasSkill("kurou") and card:isKindOf("Crossbow") then return 9 end
		if self.player:hasSkills("bazhen|yizhong") and card:isKindOf("Armor") then v = 2 end
		if self.role == "loyalist" and self.player:getKingdom()=="wei" and not self.player:hasSkill("bazhen") and getLord(self.player) and getLord(self.player):hasLordSkill("hujia") and card:isKindOf("EightDiagram") then
			v = 9
		end
		if self.player:hasSkills(sgs.lose_equip_skill) then return 10 end
	elseif card:getTypeId() == sgs.Card_TypeBasic then
		if card:isKindOf("Slash") then
			v = sgs.ai_use_value[class_name] or 0
			if self.player:hasFlag("TianyiSuccess") or self.player:hasFlag("JiangchiInvoke")
				or self:hasHeavySlashDamage(self.player, card) then v = 8.7 end
			if self.player:getPhase() == sgs.Player_Play and self:slashIsAvailable() and #self.enemies > 0 and self:getCardsNum("Slash") == 1 then v = v + 5 end
			if self:hasCrossbowEffect() then v = v + 4 end
			if card:getSkillName() == "Spear"   then v = v - 1 end
			if card:getSkillName() == "fuhun" then v = v + (self.player:getPhase() == sgs.Player_Play and 1 or -1) end
		elseif card:isKindOf("Jink") then
			if self:getCardsNum("Jink") > 1 then v = v-6 end
		elseif card:isKindOf("Peach") then
			if self.player:isWounded() then v = v + 6 end
		end
	elseif card:getTypeId() == sgs.Card_TypeTrick then
		if self.player:getWeapon() and not self.player:hasSkills(sgs.lose_equip_skill) and card:isKindOf("Collateral") then v = 2 end
		if card:getSkillName() == "shuangxiong" then v = 6 end
		if card:isKindOf("Duel") then v = v + self:getCardsNum("Slash") * 2 end
		if self.player:hasSkill("nosjizhi") then v = v + 4 end
		if self.player:hasSkill("jizhi") then v = v + 3 end
	end

	if self:hasSkills(sgs.need_kongcheng) then
		if self.player:getHandcardNum() == 1 then v = 10 end
	end
	if self.player:hasWeapon("Halberd") and card:isKindOf("Slash") and self.player:isLastHandCard(card) then v = 10 end
	if self.player:getPhase() == sgs.Player_Play then v = self:adjustUsePriority(card, v) end
	return v
end

function SmartAI:getUsePriority(card)  --优先度 要考虑目标角色才合适。。。。比如使用杀 目标为司马时 能不能先吃桃。。。
	local class_name = card:getClassName()
	local v = 0
	if card:isKindOf("EquipCard") then
		if self:hasSkills(sgs.lose_equip_skill) then return 15 end
		if card:isKindOf("Armor") and not self.player:getArmor() then v = (sgs.ai_use_priority[class_name] or 0) + 5.2
		elseif card:isKindOf("Weapon") and not self.player:getWeapon() then v = (sgs.ai_use_priority[class_name] or 0) + 3
		elseif card:isKindOf("DefensiveHorse") and not self.player:getDefensiveHorse() then v = 5.8
		elseif card:isKindOf("OffensiveHorse") and not self.player:getOffensiveHorse() then v = 5.5
		elseif card:isKindOf("Treasure") and not self.player:getTreasure() then v = (sgs.ai_use_priority[class_name] or 0) + 2
		end
		return v
	end

	v = sgs.ai_use_priority[class_name] or 0
	if class_name == "LuaSkillCard" and card:isKindOf("LuaSkillCard") then
		v = sgs.ai_use_priority[card:objectName()] or 0
	end

	return self:adjustUsePriority(card, v)
end

function SmartAI:adjustUsePriority(card, v)
	local suits = {"club", "spade", "diamond", "heart"}

	if card:getTypeId() == sgs.Card_Skill then return v end

	for _, askill in sgs.qlist(self.player:getVisibleSkillList()) do
		local callback = sgs.ai_suit_priority[askill:objectName()]
		if type(callback) == "function" then
			suits = callback(self, card):split("|")
			break
		elseif type(callback) == "string" then
			suits = callback:split("|")
			break
		end
	end

	table.insert(suits, "no_suit")
	if card:isKindOf("Slash") then
		if card:getSkillName() == "Spear" then v = v - 0.1 end
		if card:isRed() then
			v = v - 0.05
			--if self.slashAvail == 1 and self.player:hasSkill("jie") then v = v + 0.21
			--else v = v - 0.05 end
		end
		if card:isKindOf("NatureSlash") then v = v - 0.1 end
		if card:getSkillName() == "fuhun" then v = v + (self.player:getPhase() == sgs.Player_Play and 0.21 or -0.1) end
		if self.player:hasSkill("jiang") and card:isRed() then v = v + 0.21 end
		--if self.player:hasSkill("guaili") and card:isRed() then v = v + 0.21 end
		if self.player:hasSkill("wushen") and card:getSuit() == sgs.Card_Heart then v = v + 0.11 end	
	end
	if self.player:hasSkill("mingzhe") and card:isRed() then v = v + (self.player:getPhase() ~= sgs.Player_NotActive and 0.05 or -0.05) end
	if card:isKindOf("Peach") and card:getSkillName() == "shende" then v = v + 0.21 end
	if sgs.touhouCanWoodenOx(self.player) and not self.player:getPile("wooden_ox"):isEmpty()  then  --and self.player:getMark("@tianyi_Treasure") ==0
		local id_table = {}
		if not card:isVirtualCard() then id_table = { card:getEffectiveId() }
		else id_table = sgs.QList2Table(card:getSubcards()) end
		for _, id in ipairs(id_table) do
			if self.player:getPile("wooden_ox"):contains(id) then
				v = v + 0.05
				if self:hasWeiya() then
					v = v + 0.2
				end
				break
			end
		end
	end



	local suits_value = {}
	for index, suit in ipairs(suits) do
		suits_value[suit] = -index
	end
	v = v + (suits_value[card:getSuitString()] or 0) / 1000
	v = v + (13 - card:getNumber()) / 10000
	return v
end

function SmartAI:getDynamicUsePriority(card)
	if not card then return 0 end

	if card:hasFlag("AIGlobal_KillOff") then return 15 end
	local class_name = card:getClassName()
	local dynamic_value

	-- direct control
	if card:isKindOf("AmazingGrace") or card:isKindOf("FengrangCard") then
		local zhugeliang = self.room:findPlayerBySkillName("kongcheng")
		if zhugeliang and self:isEnemy(zhugeliang) and zhugeliang:isKongcheng() then
			return math.max(sgs.ai_use_priority.Slash, sgs.ai_use_priority.Duel) + 0.1
		end
	end
	if card:isKindOf("Peach") and self.player:hasSkills("kuanggu|kofkuanggu") then return 1.01 end
	if card:isKindOf("YanxiaoCard") and self.player:containsTrick("YanxiaoCard") then return 0.1 end
	if card:isKindOf("DelayedTrick") and not card:isKindOf("YanxiaoCard") and #card:getSkillName() > 0 then
		return (sgs.ai_use_priority[card:getClassName()] or 0.01) - 0.01
	end

	if self.player:hasSkill("danshou")
		and (card:isKindOf("Slash") or card:isKindOf("Duel") or card:isKindOf("AOE")
			or sgs.dynamic_value.damage_card[card:getClassName()]) then
		return 0
	end
	if card:isKindOf("Duel") then
		--[[if self:getCardsNum("FireAttack") > 0 and dummy_use.to and not dummy_use.to:isEmpty() then
			for _, p in sgs.qlist(dummy_use.to) do
				if p:getHp() == 1 then return sgs.ai_use_priority.FireAttack + 0.1 end
			end
		end]]
		if self.player:hasSkill("jidu") then
			return 8
		end
		if self:hasCrossbowEffect()
			or self.player:hasFlag("XianzhenSuccess")
			or self.player:canSlashWithoutCrossbow()
			or sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, self.player, sgs.cloneCard("slash")) > 0
			or self.player:hasUsed("FenxunCard") then
			return sgs.ai_use_priority.Slash - 0.1
		end
	end



	local value = self:getUsePriority(card) or 0
	if card:getSkillName() == "huaxiang" then --不起作用。。。
		value = value + 0.1
	end

	if card:getTypeId() == sgs.Card_TypeEquip then
		if self:hasSkills(sgs.lose_equip_skill) then value = value + 12 end
	end
	if card:isKindOf("BasicCard") and card:getSkillName() == "beishui" then
		if card:getSubcards():length() > 1 then
			value = value - 2
		end
	end
	if card:isKindOf("AmazingGrace") or card:isKindOf("FengrangCard") then
		dynamic_value = 10
		for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			dynamic_value = dynamic_value - 1
			if self:isEnemy(player) then dynamic_value = dynamic_value - ((player:getHandcardNum() + player:getHp()) / player:getHp()) * dynamic_value
			else dynamic_value = dynamic_value + ((player:getHandcardNum() + player:getHp()) / player:getHp()) * dynamic_value
			end
		end
		value = value + dynamic_value
	end

	return value
end


function SmartAI:cardNeed(card)
	if not self.friends then self.room:writeToConsole(debug.traceback()) self.room:writeToConsole(sgs.turncount) return end
	local class_name = card:getClassName()
	local suit_string = card:getSuitString()
	local value
	if card:isKindOf("Peach") then
		self:sort(self.friends,"hp")
		if self.friends[1]:getHp() < 2 then return 10 end
		if (self.player:getHp() < 3 or self.player:getLostHp() > 1 and not self:hasSkills("buqu")) or self:hasSkills("kurou|benghuai") then return 14 end
		return self:getUseValue(card)
	end
	local wuguotai = self.room:findPlayerBySkillName("buyi")
	if wuguotai and self:isFriend(wuguotai) and not card:isKindOf("BasicCard") then
		if (self.player:getHp() < 3 or self.player:getLostHp() > 1 and not self:hasSkills("buqu")) or self:hasSkills("kurou|benghuai") then return 13 end
	end
	if self:isWeak() and card:isKindOf("Jink") and self:getCardsNum("Jink") < 1 then return 12 end

	local i = 0
	for _, askill in sgs.qlist(self.player:getVisibleSkillList()) do
		if sgs[askill:objectName() .. "_keep_value"] then
			local v = sgs[askill:objectName() .. "_keep_value"][class_name]
			if v then
				i = i + 1
				if value then value = value + v else value = v end
			end
		end
	end
	if value then return value / i + 4 end
	i = 0
	for _, askill in sgs.qlist(self.player:getVisibleSkillList()) do
		if sgs[askill:objectName() .. "_suit_value"] then
			local v = sgs[askill:objectName() .. "_suit_value"][suit_string]
			if v then
				i = i + 1
				if value then value = value + v else value = v end
			end
		end
	end
	if value then return value / i + 4 end

	if card:isKindOf("Slash") and self:getCardsNum("Slash") == 0 then return 5.9 end
	if card:isKindOf("Analeptic") then
		if self.player:getHp() < 2 then return 10 end
	end
	if card:isKindOf("Slash") and (self:getCardsNum("Slash") > 0) then return 4 end
	if card:isKindOf("Crossbow") and  self:hasSkills("luoshen|yongsi|kurou|keji|wusheng|wushen",self.player) then return 20 end
	if card:isKindOf("Axe") and  self:hasSkills("luoyi|jiushi|jiuchi|pojun",self.player) then return 15 end
	if card:isKindOf("Weapon") and (not self.player:getWeapon()) and (self:getCardsNum("Slash") > 1) then return 6 end
	if card:isKindOf("Nullification") and self:getCardsNum("Nullification") == 0 then
		if self:willSkipPlayPhase() or self:willSkipDrawPhase() then return 10 end
		for _,friend in ipairs(self.friends) do
			if self:willSkipPlayPhase(friend) or self:willSkipDrawPhase(friend) then return 9 end
		end
		return 6
	end
	return self:getUseValue(card)
end

-- compare functions
sgs.ai_compare_funcs = {
	hp = function(a, b)
		local c1 = a:getHp()
		local c2 = b:getHp()
		if c1 == c2 then
			return sgs.ai_compare_funcs.defense(a, b)
		else
			return c1 < c2
		end
	end,

	handcard = function(a, b)
		local c1 = a:getHandcardNum()
		local c2 = b:getHandcardNum()
		if c1 == c2 then
			return sgs.ai_compare_funcs.defense(a, b)
		else
			return c1 < c2
		end
	end,

	handcard_defense = function(a, b)
		local c1 = a:getHandcardNum()
		local c2 = b:getHandcardNum()
		if c1 == c2 then
			return  sgs.ai_compare_funcs.defense(a, b)
		else
			return c1 < c2
		end
	end,

	value = function(a, b)
		return sgs.getValue(a) < sgs.getValue(b)
	end,

	chaofeng = function(a, b)
		local c1 = sgs.ai_chaofeng[a:getGeneralName()]  or 0
		local c2 = sgs.ai_chaofeng[b:getGeneralName()] or 0

		if c1 == c2 then
			return sgs.ai_compare_funcs.value(a, b)
		else
			return c1 > c2
		end
	end,

	defense = function(a,b)
		return sgs.getDefenseSlash(a) < sgs.getDefenseSlash(b)
	end,

	threat = function (a, b)
		local players = sgs.QList2Table(a:getRoom():getOtherPlayers(a))
		local d1 = a:getHandcardNum()
		for _, player in ipairs(players) do
			if a:canSlash(player) then
				d1 = d1+10/(sgs.getDefense(player))
			end
		end
		players = sgs.QList2Table(b:getRoom():getOtherPlayers(b))
		local d2 = b:getHandcardNum()
		for _, player in ipairs(players) do
			if b:canSlash(player) then
				d2 = d2+10/(sgs.getDefense(player))
			end
		end

		local c1 = sgs.ai_chaofeng[a:getGeneralName()]  or 0
		local c2 = sgs.ai_chaofeng[b:getGeneralName()] or 0

		return d1+c1/2 > d2+c2/2
	end,
}

function SmartAI:sort(players, key)
	if not players then self.room:writeToConsole(debug.traceback()) end
	if #players == 0 then return end
	function _sort(players, key)
		local func = sgs.ai_compare_funcs[key or "defense"]
		table.sort(players, func)
	end
	if not pcall(_sort, players, key) then self.room:writeToConsole(debug.traceback()) end
end

function SmartAI:sortByKeepValue(cards, inverse, kept, Write)

	local function adjustkeepvalue(card, v)
		local suits = {"club", "spade", "diamond", "heart"}
		for _, askill in sgs.qlist(self.player:getVisibleSkillList()) do
			local callback = sgs.ai_suit_priority[askill:objectName()]
			if type(callback) == "function" then
				suits = callback(self, card):split("|")
				break
			elseif type(callback) == "string" then
				suits = callback:split("|")
				break
			end
		end
		table.insert(suits, "no_suit")

		if card:isKindOf("Slash") then
			if card:isRed() then v = v + 0.02 end
			if card:isKindOf("NatureSlash") then v = v + 0.03 end
			if self.player:hasSkill("jiang") and card:isRed() then v = v + 0.04 end
			--if self.player:hasSkill("guaili") and card:isRed() then v = v + 0.04 end
			if self.player:hasSkill("wushen") and card:getSuit() == sgs.Card_Heart then v = v + 0.03 end
		end
		if self.player:hasSkill("mingzhe") and card:isRed() then v = v + 0.05 end

		local suits_value = {}
		for index,suit in ipairs(suits) do
			suits_value[suit] = index * 2
		end
		v = v + (suits_value[card:getSuitString()] or 0) / 100
		v = v + card:getNumber() / 500
		return v
	end

	local compare_func = function(a,b)
		local value1 = self:getKeepValue(a, kept, Write)
		local value2 = self:getKeepValue(b, kept, Write)

		local v1 = adjustkeepvalue(a, value1)
		local v2 = adjustkeepvalue(b, value2)

		if Write then
			self.keepValue[a:getId()] = v1
			self.keepValue[b:getId()] = v2
		end

		if v1 ~= v2 then
			if inverse then return v1 > v2 end
			return v1 < v2
		else
			if not inverse then return a:getNumber() > b:getNumber() end
			return a:getNumber() < b:getNumber()
		end
	end

	table.sort(cards, compare_func)
end

function SmartAI:sortByUseValue(cards, inverse)
	local compare_func = function(a, b)
		local value1 = self:getUseValue(a)
		local value2 = self:getUseValue(b)

		if value1 ~= value2 then
			if not inverse then return value1 > value2 end
			return value1 < value2
		else
			if not inverse then return a:getNumber() > b:getNumber() end
			return a:getNumber() < b:getNumber()
		end
	end

	table.sort(cards, compare_func)
end

function SmartAI:sortByUsePriority(cards, player)
	local compare_func = function(a,b)
		local value1 = self:getUsePriority(a)
		local value2 = self:getUsePriority(b)

		if value1 ~= value2 then
			return value1 > value2
		else
			--should consider the skill dangjia if your kingdonm is belong to "wai"
			return a:getNumber() > b:getNumber()
		end
	end
	table.sort(cards, compare_func)
end

function SmartAI:sortByDynamicUsePriority(cards)
	local compare_func = function(a,b)
		local value1 = self:getDynamicUsePriority(a)
		local value2 = self:getDynamicUsePriority(b)

		if value1 ~= value2 then
			return value1 > value2
		else
			return a and a:getTypeId() ~= sgs.Card_TypeSkill and not (b and b:getTypeId() ~= sgs.Card_TypeSkill)
		end
	end

	table.sort(cards, compare_func)
end

function SmartAI:sortByCardNeed(cards, inverse)
	local compare_func = function(a,b)
		local value1 = self:cardNeed(a)
		local value2 = self:cardNeed(b)

		if value1 ~= value2 then
			if inverse then return value1 > value2 end
			return value1 < value2
		else
			if not inverse then return a:getNumber() > b:getNumber() end
			return a:getNumber() < b:getNumber()
		end
	end

	table.sort(cards, compare_func)
end


function SmartAI:getPriorTarget()
	if #self.enemies == 0 then return end
	self:sort(self.enemies, "defense")
	return self.enemies[1]
end

function sgs.modifiedRoleEvaluation()
	local players = global_room:getOtherPlayers(global_room:getLord())

	if players:length() == 1 then return false end

	local rebel, loyalist, renegade = {}, {}, {}
	local rebel_num = sgs.current_mode_players["rebel"]
	local loyalist_num = sgs.current_mode_players["loyalist"]
	local renegade_num = sgs.current_mode_players["renegade"]

	for _, player in sgs.qlist(players) do
		if sgs.ai_role[player:objectName()] == "rebel" then table.insert(rebel, player)
		elseif sgs.ai_role[player:objectName()] == "loyalist" then table.insert(loyalist, player)
		elseif sgs.ai_role[player:objectName()] == "renegade" then table.insert(renegade, player) end
	end

	local sort_func = {
		rebel = function(a, b)
			return sgs.role_evaluation[a:objectName()]["loyalist"] < sgs.role_evaluation[b:objectName()]["loyalist"]
		end,
		renegade = function(a, b)
			return sgs.role_evaluation[a:objectName()]["renegade"] > sgs.role_evaluation[b:objectName()]["renegade"]
		end
	}
	if #renegade > 0 and #loyalist >= loyalist_num + renegade_num and #rebel < rebel_num then
		local newplayers = {}
		table.insertTable(newplayers, loyalist)
		table.insertTable(newplayers, renegade)
		table.sort(newplayers, sort_func["rebel"])
		for _, p in ipairs(newplayers) do
			local name = p:objectName()
			if sgs.role_evaluation[name]["loyalist"] < 0 and sgs.role_evaluation[name]["renegade"] > 0 then
				sgs.role_evaluation[name]["loyalist"] = math.min(-sgs.role_evaluation[name]["renegade"], sgs.role_evaluation[name]["loyalist"])
				sgs.role_evaluation[name]["renegade"] = 0
				sgs.outputRoleValues(p, 0)
				global_room:writeToConsole("rebel:" .. p:getGeneralName() .." Modified Success!")
				break
			end
		end
	end

	if renegade_num > 0 and #renegade == 0 then
		if #loyalist > 1 and #loyalist >= loyalist_num + renegade_num then
			table.sort(loyalist, sort_func["renegade"])
			if sgs.role_evaluation[loyalist[1]:objectName()]["renegade"] > 0 and sgs.role_evaluation[loyalist[2]:objectName()]["renegade"] == 0 then
				sgs.role_evaluation[loyalist[1]:objectName()]["renegade"] = sgs.role_evaluation[loyalist[1]:objectName()]["renegade"] + 5
				sgs.ai_role[loyalist[1]:objectName()] = "renegade"
				sgs.outputRoleValues(loyalist[1], 5)
				--五谷的错误根源在这里。。。。
				global_room:writeToConsole("renegade:" .. loyalist[1]:getGeneralName() .." Modified Success!")
			end
		end
	end

end

function sgs.evaluatePlayerRole(player)
	if not player then global_room:writeToConsole("Player is empty in role's evaluation!") return end
	local function test_func(player)
		if player:isLord() then return "loyalist" else return "." end
	end
	local res = pcall(test_func, player)
	if not res then global_room:writeToConsole(debug.traceback()) return elseif res == "loyalist" then return "loyalist" end
	if sgs.isRolePredictable() then return player:getRole() end
	return sgs.ai_role[player:objectName()]
end

function sgs.compareRoleEvaluation(player, first, second)
	if player:isLord() then return "loyalist" end
	if sgs.isRolePredictable() then return player:getRole() end
	if (first == "renegade" or second == "renegade") and sgs.ai_role[player:objectName()] == "renegade" then return "renegade" end
	if sgs.ai_role[player:objectName()] == first then return first end
	if sgs.ai_role[player:objectName()] == second then return second end
	return "neutral"
end

function sgs.isRolePredictable(classical)
	if not classical and sgs.GetConfig("RolePredictable", false) then return true end
	local mode = string.lower(global_room:getMode())
	local isMini = (mode:find("mini") or mode:find("custom_scenario"))
	if (not mode:find("0") and not isMini) or mode:find("02p") or mode:find("02_1v1") or mode:find("04_1v3")
		or mode == "06_3v3" or mode == "06_xmode" or (not classical and isMini) then return true end
	return false
end

function sgs.findIntersectionSkills(first, second)
	if type(first) == "string" then first = first:split("|") end
	if type(second) == "string" then second = second:split("|") end

	local findings = {}
	for _, skill in ipairs(first) do
		for _, compare_skill in ipairs(second) do
			if skill == compare_skill and not table.contains(findings, skill) then table.insert(findings, skill) end
		end
	end
	return findings
end

function sgs.findUnionSkills(first, second)
	if type(first) == "string" then first = first:split("|") end
	if type(second) == "string" then second = second:split("|") end

	local findings = table.copyFrom(first)
	for _, skill in ipairs(second) do
		if not table.contains(findings, skill) then table.insert(findings, skill) end
	end

	return findings
end

sgs.ai_card_intention.general = function(from, to, level)
	if sgs.isRolePredictable() then return end
	if not to then global_room:writeToConsole(debug.traceback()) return end
	if from:isLord() or level == 0 then return end
	-- 将level固定为 10或者-10，目的是由原来的忠反值的变化 更改为 统计AI跳身份的行为次数，因为感觉具体的level值不太好把握，容易出现忠反值不合理飙涨的情况
	level = level > 0 and 10 or -10

	sgs.outputRoleValues(from, level)

	local loyalist_value = sgs.role_evaluation[from:objectName()]["loyalist"]
	local renegade_value = sgs.role_evaluation[from:objectName()]["renegade"]
	local renegadeLevel = 1
	if sgs.isLordInDanger() then
		renegadeLevel = 2
	end
	if sgs.evaluatePlayerRole(to) == "loyalist" then

		if sgs.current_mode_players["rebel"] == 0 and to:isLord() and level > 0  then
			--残局对主不利者，视为绝对的内。主公可以下杀手打死
			sgs.role_evaluation[from:objectName()]["renegade"] = sgs.role_evaluation[from:objectName()]["renegade"] + 50
		end
		if not isLord(to) and (sgs.UnknownRebel or (sgs.role_evaluation[to:objectName()]["renegade"] > 0 or sgs.current_mode_players["rebel"] == 0) and not sgs.explicit_renegade) then
		else
			sgs.role_evaluation[from:objectName()]["loyalist"] = sgs.role_evaluation[from:objectName()]["loyalist"] - level
		end

		if sgs.current_mode_players["rebel"] > 0 and sgs.current_mode_players["renegade"] == 0 and
			sgs.current_mode_players["loyalist"] > 0 and level > 0 and sgs.UnknownRebel then
				--反装忠
		elseif sgs.current_mode_players["rebel"] == 0 and sgs.current_mode_players["renegade"] > 0 and not to:isLord()
				and sgs.current_mode_players["loyalist"] > 0 and level > 0 and sgs.explicit_renegade == false then
				-- 进入主忠内, 但是没人跳过内，这个时候忠臣之间的相互攻击，不更新内奸值
		elseif (sgs.ai_role[from:objectName()] == "loyalist" and level > 0) or (sgs.ai_role[from:objectName()] == "rebel" and level < 0) then
			sgs.role_evaluation[from:objectName()]["renegade"] = sgs.role_evaluation[from:objectName()]["renegade"] + math.abs(level)*renegadeLevel
		elseif sgs.ai_role[from:objectName()] ~= "rebel" and sgs.ai_role[from:objectName()] ~= "neutral" and level > 0 and to:isLord() then
			sgs.role_evaluation[from:objectName()]["renegade"] = sgs.role_evaluation[from:objectName()]["renegade"] + math.abs(level)*renegadeLevel
		end
	end

	if sgs.evaluatePlayerRole(to) == "rebel" then
		sgs.role_evaluation[from:objectName()]["loyalist"] = sgs.role_evaluation[from:objectName()]["loyalist"] + level
		if (sgs.ai_role[from:objectName()] == "rebel" and level > 0) or (sgs.ai_role[from:objectName()] == "loyalist" and level < 0) then
			sgs.role_evaluation[from:objectName()]["renegade"] = sgs.role_evaluation[from:objectName()]["renegade"] + math.abs(level)*renegadeLevel
		end
	end

	--统计人头 预测盲狙者身份 或 用于推断有装反/装忠的行为
	local rebel_num = sgs.current_mode_players["rebel"]
	local room=to:getRoom()
	local loyalist_num = room:getAlivePlayers():length() - sgs.current_mode_players["rebel"]
	local loyalist_count=0
	local rebel_count=0
	local tmp_rebel
	for _,aplayer in sgs.qlist(room:getOtherPlayers(to)) do
		if aplayer:objectName() ~= from:objectName() then
			if aplayer:isLord() or sgs.ai_role[aplayer:objectName()] == "loyalist" or  sgs.ai_role[aplayer:objectName()] == "renegade" then
				loyalist_count=loyalist_count+1
			elseif sgs.ai_role[aplayer:objectName()] == "rebel" then
				rebel_count=rebel_count+1
				tmp_rebel=aplayer
			end
		end
	end


	--盲狙的仇恨 (两个未知的中立者) --主要是帮助被盲狙方做出判断
	if  sgs.evaluatePlayerRole(to) == "neutral" and sgs.evaluatePlayerRole(from) == "neutral" and level>0 then
		--一般而言，单纯的78位互相盲狙
		if  loyalist_count+1==loyalist_num  and rebel_count+1==rebel_num then
		elseif loyalist_count+1>=loyalist_num then
			sgs.role_evaluation[from:objectName()]["loyalist"] = sgs.role_evaluation[from:objectName()]["loyalist"] + level
		elseif  rebel_count+1>=rebel_num then
			sgs.role_evaluation[from:objectName()]["loyalist"] = sgs.role_evaluation[from:objectName()]["loyalist"] - level
		end
	end

	sgs.evaluateAlivePlayersRole()

	--此角色第一次跳身份时就导致反贼数量比额定数量还多，那么说明绝对有人在装
	--目前仅用于忠内残局时通过记录识别内奸
	if loyalist_value== 0 and renegade_value == 0  then
		if sgs.evaluatePlayerRole(from) == "loyalist" and loyalist_count+1>=loyalist_num then
			sgs.fake_loyalist = true
			for _,aplayer in sgs.qlist(room:getAlivePlayers()) do
				if sgs.evaluatePlayerRole(aplayer) == "loyalist" and not aplayer:isLord() then
					sgs.fake_loyalist_players[aplayer:objectName()] = true
				end
			end
		elseif sgs.evaluatePlayerRole(from) == "rebel" and rebel_count+1>=rebel_num then
			sgs.fake_rebel = true
			for _,aplayer in sgs.qlist(room:getAlivePlayers()) do
				if sgs.evaluatePlayerRole(aplayer) == "rebel"  then
					sgs.fake_rebel_players[aplayer:objectName()] = true
				end
			end
		end
	end

	--[[
	if global_room:getTag("humanCount") and global_room:getTag("humanCount"):toInt() ==1 then
		local diffarr = {
			loyalist_value  = sgs.role_evaluation[from:objectName()]["loyalist"] - loyalist_value ,
			renegade_value  = sgs.role_evaluation[from:objectName()]["renegade"] - renegade_value
		}

		local value_changed = false

		for msgtype,diffvalue in pairs(diffarr) do
			if diffvalue ~= 0 then
				value_changed = true
				local log= sgs.LogMessage()
				log.type = "#" .. msgtype .. (diffvalue > 0 and "_inc" or "_dec")
				log.from = from
				log.arg= math.abs(math.ceil(diffvalue))
				global_room:sendLog(log)
			end
		end

		if value_changed then
			local log= sgs.LogMessage()
			log.type = sgs.role_evaluation[from:objectName()]["loyalist"] >= 0 and  "#show_intention_loyalist" or "#show_intention_rebel"
			log.from = from
			log.arg  = string.format("%d", math.abs(math.ceil(sgs.role_evaluation[from:objectName()]["loyalist"])))
			log.arg2 = string.format("%d", sgs.role_evaluation[from:objectName()]["renegade"])
			global_room:sendLog(log)
		end
	end
	]]
	sgs.outputRoleValues(from, level)
end

function sgs.outputRoleValues(player, level)
	global_room:writeToConsole(player:getGeneralName() .. " " .. level .. " " .. sgs.evaluatePlayerRole(player)
								.. " L" .. math.ceil(sgs.role_evaluation[player:objectName()]["loyalist"])
								.. " R" .. math.ceil(sgs.role_evaluation[player:objectName()]["renegade"])
								.. " " .. sgs.gameProcess(player:getRoom())
								.. " " .. sgs.current_mode_players["loyalist"] .. sgs.current_mode_players["rebel"] .. sgs.current_mode_players["renegade"])
end

function sgs.updateIntention(from, to, intention, card)
	if not to then global_room:writeToConsole(debug.traceback()) return end
	if from:objectName() == to:objectName() then return end
	sgs.ai_card_intention.general(from, to, intention)
end

function sgs.updateIntentions(from, tos, intention, card)
	for _, to in ipairs(tos) do
		if not (card and (card:isNDTrick() or card:isKindOf("BasicCard")) and #tos > 1 and to:hasSkill("xunshi")) then
			sgs.updateIntention(from, to, intention, card)
		end
	end
end

function sgs.isLordHealthy()
	local lord = global_room:getLord()
	local lord_hp
	if not lord then return true end
	if lord:hasSkill("benghuai") and lord:getHp() > 4 then lord_hp = 4
	elseif lord:hasSkill("huanmeng") then
		lord_hp = (lord:getHandcardNum() + 1) /2
	elseif lord:hasSkill("banling") then
		lord_hp = lord:getMark("lingtili")+ lord:getMark("rentili")-1
	elseif lord:hasSkill("bumie") then
		lord_hp =  lord:getMaxHp()
	else lord_hp = lord:getHp() end
	if lord:hasSkill("juxian") and lord:faceUp() then
		lord_hp = lord_hp+1
	end
	if lord:hasSkill("hualong")  and lord:getMark("hulong") == 0 then
		lord_hp = lord_hp+lord:getMaxHp()
	end
	if lord:hasSkill("yizhi")  and lord:getMark("yizhi") == 0 then
		lord_hp = lord_hp+1
	end
	return lord_hp > 3 or (lord_hp > 2 and sgs.getDefense(lord) > 3)
end

function sgs.isLordInDanger()
	local lord = global_room:getLord()
	local lord_hp
	if not lord then return false end

	if lord:hasSkill("benghuai") and lord:getHp() > 4 then lord_hp = 4
	elseif lord:hasSkill("huanmeng") then
		lord_hp = (lord:getHandcardNum() + 1 )/2
	elseif lord:hasSkill("banling") then
		lord_hp = lord:getMark("lingtili")+ lord:getMark("rentili")-1
	elseif lord:hasSkill("bumie") then
		lord_hp =  lord:getMaxHp()
	else lord_hp = lord:getHp() end
	if lord:hasSkill("jinguo") then
		lord_hp = math.max(lord_hp, lord_hp - lord:getMark("@kinki")/2)
	end
	if lord:hasSkill("juxian") and lord:faceUp() then
		lord_hp = lord_hp+1
	end
	if lord:hasSkill("hualong")  and lord:getMark("hulong") == 0 then
		lord_hp = lord_hp+lord:getMaxHp()
	end
	if lord:hasSkill("yizhi")  and lord:getMark("yizhi") == 0 then
		lord_hp = lord_hp+1
	end
	return lord_hp < 3
end

function sgs.gameProcess(room, arg)  --尼玛 不看具体技能和牌的数量么 卧槽  只有一些枚举。。。


	local rebel_num = sgs.current_mode_players["rebel"]
	local loyal_num = sgs.current_mode_players["loyalist"]
	if rebel_num == 0 and loyal_num> 0 then return "loyalist"
	elseif loyal_num == 0 and rebel_num > 1 then return "rebel" end
	local loyal_value, rebel_value = 0, 0, 0
	local health = sgs.isLordHealthy()
	local danger = sgs.isLordInDanger()
	local lord = room:getLord()
	local currentplayer = room:getCurrent()
	local averageCardnum = 0
	for _, aplayer in sgs.qlist(room:getAlivePlayers()) do
		averageCardnum = averageCardnum + aplayer:getCards("hes"):length()
	end
	averageCardnum = averageCardnum /(room:getAlivePlayers():length())
	for _, aplayer in sgs.qlist(room:getAlivePlayers()) do
		local role=aplayer:getRole()--读取的是真实身份？不是预测！！！！！！？？？？？
		if role == "rebel" then
			local rebel_hp
			if aplayer:hasSkill("benghuai") and aplayer:getHp() > 4 then rebel_hp = 4
			else rebel_hp = aplayer:getHp() end
			if  aplayer:hasSkill("huanmeng") then rebel_hp = (aplayer:getHandcardNum() +1) / 2 end
			if aplayer:getMaxHp() == 3 then rebel_value = rebel_value + 0.5 end
			--只评估防御能力？不看进攻能力！！！！！？？？？？各种爆将教做人 和奶妈教做人
			rebel_value = rebel_value + rebel_hp + math.max(sgs.getDefense(aplayer, true) - rebel_hp * 2, 0) * 0.7
			if aplayer:getDefensiveHorse() then
				rebel_value = rebel_value + 0.3
			end
			if aplayer:getCards("e"):length() >= averageCardnum then
				rebel_value = rebel_value + 0.3
			end
			if lord and aplayer:inMyAttackRange(lord) then
				rebel_value = rebel_value + 0.4
			end
			if aplayer:getMark("@pingyi") > 0 then rebel_value = rebel_value - 3  end
			if aplayer:getMark("@duanchang") > 0 and aplayer:getMaxHp() <= 3 then rebel_value = rebel_value - 1 end
			if aplayer:hasSkill("xisan") then  rebel_value = rebel_value + 2 end
			if aplayer:hasSkills("luanying+jingjie") or aplayer:hasSkills("mengxian+jingjie") then rebel_value = rebel_value + 2 end
			if aplayer:hasSkill("ganying") and lord:hasSkill("fengsu") then rebel_value = rebel_value + 2 end
			if aplayer:hasSkill("baochun") then rebel_value = rebel_value + aplayer:getMaxHp() end
			if rebel_num > 1 and aplayer:hasSkill("hpymsiyu+juhe") then rebel_value = rebel_value + 1 end
			if aplayer:hasSkill("wuchang")  then rebel_value = rebel_value + 10 end
			if  aplayer:hasSkill("shizhu") then rebel_value = rebel_value + 1 end
			if aplayer:hasSkill("jinxi") and aplayer:getMark("@jinxi") > 0 then  rebel_value = rebel_value + 2 end
			if aplayer:hasSkills("bolan+hezhou") then rebel_value = rebel_value + 10 end
		elseif role == "loyalist" or role == "lord" then
			local loyal_hp
			if aplayer:hasSkill("benghuai") and aplayer:getHp() > 4 then loyal_hp = 4
			else loyal_hp = aplayer:getHp() end
			if  aplayer:hasSkill("huanmeng") then loyal_hp = (aplayer:getHandcardNum()+1)  / 2 end
			if aplayer:getMaxHp() == 3 then loyal_value = loyal_value + 0.5 end
			loyal_value = loyal_value + (loyal_hp + math.max(sgs.getDefense(aplayer, true) - loyal_hp * 2, 0) * 0.7)
			if aplayer:getArmor() or (not aplayer:getArmor() and aplayer:hasSkills("bazhen|yizhong")) then
				loyal_value = loyal_value + 0.5
			end
			if aplayer:getDefensiveHorse() then
				loyal_value = loyal_value + 0.5
			end
			if aplayer:getCards("hes"):length() >= averageCardnum  then
				loyal_value = loyal_value + 0.3
			end
			if aplayer:getMark("@duanchang")==1 and aplayer:getMaxHp() <=3 then loyal_value = loyal_value - 1 end
			if aplayer:getMark("@pingyi") > 0 then loyal_value = loyal_value - 3  end
			if aplayer:hasSkills("bolan+hezhou") then loyal_value = loyal_value + 8 end
		end
	end
	local diff = loyal_value - rebel_value + (loyal_num + 1 - rebel_num) * 2

	if arg and arg == 1 then return diff end

	if diff >= 2 then
		if health then return "loyalist"
		else return "dilemma" end
	elseif diff >= 1 then
		if health then return "loyalish"
		elseif danger then return "dilemma"
		else return "rebelish" end
	elseif diff <= -2 then return "rebel"
	elseif diff <= -1 then
		if health then return "rebelish"
		else return "rebel" end
	elseif not health then return "rebelish"
	else return "neutral" end
end

function SmartAI:objectiveLevel(player)
	if player:objectName() == self.player:objectName() then return -2 end
	local players = self.room:getOtherPlayers(self.player)
	players = sgs.QList2Table(players)

	if #players == 1 then return 5 end

	if sgs.isRolePredictable(true) then
		if self.lua_ai:isFriend(player) then return -2
		elseif self.lua_ai:isEnemy(player) then return 5
		elseif self.lua_ai:relationTo(player) == sgs.AI_Neutrality then
			if self.lua_ai:getEnemies():isEmpty() then return 4 else return 0 end
		else return 0 end
	end

	local blindAttack = false
	if not sgs.GetConfig("AIProhibitBlindAttack", false) then
		blindAttack = true
	end

	local rebel_num = sgs.current_mode_players["rebel"]
	local loyal_num = sgs.current_mode_players["loyalist"]
	local renegade_num = sgs.current_mode_players["renegade"]
	local target_role = sgs.evaluatePlayerRole(player)
	local self_role = sgs.evaluatePlayerRole(self.player)

	if self.role == "renegade" then
		if player:isLord() and player:getHp() <= 0 and player:hasFlag("Global_Dying") then return -2 end
		--if target_role == "rebel" and player:getHp() <= 1 and not hasBuquEffect(player) and not player:hasSkills("kongcheng|tianming") and player:isKongcheng()
		--  and getCardsNum("Peach", player, self.player) == 0 and getCardsNum("Analepic", player, self.player) == 0 then return 5 end


		if rebel_num == 0 or loyal_num == 0 then
			if rebel_num > 0 then
				if rebel_num > 1 then
					if player:isLord() then
						return -2
					elseif target_role == "rebel" then
						return 5
					else
						return 0
					end
				elseif renegade_num > 1 then
					if player:isLord() then
						return 0
					elseif target_role == "renegade" then
						return 3
					else
						return 5
					end
				else
					local process = sgs.gameProcess(self.room)
					if process == "loyalist" then
						if player:isLord() then
							if not sgs.isLordHealthy() then return -1
							else return 1 end
						elseif target_role == "rebel" then
							return 0
						else
							return 5
						end
					elseif process:match("rebel") then
						if target_role == "rebel" then
							return 5
						else
							return -1
						end
					else
						if player:isLord() then
							return 0
						else
							return 5
						end
					end
				end
			elseif loyal_num > 0 then
				if sgs.explicit_renegade and renegade_num == 1 and sgs.role_evaluation[self.player:objectName()]["renegade"] == 0
					and sgs.evaluatePlayerRole(player) == "loyalist" then
					if target_role == "renegade" then return 5 else return -1 end
				end
				if player:isLord() then
					if not sgs.explicit_renegade and sgs.role_evaluation[self.player:objectName()]["renegade"] == 0 then return 0 end
					if not sgs.isLordHealthy() then return 0
					else return 1 end
				elseif target_role == "renegade" and renegade_num > 1 then
					return 3
				else
					return 5
				end
			else
				if player:isLord() then
					if sgs.isLordInDanger then return 0
					elseif not sgs.isLordHealthy() then return 3
					else return 5 end
				elseif sgs.isLordHealthy() then return 3
				else
					return 5
				end
			end
		end
		local process = sgs.gameProcess(self.room)
		local diff = sgs.gameProcess(self.room, 1)
		local diff_threshold = diff
		if self:isWeak() then diff_threshold = diff_threshold +1.5 end
		diff_threshold = diff_threshold + 1 + loyal_num - rebel_num
		if process == "neutral" or (sgs.turncount <= 1 and sgs.isLordHealthy() and self_role == "neutral") then
			if sgs.turncount <= 1 and sgs.isLordHealthy()  then
				if self:getOverflow() <= 0 then return 0 end
				--尼玛 这里是第一轮内比反还反的原因么。。。第一轮两个忠被打残 只要主健康 妈的天塌下来都不管。。。
				--第一轮也要考虑diff
				local rebelish = (sgs.current_mode_players["loyalist"] + 1 < sgs.current_mode_players["rebel"])
				if player:isLord() then return rebelish and -1 or 0 end
				if target_role == "loyalist" then return rebelish and 0 or 3.5
				elseif target_role == "rebel" then return rebelish and 3.5 or 0
				else return 0
				end
			end
			if player:isLord() then return -1 end
			--尼玛 这里的逻辑我认为问题很大
			--应该是对象的有些技能强度过高 导致局面倾斜 所以内奸要先去攻击 --我觉得判断 很大程度应该 process就应该完成。
			--而不是局势平衡的前提下去攻击这些对象
			local renegade_attack_skill = string.format("buqu|nosbuqu|%s|%s|%s|%s", sgs.priority_skill, sgs.save_skill, sgs.recover_skill, sgs.drawpeach_skill)
			for i = 1, #players, 1 do
				if not players[i]:isLord() and self:hasSkills(renegade_attack_skill, players[i]) then return 5 end
				if not players[i]:isLord() and math.abs(sgs.ai_chaofeng[players[i]:getGeneralName()] or 0) > 3 then return 5 end
			end
			if self.player:getPhase() == sgs.Player_NotActive then
				return 0
			else
				return self:getOverflow() > 0 and 3 or 0
			end
			--检测溢出只是单纯考虑存牌吧。。。不去白白浪费牌吧 但是那些不涉及用牌的技能呢？？？
			--主要目的是局势均衡时把身份跳出来？？
			--return 3  --尼玛对于忠和反都是3这个太没节操了
			-- overflow return 3 你妹 然后一贯石斧砸下去就麻痹缺牌过
		elseif process:match("rebel") then
			return target_role == "rebel" and 5 or target_role == "neutral" and 0 or -1
		elseif process:match("dilemma") then
			if target_role == "rebel" then return 5
			elseif target_role == "loyalist" or target_role == "renegade" then return 0
			elseif player:isLord() then return -2
			else return 5 end
		elseif process == "loyalish" then
			if player:isLord() or target_role == "renegade" then return 0 end
			local rebelish = (sgs.current_mode_players["loyalist"] + 1 < sgs.current_mode_players["rebel"])
			if target_role == "loyalist" then return rebelish and 0 or 3.5
			elseif target_role == "rebel" then return rebelish and 3.5 or 0
			else return 0
			end
		else
			if player:isLord() or target_role == "renegade" then return 0 end

			if diff >= diff_threshold then
				return target_role == "rebel" and -2 or 5
			else
				return target_role == "rebel" and 0 or 5
			end
		end
	end

	if self.player:isLord() or self.role == "loyalist" then
		if player:isLord() then return -2 end

		if loyal_num == 0 and renegade_num == 0 then return 5 end

		if self.role == "loyalist" and loyal_num == 1 and renegade_num == 0 then return 5 end

		if sgs.ai_role[player:objectName()] == "neutral" then
			if rebel_num > 0 then
				local current_friend_num = 0
				local current_enemy_num = 0
				local current_renegade_num = 0
				local current_neutral_num = 0
				local rebelish = sgs.gameProcess(self.room):match("rebel")
				for _, aplayer in sgs.qlist(self.room:getAlivePlayers()) do
					if sgs.ai_role[aplayer:objectName()] == "loyalist" or aplayer:objectName() == self.player:objectName() then
						current_friend_num = current_friend_num + 1
					elseif sgs.ai_role[aplayer:objectName()] == "renegade" then
						current_renegade_num = current_renegade_num + 1
					elseif sgs.ai_role[aplayer:objectName()] == "rebel" then
						current_enemy_num = current_enemy_num + 1
					elseif sgs.ai_role[aplayer:objectName()] == "neutral" then
						current_neutral_num = current_neutral_num + 1
					end
				end
				if current_friend_num >= loyal_num + (rebelish and renegade_num or 0) + 1 then
					return 5
				elseif current_enemy_num + (rebelish and 0 or current_renegade_num) >= rebel_num + (rebelish and 0 or renegade_num) then
					return -1
				end
				if blindAttack and not rebelish
				and current_friend_num >= loyal_num and current_enemy_num < rebel_num - 1 then
					return 4
				end
			elseif sgs.explicit_renegade and renegade_num == 1 then return -1 end
		end

		if rebel_num == 0 then
			if #players == 2 and self.role == "loyalist" then return 5 end
			---对于主忠内残局跳得内 可以下杀手
			if sgs.role_evaluation[player:objectName()]["renegade"] >= 50 then
				return 4
			end

			if self.player:isLord() and not self.player:hasFlag("stack_overflow_jijiang") and player:getHp() <= 2 and self:hasHeavySlashDamage(self.player, nil, player) then
				return 0
			end

			if not sgs.explicit_renegade then
				if sgs.ai_role[player:objectName()] == "rebel" then return player:getHp() > 1 and 5 or 1 end
				--给排一个序列
				local sort_func = {
						loyalist = function(a, b)
							return sgs.role_evaluation[a:objectName()]["loyalist"] > sgs.role_evaluation[b:objectName()]["loyalist"]
						end
				}
				--忠内3人以上存在时，对于劳苦功高的忠优先保证其存活
				--保证明忠健康 同时也是对内多一份钳制
				--最大限度避免内翻盘 特别是现在这个不愿意对内下手的尿性
				if #players > 2 then
					table.sort(players, sort_func["loyalist"])
					if players[1]:objectName() == player:objectName()  and sgs.role_evaluation[players[1]:objectName()]["loyalist"] > 0 then
						return -1
					end
				end

				self:sort(players, "hp")
				local maxhp = players[#players]:isLord() and players[#players - 1]:getHp() or players[#players]:getHp()
				if maxhp > 2 then return player:getHp() == maxhp and 5 or 0 end
				if maxhp == 2 then return self.player:isLord() and 0 or (player:getHp() == maxhp and 5 or 1) end
				return self.player:isLord() and 0 or 5
			else
				-- 一开始就跳反的必须是内
				if sgs.fake_rebel and sgs.fake_rebel_players[player:objectName()]  then  return 5 end
				if self.player:isLord() then
					if sgs.ai_role[player:objectName()] == "loyalist" then return -2
					else
						return player:getHp() > 1 and 4 or 0
					end
				else
					if self.role == "loyalist" and sgs.ai_role[self.player:objectName()] == "renegade" then
						if #players == 2 then return 5 end
						local renegade_value, renegade_player = 0
						for _, p in ipairs(players) do
							if sgs.role_evaluation[p:objectName()]["renegade"] > renegade_value then
								renegade_value = sgs.role_evaluation[p:objectName()]["renegade"]
								renegade_player = p
							end
						end
						if renegade_player then return renegade_player:objectName() == player:objectName() and 5 or -2
						else return 4 end
					end
					return sgs.ai_role[player:objectName()] == "loyalist" and -2 or 4
				end
			end
		end
		if loyal_num == 0 then
			if rebel_num > 2 then
				if target_role == "renegade" then return -1 end
			elseif rebel_num > 1 then
				if target_role == "renegade" then return -1 end
			elseif target_role == "renegade" then return sgs.isLordInDanger() and -1 or 4 end
		end
		if renegade_num == 0 then
			if sgs.ai_role[player:objectName()] == "loyalist" then return -2 end

			if rebel_num > 0 and sgs.turncount > 1 then
				local hasRebel
				for _, p in ipairs(players) do
					if sgs.ai_role[p:objectName()] == "rebel" then hasRebel = true sgs.UnknownRebel = false break end
				end
				if not hasRebel then
					sgs.UnknownRebel = true
					self:sort(players, "hp")
					local maxhp = players[#players]:isLord() and players[#players - 1]:getHp() or players[#players]:getHp()
					if maxhp > 2 then return player:getHp() == maxhp and 5 or 0 end
					if maxhp == 2 then return self.player:isLord() and 0 or (player:getHp() == maxhp and 5 or 1) end
					return self.player:isLord() and 0 or 5
				end
			end
		end

		if sgs.ai_role[player:objectName()] == "rebel" then return 5
		elseif sgs.ai_role[player:objectName()] == "loyalist" then return -2 end
		if target_role == "renegade" then
			if sgs.gameProcess(self.room):match("rebel") then return -2
			else return sgs.isLordInDanger() and 0 or 4 end
		end
		return 0
	elseif self.role == "rebel" then

		if loyal_num == 0 and renegade_num == 0 then return player:isLord() and 5 or -2 end

		if sgs.ai_role[player:objectName()] == "neutral" then
			local current_friend_num = 0
			local current_enemy_num = 0
			local current_renegade_num = 0
			for _, aplayer in sgs.qlist(self.room:getAlivePlayers()) do
				if sgs.ai_role[aplayer:objectName()] == "rebel" or aplayer:objectName() == self.player:objectName() then
					current_friend_num = current_friend_num + 1
				elseif sgs.ai_role[aplayer:objectName()] == "renegade" then current_renegade_num = current_renegade_num + 1
				elseif sgs.ai_role[aplayer:objectName()] == "loyalist" then current_enemy_num = current_enemy_num + 1 end
			end
			local disadvantage = sgs.gameProcess(self.room):match("loyal")
			if current_friend_num + (disadvantage and current_renegade_num or 0) >= rebel_num + (disadvantage and renegade_num or 0) then
				return 5
			elseif current_enemy_num + (disadvantage and 0 or current_renegade_num) >= loyal_num + (disadvantage and 0 or renegade_num) + 1 then
				return -2
			else
				return 0
			end
		end

		if player:isLord() then return 5
		elseif sgs.ai_role[player:objectName()] == "loyalist" then return 5 end
		local gameProcess = sgs.gameProcess(self.room)
		if target_role == "rebel" then return (rebel_num > 1 or renegade_num > 0 and gameProcess:match("loyal")) and -2 or 5 end
		if target_role == "renegade" then --对内的态度需要多样化 内中立时杀内简直逗比
			if gameProcess:match("loyal") then
				return -1
			elseif gameProcess:match("rebel") then
				return 4
			else
				return 0
			end
		end
		return 0
	end
end

function SmartAI:isFriend(other, another)
	if not other then self.room:writeToConsole(debug.traceback()) return end
	if another then
		local of, af = self:isFriend(other), self:isFriend(another)
		return of ~= nil and af ~= nil and of == af
	end
	if sgs.isRolePredictable(true) and self.lua_ai:relationTo(other) ~= sgs.AI_Neutrality then return self.lua_ai:isFriend(other) end
	if self.player:objectName() == other:objectName() then return true end
	local obj_level = self:objectiveLevel(other)
	if obj_level < 0 then return true
	elseif obj_level == 0 then return nil end
	return false
end

function SmartAI:isEnemy(other, another)
	if not other then self.room:writeToConsole(debug.traceback()) return end
	if another then
		local of, af = self:isFriend(other), self:isFriend(another)
		return of ~= nil and af ~= nil and of ~= af
	end
	if sgs.isRolePredictable(true) and self.lua_ai:relationTo(other) ~= sgs.AI_Neutrality then return self.lua_ai:isEnemy(other) end
	if self.player:objectName() == other:objectName() then return false end
	local obj_level = self:objectiveLevel(other)
	if obj_level > 0 then return true
	elseif obj_level == 0 then return nil end
	return false
end

function SmartAI:getFriendsNoself(player)
	if self:isFriend(self.player, player) then
		return self.friends_noself
	elseif self:isEnemy(self.player, player) then
		friends = sgs.QList2Table(self.lua_ai:getEnemies())
		for i = #friends, 1, -1 do
			if friends[i]:objectName() == player:objectName() or not friends[i]:isAlive() then
				table.remove(friends, i)
			end
		end
		return friends
	else
		return {}
	end
end

function SmartAI:getFriends(player)
	player = player or self.player
	if self:isFriend(self.player, player) then
		return self.friends
	elseif self:isEnemy(self.player, player) then
		return self.enemies
	else
		return {player}
	end
end

function SmartAI:getEnemies(player)
	if self:isFriend(self.player, player) then
		return self.enemies
	elseif self:isEnemy(self.player, player) then
		return self.friends
	else
		return {}
	end
end

function SmartAI:sortEnemies(players)
	local comp_func = function(a,b)
		local alevel = self:objectiveLevel(a)
		local blevel = self:objectiveLevel(b)

		if alevel~= blevel then return alevel > blevel end
		return sgs.getDefenseSlash(a) < sgs.getDefenseSlash(b)
	end
	table.sort(players,comp_func)
end

function SmartAI:updateAlivePlayerRoles()
	for _, arole in ipairs({"lord", "loyalist", "rebel", "renegade"}) do
		sgs.current_mode_players[arole] = 0
	end
	for _, aplayer in sgs.qlist(self.room:getAllPlayers()) do
		sgs.current_mode_players[aplayer:getRole()] = sgs.current_mode_players[aplayer:getRole()] + 1
	end
end

function SmartAI:updatePlayers(clear_flags)
	if clear_flags ~= false then clear_flags = true end
	if self.role ~= self.player:getRole() then
		if not ((self.role=='lord' and self.player:getRole()=='loyalist') or (self.role=='loyalist' and self.player:getRole()=='lord')) then
			sgs.role_evaluation[self.player:objectName()]["loyalist"]= 0
			sgs.role_evaluation[self.player:objectName()]["rebel"]= 0
			sgs.role_evaluation[self.player:objectName()]["renegade"]= 0
		end
		self.role = self.player:getRole()
	end
	if clear_flags then
		for _, aflag in ipairs(sgs.ai_global_flags) do
			sgs[aflag] = nil
		end
	end

	sgs.ai_updateDefense = true
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		sgs.getDefense(p, true)
	end
	sgs.ai_updateDefense = false

	if sgs.isRolePredictable(true) then
		self.friends = {}
		self.friends_noself = {}
		local friends = sgs.QList2Table(self.lua_ai:getFriends())
		for i = 1, #friends, 1 do
			if friends[i]:isAlive() and friends[i]:objectName() ~= self.player:objectName() then
				table.insert(self.friends, friends[i])
				table.insert(self.friends_noself, friends[i])
			end
		end
		table.insert(self.friends, self.player)

		local enemies = sgs.QList2Table(self.lua_ai:getEnemies())
		for i = 1, #enemies, 1 do
			if enemies[i]:isDead() or enemies[i]:objectName() == self.player:objectName() then table.remove(enemies, i) end
		end
		self.enemies = enemies

		self.retain = 2
		self.harsh_retain = false
		if #self.enemies == 0 then
			local neutrality = {}
			for _, aplayer in sgs.qlist(self.room:getOtherPlayers(self.player)) do
				if self.lua_ai:relationTo(aplayer) == sgs.AI_Neutrality and not aplayer:isDead() then table.insert(neutrality, aplayer) end
			end
			local function compare_func(a,b)
				return self:objectiveLevel(a) > self:objectiveLevel(b)
			end
			table.sort(neutrality, compare_func)
			table.insert(self.enemies, neutrality[1])
		end
		return
	end

	self:updateAlivePlayerRoles()
	sgs.evaluateAlivePlayersRole()--先更新role列表 再更新friend enemy列表

	self.enemies = {}
	self.friends = {}
	self.friends_noself = {}

	self.retain = 2
	self.harsh_retain = true

	for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		local level = self:objectiveLevel(player)
		if level < 0 then
			table.insert(self.friends_noself, player)
			table.insert(self.friends, player)
		elseif level > 0 then
			table.insert(self.enemies, player)
		end
	end
	table.insert(self.friends, self.player)

	if sgs.isRolePredictable() then return end
	self:updateAlivePlayerRoles()
	--sgs.evaluateAlivePlayersRole()--删除？
end

function sgs.evaluateAlivePlayersRole()
	local players = sgs.QList2Table(global_room:getAlivePlayers())
	local cmp = function(a, b)
		local ar_value, br_value = sgs.role_evaluation[a:objectName()]["renegade"], sgs.role_evaluation[b:objectName()]["renegade"]
		local al_value, bl_value = sgs.role_evaluation[a:objectName()]["loyalist"], sgs.role_evaluation[b:objectName()]["loyalist"]
		return (ar_value > br_value) or (ar_value == br_value and al_value > bl_value)
	end
	table.sort(players, cmp)

	sgs.explicit_renegade = false
	for i =1, #players, 1 do
		local p = players[i]
		local renegade_val = sgs.current_mode_players["rebel"] == 0 and 10 or 15
		if i <= sgs.current_mode_players["renegade"] and sgs.role_evaluation[p:objectName()]["renegade"] >= renegade_val then
			sgs.ai_role[p:objectName()] = "renegade"
			sgs.explicit_renegade = true
		else
			if (sgs.role_evaluation[p:objectName()]["loyalist"] > 0 and sgs.current_mode_players["loyalist"] > 0) or p:isLord() then
				sgs.ai_role[p:objectName()] = "loyalist"
			elseif sgs.role_evaluation[p:objectName()]["loyalist"] < 0 and sgs.current_mode_players["rebel"] > 0 then
				sgs.ai_role[p:objectName()] = "rebel"
			else
				sgs.ai_role[p:objectName()] = "neutral"
				-- if sgs.role_evaluation[p:objectName()]["loyalist"] > 0  then sgs.ai_role[p:objectName()] = "loyalist" end
				-- if sgs.role_evaluation[p:objectName()]["loyalist"] < 0  then sgs.ai_role[p:objectName()] = "rebel" end
				-- if sgs.role_evaluation[p:objectName()]["loyalist"] == 0 then sgs.ai_role[p:objectName()] = "neutral" end
			end
		end
		if sgs.current_mode_players["rebel"] == 0 and sgs.current_mode_players["loyalist"] == 0 and not p:isLord() then
			sgs.ai_role[p:objectName()] = "renegade"
			sgs.explicit_renegade = true
		end
		--woyu target isRolePredictable
		if p:getMark("AI_RolePredicted")>0 then -- if p:hasShownRole() then --not server player
			local role = p:getRole()
			if p:isLord() then role = "loyalist" end
			sgs.ai_role[p:objectName()] = role
		end
	end
	sgs.modifiedRoleEvaluation()
end

---查找room内指定objectName的player
function findPlayerByObjectName(room, name, include_death, except)
	if not room then
		return
	end
	local players = nil
	if include_death then
		players = room:getPlayers()
	else
		players = room:getAllPlayers()
	end
	if except then
		players:removeOne(except)
	end
	for _,p in sgs.qlist(players) do
		if p:objectName() == name then
			return p
		end
	end
end

--借刀动机居然是0 坑爹呢
function getTrickIntention(trick_class, target)
	local intention = sgs.ai_card_intention[trick_class]
	if type(intention) == "number" then
		return intention
	elseif type(intention == "function") then
		if trick_class == "IronChain" then
			if target and target:isChained() then return -60 else return 60 end
		elseif trick_class == "Drowning" then
			if target and target:getArmor() and target:hasSkills("yizhong|bazhen") then return 0 else return 60 end
		end
	end
	if trick_class == "Collateral" then
		return 0
	end
	if trick_class == "SupplyShortage" or trick_class == "Indulgence" then
		return 70
	end
	if sgs.dynamic_value.damage_card[trick_class] then
		return 70
	end
	if sgs.dynamic_value.benefit[trick_class] then
		return -40
	end
	if target then
		if trick_class == "Snatch" or trick_class == "Dismantlement" then
			local judgelist = target:getCards("j")
			if not judgelist or judgelist:isEmpty() then
				if not target:hasArmorEffect("SilverLion") or not target:isWounded() then
					return 80
				end
			end
		end
	end
	return 0
end

sgs.ai_choicemade_filter.Nullification.general = function(self, player, args)
	local trick_class = args[1]
	local target_objectName = args[2]
	--其实可以直接引入args里的positive信息？
	if trick_class == "Nullification" then
		if not sgs.nullification_source or not sgs.nullification_intention or type(sgs.nullification_intention) ~= "number" then
			self.room:writeToConsole(debug.traceback())
			return
		end
		if not player:hasFlag("nullifiationNul") then
			sgs.nullification_level = sgs.nullification_level + 1
		end

		if sgs.nullification_level % 2 == 0 then
			sgs.updateIntention(player, sgs.nullification_source, sgs.nullification_intention)
		elseif sgs.nullification_level % 2 == 1 then
			sgs.updateIntention(player, sgs.nullification_source, -sgs.nullification_intention)
		end
	else
		sgs.nullification_source = findPlayerByObjectName(global_room, target_objectName)
		sgs.nullification_level = 1
		if player:hasFlag("nullifiationNul") then
			sgs.nullification_level = 0
		end

		sgs.nullification_intention = getTrickIntention(trick_class, sgs.nullification_source)
		--开场无邪主公顺手跳明反
		if  trick_class == "Snatch" then
			local lord = getLord(player)
			if lord and self:playerGetRound(lord)==0 and lord:getPhase()==sgs.Player_Play then
				sgs.updateIntention(player, lord, sgs.nullification_intention)
			end
		end
		if player:objectName() ~= target_objectName then
			sgs.updateIntention(player, sgs.nullification_source, -sgs.nullification_intention)
		end
	end
end

sgs.ai_choicemade_filter.playerChosen.general = function(self, from, args)
	if from:objectName() == args[2] then return end
	local reason = string.gsub(args[1], "%-", "_")
	local to = findPlayerByObjectName(self.room, args[2])
	local callback
	if to then
		callback = sgs.ai_playerchosen_intention[reason]
	else
		callback = sgs.ai_no_playerchosen_intention[reason]
	end
	if callback and to then
		if type(callback) == "number"  then
			sgs.updateIntention(from, to, sgs.ai_playerchosen_intention[reason])
		elseif type(callback) == "function" then
			callback(self, from, to)
		end
	elseif  callback  then
		if type(callback) == "function" then
			callback(self, from)
		end
	end
end

sgs.ai_choicemade_filter.viewCards.general = function(self, from, args)
	local to = findPlayerByObjectName(self.room, args[#args])
	if to and not to:isKongcheng() then
		local flag = string.format("%s_%s_%s", "visible", from:objectName(), to:objectName())
		for _, card in sgs.qlist(to:getHandcards()) do
			if not card:hasFlag("visible") then card:setFlags(flag) end
		end
	end
end

sgs.ai_choicemade_filter.Yiji.general = function(self, from, args)
	local from = findPlayerByObjectName(self.room, args[2])
	local to = findPlayerByObjectName(self.room, args[3])
	local reason = args[1]
	local cards = {}
	local card_ids = args[4]:split("+")
	for _, id in ipairs(card_ids) do
		local card = sgs.Sanguosha:getCard(tonumber(id))
		table.insert(cards, card)
	end
	if from and to then
		local callback = sgs.ai_Yiji_intention[reason]
		if callback then
			if type(callback) == "number" and not (self:needKongcheng(to, true) and #cards == 1)
				and not (to:hasSkill("manjuan") and to:getPhase() == sgs.Player_NotActive) then
				sgs.updateIntention(from, to, sgs.ai_Yiji_intention[reason])
			elseif type(callback) == "function" then
				callback(self, from, to, cards)
			end
		elseif not (self:needKongcheng(to, true) and #cards == 1) and not (to:hasSkill("manjuan") and to:getPhase() == sgs.Player_NotActive) then
			sgs.updateIntention(from, to, -10)
		end
	end
end

sgs.ai_choicemade_filter.Rende.general = function(self, from, args)
	local from = findPlayerByObjectName(self.room, args[2])
	local to_names = args[3]:split("+")
	local tos = {}
	for _, name in ipairs(to_names) do
		local to = findPlayerByObjectName(self.room, name)
		if to then
			table.insert(tos, to)
		end
	end
	local reason = args[1]

	--data传过来的信息无法区分哪个人获得了几张id
	-- local cards = {}
	-- local card_ids = promptlist[5]:split("+")
	-- for _, id in ipairs(card_ids) do
		-- local card = sgs.Sanguosha:getCard(tonumber(id))
		-- table.insert(cards, card)
	-- end

	if from and #tos >0 then
		local callback = sgs.ai_Rende_intention[reason]
		for _,to in pairs(tos) do
			if callback then
				if type(callback) == "number" and not (self:needKongcheng(to, true) ) then --and #cards == 1
					sgs.updateIntention(from, to, sgs.ai_Rende_intention[reason])
				end
				-- elseif type(callback) == "function" then
					-- callback(self, from, to, cards)
				-- end
			elseif not (self:needKongcheng(to, true) ) then  --and #cards == 1
				sgs.updateIntention(from, to, -10)
			end
		end
	end
end

--东方杀相关
--damage event 【破坏】【创史】
--start judge 【尸解】
--【凭依】更新仇恨 【创史】更新部分仇恨
--【命运】【绯想】改判
--借刀的仇恨
function SmartAI:filterEvent(event, player, data)
	if not sgs.recorder then
		sgs.recorder = self
	end

	if event == sgs.GameStart or  event == sgs.TurnStart
	or  event == sgs.EventPhaseStart  or  event == sgs.EventPhaseEnd  or event == sgs.EventPhaseProceeding
	or event == sgs.HpChanged or event == sgs.MaxHpChanged then
		player = data:toPlayer()
	elseif event == sgs.HpRecover or event == sgs.PreHpRecover then
		player = data:toRecover().to
	elseif event == sgs.StartJudge or event == sgs.AskForRetrial or event == sgs.FinishRetrial or event == sgs.FinishJudge  then
		player = data:toJudge().who
	elseif event == sgs.ConfirmDamage or event == sgs.Predamage  or event == sgs.DamageCaused or event == sgs.Damage then
		player = data:toDamage().from
	elseif event == sgs.DamageForseen or event == sgs.DamageInflicted or event == sgs.PreDamageDone or event == sgs.DamageDone
		or event == sgs.Damaged or event == sgs.DamageComplete then
		player = data:toDamage().to
	elseif event == sgs.EnterDying  or event == sgs.QuitDying or event == sgs.AskForPeachesDone then
		player = data:toDying().who
	elseif event == sgs.AskForPeaches then --event == sgs.Dying
		player = data:toDying().nowAskingForPeaches
	elseif event == sgs.BuryVictim then  --event == sgs.Death
		player = data:toDeath().who
	--elseif event == sgs.SlashEffected   sgs.CardAsked sgs.CardResponded
	--sgs.CardsMoveOneTime
	elseif  event == sgs.CardUsed  then
		player = data:toCardUse().from
		--sgs.TargetSpecifying sgs.TargetConfirming  sgs.TargetSpecified  TargetConfirmed CardFinished
	elseif  event == sgs.CardEffect  then
		player = data:toCardEffect().to
	end


	if player and player:objectName()==self.player:objectName() then
		if sgs.debugmode and type(sgs.ai_debug_func[event])=="table" then
			for _,callback in pairs(sgs.ai_debug_func[event]) do
				if type(callback)=="function" then callback(self,player,data) end
			end
		end
		if type(sgs.ai_chat_func[event])=="table" and sgs.GetConfig("AIChat", false) and player:getState() == "robot" then
			for _,callback in pairs(sgs.ai_chat_func[event]) do
				if type(callback)=="function" then callback(self,player,data) end
			end
		end
		if type(sgs.ai_event_callback[event])=="table" then
			for _,callback in pairs(sgs.ai_event_callback[event]) do
				if type(callback)=="function" then callback(self,player,data) end
			end
		end
	end

	sgs.lastevent = event
	sgs.lasteventdata = eventdata

	if event == sgs.ChoiceMade and self == sgs.recorder then
		local s = data:toChoiceMade()

		if (s.type == sgs.ChoiceMadeStruct_Activate or s.type == sgs.ChoiceMadeStruct_CardUsed) then
			local carduse =sgs.CardUseStruct()
			local carduse_str = s.args[#s.args]

			carduse:parse(str, self.room)
			if carduse.card ~= nil then
				for _, aflag in ipairs(sgs.ai_global_flags) do
					sgs[aflag] = nil
				end
				for _, callback in ipairs(sgs.ai_choicemade_filter.cardUsed) do
					if type(callback) == "function" then
						callback(self, s.player, carduse)
					end
				end
			end
		else
			local tableName = "nil"
			if s.type == sgs.ChoiceMadeStruct_CardResponded then
				tableName = "cardResponded"
			elseif s.type == sgs.ChoiceMadeStruct_SkillInvoke then
				tableName = "skillInvoke"
			elseif s.type == sgs.ChoiceMadeStruct_SkillChoice then
				tableName = "skillChoice"
			elseif s.type == sgs.ChoiceMadeStruct_Nullification then
				tableName = "Nullification"
			elseif s.type == sgs.ChoiceMadeStruct_CardChosen then
				tableName = "cardChosen"
			elseif s.type == sgs.ChoiceMadeStruct_AGChosen then
				tableName = "AGChosen"
			elseif s.type == sgs.ChoiceMadeStruct_CardShow then
				tableName = "cardShow"
			elseif s.type == sgs.ChoiceMadeStruct_Peach then
				tableName = "peach"
			elseif s.type == sgs.ChoiceMadeStruct_CardDiscard then
				tableName = "cardDiscard"
			elseif s.type == sgs.ChoiceMadeStruct_ViewCards then
				tableName = "viewCards"
			elseif s.type == sgs.ChoiceMadeStruct_PlayerChosen then
				tableName = "playerChosen"
			elseif s.type == sgs.ChoiceMadeStruct_Yiji then
				tableName = "Yiji"
			elseif s.type == sgs.ChoiceMadeStruct_Rende then
				tableName = "Rende"
			elseif s.type == sgs.ChoiceMadeStruct_Pindian then
				tableName = "pindian"
			elseif s.type == sgs.ChoiceMadeStruct_CardExchange then
				tableName = "cardExchange"
			end

			local callbacktable = sgs.ai_choicemade_filter[tableName]
			if callbacktable and type(callbacktable) == "table" then
				local index = 1
				if s.type == sgs.ChoiceMadeStruct_CardResponded then
					--预估lack
					--pattern
					if s.args[#s.args] == "_nil_" then
						if s.args[1]:match("jink") then sgs.card_lack[s.player:objectName()]["Jink"] = 1
						elseif s.args[1]:match("slash") then sgs.card_lack[s.player:objectName()]["Slash"] = 1
						elseif s.args[1]:match("peach") then sgs.card_lack[s.player:objectName()]["Peach"] = 1 end
					end
					--改判者
					--[[if promptlist[2] == "@guicai-card" or promptlist[2] == "@guidao-card" or promptlist[2] == "@huanshi-card" then
						if promptlist[#promptlist] == "_nil_" then
							sgs.RetrialPlayer = nil
						else
							sgs.RetrialPlayer = s.player
						end
					end]]
					index = 2
				end
				local callback = callbacktable[(s.args[index]:split(":"))[1]] or callbacktable.general
				if type(callback) == "function" then
					callback(self, s.player, s.args)
				end
			end
			--[[if data:toString() == "skillInvoke:fenxin:yes" then
				for _, aplayer in sgs.qlist(self.room:getAllPlayers()) do
					if aplayer:hasFlag("FenxinTarget") then
						local temp_table = sgs.role_evaluation[player:objectName()]
						sgs.role_evaluation[player:objectName()] = sgs.role_evaluation[aplayer:objectName()]
						sgs.role_evaluation[aplayer:objectName()] = temp_table
						self:updatePlayers()
						break
					end
				end
			end]]
		end

		if  s.type == sgs.ChoiceMadeStruct_Kingdom then
			local lord = self.room:getLord()
			if lord and lord:getGeneral():isLord() then
				local hasKingdomLordSkill = false
				for _,s in sgs.qlist(lord:getVisibleSkillList()) do
					if s:isLordSkill() then
						local callback = sgs.ai_skillProperty[s:objectName()]
						if not callback or callback(self) ~= "noKingdom" then
							hasKingdomLordSkill = true
							break
						end
					end
				end
				if hasKingdomLordSkill then
					local kingdomChoice  = s.args[#s.args]
					if kingdomChoice ~= lord:getKingdom() then
						sgs.updateIntention(s.player, lord, 50)
					end
				end
			end
		end
	elseif event == sgs.CardUsed or event == sgs.CardEffect or event == sgs.GameStart or event == sgs.EventPhaseStart then
		self:updatePlayers()
	elseif event == sgs.BuryVictim or event == sgs.HpChanged or event == sgs.MaxHpChanged then
		self:updatePlayers(false)
	end

	if event == sgs.BuryVictim then
		if self == sgs.recorder then self:updateAlivePlayerRoles() end
	end

	if  event == sgs.AskForPeaches and self.player:objectName() == player:objectName() then
		local dying = data:toDying()
		if self:isFriend(dying.who) and dying.who:getHp() < 1 then
			sgs.card_lack[player:objectName()]["Peach"]=1
		end
	end
	if event == sgs.CardsMoveOneTime  then
		local move = data:toMoveOneTime()
		if move.to and self.player:objectName() == move.to:objectName() and self.player:getPhase() ~= sgs.Player_Play
		and move.to_place == sgs.Player_PlaceHand and self.player:getHandcardNum() > 1 then
			self:assignKeep(false, true)
		end
	end

	if self ~= sgs.recorder then return end

	if event == sgs.TargetConfirmed then
		local struct = data:toCardUse()
		local from  = struct.from
		local card = struct.card
		--this mark will lock ai_role for woyu target
		-- if struct.to and from and from:objectName() == player:objectName()
			-- and card:isKindOf("WoyuCard") then
			-- self.room:setPlayerMark(struct.to:first(), "woyuRole",1)
			-- self:updatePlayers()
		-- end
		if from  then  --and from:objectName() == player:objectName()
			if card:isKindOf("SingleTargetTrick") then sgs.TrickUsefrom = from end
			local to = sgs.QList2Table(struct.to)
			local callback = sgs.ai_card_intention[card:getClassName()]
			if callback then
				if type(callback) == "function" then
					callback(self, card, from, to)
				elseif type(callback) == "number" then
					sgs.updateIntentions(from, to, callback, card)
				end
			end
			if card:getClassName() == "LuaSkillCard" and card:isKindOf("LuaSkillCard") then
				local luaskillcardcallback = sgs.ai_card_intention[card:objectName()]
				if luaskillcardcallback then
					if type(luaskillcardcallback) == "function" then
						luaskillcardcallback(self, card, from, to)
					elseif type(luaskillcardcallback) == "number" then
						sgs.updateIntentions(from, to, luaskillcardcallback, card)
					end
				end
			end
		end

		local lord = getLord(self.player)
		if lord and struct.card and lord:getHp() == 1 and self:aoeIsEffective(struct.card, lord, from) then
			if struct.card:isKindOf("SavageAssault") and struct.to:contains(lord) then
				sgs.ai_lord_in_danger_SA = true
			elseif struct.card:isKindOf("ArcheryAttack") and struct.to:contains(lord) then
				sgs.ai_lord_in_danger_AA = true
			end
		end

		local to = sgs.QList2Table(struct.to)
		local isneutral = true
		for _, p in ipairs(to) do
			if sgs.ai_role[p:objectName()] ~= "neutral" then isneutral = false break end
		end
		--盲狙？
		local who = to[1]
		if sgs.turncount <= 1 and lord and who and from  and sgs.evaluatePlayerRole(from) == "neutral" then
				if  (card:isKindOf("FireAttack")
					or ((card:isKindOf("Dismantlement") or card:isKindOf("Snatch"))
						and not self:needToThrowArmor(who) and not who:hasSkills("tuntian+zaoxian")
						and not (who:getCards("j"):length() > 0 and not who:containsTrick("YanxiaoCard"))
						and not (who:getCards("e"):length() > 0 and self:hasSkills(sgs.lose_equip_skill, who))
						and not (self:needKongcheng(who) and who:getHandcardNum() == 1))
					or (card:isKindOf("Slash") and not (self:getDamagedEffects(who, from, true) or self:needToLoseHp(who, from, true, true))
						and not ((who:hasSkill("leiji") or who:hasSkills("tuntian+zaoxian")) and getCardsNum("Jink", who, from) > 0))
					or (card:isKindOf("Duel") and card:getSkillName() ~= "lijian" and card:getSkillName() ~= "noslijian"
						and not (self:getDamagedEffects(who, from) or self:needToLoseHp(who, from, nil, true, true))))
				then
				local exclude_lord = #self:exclude({lord}, card, from) > 0
				--东方杀特定判断
				--local yyc009
				if (card:getSkillName() =="chuangshi") then
					from=self.room:getCurrent()
				end
				if CanUpdateIntention(from) and exclude_lord and sgs.evaluatePlayerRole(who) == "neutral" and isneutral then
					sgs.updateIntention(from, lord, -10)
					--elseif card:isKindOf("ThunderSlash") and who:hasSkill("jingdian") then
				--else
						--对已知身份的话 纯属多余?
					--sgs.updateIntention(from, who, 10)
				end
			end
		end

		if from and sgs.ai_role[from:objectName()] == "rebel" and not self:isFriend(from, self.room:findPlayer(from:getNextAlive():objectName())) --self:isFriend(from, from:getNextAlive())
			and (card:isKindOf("SavageAssault") or card:isKindOf("ArcheryAttack") or card:isKindOf("Duel") or card:isKindOf("Slash")) then
			for _, target in ipairs(to) do
				if self:isFriend(target, from) and sgs.ai_role[target:objectName()] == "rebel" and target:getHp() == 1 and target:isKongcheng()
				and sgs.isGoodTarget(target, nil, self) and getCardsNum("Analeptic", target, from) + getCardsNum("Peach", target, from) == 0
				and self:getEnemyNumBySeat(from, target) > 0 then
					if not target:hasFlag("AI_doNotSave") then target:setFlags("AI_doNotSave") end
				end
			end
		end

		if card:isKindOf("AOE") then
			for _, t in sgs.qlist(struct.to) do
				if t:hasSkill("fangzhu") then sgs.ai_AOE_data = data break end
			end
		end

		if card:getSkillName() == "qice" and (card:isKindOf("SavageAssault") or card:isKindOf("ArcheryAttack") or card:isKindOf("Duel") or card:isKindOf("FireAttack")) then
			sgs.ai_qice_data = data
		end

	elseif event == sgs.CardEffect then
		local struct = data:toCardEffect()
		local card = struct.card
		local from = struct.from
		local to = struct.to
		local card = struct.card
		local lord = getLord(player)

		if card and card:isKindOf("AOE") and to and to:isLord() and (sgs.ai_lord_in_danger_SA or sgs.ai_lord_in_danger_AA) then
			sgs.ai_lord_in_danger_SA = nil
			sgs.ai_lord_in_danger_AA = nil
		end

	elseif event == sgs.PreDamageDone then
		--铁索 求桃预估
		local damage = data:toDamage()
		local clear = true
		if damage.card and damage.card:hasFlag("nosjiefan-slash") then clear = false end
		if clear and damage.to:isChained() then
			for _, p in sgs.qlist(self.room:getOtherPlayers(damage.to)) do
				if p:isChained() and damage.nature ~= sgs.DamageStruct_Normal then
					clear = false
					break
				end
			end
		end
		if not clear then
			if damage.nature ~= sgs.DamageStruct_Normal and not damage.chain then
				for _, p in sgs.qlist(self.room:getAlivePlayers()) do
					local added = 0
					local dyingThreshold = p:dyingThreshold()
					if p:objectName() == damage.to:objectName() and p:isChained() and (p:getHp() - damage.damage) < dyingThreshold  then
						sgs.ai_NeedPeach[p:objectName()] = damage.damage + dyingThreshold - p:getHp()
					elseif p:objectName() ~= damage.to:objectName() and p:isChained() and self:damageIsEffective(p, damage.nature, damage.from) then
						if damage.nature == sgs.DamageStruct_Fire then
							added = p:hasArmorEffect("Vine") and added + 1 or added
							sgs.ai_NeedPeach[p:objectName()] = damage.damage + dyingThreshold + added - p:getHp()
						elseif damage.nature == sgs.DamageStruct_Thunder then
							sgs.ai_NeedPeach[p:objectName()] = damage.damage + dyingThreshold + added - p:getHp()
						end
					end
				end
			end
		else
			for _, p in sgs.qlist(self.room:getAlivePlayers()) do
				sgs.ai_NeedPeach[p:objectName()] = 0
			end
		end
	elseif event == sgs.Damaged then
		local damage = data:toDamage()
		local card = damage.card
		local from = damage.from
		local to = damage.to
		local source = self.room:getCurrent()
		local reason = damage.reason
		if damage.card and damage.card:getSkillName()=="chuangshi" then  intention = 0 end
		if damage.card and from and damage.card:hasFlag("WushenDamage_" .. damage.from:objectName()) then  intention = 0 end
		if damage.nature == sgs.DamageStruct_Fire and to:hasSkill("kaifeng") then intention = 0 end
		if not damage.card then
			local intention
			if sgs.ai_quhu_effect then
				sgs.ai_quhu_effect = false
				local xunyu = self.room:findPlayerBySkillName("quhu")
				intention = 80
				from = xunyu
			elseif sgs.ai_chuangshi_effect then
				sgs.ai_chuangshi_effect = false
				local yyc009 = self.room:findPlayerBySkillName("chuangshi")
				intention = 80
				from = yyc009

			elseif from and (from:hasFlag("ShenfenUsing") or from:hasFlag("FenchengUsing")) then
				intention = 0
			elseif reason == "zhendu" then
				intention = 0
			else
				intention = 100
			end

			if damage.transfer or damage.chain then intention = 0 end
			if sgs.no_intention_damage:match(reason) then  intention = 0 end

			if from and intention ~= 0 then sgs.updateIntention(from, to, intention) end
		end
	elseif event == sgs.CardUsed then
		local struct = data:toCardUse()
		local card = struct.card
		local lord = getLord(player)
		local who
		if not struct.to:isEmpty() then who = struct.to:first() end

		if card and lord and card:isKindOf("Duel") and lord:hasFlag("AIGlobal_NeedToWake") then
			lord:setFlags("-AIGlobal_NeedToWake")
		end

		if card and (card:isKindOf("ExNihilo") or card:isKindOf("Peach"))then

			if card:getSkillName()=="chuangshi" then
				chuangshiSource = self.room:getCurrent()
				if chuangshiSource and chuangshiSource:hasSkill("chuangshi") and struct.from then
					sgs.updateIntention(chuangshiSource, struct.from, -80)
				end
			end
		end


		if card:isKindOf("Snatch") or card:isKindOf("Dismantlement") then
			for _, p in sgs.qlist(struct.to) do
				for _, c in sgs.qlist(p:getCards("hejs")) do
					self.room:setCardFlag(c, "-AIGlobal_SDCardChosen_"..card:objectName())
				end
			end
		end

		if card:isKindOf("Analeptic") then
			local cards = struct.from:getHandcards()
			cards=self:touhouAppendExpandPileToList(struct.from,cards)
			for _,c in sgs.qlist(cards) do
				self.room:setCardFlag(c, "-AIGlobal_SearchForAnaleptic")
			end
		end

		if card:isKindOf("AOE") and sgs.ai_AOE_data then
			sgs.ai_AOE_data = nil
		end

		if card:getSkillName() == "qice" and sgs.ai_qice_data then
			sgs.ai_qice_data = nil
		end

		if card:isKindOf("Slash") and struct.from:objectName() == self.room:getCurrent():objectName() and struct.m_reason == sgs.CardUseStruct_CARD_USE_REASON_PLAY
			and struct.m_addHistory then struct.from:setFlags("hasUsedSlash") end

		--借刀仇恨判断。不过没见起过作用。 借刀调整后，暂时也用不上了
		--[[if card:isKindOf("Collateral") then
			local victim
			local CollateralTargets={}
			local dummySlash = sgs.cloneCard("slash", sgs.Card_NoSuit, 0)

			for _,p in sgs.qlist(self.room:getOtherPlayers(struct.to:first())) do
				if p:hasFlag("SlashAssignee") then
					victim=p
				end
				if struct.to:first():canSlash(p,dummySlash,true) then
					table.insert(CollateralTargets,p)
				end
			end
			if victim then
					for _,p in pairs(CollateralTargets) do
					if not self:isFriend(p,victim) then
						sgs.updateIntention(struct.from, victim, 80)
					end
				end
			end
			sgs.ai_collateral = false
		end]]

	elseif event == sgs.CardsMoveOneTime then
		local move = data:toMoveOneTime()
		local from = nil   -- convert move.from from const Player * to ServerPlayer *
		local to   = nil   -- convert move.to to const Player * to ServerPlayer *
		if move.from then from = findPlayerByObjectName(self.room, move.from:objectName(), true) end
		if move.to   then to   = findPlayerByObjectName(self.room, move.to:objectName(), true) end
		local reason = move.reason
		local from_places = sgs.QList2Table(move.from_places)
		local lord
		if from then lord = getLord(from) end

		if self.room:findPlayerBySkillName("fenji") then
			sgs.ai_fenji_target = nil
			if from and from:isAlive() and move.from_places:contains(sgs.Player_PlaceHand)
				and ((move.reason.m_reason == sgs.CardMoveReason_S_REASON_DISMANTLE and move.reason.m_playerId ~= move.reason.m_targetId)
						or (to and to:objectName() ~= from:objectName() and move.to_place == sgs.Player_PlaceHand)) then
				sgs.ai_fenji_target = from
			end
		end


		for i = 0, move.card_ids:length()-1 do
			local place = move.from_places:at(i)
			local card_id = move.card_ids:at(i)
			local card = sgs.Sanguosha:getCard(card_id)

			if place == sgs.Player_DrawPile
				or (move.to_place == sgs.Player_DrawPile and not (from and tonumber(from:property("zongxuan_move"):toString()) == card_id)) then
				self.top_draw_pile_id = nil
			end

			if move.to_place == sgs.Player_PlaceHand and to then
				if card:hasFlag("visible") then
					if isCard("Slash",card, to) then sgs.card_lack[to:objectName()]["Slash"]=0 end
					if isCard("Jink",card, to) then sgs.card_lack[to:objectName()]["Jink"]=0 end
					if isCard("Peach",card, to) then sgs.card_lack[to:objectName()]["Peach"]=0 end
				else
					sgs.card_lack[to:objectName()]["Slash"]=0
					sgs.card_lack[to:objectName()]["Jink"]=0
					sgs.card_lack[to:objectName()]["Peach"]=0
				end
			end

			if move.to_place == sgs.Player_PlaceHand and to and place ~= sgs.Player_DrawPile then
				if from and from:objectName() ~= to:objectName() and place == sgs.Player_PlaceHand and not card:hasFlag("visible") then
					local flag = string.format("%s_%s_%s", "visible", from:objectName(), to:objectName())
					global_room:setCardFlag(card_id, flag, from)
				end
			end

			if reason.m_skillName == "qiaobian" and from and to  then
				local current = self.room:getCurrent()
				if table.contains(from_places, sgs.Player_PlaceDelayedTrick) then
					if card:isKindOf("YanxiaoCard") then
						sgs.updateIntention(current, from, 80)
						sgs.updateIntention(current, to, -80)
					end
					if card:isKindOf("SupplyShortage") or card:isKindOf("Indulgence") then
						sgs.updateIntention(current, from, -80)
						sgs.updateIntention(current, to, 80)
					end
				end

				if table.contains(from_places, sgs.Player_PlaceEquip) then
					sgs.updateIntention(current, to, -80)
				end

			end

			-- 张角用
			if from and from:hasFlag("AI_Playing") and from:hasSkill("leiji") and from:getPhase() == sgs.Player_Discard and isCard("Jink", card, from)
			and from:getHandcardNum() >= 2 and reason.m_reason == sgs.CardMoveReason_S_REASON_RULEDISCARD then sgs.card_lack[from:objectName()]["Jink"] = 2 end
			--弃杀时的辅助判断  被禁止出杀
			--and sgs.turncount <= 3 --为什么有轮次限制。。。。
			if from and from:hasFlag("AI_Playing") and sgs.turncount <= 3 and from:getPhase() == sgs.Player_Discard
				and reason.m_reason == sgs.CardMoveReason_S_REASON_RULEDISCARD
				and not (from:hasSkills("renjie+baiyin") and not from:hasSkill("jilve")) and not from:hasFlag("ShuangrenSkipPlay")
				and not  from:hasFlag("PlayPhaseTerminated") then

				local is_neutral = sgs.evaluatePlayerRole(from) == "neutral" and CanUpdateIntention(from)

				if isCard("Slash", card, from) and not from:hasFlag("hasUsedSlash") or from:hasFlag("JiangchiInvoke") then
					for _, target in sgs.qlist(self.room:getOtherPlayers(from)) do
						local has_slash_prohibit_skill = false
						for _, askill in sgs.qlist(target:getVisibleSkillList()) do
							local s_name = askill:objectName()
							local filter = sgs.ai_slash_prohibit[s_name]
							if filter and type(filter) == "function" and not (s_name == "tiandu" or s_name == "hujia" or s_name == "huilei" or s_name == "weidi") then
								if s_name == "xiangle" then
									local basic_num = 0
									for _, c_id in sgs.qlist(move.card_ids) do
										local c = sgs.Sanguosha:getCard(c_id)
										if c:isKindOf("BasicCard") then
											basic_num = basic_num + 1
										end
									end
									if basic_num < 2 then has_slash_prohibit_skill = true break end
								else
									has_slash_prohibit_skill = true
									break
								end
							end
						end

						if target:hasSkill("fangzhu") and target:getLostHp() < 2 then
							has_slash_prohibit_skill = true
						end
						if self:slashProhibitToEghitDiagram(card,from,target)
						or self:slashProhibitToDiaopingTarget(card,from,target) then
							has_slash_prohibit_skill = true
						end
						if from:canSlash(target, card, true) and self:slashIsEffective(card, target) --没有用from?
								and not has_slash_prohibit_skill and sgs.isGoodTarget(target,self.enemies, self) then
							if is_neutral  then --or (CanUpdateIntention(player) and self:isEnemy(player, target))
								sgs.updateIntention(from, target, -35)
								self:updatePlayers()
							end
							if (sgs.evaluatePlayerRole(from) ~= "neutral" and not from:isLord()) then

							end
						end
					end
				end

				local zhanghe = self.room:findPlayerBySkillName("qiaobian")
				local consider_zhanghe = (zhanghe and lord and self:playerGetRound(zhanghe) <= self:playerGetRound(lord) and self:isFriend(zhanghe, lord))
				if not consider_zhanghe then
					if isCard("Indulgence", card, from) and lord and not lord:hasSkill("qiaobian") then
						for _, target in sgs.qlist(self.room:getOtherPlayers(from)) do
							if not (target:containsTrick("indulgence") or target:containsTrick("YanxiaoCard") or self:hasSkills("qiaobian", target)) then
								local aplayer = self:exclude( {target}, card, from)
								if #aplayer ==1 and is_neutral then
									if not self:touhouDelayTrickBadTarget(card, target, from) then
										sgs.updateIntention(from, target, -35)
										self:updatePlayers()
									end
								end
							end
						end
					end

					if isCard("SupplyShortage", card, from) and lord and not lord:hasSkill("qiaobian") then
						for _, target in sgs.qlist(self.room:getOtherPlayers(from)) do
							if from:distanceTo(target) <= (from:hasSkill("duanliang") and 2 or 1) and
									not (target:containsTrick("supply_shortage") or target:containsTrick("YanxiaoCard") or self:hasSkills("qiaobian", target)) then
								local aplayer = self:exclude( {target}, card, from)
								if #aplayer == 1 and is_neutral then
									if not self:touhouDelayTrickBadTarget(card, target, from) then
										sgs.updateIntention(from, target, -35)
										self:updatePlayers()
									end
								end
							end
						end
					end
				end

			end
		end

	elseif event == sgs.StartJudge then
		local judge = data:toJudge()
		local reason = judge.reason
		if reason == "beige" then
			local caiwenji = self.room:findPlayerBySkillName("beige")
			local intention = -60
			if player:objectName() == caiwenji:objectName() then intention = 0 end
			sgs.updateIntention(caiwenji, player, intention)
		end
		if reason == "shijie" then
			local source = self.room:findPlayerBySkillName("shijie")
			local intention = -60
			if player:objectName() == source:objectName() then intention = 0 end
			sgs.updateIntention(source, player, intention)
		end
		sgs.JudgeResult = judge:isGood()
	elseif event == sgs.AskForRetrial then
		local judge = data:toJudge()
		if sgs.JudgeResult ~= judge:isGood() and sgs.RetrialPlayer and judge.who then
			if sgs.judge_reason:match(judge.reason) then
				if judge:isGood() then
					sgs.updateIntention(sgs.RetrialPlayer, judge.who, -10)
				else
					sgs.updateIntention(sgs.RetrialPlayer, judge.who, 10)
				end
			end
			sgs.RetrialPlayer = nil
			sgs.JudgeResult = judge:isGood()
		end
	elseif event == sgs.EventPhaseEnd and player:getPhase() ==  sgs.Player_Play then
		player:setFlags("AI_Playing")
		self:dangjiaIntention(player)
	elseif event == sgs.EventPhaseStart and player:getPhase() ==  sgs.Player_NotActive then
		if player:isLord()  and player:getMark("touhou-extra") == 0
		then sgs.turncount = sgs.turncount + 1 end

		sgs.debugmode = io.open("lua/ai/debug")
		if sgs.debugmode then sgs.debugmode:close() end


		if sgs.turncount == 1 and player:isLord() then
			local msg = ""
			local humanCount = 0
			for _, aplayer in sgs.qlist(self.room:getAllPlayers()) do
				if aplayer:getState() ~= "robot" then humanCount = humanCount +1 end
				if not aplayer:isLord() then
					msg = msg..string.format("%s\t%s\r\n",aplayer:getGeneralName(),aplayer:getRole())
				end
			end
			self.room:setTag("humanCount",sgs.QVariant(humanCount))

			if humanCount == 1 and not sgs.isRolePredictable() and not sgs.GetConfig("EnableHegemony", false) then
				-- global_room:writeToConsole(msg)
			end
		end

	elseif event == sgs.GameStart then
		sgs.debugmode = io.open("lua/ai/debug")
		if sgs.debugmode then sgs.debugmode:close() end

		if player and player:isLord() then
			if sgs.debugmode then logmsg("ai.html","<meta charset='utf-8'/>") end
		end

		if not sgs.GetConfig("AIProhibitBlindAttack", false) then
			self:roleParse()--为主忠盲狙  自动增加克制主公的明反的仇恨
		end
	end
end

function SmartAI:askForSuit(reason)
	if not reason then return sgs.ai_skill_suit.fanjian(self) end -- this line is kept for back-compatibility
	local callback = sgs.ai_skill_suit[reason]
	if type(callback) == "function" then
		if callback(self) then return callback(self) end
	end
	return math.random(0,3)
end

function SmartAI:askForSkillInvoke(skill_name, data)
	skill_name = string.gsub(skill_name, "%-", "_")
	local invoke = sgs.ai_skill_invoke[skill_name]
	if type(invoke) == "boolean" then
		return invoke
	elseif type(invoke) == "function" then
		return invoke(self, data)
	else
		local skill = sgs.Sanguosha:getSkill(skill_name)
		return skill and skill:getFrequency() == sgs.Skill_Frequent
	end
end

function SmartAI:askForChoice(skill_name, choices, data)
	local choice = sgs.ai_skill_choice[skill_name]
	if type(choice) == "string" then
		return choice
	elseif type(choice) == "function" then
		return choice(self, choices, data)
	else
		local choice_table = choices:split("+")
		for index, achoice in ipairs(choice_table) do
			if achoice == "cancel" then return achoice end
		end
		local r = math.random(1, #choice_table)
		return choice_table[r]
	end
end

function SmartAI:askForDiscard(reason, discard_num, min_num, optional, include_equip)
	min_num = min_num or discard_num
	local exchange = self.player:hasFlag("Global_AIDiscardExchanging")
	local callback = sgs.ai_skill_discard[reason]
	self:assignKeep(nil, true)
	if type(callback) == "function" then
		local cb = callback(self, discard_num, min_num, optional, include_equip)
		if cb then
			if type(cb) == "number" and not self.player:isJilei(sgs.Sanguosha:getCard(cb)) then return { cb }
			elseif type(cb) == "table" then
				for _, card_id in ipairs(cb) do
					if not exchange and self.player:isJilei(sgs.Sanguosha:getCard(card_id)) then
						return {}
					end
				end
				return cb
			end
			return {}
		end
	elseif optional then
		return {}
	end

	local flag = "hs"
	if include_equip and (self.player:getEquips():isEmpty() or not self.player:isJilei(self.player:getEquips():first())) then flag = flag .. "e" end
	local cards = self.player:getCards(flag)
	local to_discard = {}
	cards = sgs.QList2Table(cards)
	local aux_func = function(card)
		local place = self.room:getCardPlace(card:getEffectiveId())
		if place == sgs.Player_PlaceEquip then
			if card:isKindOf("SilverLion") and self.player:isWounded() then return -2
			elseif card:isKindOf("Weapon") and self.player:getHandcardNum() < discard_num + 2 and not self:needKongcheng() then return 0
			elseif card:isKindOf("OffensiveHorse") and self.player:getHandcardNum() < discard_num + 2 and not self:needKongcheng() then return 0
			elseif card:isKindOf("OffensiveHorse") then return 1
			elseif card:isKindOf("Weapon") then return 2
			elseif card:isKindOf("DefensiveHorse") then return 3
			elseif self:hasSkills("bazhen|yizhong") and card:isKindOf("Armor") then return 0
			elseif card:isKindOf("Armor") then return 4
			elseif card:isKindOf("Treasure") then return 4
			end
		elseif self:hasSkills(sgs.lose_equip_skill) then return 5
		else return 0
		end
	end
	local compare_func = function(a, b)
		if aux_func(a) ~= aux_func(b) then return aux_func(a) < aux_func(b) end
		return self:getKeepValue(a) < self:getKeepValue(b)
	end

	table.sort(cards, compare_func)
	local least = min_num
	if discard_num - min_num > 1 then
		least = discard_num - 1
	end
	for _, card in ipairs(cards) do
		if (self.player:hasSkill("qinyin") and #to_discard >= least) or #to_discard >= discard_num then break end
		if exchange or not self.player:isJilei(card) then table.insert(to_discard, card:getId()) end
	end
	return to_discard
end

sgs.ai_skill_discard.gamerule = function(self, discard_num, min_num)

	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	local to_discard = {}

	local least = min_num
	if discard_num - min_num > 1 then least = discard_num - 1 end

	for _, card in ipairs(cards) do
		if not self.player:isCardLimited(card, sgs.Card_MethodDiscard, true) then
			table.insert(to_discard, card:getId())
		end
		if (self.player:hasSkill("qinyin") and #to_discard >= least) or #to_discard >= discard_num or self.player:isKongcheng() then break end
	end

	return to_discard

end


---询问无懈可击--
--东方杀相关
--待加入【法华】【正义】【云上】
--待加入【幻梦】一系列取消伤害
--留住无邪防乐怎么弄？
--【威压】
function SmartAI:askForNullification(trick, from, to, positive) --尼玛一把明杀 就因为自己1血就把无邪打出去了？？？？？
	if self.player:isDead() then return nil end
	local cards = self.player:getCards("hes")
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)
	local null_card
	null_card = self:getCardId("Nullification") --无懈可击
	local null_num = self:getCardsNum("Nullification")

	if self:hasWeiya() then--威压
		if self:getCardsNum("Nullification") < 2 then
			return false
		end
	end
	--理智
	if from and self:isFriend(from)and from:hasSkill("lizhi")
		and to and self:isFriend(from,to)
		and sgs.dynamic_value.damage_card[trick:getClassName()] then
		return false
	end

	local current=self.room:getCurrent()
	if current:isAlive() and current:hasSkill("souji") and self:isEnemy(current) then
		return false
	end

	if self:touhouIsDamageCard(trick) then
		local dummy_damage = sgs.DamageStruct(trick, from, to, 1, sgs.DamageStruct_Normal)
		if trick:isKindOf("FireAttack") then
			dummy_damage.nature= sgs.DamageStruct_Fire
		end
		if not self:touhouNeedAvoidAttack(dummy_damage, from, to) then
			return false
		end
	end

	if null_card then null_card = sgs.Card_Parse(null_card) else return nil end --没有无懈可击
	if self.player:isLocked(null_card) then return nil end
	if (from and from:isDead()) or (to and to:isDead()) then return nil end --已死


	if trick:isKindOf("FireAttack") then
		if to:isKongcheng() or from:isKongcheng() then return nil end
		if self.player:objectName() == from:objectName() and self.player:getHandcardNum() == 1 and self.player:handCards():first() == null_card:getId() then return nil end
	end

	if ("snatch|dismantlement"):match(trick:objectName()) and to:isAllNude() then return nil end

	if self:isFriend(to) and to:hasFlag("AIGlobal_NeedToWake") then return end

	if from then
		if (trick:isKindOf("Duel") or trick:isKindOf("FireAttack") or trick:isKindOf("AOE")) and
			(self:getDamagedEffects(to, from) and self:isFriend(to)) then
			return nil
		end --原三国杀于此处考虑“绝情”“无言”、决斗、火攻、AOE
		if (trick:isKindOf("Duel") or trick:isKindOf("AOE")) and not self:damageIsEffective(to, sgs.DamageStruct_Normal) then return nil end --决斗、AOE
		if trick:isKindOf("FireAttack") and not self:damageIsEffective(to, sgs.DamageStruct_Fire) then return nil end --火攻
	end
	if (trick:isKindOf("Duel") or trick:isKindOf("FireAttack") or trick:isKindOf("AOE")) and self:needToLoseHp(to, from) and self:isFriend(to) then
		return nil --扣减体力有利
	end
	if trick:isKindOf("Drowning") and self:needToThrowArmor(to) and self:isFriend(to) then return nil end
	if positive then
		if from and (trick:isKindOf("FireAttack") or trick:isKindOf("Duel") or trick:isKindOf("AOE")) and (self:needDeath(to) or self:cantbeHurt(to, from)) then
			if self:isFriend(from) then return null_card end
			return
		end
		if ("snatch|dismantlement"):match(trick:objectName()) and not to:containsTrick("YanxiaoCard") and (to:containsTrick("indulgence") or to:containsTrick("supply_shortage")) then
			if self:isEnemy(from) then return null_card end
			if self:isFriend(to) and to:isNude() then return nil end
		end

		if trick:getSkillName() == "lijian" and trick:isKindOf("Duel") then
			if to:getHp() == 1 and sgs.ai_role[to:objectName()] == "rebel" and from and sgs.ai_role[from:objectName()] == "rebel" then return end
			if self:isFriend(to) and (self:isWeak(to) or null_num > 1 or self:getOverflow() or not self:isWeak()) then return null_card end
			return
		end

		if from and self:isEnemy(from) and (sgs.evaluatePlayerRole(from) ~= "neutral" or sgs.isRolePredictable()) then
			--使用者是敌方，自己有技能“空城”且无懈可击为最后一张手牌->命中
			if self.player:hasSkill("kongcheng") and self.player:getHandcardNum() == 1 and self.player:isLastHandCard(null_card) and trick:isKindOf("SingleTargetTrick") then
				return null_card
			end
			 --敌方在虚弱、需牌技、漫卷中使用无中生有->命中
			if trick:isKindOf("ExNihilo") and (self:isWeak(from) or self:hasSkills(sgs.cardneed_skill, from) or from:hasSkill("manjuan")) then return null_card end
			--铁索连环的目标没有藤甲->不管
			if trick:isKindOf("IronChain") and not to:hasArmorEffect("Vine") then return nil end
			if self:isFriend(to) then
				if trick:isKindOf("Dismantlement") then
					--敌方拆友方威胁牌、价值牌、最后一张手牌->命中
					if self:getDangerousCard(to) or self:getValuableCard(to) then return null_card end
					if to:getHandcardNum() == 1 and not self:needKongcheng(to) then
						if (getKnownCard(to, self.player, "TrickCard", false) == 1 or getKnownCard(to, self.player, "EquipCard", false) == 1 or getKnownCard(to, self.player, "Slash", false) == 1) then
							return nil
						end
						return null_card
					end
				else
					if trick:isKindOf("Snatch") then return null_card end
					if trick:isKindOf("Duel") and self:isWeak(to) then return null_card end
					if trick:isKindOf("FireAttack") and from:objectName() ~= to:objectName() then
						if from:getHandcardNum() > 2
							or self:isWeak(to)
							or to:hasArmorEffect("Vine")
							or to:isChained() and not self:isGoodChainTarget(to)
							then return null_card end
					end
				end
			elseif self:isEnemy(to) then
				 --敌方顺手牵羊、过河拆桥敌方判定区延时性锦囊->命中
				if (trick:isKindOf("Snatch") or trick:isKindOf("Dismantlement")) and to:getCards("j"):length() > 0 then
					return null_card
				elseif trick:isKindOf("Drowning") and self:needToThrowArmor(to) then
					return null_card
				end
			end
		end

		if self:isFriend(to) then
			if not (to:hasSkill("guanxing") and global_room:alivePlayerCount() > 4) then
				--无观星友方判定区有乐不思蜀->视“突袭”、“巧变”情形而定
				if trick:isKindOf("Indulgence") then
					if to:getHp() - to:getHandcardNum() >= 2 then return nil end
					if to:hasSkill("tuxi") and to:getHp() > 2 then return nil end
					if to:hasSkill("qiaobian") and not to:isKongcheng() then return nil end
					return null_card
				end
				--无观星友方判定区有兵粮寸断->视“鬼道”、“天妒”、“溃围”、“巧变”情形而定
				if trick:isKindOf("SupplyShortage") then
					if self:hasSkills("guidao|tiandu",to) then return nil end
					if to:hasSkill("qiaobian") and not to:isKongcheng() then return nil end
					return null_card
				end
			end
			--非无言来源使用多目标攻击性非延时锦囊
			if trick:isKindOf("AOE") then
				local lord = getLord(self.player)
				local currentplayer = self.room:getCurrent()
				--主公
				if lord and self:isFriend(lord) and self:isWeak(lord) and self:aoeIsEffective(trick, lord) and
					((lord:getSeat() - currentplayer:getSeat()) % (self.room:alivePlayerCount())) >
					((to:getSeat() - currentplayer:getSeat()) % (self.room:alivePlayerCount())) and not
					(self.player:objectName() == to:objectName() and self.player:getHp() == 1 and not self:canAvoidAOE(trick)) then
					return nil
				end
				--自己
				if self.player:objectName() == to:objectName() then
					if self:hasSkills("jieming|yiji|guixin", self.player) and
						(self.player:getHp() > 1 or self:getCardsNum("Peach") > 0 or self:getCardsNum("Analeptic") > 0) then
						return nil
					elseif not self:canAvoidAOE(trick) then
						return null_card
					end
				end
				--队友
				if self:isWeak(to) and self:aoeIsEffective(trick, to) then
					if ((to:getSeat() - currentplayer:getSeat()) % (self.room:alivePlayerCount())) >
					((self.player:getSeat() - currentplayer:getSeat()) % (self.room:alivePlayerCount())) or null_num > 1 then
						return null_card
					elseif self:canAvoidAOE(trick) or self.player:getHp() > 1 or (isLord(to) and self.role == "loyalist") then
						return null_card
					end
				end
			end
			--非无言来源对自己使用决斗
			if trick:isKindOf("Duel") then
				if self.player:objectName() == to:objectName() then
					if self:hasSkills(sgs.masochism_skill, self.player) and
						(self.player:getHp() > 1 or self:getCardsNum("Peach") > 0 or self:getCardsNum("Analeptic") > 0) then
						return nil
					elseif self:getCardsNum("Slash") == 0 then
						return null_card
					end
				end
			end
		end
		--虚弱敌方遇到桃园结义->命中
		if from then
			if self:isEnemy(to) then
				if trick:isKindOf("GodSalvation") and self:isWeak(to) then
					return null_card
				end
			end
		end

		--五谷：目前只无邪桃子和无中，其他情况待补充
		if trick:isKindOf("AmazingGrace") and self:isEnemy(to) then
			--local NP = to:getNextAlive()
			local NP = self.room:findPlayer(to:getNextAlive():objectName())
			if self:isFriend(NP) then
				local ag_ids = self.room:getTag("AmazingGrace"):toStringList()
				local peach_num, exnihilo_num, snatch_num, analeptic_num, crossbow_num = 0, 0, 0, 0, 0
				for _, ag_id in ipairs(ag_ids) do
					local ag_card = sgs.Sanguosha:getCard(ag_id)
					if ag_card:isKindOf("Peach") then peach_num = peach_num + 1 end
					if ag_card:isKindOf("ExNihilo") then exnihilo_num = exnihilo_num + 1 end
					if ag_card:isKindOf("Snatch") then snatch_num = snatch_num + 1 end
					if ag_card:isKindOf("Analeptic") then analeptic_num = analeptic_num + 1 end
					if ag_card:isKindOf("Crossbow") then crossbow_num = crossbow_num + 1 end
				end
				if (peach_num == 1 and to:getHp() < getBestHp(to)) or
					(peach_num > 0 and (self:isWeak(to) or NP:getHp() < getBestHp(NP) and self:getOverflow(NP) < 1)) then
					return null_card
				end
				if peach_num == 0 and not self:willSkipPlayPhase(NP) then
					if exnihilo_num > 0 then
						if NP:hasSkills("nosjizhi|jizhi|nosrende|rende|zhiheng") or NP:hasSkill("jilve") and NP:getMark("@bear") > 0 then return null_card end
					else
						for _, enemy in ipairs(self.enemies) do
							if snatch_num > 0 and to:distanceTo(enemy) == 1 and
								(self:willSkipPlayPhase(enemy, true) or self:willSkipDrawPhase(enemy, true)) then
								return null_card
							elseif analeptic_num > 0 and (enemy:hasWeapon("Axe") or getCardsNum("Axe", enemy, self.player) > 0) then
								return null_card
							elseif crossbow_num > 0 and getCardsNum("Slash", enemy, self.player) >= 3 then
								local slash = sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
								for _, friend in ipairs(self.friends) do
									if enemy:distanceTo(friend) == 1 and self:slashIsEffective(slash, friend, enemy) then
										return null_card
									end
								end
							end
						end
					end
				end
			end
		end

	else
		if from then
			if (trick:isKindOf("FireAttack") or trick:isKindOf("Duel") or trick:isKindOf("AOE")) and (self:needDeath(to) or self:cantbeHurt(to, from)) then
				if self:isEnemy(from) then return null_card end
				return
			end
			if trick:getSkillName() == "lijian" and trick:isKindOf("Duel") then
				if self:isEnemy(to) and (self:isWeak(to) or null_num > 1 or self:getOverflow() or not self:isWeak()) then return null_card end
				return
			end
			if from:objectName() == to:objectName() then
				if self:isFriend(from) then return null_card else return end
			end
			if not (trick:isKindOf("GlobalEffect") or trick:isKindOf("AOE")) then
				if self:isFriend(from) then
					if ("snatch|dismantlement"):match(trick:objectName()) and to:isNude() then
					elseif trick:isKindOf("FireAttack") and to:isKongcheng() then
					else return null_card end
				end
			end
		else
			if self:isEnemy(to) and (sgs.evaluatePlayerRole(to) ~= "neutral" or sgs.isRolePredictable()) then return null_card else return end
		end
	end
end

function SmartAI:getCardRandomly(who, flags)
	local cards = who:getCards(flags)
	if cards:isEmpty() then return end
	local r = math.random(0, cards:length()-1)
	local card = cards:at(r)
	if who:hasArmorEffect("SilverLion") then
		if self:isEnemy(who) and who:isWounded() and card == who:getArmor() then
			if r ~= (cards:length()-1) then
				card = cards:at(r+1)
			elseif r > 0 then
				card = cards:at(r-1)
			end
		end
	end
	return card:getEffectiveId()
end

--妈蛋 残局优势的时候需要优先拆电 我擦
function SmartAI:askForCardChosen(who, flags, reason, method)
	local isDiscard = (method == sgs.Card_MethodDiscard)
	local cardchosen = sgs.ai_skill_cardchosen[string.gsub(reason, "%-", "_")]
	local card
	if type(cardchosen) == "function" then
		card = cardchosen(self, who, flags, method)
		if card then return card:getEffectiveId() end
	elseif type(cardchosen) == "number" then
		sgs.ai_skill_cardchosen[string.gsub(reason, "%-", "_")] = nil
		for _, acard in sgs.qlist(who:getCards(flags)) do
			if acard:getEffectiveId() == cardchosen then return cardchosen end
		end
	end

	local duxinHandcards = {}
	if self.player:hasSkill("duxin") and flags:match("h") then
		duxinHandcards =  sgs.QList2Table(who:getCards(flag))
		if #duxinHandcards >0 then
			self:sortByKeepValue(duxinHandcards,true)
		end
	end

	if ("snatch|dismantlement|yinling"):match(reason)  then
		local flag = "AIGlobal_SDCardChosen_" .. reason
		local to_choose
		for _, card in sgs.qlist(who:getCards(flags)) do
			if card:hasFlag(flag) then
				card:setFlags("-" .. flag)
				to_choose = card:getId()
				break
			end
		end
		if to_choose  then
			local is_handcard
			if not who:isKongcheng() and who:handCards():contains(to_choose) then is_handcard = true end
			if is_handcard and reason == "dismantlement" and self.room:getMode() == "02_1v1" and sgs.GetConfig("1v1/Rule", "Classical") == "2013" then
				local cards = sgs.QList2Table(who:getHandcards())
				local peach, jink
				for _, card in ipairs(cards) do
					if not peach and isCard("Peach", card, who) then peach = card:getId() end
					if not jink and isCard("Jink", card, who) then jink = card:getId() end
					if peach and jink then break end
				end
				if peach or jink then return peach or jink end
				self:sortByKeepValue(cards, true)
				return cards[1]:getEffectiveId()
			else
				if not self:isFriend(who) and #duxinHandcards >0  and  self.room:getCardPlace(to_choose) ~=  sgs.Player_PlaceDelayedTrick and duxinHandcards[1]:getId() ~= to_choose then
					return duxinHandcards[1]:getId()
				else
					return to_choose
				end
			end
		end
	end

	if self:isFriend(who) then
		if flags:match("j") and not who:containsTrick("YanxiaoCard") and not (who:hasSkill("qiaobian") and who:getHandcardNum() > 0) then
			local tricks = who:getCards("j")
			local lightning, indulgence, supply_shortage
			for _, trick in sgs.qlist(tricks) do
				if trick:isKindOf("Lightning") and (not isDiscard or self.player:canDiscard(who, trick:getId())) then
					lightning = trick:getId()
				elseif trick:isKindOf("Indulgence") and (not isDiscard or self.player:canDiscard(who, trick:getId()))  then
					indulgence = trick:getId()
				elseif not trick:isKindOf("Disaster") and (not isDiscard or self.player:canDiscard(who, trick:getId())) then
					supply_shortage = trick:getId()
				end
			end

			if self:hasWizard(self.enemies) and lightning then
				return lightning
			end

			if indulgence and supply_shortage then
				if who:getHp() < who:getHandcardNum() then
					return indulgence
				else
					return supply_shortage
				end
			end

			if indulgence or supply_shortage then
				return indulgence or supply_shortage
			end
		end

		if flags:match("e") then
			if who:getArmor() and self:needToThrowArmor(who, reason == "moukui") and (not isDiscard or self.player:canDiscard(who, who:getArmor():getEffectiveId())) then
				return who:getArmor():getEffectiveId()
			end
			if who:getArmor() and self:evaluateArmor(who:getArmor(), who) < -5 and (not isDiscard or self.player:canDiscard(who, who:getArmor():getEffectiveId())) then
				return who:getArmor():getEffectiveId()
			end
			if self:hasSkills(sgs.lose_equip_skill, who) and self:isWeak(who) then
				if who:getWeapon() and (not isDiscard or self.player:canDiscard(who, who:getWeapon():getEffectiveId())) then return who:getWeapon():getEffectiveId() end
				if who:getOffensiveHorse() and (not isDiscard or self.player:canDiscard(who, who:getOffensiveHorse():getEffectiveId())) then return who:getOffensiveHorse():getEffectiveId() end
			end
		end
	else
		if #duxinHandcards >0 and (duxinHandcards[1]:isKindOf("Peach") or duxinHandcards[1]:isKindOf("Analeptic")) then
			return duxinHandcards[1]:getId()
		end

		local dangerous = self:getDangerousCard(who)
		if flags:match("e") and dangerous and (not isDiscard or self.player:canDiscard(who, dangerous)) then return dangerous end
		--拆木牛的优先度是否该提高呢？ 比dangerousCard还高
		if flags:match("e") and who:getTreasure()  and who:getPile("wooden_ox"):length() > 1 and (not isDiscard or self.player:canDiscard(who, who:getTreasure():getId())) then
			return who:getTreasure():getId()
		end
		if flags:match("e") and who:hasArmorEffect("EightDiagram") and not self:needToThrowArmor(who, reason == "moukui")
			and (not isDiscard or self.player:canDiscard(who, who:getArmor():getId())) then return who:getArmor():getId() end
		if flags:match("e") and self:hasSkills("jijiu|beige|mingce|weimu|qingcheng", who) and not self:doNotDiscard(who, "e", false, 1, reason) then
			if who:getDefensiveHorse() and (not isDiscard or self.player:canDiscard(who, who:getDefensiveHorse():getEffectiveId())) then return who:getDefensiveHorse():getEffectiveId() end
			if who:getArmor() and not self:needToThrowArmor(who, reason == "moukui") and (not isDiscard or self.player:canDiscard(who, who:getArmor():getEffectiveId())) then return who:getArmor():getEffectiveId() end
			if who:getOffensiveHorse() and (not who:hasSkill("jijiu") or who:getOffensiveHorse():isRed()) and (not isDiscard or self.player:canDiscard(who, who:getOffensiveHorse():getEffectiveId())) then
				return who:getOffensiveHorse():getEffectiveId()
			end
			if who:getWeapon() and (not who:hasSkill("jijiu") or who:getWeapon():isRed()) and (not isDiscard or self.player:canDiscard(who, who:getWeapon():getEffectiveId())) then
				return who:getWeapon():getEffectiveId()
			end
		end
		if flags:match("e") then
			local valuable = self:getValuableCard(who)
			if valuable and (not isDiscard or self.player:canDiscard(who, valuable)) then
				return valuable
			end
		end
		if flags:match("h") and (not isDiscard or self.player:canDiscard(who, "hs")) and  #duxinHandcards <=0 then
			if self:hasSkills("jijiu|qingnang|qiaobian|jieyin|beige|buyi|manjuan", who)
				and not who:isKongcheng() and who:getHandcardNum() <= 2 and not self:doNotDiscard(who, "hs", false, 1, reason) then
				return self:getCardRandomly(who, "hs")
			end
			local cards = sgs.QList2Table(who:getHandcards())
			local flag = string.format("%s_%s_%s", "visible", self.player:objectName(), who:objectName())
			if #cards <= 2 and not self:doNotDiscard(who, "hs", false, 1, reason) then
				for _, cc in ipairs(cards) do
					if (cc:hasFlag("visible") or cc:hasFlag(flag)) and (cc:isKindOf("Peach") or cc:isKindOf("Analeptic")) then
						return self:getCardRandomly(who, "hs")
					end
				end
			end
		end

		if flags:match("j") then
			local tricks = who:getCards("j")
			local lightning, yanxiao
			for _, trick in sgs.qlist(tricks) do
				if trick:isKindOf("Lightning") and (not isDiscard or self.player:canDiscard(who, trick:getId())) then
					lightning = trick:getId()
				elseif trick:isKindOf("YanxiaoCard") and (not isDiscard or self.player:canDiscard(who, trick:getId())) then
					yanxiao = trick:getId()
				end
			end
			if self:hasWizard(self.enemies, true) and lightning then
				return lightning
			end
			if yanxiao then
				return yanxiao
			end
		end

		if flags:match("h") and not self:doNotDiscard(who, "hs") then
			if (who:getHandcardNum() == 1 and sgs.getDefenseSlash(who, self) < 3 and who:getHp() <= 2) or self:hasSkills(sgs.cardneed_skill, who) then
				if #duxinHandcards >0 then
					return duxinHandcards[1]:getId()
				else
					return self:getCardRandomly(who, "hs")
				end
			end
		end

		if flags:match("e") and not self:doNotDiscard(who, "e") then
			if who:getDefensiveHorse() and (not isDiscard or self.player:canDiscard(who, who:getDefensiveHorse():getEffectiveId())) then return who:getDefensiveHorse():getEffectiveId() end
			if who:getArmor() and not self:needToThrowArmor(who, reason == "moukui") and (not isDiscard or self.player:canDiscard(who, who:getArmor():getEffectiveId())) then return who:getArmor():getEffectiveId() end
			if who:getOffensiveHorse() and (not isDiscard or self.player:canDiscard(who, who:getOffensiveHorse():getEffectiveId())) then return who:getOffensiveHorse():getEffectiveId() end
			if who:getWeapon() and (not isDiscard or self.player:canDiscard(who, who:getWeapon():getEffectiveId())) then return who:getWeapon():getEffectiveId() end
		end

		if flags:match("h") then
			if (not who:isKongcheng() and who:getHandcardNum() <= 2) and not self:doNotDiscard(who, "hs", false, 1, reason) then
				if #duxinHandcards >0 then
					return duxinHandcards[1]:getId()
				else
					return self:getCardRandomly(who, "hs")
				end
			end
		end
	end
	return -1
end

function sgs.ai_skill_cardask.nullfilter(self, data, pattern, target)
	if self.player:isDead() then return "." end
	local damage_nature = sgs.DamageStruct_Normal
	local effect
	if type(data) == "userdata" then
		effect = data:toSlashEffect()

		if effect and effect.slash then
			damage_nature = effect.nature
			if effect.slash:hasFlag("nosjiefan-slash") then
				local dying = self.room:getTag("NosJiefanTarget"):toPlayer()
				local handang = self.room:findPlayerBySkillName("nosjiefan")
				if self:isFriend(dying) and not self:isEnemy(handang) then return "." end
			end
		end
	end
	if effect and self:hasHeavySlashDamage(target, effect.slash, self.player) then return end
	if not self:damageIsEffective(nil, damage_nature, target) then return "." end
	if target and target:hasSkill("guagu") and self.player:isLord() then return "." end
	if effect and target and target:hasWeapon("IceSword") and self.player:getCards("hes"):length() > 1 then return end
	if self:getDamagedEffects(self.player, target) or self:needToLoseHp() then return "." end

	if target and sgs.ai_role[target:objectName()] == "rebel" and self.role == "rebel" and self.player:hasFlag("AI_doNotSave") then return "." end
	if target and self:needDeath() then return "." end
	if self:needBear() and self.player:getHp() > 2 then return "." end
	if self.player:hasSkill("zili") and not self.player:hasSkill("paiyi") and self.player:getLostHp() < 2 then return "." end

end

--是否该考虑接受method信息。。。有必要区分弃置/使用
--现有的card_lack的添加确认 也存在潜在小问题。。。
function SmartAI:askForCard(pattern, prompt, data)
	local target, target2
	local parsedPrompt = prompt:split(":")
	local players
	if parsedPrompt[2] then
		local players = self.room:getPlayers()
		players = sgs.QList2Table(players)
		for _, player in ipairs(players) do
			if player:getGeneralName() == parsedPrompt[2] or player:objectName() == parsedPrompt[2] then target = player break end
		end
		if parsedPrompt[3] then
			for _, player in ipairs(players) do
				if player:getGeneralName() == parsedPrompt[3] or player:objectName() == parsedPrompt[3] then target2 = player break end
			end
		end
	end
	local arg, arg2 = parsedPrompt[4], parsedPrompt[5]
	local callback = sgs.ai_skill_cardask[parsedPrompt[1]]
	if type(callback) == "function" then
		local ret = callback(self, data, pattern, target, target2, arg, arg2)
		if ret then return ret end
	end

	if data and type(data) == "number" then return end
	local card
	if pattern == "slash" then
		card= sgs.ai_skill_cardask.nullfilter(self, data, pattern, target) or self:getCardId("Slash") or "."
		if card=="." then sgs.card_lack[self.player:objectName()]["Slash"] = 1 end
	elseif pattern == "jink" then
		card= sgs.ai_skill_cardask.nullfilter(self, data, pattern, target) or self:getCardId("Jink") or "."
		if card=="." then sgs.card_lack[self.player:objectName()]["Jink"] = 1 end
	end
	return card
end

function SmartAI:askForUseCard(pattern, prompt, method)
	local use_func = sgs.ai_skill_use[pattern]
	if use_func then
		return use_func(self, prompt, method) or "."
	else
		return "."
	end
end

function SmartAI:askForAG(card_ids, refusable, reason)
	local cardchosen = sgs.ai_skill_askforag[string.gsub(reason, "%-", "_")]
	if type(cardchosen) == "function" then
		local card_id = cardchosen(self, card_ids)
		if card_id then return card_id end
	end
	if refusable and #card_ids==0 then
		return -1
	end
	if refusable and reason == "xinzhan" then
		--local next_player = self.player:getNextAlive()
		local next_player = self.room:findPlayer(self.player:getNextAlive():objectName())
		if self:isFriend(next_player) and next_player:containsTrick("indulgence") and not next_player:containsTrick("YanxiaoCard") then
			if #card_ids == 1 then return -1 end
		end
		for _, card_id in ipairs(card_ids) do
			return card_id
		end
		return -1
	end

	local ids = card_ids
	local cards = {}
	for _, id in ipairs(ids) do
		table.insert(cards, sgs.Sanguosha:getCard(id))
	end
	for _, card in ipairs(cards) do
		if card:isKindOf("Peach") then return card:getEffectiveId() end
	end
	for _, card in ipairs(cards) do
		if card:isKindOf("Indulgence") and not (self:isWeak() and self:getCardsNum("Jink") == 0) then return card:getEffectiveId() end
		if card:isKindOf("AOE") and not (self:isWeak() and self:getCardsNum("Jink") == 0) then return card:getEffectiveId() end
	end
	self:sortByCardNeed(cards)
	return cards[#cards]:getEffectiveId()
end

function SmartAI:askForCardShow(requestor, reason)
	local func = sgs.ai_cardshow[reason]
	if func then
		return func(self, requestor)
	else
		return self.player:getRandomHandCard()
	end
end

function sgs.ai_cardneed.bignumber(to, card, self)
	if not self:willSkipPlayPhase(to) and self:getUseValue(card) < 6 then
		return card:getNumber() > 10
	end
end

function sgs.ai_cardneed.equip(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return card:getTypeId() == sgs.Card_TypeEquip
	end
end

function sgs.ai_cardneed.weapon(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return card:isKindOf("Weapon")
	end
end

function SmartAI:getEnemyNumBySeat(from, to, target, include_neutral)
	target = target or from
	local players = sgs.QList2Table(self.room:getAllPlayers())
	local to_seat = (to:getSeat() - from:getSeat()) % #players
	local enemynum = 0
	for _, p in ipairs(players) do
		if  (self:isEnemy(target, p) or (include_neutral and not self:isFriend(target, p))) and ((p:getSeat() - from:getSeat()) % #players) < to_seat then
			enemynum = enemynum + 1
		end
	end
	return enemynum
end

function SmartAI:getFriendNumBySeat(from, to)
	local players = sgs.QList2Table(self.room:getAllPlayers())
	local to_seat = (to:getSeat() - from:getSeat()) % #players
	local friendnum = 0
	for _, p in ipairs(players) do
		if self:isFriend(from, p) and ((p:getSeat() - from:getSeat()) % #players) < to_seat then
			friendnum = friendnum + 1
		end
	end
	return friendnum
end

--东方杀相关
--self:touhouDamage
function SmartAI:hasHeavySlashDamage(from, slash, to, getValue)
	from = from or self.room:getCurrent()
	slash = slash or self:getCard("Slash", from)--东方函数不能用 问题因该是这里，，，
	to = to or self.player
	if not from or not to then self.room:writeToConsole(debug.traceback()) return false end
	if (to:hasArmorEffect("SilverLion") and not IgnoreArmor(from, to)) then
		if getValue then return 1
		else return false end
	end

	local dmg = 1

	local fireSlash = slash and (slash:isKindOf("FireSlash") or
		(slash:objectName() == "slash" and (from:hasWeapon("Fan") or (from:hasSkill("lihuo") and not self:isWeak(from)))))
	local thunderSlash = slash and slash:isKindOf("ThunderSlash")
	if (slash and slash:hasFlag("drank")) then
		dmg = dmg + 1
	elseif from:getMark("drank") > 0 then
		dmg = dmg + from:getMark("drank")
	end

	if to:hasArmorEffect("Vine") and not IgnoreArmor(from, to) and fireSlash then dmg = dmg + 1 end
	if from:hasWeapon("GudingBlade") and slash and to:isKongcheng() then dmg = dmg + 1 end

	if getValue then return dmg end
	return (dmg > 1)
end

function SmartAI:needKongcheng(player, keep)
	player = player or self.player
	if keep then
		return player:isKongcheng() and (player:hasSkill("kongcheng") or (player:hasSkill("zhiji") and player:getMark("zhiji") == 0))
	end

	if not player:hasFlag("stack_overflow_xiangle") then
		if player:hasSkill("beifa") and not player:isKongcheng() then
			local slash = sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
			for _, to in ipairs(self:getEnemies(player)) do
				if player:canSlash(to, slash) and not self:slashProhibit(slash, to)
				  and self:slashIsEffective(slash, to) and not self:getDamagedEffects(to, player, true)
				  and not self:needToLoseHp(to, player, true, true) then
					return true
				end
			end
		end
	end

	if not self:hasLoseHandcardEffective(player) and not player:isKongcheng() then return true end
	if player:hasSkill("zhiji") and player:getMark("zhiji") == 0 then return true end
	if player:hasSkill("shude") and player:getPhase() == sgs.Player_Play then return true end
	return self:hasSkills(sgs.need_kongcheng, player)
end

--东方杀相关
--【永恒】
function SmartAI:getLeastHandcardNum(player)
	player = player or self.player
	local least = 0
	if player:hasSkill("yongheng") and player:getPhase()==sgs.Player_NotActive then least = 3 end
	if player:hasSkill("lianying") and least < 1 then least = 1 end
	local jwfy = self.room:findPlayerBySkillName("shoucheng")
	if least < 1 and jwfy and self:isFriend(jwfy, player) then least = 1 end
	if player:hasSkill("shangshi") and least < math.min(2, player:getLostHp()) then least = math.min(2, player:getLostHp()) end
	if player:hasSkill("nosshangshi") and least < player:getLostHp() then least = player:getLostHp() end
	return least
end

function SmartAI:hasLoseHandcardEffective(player)
	player = player or self.player
	return player:getHandcardNum() > self:getLeastHandcardNum(player)
end

function SmartAI:hasCrossbowEffect(player)
	player = player or self.player
	return player:hasWeapon("Crossbow") or player:hasSkill("paoxiao")
end

function SmartAI:getCardNeedPlayer(cards, include_self)
	cards = cards or sgs.QList2Table(self.player:getHandcards())

	local cardtogivespecial = {}
	local specialnum = 0
	local keptslash = 0
	local friends={}
	local cmpByAction = function(a,b)
		return a:getRoom():getFront(a, b):objectName() == a:objectName()
	end

	local cmpByNumber = function(a,b)
		return a:getNumber() > b:getNumber()
	end

	local friends_table = include_self and self.friends or self.friends_noself
	for _, player in ipairs(friends_table) do
		if not (player:getPhase()==sgs.Player_NotActive and player:hasSkills("gaoao")) then
			local exclude = self:needKongcheng(player) or self:willSkipPlayPhase(player)
			if self:hasSkills("keji|qiaobian|shensu", player) or player:getHp() - player:getHandcardNum() >= 3
				or (player:isLord() and self:isWeak(player) and self:getEnemyNumBySeat(self.player, player) >= 1) then
				exclude = false
			end
			if self:objectiveLevel(player) <= -2 and not hasManjuanEffect(player) and not exclude then
				table.insert(friends, player)
			end
		end
	end

	local AssistTarget = self:AssistTarget()
	if AssistTarget and (self:needKongcheng(AssistTarget, true) or self:willSkipPlayPhase(AssistTarget) or AssistTarget:hasSkill("manjuan")) then
		AssistTarget = nil
	end

	if self.role ~= "renegade" then
		local R_num = sgs.current_mode_players["renegade"]
		if R_num > 0 and #friends > R_num then
			local k = 0
			local temp_friends, new_friends = {}, {}
			for _, p in ipairs(friends) do
				if k < R_num and sgs.explicit_renegade and sgs.ai_role[p:objectName()] == "renegade" then
					if AssistTarget and p:objectName() == AssistTarget:objectName() then AssistTarget = nil end
					k = k + 1
				else table.insert(temp_friends, p) end
			end
			if k == R_num then friends = temp_friends
			else
				local cmp = function(a, b)
					local ar_value, br_value = sgs.role_evaluation[a:objectName()]["renegade"], sgs.role_evaluation[b:objectName()]["renegade"]
					local al_value, bl_value = sgs.role_evaluation[a:objectName()]["loyalist"], sgs.role_evaluation[b:objectName()]["loyalist"]
					return (ar_value > br_value) or (ar_value == br_value and al_value > bl_value)
				end
				table.sort(temp_friends, cmp)
				for _, p in ipairs(temp_friends) do
					if k < R_num and sgs.role_evaluation[p:objectName()]["renegade"] > 0 then
						k = k + 1
						if AssistTarget and p:objectName() == AssistTarget:objectName() then AssistTarget = nil end
					else table.insert(new_friends, p) end
				end
				friends = new_friends
			end
		end
	end
	-- special move between liubei and xunyu and huatuo
	for _,player in ipairs(friends) do
		if player:hasSkill("jieming") or player:hasSkill("jijiu") then
			specialnum = specialnum + 1
		end
	end
	if specialnum > 1 and #cardtogivespecial == 0 and self.player:hasSkill("nosrende") and self.player:getPhase() == sgs.Player_Play then
		local xunyu = self.room:findPlayerBySkillName("jieming")
		local huatuo = self.room:findPlayerBySkillName("jijiu")
		local no_distance = self.slash_distance_limit
		local redcardnum = 0
		for _,acard in ipairs(cards) do
			if isCard("Slash",acard, self.player) then
				if self.player:canSlash(xunyu, nil, not no_distance) and self:slashIsEffective(acard, xunyu) then
					keptslash = keptslash + 1
				end
				if keptslash > 0 then
					table.insert(cardtogivespecial,acard)
				end
			elseif isCard("Duel",acard, self.player) then
				table.insert(cardtogivespecial,acard)
			end
		end
		for _, hcard in ipairs(cardtogivespecial) do
			if hcard:isRed() then redcardnum = redcardnum + 1 end
		end
		if self.player:getHandcardNum() > #cardtogivespecial and redcardnum > 0 then
			for _, hcard in ipairs(cardtogivespecial) do
				if hcard:isRed() then return hcard, huatuo end
				return hcard, xunyu
			end
		end
	end

	-- keep a jink
	local cardtogive = {}
	local keptjink = 0
	for _,acard in ipairs(cards) do
		if isCard("Jink",acard, self.player) and keptjink < 1 then
			keptjink = keptjink+1
		else
			table.insert(cardtogive,acard)
		end
	end

	-- weak
	self:sort(friends, "defense")
	for _, friend in ipairs(friends) do
		if self:isWeak(friend) and friend:getHandcardNum() < 3  then
			for _, hcard in ipairs(cards) do
				if isCard("Peach",hcard,friend) or (isCard("Jink",hcard,friend) and self:getEnemyNumBySeat(self.player,friend)>0) or isCard("Analeptic",hcard,friend) then
					return hcard, friend
				end
			end
		end
	end

	if (self.player:hasSkill("nosrende") and self.player:isWounded() and self.player:getMark("nosrende") < 2) then
		if (self.player:getHandcardNum() < 2 and self.player:getMark("nosrende") == 0) then return end
	end
	if (self.player:hasSkill("rende") and not self.player:hasUsed("RendeCard") and self.player:isWounded() and self.player:getMark("rende") < 2) then
		if (self.player:getHandcardNum() < 2 and self.player:getMark("rende") == 0) then return end

		if ((self.player:getHandcardNum() == 2 and self.player:getMark("rende") == 0) or
			(self.player:getHandcardNum() == 1 and self.player:getMark("rende") == 1)) and self:getOverflow() <= 0 then

			for _, enemy in ipairs(self.enemies) do
				if enemy:hasWeapon("GudingBlade") and
				(enemy:canSlash(self.player) or enemy:hasSkill("shensu") or enemy:hasSkill("wushen") or enemy:hasSkill("jiangchi")) then return end
			end
		end
	end

	-- Armor,DefensiveHorse
	for _, friend in ipairs(friends) do
		if friend:getHp()<=2 and friend:faceUp() then
			for _, hcard in ipairs(cards) do
				if (hcard:isKindOf("Armor") and not friend:getArmor() and not self:hasSkills("yizhong|bazhen",friend))
							or (hcard:isKindOf("DefensiveHorse") and not friend:getDefensiveHorse()) then
					return hcard, friend
				end
			end
		end
	end

	-- jijiu, jieyin
	self:sortByUseValue(cards, true)
	for _, friend in ipairs(friends) do
		if self:hasSkills("jijiu|jieyin",friend) and friend:getHandcardNum() < 4 then
			for _, hcard in ipairs(cards) do
				if (hcard:isRed() and friend:hasSkill("jijiu")) or friend:hasSkill("jieyin") then
					return hcard, friend
				end
			end
		end
	end

	--Crossbow
	for _, friend in ipairs(friends) do
		if self:hasSkills("longdan|wusheng|keji",friend) and not self:hasSkills("paoxiao",friend) and friend:getHandcardNum() >=2 then
			for _, hcard in ipairs(cards) do
				if hcard:isKindOf("Crossbow") then
					return hcard, friend
				end
			end
		end
	end

	for _, friend in ipairs(friends) do
		if getKnownCard(friend, self.player, "Crossbow") > 0 then
			for _, p in sgs.qlist(self.room:getOtherPlayers(friend)) do
				if self:isEnemy(p) and sgs.isGoodTarget(p, self.enemies, self) and friend:distanceTo(p) <= 1 then
					for _, hcard in ipairs(cards) do
						if isCard("Slash", hcard, friend) then
							return hcard, friend
						end
					end
				end
			end
		end
	end

	table.sort(friends, cmpByAction)

	for _, friend in ipairs(friends) do
		if friend:faceUp() then
			local can_slash = false
			for _, p in sgs.qlist(self.room:getOtherPlayers(friend)) do
				if self:isEnemy(p) and sgs.isGoodTarget(p, self.enemies, self) and friend:distanceTo(p) <= friend:getAttackRange() then
					can_slash = true
					break
				end
			end
			local flag =string.format("weapon_done_%s_%s",self.player:objectName(),friend:objectName())
			if not can_slash then
				for _, p in sgs.qlist(self.room:getOtherPlayers(friend)) do
					if self:isEnemy(p) and sgs.isGoodTarget(p, self.enemies, self) and friend:distanceTo(p) > friend:getAttackRange() then
						for _, hcard in ipairs(cardtogive) do
							if hcard:isKindOf("Weapon") and friend:distanceTo(p) <= friend:getAttackRange() + (sgs.weapon_range[hcard:getClassName()] or 0)
									and not friend:getWeapon() and not friend:hasFlag(flag) then
								self.room:setPlayerFlag(friend, flag)
								return hcard, friend
							end
							if hcard:isKindOf("OffensiveHorse") and friend:distanceTo(p) <= friend:getAttackRange() + 1
									and not friend:getOffensiveHorse() and not friend:hasFlag(flag) then
								self.room:setPlayerFlag(friend, flag)
								return hcard, friend
							end
						end
					end
				end
			end

		end
	end


	table.sort(cardtogive, cmpByNumber)

	for _, friend in ipairs(friends) do
		if not self:needKongcheng(friend, true) and friend:faceUp() then
			for _, hcard in ipairs(cardtogive) do
				for _, askill in sgs.qlist(friend:getVisibleSkillList()) do
					local callback = sgs.ai_cardneed[askill:objectName()]
					if type(callback)=="function" and callback(friend, hcard, self) then
						return hcard, friend
					end
				end
			end
		end
	end


	-- slash
	if self.role == "lord" and self.player:hasLordSkill("jijiang") then
		for _, friend in ipairs(friends) do
			if friend:getKingdom() == "shu" and friend:getHandcardNum() < 3  then
				for _, hcard in ipairs(cardtogive) do
					if isCard("Slash", hcard, friend) then
						return hcard, friend
					end
				end
			end
		end
	end


	-- kongcheng
	self:sort(self.enemies, "defense")
	if #self.enemies > 0 and self.enemies[1]:isKongcheng() and self.enemies[1]:hasSkill("kongcheng")
		and not hasManjuanEffect(self.enemies[1]) then
		for _, acard in ipairs(cardtogive) do
			if acard:isKindOf("Lightning") or acard:isKindOf("Collateral") or (acard:isKindOf("Slash") and self.player:getPhase() == sgs.Player_Play)
				or acard:isKindOf("OffensiveHorse") or acard:isKindOf("Weapon") or acard:isKindOf("AmazingGrace") then
				return acard, self.enemies[1]
			end
		end
	end

	if AssistTarget then
		for _, hcard in ipairs(cardtogive) do
			return hcard, AssistTarget
		end
	end

	self:sort(friends, "defense")
	for _, hcard in ipairs(cardtogive) do
		for _, friend in ipairs(friends) do
			if not self:needKongcheng(friend, true) and not friend:hasSkill("manjuan") and not self:willSkipPlayPhase(friend)
					and (self:hasSkills(sgs.priority_skill,friend) or (sgs.ai_chaofeng[self.player:getGeneralName()] or 0) > 2) then
				if (self:getOverflow() > 0 or self.player:getHandcardNum() > 3) and friend:getHandcardNum() <= 3 then
					return hcard, friend
				end
			end
		end
	end

	local shoulduse = self.player:isWounded() and (self.player:hasSkill("rende") and not self.player:hasUsed("RendeCard") and self.player:getMark("rende") < 2)
					or (self.player:hasSkill("nosrende") and self.player:getMark("nosrende") < 2)

	if #cardtogive == 0 and shoulduse then cardtogive = cards end

	self:sort(friends, "handcard")
	for _, hcard in ipairs(cardtogive) do
		for _, friend in ipairs(friends) do
			if not self:needKongcheng(friend, true) and not friend:hasSkill("manjuan") then
				if friend:getHandcardNum() <= 3 and (self:getOverflow() > 0 or self.player:getHandcardNum() > 3 or shoulduse) then
					return hcard, friend
				end
			end
		end
	end


	for _, hcard in ipairs(cardtogive) do
		for _, friend in ipairs(friends) do
			if (not self:needKongcheng(friend, true) or #friends == 1) and not friend:hasSkill("manjuan") then
				if self:getOverflow() > 0 or self.player:getHandcardNum() > 3 or shoulduse then
					return hcard, friend
				end
			end
		end
	end

	for _, hcard in ipairs(cardtogive) do
		for _, friend in ipairs(friends_table) do
			if (not self:needKongcheng(friend, true) or #friends_table == 1) and not friend:hasSkill("manjuan") then
				if self:getOverflow() > 0 or self.player:getHandcardNum() > 3 or shoulduse then
					return hcard, friend
				end
			end
		end
	end

	if #cards > 0 and shoulduse then
		local need_rende = (sgs.current_mode_players["rebel"] ==0 and sgs.current_mode_players["loyalist"] > 0 and self.player:isWounded()) or
				(sgs.current_mode_players["rebel"] >0 and sgs.current_mode_players["renegade"] >0 and sgs.current_mode_players["loyalist"] ==0 and self:isWeak())
		if need_rende then
			local players=sgs.QList2Table(self.room:getOtherPlayers(self.player))
			self:sort(players,"defense")
			self:sortByUseValue(cards, true)
			return cards[1], players[1]
		end
	end

end

function SmartAI:askForYiji(card_ids, reason)

	if reason then
		local callback = sgs.ai_skill_askforyiji[string.gsub(reason,"%-","_")]
		if type(callback) == "function" then
			local target, cardid = callback(self, card_ids)
			if target and cardid then return target, cardid end
		end
	end
	return nil, -1
end

function SmartAI:askForPindian(requestor, reason)
	local passive = { "mizhao", "lieren" }
	if self.player:objectName() == requestor:objectName() and not table.contains(passive, reason) then
		if self[reason .. "_card"] then
			return sgs.Sanguosha:getCard(self[reason .. "_card"])
		else
			self.room:writeToConsole("Pindian card for " .. reason .. " not found!!")
			return self:getMaxCard(self.player):getId()
		end
	end
	local cards = sgs.QList2Table(self.player:getHandcards())
	local compare_func = function(a, b)
		return a:getNumber() < b:getNumber()
	end
	table.sort(cards, compare_func)
	local maxcard, mincard, minusecard
	for _, card in ipairs(cards) do
		if self:getUseValue(card) < 6 then mincard = card break end
	end
	for _, card in ipairs(sgs.reverse(cards)) do
		if self:getUseValue(card) < 6 then maxcard = card break end
	end
	self:sortByUseValue(cards, true)
	minusecard = cards[1]
	maxcard = maxcard or minusecard
	mincard = mincard or minusecard

	local sameclass, c1 = true
	for _, c2 in ipairs(cards) do
		if not c1 then c1 = c2
		elseif c1:getClassName() ~= c2:getClassName() then sameclass = false end
	end
	if sameclass then
		if self:isFriend(requestor) then return self:getMinCard()
		else return self:getMaxCard() end
	end

	local callback = sgs.ai_skill_pindian[reason]
	if type(callback) == "function" then
		local ret = callback(minusecard, self, requestor, maxcard, mincard)
		if ret then return ret end
	end
	if self:isFriend(requestor) then return mincard else return maxcard end
end

sgs.ai_skill_playerchosen.damage = function(self, targets)
	local targetlist = sgs.QList2Table(targets)
	self:sort(targetlist, "hp")
	for _, target in ipairs(targetlist) do
		if self:isEnemy(target) then return target end
	end
	return targetlist[#targetlist]
end

function SmartAI:askForPlayerChosen(targets, reason)
	local playerchosen = sgs.ai_skill_playerchosen[string.gsub(reason, "%-", "_")]
	local target = nil
	if type(playerchosen) == "function" then
		target = playerchosen(self, targets)
		return target
	end
	local r = math.random(0, targets:length() - 1)
	return targets:at(r)
end


function SmartAI:ableToSave(saver, dying)
	if dying:hasSkill("skltkexue") and saver:getHp()>1 then return true end
	local current = self.room:getCurrent()
	if current and current:getPhase() ~= sgs.Player_NotActive and current:hasSkill("wansha")
		and current:objectName() ~= saver:objectName() and current:objectName() ~= dying:objectName()
		and not saver:hasSkills("jiuzhu|chunlao|nosjiefan|renxin") then

		return false
	end


	local peach = sgs.cloneCard("peach", sgs.Card_NoSuitRed, 0)

	if (saver:isCardLimited(peach, sgs.Card_MethodUse, true) or  self.room:isProhibited(saver, dying, peach))
	  and not saver:hasSkills("jiuzhu|chunlao|nosjiefan|renxin") then return false end

	return true
end

----东方杀相关
--【威压】
function SmartAI:willUsePeachTo(dying)
	--不是简单的friend啊。。。 比如反贼劣势 有人濒死求桃  但是你麻痹内得看求桃的是谁啊  还有对面有几个桃啊
	--对面一把桃子时 你一个内先出桃是个毛意思啊
	local card_str
	local forbid = sgs.cloneCard("peach")

	if self.player:isLocked(forbid) or dying:isLocked(forbid)  or self.room:isProhibited(self.player, dying, forbid)  then 
		if not self:ableToSave(self.player, dying) then --【远吠】的问题
			return "."
		end
	end
	if self.player:objectName() == dying:objectName() and not self:needDeath(dying) then
		local analeptic = sgs.cloneCard("analeptic")
		if not self.player:isLocked(analeptic) and self:getCardId("Analeptic") then return self:getCardId("Analeptic") end
		if self:getCardId("Peach") then return self:getCardId("Peach") end
	end

	--宴会用酒
	if self.player:objectName() ~= dying:objectName() and self:isFriend(dying)
	and  dying:hasLordSkill("yanhui") and self.player:getKingdom() == "zhan"
	and not self:needDeath(dying) then
		local analeptic = sgs.cloneCard("analeptic")
		if not self.player:isLocked(analeptic) and self:getCardId("Analeptic") then return self:getCardId("Analeptic") end
	end

	--物语必须救
	local lord_mouko = getLord(self.player)
	if lord_mouko and lord_mouko:hasLordSkill("tymhwuyu") then
		local wuyu_save=false
		if sgs.ai_role[dying:objectName()] == "rebel"  and self.role == "rebel" then
			wuyu_save=true
		end
		if sgs.ai_role[dying:objectName()] == "loyalist"  and (self.role == "loyalist" or self.role == "lord") then
			wuyu_save=true
		end
		if wuyu_save then
			if self.player:objectName() == dying:objectName() then
				local analeptic = sgs.cloneCard("analeptic")
				if not self.player:isLocked(analeptic) and self:getCardId("Analeptic") then return self:getCardId("Analeptic") end
			end
			if self:getCardId("Peach") then return self:getCardId("Peach") end
		end
	end

	if not sgs.GetConfig("EnableHegemony", false) and self.room:getMode() ~= "couple" and (self.role == "loyalist" or self.role == "renegade") and dying:isLord() and self.player:aliveCount() > 2 then
		return self:getCardId("Peach")
	end
	if self:hasWeiya() then
		if self:getCardsNum("Peach") < 2 then return "." end
	end
	if not sgs.GetConfig("EnableHegemony", false) and self.role == "renegade" and not (dying:isLord() or dying:objectName() == self.player:objectName())
		and (sgs.current_mode_players["loyalist"] + 1 == sgs.current_mode_players["rebel"]
				or sgs.current_mode_players["loyalist"] == sgs.current_mode_players["rebel"]
				or self.room:getCurrent():objectName() == self.player:objectName()
				or sgs.gameProcess(self.room) == "neutral")
			then
		return "."
	end

	if isLord(self.player) and dying:objectName() ~= self.player:objectName() and self:getEnemyNumBySeat(self.room:getCurrent(), self.player, self.player) > 0 and
		self:getCardsNum("Peach") == 1 and self:isWeak() and self.player:getHp() == 1 then return "." end

	if sgs.ai_role[dying:objectName()] == "renegade" and dying:objectName() ~= self.player:objectName() then
		if self.role == "loyalist" or self.role == "lord" or self.role == "renegade" then
			if sgs.current_mode_players["loyalist"] + sgs.current_mode_players["renegade"] >= sgs.current_mode_players["rebel"] then return "."
			elseif sgs.gameProcess(self.room) == "loyalist" or sgs.gameProcess(self.room) == "loyalish" or sgs.gameProcess(self.room) == "dilemma" then return "."
			end
		end
		if self.role == "rebel" or self.role == "renegade" then
			if sgs.current_mode_players["rebel"] + sgs.current_mode_players["renegade"] - 1 >= sgs.current_mode_players["loyalist"] + 1 then return "."
			elseif sgs.gameProcess(self.room) == "rebelish" or sgs.gameProcess(self.room) == "rebel" or sgs.gameProcess(self.room) == "dilemma" then return "."
			end
		end
	end

	if self.role == "renegade" then  --一个反都没有死，内奸凭什么救反。 哪怕 反贼确实“劣势”， 反贼被判为friend  关键是救完自己又不收人头
		local no_rebel_die = true
		if sgs.ai_role[dying:objectName()] == "rebel" then
			for _,p in sgs.qlist(self.room:getAllPlayers(true)) do
				if p:isDead() and p:getRole() == rebel then
					no_rebel_die =false
				end
			end
			if no_rebel_die then
			--sgs.current_mode_players["rebel"] + sgs.current_mode_players["renegade"] - 1 >= sgs.current_mode_players["loyalist"] + 1 then
				return "."
			end
		end
	end


	if self:isFriend(dying) then
		if self:needDeath(dying) then return "." end

		local lord = getLord(self.player)
		if not sgs.GetConfig("EnableHegemony", false) and self.player:objectName() ~= dying:objectName() and not dying:isLord() and
		(self.role == "loyalist" or self.role == "renegade" and self.room:alivePlayerCount() > 2) and
			((self:getCardsNum("Peach") <= sgs.ai_NeedPeach[lord:objectName()]) or
			(sgs.ai_lord_in_danger_SA and lord and getCardsNum("Slash", lord, self.player) < 1 and self:getCardsNum("Peach") < 2) or
			(sgs.ai_lord_in_danger_AA and lord and getCardsNum("Jink", lord, self.player) < 1 and self:getCardsNum("Peach") < 2)) then
			return "."
		end

		if self:getCardsNum("Peach") + self:getCardsNum("Analeptic") <= sgs.ai_NeedPeach[self.player:objectName()] and not isLord(dying) then return "." end

		if math.ceil(self:getAllPeachNum()) < dying:dyingThreshold() - dying:getHp() and not isLord(dying) then return "." end

		if not dying:isLord() and dying:objectName() ~= self.player:objectName() then
			local possible_friend = 0
			for _, friend in ipairs(self.friends_noself) do
				if (self:getKnownNum(friend) == friend:getHandcardNum() and getCardsNum("Peach", friend, self.player) == 0)
					or (self:playerGetRound(friend) < self:playerGetRound(self.player)) then
				elseif sgs.card_lack[friend:objectName()]["Peach"] == 1 then
				elseif not self:ableToSave(friend, dying) then
				elseif friend:getHandcardNum() > 0 or getCardsNum("Peach", friend, self.player) > 0 then
					possible_friend = possible_friend + 1
				end
			end
			if possible_friend == 0 and self:getCardsNum("Peach") < 1 - dying:getHp() then
				return "."
			end
		end

		local CP = self.room:getCurrent()
		if lord then
			if dying:objectName() ~= lord:objectName() and dying:objectName() ~= self.player:objectName() and lord:getHp() == 1 and
				self:isFriend(lord) and self:isEnemy(CP) and CP:canSlash(lord, nil, true) and getCardsNum("Peach", lord, self.player) < 1 and
				getCardsNum("Analeptic", lord, self.player) < 1 and #self.friends_noself <= 2 and self:slashIsAvailable(CP) and
				self:damageIsEffective(CP, nil, lord) and self:getCardsNum("Peach") <= self:getEnemyNumBySeat(CP, lord, self.player) + 1 then
				return "."
			end
		end

		local buqu = dying:getPile("buqu")
		local weaklord = 0
		if not buqu:isEmpty() then
			local same = false
			for i, card_id in sgs.qlist(buqu) do
				for j, card_id2 in sgs.qlist(buqu) do
					if i ~= j and sgs.Sanguosha:getCard(card_id):getNumber() == sgs.Sanguosha:getCard(card_id2):getNumber() then
						same = true
						break
					end
				end
			end
			if not same then return "." end
		end
		if dying:hasFlag("Kurou_toDie") and (not dying:getWeapon() or dying:getWeapon():objectName()~="Crossbow") then return "." end
		if self.player:objectName() ~= dying:objectName() and dying:hasSkill("jiushi") and dying:faceUp() and dying:getHp()== 0 then
			return "."
		end

		if (self.player:objectName() == dying:objectName()) then
			card_str = self:getCardId("Analeptic")
			if not card_str then
			 card_str = self:getCardId("Peach") end
		elseif dying:isLord() then
			card_str = self:getCardId("Peach")
		elseif self:doNotSave(dying) then return "."
		else
			for _, friend in ipairs(self.friends_noself) do
				if friend:getHp() == 1 and friend:isLord() and not friend:hasSkill("buqu") then  weaklord = weaklord + 1 end
			end
			for _, enemy in ipairs(self.enemies) do
				if enemy:getHp() == 1 and enemy:isLord() and not enemy:hasSkill("buqu") and self.player:getRole() == "renegade" then weaklord = weaklord + 1 end
			end
			if weaklord < 1 or self:getAllPeachNum() > 1 then
				card_str = self:getCardId("Peach")
			end
		end
	else --救对方的情形
		--[[if dying:hasSkill("wuhun") then --濒死者有技能“武魂”
			if not sgs.GetConfig("EnableHegemony", false) then
				local should = self.role == "renegade" and self.room:alivePlayerCount() > 2
								or (self.role == "lord" or self.role == "loyalist") and sgs.current_mode_players["rebel"] + sgs.current_mode_players["renegade"] > 1
				if should then --可能有救的必要
					local willKillLord = false
					local revengeTargets = self:getWuhunRevengeTargets() --武魂复仇目标
					if #revengeTargets > 0 then
						local lord = getLord(self.player)
						if lord then
							for _,target in pairs(revengeTargets) do
								if target:objectName() == lord:objectName() then
									willKillLord = true
									break
								end
							end
						end
					end
					if willKillLord then --主公会被武魂带走，真的有必要……
						local finalRetrial, wizard = self:getFinalRetrial(self.room:getCurrent(), "wuhun")
						if finalRetrial == 0 then --没有判官，需要考虑观星、心战、攻心的结果（已忽略）
							card_str = self:getCardId("Peach")
						elseif finalRetrial == 1 then --己方后判，需要考虑最后的判官是否有桃或桃园结义改判（已忽略）
							local flag = wizard:hasSkill("huanshi") and "hes" or "hs"
							if getKnownCard(wizard, self.player, "Peach", false, flag) > 0 or getKnownCard(wizard, self.player, "GodSalvation", false, flag) > 0 then return "." end
							card_str = self:getCardId("Peach")
						elseif finalRetrial == 2 then --对方后判，这个一定要救了……
							card_str = self:getCardId("Peach")
						end
					end
				end
			end
		end]]
	end
	if not card_str then return nil end
	return card_str
end

function SmartAI:askForSinglePeach(dying)
	local card_str = self:willUsePeachTo(dying)

	return card_str or "."
end

function SmartAI:getTurnUse()
	local cards = self.player:getHandcards()
	cards=self:touhouAppendExpandPileToList(self.player,cards)
	cards = sgs.QList2Table(cards)


	local turnUse = {}
	local slash = sgs.cloneCard("slash")
	local slashAvail = 1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, self.player, slash)
	self.slashAvail = slashAvail
	self.predictedRange = self.player:getAttackRange()
	self.slash_distance_limit = (1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_DistanceLimit, self.player, slash) > 50)

	self.weaponUsed = false
	self:fillSkillCards(cards)
	self:sortByUseValue(cards)

	if self.player:hasWeapon("Crossbow") or #self.player:property("extra_slash_specific_assignee"):toString():split("+") > 1 then
		slashAvail = 100
		self.slashAvail = slashAvail
	elseif self.player:hasWeapon("VSCrossbow") then
		slashAvail = slashAvail + 3
		self.slashAvail = slashAvail
	end
	for _, card in ipairs(cards) do
		local dummy_use = { isDummy = true }

		local type = card:getTypeId()
		self["use" .. sgs.ai_type_name[type + 1] .. "Card"](self, card, dummy_use)
		if dummy_use.card then
			if dummy_use.card:isKindOf("Slash") then
				if  dummy_use.card:hasFlag("jiuhao") or dummy_use.card:getSkillName() == "duzhua" then
					slashAvail = slashAvail+1
				end
				if slashAvail > 0 then
					slashAvail = slashAvail - 1
					table.insert(turnUse, dummy_use.card)
				elseif dummy_use.card:hasFlag("AIGlobal_KillOff") then table.insert(turnUse, dummy_use.card) end
			else
				if self.player:hasFlag("InfinityAttackRange") or self.player:getMark("InfinityAttackRange") > 0 then
					self.predictedRange = 10000
				elseif dummy_use.card:isKindOf("Weapon") then
					self.predictedRange = sgs.weapon_range[card:getClassName()]
					self.weaponUsed = true
				else
					self.predictedRange = 1
				end
				if dummy_use.card:objectName() == "Crossbow" then slashAvail = 100 self.slashAvail = slashAvail end
				if dummy_use.card:objectName() == "VSCrossbow" then slashAvail = slashAvail + 3 self.slashAvail = slashAvail end
				table.insert(turnUse, dummy_use.card)
			end
		end
	end

	return turnUse
end

function SmartAI:assignKeepNum()
	local num = self.player:getMaxCards()
	if self.player:hasSkill("qiaobian") then num = math.max(self.player:getHandcardNum() - 1, num) end
	if self.player:hasSkill("keji") then num = self.player:getHandcardNum() end
	if self.player:hasSkill("zaoyao") then num = self.player:getHandcardNum() end
	return num
end

function SmartAI:activate(use)
	self:updatePlayers()
	self:assignKeep(nil, true)
	self.toUse = self:getTurnUse()
	self:sortByDynamicUsePriority(self.toUse)
	for _, card in ipairs(self.toUse) do
		if not self.player:isCardLimited(card, card:getHandlingMethod())
			or (card:canRecast() and not self.player:isCardLimited(card, sgs.Card_MethodRecast)) then
			local type = card:getTypeId()
			self["use" .. sgs.ai_type_name[type + 1] .. "Card"](self, card, use)

			if use:isValid(nil) then
				self.toUse = nil
				return
			end
		end
	end
	self.toUse = nil
end

function SmartAI:getOverflow(player, getMaxCards)
	player = player or self.player
	local kingdom_num = 0
	if player:hasSkill("yongsi") and player:getPhase() ~= sgs.Player_NotActive and player:getPhase() ~= sgs.Player_Finish
		and not (player:hasSkill("keji") and not player:hasFlag("KejiSlashInPlayPhase")) and not player:hasSkill("conghui") then
			local kingdoms = {}
			for _, ap in sgs.qlist(self.room:getAlivePlayers()) do
				if not kingdoms[ap:getKingdom()] then
					kingdoms[ap:getKingdom()] = true
					kingdom_num = kingdom_num + 1
				end
			end
	end

	local MaxCards = player:getMaxCards()
	if player:hasSkill("qiaobian") then MaxCards = math.max(self.player:getHandcardNum() - 1, MaxCards) end
	if player:hasSkill("keji") and not player:hasFlag("KejiSlashInPlayPhase") then MaxCards = self.player:getHandcardNum() end
	if player:hasSkill("zaoyao") then MaxCards = self.player:getHandcardNum() end
	if getMaxCards and MaxCards > 0 then return MaxCards end
	if kingdom_num > 0 then
		if player:getCardCount(true) <= kingdom_num then MaxCards = 0
		else MaxCards = math.min(player:getMaxCards(), player:getCardCount(true) - kingdom_num)
		end
		if getMaxCards then return MaxCards end
	end
	if getMaxCards then return player:getMaxCards() end

	return player:getHandcardNum() - MaxCards
end

--重新定义一下东方杀里的weak概念
--东方杀相关
--【渴血】【具现】【不灭】【神德】【幻梦】主旨是队友不要傻傻的为这几个人留桃
--【禁果】则相反
function SmartAI:isWeak(player)
	player = player or self.player
	local hcard = player:getHandcardNum()
	if player:hasSkill("juxian") and player:faceUp() then return false end
	if player:hasSkill("skltkexue") then
		if self:isEnemy(player) then
			for _,p in pairs(self.enemies) do
				if p:getHp()>=2 then
					return false
				end
			end
		end
		if self:isFriend(player) then
			for _,p in pairs(self.friends) do
				if p:getHp()>=2 then
					return false
				end
			end
		end
	end
	if player:hasSkill("bumie") then return false end
	if player:hasSkill("huanmeng") then return false end
	if player:hasSkill("shende") and player:getPile("shende"):length()>=4 then return false end
	if player:hasSkill("jinguo") and player:getMark("@kinki")>=4 then return true end
	if hasBuquEffect(player) then return false end
	if player:hasSkill("hualong") and player:getMark("hualong") == 0 then return false end
	if (player:getHp() <= 2 and hcard <= 2) or player:getHp() <= 1 then return true end
	return false
end

function SmartAI:useCardByClassName(card, use)
	if not card then global_room:writeToConsole(debug.traceback()) return end
	local class_name = card:getClassName()
	local use_func = self["useCard" .. class_name]

	if use_func then
		use_func(self, card, use)
	end
end

function SmartAI:hasWizard(players,onlyharm)
	local skill
	if onlyharm then skill = sgs.wizard_harm_skill else skill = sgs.wizard_skill end
	for _, player in ipairs(players) do
		if self:hasSkills(skill, player) then
			return true
		end
	end
end

--东方杀相关
--【命运】【绯想】
function SmartAI:canRetrial(player, to_retrial, reason)
	player = player or self.player
	to_retrial = to_retrial or self.player
	if player:hasSkill("guidao") and not (reason and reason == "wuhun") then
		local blackequipnum = 0
		for _, equip in sgs.qlist(player:getEquips()) do
			if equip:isBlack() then blackequipnum = blackequipnum + 1 end
		end
		if blackequipnum + player:getHandcardNum() > 0 then return true end
	end
	if player:hasSkill("guicai") and player:getHandcardNum() > 0 then return true end
	--if player:hasSkill("huanshi") and not player:isNude() then return true end
	if player:hasSkill("jilve") and player:getHandcardNum() > 0 and player:getMark("@bear") > 0 then return true end
	if player:hasSkills("mingyun|feixiang|fengshui") then return true end
	--博丽的改判略微妙
	--if player:hasLordSkill("boli") then return true end
	return
end

--东方杀相关
--【命运】
function SmartAI:getFinalRetrial(player, reason)
	local maxfriendseat = -1
	local maxenemyseat = -1
	local maxneturalseat = -1
	local tmpfriend
	local tmpenemy
	local tmpnetural
	local wizardf, wizarde, wizardn
	player = player or self.room:getCurrent()
	--命运必然是第一个改判
	local sklt = self.room:findPlayerBySkillName("mingyun")
	if sklt then
		if self:isFriend(sklt) then
			wizardf = sklt
		elseif self:isEnemy(sklt) then
			wizarde = sklt
		else
			wizardn = sklt
		end
	end
	for _, aplayer in sgs.qlist(self.room:getAlivePlayers()) do
		if self:hasSkills(sgs.wizard_harm_skill , aplayer) and self:canRetrial(aplayer, player, reason) then
			if not aplayer:hasSkill("mingyun") then  
				if self:isFriend(aplayer) then
					tmpfriend = (aplayer:getSeat() - player:getSeat()) % (global_room:alivePlayerCount())
					if tmpfriend > maxfriendseat then
						maxfriendseat = tmpfriend
						wizardf = aplayer
					end
				elseif self:isEnemy(aplayer) then
					tmpenemy = (aplayer:getSeat() - player:getSeat()) % (global_room:alivePlayerCount())
					if tmpenemy > maxenemyseat then
						maxenemyseat = tmpenemy
						wizarde = aplayer
					end
				else
					tmpnetural = (aplayer:getSeat() - player:getSeat()) % (global_room:alivePlayerCount())
					if tmpnetural > maxneturalseat then
						maxneturalseat = tmpnetural
						wizardn = aplayer
					end
				end
			end
		end
	end
	--[[for _, aplayer in ipairs(self.friends) do
		if self:hasSkills(sgs.wizard_harm_skill .. "|huanshi", aplayer) and self:canRetrial(aplayer, player, reason) then
			tmpfriend = (aplayer:getSeat() - player:getSeat()) % (global_room:alivePlayerCount())
			if tmpfriend > maxfriendseat then
				maxfriendseat = tmpfriend
				wizardf = aplayer
			end
		end
	end
	for _, aplayer in ipairs(self.enemies) do
		if self:hasSkills(sgs.wizard_harm_skill .. "|huanshi", aplayer) and self:canRetrial(aplayer, player, reason) then
			tmpenemy = (aplayer:getSeat() - player:getSeat()) % (global_room:alivePlayerCount())
			if tmpenemy > maxenemyseat then
				maxenemyseat = tmpenemy
				wizarde = aplayer
			end
		end
	end]]
	if not wizardf  and not wizarde and not wizardn then return 0, nil
	elseif wizardn and maxfriendseat<=maxneturalseat and  maxenemyseat<=maxneturalseat then
		return 0, wizardn
	elseif wizardf and maxfriendseat>=maxneturalseat and maxenemyseat<=maxfriendseat then
		return 1, wizardf
	elseif wizarde and maxenemyseat>=maxneturalseat and maxenemyseat>=maxfriendseat then
		return 2, wizarde
	end

	--[[if maxfriendseat == -1 and maxenemyseat == -1 then return 0, nil
	elseif maxfriendseat > maxenemyseat then return 1, wizardf
	else return 2, wizarde end]]
end

--- Determine that the current judge is worthy retrial
-- @param judge The JudgeStruct that contains the judge information
-- @return True if it is needed to retrial
--东方杀相关
--【红白】【静电】【冰魄】
function SmartAI:needRetrial(judge)
	local reason = judge.reason
	local lord = getLord(self.player)
	local who = judge.who
	if reason == "lightning" then
		if who:hasSkills("wuyan|hongyan|bingpo") then return false end

		if lord and (who:isLord() or (who:isChained() and lord:isChained())) and self:objectiveLevel(lord) <= 3 then
			if lord:hasArmorEffect("SilverLion") and lord:getHp() >= 2 and self:isGoodChainTarget(lord, self.player, sgs.DamageStruct_Thunder) then return false end
			return self:damageIsEffective(lord, sgs.DamageStruct_Thunder) and not judge:isGood()
		end

		if who:hasArmorEffect("SilverLion") and who:getHp() > 1 then return false end

		if self:isFriend(who) then
			if who:isChained() and self:isGoodChainTarget(who, self.player, sgs.DamageStruct_Thunder, 3) then return false end
		else
			if who:isChained() and not self:isGoodChainTarget(who, self.player, sgs.DamageStruct_Thunder, 3) then return judge:isGood() end
		end
	end

	if reason == "indulgence" then
		if who:isSkipped(sgs.Player_Draw) and who:isKongcheng() then
			if (who:hasSkill("shenfen") and who:getMark("@wrath") >= 6)
				or (who:hasSkill("kurou") and who:getHp() >= 3)
				or (who:hasSkill("jixi") and who:getPile("field"):length() > 2)
				--or (who:hasSkill("lihun") and self:isLihunTarget(self:getEnemies(who), 0))
				or (who:hasSkill("xiongyi") and who:getMark("@arise") > 0) then
				if self:isFriend(who) then
					return not judge:isGood()
				else
					return judge:isGood()
				end
			end
		end
		if self:isFriend(who) then
			local drawcardnum = self:ImitateResult_DrawNCards(who, who:getVisibleSkillList())
			if who:getHp() - who:getHandcardNum() >= drawcardnum and self:getOverflow() < 0 then return false end
			if who:hasSkill("tuxi") and who:getHp() > 2 and self:getOverflow() < 0 then return false end
			return not judge:isGood()
		else
			return judge:isGood()
		end
	end

	if reason == "supply_shortage" then
		if self:isFriend(who) then
			if self:hasSkills("guidao|tiandu",who) then return false end
			return not judge:isGood()
		else
			return judge:isGood()
		end
	end

	if reason == "luoshen" then
		if self:isFriend(who) then
			if who:getHandcardNum() > 30 then return false end
			if self:hasCrossbowEffect(who) or getKnownCard(who, self.player, "Crossbow", false) > 0 then return not judge:isGood() end
			if self:getOverflow(who) > 1 and self.player:getHandcardNum() < 3 then return false end
			return not judge:isGood()
		else
			return judge:isGood()
		end
	end

	if reason == "tuntian" then
		if not who:hasSkill("zaoxian") and who:getMark("zaoxian") == 0 then return false end
	end

	if reason == "beige" then return true end

	if reason=="lingqi" then
		--judge.card:isRed()
		local lingqi_type=self:lingqiParse(self,who)
		if lingqi_type==2 then
			if self:isFriend(who) then
				return judge.card:getSuit() ~= sgs.Card_Heart
			elseif self:isEnemy(who) then
				return judge.card:getSuit() == sgs.Card_Heart
			else
				return false
			end
		end
		if lingqi_type==1 then
			if self:isFriend(who) then
				return judge.card:getSuit() == sgs.Card_Heart
			elseif self:isEnemy(who) then
				return judge.card:getSuit() ~= sgs.Card_Heart
			else
				return false
			end
		end
	end

	if self:isFriend(who) then
		return not judge:isGood()
	elseif self:isEnemy(who) then
		return judge:isGood()
	else
		return false
	end
end

--- Get the retrial cards with the lowest keep value
-- @param cards the table that contains all cards can use in retrial skill
-- @param judge the JudgeStruct that contains the judge information
-- @return the retrial card id or -1 if not found
--东方杀相关
--add new factor :need_reverse
function SmartAI:getRetrialCardId(cards, judge, self_card)
	if self_card == nil then self_card = true end
	local can_use = {}
	local reason = judge.reason
	local who = judge.who
	local need_reverse=false
	local function judgeIsGood(isGood,need_reverse)
		if need_reverse then
			return not isGood
		else
			return  isGood
		end
	end

	local other_suit, hasSpade = {}
	for _, card in ipairs(cards) do
		local card_x = sgs.Sanguosha:getEngineCard(card:getEffectiveId())
		if who:hasSkill("hongyan") and card_x:getSuit() == sgs.Card_Spade then
			card_x = sgs.cloneCard(card_x:objectName(), sgs.Card_Heart, card:getNumber())
		end

		if who:hasSkill("bendan") then
			card_x = sgs.cloneCard(card_x:objectName(), card_x:getSuit(), 9)
		end

		--静电 和 破坏 有逆转judgeisgood的时候
		if reason == "lightning" and who:hasSkill("jingdian") then
			need_reverse = true
		elseif reason == "pohuai" and self:isFriend(who) then
			local need_pohuai=false
			if self:pohuaiBenefit(who)>0 then
				need_pohuai=true
			elseif not need_pohuai and who:hasSkill("yuxue") then
				local callback=sgs.ai_need_damaged["yuxue"]
				if callback(self, who, who) then
					 need_pohuai=true
				end
			end
			need_reverse = not need_pohuai
		end

		if reason == "beige" and not isCard("Peach", card_x, self.player) then
			local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
			if damage.from then
				if self:isFriend(damage.from) then
					if not self:toTurnOver(damage.from, 0) and judge.card:getSuit() ~= sgs.Card_Spade and card_x:getSuit() == sgs.Card_Spade then
						table.insert(can_use, card)
						hasSpade = true
					elseif (not self_card or self:getOverflow() > 0) and judge.card:getSuit() ~= card_x:getSuit() then
						local retr = true
						if (judge.card:getSuit() == sgs.Card_Heart and who:isWounded() and self:isFriend(who))
							or (judge.card:getSuit() == sgs.Card_Diamond and self:isEnemy(who) and hasManjuanEffect(who))
							or (judge.card:getSuit() == sgs.Card_Club and self:needToThrowArmor(damage.from)) then
							retr = false
						end
						if retr
							and ((self:isFriend(who) and card_x:getSuit() == sgs.Card_Heart and who:isWounded())
								or (card_x:getSuit() == sgs.Card_Diamond and self:isEnemy(who) and hasManjuanEffect(who))
								or (card_x:getSuit() == sgs.Card_Diamond and self:isFriend(who) and not hasManjuanEffect(who))
								or (card_x:getSuit() == sgs.Card_Club and (self:needToThrowArmor(damage.from) or damage.from:isNude())))
								or (judge.card:getSuit() == sgs.Card_Spade and self:toTurnOver(damage.from, 0)) then
							table.insert(other_suit, card)
						end
					end
				else
					if not self:toTurnOver(damage.from, 0) and card_x:getSuit() ~= sgs.Card_Spade and judge.card:getSuit() == sgs.Card_Spade then
						table.insert(can_use, card)
					end
				end
			end
		elseif self:isFriend(who) and judgeIsGood(judge:isGood(card_x),need_reverse)
				and not (self_card and (self:getFinalRetrial() == 2 or self:dontRespondPeachInJudge(judge)) and isCard("Peach", card_x, self.player)) then
			table.insert(can_use, card)
		elseif self:isEnemy(who) and not judgeIsGood(judge:isGood(card_x), need_reverse)
				and not (self_card and (self:getFinalRetrial() == 2 or self:dontRespondPeachInJudge(judge)) and isCard("Peach", card_x, self.player)) then
			table.insert(can_use, card)
		end
	end
	if not hasSpade and #other_suit > 0 then table.insertTable(can_use, other_suit) end

	if next(can_use) then
		if self:needToThrowArmor() then
			for _, c in ipairs(can_use) do
				if c:getEffectiveId() == self.player:getArmor():getEffectiveId() then return c:getEffectiveId() end
			end
		end
		self:sortByKeepValue(can_use)
		return can_use[1]:getEffectiveId()
	else
		return -1
	end
end

--东方杀相关
--【不灭】【迷彩】【静电】
--【冰魄】核热怎么算？？【幻梦】怎么算。。。
--这些会导致即便是伤害取消系也不会出杀 例如静电
function SmartAI:damageIsEffective(to, nature, from)
	to = to or self.player
	from = from or self.room:getCurrent()
	nature = nature or sgs.DamageStruct_Normal

	if to:hasSkill("shenjun") and to:getGender() ~= from:getGender() and nature ~= sgs.DamageStruct_Thunder then
		return false
	end
	if to:getMark("@fenyong") > 0 and to:hasSkill("fenyong")  then
		return false
	end
	if to:getMark("@fog") > 0 and nature ~= sgs.DamageStruct_Thunder then
		return false
	end
	if to:hasSkill("ayshuiyong") and nature == sgs.DamageStruct_Fire then
		return false
	end

	if from:hasSkill("lizhi") and self:isFriend(from,to) then return false end

	if to:hasLordSkill("shichou") and to:getMark("xhate") == 1 then
		for _, p in sgs.qlist(self.room:getOtherPlayers(to)) do
			if p:getMark("hate_" .. to:objectName()) > 0 and p:getMark("@hate_to") > 0 then return self:damageIsEffective(p, nature, from) end
		end
	end
	return true
end

function SmartAI:getDamagedEffects(to, from, slash)
	from = from or self.room:getCurrent()
	to = to or self.player

	if slash then
		if from:hasWeapon("IceSword") and to:getCards("hes"):length() > 1 and not self:isFriend(from, to) then
			return false
		end
	end
	if to:hasLordSkill("shichou") then
		return sgs.ai_need_damaged.shichou(self, from, to) == 1
	end

	if from:objectName() ~= to:objectName() and self:hasHeavySlashDamage(from, nil, to) then return false end

	if sgs.isGoodHp(to) then
		for _, askill in sgs.qlist(to:getVisibleSkillList()) do
			local callback = sgs.ai_need_damaged[askill:objectName()]
			if type(callback) == "function" and callback(self, from, to) then return true end
		end
	end
	return false
end

local function prohibitUseDirectly(card, player)
	if player:isCardLimited(card, card:getHandlingMethod()) then return true end
	if card:isKindOf("Peach") and player:hasFlag("Global_PreventPeach") then return true end
	return false
end

local function getPlayerSkillList(player)
	local skills = sgs.QList2Table(player:getVisibleSkillList(true)) -- need check equip skill
	if player:hasSkill("weidi") and not player:isLord() then
		local lord = player:getRoom():getLord()
		if lord then
			for _, skill in sgs.qlist(lord:getVisibleSkillList()) do
				if skill:isLordSkill() then table.insert(skills, skill) end
			end
		end
	end
	return skills
end

function sgs.getCardPlace(room, card)
	local id = card:getEffectiveId()
	local card_place = room:getCardPlace(id)
	if card_place == sgs.Player_PlaceSpecial then
		local player = room:getCardOwner(id)
		if player then
			local pile_name = player:getPileName(id)
			if pile_name == "wooden_ox" or pile_name == "piao"  then return sgs.Player_PlaceHand end
		end
	end
	return card_place
end

local function cardsViewValuable(self, class_name, player)
	for _, skill in ipairs(getPlayerSkillList(player)) do
		local askill = skill:objectName()
		if player:hasSkill(askill) or player:hasLordSkill(askill) then
			local callback = sgs.ai_cardsview_valuable[askill]
			if type(callback) == "function" then
				local ret = callback(self, class_name, player)
				if ret then return ret end
			end
		end
	end
end

local function cardsView(self, class_name, player)

	for _, skill in ipairs(getPlayerSkillList(player)) do
		local askill = skill:objectName()
		if player:hasSkill(askill) or player:hasLordSkill(askill) then
			local callback = sgs.ai_cardsview_valuable[askill]
			if type(callback) == "function" then
				local ret = callback(self, class_name, player)
				if ret then return ret end
			end
		end
	end
	for _, skill in ipairs(getPlayerSkillList(player)) do
		local askill = skill:objectName()
		if player:hasSkill(askill) or player:hasLordSkill(askill) then
			local callback = sgs.ai_cardsview[askill]
			if type(callback) == "function" then
				local ret = callback(self, class_name, player)
				if ret then return ret end
			end
		end
	end
end

local function getSkillViewCard(card, class_name, player, card_place)
	for _, skill in ipairs(getPlayerSkillList(player)) do
		local askill = skill:objectName()
		if player:hasSkill(askill) or player:hasLordSkill(askill) then
			local callback = sgs.ai_view_as[askill]
			if type(callback) == "function" then
				local skill_card_str = callback(card, player, card_place, class_name)
				if skill_card_str then
					local skill_card = sgs.Card_Parse(skill_card_str)
					if skill_card:isKindOf(class_name) and not player:isCardLimited(skill_card, skill_card:getHandlingMethod()) then return skill_card_str end
				end
			end
		end
	end
end

function isCard(class_name, card, player)
	if not player or not card then global_room:writeToConsole(debug.traceback()) end
	if not card:isKindOf(class_name) then
		local place
		local id = card:getEffectiveId()
		if global_room:getCardOwner(id) == nil or global_room:getCardOwner(id):objectName() ~= player:objectName() then place = sgs.Player_PlaceHand
		--else place = global_room:getCardPlace(card:getEffectiveId()) end
		else place = sgs.getCardPlace(global_room, card) end
		if getSkillViewCard(card, class_name, player, place) then return true end
		if player:hasSkill("wushen") and card:getSuit() == sgs.Card_Heart and class_name == "Slash" then return true end
		if player:hasSkill("jinjiu") and card:isKindOf("Analeptic") and class_name == "Slash" then return true end
		if player:hasSkill("aoyi") and card:isNDTrick() and class_name == "IceSlash" then return true end
	else
		if player:hasSkill("wushen") and card:getSuit() == sgs.Card_Heart and class_name ~= "Slash" then return false end
		if player:hasSkill("jinjiu") and class_name == "Analeptic" then return false end
		if player:hasSkill("aoyi") and card:isNDTrick() and class_name ~= "IceSlash" then return false end
		if not prohibitUseDirectly(card, player) then return true end
	end
	return false
end

function SmartAI:getRealNumber(card,player)
	player = player or self.player
	if player:hasSkill("bendan")  then
		return 9
	end
	return card:getNumber()
end

function SmartAI:getMaxCard(player)
	player = player or self.player

	if player:isKongcheng() then
		return nil
	end


	local cards = player:getHandcards()
	local max_card, max_point = nil, 0
	for _, card in sgs.qlist(cards) do
		local flag = string.format("%s_%s_%s", "visible", global_room:getCurrent():objectName(), player:objectName())
		if (player:objectName() == self.player:objectName() and not self:isValuableCard(card)) or card:hasFlag("visible") or card:hasFlag(flag) then
			local point = self:getRealNumber(card,player)
			if point > max_point then
				max_point = point
				max_card = card
			end
		end
	end
	if player:objectName() == self.player:objectName() and not max_card then
		for _, card in sgs.qlist(cards) do
			local point = self:getRealNumber(card,player)
			if point > max_point then
				max_point = point
				max_card = card
			end
		end
	end

	if player:objectName() ~= self.player:objectName() then return max_card end

	if (player:hasSkills("tianyi|dahe|xianzhen") or self.player:hasFlag("AI_XiechanUsing")) and max_point > 0 then
		for _, card in sgs.qlist(cards) do
			if self:getRealNumber(card,player) == max_point and not isCard("Slash", card, player) then
				return card
			end
		end
	end
	if player:hasSkill("qiaoshui") and max_point > 0 then
		for _, card in sgs.qlist(cards) do
			if self:getRealNumber(card,player) == max_point and not card:isNDTrick() then
				return card
			end
		end
	end

	return max_card
end

function SmartAI:getMinCard(player)
	player = player or self.player

	if player:isKongcheng() then
		return nil
	end

	local cards = player:getHandcards()
	local min_card, min_point = nil, 14
	for _, card in sgs.qlist(cards) do
		local flag = string.format("%s_%s_%s", "visible", global_room:getCurrent():objectName(), player:objectName())
		if player:objectName() == self.player:objectName() or card:hasFlag("visible") or card:hasFlag(flag) then
			local point = self:getRealNumber(card,player)
			if point < min_point then
				min_point = point
				min_card = card
			end
		end
	end

	return min_card
end

function SmartAI:getKnownNum(player)
	player = player or self.player
	if not player then
		return self.player:getHandcardNum()
	else
		local cards = player:getHandcards()
		local known = 0
		for _, card in sgs.qlist(cards) do
			local flag=string.format("%s_%s_%s","visible",global_room:getCurrent():objectName(),player:objectName())
			if card:hasFlag("visible") or card:hasFlag(flag) then
				known = known + 1
			end
		end
		return known
	end
end

function getKnownNum(player)
	if not player then global_room:writeToConsole(debug.traceback()) return end
	local cards = player:getHandcards()
	cards=sgs.touhouAppendExpandPileToList(player,cards)
	local known = 0
	for _, card in sgs.qlist(cards) do
		local flag=string.format("%s_%s_%s","visible",global_room:getCurrent():objectName(),player:objectName())
		if card:hasFlag("visible") or card:hasFlag(flag) then
			known = known + 1
		end
	end
	return known
end

function getKnownCard(player, from, class_name, viewas, flags, pile)
	if not player or (flags and type(flags) ~= "string") then global_room:writeToConsole(debug.traceback()) return 0 end
	flags = flags or "hs"
	player = findPlayerByObjectName(global_room, player:objectName())
	from = from or global_room:getCurrent()
	pile = pile or true
	local cards = player:getCards(flags)
	if flags:match("h") and pile then
		cards=sgs.touhouAppendExpandPileToList(player,cards)
	end
	local known = 0
	local suits = {["club"] = 1, ["spade"] = 1, ["diamond"] = 1, ["heart"] = 1}
	for _, card in sgs.qlist(cards) do
		local flag = string.format("%s_%s_%s", "visible", from:objectName(), player:objectName())
		if card:hasFlag("visible") or card:hasFlag(flag) or player:objectName() == from:objectName() then
			if (viewas and isCard(class_name, card, player)) or card:isKindOf(class_name)
				or (suits[class_name] and card:getSuitString() == class_name)
				or (class_name == "red" and card:isRed()) or (class_name == "black" and card:isBlack()) then
				known = known + 1
			end
		end
	end
	return known
end

function SmartAI:getCardId(class_name, player, acard, exclude_subclass_name)

	player = player or self.player
	local cards
	if acard then cards = { acard }
	else
		cards = player:getCards("hes")
		for _, key in sgs.list(player:getPileNames()) do
			for _, id in sgs.qlist(player:getPile(key)) do
				cards:append(sgs.Sanguosha:getCard(id))
			end
		end
		cards = sgs.QList2Table(cards)
	end
	self:sortByUsePriority(cards, player)

	local card_str = cardsViewValuable(self, class_name, player)
	if card_str then return card_str end

	--戏画可以参考？
	--local guhuo_str = self:getGuhuoCard(class_name, false)
	--if guhuo_str then return guhuo_str end

	local viewArr, cardArr = {}, {}

	for _, card in ipairs(cards) do
		local viewas, cardid
		--local card_place = self.room:getCardPlace(card:getEffectiveId())
		local card_place = sgs.getCardPlace(self.room, card)
		viewas = getSkillViewCard(card, class_name, player, card_place)
		if viewas then table.insert(viewArr, viewas) end
		if card:isKindOf(class_name) and ((not exclude_subclass_name) or not card:isKindOf(exclude_subclass_name))
			and not prohibitUseDirectly(card, player) and card_place ~= sgs.Player_PlaceSpecial then
			table.insert(cardArr, card:getEffectiveId())
		end
	end
	if #viewArr > 0 or #cardArr > 0 then
		local viewas, cardid
		viewas = #viewArr > 0 and viewArr[1]
		cardid = #cardArr > 0 and cardArr[1]
		local viewCard
		if viewas then viewCard = sgs.Card_Parse(viewas) end
		return (player:hasSkill("chongzhen") and viewCard and viewCard:getSkillName() == "longdan") and (viewas or cardid) or (cardid or viewas)
	end
	return cardsView(self, class_name, player)
end

function SmartAI:getCard(class_name, player, exclude_subclass_name)
	player = player or self.player
	local card_id = self:getCardId(class_name, player, nil, exclude_subclass_name)
	if card_id then return sgs.Card_Parse(card_id) end
end

function SmartAI:getCards(class_name, flag, exclude_subclass_name)
	local player = self.player
	local room = self.room
	if flag and type(flag) ~= "string" then room:writeToConsole(debug.traceback()) return {} end

	local private_pile
	if not flag then private_pile = true end
	flag = flag or "hes"
	local all_cards = player:getCards(flag)
	if private_pile then
		for _, key in sgs.list(player:getPileNames()) do
			for _, id in sgs.qlist(player:getPile(key)) do
				all_cards:append(sgs.Sanguosha:getCard(id))
			end
		end
	elseif flag:match("h") then
		all_cards=self:touhouAppendExpandPileToList(player,all_cards)
	end

	local cards = {}
	local card_place, card_str

	card_str = cardsViewValuable(self, class_name, player)
	if card_str then
		card_str = sgs.Card_Parse(card_str)
		table.insert(cards, card_str)
	end

	for _, card in sgs.qlist(all_cards) do
		--card_place = room:getCardPlace(card:getEffectiveId())
		card_place = sgs.getCardPlace(room, card)
		if class_name == "." and card_place ~= sgs.Player_PlaceSpecial then table.insert(cards, card)
		elseif class_name == "sqchuangshi"  and not prohibitUseDirectly(card, player) then table.insert(cards, card)
		elseif card:isKindOf(class_name)  and ((not exclude_subclass_name) or not card:isKindOf(exclude_subclass_name)) 
			and not prohibitUseDirectly(card, player) and card_place ~= sgs.Player_PlaceSpecial then table.insert(cards, card)
		else
			card_str = getSkillViewCard(card, class_name, player, card_place)
			if card_str then
				card_str = sgs.Card_Parse(card_str)
				table.insert(cards, card_str)
			end
		end
	end

	card_str = cardsView(self, class_name, player)
	if card_str then
		card_str = sgs.Card_Parse(card_str)
		table.insert(cards, card_str)
	end

	return cards
end

--【荧火】【穿壁】
--【七曜】？
function getCardsNum(class_name, player, from, exclude_subclass_name)
	if not player then
		global_room:writeToConsole(debug.traceback())
		return 0
	end
	local cards = player:getHandcards()
	cards=sgs.touhouAppendExpandPileToList(player,cards)
	local cards = sgs.QList2Table(cards)
	local num = 0
	local shownum = 0
	local redpeach = 0
	local redslash = 0
	local blackcard = 0
	local blacknull = 0
	local equipnull = 0
	local equipcard = 0
	local heartslash = 0
	local heartpeach = 0
	local spadenull = 0
	local spadewine = 0
	local spadecard = 0
	local diamondcard = 0
	local clubcard = 0
	local slashjink = 0
	local ndtrick = 0
	from = from or global_room:getCurrent()

	if not player then
		return #getCards(class_name, player, from, exclude_subclass_name)
	else
		for _, card in ipairs(cards) do
			local flag = string.format("%s_%s_%s", "visible", from:objectName(), player:objectName())
			if card:hasFlag("visible") or card:hasFlag(flag) or from:objectName() == player:objectName() then
				shownum = shownum + 1
				if card:isKindOf(class_name) and ((not exclude_subclass_name) or not card:isKindOf(exclude_subclass_name)) then
					num = num + 1
				end
				if card:isKindOf("EquipCard") then
					equipcard = equipcard + 1
				end
				if card:isKindOf("Slash") or card:isKindOf("Jink") then
					slashjink = slashjink + 1
				end
				if card:isRed() then
					if not card:isKindOf("Slash") then
						redslash = redslash + 1
					end
					if not card:isKindOf("Peach") then
						redpeach = redpeach + 1
					end
				end
				if card:isBlack() then
					blackcard = blackcard + 1
					if not card:isKindOf("Nullification") then
						blacknull = blacknull + 1
					end
				end
				if card:getSuit() == sgs.Card_Heart then
					if not card:isKindOf("Slash") then
						heartslash = heartslash + 1
					end
					if not card:isKindOf("Peach") then
						heartpeach = heartpeach + 1
					end
				end
				if card:getSuit() == sgs.Card_Spade then
					if not card:isKindOf("Nullification") then
						spadenull = spadenull + 1
					end
					if not card:isKindOf("Analeptic") then
						spadewine = spadewine + 1
					end
				end
				if card:getSuit() == sgs.Card_Diamond and not card:isKindOf("Slash") then
					diamondcard = diamondcard + 1
				end
				if card:getSuit() == sgs.Card_Club then
					clubcard = clubcard + 1
				end
				if card:isNDTrick() then
					ndtrick = ndtrick+1
				end
			end
		end
	end
	local ecards = player:getCards("e")
	for _, card in sgs.qlist(ecards) do
		equipcard = equipcard + 1
		if player:getHandcardNum() > player:getHp() then
			equipnull = equipnull + 1
		end
		if card:isRed() then
			redpeach = redpeach + 1
			redslash = redslash + 1
		end
		if card:getSuit() == sgs.Card_Heart then
			heartpeach = heartpeach + 1
		end
		if card:getSuit() == sgs.Card_Spade then
			spadecard = spadecard + 1
		end
		if card:getSuit() == sgs.Card_Diamond  then
			diamondcard = diamondcard + 1
		end
		if card:getSuit() == sgs.Card_Club then
			clubcard = clubcard + 1
		end
	end

	if class_name == "Slash" then
		local slashnum
		if player:hasSkill("wusheng") then
			slashnum = redslash + num + (player:getHandcardNum() - shownum) * 0.69
		elseif player:hasSkill("wushen") then
			slashnum = heartslash + num + (player:getHandcardNum() - shownum)*0.5
		elseif player:hasSkill("nosgongqi") then
			slashnum = equipcard + num + (player:getHandcardNum() - shownum)*0.5
		elseif player:hasSkill("longdan") then
			slashnum = slashjink + (player:getHandcardNum() - shownum)*0.72
		else
			slashnum = num+(player:getHandcardNum() - shownum)*0.35
		end
		if player:hasSkill("yongheng") and player:getPhase() == sgs.Player_NotActive  then
			if player:hasWeapon("Spear") then
				slashnum = 1000
			end
		end
		return player:hasSkill("wushuang") and slashnum*2 or slashnum
	elseif class_name == "Jink" then
		--if player:hasSkill("yingguang") and player:getMark("@yingguang")>0 then
		--  return 100
		--end
		if player:hasSkill("chuanbi") then
			current=player:getRoom():getCurrent()
			if not (current:isAlive() and current:getWeapon())then
				return 100
			end
		end
		if player:hasSkill("qingguo") then
			return blackcard + num + (player:getHandcardNum() - shownum)*0.85
		elseif player:hasSkill("longdan") then
			return slashjink + (player:getHandcardNum() - shownum)*0.72
		elseif player:hasSkill("kofqingguo") then
			return num + (player:getHandcardNum() - shownum) * 0.6 + player:getEquips():length()
		else
			return num + (player:getHandcardNum() - shownum)*0.6
		end
	elseif class_name == "Peach" then
		if player:hasSkill("jijiu") then
			return num + redpeach + (player:getHandcardNum() - shownum)*0.6
		elseif player:hasSkill("chunlao") then
			return num + player:getPile("wine"):length()
		elseif player:hasSkill("jiuzhu") then
			return math.max(num, math.max(0, math.min(player:getCardCount(true), player:getHp() - 1)))
		elseif player:hasSkill("shende") then
			return num+player:getPile("shende"):length()/2
		else
			return num
		end
	elseif class_name == "Nullification" then
		if player:hasSkill("kanpo") then
			return num + blacknull + (player:getHandcardNum() - shownum)*0.5
		elseif player:hasSkill("yanzheng") then
			return num + equipnull
		else
			return num
		end
	else
		return num
	end
end

function SmartAI:getCardsNum(class_name, flag, selfonly, exclude_subclass_name)
	local player = self.player
	local n = 0
	if type(class_name) == "table" then
		for _, each_class in ipairs(class_name) do
			n = n + self:getCardsNum(each_class, flag, selfonly)
		end
		return n
	end
	n = #self:getCards(class_name, flag, exclude_subclass_name)

	card_str = cardsView(self, class_name, player)
	if card_str then
		card_str = sgs.Card_Parse(card_str)
		if card_str:getSkillName() == "Spear" or card_str:getSkillName() == "fuhun" then
			n = n + math.floor(player:getHandcardNum() / 2) - 1
		elseif card_str:getSkillName() == "jiuzhu" then
			n = math.max(n, math.max(0, math.min(player:getCardCount(), player:getHp() - 1)))
		elseif card_str:getSkillName() == "renxin" then
			n = n + 1
		--elseif card_str:getSkillName() == "bllmwuyu" then
		--  n = n + 1
		end
	end

	if selfonly then return n end
	if class_name == "Jink" then
		if player:hasLordSkill("hujia") then
			local lieges = self.room:getLieges("wei", player)
			for _, liege in sgs.qlist(lieges) do
				if self:isFriend(liege, player) then
					n = n + getCardsNum("Jink", liege, self.player)
				end
			end
		end
		if player:hasLordSkill("tianren") and player:getPhase()==sgs.Player_NotActive then
			local lieges = self.room:getLieges("zhan", player)
			for _, liege in sgs.qlist(lieges) do
				if self:isFriend(liege, player) then
					n = n + getCardsNum("Jink", liege, self.player)
				end
			end
		end
	elseif class_name == "Slash" then
		if player:hasSkill("wushuang") then
			n = n * 2
		end
		if player:hasLordSkill("jijiang") then
			local lieges = self.room:getLieges("shu", player)
			for _, liege in sgs.qlist(lieges) do
				if self:isFriend(liege, player) then
				n = n + getCardsNum("Slash", liege, self.player)
				end
			end
		end
		if player:hasLordSkill("tianren") and player:getPhase()==sgs.Player_NotActive  then
			local lieges = self.room:getLieges("zhan", player)
			for _, liege in sgs.qlist(lieges) do
				if self:isFriend(liege, player) then
				n = n + getCardsNum("Slash", liege, self.player)
				end
			end
		end
	end
	return n
end

function SmartAI:getAllPeachNum(player)
	player = player or self.player
	local n = 0
	for _, friend in ipairs(self:getFriends(player)) do
		local num = self.player:objectName() == friend:objectName() and self:getCardsNum("Peach") or getCardsNum("Peach", friend, self.player)
		n = n + num
	end
	return n
end
function SmartAI:getRestCardsNum(class_name, yuji)
	yuji = yuji or self.player
	local ban = sgs.Sanguosha:getBanPackages()
	ban = table.concat(ban, "|")
	sgs.discard_pile = self.room:getDiscardPile()
	local totalnum = 0
	local discardnum = 0
	local knownnum = 0
	local card
	for i=1, sgs.Sanguosha:getCardCount() do
		card = sgs.Sanguosha:getEngineCard(i-1)
		-- if card:isKindOf(class_name) and not ban:match(card:getPackage()) then totalnum = totalnum+1 end
		if card:isKindOf(class_name) then totalnum = totalnum + 1 end
	end
	for _, card_id in sgs.qlist(sgs.discard_pile) do
		card = sgs.Sanguosha:getEngineCard(card_id)
		if card:isKindOf(class_name) then discardnum = discardnum + 1 end
	end
	for _, player in sgs.qlist(self.room:getOtherPlayers(yuji)) do
		knownnum = knownnum + getKnownCard(player, self.player, class_name)
	end
	return totalnum - discardnum - knownnum
end

function SmartAI:hasSuit(suit_strings, include_equip, player)
	return self:getSuitNum(suit_strings, include_equip, player) > 0
end

function SmartAI:getSuitNum(suit_strings, include_equip, player)
	player = player or self.player
	local n = 0
	local flag = include_equip and "hes" or "hs"
	local allcards
	if player:objectName() == self.player:objectName() then
		allcards = sgs.QList2Table(player:getCards(flag))
	else
		allcards = include_equip and sgs.QList2Table(player:getEquips()) or {}
		local handcards = sgs.QList2Table(player:getHandcards())
		local flag = string.format("%s_%s_%s","visible", self.player:objectName(), player:objectName())
		for i = 1, #handcards, 1 do
			if handcards[i]:hasFlag("visible") or handcards[i]:hasFlag(flag) then
				table.insert(allcards,handcards[i])
			end
		end
	end
	for _, card in ipairs(allcards) do
		for _, suit_string in ipairs(suit_strings:split("|")) do
			if card:getSuitString() == suit_string
				or (suit_string == "black" and card:isBlack()) or (suit_string == "red" and card:isRed()) then
				n = n + 1
			end
		end
	end
	return n
end

function SmartAI:hasSkill(skill)
	local skill_name = skill
	if type(skill) == "table" then
		skill_name = skill.name
	end

	local real_skill = sgs.Sanguosha:getSkill(skill_name)
	if real_skill and real_skill:isLordSkill() then
		return self.player:hasLordSkill(skill_name)
	else
		return self.player:hasSkill(skill_name)
	end
end

function SmartAI:hasSkills(skill_names, player)
	player = player or self.player
	if type(player) == "table" then
		for _, p in ipairs(player) do
			if p:hasSkills(skill_names) then return true end
		end
		return false
	end
	if type(skill_names) == "string" then
		return player:hasSkills(skill_names)
	end
	return false
end

function SmartAI:fillSkillCards(cards)
	local i = 1
	while i <= #cards do
		if prohibitUseDirectly(cards[i], self.player) and not cards[i]:canRecast() then
		--存在虽然不能使用，但是可以重铸的情况
			table.remove(cards, i)
		else
			i = i + 1
		end
	end
	for _, skill in ipairs(sgs.ai_skills) do
		if self:hasSkill(skill) or (skill.name == "shuangxiong" and self.player:hasFlag("shuangxiong")) then
			local skill_card = skill.getTurnUseCard(self, #cards == 0)
			if skill_card then table.insert(cards, skill_card) end
		end
	end
end

function SmartAI:useSkillCard(card,use)
	local name
	if card:isKindOf("LuaSkillCard") then
		name = "#" .. card:objectName()
	else
		name = card:getClassName()
	end


	if not use.isDummy then
		--modian 和 modian_attach两个skill都能发动  但技能卡的skillname注明的是modian_attach
		--魔典强行耦合。。。
		if (card:getSkillName() == "modian_attach") then
			if not self.player:hasSkill("modian") and not self.player:hasSkill("modian_attach") then
				return
			end
		else
			if not self.player:hasSkill(card:getSkillName()) and not self.player:hasLordSkill(card:getSkillName()) then
				return
			end
		end
	end
	if sgs.ai_skill_use_func[name] then
		sgs.ai_skill_use_func[name](card, use, self)
		if use.to then
			if not use.to:isEmpty() and sgs.dynamic_value.damage_card[name] then
				for _, target in sgs.qlist(use.to) do
					if self:damageIsEffective(target) then return end
				end
				use.card = nil
			end
		end
		return
	end


	if self["useCard"..name] then
		self["useCard"..name](self, card, use)
	end
end

function SmartAI:useBasicCard(card, use)
	if not card then global_room:writeToConsole(debug.traceback()) return end
	if self.player:hasSkill("ytchengxiang") and self.player:getHandcardNum() < 8 and card:getNumber() < 7 then return end
	if not (card:isKindOf("Peach") and self.player:getLostHp() > 1) and self:needBear() then return end
	if self:touhouNeedBear(card)  then return end
	if self:needRende() then return end
	if self:hasSkill("chaoren") then
		local acard = sgs.Sanguosha:getCard(self.room:getDrawPile():first())
		if acard:getClassName()==card:getClassName() then
			self:useCardByClassName(acard, use)
		elseif acard:isKindOf("Slash") and card:isKindOf("Slash") then
			self:useCardByClassName(acard, use)
		end
	end
	self:useCardByClassName(card, use)
end

--东方杀相关
--【幻梦】
function SmartAI:aoeIsEffective(card, to, source)
	local players = self.room:getAlivePlayers()
	players = sgs.QList2Table(players)
	source = source or self.room:getCurrent()
	if  to:hasSkill("huanmeng") then
		return false
	end
	if to:hasArmorEffect("Vine") and (card:isKindOf("SavageAssault") or card:isKindOf("ArcheryAttack")) then
		return false
	end
	if card:isKindOf("Drowning") and not to:canDiscard(to, "e") then
		return false
	end	
	if self.room:isProhibited(self.player, to, card) then
		return false
	end
	if to:isLocked(card) then
		return false
	end

	if not self:hasTrickEffective(card, to, source) or not self:damageIsEffective(to, sgs.DamageStruct_Normal, source) then
		return false
	end
	return true
end

function SmartAI:canAvoidAOE(card)
	if not self:aoeIsEffective(card, self.player) then return true end
	if card:isKindOf("SavageAssault") then
		if self:getCardsNum("Slash") > 0 then
			return true
		end
	end
	if card:isKindOf("ArcheryAttack") then
		if self:getCardsNum("Jink") > 0 or (self:hasEightDiagramEffect() and self.player:getHp() > 1) then
			return true
		end
	end
	return false
end

function SmartAI:getDistanceLimit(card, from)
	from = from or self.player
	if (card:isKindOf("Snatch") or card:isKindOf("SupplyShortage")) and card:getSkillName() ~= "qiaoshui" then
		return 1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_DistanceLimit, from, card)
	end
end

function SmartAI:exclude(players, card, from)
	from = from or self.player
	local excluded = {}
	local limit = self:getDistanceLimit(card, from)
	local range_fix = 0

	if type(players) ~= "table" then players = sgs.QList2Table(players) end

	if card:isVirtualCard() then
		for _, id in sgs.qlist(card:getSubcards()) do
			if from:getOffensiveHorse() and from:getOffensiveHorse():getEffectiveId() == id then range_fix = range_fix + 1 end
		end
		if card:getSkillName() == "jixi" then range_fix = range_fix + 1 end
	end

	for _, player in ipairs(players) do
		if not self.room:isProhibited(from, player, card) then
			local should_insert = true
			if limit then
				should_insert = from:distanceTo(player, range_fix) <= limit
			end
			if should_insert then
				table.insert(excluded, player)
			end
		end
	end
	return excluded
end


function SmartAI:getJiemingChaofeng(player)
	local max_x , chaofeng = 0 , 0
	for _, friend in ipairs(self:getFriends(player)) do
		local x = math.min(friend:getMaxHp(), 5) - friend:getHandcardNum()
		if x > max_x then
			max_x = x
		end
	end
	if max_x < 2 then
		chaofeng = 5 - max_x * 2
	else
		chaofeng = (-max_x) * 2
	end
	return chaofeng
end

--东方杀相关
--对于to的评分 不是对于from的
function SmartAI:getAoeValueTo(card, to, from)
	local value, sj_num = 0, 0
	if card:isKindOf("ArcheryAttack") then sj_num = getCardsNum("Jink", to, from) end
	if card:isKindOf("SavageAssault") then sj_num = getCardsNum("Slash", to, from) end

	if self:aoeIsEffective(card, to, from) then
		local jink = sgs.cloneCard("jink")
		local slash = sgs.cloneCard("slash")
		local isLimited
		if card:isKindOf("ArcheryAttack") and to:isCardLimited(jink, sgs.Card_MethodResponse) then isLimited = true
		elseif card:isKindOf("SavageAssault") and to:isCardLimited(slash, sgs.Card_MethodResponse) then isLimited = true end
		if card:isKindOf("SavageAssault") and sgs.card_lack[to:objectName()]["Slash"] == 1
			or card:isKindOf("ArcheryAttack") and sgs.card_lack[to:objectName()]["Jink"] == 1
			or sj_num < 1 or isLimited then
			value = -70
		else
			value = -50
		end
		value = value + math.min(20, to:getHp() * 5)

		if self:getDamagedEffects(to, from) then value = value + 40 end
		if self:needToLoseHp(to, from, nil, true) then value = value + 10 end
        
		if card:isKindOf("Drowning") then
			if to:hasSkills(sgs.lose_equip_skill) and to:canDiscard(to, "e") then
				value = value + 50
			end
			if self:needToThrowArmor(to) and  self:isFriend(to) then
				value = value + 20
			end
		end
		if card:isKindOf("ArcheryAttack") then
			if to:hasSkills("leiji|nosleiji") and (sj_num >= 1 or self:hasEightDiagramEffect(to)) and self:findLeijiTarget(to, 50, from) then
				value = value + 100
				if self:hasSuit("spade", true, to) then value = value + 150
				else value = value + to:getHandcardNum()*35
				end
			elseif self:hasEightDiagramEffect(to) then
				value = value + 20
				if self:getFinalRetrial(to) == 2 then
					value = value - 15
				elseif self:getFinalRetrial(to) == 1 then
					value = value + 10
				end
			end
		end

		if card:isKindOf("ArcheryAttack") and sj_num >= 1 then
			if self:hasSkills("mingzhe|gushou", to) then value = value + 8 end
			if to:hasSkill("xiaoguo") then value = value - 4 end
		elseif card:isKindOf("SavageAssault") and sj_num >= 1 then
			if to:hasSkill("xiaoguo") then value = value - 4 end
		end


		local wansha = self.room:getCurrent() and self.room:getCurrent():hasSkill("wansha")
		if wansha and to:getHp() == 1 and (sgs.card_lack[to:objectName()]["Peach"] == 1 or getCardsNum("Peach", to, from) == 0) then
			value = value - 30
			if self:isFriend(to) and self:getCardsNum("Peach") >= 1 then
				value = value + 10
			end
		end


		if self.room:getMode() ~= "06_3v3" and self.room:getMode() ~= "06_XMode" then
			if to:getHp() == 1 and isLord(from) and sgs.evaluatePlayerRole(to) == "loyalist" and self:getCardsNum("Peach") == 0 then
				value = value - from:getCardCount() * 20
			end
		end

		if to:getHp() > 1 then
			if to:hasSkill("jianxiong") then
				value = value + ((card:isVirtualCard() and card:subcardsLength()*10) or 10)
			end
			if to:hasSkill("tanlan") and self:isEnemy(to) and not from:isKongcheng() then value = value + 10 end
		end
	else
		value = value + 10
	end
	value=value + self:touhouGetAoeValueTo(card, to, from)
	return value
end

function getLord(player)
	if not player then global_room:writeToConsole(debug.traceback()) return end

	if sgs.GetConfig("EnableHegemony", false) then return nil end
	local room = global_room
	player = findPlayerByObjectName(room, player:objectName(), true)

	local mode = string.lower(room:getMode())
	if mode == "06_3v3" then
		if player:getRole() == "lord" or player:getRole() == "renegade" then return player end
		if player:getRole() == "loyalist" then return room:getLord() end
		for _, p in sgs.qlist(room:getAllPlayers()) do
			if p:getRole() == "renegade" then return p end
		end
	end
	return room:getLord() or player
end

function isLord(player)
	return player and getLord(player) and getLord(player):objectName() == player:objectName()
end

--东方杀相关
--【理智】
function SmartAI:getAoeValue(card, player)
	local attacker = player or self.player
	local good, bad = 0, 0
	local lord = getLord(self.player)

	local canHelpLord = function()
		if not lord or self:isEnemy(lord, attacker) then return false end
		if self.player:hasSkill("qice") and card:isVirtualCard() then return false end

		local peach_num, null_num, slash_num, jink_num = 0, 0, 0, 0
		if card:isVirtualCard() and card:subcardsLength() > 0 then
			for _, subcardid in sgs.qlist(card:getSubcards()) do
				local subcard = sgs.Sanguosha:getCard(subcardid)
				if isCard("Peach", subcard, attacker) then peach_num = peach_num - 1 end
				if isCard("Slash", subcard, attacker) then slash_num = slash_num - 1 end
				if isCard("Jink", subcard, attacker) then jink_num = jink_num - 1 end
				if isCard("Nullification", subcard, attacker) then null_num = null_num - 1 end
			end
		end

		if card:isKindOf("SavageAssault") and lord:hasLordSkill("jijiang") and self.player:getKingdom() == "shu" and
			self:getCardsNum("Slash") > slash_num then return true end
		if card:isKindOf("SavageAssault") and lord:hasLordSkill("tianren") and lord:getPhase()==sgs.Player_NotActive
		and self.player:getKingdom() == "zhan" and
			self:getCardsNum("Slash") > slash_num then return true end

		if card:isKindOf("ArcheryAttack") and lord:hasLordSkill("hujia") and self.player:getKingdom() == "wei" and
			self:getCardsNum("Jink") > jink_num then return true end
		if card:isKindOf("ArcheryAttack") and lord:hasLordSkill("tianren") and lord:getPhase()==sgs.Player_NotActive
		and self.player:getKingdom() == "zhan" and
			self:getCardsNum("Jink") > jink_num then return true end

		if self:getCardsNum("Peach") > peach_num then return true end

		local goodnull, badnull = 0, 0
		for _, p in sgs.qlist(self.room:getAlivePlayers()) do
			if self:isFriend(lord, p) then
				goodnull = goodnull +  getCardsNum("Nullification", p, attacker)
			else
				badnull = badnull +  getCardsNum("Nullification", p, attacker)
			end
		end
		return goodnull - null_num - badnull >= 2
	end

	local isEffective_F, isEffective_E = 0, 0
	for _, friend in ipairs(self:getFriendsNoself(attacker)) do
		good = good + self:getAoeValueTo(card, friend, attacker)
		if self:aoeIsEffective(card, friend, attacker) then isEffective_F = isEffective_F + 1 end
	end

	for _, enemy in ipairs(self:getEnemies(attacker)) do
		bad = bad + self:getAoeValueTo(card, enemy, attacker)
		if self:aoeIsEffective(card, enemy, attacker) then isEffective_E = isEffective_E + 1 end
	end

	if isEffective_F == 0 and isEffective_E == 0 then
		return attacker:hasSkills("jizhi|nosjizhi") and 10 or -100
	elseif isEffective_E == 0 then
		return -100
	end
	if not sgs.GetConfig("EnableHegemony", false) then
		if self.role ~= "lord" and sgs.isLordInDanger() and self:aoeIsEffective(card, lord, attacker) and not canHelpLord() and not hasBuquEffect(lord) then
			if self:isEnemy(lord) then
				good = good + (lord:getHp() == 1 and 200 or 150)
				if lord:getHp() <= 2 then
					if #self.enemies == 1 then good = good + 150 - lord:getHp() * 50 end
					if lord:isKongcheng() then good = good + 150 - lord:getHp() * 50 end
				end
			else
				bad = bad + (lord:getHp() == 1 and 2013 or 250)
			end
		end
	end


	local enemy_number = 0
	for _, player in sgs.qlist(self.room:getOtherPlayers(attacker)) do
		if self:cantbeHurt(player, attacker) and self:aoeIsEffective(card, player, attacker) then
			if player:hasSkill("wuhun") and not self:isWeak(player) and attacker:getMark("@nightmare") == 0 then
				if attacker:objectName() == self.player:objectName() and self.role ~= "renegade" and self.role ~= "lord" then
				elseif attacker:objectName() ~= self.player:objectName() and not (self:isFriend(attacker) and attacker:objectName() == lord:objectName()) then
				else
					bad = bad + 250
				end
			else
				bad = bad + 250
			end
		end

		if player:hasSkill("dushi") and not attacker:hasSkill("benghuai") and self:isFriend(attacker) and self:isWeak(player) then bad = bad + 40 end

		if self:aoeIsEffective(card, player, attacker) and not self:isFriend(player, attacker) then enemy_number = enemy_number + 1 end
	end

	local forbid_start = true
	if attacker:hasSkills("nosjizhi|jizhi") then
		forbid_start = false
		good = good + 51
	end
	if attacker:hasSkills("shenfen+kuangbao") then
		forbid_start = false
		good = good + 3 * enemy_number
		if self.player:getMark("@wrath") > 0 then
			good = good + enemy_number
		end
	end
	if not sgs.GetConfig("EnableHegemony", false) then
		if forbid_start and sgs.turncount < 2 and attacker:getSeat() <= 3 and card:isKindOf("SavageAssault") and enemy_number > 0 then
			if self.role ~= "rebel" then
				good = good + (isEffective_E > 0 and 50 or 0)
			else
				bad = bad + (isEffective_F > 0 and 50 or 0)
			end
		end
		if sgs.current_mode_players["rebel"] == 0 and attacker:getRole() ~= "lord" and sgs.current_mode_players["loyalist"] > 0 and self:isWeak(lord) then
			bad = bad + 300
		end
	end
	if attacker:hasSkills("jianxiong|luanji|qice|manjuan") then good = good + 2 * enemy_number end
	if attacker:hasSkills("lizhi") then good = 2 *good  end
	local xiahou = self.room:findPlayerBySkillName("yanyu")
	if xiahou and self:isEnemy(xiahou) and xiahou:getMark("YanyuDiscard2") > 0 then bad = bad + 50 end

	for _, target in sgs.qlist(self.room:getOtherPlayers(attacker)) do
		local fakeDamage = sgs.DamageStruct(card, attacker, target, 1, self:touhouDamageNature(card, attacker, target))
		local effect, willEffect = self:touhouDamageEffect(fakeDamage,from,to)
		if effect then
			good = good + 60
		end
		if self:touhouCardAttackWaste(card,attacker,target) then
			if self:isFriend(attacker,target) then
				good= good+80
			elseif  self:isEnemy(attacker,target) then
				bad = bad+80
			end
		end
	end
	for _, target in sgs.qlist(self.room:getOtherPlayers(attacker)) do
		for _, askill in sgs.qlist(target:getVisibleSkillList()) do
			local s_name = askill:objectName()
			if target:hasSkill(s_name) then  
				local filter = sgs.ai_trick_prohibit[s_name]
				if filter and type(filter) == "function" and filter(self, attacker, target, card) then
					good=-100
					break
				end
			end
		end
		if good<=-100 then
			break
		end
	end
	return good - bad
end

--东方杀相关
--【正义】【云上】【幻梦】【法华】
--【红白】【静电】之于闪电是否该这么写呢？
function SmartAI:hasTrickEffective(card, to, from)
	from = from or self.room:getCurrent()
	to = to or self.player
	if self.room:isProhibited(from, to, card) then return false end
	--if to:hasSkill("zhengyi")  and not card:isKindOf("DelayedTrick") and card:isBlack() then return false end
	if to:hasSkill("yunshang") and from:objectName() ~= to:objectName()  and not from:inMyAttackRange(to) then return false end
	if to:hasSkill("fenghua") then
        for _, id in sgs.qlist(to:getPile("fenghua")) do
			if sgs.Sanguosha:getCard(id):getSuit() == card:getSuit() then
				return false
			end
		end
	end
	if to:hasSkill("jinfa") and not self:hasWeiya() then
		local count = 0
		for _,p in sgs.qlist(self.room:getAlivePlayers()) do
			if self:isFriend(to, p)  and sgs.dynamic_value.benefit[card:getClassName()] then
				count =  count + getCardsNum("Nullification", p, self.player)
			end
			if self:isEnemy(to, p) and
			(sgs.dynamic_value.damage_card[card:getClassName()] or sgs.dynamic_value.control_card[card:getClassName()]) then
				count =  count  + getCardsNum("Nullification", p, self.player)
			end
			if count > 0 then break end
		end
		if count == 0 then return false end
	end
	--[[if to:hasSkill("yicun") and card:isKindOf("Duel") then
		if self:yicunEffective(card, to, from) then
			return false
		end
	end]]
	if to:hasSkill("weizhuang") and card:isNDTrick() then
		local basics = getCardsNum("BasicCard", from, self.player)
		if sgs.dynamic_value.damage_card[card:getClassName()] then
			if not self:isFriend(to, from) and basics < 1 then
				return false
			end
		elseif sgs.dynamic_value.benefit[card:getClassName()] then
			if not self:isFriend(to, from)  then
				return false
			end
		elseif sgs.dynamic_value.control_card[card:getClassName()] then
			if not self:isFriend(to, from) and basics < 1 then
				return false
			end
		end
	end
	if to:hasSkill("huiwu")  then
		if  self:isFriend(to, from) and sgs.dynamic_value.damage_card[card:getClassName()]  then
			return false
		end
		if sgs.dynamic_value.benefit[card:getClassName()] and self:isEnemy(to, from) then
			return false
		end
	end
	if to:hasLordSkill("fahua") then
		canFahua=false
		for _,p in sgs.qlist(self.room:getOtherPlayers(to)) do
			if p:getKingdom() =="xlc" and self:isFriend(p,to) then
				canFahua= self:hasTrickEffective(card, p, from)
				if canFahua then return true end
			end
		end
		--if canFahua then return canFahua end
	end
	if to:getMark("@late") > 0 and not card:isKindOf("DelayedTrick") then return false end
	if to:getPile("dream"):length() > 0 and to:isLocked(card) then return false end
	if to:hasSkill("huanmeng") then
		if card:isKindOf("DelayedTrick")  then
			if card:isKindOf("SupplyShortage") then
				return false
			end
		else
			if card:isKindOf("Duel") or card:isKindOf("SavageAssault") or card:isKindOf("ArcheryAttack")
				or card:isKindOf("GodSalvation") or card:isKindOf("IronChain") or card:isKindOf("FireAttack")  then
				return false
			end
		end
	end

	local nature = sgs.DamageStruct_Normal
	if card:isKindOf("FireAttack") then nature = sgs.DamageStruct_Fire end

	if (card:isKindOf("Duel") or card:isKindOf("FireAttack") or card:isKindOf("ArcheryAttack") or card:isKindOf("SavageAssault")) then
		self.equipsToDec = sgs.getCardNumAtCertainPlace(card, from, sgs.Player_PlaceEquip)
		local eff = self:damageIsEffective(to, nature, from)
		self.equipsToDec = 0
		if not eff then return false end
	end
	
	if to:hasArmorEffect("IronArmor") and (card:isKindOf("FireAttack") or card:isKindOf("IronChain")) then return false end
	--**东方杀相关
	if card:isKindOf("Duel") or card:isKindOf("FireAttack")  then
		local fakeDamage = sgs.DamageStruct(card, from, to, 1, self:touhouDamageNature(card,from,to))
		if not self:touhouNeedAvoidAttack(fakeDamage, from, to, true) then
			return false
		end
	end
	--***
	return true
end

function SmartAI:useTrickCard(card, use)
	if not card then global_room:writeToConsole(debug.traceback()) return end
	--handcardsNum will not reduce
	if self.player:isLocked(card) then return end
	if self:needBear() and not ("amazing_grace|ex_nihilo|snatch|iron_chain|collateral|lure_tiger"):match(card:objectName()) then return end
	if self:touhouNeedBear(card) and not ("amazing_grace|ex_nihilo|snatch|iron_chain|collateral"):match(card:objectName()) then  return end
	if self:needRende() and not card:isKindOf("ExNihilo") then return end
	if card:isKindOf("AOE") and not card:isKindOf("BurningCamps") then
		local others = self.room:getOtherPlayers(self.player)
		others = sgs.QList2Table(others)
		local avail = #others
		local avail_friends = 0
		for _, other in ipairs(others) do
			if self.room:isProhibited(self.player, other, card) then
				avail = avail - 1
			elseif (card:isKindOf("Drowning") and not other:canDiscard(other, "e")) then
				avail = avail - 1
			elseif self:isFriend(other) then
				avail_friends = avail_friends + 1
			end
		end
		if avail < 1 then return end

		local mode = global_room:getMode()
		if mode:find("p") and mode >= "04p" then
			if self.player:isLord() and sgs.turncount < 2 and card:isKindOf("ArcheryAttack") and self:getOverflow() < 1 then return end
			if self.role == "loyalist" and sgs.turncount < 2 and card:isKindOf("ArcheryAttack") then return end
			if self.role == "rebel" and sgs.turncount < 2 and card:isKindOf("SavageAssault") then return end
			if sgs.turncount < 2 and card:isKindOf("Drowning") then return end
		end

		local good = self:getAoeValue(card)
		if good > 0 then
			use.card = card
		end
	else
	    if card:isKindOf("Dismantlement") or card:isKindOf("Snatch") or card:isKindOf("AwaitExhausted") 
      	   or card:isKindOf("ExNihilo") then
			if not use.isDummy then
			local analeptic = self:searchForMagicAnaleptic(use, target, card)
			if analeptic and self:shouldUseMagicAnaleptic(use.card) and analeptic:getEffectiveId() ~= card:getEffectiveId() then
				use.card = analeptic
				if use.to then use.to = sgs.SPlayerList() end
				return
			end
			end
        end		
		self:useCardByClassName(card, use)
	end
	if use.to then
		if not use.to:isEmpty() and sgs.dynamic_value.damage_card[card:getClassName()] then
			local nature = card:isKindOf("FireAttack") and sgs.DamageStruct_Fire or sgs.DamageStruct_Normal
			for _, target in sgs.qlist(use.to) do
				if self:damageIsEffective(target, nature) then return end
			end
			use.card = nil
		end
	end
end

sgs.weapon_range = {}

function SmartAI:hasEightDiagramEffect(player)
	player = player or self.player
	return player:hasArmorEffect("EightDiagram") or player:hasArmorEffect("bazhen")
end

function SmartAI:hasCrossbowEffect(player)
	player = player or self.player
	return player:hasWeapon("Crossbow") or player:hasSkill("paoxiao")
end

sgs.ai_weapon_value = {}

function SmartAI:evaluateWeapon(card, player)
	player = player or self.player
	local deltaSelfThreat = 0
	local currentRange
	if not card then return -1
	else
		currentRange = sgs.weapon_range[card:getClassName()] or 0
	end
	for _, enemy in ipairs(self:getEnemies(player)) do
		if player:distanceTo(enemy) <= currentRange then
			deltaSelfThreat = deltaSelfThreat + 6 / sgs.getDefense(enemy)
		end
	end

	local slash_num = player:objectName() == self.player:objectName() and self:getCardsNum("Slash") or getCardsNum("Slash", player, self.player)
	local analeptic_num = player:objectName() == self.player:objectName() and self:getCardsNum("Analeptic", "hs", true, "MagicAnaleptic") or getCardsNum("Analeptic", player, self.player)
	local peach_num = player:objectName() == self.player:objectName() and self:getCardsNum("Peach") or getCardsNum("Peach", player, self.player)
	if card:isKindOf("Crossbow") and not player:hasSkill("paoxiao") and deltaSelfThreat ~= 0 then
		deltaSelfThreat = deltaSelfThreat + slash_num * 3 - 2
		if player:hasSkill("kurou") then deltaSelfThreat = deltaSelfThreat + peach_num + analeptic_num + self.player:getHp() end
		if player:getWeapon() and not self:hasCrossbowEffect(player) and not player:canSlashWithoutCrossbow() and slash_num > 0 then
			for _, enemy in ipairs(self:getEnemies(player)) do
				if player:distanceTo(enemy) <= currentRange
					and (sgs.card_lack[enemy:objectName()]["Jink"] == 1 or slash_num >= enemy:getHp()) then
					deltaSelfThreat = deltaSelfThreat + 10
				end
			end
		end
	end
	local callback = sgs.ai_weapon_value[card:objectName()]
	if type(callback) == "function" then
		deltaSelfThreat = deltaSelfThreat + (callback(self, nil, player) or 0)
		for _, enemy in ipairs(self:getEnemies(player)) do
			if player:distanceTo(enemy) <= currentRange and callback then
				local added = sgs.ai_slash_weaponfilter[card:objectName()]
				if added and type(added) == "function" and added(self, enemy, player) then deltaSelfThreat = deltaSelfThreat + 1 end
				deltaSelfThreat = deltaSelfThreat + (callback(self, enemy, player) or 0)
			end
		end
	end

	if player:hasSkill("jijiu") and card:isRed() then deltaSelfThreat = deltaSelfThreat + 0.5 end
	if player:hasSkills("qixi|guidao") and card:isBlack() then deltaSelfThreat = deltaSelfThreat + 0.5 end

	return deltaSelfThreat
end

sgs.ai_armor_value = {}

function SmartAI:evaluateArmor(card, player)
	player = player or self.player
	local ecard = card or player:getArmor()
	if not ecard then return 0 end

	local value = 0
	if player:hasSkill("jijiu") and ecard:isRed() then value = value + 0.5 end
	if player:hasSkills("qixi|guidao") and ecard:isBlack() then value = value + 0.5 end
	for _, askill in sgs.qlist(player:getVisibleSkillList()) do
		local callback = sgs.ai_armor_value[askill:objectName()]
		if type(callback) == "function" then
			return value + (callback(ecard, player, self) or 0)
		end
	end
	local callback = sgs.ai_armor_value[ecard:objectName()]
	if type(callback) == "function" then
		return value + (callback(player, self) or 0)
	end
	return value + 0.5
end

function SmartAI:getSameEquip(card, player)
	player = player or self.player
	if not card then return end
	if card:isKindOf("Weapon") then return player:getWeapon()
	elseif card:isKindOf("Armor") then return player:getArmor()
	elseif card:isKindOf("DefensiveHorse") then return player:getDefensiveHorse()
	elseif card:isKindOf("OffensiveHorse") then return player:getOffensiveHorse()
	elseif card:isKindOf("Treasure") then return player:getTreasure() end
end

--东方杀相关
--不能乱顶掉装备
function SmartAI:useEquipCard(card, use)
	if not card then global_room:writeToConsole(debug.traceback()) return end

	if self:touhouNeedBear(card)  then  return end
	--似乎应该分具体装备种类
    if self.player:isLocked(card) then return end
	
	--除了藤甲外，还应该有 感应之于马匹，魔操等拆装流
	if card:isKindOf("Vine") and not self.player:hasSkill("huanmeng") then
		if self.player:hasSkills("here|yexing")  then return end
		local lunar = self.room:findPlayerBySkillName("zhuonong")
		if lunar and not self:isFriend(lunar) then return end
		local utsuho = self.room:findPlayerBySkillName("here")
		if utsuho and not self:isFriend(utsuho) then return end
	end


	if self.player:hasSkill("ytchengxiang") and self.player:getHandcardNum() < 8 and card:getNumber() < 7 and self:getSameEquip(card) then return end
	if self:hasSkills("kofxiaoji|xiaoji") and self:evaluateArmor(card) > -5 then
		use.card = card
		return
	end
	if self:hasSkills(sgs.lose_equip_skill) and self:evaluateArmor(card) > -5 and #self.enemies > 1 then
		use.card = card
		return
	end
	if self.player:getHandcardNum() == 1 and self:needKongcheng() and self:evaluateArmor(card) > -5 then
		use.card = card
		return
	end
	local same = self:getSameEquip(card)
	local zzzh, isfriend_zzzh, isenemy_zzzh = self.room:findPlayerBySkillName("guzheng")
	if zzzh then
		if self:isFriend(zzzh) then isfriend_zzzh = true
		else isenemy_zzzh = true
		end
	end
	if same then
		if (self:hasSkill("nosgongqi") and self:slashIsAvailable())
			or (self.player:hasSkill("nosrende") and self:findFriendsByType(sgs.Friend_Draw))
			or (self.player:hasSkill("rende") and not self.player:hasUsed("RendeCard") and self:findFriendsByType(sgs.Friend_Draw))
			or (self.player:hasSkills("yongsi|renjie") and self:getOverflow() < 2)
			or (self.player:hasSkills("qixi|duanliang|yinling") and (card:isBlack() or same:isBlack()))
			or (self.player:hasSkills("guose") and (card:getSuit() == sgs.Card_Diamond or same:getSuit() == sgs.Card_Diamond))
			or (self.player:hasSkill("jijiu") and (card:isRed() or same:isRed()))
			or (self.player:hasSkill("guidao") and same:isBlack() and card:isRed())
			or self.player:hasSkill("junxing")
			or isfriend_zzzh
			then return end
	end
	local canUseSlash = self:getCardId("Slash") and self:slashIsAvailable(self.player)
	self:useCardByClassName(card, use)
	
	if use.card then return end
	if card:isKindOf("Weapon") then
		if self:needBear() then return end
		if self.player:hasSkill("jiehuo") and self.player:getMark("jiehuo") < 0 and card:isRed() then return end
		if self.player:hasSkill("zhulou") and same then return end
		if self.player:hasSkill("taichen") and same then
			local dummy_use = { isDummy = true }
			self:useSkillCard(sgs.Card_Parse("@TaichenCard=" .. same:getEffectiveId()), dummy_use)
			if dummy_use.card then return end
		end
		if same and self.player:hasSkill("qiangxi") and not self.player:hasUsed("QiangxiCard") then
			local dummy_use = { isDummy = true }
			self:useSkillCard(sgs.Card_Parse("@QiangxiCard=" .. same:getEffectiveId()), dummy_use)
			if dummy_use.card and dummy_use.card:getSubcards():length() == 1 then return end
		end
		if self.player:hasSkill("nosrende") or (self.player:hasSkill("rende") and not self.player:hasUsed("RendeCard")) then
			for _, friend in ipairs(self.friends_noself) do
				if not friend:getWeapon() then return end
			end
		end
		if self:hasSkills("paoxiao|nosfuhun", self.player) and card:isKindOf("Crossbow") then return end
		if not self:needKongcheng() and not self:hasSkills(sgs.lose_equip_skill) and self:getOverflow() <= 0 and not canUseSlash then return end
		if (not use.to) and self.weaponUsed and not self:hasSkills(sgs.lose_equip_skill) then return end
		if (self.player:hasSkill("zhiheng") or self.player:hasSkill("jilve") and self.player:getMark("@bear") > 0)
			and not self.player:hasUsed("ZhihengCard") and self.player:getWeapon() and not card:isKindOf("Crossbow") then return end
		if not self:needKongcheng() and self.player:getHandcardNum() <= self.player:getHp() - 2 then return end
		if not self.player:getWeapon() or self:evaluateWeapon(card) > self:evaluateWeapon(self.player:getWeapon()) then
			use.card = card
		end
	elseif card:isKindOf("Armor") then
		if self:needBear() and self.player:getLostHp() == 0 then return end
		local lion = self:getCard("SilverLion")
		if lion and self.player:isWounded() and not self.player:hasArmorEffect("SilverLion") and not card:isKindOf("SilverLion")
			and not (self:hasSkills("bazhen|yizhong") and not self.player:getArmor()) then
			use.card = lion
			return
		end
		if (self.player:hasSkill("nosrende") or (self.player:hasSkill("rende") and not self.player:hasUsed("RendeCard"))) and self:evaluateArmor(card) < 4 then
			for _, friend in ipairs(self.friends_noself) do
				if not friend:getArmor() then return end
			end
		end
		if self:evaluateArmor(card) > self:evaluateArmor() or isenemy_zzzh and self:getOverflow() > 0 then use.card = card end
		return
	elseif card:isKindOf("Treasure") then
		if card:isKindOf("WoodenOx") then
			local zhanghe = self.room:findPlayerBySkillName("qiaobian")
			local wuguotai = self.room:findPlayerBySkillName("ganlu")
			if (zhanghe and self:isEnemy(zhanghe)) or (wuguotai and self:isEnemy(wuguotai)) then return end
			use.card = card
		else
		    use.card = card
		end
	elseif self:needBear() then return
	elseif card:isKindOf("OffensiveHorse") then
		if self.player:hasSkill("jiehuo") and self.player:getMark("jiehuo") < 0 and card:isRed() then return end
		if (self.player:hasSkill("nosrende") or (self.player:hasSkill("rende") and not self.player:hasUsed("RendeCard"))) then
			for _,friend in ipairs(self.friends_noself) do
				if not friend:getOffensiveHorse() then return end
			end
			use.card = card
			return
		else
			if not self:hasSkills(sgs.lose_equip_skill) and self:getOverflow() <= 0 and not (canUseSlash or self:getCardId("Snatch")) then
				return
			else
				if self.lua_ai:useCard(card) then
					use.card = card
					return
				end
			end
		end
	elseif card:isKindOf("DefensiveHorse") then
		local tiaoxin = true
		if self.player:hasSkill("tiaoxin") then
			local dummy_use = { isDummy = true, defHorse = true }
			self:useSkillCard(sgs.Card_Parse("@TiaoxinCard=."), dummy_use)
			if not dummy_use.card then tiaoxin = false end
		end
		if tiaoxin and self.lua_ai:useCard(card) then
			use.card = card
		end
	elseif self.lua_ai:useCard(card) then
		use.card = card
	end
end

function SmartAI:damageMinusHp(self, enemy, type)
		local trick_effectivenum = 0
		local slash_damagenum = 0
		local analepticpowerup = 0
		local effectivefireattacknum = 0
		local basicnum = 0
		local cards = self.player:getCards("hes")
		cards = sgs.QList2Table(cards)
		for _, acard in ipairs(cards) do
			if acard:getTypeId() == sgs.Card_TypeBasic and not acard:isKindOf("Peach") then basicnum = basicnum + 1 end
		end
		for _, acard in ipairs(cards) do
			if ((acard:isKindOf("Duel") or acard:isKindOf("SavageAssault") or acard:isKindOf("ArcheryAttack") or acard:isKindOf("FireAttack"))
			and not self.room:isProhibited(self.player, enemy, acard))
			or ((acard:isKindOf("SavageAssault") or acard:isKindOf("ArcheryAttack")) and self:aoeIsEffective(acard, enemy)) then
				if acard:isKindOf("FireAttack") then
					if not enemy:isKongcheng() then
					effectivefireattacknum = effectivefireattacknum + 1
					else
					trick_effectivenum = trick_effectivenum -1
					end
				end
				trick_effectivenum = trick_effectivenum + 1
			elseif acard:isKindOf("Slash") and self:slashIsEffective(acard, enemy) and ( slash_damagenum == 0 or self:hasCrossbowEffect())
				and (self.player:distanceTo(enemy) <= self.player:getAttackRange()) then
				if not (enemy:hasSkill("xiangle") and basicnum < 2) then slash_damagenum = slash_damagenum + 1 end
				if self:getCardsNum("Analeptic") > 0 and analepticpowerup == 0
				  and not (enemy:hasArmorEffect("SilverLion") or self:hasEightDiagramEffect(enemy))
				  and not IgnoreArmor(self.player, enemy) then
					slash_damagenum = slash_damagenum + 1
					analepticpowerup = analepticpowerup + 1
				end
				if self.player:hasWeapon("GudingBlade")
					and (enemy:isKongcheng() or (self.player:hasSkill("lihun") and enemy:isMale() and not enemy:hasSkill("kongcheng")))
					and not (enemy:hasArmorEffect("SilverLion") and not IgnoreArmor(self.player, enemy)) then
					slash_damagenum = slash_damagenum + 1
				end
			end
		end
		if type == 0 then return (trick_effectivenum + slash_damagenum - effectivefireattacknum - enemy:getHp())
		else return  (trick_effectivenum + slash_damagenum - enemy:getHp()) end
	return -10
end

function SmartAI:needRende()
	return self.player:getLostHp() > 1 and self:findFriendsByType(sgs.Friend_Draw)
			and ((self.player:hasSkill("nosrende") and self.player:getMark("nosrende") < 2)
				or (self.player:hasSkill("rende") and not self.player:hasUsed("RendeCard") and self.player:getMark("rende") < 2))
end

--东方杀相关
--【无念】  延迟卖血类
--【护卫】不然坑死了  【五欲】
function getBestHp(player)
	local arr = {ganlu = 1, yinghun = 2, nosmiji = 1, xueji = 1, baobian = math.max(0, player:getMaxHp() - 3)}
	if player:hasSkill("huwei") then return 3 end
	if player:hasSkill("wunian") then
		if player:getArmor() or player:getDefensiveHorse() then
			return player:getMaxHp()-1
		end
	end
	if player:hasSkill("bllmwuyu") then
		return player:getMaxHp()-2
	end
	if player:hasSkills("banling+rengui") then
		return player:getMaxHp()-1
	end
	if player:hasSkill("jiushu") then
		return player:getMaxHp()-2
	end

	for skill,dec in pairs(arr) do
		if player:hasSkill(skill) then
			return math.max( (player:isLord() and 3 or 2) ,player:getMaxHp() - dec)
		end
	end
	if player:hasSkills("renjie+baiyin") and player:getMark("baiyin") == 0 then return (player:getMaxHp() - 1) end
	if player:hasSkills("quanji+zili") and player:getMark("zili") == 0 then return (player:getMaxHp() - 1) end
	return player:getMaxHp()
end

--东方杀相关
--保持最佳血线 和getbesthp相关
function SmartAI:needToLoseHp(to, from, isSlash, passive, recover)
	from = from or self.room:getCurrent()
	to = to or self.player
	if isSlash then
		if from:hasWeapon("IceSword") and to:getCards("hes"):length() > 1 and not self:isFriend(from, to) then
			return false
		end
	end
	if self:hasHeavySlashDamage(from, nil, to) then return false end
	if to:hasSkill("pudu") and not recover then return false end
	local n = getBestHp(to)

	if not passive then
		if to:getMaxHp() > 2 then
			if self:hasSkills("longluo|miji", to) and self:findFriendsByType(sgs.Friend_Draw, to) then n = math.min(n, to:getMaxHp() - 1) end
			if to:hasSkill("rende") and not self:willSkipPlayPhase(to) and self:findFriendsByType(sgs.Friend_Draw, to) then n = math.min(n, to:getMaxHp() - 1) end
		end
	end

	local friends = self:getFriendsNoself(to)
	local need_jieyin
	local xiangxiang = self.room:findPlayerBySkillName("jieyin")
	if xiangxiang and xiangxiang:isWounded() and self:isFriend(xiangxiang, to) and not to:isWounded() and to:isMale() then
		need_jieyin = true
		self:sort(friends, "hp")
		for _, friend in ipairs(friends) do
			if friend:isMale() and friend:isWounded() then need_jieyin = false end
		end
		if need_jieyin then n = math.min(n, to:getMaxHp() - 1) end
	end
	if recover then return to:getHp() >= n end
	return to:getHp() > n
end

function IgnoreArmor(from, to)
	if not from or not to then global_room:writeToConsole(debug.traceback()) return end
	if from:hasWeapon("QinggangSword") or to:getMark("Armor_Nullified") > 0 then
		-- or #to:getTag("Qinggang"):toStringList() > 0 then
		return true
	end
	return false
end

function SmartAI:needToThrowArmor(player)
	player = player or self.player
	if not player:getArmor() or not player:hasArmorEffect(player:getArmor():objectName()) then return false end
	if self:evaluateArmor(player:getArmor(), player) <= -2 then return true end
	if player:hasArmorEffect("SilverLion") and player:isWounded() then
		if self:isFriend(player) then
			if player:objectName() == self.player:objectName() then
				return true
			else
				return self:isWeak(player) and not player:hasSkills(sgs.use_lion_skill)
			end
		else
			return true
		end
	end
	local FS = sgs.cloneCard("fire_slash", sgs.Card_NoSuit, 0)
	if not self.player:hasSkill("moukui") and player:hasArmorEffect("Vine") and player:objectName() ~= self.player:objectName() and self:isEnemy(player)
		and self.player:getPhase() == sgs.Player_Play and self:slashIsAvailable() and not self:slashProhibit(FS, player, self.player) and not IgnoreArmor(self.player, player)
		and (self:getCard("FireSlash") or (self:getCard("Slash") and (self.player:hasWeapon("Fan") or self.player:hasSkills("lihuo|zonghuo") or self:getCardsNum("Fan") >= 1)))
		and (player:isKongcheng() or sgs.card_lack[player:objectName()]["Jink"] == 1 or getCardsNum("Jink", player, self.player) < 1) then
		return true
	end
	return false
end

--东方杀相关
--【海底】【迷彩】【纪念】
function SmartAI:doNotDiscard(to, flags, conservative, n, cant_choose)
	if not to then global_room:writeToConsole(debug.traceback()) return end
	n = n or 1
	flags = flags or "hes"
	if to:isNude() then return true end
	conservative = conservative or (sgs.turncount <= 2 and self.room:alivePlayerCount() > 2)
	local enemies = self:getEnemies(to)
	if #enemies == 1 and self:hasSkills("noswuyan|qianxun|weimu", enemies[1]) and self.room:alivePlayerCount() == 2 then conservative = false end
	if to:hasSkill("tuntian") and to:hasSkill("zaoxian") and to:getPhase() == sgs.Player_NotActive and (conservative or #self.enemies > 1) then return true end

	if cant_choose then
		if to:hasSkill("lirang") and #self.enemies > 1 then return true end
		if self:needKongcheng(to) and to:getHandcardNum() <= n then return true end
		if self:getLeastHandcardNum(to) <= n then return true end
		if self:hasSkills(sgs.lose_equip_skill, to) and to:hasEquip() then return true end
		if self:needToThrowArmor(to) then return true end
	else
		if flags:match("e") then
			if to:hasSkills("jieyin+xiaoji") and to:getDefensiveHorse() then return false end
			if to:hasSkills("jieyin+xiaoji") and to:getArmor() and not to:getArmor():isKindOf("SilverLion") then return false end
		end
		if flags == "hs" or (flags == "hes" and not to:hasEquip()) then
			if to:isKongcheng() or not self.player:canDiscard(to, "hs") then return true end
			if not self:hasLoseHandcardEffective(to) then return true end
			if to:getHandcardNum() == 1 and self:needKongcheng(to) then return true end
			if #self.friends > 1 and to:getHandcardNum() == 1 and to:hasSkill("sijian") then return false end
			if to:isWounded() and to:hasSkill("haidi")  and to:getHandcardNum() == 1 and self:isEnemy(to) then return true end
			if to:hasSkill("micai")  and to:getHandcardNum() == 1 and self:isEnemy(to) then return true end
			if to:hasSkill("jinian") and (not to:hasFlag("jinian_used"))  and to:getHandcardNum() == 1 then return true end
		elseif flags == "e" or (flags == "hes" and to:isKongcheng()) then
			if not to:hasEquip() then return true end
			if self:hasSkills(sgs.lose_equip_skill, to) then return true end
			if to:getCardCount(true) == 1 and self:needToThrowArmor(to) then return true end
		end
		if flags == "hes" and n == 2 then
			if not self.player:canDiscard(to, "e") then return true end
			if to:getCardCount(true) < 2 then return true end
			if not to:hasEquip() then
				if not self:hasLoseHandcardEffective(to) then return true end
				if to:getHandcardNum() <= 2 and self:needKongcheng(to) then return true end
			end
			if self:hasSkills(sgs.lose_equip_skill, to) and to:getHandcardNum() < 2 then return true end
			if to:getCardCount(true) <= 2 and self:needToThrowArmor(to) then return true end
		end
	end
	if flags == "hes" and n > 2 then
		if not self.player:canDiscard(to, "e") then return true end
		if to:getCardCount() < n then return true end
	end
	return false
end

function SmartAI:findPlayerToDiscard(flags, include_self, isDiscard, players, return_table)
	local player_table = {}
	if isDiscard == nil then isDiscard = true end
	local friends, enemies = {}, {}
	if not players then
		friends = include_self and self.friends or self.friends_noself
		enemies = self.enemies
	else
		for _, player in sgs.qlist(players) do
			if self:isFriend(player) and (include_self or player:objectName() ~= self.player:objectName()) then table.insert(friends, player)
			elseif self:isEnemy(player) then table.insert(enemies, player) end
		end
	end
	flags = flags or "hes"

	self:sort(enemies, "defense")
	if flags:match("e") then
		for _, enemy in ipairs(enemies) do
			if self.player:canDiscard(enemy, "e") then
				local dangerous = self:getDangerousCard(enemy)
				if dangerous and (not isDiscard or self.player:canDiscard(enemy, dangerous)) then
					table.insert(player_table, enemy)
				end
			end
		end
		for _, enemy in ipairs(enemies) do
			if enemy:hasArmorEffect("EightDiagram") and not self:needToThrowArmor(enemy) and self.player:canDiscard(enemy, enemy:getArmor():getEffectiveId()) then
				table.insert(player_table, enemy)
			end
		end
	end

	if flags:match("j") then
		for _, friend in ipairs(friends) do
			if ((friend:containsTrick("indulgence") and not friend:hasSkill("keji")) or friend:containsTrick("supply_shortage"))
				and not friend:containsTrick("YanxiaoCard") and not (friend:hasSkill("qiaobian") and not friend:isKongcheng())
				and (not isDiscard or self.player:canDiscard(friend, "j")) then
				table.insert(player_table, friend)
			end
		end
		for _, friend in ipairs(friends) do
			if friend:containsTrick("lightning") and self:hasWizard(enemies, true) and (not isDiscard or self.player:canDiscard(friend, "j")) then table.insert(player_table, friend) end
		end
		for _, enemy in ipairs(enemies) do
			if enemy:containsTrick("lightning") and self:hasWizard(enemies, true) and (not isDiscard or self.player:canDiscard(enemy, "j")) then table.insert(player_table, enemy) end
		end
	end

	if flags:match("e") then
		for _, friend in ipairs(friends) do
			if self:needToThrowArmor(friend) and (not isDiscard or self.player:canDiscard(friend, friend:getArmor():getEffectiveId())) then
				table.insert(player_table, friend)
			end
		end
		for _, enemy in ipairs(enemies) do
			if self.player:canDiscard(enemy, "e") then
				local valuable = self:getValuableCard(enemy)
				if valuable and (not isDiscard or self.player:canDiscard(enemy, valuable)) then
					table.insert(player_table, enemy)
				end
			end
		end
		for _, enemy in ipairs(enemies) do
			if self:hasSkills("jijiu|beige|mingce|weimu|qingcheng", enemy) and not self:doNotDiscard(enemy, "e") then
				if enemy:getDefensiveHorse() and (not isDiscard or self.player:canDiscard(enemy, enemy:getDefensiveHorse():getEffectiveId())) then table.insert(player_table, enemy) end
				if enemy:getArmor() and not self:needToThrowArmor(enemy) and (not isDiscard or self.player:canDiscard(enemy, enemy:getArmor():getEffectiveId())) then table.insert(player_table, enemy) end
				if enemy:getOffensiveHorse() and (not enemy:hasSkill("jijiu") or enemy:getOffensiveHorse():isRed()) and (not isDiscard or self.player:canDiscard(enemy, enemy:getOffensiveHorse():getEffectiveId())) then
					table.insert(player_table, enemy)
				end
				if enemy:getWeapon() and (not enemy:hasSkill("jijiu") or enemy:getWeapon():isRed()) and (not isDiscard or self.player:canDiscard(enemy, enemy:getWeapon():getEffectiveId())) then
					table.insert(player_table, enemy)
				end
			end
		end
	end

	if flags:match("h") then
		for _, enemy in ipairs(enemies) do
			local cards = sgs.QList2Table(enemy:getHandcards())
			local flag = string.format("%s_%s_%s","visible", self.player:objectName(), enemy:objectName())
			if #cards <= 2 and not enemy:isKongcheng() and not (enemy:hasSkills("tuntian+zaoxian") and enemy:getPhase() == sgs.Player_NotActive) then
				for _, cc in ipairs(cards) do
					if (cc:hasFlag("visible") or cc:hasFlag(flag)) and (cc:isKindOf("Peach") or cc:isKindOf("Analeptic")) and (not isDiscard or self.player:canDiscard(enemy, cc:getId())) then
						table.insert(player_table, enemy)
					end
				end
			end
		end
	end

	if flags:match("e") then
		for _, enemy in ipairs(enemies) do
			if enemy:hasEquip() and not self:doNotDiscard(enemy, "e") and (not isDiscard or self.player:canDiscard(enemy, "e")) then
				table.insert(player_table, enemy)
			end
		end
	end

	if flags:match("j") then
		for _, enemy in ipairs(enemies) do
			if enemy:containsTrick("YanxiaoCard") and (not isDiscard or self.player:canDiscard(enemy, "j")) then table.insert(player_table, enemy) end
		end
	end

	if flags:match("h") then
		self:sort(enemies, "handcard")
		for _, enemy in ipairs(enemies) do
			if (not isDiscard or self.player:canDiscard(enemy, "hs")) and not self:doNotDiscard(enemy, "hs") then
				table.insert(player_table, enemy)
			end
		end
	end

	if flags:match("h") then
		local zhugeliang = self.room:findPlayerBySkillName("kongcheng")
		if zhugeliang and self:isFriend(zhugeliang) and zhugeliang:getHandcardNum() == 1 and self:getEnemyNumBySeat(self.player, zhugeliang) > 0
			and zhugeliang:getHp() <= 2 and (not isDiscard or self.player:canDiscard(zhugeliang, "hs")) then
			table.insert(player_table, zhugeliang)
		end
	end
	if return_table then return player_table
	else
		if #player_table == 0 then return nil else return player_table[1] end
	end
end

function SmartAI:findPlayerToDraw(include_self, drawnum)
	drawnum = drawnum or 1
	local players = sgs.QList2Table(include_self and self.room:getAllPlayers() or self.room:getOtherPlayers(self.player))
	local friends = {}
	for _, player in ipairs(players) do
		if self:isFriend(player) and not hasManjuanEffect(player)
			and not (player:hasSkill("kongcheng") and player:isKongcheng() and drawnum <= 2) then
			table.insert(friends, player)
		end
	end
	if #friends == 0 then return end

	self:sort(friends, "defense")
	for _, friend in ipairs(friends) do
		if friend:getHandcardNum() < 2 and not self:needKongcheng(friend) and not self:willSkipPlayPhase(friend) then
			return friend
		end
	end

	local AssistTarget = self:AssistTarget()
	if AssistTarget then
		for _, friend in ipairs(friends) do
			if friend:objectName() == AssistTarget:objectName() and not self:willSkipPlayPhase(friend) then
				return friend
			end
		end
	end

	for _, friend in ipairs(friends) do
		if self:hasSkills(sgs.cardneed_skill, friend) and not self:willSkipPlayPhase(friend) then
			return friend
		end
	end

	self:sort(friends, "handcard")
	for _, friend in ipairs(friends) do
		if not self:needKongcheng(friend) and not self:willSkipPlayPhase(friend) then
			return friend
		end
	end
	return nil
end

function SmartAI:dontRespondPeachInJudge(judge)
	if not judge or type(judge) ~= "userdata" then self.room:writeToConsole(debug.traceback()) return end
	local peach_num = self:getCardsNum("Peach")
	if peach_num == 0 then return false end
	if self:willSkipPlayPhase() and self:getCardsNum("Peach") > self:getOverflow(self.player, true) then return false end

	local card = self:getCard("Peach")
	local dummy_use = { isDummy = true }
	self:useBasicCard(card, dummy_use)
	if dummy_use.card then return true end

	if peach_num <= self.player:getLostHp() then return true end

	if peach_num > self.player:getLostHp() then
		for _, friend in ipairs(self.friends) do
			if self:isWeak(friend) then return true end
		end
	end

	--judge.reason:baonue,neoganglie,ganglie,caizhaoji_hujia
	if judge.reason == "tuntian" and judge.who:getMark("zaoxian") == 0 and judge.who:getPile("field"):length() < 2 then return true
	elseif (judge.reason == "EightDiagram" or judge.reason == "bazhen") and
		self:isFriend(judge.who) and not self:isWeak(judge.who) then return true
	elseif judge.reason == "nosmiji" and judge.who:getLostHp() == 1 then return true
	elseif judge.reason == "tieji" then return true
	elseif judge.reason == "beige" then return true
	end

	return false
end

function CanUpdateIntention(player)
	if not player then global_room:writeToConsole(debug.traceback()) end
	local current_rebel_num, current_loyalist_num = 0, 0
	local rebel_num = sgs.current_mode_players["rebel"]

	for _, aplayer in sgs.qlist(global_room:getAlivePlayers()) do
		if sgs.ai_role[aplayer:objectName()] == "rebel" then current_rebel_num = current_rebel_num + 1 end
	end

	if sgs.ai_role[player:objectName()] == "rebel" and current_rebel_num >= rebel_num then return false
	elseif sgs.ai_role[player:objectName()] == "neutral" and current_rebel_num + 1 >= rebel_num then return false end

	return true
end

--专门辅助人类？
function SmartAI:AssistTarget()
	if sgs.ai_AssistTarget_off then return end
	local human_count, player = 0
	if not sgs.ai_AssistTarget then
		for _, p in sgs.qlist(self.room:getAlivePlayers()) do
			if p:getState() ~= "robot" then
				human_count = human_count + 1
				player = p
			end
		end
		if human_count == 1 and player then
			sgs.ai_AssistTarget = player
		else
			sgs.ai_AssistTarget_off = true
		end
	end
	if player and not player:getAI() and player:isAlive() and self:isFriend(player) and player:objectName() ~= self.player:objectName() then return player end
	return
end

function SmartAI:findFriendsByType(prompt, player)
	player = player or self.player
	local friends = self:getFriendsNoself(player)
	if #friends < 1 then return false end
	if prompt == sgs.Friend_Draw then
		for _, friend in ipairs(friends) do
			if not friend:hasSkill("manjuan") and not self:needKongcheng(friend, true) then return true end
		end
	elseif prompt == sgs.Friend_Male then
		for _, friend in ipairs(friends) do
			if friend:isMale() then return true end
		end
	elseif prompt == sgs.Friend_MaleWounded then
		for _, friend in ipairs(friends) do
			if friend:isMale() and friend:isWounded() then return true end
		end
	elseif prompt == sgs.Friend_All then
		return true
	else
		global_room:writeToConsole(debug.traceback())
		return
	end
	return false
end

function hasBuquEffect(player)
	return (player:hasSkill("buqu") and player:getPile("buqu"):length() <= 4) or (player:hasSkill("nosbuqu") and player:getPile("nosbuqu"):length() <= 4)
end

---*****由此开始为东方杀自建的ai函数
function SmartAI:touhouDamageNature(card,from,to)
	local nature=sgs.DamageStruct_Normal
	if not card then return nature end
	if  card:isKindOf("FireSlash") then
		nature= sgs.DamageStruct_Fire
	end
	if card:isKindOf("ThunderSlash") then
		nature= sgs.DamageStruct_Thunder
	end
	if  card:isKindOf("FireAttack") then
		nature= sgs.DamageStruct_Fire
	end
	if  card:isKindOf("IceSlash") then
		nature= sgs.DamageStruct_Ice
	end
	if card:isKindOf("Slash") then
		if (from and from:hasSkill("here")) or
		 (to and to:hasSkill("here")) then
			nature= sgs.DamageStruct_Fire
		end
	end

	return nature
end

--东方杀相关
--原创 完全针对东方杀的【伤害计算】
function SmartAI:touhouDamage(original_damage,from,to,step)
	local damage = original_damage
	if to:hasSkill("huanmeng") then
		damage.damage=0
		return damage
	end
	step = step or 1

	if (damage.card and damage.card:isKindOf("Slash") and from) then
		local baka = self.room:findPlayerBySkillName("wushen")
		if (baka and from:getWeapon()) then from = baka end
	end
	if step <= 1 then
	--分别对应伤害各个时机
		damage=self:touhouConfirmDamage(damage,from,to)
	end
	-- Predamage,
	--DamageForseen,
	if step <=2 then
		damage=self:touhouDamageCaused(damage,from,to)
	end
	if step <=3 then
		damage=self:touhouDamageInflicted(damage,from,to)
	end
	return damage
end

function SmartAI:touhouConfirmDamage(damage,from,to)
	--基本卡牌
	if damage.card and not damage.chain and not damage.transfer then
		if damage.card:isKindOf("Slash") then
			if (damage.card:hasFlag("drank")) then
				damage.damage=damage.damage+1
			elseif from and from:getMark("drank") > 0 then
				damage.damage=damage.damage+from:getMark("drank")
			end
		end
		--【魔法】加成
		if damage.card:hasFlag("mofa_card") then --这是已经使用了卡。。。如果是player的dummyuse呢？？？
			damage.damage=damage.damage+1
		end
		--【浴血】加成
		if damage.card:hasFlag("yuxueinvoked") then
			damage.damage=damage.damage+1
		end
		if damage.card:hasFlag("jidu_card") then
			damage.damage=damage.damage+1
		end
	end
	return damage
	--由于confirm时不需要check技能，所以没有用查询技能列表的方式，直接在此函数枚举
end
--东方杀相关
--造成伤害时
function SmartAI:touhouDamageCaused(damage,from,to)
	if from and from:isAlive() then
		for _, askill in sgs.qlist(from:getVisibleSkillList()) do
			local s_name = askill:objectName()
			if  from:hasSkill(s_name) then --需要check技能无效
				local filter = sgs.ai_damageCaused[s_name]
				if filter and type(filter) == "function"  then
					damage= filter(self, damage)
				end
			end
		end
	end
	--check 基本卡牌
	if damage.card and not damage.chain and not damage.transfer then
		if from and to:isKongcheng() and from:hasWeapon("GudingBlade") then
			damage.damage=damage.damage+1
		end
	end
	return damage
end

--东方杀相关
-- 可能承受此伤害时+可能防止伤害
function SmartAI:touhouDamageInflicted(damage,from,to)

	--幻月的时机真蛋疼。。。比其他防止伤害系可能早触发 也可能晚触发
	--而且本身也只是一个可能事件 不是必然事件。。。
	--[[if to:getMark("@huanyue")>0 and damage.card and damage.card:isKindOf("TrickCard") then
		local neet= self.room:findPlayerBySkillName("huanyue")
		if neet and neet:getHandcardNum()>3 and self:isEnemy(neet,to) then
			damage.damage=damage.damage+1
		end
	end]]
	--限定技终焉也不是必然事件 蛋疼。。。
	--变幻也不是必然事件。。。
	--折射。。正体。。。一个比一个蛋疼。。。

	--能正常衡量的只有必然减少伤害的事件
	for _, askill in sgs.qlist(to:getVisibleSkillList()) do
		local s_name = askill:objectName()
		if  to:hasSkill(s_name) then--需要check技能无效
			local filter = sgs.ai_damageInflicted[s_name]
			if filter and type(filter) == "function"  then
				damage= filter(self, damage)
			end
			if damage.damage<1 then
				damage.damage=0
				return damage
			end
		end
	end

	--技能优先于藤甲等防具
	if from and not self:touhouIgnoreArmor(damage.card,from,to)  then
		if to:hasArmorEffect("Vine") then
			if damage.nature == sgs.DamageStruct_Fire  then
				damage.damage=damage.damage+1
			end
		end
		if to:hasArmorEffect("SilverLion")  then
			damage.damage=1
		end
		if to:hasArmorEffect("Camouflage") then
			local camouflage = true
			for _,p in sgs.qlist(self.room:getAlivePlayers()) do
				local armor = p:getArmor()
				if armor and armor:objectName() ~= "Camouflage" then
					camouflage = false
					break
				end
			end
			if camouflage then damage.damage = 0 end
		end
	end

	if damage.damage<1 then
		damage.damage=0
		return damage
	end

	--不灭特殊 比藤甲时机还靠后  采用枚举
	if to:hasSkill("bumie") then
		if damage.damage>= to:getHp() then
			damage.damage=to:getHp()-1
		end
	end

	if damage.damage<1 then damage.damage=0 end
	return damage
end
--东方杀相关
--造成伤害时有特殊效果的情况  和 slashIsEffective 有明显关联。。。
function SmartAI:touhouDamageEffect(damage,from,to)
	if not from or not to then return false, false end
	if to:hasSkill("huanmeng") then return false, false end
	local hasEffect = false
	local willUse = true
	--有时间需要好好整理willuse的情况


	if damage.card  then
		if from:hasSkill("dongjie")  and not from:hasFlag("dongjie") then
			--第四个参数ignoreDamageEffect 一定要是true 否则无限嵌套
			if (damage.damage >= to:getHp() and self:isEnemy(from, to)) then
				willUse = not self:touhouNeedAvoidAttack(damage, from, to, true)
			end
			return true, willUse
		end
		if  damage.card:isKindOf("Slash") then
			--if self:sidieEffect(from) then
			--	return true, willUse
			--end
			if from:hasWeapon("IceSword") and not to:isNude() then
				return true, willUse
			end
		end
	end
	if from:hasSkill("lizhi") then
		return true, willUse
	end
	if from:hasSkill("shenyin") and not to:isNude() then
		return true, willUse
	end
	if from:hasSkill("huanming")  and from:getMark("huanming") == 0 then
		if  not self:isFriend(from,to) and to:getHp() > from:getHp() then
			return true, willUse
		end
	end
	if from:hasSkill("zuosui")  then
		if self:isFriend(from,to) then
			if to:getCards("hes"):length()<=4 then
				return true, willUse
			end
		else
			return true, willUse
		end

	end
	if from:hasSkill("lianmu") and not from:hasFlag("lianmu_used")  then
		if damage.card and damage.card:isKindOf("Slash")  and not damage.card:hasFlag("lianmu_damage")then
			return true, willUse
		end
	end
	return false, willUse
end


--东方杀相关
--无效类
--【正义】【云上】【野性】【天仪】
--“仁王”藤甲
--先天性无效
function SmartAI:touhouEffectNullify(card,from,to)
	if not card then return false end
		--if card:isBlack() and to:hasSkill("zhengyi") then
		--  return true
		--end
		if card:isKindOf("TrickCard") then
			if to:hasSkill("yunshang") and  from:objectName() ~= to:objectName() and not from:inMyAttackRange(to) then
				return true
			end
		end
		if card:isKindOf("Slash") then
			if card:getSuit()==  sgs.Card_Spade and to:getMark("tianyi_spade")>0then
				return true
			end
			if card:getSuit()==  sgs.Card_Heart and to:getMark("tianyi_heart")>0 then
				return true
			end
			if card:getSuit()==  sgs.Card_Club and to:getMark("tianyi_club")>0then
				return true
			end
			if card:getSuit()==  sgs.Card_Diamond and to:getMark("tianyi_diamond")>0then
				return true
			end
		end
		if to:hasSkill("yexing") and to:getMark("@shi") ==0 then
			if card:isKindOf("Slash") and not card:isKindOf("NatureSlash") then
				return true
			end
			if card:isKindOf("SavageAssault") or card:isKindOf("ArcheryAttack")then
				return true
			end
		end


		--防具相关
		if to:hasArmorEffect("Vine") then
			if card:isKindOf("Slash") and not card:isKindOf("NatureSlash") and not self:touhouIgnoreArmor(card,from,to) then
				return true
			end
			if card:isKindOf("SavageAssault") or card:isKindOf("ArcheryAttack")then
				return true
			end
		end
		if to:hasArmorEffect("IronArmor") then
			if card:isKindOf("NatureSlash") or card:isKindOf("IronChain") or card:isKindOf("FireAttack")then
				return true
			end
		end
		if to:hasArmorEffect("RenwangShield") and not self:touhouIgnoreArmor(card,from,to) then
			--[[if not self:isFriend(to,from) and from:hasSkill("guaili")
			and from:getHandcardNum()>1 and card:isKindOf("Slash") then
				return false
			end]]
			if  card:isKindOf("Slash") and card:isBlack() then
				return true
			end
		end
	return false
end

--判定一张卡牌是否已经对目标无效
function SmartAI:touhouCardUseEffectNullify(use,target)
	for _, name in ipairs(use.nullified_list) do
		if name == target:objectName() or name == "_ALL_TARGETS" then
			return true
		end
	end
	--仅考虑了use系列时机下，没有考虑effect类
	--effect.to->setFlags("Global_NonSkillNullify")
	--野性 天仪这种不在card这里设flag的 需要枚举？？ 还是另立函数？
	return false
end

--东方杀相关
--卖血有益类
--还应该结合血本身的价值
--没考虑伤害无效 防止等只赚了一牌差的情况
function SmartAI:touhouDamageBenefit(damage,from,to)
	local benefitF=0
	local benefitE=0
	local benefitTo=-2
	local benefitFrom=0
	if to:hasSkill("wangyue") then
		if self:isEnemy(from,to) then
		end
	end
end


function SmartAI:touhouDamageProhibit(damage,from,to)
	for _, askill in sgs.qlist(to:getVisibleSkillList()) do
		local s_name = askill:objectName()
		if  to:hasSkill(s_name) then--需要check技能无效
			local filter = sgs.ai_damage_prohibit[s_name]
			if filter and type(filter) == "function" and filter(self, from, to, damage) then
				return true
			end
		end
	end
	return false
end

--东方杀相关
--评估血的价值
function SmartAI:touhouHpValue(player)

end

--东方杀相关
--铁锁伤害计算
--铁锁中的正体transfer会记为0 对于转移目标为非铁锁状态的暂时不管。。。
function SmartAI:touhouChainDamage(damage,from,to)
	local x=0
	local y=0
	damage=self:touhouDamage(damage,from,to)
	if self:isFriend(to) then
		y=damage.damage
	end
	if self:isEnemy(to) then
		x=damage.damage
	end
	if damage.nature==sgs.DamageStruct_Normal  or not damage.chain  or not to:isChained() then
		return x,y
	end
	if self:touhouBreakDamage(damage,to) then
		return x,y
	end
	--开始传导铁锁
	damage.chain=true
	local tos = sgs.SPlayerList()
	for _,p in sgs.qlist(self.room:getOtherPlayers(to)) do
		if p:isChained() then
			tos:append(p)
		end
	end
	if not tos:isEmpty() then
		self.room:sortByActionOrder(tos)
		for _,p  in sgs.qlist(tos) do
			local real_damage=self:touhouDamage(damage,from,p)
			if self:isFriend(to,p) then
				y=y+real_damage.damage
			end
			if self:isEnemy(to,p) then
				x=x+real_damage.damage
			end
			if self:touhouBreakDamage(real_damage,p) then
				return x,y
			end
		end
	end
	return x,y
end

function SmartAI:touhouIgnoreArmor(card,from,to)
	if from:hasWeapon("QinggangSword") then
		return true
	end
	if not card then return false end
	return false
end

--东方杀相关
--【死蝶】
--【白楼】【楼观】【怪力】【幻视】
--[[function SmartAI:touhouSidieTarget(card,from)
	local targets={}
	if from:getPhase() ~=sgs.Player_Play then
		return targets
	end
	for _,p in sgs.qlist(self.room:getOtherPlayers(from)) do
		if self:isFriend(from,p) and from:canSlash(p,card,true) then
			if p:hasSkills("guaili|shuangren|huwei|lianxi|shende") then
				table.insert(targets,p:objectName())
			end
		end
	end
	return targets
end]]

--东方杀相关
--所有补拍流 弃牌流都要注意的函数
--【永恒】
function SmartAI:touhouHandCardsFix(player)
	if player:hasSkill("yongheng") and player:getPhase()==sgs.Player_NotActive then
		return true
	end
	return false
end

--【永恒】【吸散】【死宅】【集厄】【控牌】
function SmartAI:touhouDrawCardsInNotActive(player)
	if player:hasSkills("yongheng|xisan|sizhai|jie|kongpiao") then
		return true
	end
	return false
end

--【悔悟】【护卫】【神德】
--伤害为0系列
--还应该加入卖血收益系列？？
---万件+穿壁+虫子
function SmartAI:touhouGetAoeValueTo(card, to, from)
	local value=0
	local current=self.room:getCurrent()
	local weight=1
	if current:hasSkill("weiya") then weight=2 end
	if to:hasSkill("huiwu") and self:isFriend(to,from) then value = 100 end
	if card:isKindOf("SavageAssault") then
		if to:hasSkill("huwei")   then
			value = value + 120*weight
		end
		if to:hasSkill("shende") and  getCardsNum("Slash", to, from) >= 1  then
			value = value + 20
		end
	end
	if card:isKindOf("ArcheryAttack") then
		if to:hasSkill("chuanbi") and current and not current:getWeapon() then
			value = value + 50
		end
		if to:hasSkill("yingguang") and to:getMark("@yingguang")>0 then
			value = value + 50
			if to:hasSkill("yangchong") then
				value = value + 50*weight
			end
		end
	end
	return value
end


function SmartAI:touhouDelayTrickBadTarget(card, to, from)
	from = from or self.player

	for _, askill in sgs.qlist(to:getVisibleSkillList()) do
		local s_name = askill:objectName()
		local filter = sgs.ai_trick_prohibit[s_name]
		if filter and type(filter) == "function"
		and filter(self, from, to, card) then
			return true
		end
	end

	if card:isKindOf("SupplyShortage") then
		if to:hasSkills("huanmeng|songjing|guoke") then
			return true
		end
	end
	return false
end
--用于体力流失的评估
function SmartAI:touhouHpLocked(player)
	if player:hasSkill("huanmeng") then return true end
	if player:getHp()==1 and player:hasSkill("bumie") then
		return true
	end
	return false
end
--目前仅仅为防止斗酒的损失而建立
--还有很大改进余地
function SmartAI:touhouIsPriorUseOtherCard(player,card,phase)
	if not player or not card then return false end

	phase = phase or sgs.Player_Play
	local cardtypes ={"Slash","Analeptic","Peach",
	"AmazingGrace","GodSalvation","SavageAssault","ArcheryAttack",
	"Duel","ExNihilo","Snatch","Dismantlement","Collateral","IronChain",
	"FireAttack","SupplyShortage","Indulgence","Lightning",
	"EquipCard"}
	local types={}
	for _,t in pairs (cardtypes)do
		if not card:isKindOf(t) and getCardsNum(t, player, self.player)>0 then
			table.insert(types,t)
		end
	end
	for _,c in sgs.qlist(player:getCards("hs"))do
		if not (c:getId() == card:getId()) then
			for _,t in pairs(types) do
				if c:isKindOf(t) then
					if self:touhouDummyUse(player,c) then
						return true
					end
				end
			end
		end
	end

	return false
end

function SmartAI:touhouDummyUse(player,card)
	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	if not card:isAvailable(player) then
		return false
	end
	if card:isKindOf("Slash") then
		if sgs.Slash_IsAvailable(player)  then
			self:useBasicCard(card, dummy_use)
			return not dummy_use.to:isEmpty()
		end
	--elseif card:isKindOf("Peach") then
	--  return player:isWounded()
	elseif card:isKindOf("AOE") or card:isKindOf("GlobalEffect") then
		return true
	elseif card:isKindOf("TrickCard") then
		self:useTrickCard(card, dummy_use)
		return not dummy_use.to:isEmpty()
	end
	return false
end
--东方规则下的判断雌雄剑
function SmartAI:touhouIsSameWithLordKingdom(player)
	player= player or self.player
	local lord =self.room:getLord()
	return lord:getKingdom()==player:getKingdom()
end

--主要用于要求出杀闪等respone时
function SmartAI:touhouNeedAvoidAttack(damage,from,to,ignoreDamageEffect, damageStep)
	if to:hasSkill("xuying") and to:getHandcardNum() > 0 and damage.card and damage.card:isKindOf("Slash") then return true end
	ignoreDamageEffect = ignoreDamageEffect or false
	damageStep = damageStep or 1
	local effect, willEffect = false, false
	if not ignoreDamageEffect then
		effect, willEffect = self:touhouDamageEffect(damage,from,to)
		if from and effect and self:isFriend(from,to) then
			return false
		end
	end

	local real_damage = self:touhouDamage(damage,from,to, damageStep)

	if real_damage.damage<1 then
		if not from  or not effect then
			return false
		end
	else
		local recover = self:touhouRecoverAfterAttack(real_damage, to)
		if not effect and to:getHp() <= to:getHp()-real_damage.damage+recover then
			return false
		end
	end
	return true
end
function SmartAI:touhouCardAttackWaste(card,from,to)
	local fakeDamage=sgs.DamageStruct()
	fakeDamage.card=card
	fakeDamage.nature= self:touhouDamageNature(card,from,to)
	fakeDamage.damage=1
	fakeDamage.from=from
	fakeDamage.to=to

	return not self:touhouNeedAvoidAttack(fakeDamage,from,to)
end

--目前只有【常青】【狡黠】
function SmartAI:touhouRecoverAfterAttack(damage,player)
	player = player or self.player
	if player:hasSkill("changqing") and damage.damage ==player:getHp() and self.room:getAlivePlayers():length()>=5 then
		return 1
	end
	if player:hasSkill("jiaoxia")  and player:getCards("e"):length()==0
	and damage.damage ==player:getHp() and damage.damage == 1 and self:invokeTouhouJudge(player) then
		return 1
	end
	return 0
end

function SmartAI:touhouHasLightningBenefit(player)
	if player:hasSkills("feixiang|mingyun|mingyun|jingdian") then
		return true
	end
	return false
end
-- a test for predict role by parsing skill property
function SmartAI:roleParse()--为主忠盲狙  自动增加克制主公的明反的仇恨
	local lord = self.room:getLord()
	if not lord then return end
	--可以转化为列表型
	if  lord:hasSkills("shanji|jingjie|shende") then
		for _,p in sgs.qlist(self.room:getOtherPlayers(lord)) do
			if p:hasSkills("changshi") then
				sgs.updateIntention(p, lord, 150)
			end
		end
	end
	if  lord:hasSkills("yongheng") then
		for _,p in sgs.qlist(self.room:getOtherPlayers(lord)) do
			if p:hasSkills("fengrang|maihuo|shitu|jiyi|banyue|huwei|baochun|mocao") then
				sgs.updateIntention(p, lord, 150)
			end
		end
	end
	local minoriko = self.room:findPlayerBySkillName("fengrang")
	if minoriko and not minoriko:isLord() and  minoriko:getSeat() - 1 <= 2 then
		sgs.updateIntention(minoriko, lord, 40)
	end
	local reisen = self.room:findPlayerBySkillName("ningshi")
	if reisen and not reisen:isLord() and  reisen:getSeat() - 1 <= 2 then
		sgs.updateIntention(reisen, lord, 40)
	end
	--local miko = self.room:findPlayerBySkillName("hongfo")
	--if miko and not miko:isLord() and miko:getKingdom() ~= lord:getKingdom() then
	--  sgs.updateIntention(miko, lord, 150)
	--end
	--need check player number and current role
	for _,p in sgs.qlist(self.room:getOtherPlayers(lord)) do
		for _,skill in sgs.qlist(p:getVisibleSkillList()) do
			if self:skillPropertyParse(skill,lord) == "loyalist" then
				sgs.updateIntention(p, lord, -150)
			end
		end
	end
end
function SmartAI:skillPropertyParse(skill,lord)
	for _,s in sgs.qlist(lord:getVisibleSkillList()) do
		local callback1 = sgs.ai_skillProperty[skill:objectName()]
		local callback2 = sgs.ai_skillProperty[s:objectName()]
		if callback1 and callback2 then
			if  skillPropertyCompare(callback1(self),callback2(self))== "benefit" then
				return "loyalist"
			end
		end
	end
	return nil
end

function skillPropertyCompare(skillProperty1,skillProperty2)
	if (skillProperty1 == "cause_judge" or skillProperty1 =="use_delayed_trick")  and  skillProperty2 =="wizard_harm" then
		return "benefit"
	end
	return ""
end


function SmartAI:touhouBulidJudge(reason, who)
	local player = who or self.player
	local callback = sgs.ai_judge_model[reason]
	if callback and type(callback) == "function" then
		return callback(self,  who)
	end
	return nil
end

function SmartAI:touhouGetJudges(player)
	local judgeReasons = {}
	player = player or self.room:findPlayer(self.player:getNextAlive():objectName()) --self.player:getNextAlive()
	for _, askill in sgs.qlist(player:getVisibleSkillList()) do
		local s_name = askill:objectName()
		if player:hasSkill(s_name) then--需要check技能无效
			local filter = sgs.ai_judge_model[s_name]
			if filter and type(filter) == "function"  then
				table.insert(judgeReasons, s_name)
			end
		end
	end


	local judge = player:getCards("j")
	judge = sgs.QList2Table(judge)
	judge = sgs.reverse(judge)
	for _,needjudge in pairs(judge) do
		table.insert(judgeReasons, needjudge:objectName())
	end
	return judgeReasons
end

--将木牛 票等id 加入 手牌的list中
function SmartAI:touhouAppendExpandPileToList(player,cards)
	if sgs.touhouCanWoodenOx(player) then
		for _, id in sgs.qlist(player:getPile("wooden_ox")) do
			cards:prepend(sgs.Sanguosha:getCard(id))
		end
	end
	if player:hasSkill("shanji") then
		for _, id in sgs.qlist(player:getPile("piao")) do
			cards:prepend(sgs.Sanguosha:getCard(id))
		end
	end
	--[[if player:hasSkill("xinhua") then
		for _, p in sgs.qlist(self.room:getOtherPlayers(player)) do
		    for _, id in sgs.qlist(p:getShownHandcards()) do
				cards:prepend(sgs.Sanguosha:getCard(id))
			end
		end
	end]]
	--if player:hasSkill("chaoren") then
	--  cards:prepend(sgs.Sanguosha:getCard(self.room:getDrawPile():first()))
	--end
	return cards
end
function sgs.touhouCanWoodenOx(player)
	local treasure = player:getTreasure()
	if treasure and  treasure:isKindOf("WoodenOx") and not player:isBrokenEquip(treasure:getId()) then
		return true
	end
	return false
end
function sgs.touhouAppendExpandPileToList(player,cards)
	if sgs.touhouCanWoodenOx(player) then
		for _, id in sgs.qlist(player:getPile("wooden_ox")) do
			cards:prepend(sgs.Sanguosha:getCard(id))
		end
	end
	if player:hasSkill("shanji") then
		for _, id in sgs.qlist(player:getPile("piao")) do
			cards:prepend(sgs.Sanguosha:getCard(id))
		end
	end
	return cards
end

function SmartAI:touhouNeedToWake(player)
	player = player or self.player
	local wake_timing="UnKnown"
	local wake_mode="UnKnown"
	--通用类
	--trick_prohibit更多用于表示badtarget 不代表完全不能对该目标使用
	for _, askill in sgs.qlist(player:getVisibleSkillList()) do
		local s_name = askill:objectName()
		--要求 已觉醒标记名 等于 技能名
		if askill:getFrequency() == sgs.Skill_Wake and player:getMark(s_name)==0 then
			local filter = sgs.ai_needToWake[s_name]
			if filter and type(filter) == "function" then
				wake_mode,wake_timing=filter(self,player)
			end
		end
	end
	return wake_mode,wake_timing
end

--暂时仿照原FindPlayerToDraw
--其实我更想把这个值量化
function SmartAI:touhouFindPlayerToDraw(include_self, drawnum, players)
	drawnum = drawnum or 1
	if not players or players:isEmpty() then
		players = sgs.QList2Table(include_self and self.room:getAllPlayers() or self.room:getOtherPlayers(self.player))
	else
		players = sgs.QList2Table(players)
	end
	local friends = {}
	for _, player in ipairs(players) do
		if self:isFriend(player) and not self:cautionRenegade(self.player, player)  then--
			if not (player:hasSkill("micai") and player:isKongcheng() and drawnum <= 2) then
				--其实不应该完全否定 所以还是量化好。。。
				if not (player:getPhase()==sgs.Player_NotActive and player:hasSkills("gaoao|yongheng")) then
					table.insert(friends, player)
				end
			end
		end
	end
	if #friends == 0 then return end

	self:sort(friends, "defense")
	for _, friend in ipairs(friends) do
		if friend:getHandcardNum() < 2 and not self:needKongcheng(friend) and not self:willSkipPlayPhase(friend) then
			return friend
		end
	end

	local AssistTarget = self:AssistTarget()
	if AssistTarget then
		for _, friend in ipairs(friends) do
			if friend:objectName() == AssistTarget:objectName() and not self:willSkipPlayPhase(friend) then
				return friend
			end
		end
	end

	for _, friend in ipairs(friends) do
		if self:hasSkills(sgs.cardneed_skill, friend) and not self:willSkipPlayPhase(friend) then
			return friend
		end
	end

	self:sort(friends, "handcard")
	for _, friend in ipairs(friends) do
		if not self:needKongcheng(friend) and not self:willSkipPlayPhase(friend) then
			return friend
		end
	end
	return nil
end
function SmartAI:touhouIsDamageCard(card)

	--local classes ={"Slash","Jink","SavageAssault","ArcheryAttack","Duel","FireAttack"}  ;
	--for _,class in pairs (classes) do
		if sgs.dynamic_value.damage_card[card:getClassName()] then
			return true
		end
	--end
	return false
end

function SmartAI:cautionRenegade(player, friend)
	if sgs.current_mode_players["renegade"] == 0  then --or self.player:getRole() == "renegade"
		return false
	end
	--if sgs.ai_role[player:objectName()] == "renegade" then
		-- return false
	--end

	local hasFakeFriend = false
	local role = self.player:getRole()
	local friends =  self:getFriends()
	if role == "loyalist" or self.player:isLord()  then
		if #friends >= sgs.current_mode_players["loyalist"]+ sgs.current_mode_players["renegade"]+sgs.current_mode_players["lord"] then
			hasFakeFriend = true
		end
	elseif role == "rebel" then
		if #friends > sgs.current_mode_players["rebel"] then
			hasFakeFriend = true
		end
	end
	if not hasFakeFriend then
		return false
	end

	local need_caution = false
	process = sgs.gameProcess(self.room)
	if role == "loyalist" or self.player:isLord()  then
		if process:match("loyal") or process == "neutral" then
			need_caution =  true
		end
	elseif role == "rebel"   then
		if process:match("rebel") or process == "neutral" then
			need_caution =  true
		end
	end
	if not need_caution then return false end


	local sort_func = {
		loyalist = function(a, b)
			return sgs.role_evaluation[a:objectName()]["loyalist"] < sgs.role_evaluation[b:objectName()]["loyalist"]
		end,
		rebel = function(a, b)
			return sgs.role_evaluation[a:objectName()]["rebel"] < sgs.role_evaluation[b:objectName()]["rebel"]
		end,
		renegade = function(a, b)
			return sgs.role_evaluation[a:objectName()]["renegade"] < sgs.role_evaluation[b:objectName()]["renegade"]
		end
	}

	local friends_noself = self:getFriendsNoself(player)
	if  #friends_noself == 0 then return false end
	if role == "loyalist" or self.player:isLord()  then
		table.sort(friends_noself, sort_func["loyalist"])
	elseif role == "rebel"   then
		table.sort(friends_noself, sort_func["renegade"])
	end
	return  friends_noself[1]:objectName() == friend:objectName()
end

function SmartAI:trickProhibit(card, enemy, from)
	from = from or self.player
	for _, askill in sgs.qlist(enemy:getVisibleSkillList()) do
		local s_name = askill:objectName()
		if  enemy:hasSkill(s_name) then  
			local filter = sgs.ai_trick_prohibit[s_name]
			if filter and type(filter) == "function" and filter(self, from, enemy, card) then
				return true
			end
		end
	end
	return false
end

function SmartAI:touhouNeedBear(card,from,tos)
	from = from or self.player
	tos = tos or sgs.SPlayerList()
	for _, askill in sgs.qlist(from:getVisibleSkillList()) do
		local s_name = askill:objectName()
		if  from:hasSkill(s_name) then  
			local filter = sgs.ai_need_bear[s_name]
			if filter and type(filter) == "function" and filter(self, card,from,tos) then
				return true
			end
		end
	end
	return false
end


function SmartAI:touhouClassMatch(classes, class_name)
	if (classes == class_name or classes:match("sqchuangshi")) then
		return true
	end
	return false
end






-----*****由此开始为原三国杀武将包中的smart-ai
--不要对其伤害 god-ai
function SmartAI:cantbeHurt(player, from, damageNum)
	from = from or self.player
	damageNum = damageNum or 1
	local maxfriendmark = 0
	local maxenemymark = 0
	local dyingfriend = 0
	if player:hasSkill("wuhun") and not player:isLord() and #(self:getFriendsNoself(player)) > 0 then
		for _, friend in ipairs(self:getFriends(from)) do
			local friendmark = friend:getMark("@nightmare")
			if friendmark > maxfriendmark then maxfriendmark = friendmark end
		end
		for _, enemy in ipairs(self:getEnemies(from)) do
			local enemymark = enemy:getMark("@nightmare")
			if enemymark > maxenemymark and enemy:objectName() ~= player:objectName() then maxenemymark = enemymark end
		end
		if self:isEnemy(player, from) then
			if maxfriendmark + damageNum - player:getHp() / 2 >= maxenemymark and not (#(self:getEnemies(from)) == 1 and #(self:getFriends(from)) + #(self:getEnemies(from)) == self.room:alivePlayerCount())
				and not (from:getMark("@nightmare") == maxfriendmark and from:getRole() == "loyalist") then
				return true
			end
		elseif maxfriendmark + damageNum - player:getHp() / 2 > maxenemymark then
			return true
		end
	end
	if player:hasSkill("duanchang") and not player:isLord() and #(self:getFriendsNoself(player)) > 0 and player:getHp() <= 1 then
		if not (from:getMaxHp() == 3 and from:getArmor() and from:getDefensiveHorse()) then
			if from:getMaxHp() <= 3 or (from:isLord() and self:isWeak(from)) then return true end
			if from:getMaxHp() <= 3 or (self.room:getLord() and from:getRole() == "renegade") then return true end
		end
	end
	--[[if player:hasSkill("tianxiang") and getKnownCard(player, from, "diamond", false) + getKnownCard(player, from, "club", false) < player:getHandcardNum() then
		local peach_num = self.player:objectName() == from:objectName() and self:getCardsNum("Peach") or getCardsNum("Peach", from, self.player)
		for _, friend in ipairs(self:getFriends(from)) do
			if friend:getHp() < 2 and peach_num then
				dyingfriend = dyingfriend + 1
			end
		end
		if dyingfriend > 0 and player:getHandcardNum() > 0 then
			return true
		end
	end]]
	return false
end

-- god-ai
function SmartAI:needDeath(player)
	local maxfriendmark = 0
	local maxenemymark = 0
	player = player or self.player
	if player:hasSkill("wuhun") and #(self:getFriendsNoself(player)) > 0 then
		for _, aplayer in sgs.qlist(self.room:getAlivePlayers()) do
			local mark = aplayer:getMark("@nightmare")
			if self:isFriend(player,aplayer) and player:objectName() ~= aplayer:objectName() then
				if mark > maxfriendmark then maxfriendmark = mark end
			end
			if self:isEnemy(player,aplayer) then
				if mark > maxenemymark then maxenemymark = mark end
			end
			if maxfriendmark > maxenemymark then return false
			elseif maxenemymark == 0 then return false
			else return true end
		end
	end
	return false
end
--god-ai
function SmartAI:doNotSave(player)
	if (player:hasSkill("niepan") and player:getMark("@nirvana") > 0 and player:getCards("e"):length() < 2)
		or (player:hasSkill("fuli") and player:getMark("@laoji") > 0 and player:getCards("e"):length() < 2) then
		return true
	end
	if player:hasFlag("AI_doNotSave") then return true end
	return false
end
--god-ai
function SmartAI:needBear(player)
	player = player or self.player
	return player:hasSkills("renjie+baiyin") and not player:hasSkill("jilve") and player:getMark("@bear") < 4
end
--standard-ai
function SmartAI:willSkipPlayPhase(player, NotContains_Null)
	local player = player or self.player

	if player:isSkipped(sgs.Player_Play) then return true end

	local fuhuanghou = self.room:findPlayerBySkillName("zhuikong")
	if fuhuanghou and fuhuanghou:objectName() ~= player:objectName() and self:isEnemy(player, fuhuanghou)
		and fuhuanghou:isWounded() and fuhuanghou:getHandcardNum() > 1 and not player:isKongcheng() and not self:isWeak(fuhuanghou) then
		local max_card = self:getMaxCard(fuhuanghou)
		local player_max_card = self:getMaxCard(player)
		if (max_card and player_max_card and max_card:getNumber() > player_max_card:getNumber()) or (max_card and max_card:getNumber() >= 12) then return true end
	end

	local friend_null = 0
	local friend_snatch_dismantlement = 0
	local cp = self.room:getCurrent()
	if self.player:objectName() == cp:objectName() and self.player:objectName() ~= player:objectName() and self:isFriend(player) then
		for _, hcard in sgs.qlist(self.player:getCards("hes")) do
			if (isCard("Snatch", hcard, self.player) and self.player:distanceTo(player) == 1) or isCard("Dismantlement", hcard, self.player) then
				local trick = sgs.cloneCard(hcard:objectName(), hcard:getSuit(), hcard:getNumber())
				if self:hasTrickEffective(trick, player) then friend_snatch_dismantlement = friend_snatch_dismantlement + 1 end
			end
		end
	end
	if not NotContains_Null then
		for _, p in sgs.qlist(self.room:getAllPlayers()) do
			if self:isFriend(p, player) then friend_null = friend_null + getCardsNum("Nullification", p, self.player) end
			if self:isEnemy(p, player) then friend_null = friend_null - getCardsNum("Nullification", p, self.player) end
		end
	end
	if player:containsTrick("indulgence") then
		if player:containsTrick("YanxiaoCard") or self:hasSkills("keji|conghui",player) or (player:hasSkill("qiaobian") and not player:isKongcheng()) then return false end
		if friend_null + friend_snatch_dismantlement > 1 then return false end
		return true
	end
	return false
end
--standard-ai
function SmartAI:willSkipDrawPhase(player, NotContains_Null)
	local player = player or self.player
	local friend_null = 0
	local friend_snatch_dismantlement = 0
	local cp = self.room:getCurrent()
	if not NotContains_Null then
		for _, p in sgs.qlist(self.room:getAllPlayers()) do
			if self:isFriend(p, player) then friend_null = friend_null + getCardsNum("Nullification", p, self.player) end
			if self:isEnemy(p, player) then friend_null = friend_null - getCardsNum("Nullification", p, self.player) end
		end
	end
	if self.player:objectName() == cp:objectName() and self.player:objectName() ~= player:objectName() and self:isFriend(player) then
		for _, hcard in sgs.qlist(self.player:getCards("hes")) do
			if (isCard("Snatch", hcard, self.player) and self.player:distanceTo(player) == 1) or isCard("Dismantlement", hcard, self.player) then
				local trick = sgs.cloneCard(hcard:objectName(), hcard:getSuit(), hcard:getNumber())
				if self:hasTrickEffective(trick, player) then friend_snatch_dismantlement = friend_snatch_dismantlement + 1 end
			end
		end
	end
	if player:containsTrick("supply_shortage") then
		if player:containsTrick("YanxiaoCard") or self:hasSkills("shensu|jisu", player) or (player:hasSkill("qiaobian") and not player:isKongcheng()) then return false end
		if friend_null + friend_snatch_dismantlement > 1 then return false end
		return true
	end
	return false
end
--standard-ai
function SmartAI:isValuableCard(card, player)
	player = player or self.player
	if (isCard("Peach", card, player) and getCardsNum("Peach", player, self.player) <= 2)
		or (self:isWeak(player) and isCard("Analeptic", card, player))
		or (player:getPhase() ~= sgs.Player_Play
			and ((isCard("Nullification", card, player) and getCardsNum("Nullification", player, self.player) < 2 and player:hasSkills("jizhi|nosjizhi|jilve"))
				or (isCard("Jink", card, player) and getCardsNum("Jink", player, self.player) < 2)))
		or (player:getPhase() == sgs.Player_Play and isCard("ExNihilo", card, player) and not player:isLocked(card)) then
		return true
	end
	local dangerous = self:getDangerousCard(player)
	if dangerous and card:getEffectiveId() == dangerous then return true end
	local valuable = self:getValuableCard(player)
	if valuable and card:getEffectiveId() == valuable then return true end
end
--standard-ai
function SmartAI:canUseJieyuanDecrease(damage_from, player)
	if not damage_from then return false end
	local player = player or self.player
	if player:hasSkill("jieyuan") and damage_from:getHp() >= player:getHp() then
		for _, card in sgs.qlist(player:getHandcards()) do
			local flag = string.format("%s_%s_%s", "visible", self.room:getCurrent():objectName(), player:objectName())
			if player:objectName() == self.player:objectName() or card:hasFlag("visible") or card:hasFlag(flag) then
				if card:isRed() and not isCard("Peach", card, player) then return true end
			end
		end
	end
	return false
end
--thicket-ai
function SmartAI:toTurnOver(player, n, reason) -- @todo: param of toTurnOver
	if not player then global_room:writeToConsole(debug.traceback()) return end
	n = n or 0
	if self:isEnemy(player) then
		local manchong = self.room:findPlayerBySkillName("junxing")
		if manchong and self:isFriend(player, manchong) and self:playerGetRound(manchong) < self:playerGetRound(player)
			and manchong:faceUp() and not self:willSkipPlayPhase(manchong)
			and not (manchong:isKongcheng() and self:willSkipDrawPhase(manchong)) then
			return false
		end
	end
	if reason and reason == "fangzhu" and player:getHp() == 1 and sgs.ai_AOE_data then
		local use = sgs.ai_AOE_data:toCardUse()
		if use.to:contains(player) and self:aoeIsEffective(use.card, player)
			and self:playerGetRound(player) > self:playerGetRound(self.player)
			and player:isKongcheng() then
			return false
		end
	end
	if player:hasUsed("ShenfenCard") and player:faceUp() and player:getPhase() == sgs.Player_Play
		and (not player:hasUsed("ShenfenCard") and player:getMark("@wrath") >= 6 or player:hasFlag("ShenfenUsing")) then
		return false
	end
	if n > 1 and player:hasSkill("jijiu") and not hasManjuanEffect(player) then
		return false
	end
	if not player:faceUp() and not player:hasFlag("ShenfenUsing") and not player:hasFlag("GuixinUsing") then
		return false
	end
	if (self:hasSkills("jushou|neojushou|nosjushou|kuiwei", player) and player:getPhase() <= sgs.Player_Finish)
		or (player:hasSkill("lihun") and not player:hasUsed("LihunCard") and player:faceUp() and player:getPhase() == sgs.Player_Play) then
		return false
	end
	return true
end
--wind-ai
function SmartAI:canLiegong(to, from)
	from = from or self.room:getCurrent()
	to = to or self.player
	if not from then return false end
	if from:hasSkill("liegong") and from:getPhase() == sgs.Player_Play and (to:getHandcardNum() >= from:getHp() or to:getHandcardNum() <= from:getAttackRange()) then return true end
	if from:hasSkill("kofliegong") and from:getPhase() == sgs.Player_Play and to:getHandcardNum() >= from:getHp() then return true end
	return false
end
--wind-ai
function SmartAI:findLeijiTarget(player, leiji_value, slasher, latest_version)
	if not latest_version then
		return self:findLeijiTarget(player, leiji_value, slasher, 1) or self:findLeijiTarget(player, leiji_value, slasher, -1)
	end
	if not player:hasSkill(latest_version == 1 and "leiji" or "nosleiji") then return nil end
	if slasher then
		if not self:slashIsEffective(sgs.cloneCard("slash"), player, slasher, slasher:hasWeapon("QinggangSword")) then return nil end
		if slasher:hasSkill("liegong") and slasher:getPhase() == sgs.Player_Play and self:isEnemy(player, slasher)
			and (player:getHandcardNum() >= slasher:getHp() or player:getHandcardNum() <= slasher:getAttackRange()) then
			return nil
		end
		if slasher:hasSkill("kofliegong") and slasher:getPhase() == sgs.Player_Play
			and self:isEnemy(player, slasher) and player:getHandcardNum() >= slasher:getHp() then
			return nil
		end
		if not latest_version then
			if not self:hasSuit("spade", true, player) and player:getHandcardNum() < 3 then return nil end
		else
			if not self:hasSuit("black", true, player) and player:getHandcardNum() < 2 then return nil end
		end
		if not (getKnownCard(player, self.player, "Jink", true) > 0
				or (getCardsNum("Jink", player, self.player) >= 1 and sgs.card_lack[player:objectName()]["Jink"] ~= 1 and player:getHandcardNum() >= 3)
				or (not self:isWeak(player) and self:hasEightDiagramEffect(player) and not slasher:hasWeapon("QinggangSword"))) then
			return nil
		end
	end
	local getCmpValue = function(enemy)
		local value = 0
		if not self:damageIsEffective(enemy, sgs.DamageStruct_Thunder, player) then return 99 end
		if enemy:hasSkill("hongyan") then
			if latest_version == -1 then return 99
			elseif not self:hasSuit("club", true, player) and player:getHandcardNum() < 3 then value = value + 80
			else value = value + 70 end
		end
		if self:cantbeHurt(enemy, player, latest_version == 1 and 1 or 2) or self:objectiveLevel(enemy) < 3
			or (enemy:isChained() and not self:isGoodChainTarget(enemy, player, sgs.DamageStruct_Thunder, latest_version == 1 and 1 or 2)) then return 100 end
		if not sgs.isGoodTarget(enemy, self.enemies, self) then value = value + 50 end
		if not latest_version and enemy:hasArmorEffect("SilverLion") then value = value + 20 end
		if self:hasSkills(sgs.exclusive_skill, enemy) then value = value + 10 end
		if self:hasSkills(sgs.masochism_skill, enemy) then value = value + 5 end
		if enemy:isChained() and self:isGoodChainTarget(enemy, player, sgs.DamageStruct_Thunder, latest_version == 1 and 1 or 2) and #(self:getChainedEnemies(player)) > 1 then value = value - 25 end
		if enemy:isLord() then value = value - 5 end
		value = value + enemy:getHp() + sgs.getDefenseSlash(enemy, self) * 0.01
		if latest_version and player:isWounded() and not self:needToLoseHp(player) then value = value + 15 end
		return value
	end

	local cmp = function(a, b)
		return getCmpValue(a) < getCmpValue(b)
	end

	local enemies = self:getEnemies(player)
	table.sort(enemies, cmp)
	for _,enemy in ipairs(enemies) do
		if getCmpValue(enemy) < leiji_value then return enemy end
	end
	return nil
end
--wind-ai
function SmartAI:needLeiji(to, from)
	return self:findLeijiTarget(to, 50, from)
end

--bgm ai 坑爹的漫卷
function hasManjuanEffect(player)
	return player:hasSkill("manjuan") and player:getPhase() == sgs.Player_NotActive
end
--3v3
sgs.ai_skill_choice["3v3_direction"] = function(self, choices, data)
	local card = data:toCard()
	local aggressive = (card and card:isKindOf("AOE"))
	--if self:isFriend(self.player:getNextAlive()) == aggressive then return "cw" else return "ccw" end
	if self:isFriend(self.room:findPlayer(self.player:getNextAlive():objectName())) == aggressive then return "cw" else return "ccw" end
end

 sgs.ai_skill_choice["askForTriggerOrder"] = function(self, choices, data)
	local invokes = choices:split("+")
	--skillname ： owner ： invoker ： preferedTarget  ： index
	-- 次数类 比如盛宴 @shengyan:sgs1:sgs1
	--铁骑类 比如雌雄剑 @DoubleSword:sgs1:sgs1:sgs2:1  和  @DoubleSword:sgs1:sgs1:sgs3:2
	--颂威类 比如血裔 @huazhong:sgs1:sgs2  和  @huazhong:sgs1:sgs3
	for _, invoke in ipairs(invokes) do
		if (invoke ~= "cancel") and  (not invoke:match("pingyi:")) then--先触发凭依以外的卖血技能后再考虑凭依
			return invoke
		end
	end
	return invokes[1]
 end

--开始添加ai文件
dofile "lua/ai/debug-ai.lua"
dofile "lua/ai/imagine-ai.lua"
dofile "lua/ai/standard_cards-ai.lua"
dofile "lua/ai/maneuvering-ai.lua"
dofile "lua/ai/test_card-ai.lua"
dofile "lua/ai/classical-ai.lua"
--dofile "lua/ai/standard-ai.lua" (dofile "lua/ai/guanxing-ai.lua" in this)
dofile "lua/ai/chat-ai.lua"
--dofile "lua/ai/basara-ai.lua"
--dofile "lua/ai/hegemony-ai.lua"
--dofile "lua/ai/hulaoguan-ai.lua"
dofile "lua/ai/guanxing-ai.lua"

local loaded = "standard|standard_cards|maneuvering|test_card|sp"

local files = table.concat(sgs.GetFileNames("lua/ai"), " ")
local LUAExtensions = string.split(string.lower(sgs.GetConfig("LuaPackages", "")), "+")
local LUAExtensionFiles = table.concat(sgs.GetFileNames("extensions/ai"), " ")

for _, aextension in ipairs(sgs.Sanguosha:getExtensions()) do
	if table.contains(LUAExtensions, string.lower(aextension)) then
		if LUAExtensionFiles:match(string.lower(aextension)) then
			dofile("extensions/ai/" .. string.lower(aextension) .. "-ai.lua")
		end
	elseif not loaded:match(aextension) and files:match(string.lower(aextension)) then
		dofile("lua/ai/" .. string.lower(aextension) .. "-ai.lua")
	end

end

--dofile "lua/ai/sp-ai.lua"
--dofile "lua/ai/special3v3-ai.lua"

for _, ascenario in ipairs(sgs.Sanguosha:getModScenarioNames()) do
	if not loaded:match(ascenario) and files:match(string.lower(ascenario)) then
		dofile("lua/ai/" .. string.lower(ascenario) .. "-ai.lua")
	end
end
