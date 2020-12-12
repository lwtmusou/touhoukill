
--天空璋：坂田合欢
--[磨刀]
local modao_skill={}
modao_skill.name="modao"
table.insert(sgs.ai_skills,modao_skill)
modao_skill.getTurnUseCard=function(self)
	local cards = self.player:getCards("hs")
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

