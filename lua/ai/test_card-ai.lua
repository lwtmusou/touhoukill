local Jade_skill = {}
Jade_skill.name = "JadeSeal"
table.insert(sgs.ai_skills, Jade_skill)
Jade_skill.getTurnUseCard = function(self, inclusive)
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