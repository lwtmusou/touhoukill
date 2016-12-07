
sgs.ai_skill_playerchosen.sidou = function(self, targets)
	if self.player:getHp()== 1 then
		return nil
	end
	local wizzard=self:invokeTouhouJudge()
	for _,p in sgs.qlist(targets) do
		if not wizzard and p:containsTrick("lightning") then
			return p
		end
		if self:isFriend(p) then
			if p:containsTrick("indulgence") or p:containsTrick("supply_shortage") then
				return p
			end
		end
	end
	for _,p in sgs.qlist(targets) do
		if self:isEnemy(p) and (p:getEquip(1) or p:getEquip(2) or p:getEquip(3) or p:getEquip(4)) then
			return p
		end
	end
	return nil
end
sgs.ai_skill_cardchosen.sidou = function(self, who, flags)
	local wizzard=self:invokeTouhouJudge()
	if self:isFriend(who) then
		cards=who:getCards("j")
		cards = sgs.QList2Table(cards)
		if #cards>0 then
			return cards[1]
		end
	end
	if self:isEnemy(who) then
		cards=who:getCards("e")
		cards = sgs.QList2Table(cards)
		ecards={}
		for _,c in pairs (cards) do
			if not c:isKindOf("Weapon") then
				table.insert(ecards,c)
			end
		end
		if #ecards>0 then
			return ecards[1]
		end
	end
end
sgs.ai_choicemade_filter.cardChosen.sidou = sgs.ai_choicemade_filter.cardChosen.dismantlement


function SmartAI:findRealKiller(victim,damage)
	local mouko = self.room:getLord()
	if mouko and mouko:isAlive() and mouko:hasLordSkill("tymhwuyu") then
		return mouko
	end
	if damage then
		return damage.from
	else
		return nil
	end
end
sgs.ai_skillProperty.tymhwuyu = function(self)
	return "noKingdom"
end


sgs.ai_skill_invoke.huanyue = function(self,data)
	local damage = self.player:getTag("huanyue_damage"):toDamage()
	if not self:isEnemy(damage.to) then return false end
	local tempDamage = damage.damage
	damage.damage = damage.damage+1
	local realDamage = self:touhouDamage(damage,damage.from,damage.to)
	if realDamage.damage <= tempDamage then return false end

	local canDamage = self:touhouNeedAvoidAttack(damage,damage.from,damage.to)
	if canDamage then
		local blacknum = getKnownCard(self.player, self.player, "black", false, "hs")
		if (blacknum >= self.player:getHandcardNum()) then return true end
		if damage.to:hasSkill("duxin") then return false end
		local rate= blacknum / self.player:getHandcardNum()
		return rate > 1/2
	end
	return false
end
sgs.ai_cardneed.huanyue = function(to, card, self)
	return  card:isBlack()
end
sgs.ai_choicemade_filter.skillInvoke.huanyue = function(self, player, args)
	local damage = player:getTag("huanyue_damage"):toDamage()
	if damage.to and args[#args] == "yes" then
		sgs.updateIntention(player, damage.to, 50)
	end
end

sgs.ai_skill_invoke.sizhai = true

sgs.ai_skill_invoke.yuanhu = function(self,data)
	local source = self.player:getTag("yuanhu"):toPlayer()
	return source and self:isFriend(source)
end
sgs.ai_choicemade_filter.skillInvoke.yuanhu = function(self, player, args)
	local source = player:getTag("yuanhu"):toPlayer()
	if source and args[#args] == "yes" then
		sgs.updateIntention(player, source, -30)
	end
end
sgs.ai_skill_discard.yuanhu = function(self,discard_num, min_num)
	local target = self.player:getTag("yuanhu_target"):toPlayer()
	local to_discard = {}
	if not target or not self:isFriend(target) then return to_discard end

	local toGive, allcards = {}, {}
	local keep
	for _, card in sgs.qlist(self.player:getCards("hs")) do
		if not keep and (isCard("Jink", card, self.player) or isCard("Analeptic", card, self.player)) then
			keep = true
		else
			table.insert(toGive, card)
		end
		table.insert(allcards, card)
	end
	local cards = #toGive > 0 and toGive or allcards
	self:sortByKeepValue(cards, true)
	table.insert(to_discard, cards[1]:getId())
	--默认分配一张 其他情况暂时不计
	return to_discard
end
sgs.ai_choicemade_filter.cardExchange.yuanhu = function(self, player, args)
	local target = player:getTag("yuanhu_target"):toPlayer()
	if target and args[#args] ~= "_nil_" then
		sgs.updateIntention(player, target, -30)
	end
end

local hunpo_skill = {}
hunpo_skill.name = "hunpo"
table.insert(sgs.ai_skills, hunpo_skill)
function hunpo_skill.getTurnUseCard(self)
	if self.player:getMaxHp()>= 4 then return nil end
	if self.player:isNude() then return nil end
	local cards={}
	if self:getOverflow() >0 then
		cards= sgs.QList2Table(self.player:getHandcards())
	else
		cards= sgs.QList2Table(self.player:getCards("hes"))
	end
	self:sortByKeepValue(cards)
	if #cards>0 then
		return sgs.Card_Parse("@HunpoCard="..cards[1]:getId())
	end
	return nil
end
sgs.ai_skill_use_func.HunpoCard = function(card, use, self)
	use.card=card
end


sgs.ai_skill_invoke.fanji = function(self,data)
	local target=self.player:getTag("fanji_damage"):toDamage().from
	local buff =self.player:getTag("fanji_damage"):toDamage().to
	if target and buff then
		if self:isEnemy(target) then
			if buff:objectName()==self.player:objectName() then
				return true
			else
				if self.player:getMaxHp()>=self.player:getHp() then
					return true
				end
			end
		end
	end
	return false
end
sgs.ai_skill_choice.fanji= function(self, choices, data)
	if self.player:isWounded() then
		return "maxhp"
	end
	return "hp"
end


sgs.ai_skill_invoke.zaiwu = function(self,data)
	local target = self.player:getTag("zaiwu"):toPlayer()
	if not target then return false end
	local plus = self.player:getHp() > target:getHp()
	if plus then
		return self:isFriend(target)
	end
	return self:isEnemy(target)
end
sgs.ai_choicemade_filter.skillInvoke.zaiwu = function(self, player, args)
	local target = player:getTag("zaiwu"):toPlayer()
	if target  then
		local plus = player:getHp() > target:getHp()
		if plus and args[#args] == "yes" then
			sgs.updateIntention(player, target, -30)
		elseif not plus and args[#args] == "yes" then
			sgs.updateIntention(player, target, 50)
		end
	end
end
sgs.ai_skill_invoke.mengwei = function(self,data)
	local target = data:toPlayer()
	if not target then return false end
	--太复杂 先默认
	return self:getOverflow() <= 0 and self:isFriend(target)
end
sgs.ai_choicemade_filter.skillInvoke.mengwei = function(self, player, args)
	local target = player:getTag("mengwei"):toPlayer()
	if target  and args[#args] == "yes" then
		sgs.updateIntention(player, target, -30)
	end
end

sgs.ai_skill_cardask["@xiubu-self"] = function(self, data)
	if sgs.ai_skill_invoke.xiubu(self, data) then
		local dis = self:askForDiscard("Dummy", 1, 1, false, false)
		return tostring(dis[1])
	end
end

sgs.ai_skill_invoke.xiubu =function(self,data)
	local target = data:toPlayer()
	if not target then return false end
	if self:isEnemy(target) then
		if target:getHandcardNum() <= 1 or target:getHp() ==1 then
			return true
		end
	elseif self:isFriend(target) then
		if target:getHandcardNum() >3 and target:getHp() > 1 then
			return true
		end
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.xiubu = function(self, player, args)
	local target = self.room:getCurrent()
	if target and target:getHandcardNum() <= 1 and args[#args] == "yes" then
		sgs.updateIntention(player, target, 50)
	end
end


sgs.ai_cardneed.xiubu = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  (not to:getWeapon() and  getCardsNum("Weapon",to,self.player)<1 and card:isKindOf("Weapon"))
		or (not to:getOffensiveHorse() and  getCardsNum("OffensiveHorse",to,self.player)<1 and card:isKindOf("OffensiveHorse"))
	end
end