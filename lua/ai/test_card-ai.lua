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




function SmartAI:useCardSuperPeach(card, use)
	if self:cautionDoujiu(self.player,card) then
		return
	end
	
    local targets = {}
	local good_targets = {}
	for _,f in ipairs (self.friends) do
		if f:isDebuffStatus() then
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
					use.to:append(self.player)
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

sgs.ai_card_intention.SuperPeach = sgs.ai_card_intention.Peach

sgs.weapon_range.Gun = 4

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
	if not player:hasFlag("Pagoda_used") then
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
sgs.ai_use_priority.AwaitExhausted = sgs.ai_use_value.ExNihilo - 1
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