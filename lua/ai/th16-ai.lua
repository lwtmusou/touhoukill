
--天空璋：摩多罗
--[秘神]
local mishen_skill={}
mishen_skill.name="mishen"
table.insert(sgs.ai_skills,mishen_skill)
mishen_skill.getTurnUseCard=function(self)
	local avail = {}
	for _,c in sgs.qlist(self.player:getCards("e")) do
        if (not self.player:isBrokenEquip(c:getEffectiveId())) then
            table.insert(avail, c)
		end
    end

	self:sortByUseValue(avail,true)

	--[[if #avail == 0 then 
		if not self.player:isChained() then
			local card_str = ("lure_tiger:mishen[%s:%s]=%d"):format("to_be_decided", 0, -1)
			local skillcard = sgs.Card_Parse(card_str)
			assert(skillcard)
			return skillcard
		end
	else
		local card = avail[1]
		local suit = card:getSuitString()
		local number = card:getNumberString()
		local card_id = card:getEffectiveId()
		local card_str = ("lure_tiger:mishen[%s:%s]=%d"):format(suit, number, card_id)
		local skillcard = sgs.Card_Parse(card_str)
		assert(skillcard)
		return skillcard
	end]]
	if #avail == 0 then
		if not self.player:isChained() then
			return sgs.Card_Parse("@MishenCard=.")
		end
	else
		return sgs.Card_Parse("@MishenCard=" .. avail[1]:getEffectiveId())
	end
end
sgs.ai_skill_use_func.MishenCard = function(card, use, self)
	local lt =sgs.cloneCard("lure_tiger")
	lt:setSkillName("mishen")
	
	assert(lt)
	self:useTrickCard(lt, use)
	
	if not use.card then return end
	use.card=card
end


--[后光]
sgs.ai_skill_invoke.houguang = true
sgs.ai_skill_invoke.houguanghide =function(self,data)
	local user = self.room:getLord()
	if not user then  return false end
	return self:isFriend(user)
end


--天空璋：坂田合欢
--[磨刀]
local modao_skill={}
modao_skill.name="modao"
table.insert(sgs.ai_skills,modao_skill)
modao_skill.getTurnUseCard=function(self)
	local cards = self.player:getCards("hes")
	cards=self:touhouAppendExpandPileToList(self.player, cards)
	cards=sgs.QList2Table(cards)
	local card
	self:sortByUseValue(cards,true)
	for _,acard in ipairs(cards) do
		if acard:getSuit() == sgs.Card_Club then
			card = acard
			break
		end
	end
	if not card then return nil end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local card_str = ("bone_healing:modao[%s:%s]=%d"):format(suit, number, card_id)
	local skillcard = sgs.Card_Parse(card_str)
	assert(skillcard)
	return skillcard
end
sgs.ai_cardneed.modao = function(to, card, self)
	return card:getSuit() == sgs.Card_Club
end


--天空璋：高丽野阿吽 
--[镇社]
local zhenshe_skill={}
zhenshe_skill.name="zhenshe"
table.insert(sgs.ai_skills,zhenshe_skill)
zhenshe_skill.getTurnUseCard=function(self)
	local cards = self.player:getCards("hes")
	cards=self:touhouAppendExpandPileToList(self.player, cards)
	cards=sgs.QList2Table(cards)
	local card
	self:sortByUseValue(cards,true)
	for _,acard in ipairs(cards) do
		if acard:getSuit() == sgs.Card_Heart then
			card = acard
			break
		end
	end
	if not card then return nil end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local card_str = ("saving_energy:zhenshe[%s:%s]=%d"):format(suit, number, card_id)
	local skillcard = sgs.Card_Parse(card_str)
	assert(skillcard)
	return skillcard
end


--天空璋：矢田寺成美 
--[菩提]
sgs.ai_skill_invoke.puti = true

sgs.ai_skill_discard.puti = function(self,discard_num)
	local gamerule_discard={}
	if (getCardsNum("Slash", self.player, self.player) == 0) then
		return gamerule_discard
	end
	
	gamerule_discard = sgs.ai_skill_discard.juhe(self,  1)
	
	return gamerule_discard
end

--[藏法]
sgs.ai_skill_playerchosen.zangfa = function(self, targets)
	local use = self.player:getTag("zangfa_use"):toCardUse()
	for _,p in sgs.qlist(targets) do
		local res = wunian_judge(self,p, use.card)
		if res==1 and self:isEnemy(p) then
			return p
		elseif res==2 and self:isFriend(target)  then
			return p
		end
	end

	return nil
end

sgs.ai_playerchosen_intention.zangfa = function(self, from, to)
	local use = from:getTag("zangfa_use"):toCardUse()
	local res = wunian_judge(self, to, use.card)
	if res==1 then
		sgs.updateIntention(from, to, 30)
	elseif res== 2 then
		sgs.updateIntention(from, to, -30)
	end
end


--天空璋：SP摩多罗隐岐奈 
--[门扉]
local menfei_skill = {}
menfei_skill.name = "menfei"
table.insert(sgs.ai_skills, menfei_skill)
menfei_skill.getTurnUseCard = function(self)
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:getMark("@door") > 0 then return nil end
	end
	return sgs.Card_Parse("@MenfeiCard=.")
end

sgs.ai_skill_use_func.MenfeiCard = function(card, use, self)
	use.card = card
	if use.to then
		use.to:append(self.player)
		if use.to:length() >= 1 then return end
	end
end


--[后户]
sgs.ai_skill_invoke.houhu =function(self,data)
	local use = data:toCardUse()
	local target 
	for _,p in sgs.qlist(self.room:getAlivePlayers())do
		if p:getMark("@door") > 0 then 
			target = p
			break
		end
	end
	
	if use.to:contains(target) then
		return true
	else
		local res = wunian_judge(self,target, use.card)
		if res==1 and self:isEnemy(target) then
			return true
		elseif res==2 and self:isFriend(target)  then
			return true
		end
	end
	return false
end

