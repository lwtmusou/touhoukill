
sgs.ai_skill_cardask["@zaoxing-show"] = function(self)
	local suits = {0,0,0,0}
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if (c:getSuit() <= 3) and (not self.player:isShownHandcard(c:getId())) then
			suits[c:getSuit() + 1] = suits[c:getSuit() + 1] + 1
		end
	end
	
	local maxs = 1
	for i,n in ipairs(suits) do
		if suits[maxs] < n then maxs = i end
	end
	
	if suits[maxs] == 0 then return "." end
	
	local maxc = {}
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if (c:getSuit() == maxs - 1) and (not self.player:isShownHandcard(c:getId())) then
			table.insert(maxc, tostring(c:getEffectiveId()))
		end
	end
	
	return "$" .. table.concat(maxc, "+")
end

sgs.ai_skill_cardask["@zaoxing-recast"] = function(self, data)
	local zxproperty = self.player:property("zaoxing2"):toString()
	local zxsuit = ((zxproperty == "black") and sgs.Card_Black or ((zxproperty == "red") and sgs.Card_Red or sgs.Card_Colorless))
	local cards = {}
	for _, id in sgs.qlist(self.player:getShownHandcards()) do
		local c = sgs.Sanguosha:getCard(id)
		if (c:getColor() == zxsuit) and (not c:isKindOf("Peach")) then table.insert(cards, c) end
	end
	
	if #cards == 0 then return "." end
	
	self:sortByKeepValue(cards)
	local r = "$" .. tostring(cards[1]:getId())
	
	local use = data:toCardUse()
	if self:isFriend(use.to:first()) and (#cards >= 2) then
		r = r .. "+" .. tostring(cards[2]:getId())
	end
	
	return r
end

sgs.ai_skill_playerchosen.lingshou = function(self, targets)
	local n = {}
	for _, p in sgs.qlist(targets) do
		if self:isEnemy(p) then table.insert(n, p) end
	end
	
	if #n == 0 then return nil end
	
	local mostp =1
	for _, p in ipairs(n) do
		if p:getHandcardNum() > n[mostp]:getHandcardNum() then mostp = _ end
	end
	
	return n[mostp]
end

sgs.ai_skill_askforag.lingshou = function(self, ids)
	local cards = {}
	for _, id in ipairs(ids) do
		table.insert(cards, sgs.Sanguosha:getCard(id))
	end
	
	self:sortByKeepValue(cards)
	return cards[#cards]:getId()
end

sgs.ai_skill_use["@@LingshouOtherVS"] = function(self)
	local idstrings = string.split(self.player:property("lingshouSelected"):toString(), "+")
	local slash = sgs.Sanguosha:cloneCard("Slash", sgs.Card_SuitToBeDecided, -1)
	slash:setSkillName("_lingshou")
	for _, s in ipairs(idstrings) do
		slash:addSubcard(tonumber(s))
	end
	
	local use = {isDummy = true, to = sgs.SPlayerList()}
	self:useCardSlash(slash, use)
	slash:deleteLater()
	if use.card and not use.to:isEmpty() then
		local slist = {}
		for _, s in sgs.qlist(use.to) do
			table.insert(slist, s:objectName())
		end
		return slash:toString() .. "->" .. table.concat(slist, "+")
	end
	
	return "."
end


-- zhuying目前没人发动？啥鬼？
local zhuying_s={}
zhuying_s.name="zhuying_attach"
table.insert(sgs.ai_skills,zhuying_s)
zhuying_s.getTurnUseCard=function(self)
	if self.player:getKingdom() ~= "gxs" then return nil end
	if self.player:isKongcheng() then return nil end
	local flag = false
	for _, p in sgs.qlist(self.player:getAliveSiblings()) do
		if p:hasLordSkill("zhuying") and (not p:hasFlag("zhuying_selected")) and self:isFriend(p) then
			flag = true
			break
		end
	end
	if not flag then return nil end

	local cards = self.player:getHandcards()
	local cl = {}
	for _, c in sgs.qlist(cards) do
		if not (c:isKindOf("Peach")) then
			if (c:isKindOf("Snatch") or c:isKindOf("Slash") or c:isKindOf("SupplyShortage")) then
				if #self:getCards(c:objectName()) >= 2 then
					table.insert(cl, c)
				end
			else
				table.insert(cl, c)
			end
		end
	end
	if #cl == 0 then return nil end
	
	self:sortByUseValue(cl)
	return sgs.Card_Parse("@ZhuyingCard:" .. tostring(cl[1]:getId()))
end

sgs.ai_skill_use_func.ZhuyingCard = function(card, use, self)
	local ps = {}
	if (#self:getCards("Snatch") > 0 or #self:getCards("SupplyShortage") > 0) then
		for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if (self.player:distanceTo(p) == 2) and self:isEnemy(p) then table.insert(ps,p) end
		end
	end
	
	if #ps == 0 then
		if #self:getCards("Slash") > 0 then
			for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
				if (self.player:distanceTo(p) == self.player:getAttachRange() + 1) and self:isEnemy(p) then table.insert(ps,p) end
			end
		end
	end
	
	if #ps == 0 then return end
	self:sortEnemies(ps)
	use.card = card
	if use.to then use.to:append(ps[1]) end
end

sgs.ai_use_priority.ZhuyingCard =  sgs.ai_use_priority.Snatch + 0.001

sgs.ai_skill_playerchosen.zhuying_multilord = function(self,targets)
	for _, p in sgs.qlist(targets) do
		if self:isFriend(p) then return p end
	end
end

sgs.ai_skill_invoke.zhuying_effect = function(self, data)
	local p = self.room:findPlayerByObjectName(string.sub(data:toString(), 8))
	if p then return self:isFriend(p) end
	return false
end
