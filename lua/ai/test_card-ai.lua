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