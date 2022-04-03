--纯狐
--[瑕秽]
function getChunhuaCard(player)
	local damage = "snatch|dismantlement|amazing_grace|archery_attack|savage_assault|iron_chain|collateral|fire_attack|drowning|await_exhausted|known_both|duel|lure_tiger"
	local recover = "slash|fire_slash|snatch|dismantlement|amazing_grace|archery_attack|savage_assault|iron_chain|collateral|fire_attack|drowning|await_exhausted|known_both|duel|lure_tiger"
	local damages = damage:split("|")
	local recovers = recover:split("|")

	for _,c in sgs.qlist(player:getCards("h")) do
		local name = c:objectName()
		if c:isBlack() and table.contains(damages, name) then
			return c
		elseif c:isRed() and table.contains(recovers, name) then
			return c
		end
	end
	return nil
end
sgs.ai_skill_invoke.xiahui = function(self, data)
	return true
end
sgs.ai_skill_playerchosen.xiahui = function(self,targets)
	if (targets:contains(self.player) and self.player:hasSkill("chunhua")) then
		local card = getChunhuaCard(self.player)
		if card then return self.player end
	end
	for _,p in sgs.qlist(targets) do
		if not self:isFriend(p) then return p end
	end
	return nil
end
sgs.ai_skill_cardchosen.xiahui = function(self, who, flags)
	if self.player:objectName() == who:objectName() then
		local card = getChunhuaCard(self.player)
		if card then return card end
	end
end

--[纯化]
--[[
sgs.ai_skill_invoke.chunhua = function(self, data)
	local use = data:toCardUse()
	local e, f = 0, 0
	for _,p in sgs.qlist(use.to) do
		if use.card:isBlack() or (use.card:isRed() and p:isWounded()) then
			if self:isFriend(p) then
				f = f+1
			elseif self:isEnemy(p) then
				e = e+1
			end
		end
	end
	if use.card:isBlack() and e > f then return true end
	if use.card:isRed() and f > e then return true end
	return false
end
]]
sgs.ai_skill_choice.chunhua= function(self,  choices, data)
	local use = data:toCardUse()
	local e, f = 0, 0
	for _,p in sgs.qlist(use.to) do
		if choices:match("black") or (choices:match("red") and p:isWounded()) then
			if self:isFriend(p) then
				f = f+1
			elseif self:isEnemy(p) then
				e = e+1
			end
		end
	end
	if  e > f and choices:match("black") then return "black" end
	if f > e and choices:match("red") then return "red" end
	return "cancel"
end

--[杀意]
sgs.ai_skill_invoke.shayi = true
sgs.ai_skill_invoke.shayi_change = function(self, data)
	local lord = self.room:getLord()
	return self:isFriend(lord)
end

sgs.ai_skill_use["@@shayi"] = function(self, prompt)

	local ids=self.player:getTag("shayi"):toIntList()
	for _,id in sgs.qlist(ids) do
		local card = sgs.Sanguosha:getCard(id)
		local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
		self:useBasicCard(card, dummy_use)
		if dummy_use.card and not dummy_use.to:isEmpty() then
			local target_objectname = {}
			for _, p in sgs.qlist(dummy_use.to) do
				if self:isEnemy(p) then
					table.insert(target_objectname, p:objectName())
					return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
				end
			end
		end
	end
	return "."
end

--赫卡提亚·拉碧斯拉祖利
sgs.shownCard_skill = "chunhua|santi"
--[三体]
sgs.ai_skill_invoke.santi =  true

--克劳恩皮丝
--[狂乱]
sgs.ai_skill_invoke.kuangluan1 = function(self, data)
	local target = data:toPlayer()

	if self:isEnemy(target) and  not self:hasSkills(sgs.shownCard_skill, target) then return true end
	if self:isFriend(target) and  self:hasSkills(sgs.shownCard_skill, target) then return true end
	return false
end
sgs.ai_skill_invoke.kuangluan2 = function(self, data)
	local target = data:toPlayer()
	if self:isEnemy(target) and  not target:hasSkill("jinguo") then return true end
	if self:isFriend(target) and  target:hasSkill("santi") then return true end
	return false
end
--[狱意]
sgs.ai_skill_cardask["@yuyi_discard"] = function(self, data)
	local damage = data:toDamage()
	if self:isFriend(damage.from) or not self.player:canDiscard(damage.from, "hs")
		or not self.player:canDiscard(self.player, "hs")    then
		return "."
	end

	local red, black = false, false
	for _,c in sgs.qlist(damage.from:getCards("s")) do
	   if (c:isBlack()) then black = true end
	   if (c:isRed()) then red = true end
	   if (black and red) then break end
	end
	local reds, blacks = {}, {}
	local cards = sgs.QList2Table(self.player:getCards("hs"))
	for _,c in ipairs(cards) do
	   if (c:isBlack()) then table.insert(blacks, c) end
	   if (c:isRed()) then table.insert(reds, c) end
	end

	local throw
	if red and #blacks > 0 then
		self:sortByUseValue(blacks, true)
		throw = blacks[1]
	elseif black and #reds > 0 then
		self:sortByUseValue(reds, true)
		throw = reds[1]
	else
		self:sortByUseValue(cards, true)
		throw = cards[1]
	end
	if throw:isRed() then
		self.player:setTag("yuyi_color",sgs.QVariant("red"))
	else
		self.player:setTag("yuyi_color",sgs.QVariant("black"))
	end
	return "$" .. throw:getId()
end

sgs.ai_skill_cardchosen.yuyi = function(self, who, flags)
	local color = self.player:getTag("yuyi_color"):toString()
	self.player:removeTag("yuyi_color")
	local reds, blacks = {}, {}
	for _,c in sgs.qlist(who:getCards("s")) do
	   if (c:isBlack()) then table.insert(blacks, c) end
	   if (c:isRed()) then table.insert(reds, c) end
	end
	if color == "red" and #blacks > 0 then
		self:sortByUseValue(blacks, true)
		return blacks[1]
	elseif color == "black" and #reds > 0 then
		self:sortByUseValue(reds, true)
		return reds[1]
	end

	local id = self:askForCardChosen(who, "hs", "dismantlement", sgs.Card_MethodDiscard)
	return sgs.Sanguosha:getCard(id)
end

--[[sgs.ai_skill_invoke.yuyi = function(self, data)
	local current = self.room:getCurrent()
	if self.player:isCurrent() then
		local e , f = 0, 0
		for _,p in sgs.qlist(self.room:getAllPlayers()) do
			if not p:getShownHandcards():isEmpty() then
				if self:isEnemy(p) then e = e+1
				elseif self:isFriend(p) then f = f+1 end
			end
		end
		return f > 0 and f >= e
	elseif current then
		--回合外暂不考虑太多
		if self:isFriend(current) and current:hasSkill("santi") then
			return true
		end
		if self:isEnemy(current) and current:hasSkill("qiuwen") then
			return true
		end
	end
	return false
end]]

--稀神探女
--[舌祸]
sgs.ai_skill_use["BasicCard+^Jink,EquipCard|.|.|shehuo"] = function(self, prompt, method)
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
--反手使用锦囊 （弃疗、、、）
sgs.ai_skill_use["TrickCard+^Nullification,EquipCard|.|.|shehuo"] = function(self, prompt, method)
	local target = self.player:getTag("shehuo_target"):toPlayer()
	if not target or not self:isEnemy(target) then return "." end
	--local cards =  self:getCards("Slash", "hs")
	--local target_objectname = {}

	--if #cards > 0 then
	--    table.insert(target_objectname, target:objectName())
	--  return cards[1]:toString() .. "->" .. table.concat(target_objectname, "+")
	--end
	return "."
end

--[慎言]
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
sgs.ai_skill_invoke.shenyan = function(self, data)
	local current = self.room:getCurrent()
	return current and self:isFriend(current)
end

--哆来咪·苏伊特
--[捕梦]
sgs.ai_skill_invoke.bumeng = function(self, data)
	local from = data:toPlayer()
	--from = findPlayerByObjectName(self.room, move.from:objectName(), true)
	return from and self:isFriend(from)
end
sgs.ai_choicemade_filter.skillInvoke.bumeng = function(self, player, args)
	local move = player:getTag("bumeng"):toMoveOneTime()
	local from = nil
	if move.from then from = findPlayerByObjectName(self.room, move.from:objectName(), true) end
	if from and args[#args] == "yes" then
		sgs.updateIntention(player, from, -20)
	end
end
--[入梦]
sgs.ai_skill_invoke.rumeng = function(self, data)
	local current = self.room:getCurrent()
	return current and self:isEnemy(current)
end


--铃瑚
--[[sgs.ai_skill_playerchosen.yuejian = function(self,targets)
	if #self.enemies == 0 then return nil end
	self:sort(self.enemies,"defense")
	if self:isWeak(self.player) then
		self.yuejian_card = self:getMinCard():getEffectiveId()
	else
		self.yuejian_card = self:getMaxCard():getEffectiveId()
	end
	return self.enemies[1]
end
sgs.ai_playerchosen_intention.yuejian = 10
]]
--[月见]
sgs.ai_skill_use["@@yuejian!"] = function(self, prompt)
	local cardname = "await_exhausted"
	local card=sgs.cloneCard(cardname, sgs.Card_NoSuit, 0)
	card:setSkillName("yuejian")
	card:deleteLater()
	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	self:useTrickCard(card, dummy_use)
	if dummy_use.card and not dummy_use.to:isEmpty() then
		local target_objectname = {}
		for _, p in sgs.qlist(dummy_use.to) do
			table.insert(target_objectname, p:objectName())
		end
		if #target_objectname>0 then
			return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
		end
	end
	return "."
end


local yuejian_skill = {}
yuejian_skill.name = "yuejian"
table.insert(sgs.ai_skills, yuejian_skill)
yuejian_skill.getTurnUseCard = function(self)
	if self.player:isKongcheng() or self.player:hasUsed("YuejianCard") then return nil end
	return sgs.Card_Parse("@YuejianCard=.")
end
sgs.ai_skill_use_func.YuejianCard = function(card, use, self)
	local yuejian_card1 = self:getMaxCard()
	local point1 = yuejian_card1:getNumber()
	local yuejian_card2
	local point2 = 6
	local target
	self:sort(self.enemies, "defenseSlash")
	for _,p in ipairs (self.enemies) do
		if not p:isKongcheng() then
		    if not target then target = p end
			yuejian_card2 = self:getMaxCard(target)
			if yuejian_card2 then point2 = yuejian_card2:getNumber() end
			if point2 < point1 then
				target = p
			end
		end
	end
	
	if target then		
		if point2 < point1 then
			if self.player:hasSkill("jiangguo") and sgs.Analeptic_IsAvailable(self.player) then
				local dango = self.player:getPile("dango")
				if not use.isDummy  and dango:length() > 0 then
					local ana = sgs.cloneCard("magic_analeptic", sgs.Card_SuitToBeDecided, -1)
					ana:setSkillName("jiangguo")
					ana:addSubcard(dango:first())
					
					ana:deleteLater()
					use.card = ana
					if use.to then use.to = sgs.SPlayerList() end
					return
				end
			--self:useCardByClassName(card, use)
			end
		else
			yuejian_card1 = self:getMinCard()
		end
		
		use.card = card
		if use.to then
			use.to:append(target)
			self.yuejian_card = yuejian_card1:getEffectiveId()
			if use.to:length() >= 1 then return end
		end
	end
end
sgs.ai_use_priority.YuejianCard = sgs.ai_use_priority.AwaitExhausted - 0.2
--[浆果]
function sgs.ai_cardsview_valuable.jiangguo(self, class_name, player)
	if class_name == "Analeptic" then
	    local dying = self.room:getCurrentDyingPlayer() 
		if dying and dying:objectName() == self.player:objectName() then  
			local dango = self.player:getPile("dango")
			if dango:isEmpty() then return nil end
			local card = sgs.Sanguosha:getCard(dango:first())
		
			local suit = card:getSuitString()
			local number = card:getNumberString()
			local card_id = card:getEffectiveId()
			return ("magic_analeptic:jiangguo[%s:%s]=%d"):format(suit, number, card_id)
		end
	end
end

--清兰
--[异弹]
local yidan_skill = {}
yidan_skill.name = "yidan"
table.insert(sgs.ai_skills, yidan_skill)
yidan_skill.getTurnUseCard = function(self, inclusive)
	--青兰ai写起来相当弃疗
	if (self.player:hasUsed("YidanCard")) then return nil end
	
	local cards = self.player:getCards("hs")
	cards=self:touhouAppendExpandPileToList(self.player,cards)
	if cards:isEmpty() then return nil end
	local avail = sgs.QList2Table(cards)
	
	self:sortByUseValue(avail)
	if #avail == 0 then return nil end

	return sgs.Card_Parse("@YidanCard="..avail[1]:getEffectiveId())
	
end
sgs.ai_skill_use_func.YidanCard=function(card,use,self)
	local target
	local card_name = "light_slash"

	local slash = sgs.cloneCard(card_name, sgs.Card_SuitToBeDecided, -1)
	slash:addSubcard(card)
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self:isEnemy(p) and not p:isKongcheng() and p:getShownHandcards():isEmpty() then
			if  not self.player:isProhibited(p, slash) then
				target = p
				break
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


sgs.ai_use_priority.YidanCard = sgs.ai_use_priority.ThunderSlash + 0.2
--sgs.dynamic_value.damage_card.YidanCard = true

--[血杵] 默认触发