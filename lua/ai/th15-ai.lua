sgs.ai_skill_use["BasicCard+^Jink,TrickCard+^Nullification+^Lightning,EquipCard|.|.|shehuo"] = function(self, prompt, method)
	local target = self.player:getTag("shehuo_target"):toPlayer()
	if not target or not self:isEnemy(target) then return "." end 
	local cards =  self:getCards("Slash", "hs")
	local target_objectname = {}
	
	if #cards > 0 then
	    table.insert(target_objectname, target:objectName())
		return cards[1]:toString() .. "->" .. table.concat(target_objectname, "+")
	end
	return "."		
end


sgs.ai_skill_use["@@shenyan"] = function(self, prompt)
	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }

	local card=sgs.cloneCard("await_exhausted", sgs.Card_NoSuit, 0)
	card:setSkillName("shenyan")
	card:deleteLater()

	self:useTrickCard(card, dummy_use)
	if not dummy_use.card then return false end
	if not dummy_use.to:isEmpty() then
		local target_objectname = {}
		
		for _, p in sgs.qlist(dummy_use.to) do
			--if (self:isEnemy(p) and not p:isChained())
			table.insert(target_objectname, p:objectName())
			if #target_objectname==2 then break end
		end	
		if #target_objectname>0 then
			return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
		end
	end
	return "."
end




local yidan_skill = {}
yidan_skill.name = "yidan"
table.insert(sgs.ai_skills, yidan_skill)
yidan_skill.getTurnUseCard = function(self, inclusive)
    local cards = self.player:getCards("hs")
	cards=self:touhouAppendExpandPileToList(self.player,cards)
	if cards:isEmpty() then return nil end
	local suits = {}
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self:isEnemy(p) then 
			for _,c in sgs.qlist(p:getCards("es")) do
				local suit = c:getSuit()
				if not table.contains(suits,suit) then
					table.insert(suits, suit)
					if #suits >= 4 then break end
				end
			end
		end
		if #suits >= 4 then break end
	end
	
	if #suits == 0 then return nil end
	local avail = {}
	for _,c in sgs.qlist(cards) do
		local suit = c:getSuit()
        if table.contains(suits,suit) then
			table.insert(avail, c)
	    end
	end
	self:sortByUseValue(avail)
	if #avail == 0 then return nil end
	return sgs.Card_Parse("@YidanCard="..avail[1]:getEffectiveId())
end
sgs.ai_skill_use_func.YidanCard=function(card,use,self)
	local target
	local slash = sgs.cloneCard("slash", sgs.Card_SuitToBeDecided, -1)
	slash:addSubcard(card)
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self:isEnemy(p) then 
			for _,c in sgs.qlist(p:getCards("es")) do
				if  slash:getSuit() == c:getSuit() and not self.player:isProhibited(p, slash) then
					target = p
					break
				end
			end
		end
	end
	
	if target then
		use.card = card
		if use.to then 
			use.to:append(target) 
            if use.to:length() >= 1 then return end
		end
	end
end