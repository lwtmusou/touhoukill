function SmartAI:qianyiPhase(target)
	--willSkipPlayPhase 没考虑跳判定
	if self:isEnemy(target) then
		if self:getOverflow(target) >= -1  then 
			return sgs.Player_Play
		end
		return sgs.Player_Draw
	elseif self:isFriend(target) then
		return sgs.Player_Discard
	end
	return sgs.Player_NotActive
end
function SmartAI:qianyiValue(card, target, phase)
	local card1 = self:getSameEquip(card, target)
	local equipped
	if card1 and card1:getEffectiveId() > 0 then
		equipped = sgs.Sanguosha:getCard(card1:getEffectiveId())
	end
	local value = 0
	local value1 = 0
	local value2 = 0
	local value3 = 0
	if card:isKindOf("Weapon") then
		value1 = self:evaluateWeapon(card, self.player)
		value2 = self:evaluateWeapon(card, target)
		value3 = equipped and self:evaluateWeapon(equipped, target) or 0 
	elseif card:isKindOf("Armor") then
		value1 = self:evaluateArmor(card, self.player)
		value2 = self:evaluateArmor(card, target)
		value3 = equipped and self:evaluateArmor(equipped, target) or 0 
	else
		value1 = 3
		value2 = 3
		value3 = equipped and 3 or 0
	end
	if self:isFriend(target) then
		value = value - value1 + value2 - value3
		if phase == sgs.Player_Discard then
			if self:getOverflow(target) > 0 then
				value = value +  self:getOverflow(target)
			end
		end
	else
		value = value - value1 - value2 + value3
		if phase == sgs.Player_Draw then
			value = value + 5
		elseif phase == sgs.Player_Play then
			if self:getOverflow(target) >= -1 then
				value = value +  self:getOverflow(target) + 2
			end
		end
	end
	if self.player:isWounded() then
		value = value + 5
	end
	return value
end

sgs.ai_skill_cardask["@qianyi"] = function(self, data)
	local current = self.room:getCurrent()
	local phase = self:qianyiPhase(current)
	local cards = {}

	for _,c in sgs.qlist(self.player:getCards("e")) do
		local array={card = c, value=self:qianyiValue(c, current, phase)}
		table.insert(cards,array)
	end
	local compare_func = function(a, b)
		return a.value > b.value 
	end
	table.sort(cards, compare_func)
	
	if cards[1].value > 0 then
		return "$" .. cards[1].card:getId()
	end
	return "."
end
sgs.ai_skill_choice.qianyi = function(self, choices, data)
	local current = self.room:getCurrent()
	local phase = self:qianyiPhase(current)
	if phase == sgs.Player_Discard then
		return "discard"
	elseif phase == sgs.Player_Play then
		return "play"
	elseif phase == sgs.Player_Draw then
		return "draw"
	end
	return "finish"
end
sgs.ai_choicemade_filter.skillChoice.qianyi = function(self, player, args)
	local choice = args[#args]
	local current = self.room:getCurrent()
	if not current then return end
	if  choice == "play" or choice == "draw" then 
		sgs.updateIntention(player, current, 80)
	elseif choice == "discard" then
		sgs.updateIntention(player, current, -80)
	end
end


sgs.ai_skill_playerchosen.mengxiao = function(self, targets)
	local move = self.player:getTag("mengxiao"):toMoveOneTime()
	--暂时没管闪电等特殊情况
	for _,p in sgs.qlist(targets) do
		if self:isFriend(p) then
			for _,c in sgs.qlist(p:getCards("j")) do
				for _,id in sgs.qlist(move.card_ids) do
					if sgs.Sanguosha:getCard(id):getSuit() == c:getSuit() then
						return p
					end
				end
			end			
		end
	end
	for _,p in sgs.qlist(targets) do
		if self:isEnemy(p) then
			for _,c in sgs.qlist(p:getCards("e")) do
				for _,id in sgs.qlist(move.card_ids) do
					if sgs.Sanguosha:getCard(id):getSuit() == c:getSuit() then
						return p
					end
				end
			end			
		end
	end
	return nil
end

sgs.ai_skill_cardchosen.mengxiao = function(self, who, flags)
	self.player:gainMark("@nima")
	local cards = {}
	local flag = flags
	if self:isFriend(who) then
		flag = "j"
	elseif self:isEnemy(who) then
		flag = "e"
	end
	local move = self.player:getTag("mengxiao"):toMoveOneTime()
	for _,c in sgs.qlist(who:getCards(flag)) do
		for _,id in sgs.qlist(move.card_ids) do
			if sgs.Sanguosha:getCard(id):getSuit() == c:getSuit() then
				table.insert(cards,c)
			end
		end
	end
	if #cards>0 then
		return cards[1]
	end
end
sgs.ai_choicemade_filter.cardChosen.sidou = sgs.ai_choicemade_filter.cardChosen.snatch
