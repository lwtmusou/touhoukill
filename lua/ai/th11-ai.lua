function SmartAI:xiangqiDamageEffect(to)
	local fakeDamage=sgs.DamageStruct()
	fakeDamage.card=nil
	fakeDamage.nature= sgs.DamageStruct_Normal
	fakeDamage.damage=1
	fakeDamage.from=self.player
	fakeDamage.to= to
	local damage_effect = self:touhouNeedAvoidAttack(fakeDamage, self.player, to)
	if  to:objectName() == self.player:objectName() then
		damage_effect = false
	end
	return damage_effect
end
sgs.ai_skill_invoke.xiangqi = function(self,data)
	local from=self.player:getTag("xiangqi_from"):toPlayer()
	local to =self.player:getTag("xiangqi_to"):toPlayer()
	local damCard=self.player:getTag("xiangqi_card"):toCard()
	if not from or not to then return false end
	if not damCard then return false end

	local damage_effect = self:xiangqiDamageEffect(to)

	if not damage_effect then
		if self:isEnemy(from) and self:isFriend(to)  then
			return true
		elseif self:isFriend(from) and self:isFriend(to) and self:getOverflow(from) >3 then
			return true
		end
	end
	if self:isFriend(from)  and not self:isEnemy(to)  then
		return false
	elseif not self:isEnemy(from) and self:isFriend(to)  then
		return false
	end

	if damage_effect  then
		local needCausedamage = false
		if  self:isEnemy(to) then
			needCausedamage = true
		end

		local knownBasicNum = getKnownCard(from, self.player, "BasicCard", false, "hs")
		local knownTrickNum = getKnownCard(from, self.player, "TrickCard", false, "hs")
		local knownEquipNum = getKnownCard(from, self.player, "EquipCard", false, "hs")
		local UnknownNum = from:getHandcardNum() - knownBasicNum - knownTrickNum - knownEquipNum
		local knownSameNum = 0
		local knownDiffNum = 0
		if damCard:isKindOf("BasicCard") then
			knownSameNum  = knownBasicNum
			knownDiffNum  = knownTrickNum + knownEquipNum
		else
			knownSameNum  = knownTrickNum
			knownSameNum  = knownBasicNum + knownEquipNum
		end

		if self.player:hasSkill("duxin") then
			if needCausedamage and knownSameNum >0 then
				return true
			elseif not needCausedamage and knownDiffNum >0 then
				return true
			end
		end
		if damCard:isKindOf("BasicCard") then
			if self:isEnemy(to) then
				return true
			end
			if self:isFriend(to) then
				return  from:getHandcardNum()>=4
			end
		end
		if damCard:isKindOf("TrickCard") then
			if self:isFriend(to) then
				return true
			end
		end
	end
	return false
end
sgs.ai_skill_cardchosen.xiangqi = function(self, who, flags)
	local from=self.player:getTag("xiangqi_from"):toPlayer()
	local to =self.player:getTag("xiangqi_to"):toPlayer()

	local damCard=self.player:getTag("xiangqi_card"):toCard()

	local damage_effect = self:xiangqiDamageEffect(to)


	local cards = from:getHandcards()
	cards = sgs.QList2Table(cards)

	if self.player:hasSkill("duxin") then
		if not damage_effect then
			local needReverse = not self:isFriend(from)
			self:sortByKeepValue(cards, needReverse)
			return cards[1]
		end

		local same =false
		if  self:isEnemy(to)  then
			same=true
		end

		self:sortByUseValue(cards, same)
		local keyword
		if  damCard:isKindOf("BasicCard") then
			keyword="BasicCard"
		else
			keyword="TrickCard"
		end
		for _,c in pairs (cards)do
			if same then
				if c:isKindOf(keyword) then
					return c
				end
			else
				if not c:isKindOf(keyword) then
					return c
				end
			end
		end
		return cards[1]
	end

	local j = math.random(1, #cards)
	return cards[j]
end
sgs.ai_slash_prohibit.xiangqi = function(self, from, to, card)
	if self:isFriend(from,to) then  return false end

	local can_kill = false
	local fakeDamage=sgs.DamageStruct()
	fakeDamage.card=card
	fakeDamage.nature= self:touhouDamageNature(card,from,to)
	fakeDamage.damage=1
	fakeDamage.from=slash
	fakeDamage.to=to
	local final_damage=self:touhouDamage(fakeDamage,from, to)
	if final_damage.damage >= to:getHp() then
		can_kill = true
	end

	if not can_kill then
		local peach_num = 0
		for _,c in sgs.qlist(from:getCards("hs")) do
			if c:isKindOf("Peach") or c:isKindOf("Analeptic") then
				peach_num = peach_num + 1
			end
		end
		if peach_num >0 then
			if to:hasSkill("duxin") then
				return true
			else
				return peach_num >= ( from:getHandcardNum() /2)
			end
		end
	end
	return false
end
sgs.ai_cardneed.xiangqi = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  (not to:getWeapon() and  getCardsNum("Weapon",to,self.player)<1 and card:isKindOf("Weapon"))
		or (not to:getOffensiveHorse() and  getCardsNum("OffensiveHorse",to,self.player)<1 and card:isKindOf("OffensiveHorse"))
	end
end

sgs.ai_skill_invoke.huzhu = function(self,data)
	cards =self.player:getCards("hs")
	cardname="Jink"
	for _,card in sgs.qlist(cards) do
		if card:isKindOf(cardname) then
			return false
		end
	end
	if self:isWeak(self.player) then
		return true
	else
		self:sort(self.friends_noself)
		for _,p in pairs (self.friends_noself) do
			if not self:isWeak(p) then
				return true
			end
		end
	end
	return false
end
sgs.ai_skill_invoke.huzhu_change = function(self,data)
	local lord=self.room:getTag("huzhu_target"):toPlayer()
	local jinks={}
	if not self:isFriend(lord) then return false end
	for _,card in sgs.qlist(self.player:getCards("hs")) do
		if card:isKindOf("Jink") then
			table.insert(jinks,card)
		end
	end
	if #jinks <2 then
		if self:isWeak(lord) then
			return true
		else
			return false
		end
	end
	if #jinks >1 then return true end
end
sgs.ai_choicemade_filter.skillInvoke.huzhu_change = function(self, player, args)
	local target=self.room:getTag("huzhu_target"):toPlayer()
	if target and args[#args] == "yes" then
			sgs.updateIntention(player, target, -60)
	end
end


local maihuo_skill = {}
maihuo_skill.name = "maihuo"
table.insert(sgs.ai_skills, maihuo_skill)
function maihuo_skill.getTurnUseCard(self)
	if self.player:hasUsed("MaihuoCard") then return nil end
	local handcards = sgs.QList2Table(self.player:getHandcards())
	if #handcards==0 then return nil end
	self:sortByUseValue(handcards)
	local reds={}
	for _,c in pairs (handcards) do
		if c:isRed() then
			table.insert(reds,c)
		end
	end
	if #reds>0 then
		return sgs.Card_Parse("@MaihuoCard=" .. reds[1]:getEffectiveId())
	else
		return sgs.Card_Parse("@MaihuoCard=" .. handcards[1]:getEffectiveId())
	end
end
sgs.ai_skill_use_func.MaihuoCard = function(card, use, self)
		local target = self:touhouFindPlayerToDraw(false, 2)
		if not target and #self.friends_noself>0 then
			target= self.friends_noself[1]
		end
		if target then
			use.card = card
			if use.to then
				use.to:append(target)
				if use.to:length() >= 1 then return end
			end
		end
end
sgs.maihuo_suit_value = {
	heart=3.9,
	diamond = 3.9
}
sgs.ai_use_value.MaihuoCard = 7
sgs.ai_use_priority.MaihuoCard = 7
sgs.ai_card_intention.MaihuoCard = -70

sgs.ai_skill_property.maihuo = { effect = {{"DrawEffect"},{"DrawEffect"}},
--effect1 means that target draw, effect2 means that maihuo skillonwer draw
	trigger =       {{"NotActive"}, {"Unkown"}},
	target =        {{"OtherAlivePlayers"}, {"SkillOwner"}},
}


function SmartAI:getDamageSource(attacker)
	if not attacker or attacker:hasSkill("wunian")  then
		return false
	end
	return true
end

sgs.ai_skill_use["@@yaoban"] = function(self, prompt)
	if self.player:isKongcheng() then return "." end

	local targets={}
	for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if p:hasFlag("Global_yaobanFailed") and self:isEnemy(p) then
			local fakeDamage=sgs.DamageStruct()
			fakeDamage.card=nil
			fakeDamage.nature= sgs.DamageStruct_Normal
			fakeDamage.damage=1
			fakeDamage.from=self.player
			fakeDamage.to=p
			local yaoban_effect = self:touhouNeedAvoidAttack(fakeDamage, self.player, p)
			--local yaoban_effect= self:touhouDamage(fakeDamage,self.player,p).damage>0
			if not  yaoban_effect then continue end
			table.insert(targets,p)
		end
	end

	if #targets ==0 then return "."  end
	self:sort(targets,"hp")
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)
	--return "#yaoban:" ..cards[1]:getEffectiveId().. ":->" .. targets[1]:objectName()
	return "@YaobanCard=" ..cards[1]:getEffectiveId().. "->" .. targets[1]:objectName()
end
sgs.ai_card_intention.YaobanCard = 60
sgs.ai_slash_prohibit.yaoban = function(self, from, to, card)
	local fakeDamage=sgs.DamageStruct()
	fakeDamage.card=nil
	fakeDamage.nature= sgs.DamageStruct_Normal
	fakeDamage.damage=1
	fakeDamage.from=to

	if self:isEnemy(from,to) then
		if to:hasSkill("yaoban") and to:hasSkill("here") and not self:isWeak(to) and not to:isKongcheng() then
			for _,friend in ipairs(self:getFriends(from)) do
				fakeDamage.to=friend
				if friend:getHp()<2 and self:touhouDamageInflicted(fakeDamage,to,friend).damage>0 then
					return true
				end
			end
		end
	end
	return false
end

function SmartAI:copyHereSlash(card)
	if not card:isKindOf("Slash") then
		return card
	end
	if card:isKindOf("FireSlash")then
		return card
	end
	local new_slash = sgs.cloneCard("fire_slash",card:getSuit(), card:getNumber())
	if (card:getSubcards():length()>0) then
		new_slash:addSubcards(card:getSubcards())
	else
		local id = card:getEffectiveId()
		if (id>-1) then
			new_slash:addSubcard(id)
		end
	end
	if (card:getSkillName()) then
		new_slash:setSkillName(card:getSkillName())
	end
	for _,flag in pairs(card:getFlags()) do
		new_slash:setFlags(flag)
	end
	return new_slash
end

sgs.ai_skill_invoke.yuanling = function(self,data)
	local target=self.player:getTag("yuanling"):toPlayer()
	if self:isEnemy(target) then
		return true
	end
end


function Cansave(self,dying,need_peachs)
	local all =self.room:getAlivePlayers()
	local peach_asked=true
	self.room:sortByActionOrder(all)
	local peachs=0
	for _,p in sgs.qlist(all) do
		if peach_asked and p:objectName() ==self.player:objectName() then
			peach_asked=false
		end
		if peach_asked then
			continue
		end
		if self:isFriend(p,dying) then
			peachs=peachs+ getCardsNum("Peach", p, self.player)
			if p:objectName() ==dying:objectName() then
				peachs=peachs+ getCardsNum("Analeptic", p, self.player)
			end
		end
	end

	return peachs +1 >= need_peachs
end
sgs.ai_skill_cardask["@songzang"] = function(self,data)
	local dying = data:toDying()
	local source = self:findRealKiller(dying.who, dying.damage)
	local executor = self:executorRewardOrPunish(dying.who, dying.damage)

	local self_role = self.player:getRole()
	local target=self.room:getCurrentDyingPlayer()
	local target_role=sgs.ai_role[target:objectName()]
	local need_kill=false
	local need_peachs = math.abs(1-target:getHp())


	if self_role== "loyalist" or self_role =="lord" then
		if self:isEnemy(target)  then
			if self:getOverflow()>0 then
				need_kill=true
			else
				local can_save= Cansave(self,target,need_peachs)
				if can_save then
					need_kill=true
				elseif target_role=="rebel"  then
					if source and  self:isFriend(source) then
					else
						need_kill=true
					end
				end
			end
		end
	end
	if self_role== "renegade" then
		local can_save=Cansave(self,target,need_peachs)
		if self:isEnemy(target) then
			if self:getOverflow()>0 and not (target:isLord() and self.room:alivePlayerCount()>2) then
				need_kill=true
			end
			if can_save then
				need_kill=true
			elseif target_role=="rebel" then
				if source and source:hasLordSkill("tymhwuyu") then
				else
					need_kill=true
				end
			end
		else
			if can_save then
			elseif target_role=="rebel" then
				if source and source:hasLordSkill("tymhwuyu") then
				else
					need_kill=true
				end
			end
		end
	end
	if self_role== "rebel" then
		if self:isFriend(target)
		and source and not self:isFriend(source)
		and not source:hasLordSkill("tymhwuyu") then
			local card_str = self:willUsePeachTo(target)
			if card_str =="." then
				need_kill=true
			end
		end
		if self:isEnemy(target) then
			if self:getOverflow()>0 then
				need_kill=true
			elseif target:isLord() then
				need_kill=true
			else
				need_kill = Cansave(self,target,need_peachs)
			end
		end
	end

	if not need_kill  then return "." end
	local cards ={}
	for _,card in sgs.qlist(self.player:getCards("hes")) do
		if card:getSuit()==sgs.Card_Spade then
			table.insert(cards,card)
		end
	end
	if #cards==0 then return "." end
	self:sortByKeepValue(cards)
	return "$" .. cards[1]:getId()
end
sgs.songzang_suit_value = {
	spade = 4.9
}
sgs.ai_cardneed.songzang = function(to, card, self)
	return  card:getSuit()==sgs.Card_Spade
end


function SmartAI:canGuaili(slash)
	if not self.player:hasSkill("guaili") then return false end
	for _,c in sgs.qlist(self.player:getCards("hs")) do
		if c:getEffectiveId() ~= slash:getEffectiveId() and c:isRed() then
			return true
		end
	end
	return false
end
sgs.ai_skill_cardask["@guaili"] = function(self, data)
	local effect = data:toSlashEffect()
	if self:isEnemy(effect.to) then
		local redCards = {}
		for _,c in sgs.qlist(self.player:getCards("hs")) do
			if  c:isRed() then
				table.insert(redCards, c)
			end
		end
		if #redCards == 0 then return "." end

		self:sortByKeepValue(redCards)
		local need_discard = false
		if self:getOverflow(self.player) <=1 and redCards[#redCards]:isKindOf("Peach") then
			if self:hasHeavySlashDamage(effect.from, effect.slash, effect.to) then
				need_discard = true
			elseif getCardsNum("Jink", effect.to, self.player) < 1 or sgs.card_lack[effect.to:objectName()]["Jink"] >0 then
				need_discard = true
			elseif self:isWeak(effect.to) then
				need_discard = true
			end
		else
			need_discard= true
		end
		if need_discard then
			return "$" .. redCards[#redCards]:getId()
		end
	end
	return "."
end
sgs.guaili_suit_value = {
	heart = 3.9,
	diamond = 3.9
}
sgs.guaili_keep_value = {
	Slash = 7
}
sgs.ai_cardneed.guaili = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return card:isKindOf("Slash") or card:isRed()
	end
end


local jiuhao_skill = {}
jiuhao_skill.name = "jiuhao"
table.insert(sgs.ai_skills, jiuhao_skill)
jiuhao_skill.getTurnUseCard = function(self, inclusive)
	if not self.player:hasFlag("jiuhao") or self.player:hasFlag("jiuhaoused") then return nil end
	return sgs.Card_Parse("@JiuhaoCard=.")
end
sgs.ai_skill_use_func.JiuhaoCard=function(card,use,self)
	local card = sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
	card:setSkillName("jiuhao")
	card:setFlags("jiuhao")
	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	self:useBasicCard(card, dummy_use)

	if not dummy_use.card then return end


	if dummy_use.to and not dummy_use.to:isEmpty() then
		slash_targets = 1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, card)
		for _, p in sgs.qlist(dummy_use.to) do
			use.card = card
			if use.to then use.to:append(p) end
			if not use.to or self.slash_targets <= use.to:length() then return end
		end
	end
end
sgs.ai_cardneed.jiuhao = function(to, card, self)
	return  card:isKindOf("Analeptic") or card:isKindOf("Peach")
end
sgs.ai_use_priority.JiuhaoCard = sgs.ai_use_priority.Slash


sgs.ai_skill_cardask["@jidu"] = function(self, data)
	local use = data:toCardUse()
	if self:touhouCardUseEffectNullify(use, self.player) then return "." end
	local e = 0
	local f = 0
	--需要预估伤害值
	for _,p in sgs.qlist(use.to) do
		if self:isFriend(p) then
			f = f+1
		end
		if self:isEnemy(p) then
			e = e+1
		end
	end
	if e <= f then return "." end

	local cards = sgs.QList2Table(self.player:getCards("hes"))
	self:sortByUseValue(cards)
	if #cards <= 0 then return "." end
	return "$" .. cards[1]:getId()
end

sgs.ai_skill_invoke.gelong = function(self,data)
	local damage = data:toDamage()
	return not self:isFriend(damage.from)
end
sgs.ai_skill_choice.gelong= function(self)
	if not self.player:faceUp() or self.player:isWounded() then
		return "gelong2"
	end
	return "gelong1"
end



sgs.ai_skill_playerchosen.rebing = function(self, targets)
	local current = self.room:getCurrent()
	local hasSlash  = getCardsNum("Slash", current, self.player) > 0
	if (self:isFriend(current) and not hasSlash) then return nil end
	local otherTargets = {}
	local enemyTargets = {}
	for _,p in sgs.qlist(targets) do
		if (self:isEnemy(p)) then
			table.insert(enemyTargets, p)
		else
			table.insert(otherTargets, p)
		end
	end
	if #enemyTargets > 0 then
		self:sort(enemyTargets, "defenseSlash")
		--暂时不考虑什么  SmartAI:slashProhibit(card, enemy, from)
		--和damageEffect
		return enemyTargets[1]
	end
	if #otherTargets > 0 and self:isEnemy(current) then
		self:sort(enemyTargets, "defenseSlash")
		target = otherTargets[#otherTargets]
		--没有具体的slash 无法考证slashEffective
		if not hasSlash then
			return target
		elseif not target:isWeak() and current:getHandcardNum() <=2 then
			return target
		end
	end
	return nil
end



sgs.ai_skill_invoke.diaoping  =function(self,data)
	if self.player:isKongcheng() then return false end
	local use=self.player:getTag("diaoping_slash"):toCardUse()

	if not use.from or self:isFriend(use.from) then return false end
	local hasFriend = false
	local hasWeakFriend = false
	local hasSelf = false
	for _,p in sgs.qlist(use.to) do
		if self:isFriend(p) and not (hasFriend and  hasWeakFriend) then
			if self:slashIsEffective(use.card, p, use.from) and not self:touhouCardUseEffectNullify(use, p) then
				local fakeDamage=sgs.DamageStruct()
				fakeDamage.card=use.card
				fakeDamage.nature= self:touhouDamageNature(use.card, use.from, p)
				fakeDamage.damage=1
				fakeDamage.from=use.from
				fakeDamage.to= p
				if self:touhouNeedAvoidAttack(fakeDamage,use.from,p) then
					hasFriend = true
					if p:objectName() == self.player:objectName() then
						hasSelf = true
					elseif self:isWeak(p) then
						hasWeakFriend = true
					end
				end
			end
		end
	end

	if  self.player:getHandcardNum()>=2 then
		return hasFriend
	else
		local lastCard = self.player:getCards("hs"):first()
		if hasWeakFriend and not hasSelf then
			return not lastCard:isKindOf("Peach")
		elseif hasWeakFriend and hasSelf then
			local maxCard = self:getMaxCard(use.from)
			local maxPoint
			if maxCard then
				maxPoint = maxCard:getNumber()
			else
				maxPoint = 0
			end
			if lastCard:isKindOf("Peach") then
				maxPoint = maxPoint + use.from:getHandcardNum()/4
			end
			return lastCard:getNumber() > maxPoint
		elseif not hasWeakFriend and hasSelf then
			return not (lastCard:isKindOf("Peach") or lastCard:isKindOf("Analeptic") or lastCard:isKindOf("Jink"))
		end
	end
	return false
end
sgs.ai_cardneed.diaoping = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  (not to:getWeapon() and  getCardsNum("Weapon",to,self.player)<1 and card:isKindOf("Weapon"))
		or (not to:getOffensiveHorse() and  getCardsNum("OffensiveHorse",to,self.player)<1 and card:isKindOf("OffensiveHorse"))
		or card:getNumber()>10
	end
end
function SmartAI:slashProhibitToDiaopingTarget(card,from,enemy)
	local diaopingEffect, kisume  = self:hasDiaopingEffect(from,enemy)
	if kisume then
		local from_card = self:getMaxCard(from)
		local kisume_card = self:getMaxCard(kisume)
		if not from_card then  return true end
		local from_point = from_card:getNumber()
		local kisume_point = 6
		if kisume_card then
			kisume_point = kisume_card:getNumber()
		end
		local handcardnum = kisume:getHandcardNum()
		for i =1, handcardnum, 1 do
			kisume_point = kisume_point + handcardnum/2
		end
		kisume_point = math.min(kisume_point,14)
		return from_point < kisume_point
	end
	return false
end
function SmartAI:hasDiaopingEffect(from,target)
	local kisume  = self.room:findPlayerBySkillName("diaoping")
	if kisume and not kisume:isKongcheng()
	and (kisume:inMyAttackRange(target)  or kisume:objectName() == target:objectName()) and not self:isEnemy(kisume, target)
	and from:getHandcardNum()>=2 and from:faceUp() then
		return true ,kisume
	end
	return false, nil
end

sgs.ai_skill_choice.cuiji=function(self, choices, data)
	local s = choices:split("+")
	return s[#s]
end
sgs.ai_skill_choice.cuiji_suit =function(self)
	if self:isWeak(self.player) then
		return "red"
	else
		return "black"
	end

end

local baigui_skill = {}
baigui_skill.name = "baigui"
table.insert(sgs.ai_skills, baigui_skill)
baigui_skill.getTurnUseCard = function(self, inclusive)
		local cards = self.player:getCards("hs")

		cards=self:touhouAppendExpandPileToList(self.player,cards)
		cards = sgs.QList2Table(cards)
		self:sortByUseValue(cards, true)

		local can_use=false
		local weak_targets1=0
		local weak_targets2=0
		targets1=self:getEnemies(self.player)
		targets2=self:getFriends(self.player)
		if #targets1 >= #targets2+1 then
			can_use=true
		end
		for _,target in pairs(targets1) do
			if self:isWeak(target) then
				weak_targets1=weak_targets1+1
			end
		end
		for _,target in pairs(targets2) do
			if self:isWeak(target) then
				weak_targets2=weak_targets2+1
			end
		end
		if weak_targets1>weak_targets2 then
			can_use=true
		end

		if not can_use then return false end
		local spade_card
		for _, card in ipairs(cards) do
				if  card:getSuit()==sgs.Card_Spade and  not isCard("Peach", card, self.player) and not isCard("ExNihilo", card, self.player) then
						spade_card = card
						break
				end
		end
		if spade_card then
				local suit = spade_card:getSuitString()
				local number = spade_card:getNumberString()
				local card_id = spade_card:getEffectiveId()
				local trick_str = ("savage_assault:baigui[%s:%s]=%d"):format(suit, number, card_id)
				local trick = sgs.Card_Parse(trick_str)

				assert(trick)
				return trick
		end
end
sgs.baigui_suit_value = {
	spade = 3.9
}
sgs.ai_use_value.baigui = sgs.ai_use_value.SavageAssault
sgs.ai_use_priority.baigui = sgs.ai_use_priority.SavageAssault
sgs.ai_cardneed.baigui = function(to, card, self)
	return  card:getSuit()==sgs.Card_Spade
end

sgs.ai_view_as.jiuchong = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card:getSuit()==sgs.Card_Heart and card_place == sgs.Player_PlaceHand then
		return ("analeptic:jiuchong[%s:%s]=%d"):format(suit, number, card_id)
	end
end

local jiuchong_skill = {}
jiuchong_skill.name = "jiuchong"
table.insert(sgs.ai_skills, jiuchong_skill)
jiuchong_skill.getTurnUseCard = function(self, inclusive)
		local cards = self.player:getCards("hs")
		cards=self:touhouAppendExpandPileToList(self.player,cards)
		cards = sgs.QList2Table(cards)
		self:sortByUseValue(cards, true)

		local heart_card
		for _, card in ipairs(cards) do
				if  card:getSuit()==sgs.Card_Heart and  not isCard("Peach", card, self.player) and not isCard("ExNihilo", card, self.player)
					   and (self:getUseValue(card) < sgs.ai_use_value.Analeptic or inclusive or sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, self.player, sgs.cloneCard("slash")) > 0) then
						heart_card = card
						break
				end
		end
		if heart_card then
				local suit = heart_card:getSuitString()
				local number = heart_card:getNumberString()
				local card_id = heart_card:getEffectiveId()
				local analeptic_str = ("analeptic:jiuchong[%s:%s]=%d"):format(suit, number, card_id)
				local analeptic = sgs.Card_Parse(analeptic_str)

				assert(analeptic)
				return analeptic
		end
end
sgs.jiuchong_suit_value = {
	heart = 3.9
}
sgs.ai_cardneed.jiuchong = function(to, card, self)
	return  card:getSuit()==sgs.Card_Heart
end

