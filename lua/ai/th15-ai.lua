
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
				if  card:getSuit() == c:getSuit() and not self.player:isProhibited(p, slash) then
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