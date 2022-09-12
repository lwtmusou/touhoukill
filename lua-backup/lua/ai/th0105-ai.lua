--魅魔
--[魅灵]
sgs.ai_skill_invoke.meiling = function(self,data)
	local damage = self.player:getTag("meiling"):toDamage()
	local will_damage = (self.player:distanceTo(damage.to)  > self.player:getLostHp())
	if (not will_damage) then
		return true
	end
	if (self:isFriend(damage.from) and damage.from:hasSkill("moli")) then
		if (not self.player:hasFlag("meiling_AIDamage")) then
			self.player:setFlags("meiling_AIDamage")
			return true
		end
	end
	local point = self:isWeak(self.player) and -2 or 0
	for _, id in sgs.qlist(damage.card:getSubcards()) do
		local card_x = sgs.Sanguosha:getEngineCard(id)
		if card_x:isKindOf("Peach") then
			point = point + 3
		elseif card_x:isKindOf("TrickCard") then
			point = point + 2
		else
			point = point + 1
		end
	end

	return point > 1
end


--冈崎梦美
--[次元]
sgs.ai_skill_choice.ciyuan = function(self, choices, data)
	local choice_table = choices:split("+")
	if choices:match("cancel") then
		if not self.player:getCards("j"):isEmpty() then
			self.room:setPlayerMark(self.player, "ciyuan_ai", 5) --"finish"
			return "judge"
		end
		if self.player:hasSkill("shigui") then
			local useCount = 0
			for _,c in sgs.qlist(self.player:getCards("hs")) do
				if c:isKindOf("EquipCard") then
					useCount = useCount + 1
					continue
				end
				local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
				if c:isKindOf("TrickCard") then
					self:useTrickCard(c, dummy_use)
				else
					self:useBasicCard(c, dummy_use)
				end
				if not dummy_use.to:isEmpty() then
					useCount = useCount + 1
				end
			end
			local draw = 4 - (self.player:getHandcardNum() - useCount)
			if draw >= 3 and self.player:getLostHp() < 3 then
				self.room:setPlayerFlag(self.player, "shigui_Not_Need_Bear")
				self.room:setPlayerMark(self.player, "ciyuan_ai", 5) --"finish"
				return "draw"
			end
			local remain = self.player:getHandcardNum() - useCount
			if self.player:isWounded() and remain <= 4 and self.player:getHandcardNum() >=2 then
				remain = math.max(remain, 2)
				self.room:setPlayerMark(self.player, "ciyuan_ai", remain - 1)
				return "play"
			end
		end
	else
		local index = self.player:getMark("ciyuan_ai")
		if index > 0 then
			return choice_table[index]
		end
	end

	if choices:match("cancel") then
		return "cancel"
	else
		return choice_table[1]
	end
end

--[时轨]
sgs.ai_skill_invoke.shigui = function(self,data)
	local phase = self.player:getPhase()
	local pahseCount = self.player:getMark("shigui")
	local diff = pahseCount - self.player:getHandcardNum()

	if diff < 2 and diff > 0 then
		if  diff > 2 then
			return true
		elseif diff == 2 then
			return not self:isWeak(self.player)
		end
	end
	if diff < 0 and diff > -2 then
		if not self.player:isWounded() then return false end
		if  math.abs(diff) <= self:getOverflow(self.player) or pahseCount <= 4 then
			return true
		elseif self:isWeak(self.player) and math.abs(diff) <= 2 then
			return true
		end
	end
	return false
end

sgs.ai_need_bear.shigui = function(self, card,from,tos)
	from = from or self.player
	local pahseCount = from:getMark("shigui")
	if from:isWounded() then
		if not from:hasFlag("shigui_Not_Need_Bear")
			and from:getHandcardNum() > pahseCount and from:getHandcardNum() == pahseCount + 1 then
			return true
		end
	end
	return false
end

--[虫洞]
sgs.ai_skill_invoke.chongdong =function(self,data)
	local target= data:toPlayer()
	if not target then  return false end
	return self:isFriend(target)
end
sgs.ai_choicemade_filter.skillInvoke.chongdong = function(self, player, args, data)
	local target =data:toPlayer()
	if target then
		if args[#args] == "yes" then
			sgs.updateIntention(player, target, -30)
		else
			sgs.updateIntention(player, target, 10)
		end
	end
end

--北白河千百合
--[侦测]
sgs.ai_skill_use["@@zhence"] = function(self, prompt)
	local change = self.player:getTag("zhence"):toPhaseChange()
	if change.to == sgs.Player_Draw or (change.to == sgs.Player_Play and self:getOverflow(self.player) < 2) then
		local card = sgs.cloneCard("fire_attack", sgs.Card_NoSuit, 0)
		card:setSkillName("zhence")
		card:deleteLater()
		local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
		self:useTrickCard(card, dummy_use)
		if not dummy_use.card then return "." end
		if not dummy_use.to:isEmpty() then
			local target_objectname = {}
			for _, p in sgs.qlist(dummy_use.to) do
				table.insert(target_objectname, p:objectName())
			end
			return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
		end
	end
	return "."
end

--[时驱]
sgs.ai_skill_choice.shiqu = function(self, choices, data)
	local current = self.room:getCurrent()
	if self:isEnemy(current) and choices:match("shiqu_discard") and self:getOverflow(current) >= 2 then
		return "shiqu_discard"
	end
	if self:isFriend(current) then
		if choices:match("shiqu_play") and current:getHandcardNum() > 3 then
			return "shiqu_play"
		end
	end
	if choices:match("shiqu_draw") then
		return "shiqu_draw"
	end
	return "cancel"
end

sgs.ai_skill_use["@@shiqu"] = function(self, prompt)
	local current = self.room:getCurrent()
	local choice = self.player:getTag("shiqu"):toString()
	local cards = self.player:getCards("hes")
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)

	if choice == "shiqu_discard" and self:isEnemy(current) then
		return "@ShiquCard=" ..cards[1]:getEffectiveId().. "->" .. current:objectName()
	end
	if choice == "shiqu_play" and self:isFriend(current) and current:getHandcardNum() > 3 then
		return "@ShiquCard=" ..cards[1]:getEffectiveId().. "->" .. current:objectName()
	end
	if choice == "shiqu_draw" then
		local target = (self:isFriend(current) and self:isWeak(current))  and current or self.player
		return "@ShiquCard=" ..cards[1]:getEffectiveId().. "->" .. target:objectName()
	end
	return "."
end
sgs.ai_card_intention.ShiquCard = function(self, card, from, tos)
	local choice = from:getTag("shiqu"):toString()
	if choice == "shiqu_discard" then
		sgs.updateIntention(from, tos[1], 80)
	elseif choice == "shiqu_play" or choice == "shiqu_draw" then
		sgs.updateIntention(from, tos[1], -80)
	end
end


--朝仓理香子
--[求索]
sgs.ai_skill_cardask["@qiusuo"] = function(self, data)
	local point_num = {}
	for i = 1, 13 do
		local n = 0
		for _,id in sgs.qlist(self.player:getPile("zhenli"))do
			if sgs.Sanguosha:getCard(id):getNumber() == i then
				n = n + 1
			end
		end
		local array={point =i, num=n}
		table.insert(point_num,array)
	end
	local compare_func = function(a, b)
		return a.num > b.num
	end
	table.sort(point_num, compare_func)


	local point = 0
	for _, t in ipairs(point_num) do
		if t.num > 0 then
			point = t.point
			break
		end
	end

	local ids = {}
	for _,id in sgs.qlist(self.player:getPile("zhenli"))do
			if sgs.Sanguosha:getCard(id):getNumber() == point then
				table.insert(ids, id)
			end
	end

	if #ids > 0 then
		return "$" .. table.concat(ids, "+")
	end
	return "."
end


--卡娜·安娜贝拉尔
--[梦消]
sgs.ai_skill_choice.mengxiao = function(self, choices, data)
	local current = self.room:getCurrent()
	if choices:match("indulgence") then
		for _,p in pairs(self.enemies) do
			if self:getOverflow(p) > 1 then
				return "indulgence"
			end
		end
	end
	if choices:match("supply_shortage") then
		return "supply_shortage"
	elseif choices:match("indulgence") then
		return "indulgence"
	end
	return "cancel"
end
sgs.ai_skill_invoke.mengxiao = function(self,data)
	local current = self.room:getCurrent()
	return current and current:isAlive() and self:isEnemy(current)
end
sgs.ai_choicemade_filter.skillInvoke.mengxiao = function(self, player, args)
	local current = self.room:getCurrent()
	if current then
		if args[#args] == "yes" then
			sgs.updateIntention(player, current, 60)
		end
	end
end
sgs.ai_skill_cardchosen.mengxiao = function(self, who, flags)
	local current = self.room:getCurrent()
	local cards= {}
	for _,c in sgs.qlist(who:getCards("j")) do
		if (not who:isProhibited(current, c) and not current:containsTrick(c:objectName())) then
			table.insert(cards, c)
		end
	end
	if #cards>0 then
		return cards[1]
	end
end
sgs.ai_trick_prohibit.mengxiao = function(self, from, to, card)
	if not card:isKindOf("DelayedTrick")  then return false end
	if self:isFriend(from,to) then return false end
	if self.room:alivePlayerCount()==2 then
		return false
	end
	local nextp = self.room:nextPlayer(from)
	while true do
		if nextp:objectName() == from:objectName() or nextp:objectName() == to:objectName() then
			break
		end
		if self:isFriend(self.player, nextp) then return true end
		nextp = self.room:nextPlayer(nextp)
	end
	return false
end

--[路标]
sgs.ai_skill_invoke.lubiao =  true
function lubiaoInvoke(self, card)
	for _,p in sgs.qlist(self.room:getAlivePlayers())do
		for _,c in sgs.qlist(p:getCards("j")) do
			if (c:getColor() == card:getColor()) then
				return true
			end
		end
	end
	return false
end
--[[sgs.ai_slash_prohibit.lubiao = function(self, from, to, card)
	if not to:hasSkill("lubiao") then return false end
	if lubiaoInvoke(self) then
		local damage=sgs.DamageStruct(card, from, to, 1, self:touhouDamageNature(card,from,to))
		return self:touhouNeedAvoidAttack(damage, from, to)
	end
	return false
end]]
sgs.ai_damageInflicted.lubiao =function(self, damage)
	if damage.card and lubiaoInvoke(self, damage.card)then
		damage.damage= damage.damage - 1
	end
	return damage
end

--旧作幽香
--[夜魇]
sgs.ai_skill_invoke.yeyan =  true
function yeyan_judge(self,target,card)
	if card:isKindOf("AmazingGrace") then
		return 2
	end
	if card:isKindOf("GodSalvation") then
		if target:isWounded() then
			return 2
		end
	end
	if card:isKindOf("IronChain") then
		if target:isChained() then
			return 2
		else
			return 1
		end
	end
	if   card:isKindOf("Dismantlement") or card:isKindOf("Snatch") then
		if self:isFriend(target) and target:getCards("j"):length()>0 then
			return 2
		end
		if self:isEnemy(target) and target:getCards("j"):length()>0  and target:isNude() then
			return 2
		end
		return 1
	end
	return 1
end
sgs.ai_skill_cardask["yeyan-show"] = function(self, data)
	local target = self.player:getTag("yeyan_target"):toPlayer()
	local use = data:toCardUse()
	local showcard = self.player:getTag("yeyan_card"):toCard()
	local res = yeyan_judge(self, target, use.card)

	if (res == 1 and self:isEnemy(target))  or (res== 2 and self:isFriend(target)) then
		local cards = {}
		for _,c in sgs.qlist(self.player:getCards("hs")) do
			if c:getTypeId() == showcard:getTypeId() then
				table.insert(cards, c)
			end
		end
		if #cards > 0 then
			self:sortByKeepValue(cards)
			return "$" .. cards[1]:getId()
		end
	end
	return "."
end

--[幽月]
sgs.ai_skill_invoke.youyue = true

--梦月 ＆ 幻月
--[幻痛]
sgs.ai_skill_invoke.huantong = true
local function  huantongValue(cards, self, damage, huantongDamage)
	local tmp = damage
	tmp.damage = huantongDamage
	local tmp = self:touhouDamage(tmp, tmp.from, tmp.to)
	local value = 0
	if (tmp.damage < damage.damage and self:isFriend(damage.to)) or
	(tmp.damage > damage.damage and self:isEnemy(damage.to)) then
		value = value + 2 * math.abs(damage.damage - tmp.damage)
	end
	if  tmp.damage > damage.to:getHp() then
		if self:isFriend(damage.to) then
			if self:hasWeiya() then  value = value - 10 end
			value = value - 1
		else
			value = value + 1
		end
	end
	for _,c in ipairs(cards) do
		if c:isKindOf("Peach") or c:isKindOf("Analeptic") then
			if self:isFriend(damage.to) then
				value = value + 3 - tmp.damage
			else
				value = value - 2
			end
		else
			if self:isFriend(damage.to) then
				value = value + 1
			else
				value = value - 1
			end
		end
	end
	--if tmp.damage >= 2 and self.player:hasSkill("mengyan") and self.player:isWounded() then
		-- recover count as 2 value
		--value = value + self.player:getPile("dream"):length()
	--end
	return value
end
sgs.ai_skill_cardask["@huantong"] = function(self, data)
	local damage = data:toDamage()
	if not self:isFriend(damage.to) and not self:isEnemy(damage.to) then return "." end
	local ids = self.player:getPile("dream")
	local basics, others = {}, {}
	for _,id in sgs.qlist(ids) do
		local card=sgs.Sanguosha:getCard(id)
		if card:isKindOf("BasicCard") then
			table.insert(basics, card)
		else
			table.insert(others, card)
		end
	end
	local inverse = self:isFriend(damage.to)
	self:sortByUseValue(basics, inverse)
	--if self:cautionChangshi() then --有千年紫敌人，早点清除有价值的牌
	--	self:sortByUseValue(others, true)
	--end

	local combines = {}
	if #basics >=2 then
		local cards = {basics[1], basics[2]}
		local arr = {combine = cards, value = huantongValue(cards, self, damage, 2)}
		table.insert(combines, arr)
	end
	if #basics > 0 and #others > 0 then
		local cards = {basics[1], others[1]}
		local arr = {combine = cards, value = huantongValue(cards, self, damage, 1)}
		table.insert(combines, arr)
	end
	if #others >=2 then
		cards = {others[1], others[2]}
		local arr = {combine = cards, value = huantongValue(cards, self, damage, 0)}
		table.insert(combines, arr)
	end

	local compare_func = function(a, b)
		return a.value > b.value
	end

	table.sort(combines, compare_func)
	if combines[1].value > 0 then
		local huantongIds = {}
		for _,c in ipairs (combines[1].combine) do
			table.insert(huantongIds, c:getId())
		end
		return "$" .. table.concat(huantongIds, "+")
	end
	return "."
end


sgs.ai_cardneed.huantong = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  (not to:getWeapon() and  getCardsNum("Weapon",to,self.player)<1 and card:isKindOf("Weapon"))
		or (not to:getOffensiveHorse() and  getCardsNum("OffensiveHorse",to,self.player)<1 and card:isKindOf("OffensiveHorse"))
	end
end

--[梦湮]
sgs.ai_skill_invoke.mengyan = true


--艾丽
--[镰幕]
sgs.ai_skill_use["@@lianmu"] = function(self, prompt)
	local slash = sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
	slash:setSkillName("lianmu");
	slash:deleteLater()
	local dummy_use = { isDummy = true, to = sgs.SPlayerList()}
	self:useBasicCard(slash, dummy_use)
	local targets = {}
	if dummy_use.card and  not dummy_use.to:isEmpty() then
		for _, p in sgs.qlist(dummy_use.to) do
			table.insert(targets, p:objectName())
		end
	end

	if #targets >0 then
		return "@LianmuCard=.->" .. table.concat(targets, "+")
	end
	return "."
end

--[幻卫]
sgs.ai_skill_invoke.huanwei =  true
sgs.ai_damageCaused.huanwei = function(self, damage)
	if damage.card and not damage.chain and not damage.transfer then
		if  damage.card:isKindOf("Slash") and  damage.card:getSuit() == sgs.Card_Spade then
			damage.damage = math.max(0, damage.damage -1)
		end
	end
	return damage
end


--神绮
--[创世]
sgs.ai_skill_use["@@sqchuangshi"] = function(self, prompt)
	local targets={}
	for _, p in ipairs(self.friends_noself) do
		table.insert(targets,p:objectName())
	end
	if #targets >1 then
		return "@SqChuangshiCard=.->" .. table.concat(targets, "+")
	end
	return "."
end

--暂时不考虑是否使得整个创世过程继续下去，以及源法有谁能摸牌的问题。
sgs.ai_skill_use["BasicCard+^Jink,TrickCard+^Nullification,EquipCard|.|.|sqchuangshi"] = function(self, prompt, method)
	local cards =  self:getCards("sqchuangshi", "hs")
	self:sortByUseValue(cards)
	for _, card in ipairs(cards) do
		if card:getTypeId() == sgs.Card_TypeTrick and not card:isKindOf("Nullification") then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			self:useTrickCard(card, dummy_use)
			if dummy_use.card then
				if dummy_use.to:isEmpty() then
					if card:isKindOf("IronChain") then
						return "."
					end
					return dummy_use.card:toString()
				else
					local target_objectname = {}
					for _, p in sgs.qlist(dummy_use.to) do
						table.insert(target_objectname, p:objectName())
					end
					return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
				end
			end
		elseif card:getTypeId() == sgs.Card_TypeBasic and not card:isKindOf("Jink") then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			self:useBasicCard(card, dummy_use)
			if dummy_use.card then
				if dummy_use.to:isEmpty() then
					return dummy_use.card:toString()
				else
					local target_objectname = {}
					for _, p in sgs.qlist(dummy_use.to) do
						table.insert(target_objectname, p:objectName())
					end
					return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
				end
			end
		elseif card:getTypeId() == sgs.Card_TypeEquip then
			local dummy_use = { isDummy = true }
			self:useEquipCard(card, dummy_use)
			if dummy_use.card then
				return dummy_use.card:toString()
			end
		end
	end
	return "."
end

--[源法]
sgs.ai_skill_invoke.yuanfa  = function(self)
	local f = 0
	local e = 0
	for _,p in sgs.qlist(self.room:getAllPlayers()) do
		if  p:getMark("yuanfa") == 0 then continue end
		if self:isFriend(p) then
			f = f + 1
		end
		if self:isEnemy(p) then
			e = e + 1
		end
	end
	return f > e
end


--旧作爱丽丝
--[怪绮]
function turnUse_guaiqi(self)
	local piles = self.player:getPile("modian")
	local guaiqis = {}-- for use trick
	local tricks = {}-- for use slash
	local slashes = {}
	for _,id in sgs.qlist(piles) do
		local c = sgs.Sanguosha:getCard(id)
		if c:isKindOf("TrickCard") then
			table.insert(tricks, c)
		end
		if c:isKindOf("Slash") then
			table.insert(slashes, c)
		end
	end
	if (#slashes > 0) then
		for _,id in sgs.qlist(piles) do
			local c = sgs.Sanguosha:getCard(id)
			if c:isKindOf("TrickCard") and not c:isKindOf("Nullification") and not table.contains(guaiqis,  c:objectName()) then
				table.insert(guaiqis, c:objectName())
			end
		end
	end
	if (#tricks > 0) then
		table.insert(guaiqis, "slash")
	end
	if #guaiqis == 0 then return nil end



	local choices={}
	for i = 1, #guaiqis do
		local forbiden = guaiqis[i]
		local forbid
		if forbiden == "slash" then
			forbid = sgs.cloneCard(forbiden, tricks[1]:getSuit(),tricks[1]:getNumber())
		else
			forbid = sgs.cloneCard(forbiden, slashes[1]:getSuit(),slashes[1]:getNumber())
		end
		if self.player:isCardLimited(forbid, sgs.Card_MethodUse, true) or not forbid:isAvailable(self.player) then
		else
			table.insert(choices,guaiqis[i])
		end
	end
	if #choices == 0 then return nil end
	local choice
	if not choice and table.contains(choices,"dismantlement") then
		for _,p in pairs(self.friends_noself) do
			if p:containsTrick("indulgence") or  p:containsTrick("supply_shortage")  then
				choice="dismantlement"
			end
		end
	end
	if not choice and table.contains(choices,"god_salvation") then
		local aoe = sgs.cloneCard("god_salvation",slashes[1]:getSuit(), slashes[1]:getNumber())
		if self:willUseGodSalvation(aoe) then
				choice="god_salvation"
		end
	end
	if not choice and table.contains(choices,"savage_assault") then
		local aoe = sgs.cloneCard("savage_assault",slashes[1]:getSuit(), slashes[1]:getNumber())
		if self:getAoeValue(aoe, self.player)>0 then
				choice="savage_assault"
		end
	end
	if not choice and table.contains(choices,"archery_attack") then
		local aoe = sgs.cloneCard("archery_attack",slashes[1]:getSuit(), slashes[1]:getNumber())
		if self:getAoeValue(aoe, self.player)>0 then
				choice="archery_attack"
		end
	end
	if not choice and table.contains(choices,"ex_nihilo")  then
		choice="ex_nihilo"
	end
	if not choice then
		choice = choices[1]
	end

	local str  = (choice..":guaiqi[%s:%s]="):format("to_be_decided", -1)
	if (choice == "slash") then
		self:sortByUseValue(tricks, true)
		str = str .. tricks[1]:getEffectiveId()
	else
		self:sortByUseValue(slashes, true)
		str = str .. slashes[1]:getEffectiveId()
	end
	return str
end

local guaiqi_skill = {}
guaiqi_skill.name = "guaiqi"
table.insert(sgs.ai_skills, guaiqi_skill)
guaiqi_skill.getTurnUseCard = function(self)
	local str= turnUse_guaiqi(self)
	if not str then return nil end
	local parsed_card = sgs.Card_Parse(str)
	return parsed_card
end

function sgs.ai_cardsview_valuable.guaiqi(self, class_name, player)
	if class_name == "sqchuangshi" then
		return turnUse_guaiqi(self)
	end

	if (sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
		return nil
	end
	if class_name == "Slash" then
		local piles = self.player:getPile("modian")
		local modians = {}
		for _,id in sgs.qlist(piles) do
			local c = sgs.Sanguosha:getCard(id)
			if c:isKindOf("TrickCard") then
				table.insert(modians, c)
			end
		end
		if #modians == 0 then return nil end
		self:sortByUseValue(modians, true)
		local suit = modians[1]:getSuitString()
		local number = modians[1]:getNumberString()
		local card_id = modians[1]:getEffectiveId()
		return ("slash:guaiqi[%s:%s]=%d"):format(suit, number, card_id)
	end


	if class_name == "Nullification" then
		local hasNul = false
		local piles = self.player:getPile("modian")
		local modians = {}
		for _,id in sgs.qlist(piles) do
			local c = sgs.Sanguosha:getCard(id)
			if c:isKindOf("Slash") then
				table.insert(modians, c)
			end
			if c:isKindOf("Nullification") then
				hasNul = true
			end
		end
		if not hasNul then return nil end
		if #modians == 0 then return nil end
		self:sortByUseValue(modians, true)

		local suit = modians[1]:getSuitString()
		local number = modians[1]:getNumberString()
		local card_id = modians[1]:getEffectiveId()
		return ("nullification:guaiqi[%s:%s]=%d"):format(suit, number, card_id)
	end
	return nil
end

--[魔典]
local modianvs_skill = {}
modianvs_skill.name = "modian_attach"
table.insert(sgs.ai_skills, modianvs_skill)
function modianvs_skill.getTurnUseCard(self)
	if self.player:isKongcheng() then return nil end
	if self.player:hasFlag("Forbidmodian") then return nil end

	local cards = sgs.QList2Table(self.player:getCards("hs"))
	local card
	self:sortByUseValue(cards,true)
	for _,acard in ipairs(cards)  do
		if acard:isNDTrick() or acard:isKindOf("Slash") then
			card = acard
			break
		end
	end
	if not card then return nil end

	local skillcard = sgs.Card_Parse("@ModianCard="..card:getEffectiveId())

	assert(skillcard)
	return skillcard
end
local modianself_skill = {}
modianself_skill.name = "modian"
table.insert(sgs.ai_skills, modianself_skill)
function modianself_skill.getTurnUseCard(self)
	return modianvs_skill.getTurnUseCard(self)
end
sgs.ai_skill_use_func.ModianCard = function(card, use, self)
	local targets = {}
	for _,friend in ipairs(self.friends) do
		if friend:hasSkill("modian") and not friend:hasFlag("modianInvoked") then
			if not friend:isWounded() and friend:getPile("modian"):length() >= friend:getHp()  then
				continue
			end
			table.insert(targets, friend)
		end
	end
	if #targets > 0 then
		use.card = card
		if use.to then
			use.to:append(targets[1])
			return
		end
	end
end
sgs.ai_card_intention.ModianCard = -40

sgs.ai_skill_cardask["@modian"] = function(self, data)
	local ids = self.player:getPile("modian")
	local tricks = {}
	for _,id in sgs.qlist(ids) do
		local card = sgs.Sanguosha:getCard(id)
		if not card:isKindOf("TrickCard") then
			return "$" .. id
		end
		table.insert(tricks, card)
	end
	self:sortByUseValue(tricks,true)
	return "$" .. tricks[1]:getId()
end

--[[sgs.ai_skill_choice.modian = function(self, choices, data)
	if choices:match("recover") then return "recover" end
	return "draw"
end]]

--萨丽艾尔
--[报死]
sgs.ai_skill_use["@@baosi"] = function(self, prompt)
	self:sort(self.enemies,"defense")
	local enemies = {}
	for _,p in pairs(self.enemies) do
		if p:hasFlag("Global_baosiFailed") then
			table.insert(enemies, p:objectName())
		end
	end
	if #enemies==0 then return "." end
	return "@BaosiCard=.->" .. table.concat(enemies, "+")
end

--[魔眼]
local moyan_skill = {}
moyan_skill.name = "moyan"
table.insert(sgs.ai_skills, moyan_skill)
function moyan_skill.getTurnUseCard(self)
	if self.player:getMark("@moyan")==0 or not self.player:isWounded() then return nil end
	return sgs.Card_Parse("@MoyanCard=.")
end
sgs.ai_skill_use_func.MoyanCard=function(card,use,self)
	local targets = {}
	for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self:isEnemy(p) then
			table.insert(targets, p)
		end
		if #targets >= self.player:getLostHp() then break end
	end
	if #targets > 0 then
		use.card = card
		if use.to then
			for _,p in ipairs(targets) do
				use.to:append(p)
			end
			return
		end
	end
end
sgs.ai_use_value.MoyanCard = 2
sgs.ai_use_priority.MoyanCard = sgs.ai_use_priority.ExNihilo - 0.2
 sgs.ai_card_intention.MoyanCard = 50

--矜羯罗
--[纵酒]
local zongjiu_skill = {}
zongjiu_skill.name = "zongjiu"
table.insert(sgs.ai_skills, zongjiu_skill)
function zongjiu_skill.getTurnUseCard(self)
	if self.player:getCards("s"):isEmpty() then return nil end
	local slash = self:getCard("Slash")
	if not slash or not self:shouldUseAnaleptic(self.player, slash) then return nil end
	local ana = sgs.cloneCard("analeptic", sgs.Card_NoSuit, 0)
	ana:deleteLater()
	if not sgs.Analeptic_IsAvailable(self.player, ana) or self.player:isCardLimited(ana, sgs.Card_MethodUse) then   return nil  end
	local cards = self.player:getCards("s")
	cards = sgs.QList2Table(cards)
	self:sortByCardNeed(cards)
	return sgs.Card_Parse("@ZongjiuCard=" .. cards[1]:getEffectiveId())
end
sgs.ai_skill_use_func.ZongjiuCard=function(card,use,self)
	use.card = card
end
sgs.ai_use_priority.ZongjiuCard = sgs.ai_use_priority.Slash + 0.4
sgs.ai_skill_cardask["@zongjiu"] = function(self, data)
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)
	self:sortByCardNeed(cards)
	if #cards > 0 then
		return "$" .. cards[1]:getId()
	end
	return "."
end

function sgs.ai_cardsview_valuable.zongjiu(self, class_name, player)
	if class_name == "Analeptic" then
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying then return nil end
		if self:isFriend(dying, player) then
			local cards = self.player:getCards("s")
			cards = sgs.QList2Table(cards)
			if #cards== 0 then return nil end
			self:sortByCardNeed(cards)
			return "@ZongjiuCard=" .. cards[1]:getEffectiveId()
		end
		return nil
	end
end

--梦子
--[奇刃]
local qiren_skill = {}
qiren_skill.name = "qiren"
table.insert(sgs.ai_skills, qiren_skill)
function qiren_skill.getTurnUseCard(self)
	if self.player:hasFlag("qirenUsed") then return nil end
	local shows = {}
	local cards1 = self.player:getCards("s")
	for _, c in sgs.qlist(cards1) do
		if  not c:isKindOf("Jink") and not c:isKindOf("Nullification") then
			table.insert(shows, c)
		end
	end
	local hiddens = {}
	local cards2 = self.player:getCards("h")
	cards2 =self:touhouAppendExpandPileToList(self.player, cards2)
	for _, c in sgs.qlist(cards2) do
		if not c:isKindOf("EquipCard") and not c:isKindOf("Jink") and not c:isKindOf("Nullification") and not c:isKindOf("DelayedTrick") then
			table.insert(hiddens, c)
		end
	end
	if #shows == 0 or #hiddens == 0 then return nil end

	--目前只考虑单目标变多目标
	local use = {}
	for _,c in ipairs(shows) do
		if c:isKindOf("AOE") or c:isKindOf("GlobalEffect") or c:isKindOf("IronChain") or c:isKindOf("LureTiger") or c:isKindOf("KnownBoth") then
			table.insert(use, c:getEffectiveId())
			break
		end
	end
	for _,c in ipairs(hiddens) do
		if c:isKindOf("BaosiCard") then
			table.insert(use, c:getEffectiveId())
			break
		elseif not c:isKindOf("AOE") and not c:isKindOf("GlobalEffect") and not c:isKindOf("IronChain")
		and not c:isKindOf("LureTiger") and not c:isKindOf("KnownBoth") then
			table.insert(use, c:getEffectiveId())
			break
		end
	end

	if #use ~= 2 then return nil end
	return sgs.Card_Parse("@QirenCard=" .. table.concat(use, "+"))
end
sgs.ai_skill_use_func.QirenCard=function(card,use,self)
	local tmp = sgs.Sanguosha:getCard((card:getSubcards():first()))
	local effect = sgs.Sanguosha:getCard((card:getSubcards():last()))
	local targets = {}
	if tmp:isKindOf("AOE") then
		for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if not self.room:isProhibited(self.player, p, tmp) then
				table.insert(targets, p)
			end
		end
	elseif tmp:isKindOf("GlobalEffect") then
		for _, p in sgs.qlist(self.room:getAlivePlayers()) do
			if not self.room:isProhibited(self.player, p, tmp) then
				table.insert(targets, p)
			end
		end
	else
		local init_num = 1
		if tmp:isKindOf("IronChain") or tmp:isKindOf("LureTiger") or tmp:isKindOf("KnownBoth") then init_num =2 end
		local total_num = init_num + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, tmp)
		local positive = effect:isKindOf("Peach") or effect:isKindOf("Analeptic") or effect:isKindOf("ExNihilo")
		for _, p in sgs.qlist(self.room:getAlivePlayers()) do
			if not self.room:isProhibited(self.player, p, tmp) and tmp:targetFilter(sgs.PlayerList(), p, self.player) then
				if positive and self:isFriend(p) then
					table.insert(targets, p)
				elseif not positive and self:isEnemy(p)then
					table.insert(targets, p)
				end
			end
			if #targets >= total_num then break end
		end
	end

	if #targets > 0 then
		use.card = card
		if use.to then
			for _, p in ipairs (targets) do
				use.to:append(p)
			end
			if use.to:length() >= 1 then return end
		end
	end
end
sgs.ai_use_priority.QirenCard = 10

-- 雪 ＆ 舞
--[戮力]
sgs.ai_skill_use["@@luli"] = function(self, prompt)

	local num = self.player:getMark("luli")
	local ids = sgs.IntList()
    --简单制衡
	for _,c in sgs.qlist(self.player:getCards("hs")) do
	    if c:isKindOf("TrickCard") and not c:isKindOf("Nullification") then
			ids:append(c:getEffectiveId())
		elseif c:isKindOf("EquipCard") then
			ids:append(c:getEffectiveId())
		end
		if ids:length() >= num then break end
	end
	
	if (num - ids:length()) >= 3 then
		for _,c in sgs.qlist(self.player:getCards("e")) do
			ids:append(c:getEffectiveId())
			if ids:length() >= num then break end
		end
	end

	
	
	--默认全制衡？
	if not ids:isEmpty() then
        
		local recast = {}
		for _,id in sgs.qlist(ids) do
			table.insert(recast, tostring(id))
		end
		return "@LuliCard=" ..table.concat(recast, "+").."->."
	end
	return "."
end

--[协舞]
sgs.ai_skill_choice.xiewu = function(self, choices, data)
	local target = data:toCardUse().from
	if not target then
		target = data:toCardResponse().m_from
	end
	if self:isFriend(target) then return "draw" end
	if self:isEnemy(target) and choices:match("discard") then return "discard" end
	return "cancel"
end

sgs.ai_choicemade_filter.skillChoice.xiewu = function(self, player, args, data)
	local target = data:toCardUse().from
	if not target then
		target = data:toCardResponse().m_from
	end
	local choice = args[#args]
	
	if  choice == "draw" then
		sgs.updateIntention(player, target, -40)
	elseif choice == "discard" then
		sgs.updateIntention(player, target, 40)
	end

end


--明罗
--[暗流]
sgs.ai_skill_invoke.anliu = function(self, data)
	local target =self.player:getTag("anliu-target"):toPlayer()
	return self:isEnemy(target)
end

sgs.ai_choicemade_filter.skillInvoke.anliu = function(self, player, args)
	local target =player:getTag("anliu-target"):toPlayer()
	if target then
		if args[#args] == "yes" then
			sgs.updateIntention(player, target, 50)
		else
			sgs.updateIntention(player, target, -10)
		end
	end
end

sgs.ai_skill_playerchosen.anliu = function(self, targets)
	--slash数量默认为1 好了， 不取求转化杀的子卡了

	for _, target in sgs.qlist(targets) do
		if self:isEnemy(target) then
			for _,id in sgs.qlist(target:getShownHandcards()) do
				if sgs.Sanguosha:getCard(id):isKindOf("BasicCard") then
					return target
				end
			end

			basic = getKnownCard(target, self.player, "Basic") + 1
			total = target:getHandcardNum() + 1
			if basic/ total >= 0.5 then
				return target
			end
		end
	end

	for _, target in sgs.qlist(targets) do
		if self:isFriend(target) then
			return target
		end
	end

	return nil
end

sgs.ai_skill_use["@@zhanche"] = function(self)
	-- return "@ZhancheCard=5->sgs1"
	local t = sgs.QList2Table(self.player:getCards("hs"))
	if #t == 0 then return "." end
	self:sortByUseValue(t, true)
	if self:getUseValue(t[1]) >= sgs.ai_use_value.Peach then return "." end
	local enemies = {}
	local enemy
	for _,enemy in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self:isEnemy(enemy) and (not enemy:getEquips():isEmpty()) then table.insert(enemies, enemy) end
	end
	if #enemies == 0 then return "." end
	self:sort(enemies, "threat")
	return "@ZhancheCard=" .. tostring(t[1]:getEffectiveId()) .. "->" .. enemies[1]:objectName()
end

sgs.ai_skill_cardask["@zhanche-robbed"] = function(self)
	local t = sgs.QList2Table(self.player:getCards("hes"))
	if #t == 0 then return "." end
	self:sortByKeepValue(t, true)
	return t[1]:getEffectiveId()
end

sgs.ai_skill_playerchosen.huosui = function(self, targets)
	local _targets = sgs.QList2Table(targets)
	self:sort(_targets)
	for _, current in ipairs(_targets) do
		if not self:isFriend(current) then
			local viewAsSlash = sgs.Sanguosha:cloneCard("Slash", sgs.Card_SuitToBeDecided, -1)
			viewAsSlash:setSkillName("_huosui")
			local use = {isDummy = true, to = sgs.SPlayerList()}
			use.to:append(current)
			self:useCardSlash(viewAsSlash, use)
			viewAsSlash:deleteLater()
			if use.card then return current end
		end
	end
end


local yihuan_skill = {}
yihuan_skill.name = "yihuan"
table.insert(sgs.ai_skills, yihuan_skill)
yihuan_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("YihuanCard") then return end
	local cards = self.player:getCards("hes")
	local card
	local t = {}
	for _, card in  sgs.qlist(cards) do
		if (not self.player:isJilei(card)) and (card:getTypeId() ~= sgs.Card_TypeEquip) and (not card:canDamage()) then
			table.insert(t, card)
		end
	end
	if #t == 0 then return end
	self:sortByKeepValue(t, true)
	return sgs.Card_Parse("@YihuanCard=" .. tostring(t[1]:getEffectiveId()))
end

sgs.ai_skill_use_func.YihuanCard=function(card,use,self)
	local others = self.room:getOtherPlayers(self.player)
	local enemies, enemy = {}
	for _, enemy in sgs.qlist(others) do
		if self:isEnemy(enemy) then table.insert(enemies, enemy) end
	end
	if #enemies == 0 then return end
	self:sort(enemies)
	use.card = card
	if use.to then use.to:append(enemies[1]) end
end

sgs.ai_use_priority.YihuanCard = sgs.ai_use_priority.Slash - 0.3

sgs.ai_cardneed.wuzui = function(to, card, self)
	if to:hasSkill("wuzui") then return card:canDamage() end
end

sgs.ai_skill_invoke.wuzui = true

sgs.ai_skill_cardask["@wuzui-put"] = function(self, data)
	local damageCardType = self.player:property("wuzui"):toInt()
	local cards = self.player:getCards("hes")
	local t, card = {}
	for _, card in sgs.qlist(cards) do
		if (card:getTypeId() == damageCardType) and card:canDamage() then
			table.insert(t, card)
		end
	end
	if #t == 0 then return "." end
	self:sortByKeepValue(t, true)
	return "$" .. tonumber(t[1]:getEffectiveId())
end

sgs.ai_skill_cardask["@wuzui-discard"] = function(self, data)
	local ids = sgs.QList2Table(self.player:getPile("guilt"))
	if #ids == 0 then return "." end
	local t = {}
	for _, id in ipairs(ids) do
		table.insert(t, sgs.Sanguosha:getCard(id))
	end
	local damage = data:toDamage()
	local to = damage.to
	if self:isFriend(to) then return "." end
	if to:getHp() - damage.damage - 1 > to:dyingThreshold() then return "." end
	self:sortByUseValue(t)
	return "$" .. tonumber(t[1])
end
