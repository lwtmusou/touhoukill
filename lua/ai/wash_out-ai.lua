function SmartAI:useCardLightSlash(...)
	self:useCardSlash(...)
end

sgs.ai_card_intention.LightSlash = sgs.ai_card_intention.Slash

sgs.ai_use_value.LightSlash = 4.55
sgs.ai_keep_value.LightSlash = 3.66
sgs.ai_use_priority.LightSlash = 2.5
sgs.dynamic_value.damage_card.LightSlash = true

function SmartAI:useCardIronSlash(...)
	self:useCardSlash(...)
end

sgs.ai_card_intention.IronSlash = sgs.ai_card_intention.Slash

sgs.ai_use_value.IronSlash = 4.6
sgs.ai_keep_value.IronSlash = 3.63
sgs.ai_use_priority.IronSlash = 2.5
sgs.dynamic_value.damage_card.IronSlash = true

function SmartAI:useCardPowerSlash(...)
	self:useCardSlash(...)
end

sgs.ai_card_intention.PowerSlash = sgs.ai_card_intention.Slash

sgs.ai_use_value.PowerSlash = 4.6
sgs.ai_keep_value.PowerSlash = 3.63
sgs.ai_use_priority.PowerSlash = 2.5
sgs.dynamic_value.damage_card.PowerSlash = true

function SmartAI:shouldUseMagicAnaleptic(trick)
	if sgs.turncount <= 1 and self.role == "renegade" and sgs.isLordHealthy() and self:getOverflow() < 2 then return false end

	local nul_f, nul_e = 0, 0
	for _, f in ipairs(self.friends)do
		nul_f = nul_f + getCardsNum("Nullification", f)
	end
	for _, e in ipairs(self.enemies)do
		nul_e = nul_e + getCardsNum("Nullification", e)
	end
	return nul_f >= nul_e
end

function SmartAI:searchForMagicAnaleptic(use, enemy, trick)

	if not self.toUse then return nil end
	if not use.to then return nil end

	local analeptic = self:getCard("MagicAnaleptic")
	if not analeptic then return nil end
	if not sgs.Analeptic_IsAvailable(self.player) then return nil end
	--local shouldUse = false
	--有一些防锦囊的技能需要判断
	--if not shouldUse then return nil end

	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:fillSkillCards(cards)

	local card_str = self:getCardId("MagicAnaleptic")
	if card_str then  return sgs.Card_Parse(card_str) end

	for _, anal in ipairs(cards) do
		if (anal:isKindOf("MagicAnaleptic")) and not (anal:getEffectiveId() == trick:getEffectiveId()) then
			return anal
		end
	end
end

sgs.ai_use_priority.MagicAnaleptic = sgs.ai_use_priority.Analeptic  - 0.2

function SmartAI:useCardSuperPeach(...)
	self:useCardPeach(...)
end
sgs.ai_use_value.SuperPeach = sgs.ai_use_value.Peach - 0.2
sgs.ai_use_priority.SuperPeach = 0.7
--[[
function SmartAI:useCardSuperPeach(card, use)
	if self:cautionDoujiu(self.player,card) then
		return
	end

	local targets = {}
	local good_targets = {}
	for _,f in ipairs (self.friends) do
		if f:isDebuffStatus() and (not f:isRemoved()) then
			table.insert(targets, f)
			if f:isWounded() then
				table.insert(good_targets, f)
			end
		end
	end

	if #targets <= 0 then return end

	local lord= getLord(self.player)
	if self.player:isDebuffStatus() and self.player:isWounded() then
		if self.player:hasArmorEffect("SilverLion") then
			for _, card in sgs.qlist(self.player:getHandcards()) do
				if card:isKindOf("Armor") and self:evaluateArmor(card) > 0 then
					if use.to then use.to:append(self.player) end
					use.card = card
					return
				end
			end
		end

		local SilverLion, OtherArmor
		for _, card in sgs.qlist(self.player:getHandcards()) do
			if card:isKindOf("SilverLion") then
				SilverLion = card
			elseif card:isKindOf("Armor") and not card:isKindOf("SilverLion") and self:evaluateArmor(card) > 0 then
				OtherArmor = true
			end
		end

		if SilverLion and OtherArmor then
			use.card = SilverLion
			return
		end
	end

	if #good_targets > 0 then
		self:sort(good_targets, "hp")
		for _,p in ipairs(good_targets) do
			if not self:needToLoseHp(p, nil, nil, nil, true) then
				use.card = card
				if use.to then
					use.to:append(p)
					return
				end
			end
		end
	end
	local mustusepeach = false
	for _, enemy in ipairs(self.enemies) do
		if self.player:getHandcardNum() < 3 and
				(self:hasSkills(sgs.drawpeach_skill,enemy) or getCardsNum("Dismantlement", enemy) >= 1
					or enemy:hasSkill("jixi") and enemy:getPile("field"):length() >0 and enemy:distanceTo(self.player) == 1
					or enemy:hasSkill("qixi") and getKnownCard(enemy, self.player, "black", nil, "hes") >= 1
					or getCardsNum("Snatch", enemy) >= 1 and enemy:distanceTo(self.player) == 1
					or (enemy:hasSkill("tiaoxin") and self.player:inMyAttackRange(enemy) and self:getCardsNum("Slash") < 1 or not self.player:canSlash(enemy)))
				then
			mustusepeach = true
			break
		end
	end

	if self.player:getHp() == 1 and not (lord and self:isFriend(lord) and lord:getHp() < 2 and self:isWeak(lord)) then
		mustusepeach = true
	end
	if mustusepeach then
		self:sort(targets, "hp")
		for _,p in ipairs(targets) do
			if not self:needToLoseHp(p, nil, nil, nil, true) then
				use.card = card
				if use.to then
					use.to:append(p)
					return
				end
			end
		end
	end
end
]]
sgs.ai_card_intention.SuperPeach = sgs.ai_card_intention.Peach

sgs.weapon_range.Gun = 4
sgs.weapon_range.Pillar = 3
sgs.ai_skill_use["@@Pillar"] = function(self, prompt, method)

	local cards = self.player:getCards("hs")
	cards=self:touhouAppendExpandPileToList(self.player, cards)
	local basic_cards =  {}  --sgs.QList2Table(cards)
	for _,c in sgs.qlist(cards) do
		if (c:isKindOf("BasicCard")) then
			table.insert(basic_cards, c)
		end
	end
	if #basic_cards == 0 then return "." end

	self:sortByUseValue(basic_cards, false)

	local card = sgs.cloneCard("slash", sgs.Card_SuitToBeDecided, -1)
	card:addSubcard(basic_cards[1])
	card:deleteLater()

	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	card:setSkillName("_Pillar")

	self:useBasicCard(card, dummy_use)

	if not dummy_use.card or dummy_use.to:isEmpty() then return "." end
	local target_objectname = {}
	for _, p in sgs.qlist(dummy_use.to) do
		table.insert(target_objectname, p:objectName())
	end
	return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
end
sgs.weapon_range.Hakkero = 3
sgs.ai_skill_invoke.Hakkero = function(self, data)
	local effect = data:toSlashEffect()
	return  effect.to and self:isEnemy(effect.to)
end

sgs.ai_skill_invoke.Hagoromo = true

local Jade_skill = {}
Jade_skill.name = "JadeSeal"
table.insert(sgs.ai_skills, Jade_skill)
Jade_skill.getTurnUseCard = function(self, inclusive)
	local treasure = self.player:getTreasure()
	if not treasure then return nil end
	if self.player:isBrokenEquip(treasure:getId()) then return nil end
	if self.player:hasFlag("JadeSeal_used") then
		return nil
	end
	local c = sgs.cloneCard("known_both", sgs.Card_NoSuit, 0)
	c:setSkillName("JadeSeal")
	c:setCanRecast(false)
	return c
end

sgs.ai_use_priority.JadeSeal = 7

sgs.ai_view_as.Pagoda = function(card, player, card_place)
	if not player:hasFlag("Pagoda_used")  then
		local suit = card:getSuitString()
		local number = card:getNumberString()
		local card_id = card:getEffectiveId()
		if card_place == sgs.Player_PlaceHand then
			if card:isBlack() then
				return ("nullification:Pagoda[%s:%s]=%d"):format(suit, number, card_id)
			end
		end
	end
end
sgs.ai_skill_invoke.Pagoda = function(self,data)
	local effect = data:toCardEffect()
	local card =  effect.card
	local target = effect.to
	if card:isKindOf("IronChain") or card:isKindOf("LureTiger") or card:isKindOf("AwaitExhausted") then
		return true
	end
	if card:isKindOf("AOE") or card:isKindOf("GlobalEffect") then
		local e, f = 0, 0
		for _, p in sgs.qlist(self.room:getAlivePlayers()) do
			if self:playerGetRound(p) > self:playerGetRound(target) then
				if self:isFriend(p) then
					f = f+1
				elseif self:isEnemy(p) then
					e = e+1
				end
			end
		end
		if ( e > f and card:isKindOf("GlobalEffect")) then
			return true
		elseif ( f >e and card:isKindOf("AOE")) then
			return true
		end
	end
	return false
end

function SmartAI:useCardAwaitExhausted(card, use)

	local targets = sgs.PlayerList()
	local total_num = 2 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, card)
	self:sort(self.friends, "defense")
	sgs.reverse(self.friends)
	for _, friend in ipairs(self.friends) do
		if card:targetFilter(targets, friend, self.player) and not targets:contains(friend)
				and self:hasTrickEffective(card, friend, self.player)
				and not friend:isNude()
				and not (friend:hasSkill("gaoao") and not friend:isCurrent())then
			use.card = card
			targets:append(friend)
			if use.to then
				use.to:append(friend)
				if use.to:length() == total_num then return end
			end
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if card:targetFilter(targets, enemy, self.player) and not targets:contains(enemy)
				and self:hasTrickEffective(card, enemy, self.player)
				and not enemy:isNude()
				and (enemy:hasSkill("gaoao") and not enemy:isCurrent())then
			use.card = card
			targets:append(enemy)
			if use.to then
				use.to:append(enemy)
				if use.to:length() == total_num then return end
			end
		end
	end
end
sgs.ai_use_value.AwaitExhausted = 7
sgs.ai_use_priority.AwaitExhausted = sgs.ai_use_priority.ExNihilo - 1
sgs.ai_card_intention.AwaitExhausted = function(self, card, from, tos)
	for _, to in ipairs(tos) do
		local gaoao = to:hasSkill("gaoao") and not to:isCurrent()
		if not gaoao then
			sgs.updateIntention(from, to, -10)
		else
			sgs.updateIntention(from, to, 40)
		end
	end
end

function SmartAI:willUseAllianceFeast(card)
	if not card then self.room:writeToConsole(debug.traceback()) return false end
	local good, bad = 0, 0
	--可以细化的机制判断
	--空城 永恒类
	-- 明牌 或者横置的具体张数
	--三种debuff各自价值
	for _, friend in ipairs(self.friends) do
		good = good + 10 * getCardsNum("Nullification", friend, self.player)
		if self:hasTrickEffective(card, friend, self.player) then
			if (friend:getShownHandcards():length() > 0 or  friend:getBrokenEquips():length() > 0 or friend:isChained()) then
				good = good + 10
			elseif friend:hasSkill("huiwu") then good = good + 5
			end
		end
	end

	for _, enemy in ipairs(self.enemies) do
		bad = bad + 10 * getCardsNum("Nullification", enemy, self.player)
		if self:hasTrickEffective(card, enemy, self.player) then
			if (enemy:getShownHandcards():length() > 0 or  enemy:getBrokenEquips():length() > 0 or enemy:isChained()) then
				bad = bad + 10

			end
		end
	end
	return (good - bad > 5)
end

function SmartAI:useCardAllianceFeast(card, use)
	if self:willUseAllianceFeast(card) then
		use.card = card
	end
end
sgs.ai_use_value.AllianceFeast = 3.6
sgs.ai_use_priority.AllianceFeast = 7.1
sgs.ai_keep_value.AllianceFeast = 3.32
sgs.dynamic_value.benefit.AllianceFeast = true

function SmartAI:useCardBoneHealing(card, use)
	local total_num = 1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, card)

	local enemies = self:exclude(self.enemies, card)
	local targets = {}

	for _,e in ipairs(enemies) do
		if e:isDebuffStatus() then
			table.insert(targets, e)
		end
	end
	if #targets > 0 then
		self:sort(targets, "hp")

		for _,t in ipairs(targets) do
			use.card = card
			if use.to then
				use.to:append(t)
			end
			if not use.to or total_num <= use.to:length() then return end
		end
	end
end
sgs.ai_use_value.BoneHealing = 5.1
sgs.ai_use_priority.BoneHealing = sgs.ai_use_priority.ThunderSlash + 0.1
sgs.ai_keep_value.BoneHealing = 2.5
sgs.dynamic_value.damage_card.BoneHealing = true
--仇恨由伤害事件更新好了
--sgs.ai_card_intention.BoneHealing = 30

function SmartAI:useCardSpringBreath(card, use)
	local friends = self:exclude(self.friends, card)
	if #friends == 0 then return end
	--local target = self:touhouFindPlayerToDraw(false, 6)

	local target  = friends[1]
	if target then
		use.card = card
		if use.to then use.to:append(target) end
		return
	end
end
sgs.ai_use_value.SpringBreath = 11
sgs.ai_use_priority.SpringBreath = sgs.ai_use_priority.SavingEnergy
sgs.ai_card_intention.SpringBreath = -20
