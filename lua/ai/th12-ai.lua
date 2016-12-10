
local pudu_skill = {}
pudu_skill.name = "pudu"
table.insert(sgs.ai_skills, pudu_skill)
pudu_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("PuduCard") then return nil end
	local can=false
	if self.player:getHp()>3 then
		can=true
	elseif self.player:getHp()==1   then
		local value = self:getCardsNum("Analeptic")+self:getCardsNum("Peach")
		if self.player:hasSkill("jiushu") and value>0 then
			value = value+2
		end
		if self:getCardsNum("Jink")>0 then
			value = value+1
		end
		if value>=2 then
			can=true
		end
	end
	if can then
		return sgs.Card_Parse("@PuduCard=.")
	end
	return nil
end
sgs.ai_skill_use_func.PuduCard=function(card,use,self)
	self:sort(self.friends_noself, "defense")
	for _, p in ipairs(self.friends_noself) do
		if p:isWounded()  and  p:getHp()+1 <= getBestHp(p)  then
			use.card = card
			if use.to then
				use.to:append(p)
				if use.to:length() >= 1 then return end
			end
		end
	end
end
sgs.ai_card_intention.PuduCard = -100
sgs.ai_use_priority.PuduCard = sgs.ai_use_priority.Analeptic + 0.2

sgs.ai_skill_invoke.jiushu = true


local fahua_loyalist=function(self,from,to,card)
	if self:touhouEffectNullify(card,from,to) then
		return true
	end
	if not self:hasTrickEffective(card, to, from) then
		return true
	end
	for _, askill in sgs.qlist(to:getVisibleSkillList()) do
		local s_name = askill:objectName()
		local filter = sgs.ai_trick_prohibit[s_name]
		if filter and type(filter) == "function"
			and filter(self, from, to, card) then
			return true
		end
	end
	return false
end

sgs.ai_skill_invoke.fahua = true
sgs.ai_skill_invoke.fahua_change = function(self,data)
	local lord=self.room:getTag("fahua_target"):toPlayer()
	local use=self.room:getTag("fahua_use"):toCardUse()
	if not self:isFriend(lord) then
		return false
	end
	if self.player:isChained() and use.card:isKindOf("IronChain") then
		return true
	end
	if fahua_loyalist(self,use.from,self.player,use.card) then
		return true
	end
	local lieges = self.room:getLieges("xlc",lord)
	self.room:sortByActionOrder(lieges)
	fahua_invoked=true
	for _, liege in sgs.qlist(lieges) do
		if liege:objectName()== self.player:objectName() then
			fahua_invoked=false
			continue
		end
		if not fahua_invoked and self:isFriend(lord,liege) then
			if  fahua_loyalist(self,use.from,liege,use.card) then
				return false
			end
		end
	end

	if self:isWeak(lord) then
		return true
	elseif not self:isWeak(self.player) then
		return true
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.fahua_change = function(self, player, args)
	local target=self.room:getTag("fahua_target"):toPlayer()
	if target then
		if args[#args] == "yes" then
			sgs.updateIntention(player, target, -60)
		end
	end
end
sgs.ai_trick_prohibit.fahua = function(self, from, to, card)
	if not to:hasLordSkill("fahua") then return false end
	local lieges = self.room:getLieges("xlc", to)
	for _, liege in sgs.qlist(lieges) do
		if self:isFriend(liege, to) then
			if fahua_loyalist(self,from,liege,card) then
				return true
			end
		end
	end
	return false
end



local weizhi_skill = {}
weizhi_skill.name = "weizhi"
table.insert(sgs.ai_skills, weizhi_skill)
function weizhi_skill.getTurnUseCard(self)
	if not self.player:hasUsed("WeizhiCard") then
		return sgs.Card_Parse("@WeizhiCard=.")
	end
end

sgs.ai_skill_use_func.WeizhiCard = function(card, use, self)
	local unpreferedCards = {}

	local cards = {}
	local zcards = {}
	for _,c in sgs.qlist(self.player:getHandcards()) do
		if not c:isKindOf("TrickCard") then
			table.insert(cards,c)
			table.insert(zcards,c)
		end
	end
	for _,c in sgs.qlist(self.player:getCards("e")) do
		table.insert(zcards,c)
	end
	if self.player:getHp() < 3 then
		local use_slash, keep_jink, keep_analeptic, keep_weapon = false, false, false
		for _, zcard in pairs(zcards) do
			if not isCard("Peach", zcard, self.player)  then
				local shouldUse = true
				if isCard("Slash", zcard, self.player) and not use_slash then
					local dummy_use = { isDummy = true , to = sgs.SPlayerList()}
					self:useBasicCard(zcard, dummy_use)
					if dummy_use.card then
						if dummy_use.to then
							for _, p in sgs.qlist(dummy_use.to) do
								if p:getHp() <= 1 then
									shouldUse = false
									if self.player:distanceTo(p) > 1 then keep_weapon = self.player:getWeapon() end
									break
								end
							end
							if dummy_use.to:length() > 1 then shouldUse = false end
						end
						if not self:isWeak() then shouldUse = false end
						if not shouldUse then use_slash = true end
					end
				end
				if zcard:getTypeId() == sgs.Card_TypeEquip and not self.player:hasEquip(zcard) then
					local dummy_use = { isDummy = true }
					self:useEquipCard(zcard, dummy_use)
					if dummy_use.card then shouldUse = false end
					if keep_weapon and zcard:getEffectiveId() == keep_weapon:getEffectiveId() then shouldUse = false end
				end
				if self.player:hasEquip(zcard) and zcard:isKindOf("Armor") and not self:needToThrowArmor() then shouldUse = false end
				if self.player:hasEquip(zcard) and zcard:isKindOf("DefensiveHorse") and not self:needToThrowArmor() then shouldUse = false end
				if isCard("Jink", zcard, self.player) and not keep_jink then
					keep_jink = true
					shouldUse = false
				end
				if self.player:getHp() == 1 and isCard("Analeptic", zcard, self.player) and not keep_analeptic then
					keep_analeptic = true
					shouldUse = false
				end
				if shouldUse then table.insert(unpreferedCards, zcard:getId()) end
			end
		end
	end

	if #unpreferedCards == 0 then
		local use_slash_num = 0
		self:sortByKeepValue(cards)
		for _, card in ipairs(cards) do
			if card:isKindOf("Slash") then
				local will_use = false
				if use_slash_num <= sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, self.player, card) then
					local dummy_use = { isDummy = true }
					self:useBasicCard(card, dummy_use)
					if dummy_use.card then
						will_use = true
						use_slash_num = use_slash_num + 1
					end
				end
				if not will_use then table.insert(unpreferedCards, card:getId()) end
			end
		end

		local num = self:getCardsNum("Jink") - 1
		if self.player:getArmor() then num = num + 1 end
		if num > 0 then
			for _, card in ipairs(cards) do
				if card:isKindOf("Jink") and num > 0 then
					table.insert(unpreferedCards, card:getId())
					num = num - 1
				end
			end
		end
		for _, card in ipairs(cards) do
			if (card:isKindOf("Weapon") and self.player:getHandcardNum() < 3) or card:isKindOf("OffensiveHorse")
				or self:getSameEquip(card, self.player)  then
				table.insert(unpreferedCards, card:getId())
			end
		end

		if self.player:getWeapon() and self.player:getHandcardNum() < 3 then
			table.insert(unpreferedCards, self.player:getWeapon():getId())
		end

		if self:needToThrowArmor() then
			table.insert(unpreferedCards, self.player:getArmor():getId())
		end

		if self.player:getOffensiveHorse() and self.player:getWeapon() then
			table.insert(unpreferedCards, self.player:getOffensiveHorse():getId())
		end
	end

	local use_cards = {}
	for index = #unpreferedCards, 1, -1 do
		if not self.player:isJilei(sgs.Sanguosha:getCard(unpreferedCards[index])) then table.insert(use_cards, unpreferedCards[index]) end
	end

	if #use_cards > 0 then
			use.card = sgs.Card_Parse("@WeizhiCard=" .. table.concat(use_cards, "+"))
			return
	end
end

sgs.ai_use_value.WeizhiCard =  9
sgs.ai_use_priority.WeizhiCard = 6
sgs.dynamic_value.benefit.WeizhiCard = true
sgs.ai_cardneed.weizhi = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  not card:isKindOf("TrickCard")
	end
end

sgs.ai_skill_cardask["@weizhuang-discard"] = function(self, data)
	local target =self.player:getTag("weizhuang_target"):toPlayer()
	local use =data:toCardUse()
	if not self:isEnemy(target) and not self:isFriend(target) then return "." end

	effect=false
	if self:isFriend(target)  then
		if ("snatch|dismantlement"):match(use.card:objectName())
		and target:getCards("j"):length()>0
		then
			effect = true
		end
		if use.card:isKindOf("GodSalvation") and target:isWounded() then
			effect = true
		end
	end

	if self:isEnemy(target) then
		effect=true
		if use.card:isKindOf("AmazingGrace") then
			effect = false
		end
		if use.card:isKindOf("GodSalvation")  then
			effect = false
		end
	end
	if not effect then return "." end
	local cards = sgs.QList2Table(self.player:getCards("hs"))
	cards1={}
	for _,card in pairs(cards) do
		if card:isKindOf("BasicCard") then
			table.insert(cards1,card)
		end
	end
	if #cards1==0 then return "." end
	self:sortByCardNeed(cards1)

	return "$" .. cards1[1]:getId()
end
sgs.ai_trick_prohibit.weizhuang = function(self, from, to, card)
	if  card:isNDTrick() then
		local basics={"Jink","Slash","Peach","Analeptic"}
		basic_num=0
		for _,str in pairs (basics) do
			basic_num=basic_num+ getCardsNum(str, from, self.player)
			if basic_num>0 then
				return false
			end
		end
		return true
	end
	return false
end


--function SmartAI:hasTrickEffective(card, to, from) ??
sgs.ai_skill_cardask["@zhengyi"] = function(self, data)
	local use =data:toCardUse()
	if self:touhouCardUseEffectNullify(use,self.player) then
		return "."
	end
	if use.card:isNDTrick() and not self:hasTrickEffective(use.card, self.player, use.from)then
		return "."
	end
	if use.card:isKindOf("Slash") and not self:slashIsEffective(use.card, self.player, use.from) then
		return "."
	end
	local pattern = self:lingqiParse(self,self.player,use)
	if pattern == 2 then
		local cards = {}
		for _,c in sgs.qlist(self.player:getCards("hes")) do
			if c:isRed() then
				table.insert(cards, c)
			end
		end
		if #cards==0 then return "." end
		self:sortByUseValue(cards, true)
		return "$" .. cards[1]:getId()
	end
	return "."
end
sgs.ai_cardneed.zhengyi = function(to, card, self)
	return  card:isRed()
end


sgs.ai_skill_playerchosen.baota = function(self, targets)
	local target =self:touhouFindPlayerToDraw(false, 1)
	if not target and #self.friends_noself>0 then
		target= self.friends_noself[1]
	end
	if target then
		return target
	end
	return nil
end
sgs.ai_playerchosen_intention.baota = -70
sgs.ai_no_playerchosen_intention.baota =function(self, from)
	local lord =self.room:getLord()
	if lord  then
		sgs.updateIntention(from, lord, 10)
	end
end


sgs.ai_skill_invoke.shuinan = function(self,data)
	local target=data:toPlayer()
	if target and self:isEnemy(target) then
		return true
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.shuinan = function(self, player, args)
	local use = player:getTag("shuinan_use"):toCardUse()
	if use and use.from then
		if args[#args] == "yes" then
			sgs.updateIntention(player, use.from, 40)
		end
	end
end


local nihuo_skill = {}
nihuo_skill.name = "nihuo"
table.insert(sgs.ai_skills, nihuo_skill)
nihuo_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("NihuoCard") then return nil end
	return sgs.Card_Parse("@NihuoCard=.")
end
sgs.ai_skill_use_func.NihuoCard=function(card,use,self)
	if #self.enemies == 0 then return end
	local n1 = self:getCardsNum("Slash")
	if n1 == 0 then return end

	local duel = sgs.cloneCard("duel")
	local targets = {}
	for _, from in ipairs(self.enemies) do
		if not from:isCardLimited(duel,sgs.Card_MethodUse) and not self.room:isProhibited(from, self.player, duel) then
			table.insert(targets, from)
		end
		--self:touhouCardAttackWaste(duel,self.player,enemy)
	end

	local cmp = function(a, b)
		local v1 = getCardsNum("Slash", a) + a:getHp()
		local v2 = getCardsNum("Slash", b) + b:getHp()

		if self:getDamagedEffects(a, self.player) then v1 = v1 + 20 end
		if self:getDamagedEffects(b, self.player) then v2 = v2 + 20 end

		if not self:isWeak(a) and a:hasSkill("jianxiong") then v1 = v1 + 10 end
		if not self:isWeak(b) and b:hasSkill("jianxiong") then v2 = v2 + 10 end

		if self:needToLoseHp(a) then v1 = v1 + 5 end
		if self:needToLoseHp(b) then v2 = v2 + 5 end

		if self:hasSkills(sgs.masochism_skill, a) then v1 = v1 + 5 end
		if self:hasSkills(sgs.masochism_skill, b) then v2 = v2 + 5 end

		if not self:isWeak(a) and a:hasSkill("jiang") then v1 = v1 + 5 end
		if not self:isWeak(b) and b:hasSkill("jiang") then v2 = v2 + 5 end

		if a:hasLordSkill("jijiang") or a:hasLordSkill("tianren")  then v1 = v1 + self:JijiangSlash(a) * 2 end
		if b:hasLordSkill("jijiang") or b:hasLordSkill("tianren")  then v2 = v2 + self:JijiangSlash(b) * 2 end

		if v1 == v2 then return sgs.getDefenseSlash(a, self) < sgs.getDefenseSlash(b, self) end

		return v1 < v2
	end

	if #targets == 0 then return end

	table.sort(targets, cmp)
	local nihuo_targets = {}
	for _, enemy in ipairs(targets) do
		local useduel
		local n2 = getCardsNum("Slash",enemy)
		if sgs.card_lack[enemy:objectName()]["Slash"] == 1 then n2 = 0 end
		useduel = n1 > n2 or self:needToLoseHp(self.player, nil, nil, true)
					or self:getDamagedEffects(self.player, enemy) or (n2 < 1 and sgs.isGoodHp(self.player))
					or ((self:hasSkill("jianxiong") or self.player:getMark("shuangxiong") > 0) and sgs.isGoodHp(self.player))
		-- 需要仇恨达到3么。。。
		if (not use.current_targets or not table.contains(use.current_targets, enemy:objectName()))
			and self:objectiveLevel(enemy) > 3 and not self:cantbeHurt(enemy) and useduel and sgs.isGoodTarget(enemy, enemies, self) then
			if not table.contains(nihuo_targets, enemy) then table.insert(nihuo_targets, enemy) end
		end
	end
	if #nihuo_targets == 0 then return end

	self:sort(nihuo_targets, "hp")
	use.card = card
	if use.to then
		use.to:append(nihuo_targets[1])
		if use.to:length() >= 1 then return end
	end
end
sgs.ai_use_priority.NihuoCard = sgs.ai_use_priority.Duel - 0.2
sgs.ai_card_intention.NihuoCard = 70
sgs.ai_cardneed.nihuo = function(to, card, self)
	return  card:isKindOf("Slash")
end


--SmartAI:getAoeValue(card, player)
--sgs.ai_skill_cardask.aoe
sgs.ai_skill_invoke.lizhi = function(self,data)
	local d = self.player:getTag("lizhi"):toDamage()
	local target = d.to
	if not self:isEnemy(target) then
		return true
	end
	local canDamage = self:touhouNeedAvoidAttack(d, self.player, target, true)
	if not canDamage then return true end
	local isSlash = false
	if d.card and d.card:isKindOf("Slash") then  isSlash = true end
	if self:getDamagedEffects(target, self.player, isSlash) or self:needToLoseHp(target, self.player, isSlash, true)  then return true end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.lizhi = function(self, player, args)
	local d = self.player:getTag("lizhi"):toDamage()
	if d and d.to then
		if args[#args] == "yes" then
			if self:isEnemy(player, d.to) then
			else
				sgs.updateIntention(player, d.to, -20)
			end
		elseif args[#args] == "no" then
			sgs.updateIntention(player, d.to, 60)
		end
	end
end
sgs.ai_cardneed.lizhi = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  card:isKindOf("AOE")
	end
end



sgs.ai_skill_invoke.souji = true


sgs.ai_skill_invoke.tansuo = true


sgs.ai_skill_playerchosen.yiwang = function(self, targets)
	local target_table= sgs.QList2Table(targets)
	self:sort(target_table,"hp")
	for _,p in pairs (target_table) do
		if self:isFriend(p) then
			return p
		end
	end
	return nil
end
sgs.ai_playerchosen_intention.yiwang = -70
sgs.ai_no_playerchosen_intention.yiwang = function(self, from)
	local lord = self.room:getLord()
	if lord and lord:isWounded() then
		sgs.updateIntention(from, lord, 10)
	end
end
sgs.yiwang_keep_value = {
	EquipCard = 7
}
sgs.ai_cardneed.yiwang = function(to, card, self)
		return  card:isKindOf("EquipCard")
end
sgs.ai_need_bear.yiwang = function(self, card,from,tos)
	from = from or self.player
	if card:isKindOf("EquipCard") then
		if not self:getSameEquip(card,from) then return false end
		for _,p in sgs.qlist(self.room:getAlivePlayers()) do
			if p:getLostHp()>0 and self:isFriend(from,p) then
				return false
			end
		end
		return true
	end
	return false
end


sgs.ai_skill_choice.jingxia=function(self)
	local damage=self.player:getTag("jingxia"):toDamage()
	local from=damage.from
	local fieldcard=sgs.SPlayerList()
	local fieldcard1=sgs.SPlayerList()
	for _, p in sgs.qlist(self.room:getAllPlayers()) do
		if self:isEnemy(p) then
			if self.player:canDiscard(p, "e")   then
				fieldcard:append(p)
			end
		end
		if self:isFriend(p) then
			if self.player:canDiscard(p, "j") and not p:containsTrick("lightning") then
				fieldcard1:append(p)
			end
		end
		if p:containsTrick("lightning") and  not self:invokeTouhouJudge() then
			fieldcard1:append(p)
		end
	end
	if self:isWeak(self.player) and self.player:canDiscard(self.player, "e") then
		return "discardfield"
	end
	if fieldcard1:length()>0 then return "discardfield" end
	if fieldcard:length()>1 then return "discardfield" end
	if from and self:isEnemy(from) and not from:isNude() and  self.player:canDiscard(from, "hes") then
		return "discard"
	end
	return "dismiss"
end
sgs.ai_skill_playerchosen.jingxia = function(self, targets)
	local fieldcard={}
	local fieldcard1={}
	for _, p in sgs.qlist(targets) do
		 if self:isEnemy(p) then
			if self.player:canDiscard(p, "e")   then
				table.insert(fieldcard,p)
			end
		end
		if self:isFriend(p) then
			if self.player:canDiscard(p, "j") and not p:containsTrick("lightning") then
				table.insert(fieldcard1,p)
			end
		end
		if p:containsTrick("lightning") and  not self:invokeTouhouJudge() then
			table.insert(fieldcard1,p)
		end
	end

	if self:isWeak(self.player) and self.player:getCards("e"):length()>0   and self.player:canDiscard(self.player, "e") then
		return self.player
	end

	if #fieldcard1>0 then return fieldcard1[1] end
	if #fieldcard > 0 then
		self:sort(fieldcard,"hp")
		return fieldcard[1]
	end
	return nil
end
sgs.ai_skill_cardchosen.jingxia = function(self, who, flags)
	if self:isFriend(who) then
		cards=who:getCards("j")
		cards = sgs.QList2Table(cards)
		for _,c in pairs (cards) do
			if c:isKindOf("Lightning")  then
				if not self:invokeTouhouJudge() then
					return c
				end
			else
				return c
			end
		end
		cards=who:getCards("e")
		cards = sgs.QList2Table(cards)
		if #cards>0 then
			return cards[1]
		end
	end
	if self:isEnemy(who) then
		if flags:match("j") then
			cards=who:getCards("j")
			cards = sgs.QList2Table(cards)
			for _,c in pairs (cards) do
				if c:isKindOf("Lightning")  then
					if not self:invokeTouhouJudge() then
						return c
					end
				end
			end
		end
		cards=who:getCards("e")
		cards = sgs.QList2Table(cards)
		if #cards>0 then
			return cards[1]
		end
		return sgs.Sanguosha:getCard(self:getCardRandomly(who, flags))
	end
end
--sgs.ai_playerchosen_intention.jingxia = 50
sgs.ai_choicemade_filter.cardChosen.jingxia = sgs.ai_choicemade_filter.cardChosen.dismantlement


sgs.ai_skill_invoke.bianhuan = function(self, data)
	local damage =data:toDamage()
	local x=self.player:getLostHp()
	--case:heavy damage
	if damage.damage>1 or x>1 then
		return true
	end
	--case:has losthp
	if self.player:getHp()<2 and x>0 then
		return true
	end
	--case:should die
	if self.player:getMaxHp() == 1 and self.player:getRole() == "rebel"
	and self:getAllPeachNum(self.player)+ self:getCardsNum("Analeptic")<damage.damage then
		local killer = self:findRealKiller(self.player, damage)
		if killer then
			return not self:isFriend(killer) and not killer:hasLordSkill("tymhwuyu")
		end
	end
	return false
end

local nuhuo_skill = {}
nuhuo_skill.name = "nuhuo"
table.insert(sgs.ai_skills, nuhuo_skill)
nuhuo_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("NuhuoCard") then return nil end
	local slash = sgs.cloneCard("slash", sgs.Card_NoSuit, 0)

	slash:deleteLater()
	if self.player:isCardLimited(slash,sgs.Card_MethodUse) then return nil end

	return sgs.Card_Parse("@NuhuoCard=.")
end
sgs.ai_skill_use_func.NuhuoCard=function(card,use,self)
	local slash = sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
	--可以用dummyuse更加简洁？？
	local targets={}--enemy
	local targets1={}--friend
	local others=self.room:getOtherPlayers(self.player)
	for _,p in sgs.qlist(others) do
		if  self.player:inMyAttackRange(p)
		and self:canAttack(p) and self.player:canSlash(p,slash,true)  then
			local fakeDamage=sgs.DamageStruct(slash, self.player, p, 1, self:touhouDamageNature(slash, self.player, p))
			local effect, willEffect = self:touhouDamageEffect(fakeDamage, self.player, p)
			if p:hasSkill("xuying") or self:touhouDamage(fakeDamage,self.player,p).damage>0   or effect then
				if self:isEnemy(p) then
					table.insert(targets,p)
				end
				if self:isFriend(p) then
					table.insert(targets1,p)
				end
			end
		end
	end
	if #targets==0 then return  end
	for _,p in ipairs(self.friends_noself) do
		if p:hasSkill("lizhi") then
			use.card = card
			if use.to then
				use.to:append(p)
				if use.to:length() >= 1 then return end
			end
		end
	end
	if self.player:getHp()<2 then return false end
	if #self.friends_noself <1 and #targets1>0 then return false end
	local chooser = #self.friends_noself>0 and self.friends_noself[1] or others:first()
	use.card = card
	if use.to then
		use.to:append(chooser)
		if use.to:length() >= 1 then return end
	end
end
sgs.ai_skill_playerchosen.nuhuo = function(self, targets)
	local slash = sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
	local source=self.room:getCurrent()
	local target_table= sgs.QList2Table(targets)
	self:sort(target_table,"hp")
	for _,p in pairs (target_table) do
		if self:isEnemy(p) and self:canAttack(p,source) then
			local fakeDamage=sgs.DamageStruct(slash, source, p, 1, self:touhouDamageNature(slash, source, p))
			local effect, willEffect = self:touhouDamageEffect(fakeDamage, source, p)
			if p:hasSkill("xuying") or self:touhouDamage(fakeDamage,source,p).damage>0   or effect then
				return p
			end
		end
	end
	return target_table[1]
end
sgs.ai_playerchosen_intention.nuhuo =function(self, from, to)
	local source=self.room:getCurrent()
	local slash = sgs.cloneCard("slash")
	for _,p in sgs.qlist(self.room:getOtherPlayers(source)) do
		if source:canSlash(p,slash,true) and not self:isFriend(p,to) then
			-- and self:canAttack(p,source)
			local fakeDamage=sgs.DamageStruct(slash, source, p, 1, self:touhouDamageNature(slash, source, p))
			local effect, willEffect = self:touhouDamageEffect(fakeDamage, source, p)
			if p:hasSkill("xuying") or self:touhouDamage(fakeDamage,source,p).damage>0   or effect then
				sgs.updateIntention(from, to, 10)
				break
			end
		end
	end
end

sgs.ai_skill_invoke.shanshi = function(self, data)
	local target = data:toPlayer()
	return target and self:isFriend(target)
end
sgs.ai_choicemade_filter.skillInvoke.shanshi = function(self, player, args)
	local target = player:getTag("shanshi"):toPlayer()
	if target  and args[#args] == "yes" then
		sgs.updateIntention(player,  target, -30)
	end
end


local shuxin_skill = {}
shuxin_skill.name = "shuxin"
table.insert(sgs.ai_skills, shuxin_skill)
shuxin_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ShuxinCard") then return nil end
	return sgs.Card_Parse("@ShuxinCard=.")
end

function SmartAI:shuxinValue(target)
	local value = 0
	if not self:isFriend(target) and not self:isEnemy(target) then return 0 end
	if self:touhouHandCardsFix(target) and self:isEnemy(target) then
		return 0
	end
	if self:touhouHandCardsFix(target) and self:isFriend(target) then
		value = value + 1
	end
	local canShanshi = self.player:hasSkill("shanshi") and self.player:getMark("shanshi_invoke") == 0
	if canShanshi and self:isFriend(target) and self.player:objectName() ~= target:objectName() then
		value = value + 1
	end
	local black = getKnownCard(target, self.player, "black", false, "hes", false)
	local recover = false
	if target:isWounded() and black >= target:getHp() then recover = true end
	if recover then
		if self:isFriend(target) then
			value = value + 2
		end
	elseif self:isEnemy(target) then
		value = value + 1
	end
	return value
end
sgs.ai_skill_use_func.ShuxinCard=function(card,use,self)
	local targets_table = {}
	for _,p in sgs.qlist(self.room:getAllPlayers()) do
		if not p:isKongcheng() then
			local array={player= p, value=self:shuxinValue(p)}
			table.insert(targets_table,array)
		end
	end
	local compare_func = function(a, b)
		return a.value > b.value
	end
	table.sort(targets_table, compare_func)

	if targets_table[1].value > 0 then
		use.card = card
		if use.to then
			use.to:append(targets_table[1].player)
			if use.to:length() >= 1 then return end
		end
	end
end
sgs.ai_use_priority.ShuxinCard = sgs.ai_use_priority.Dismantlement + 0.2
--情况太tm复杂
sgs.ai_card_intention.ShuxinCard = function(self, card, from, tos)
	local canShanshi = from:hasSkill("shanshi") and from:getMark("shanshi_invoke") == 0
	if not tos[1]:isWounded() and not canShanshi and not self:touhouHandCardsFix(tos[1]) then
		sgs.updateIntention(from, tos[1], 50)
	end
end
sgs.ai_skill_cardask["@shuxin"] = function(self, data)
	local hasBlack = false
	local blacks = {}
	for _,c in sgs.qlist(self.player:getCards("hes")) do
		if c:isBlack() then
			table.insert(blacks, c)
			if self.room:getCardPlace(c:getEffectiveId()) == sgs.Player_PlaceHand then
				hasBlack = true
			end
		end
	end
	local cost = 0
	if self.player:isWounded() and #blacks >= self.player:getHp() then
		if self:isWeak(self.player) or self.player:getHp() <= 2 then
			cost = self.player:getHp()
		end
	end
	if hasBlack then
		cost = math.max(cost, 1)
	end



	if cost > 0 then
		self:sortByKeepValue(blacks, true)
		local ids = {}
		for i = 1, cost, 1 do
			table.insert(ids, blacks[i]:getId())
		end
		return "$" .. table.concat(ids, "+")
	end
	return "."
end
