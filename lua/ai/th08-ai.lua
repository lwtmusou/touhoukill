--蓬莱山辉夜
--[永恒]
sgs.ai_cardneed.yongheng = function(to, card, self)
	return  card:isKindOf("Spear")
end
sgs.ai_skill_invoke.yongheng =  true
sgs.ai_skill_invoke.yongheng_hegemony =  true

--[竹取]
sgs.ai_skill_invoke.zhuqu = function(self, data)
		local to =data:toPlayer()
		return self:isFriend(to)
end
sgs.ai_choicemade_filter.skillInvoke.zhuqu = function(self, player, args, data)
	local to=data:toPlayer()
	if to then
		if args[#args] == "yes" then
			sgs.updateIntention(player, to, -60)
		else
			sgs.updateIntention(player, to, 60)
		end
	end
end

--[须臾 国]
sgs.ai_skill_invoke.xuyu_hegemony  = function(self)
	return true
end

--八意永琳
--[睿智]
sgs.ai_skill_invoke.ruizhi  = function(self)
	return self:invokeTouhouJudge()
end
sgs.ai_need_damaged.ruizhi = function(self, attacker, player)
	if not attacker then return end
	if player:getHp()<player:getMaxHp() then return end
	effect=self.player:getTag("ruizhi_effect"):toCardEffect()
	if not effect or not effect.card  then return end
	local damageTrick = self:touhouIsDamageCard(effect.card)
	if damageTrick and not self:needtouhouDamageJudge()  then
		return false
	end

	if damageTrick  then
		local fakeDamage=sgs.DamageStruct()
		fakeDamage.card=effect.card
		fakeDamage.nature= self:touhouDamageNature(effect.card,effect.from,effect.to)
		fakeDamage.damage=1
		fakeDamage.from=effect.from
		fakeDamage.to= effect.to
		local finalDamage = self:touhouDamage(fakeDamage,effect.from,effect.to)
		return finalDamage.damage < 2
	end
	return damageTrick
end
sgs.ai_skillProperty.ruizhi = function(self)
	return "cause_judge"
end

--[秘药]
local miyao_skill = {}
miyao_skill.name = "miyao"
table.insert(sgs.ai_skills, miyao_skill)
function miyao_skill.getTurnUseCard(self)
	if self.player:hasUsed("MiyaoCard") then return nil end
	 return sgs.Card_Parse("@MiyaoCard=.")
end
sgs.ai_skill_use_func.MiyaoCard = function(card, use, self)
		local kaguya
		local friendtarget
		self:sort(self.friends_noself,"hp")
		for _, p in ipairs(self.friends_noself) do
			if self:touhouHandCardsFix(p) then
				kaguya=p
			end
			if p:isWounded() and not p:isKongcheng() and  p:getHp()+1 <= getBestHp(p) then
				friendtarget=p
				break
			end
		end

		sgs.ai_use_priority.MiyaoCard = friendtarget and 3 or 8

		if friendtarget then
			use.card = card
			if use.to then
				use.to:append(friendtarget)
				if use.to:length() >= 1 then return end
			end
		end
		self:sort(self.enemies,"handcard")
		for _, p in ipairs(self.enemies) do
			if (not p:isWounded()) and (not p:isKongcheng())
			and not  self:touhouHandCardsFix(p) then
				use.card = card
				if use.to then
						use.to:append(p)
						if use.to:length() >= 1 then return end
				end
			end
		end
		if kaguya then
			use.card = card
			if use.to then
				use.to:append(kaguya)
				if use.to:length() >= 1 then return end
			end
		end
end
sgs.ai_use_value.MiyaoCard = 10

sgs.ai_card_intention.MiyaoCard = function(self, card, from, tos)
	if tos[1]:isWounded() then
		sgs.updateIntentions(from, tos, -90)
	else
		if self:touhouHandCardsFix(tos[1]) then
			sgs.updateIntentions(from, tos, -10)
		else
			sgs.updateIntentions(from, tos, 40)
		end
	end
end
sgs.ai_skill_discard.miyao = function(self)
	local to_discard = {}
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)

	table.insert(to_discard, cards[1]:getEffectiveId())

	return to_discard
end

--[药矢 国]
sgs.ai_skill_invoke.yaoshi_hegemony = function(self, data)
	local to =data:toPlayer()
	return self:isFriend(to)
end

--藤原妹红
--[凯风]
sgs.ai_skill_invoke.kaifeng = true
sgs.ai_benefitBySlashed.kaifeng = function(self, card,source,target)
	if card:isKindOf("FireSlash")  and  target:getHp() < source:getHp()
	and  target:getHp() > target:dyingThreshold() then
		return true
	end
	return false
end
sgs.ai_slash_prohibit.kaifeng = function(self, from, to, card)
	if self:isFriend(from,to) then
		return false
	end
	if card:isKindOf("FireSlash")  and  to:getHp() < from:getHp()
	and  to:getHp() > to:dyingThreshold() then
		return true
	end
	return false
end

--[凤翔]
sgs.ai_skill_invoke.fengxiang_show = true
local fengxiang_skill={}
fengxiang_skill.name="fengxiang"
table.insert(sgs.ai_skills,fengxiang_skill)
fengxiang_skill.getTurnUseCard=function(self)
	local cards = self.player:getCards("hs")
	cards=self:touhouAppendExpandPileToList(self.player, cards)
	cards=sgs.QList2Table(cards)
	local card
	self:sortByUseValue(cards,true)
	for _,acard in ipairs(cards) do
		if acard:isRed() and not acard:isKindOf("Peach") and (self:getDynamicUsePriority(acard) < sgs.ai_use_value.FireAttack or self:getOverflow() > 0) then
			if acard:isKindOf("Slash") and self:getCardsNum("Slash") == 1 then
				local keep
				local dummy_use = { isDummy = true , to = sgs.SPlayerList() }
				self:useBasicCard(acard, dummy_use)
				if dummy_use.card and dummy_use.to and dummy_use.to:length() > 0 then
					for _, p in sgs.qlist(dummy_use.to) do
						if p:getHp() <= 1 then keep = true break end
					end
					if dummy_use.to:length() > 1 then keep = true end
				end
				if keep then sgs.ai_use_priority.Slash = sgs.ai_use_priority.FireAttack + 0.1
				else
					sgs.ai_use_priority.Slash = 2.6
					card = acard
					break
				end
			else
				card = acard
				break
			end
		end
	end
	if not card then return nil end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local card_str = ("fire_attack:fengxiang[%s:%s]=%d"):format(suit, number, card_id)
	local skillcard = sgs.Card_Parse(card_str)
	assert(skillcard)
	return skillcard
end
sgs.ai_cardneed.fengxiang = function(to, card, self)
	return card:isRed()
end


--[凤翔 国]
local fengxiang_hegemony_skill={}
fengxiang_hegemony_skill.name="fengxiang_hegemony"
table.insert(sgs.ai_skills,fengxiang_hegemony_skill)
fengxiang_hegemony_skill.getTurnUseCard=function(self)
	local cards = self.player:getCards("hs")
	cards=self:touhouAppendExpandPileToList(self.player, cards)
	cards=sgs.QList2Table(cards)
	local card
	self:sortByUseValue(cards,true)
	for _,acard in ipairs(cards) do
		if acard:isRed() and not acard:isKindOf("Peach") and (self:getDynamicUsePriority(acard) < sgs.ai_use_value.FireAttack or self:getOverflow() > 0) then
			if acard:isKindOf("Slash") and self:getCardsNum("Slash") == 1 then
				local keep
				local dummy_use = { isDummy = true , to = sgs.SPlayerList() }
				self:useBasicCard(acard, dummy_use)
				if dummy_use.card and dummy_use.to and dummy_use.to:length() > 0 then
					for _, p in sgs.qlist(dummy_use.to) do
						if p:getHp() <= 1 then keep = true break end
					end
					if dummy_use.to:length() > 1 then keep = true end
				end
				if keep then sgs.ai_use_priority.Slash = sgs.ai_use_priority.FireAttack + 0.1
				else
					sgs.ai_use_priority.Slash = 2.6
					card = acard
					break
				end
			else
				card = acard
				break
			end
		end
	end
	if not card then return nil end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local card_str = ("fire_attack:fengxiang_hegemony[%s:%s]=%d"):format(suit, number, card_id)
	local skillcard = sgs.Card_Parse(card_str)
	assert(skillcard)
	return skillcard
end
sgs.ai_cardneed.fengxiang_hegemony = function(to, card, self)
	return card:isRed()
end

--[凯风 国]
sgs.ai_skill_invoke.kaifeng_hegemony = true



--铃仙·优昙华院·因幡
--[狂躁]
function SmartAI:isKuangzaoTarget(enemy)
	if not enemy then self.room:writeToConsole(debug.traceback()) return end
	if not enemy:inMyAttackRange(self.player) then  return false end
	local lackSlash = false
	if (getCardsNum("Slash", enemy, self.player) < 1
		or sgs.card_lack[enemy:objectName()]["Slash"] == 1) then
		lackSlash = true
	end

	local damageBenefit = true
	if self:getDamagedEffects(enemy, nil, false) then
		damageBenefit = false
	else
		local fakeDamage=sgs.DamageStruct()
		fakeDamage.nature= sgs.DamageStruct_Normal
		fakeDamage.damage=1
		fakeDamage.to=enemy
		damageBenefit = self:touhouNeedAvoidAttack(fakeDamage,nil,enemy)
	end
	local huanshiTarget
	if self.player:hasSkill("huanshi") then
		local targets = sgs.SPlayerList()
		for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if p:objectName() ~= enemy:objectName() and enemy:inMyAttackRange(p) then
				targets:append(p)
			end
		end
		if targets:length()>0 then
			local fakeUse = sgs.CardUseStruct()
			fakeUse.from = enemy
			local faketo = sgs.SPlayerList()
			faketo:append(self.player)
			fakeUse.to = faketo
			fakeUse.card = sgs.cloneCard("slash")
			local _data = sgs.QVariant()
			_data:setValue(fakeUse)
			self.player:setTag("huanshi_source",_data)
			huanshiTarget = sgs.ai_skill_playerchosen.huanshi(self, targets)
		end
	end
	local enemyStatus = 0
	if not lackSlash  then
		enemyStatus = enemyStatus +40
		enemyStatus = enemyStatus + 10*(getCardsNum("Slash", enemy, self.player))
	end
	if not damageBenefit then
		enemyStatus = enemyStatus + 100
	end
	if huanshiTarget then
		enemyStatus = enemyStatus - 20
	end
	local selfStatus = 0
	if  self:getCardsNum("Jink") > 0  then
		selfStatus  = selfStatus +50
	end
	if self.player:getArmor() and not self:touhouIgnoreArmor(sgs.cloneCard("slash"),enemy,self.player) then
		selfStatus  = selfStatus + 70
		if self.player:hasArmorEffect("SilverLion") then
			selfStatus  = selfStatus - 60
		end
	end
	selfStatus = selfStatus + self.player:getHp()*10
	return selfStatus > enemyStatus
end
local kuangzao_skill = {}
kuangzao_skill.name = "kuangzao"
table.insert(sgs.ai_skills, kuangzao_skill)
function kuangzao_skill.getTurnUseCard(self)
	if self.player:hasUsed("KuangzaoCard") then return nil end
	return sgs.Card_Parse("@KuangzaoCard=.")
end
sgs.ai_skill_use_func.KuangzaoCard = function(card, use, self)
		local slash=sgs.cloneCard("slash")
		self:sort(self.enemies,"handcard")
		for _, p in ipairs(self.enemies) do
			if  self:isKuangzaoTarget(p) then
					use.card = card
					if use.to then
						use.to:append(p)
						if use.to:length() >= 1 then return end
					end
			end
		end
end
sgs.ai_use_value.KuangzaoCard = 8
sgs.ai_use_priority.KuangzaoCard = 0
sgs.ai_card_intention.KuangzaoCard = 50
sgs.ai_skill_cardask["@kuangzao-annoying"] = function(self, data)
	local cards = sgs.QList2Table(self.players:getCards("hes"))

	local effect = data:toCardEffect()
	local from = effect.from
	self.player:setFlags("slashTargetFix")
	self.player:setFlags("slashNoDistanceLimit")
	self.player:setFlags("slashTargetFixToOne")
	effect.from:setFlags("SlashAssignee")
	local slashUse = sgs.ai_skill_use.slash(self, "dummy")
	effect.from:setFlags("-SlashAssignee")
	self.player:setFlags("-slashTargetFixToOne")
	self.player:setFlags("-slashNoDistanceLimit")
	self.player:setFlags("-slashTargetFix")
	local slash
	if slashUse ~= "." then
		local slashStr = string.split(slashUse, "->")
		slash = sgs.Card_Parse(slashStr)
	end

	if self.player:getArmor() and self.player:getArmor():isKindOf("SliverLion") and self.player:isWounded() and not slash then
		return tostring(self.player:getArmor():getId())
	end
	
	self:sortByKeepValue(cards)
	for _, c in ipairs(cards) do
		if not slash then
			return tostring(c:getId())
		end
		if not slash:isVirtualCard() then
			if c:getId() ~= slash:getId() then
				return tostring(c:getId())
			end
		else
			for _, id in slash:getSubcards() do
				if c:getId() ~= id then
					return tostring(c:getId())
				end
			end
		end
	end
	return tostring(cards[1]:getId())
end

--[幻视]
sgs.ai_skill_playerchosen.huanshi = function(self, targets)
	local enemies={}
	local friends= sgs.SPlayerList()
	local attacker=self.player:getTag("huanshi_source"):toCardUse().from
	local slash = self.player:getTag("huanshi_source"):toCardUse().card
	for _,p in sgs.qlist(targets) do
		if self:isEnemy(p) then
			table.insert(enemies,p)
		elseif self:isFriend(p) then
			friends:append(p)
		end
	end
	if #enemies>0 then
		self:sort(enemies, "defenseSlash")
		for i=1, #enemies ,1 do
			if self:slashIsEffective(slash, enemies[i], attacker) and not self:getDamagedEffects(enemies[i], attacker, true) then
				local fakeDamage=sgs.DamageStruct()
				fakeDamage.nature= self:touhouDamageNature(slash,attacker,enemies[i])
				fakeDamage.damage = 1
				fakeDamage.from = attacker
				fakeDamage.to=enemies[i]
				if self:touhouNeedAvoidAttack(fakeDamage,attacker,enemies[i]) then
					return enemies[i]
				end
			end
		end
	end
	for _,f in sgs.qlist(friends) do
		if self:getDamagedEffects(f, attacker, true) then
			return f
		end
	end
	return nil
end
sgs.ai_playerchosen_intention.huanshi = function(self, from, to)
	local attacker = from:getTag("huanshi_source"):toCardUse().from
	if  not self:getDamagedEffects(to, attacker, true) then
		sgs.updateIntention(from, to, 60)
	end
end

--上白泽慧音
--[虚史]
sgs.ai_skill_invoke.xushi =function(self,data)
	local use=self.player:getTag("xushi_use"):toCardUse()
	--对敌无脑true
        if self:isEnemy(use.from) then
		return true
	end
	return false
end

--只指定一个？
sgs.ai_skill_playerchosen.xushi = function(self, targets)
	local use=self.player:getTag("xushi_use"):toCardUse()
    for _,p in sgs.qlist(targets) do
        	if self:isEnemy(p) == self:isEnemy(use.to:first()) then
			return p
        end
	end

	return targets:first()
end
sgs.ai_skill_invoke.shishi =function(self,data)
	local use=self.player:getTag("shishi_use"):toCardUse()
	if self:isEnemy(use.from) then
		return true
	else
		if self:touhouIsDamageCard(use.card) then
			local f=0
			for _,p in sgs.qlist(use.to) do
				if self:isFriend(p) then
					f=f+1
				end
			end
			if f>0 and use.to:length()-f<f then
				return true
			end
		end
	end
	return false
end

--[新月]
sgs.ai_skill_invoke.xinyue =function(self,data)
	local target=data:toDamage().from
	if target and self:isEnemy(target) then
		return true
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.xinyue = function(self, player, args, data)
	local damage = data:toDamage()
	local from=damage.from
	if from:getHandcardNum()>player:getHp() then
		if args[#args] == "yes" then
			sgs.updateIntention(player, from, 60)
		else
			sgs.updateIntention(player, from, -60)
		end
	end
end
sgs.ai_need_damaged.xinyue = function(self, attacker, player)
	if not self:getDamageSource(attacker) then return false end
	if  self:isEnemy(attacker,player) then
		if player:getLostHp()>0 then
			return false
		else
			if attacker:getHandcardNum() >= 5  and not self:touhouDrawCardsInNotActive(attacker) then
				return true
			end
		end
	end
	return false
end
--[新月 国]
sgs.ai_skill_invoke.xinyue_hegemony =function(self,data)
	local target=data:toDamage().from
	if target and self:isEnemy(target) and target:getHandcardNum() > self.player:getHp() then
		return true
	end
	return false
end

--因幡天为
--[布陷]
sgs.ai_skill_use["@@buxian"] = function(self, prompt)
	local handcards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(handcards)
	if #handcards==0 then return "." end

	self:sort(self.enemies,"handcard")
	local targets={}
	for _, p in ipairs(self.enemies) do
		if not p:isKongcheng()  then
			table.insert(targets,p:objectName())
		end
		if #targets==2 then
			break
		end
	end
	if #targets >1 then
		return "@BuxianCard=".. handcards[1]:getId() .."->" .. table.concat(targets, "+")
	end
	return "."
end
sgs.ai_card_intention.BuxianCard = 50
--[幸运]
sgs.ai_skill_use["@@xingyun"] = function(self, prompt)
	local move = self.player:getTag("xingyun_move"):toMoveOneTime()
	local ids = {}
	for _, id in sgs.qlist(move.card_ids) do
		if sgs.Sanguosha:getCard(id):getSuit() == sgs.Card_Heart
		  and self.room:getCardPlace(id) == sgs.Player_PlaceHand then
			table.insert(ids, id)
		end
	end
	if #ids > 0 then
		return "@XingyunCard=" .. table.concat(ids, "+")
	end
	return "."
end
--sgs.ai_skill_invoke.xingyun = true
sgs.ai_skill_choice.xingyun = "recover"
sgs.ai_skill_playerchosen.xingyun = function(self, targets)
	target_table = self:getFriends(self.player)
	if #target_table==0 then return false end
	local xingyun_target
	self:sort(target_table, "handcard")

	for _,target in pairs(target_table) do
		if not xingyun_target then
			xingyun_target=target
		else
			if self:isWeak(target) and (not self:isWeak(xingyun_target)) then
				xingyun_target=target
			end
		end
	end
	if xingyun_target  then
		return xingyun_target
	end
	return nil
end
sgs.ai_playerchosen_intention.xingyun = -60
sgs.ai_cardneed.xingyun = function(to, card, self)
	return card:getSuit()==sgs.Card_Heart
end
sgs.xingyun_suit_value = {
	heart = 4.8
}

--[幸运 国]
sgs.ai_skill_use["@@xingyun_hegemony"] = function(self, prompt)
	local move = self.player:getTag("xingyun_move"):toMoveOneTime()
	local ids = {}
	for _, id in sgs.qlist(move.card_ids) do
		if sgs.Sanguosha:getCard(id):getSuit() == sgs.Card_Heart
		  and self.room:getCardPlace(id) == sgs.Player_PlaceHand then
			table.insert(ids, id)
		end
	end
	if #ids > 0 then
		return "@XingyunHegemonyCard=" .. ids[1]
	end
	return "."
end


--米斯蒂娅·萝蕾拉
--[夜歌]
sgs.ai_skill_use["@@yege"] = function(self, prompt)
	local current = self.room:getCurrent()
	if self:isEnemy(current) then
		local cards = self.player:getHandcards()
		cards=self:touhouAppendExpandPileToList(self.player,cards)

	    if self.room:getMode():find("hegemony") then
		    local diamonds = {}
			for _,c in sgs.qlist(cards) do
				if c:getSuit() == sgs.Card_Diamond then
					table.insert(diamonds, c)
				end
			end
			self:sortByUseValue(diamonds)
			if #diamonds > 0 then
				return "@YegeCard=".. diamonds[1]:getId() .."->" .. current:objectName()
			end
		end
		cards = sgs.QList2Table(cards)
		self:sortByUseValue(cards)
		if #cards == 0 then return "." end
		return "@YegeCard=".. cards[1]:getId() .."->" .. current:objectName()
	end
	return "."
end

sgs.ai_cardneed.yege = function(to, card, self)
    if self.room:getMode():find("hegemony") then
		return card:getSuit()==sgs.Card_Diamond
	end
end

--[牢笼]
--默认发动?
-- Fs：只是随便找个目标而已，askForPlayerChosen就这样


--莉格露·奈特巴格
--[萤火]
sgs.ai_skill_invoke.yinghuo  = true
sgs.ai_cardneed.yinghuo = function(to, card, self)
	return card:getTypeId() == sgs.Card_TypeBasic
end

--视为技的萤火
local yinghuo_skill = {}
yinghuo_skill.name = "yinghuo"
table.insert(sgs.ai_skills, yinghuo_skill)
function yinghuo_skill.getTurnUseCard(self)
	local slashes = {}
	local anas = {}
	local peaches = {}
	for _, c in sgs.qlist(self.player:getCards("h")) do
		if c:isKindOf("Slash") and self.player:getMark("yinghuo_record_slash") == 0 then
			table.insert(slashes, c)
		elseif c:isKindOf("Analeptic") and self.player:getMark("yinghuo_record_analeptic") == 0 then
			table.insert(anas, c)
		elseif c:isKindOf("Peach") and self.player:getMark("yinghuo_record_peach") == 0 and self.player:isWounded() and c:isAvailable(self.player) then
			table.insert(peaches, c)
		end
	end

	local card
	if #peaches > 0 then
		card = peaches[1]
	elseif #slashes>0 and #anas >0   then  --and self:shouldUseAnaleptic(self.player, slashes[1])

		if sgs.Analeptic_IsAvailable(self.player, anas[1]) and not self.player:isCardLimited(anas[1], sgs.Card_MethodUse) then
			card = anas[1]
		end
	elseif #slashes>0 and sgs.Slash_IsAvailable(self.player) then
		card = slashes[1]
	end
	if not card then return nil end
	return sgs.Card_Parse("@YinghuoCard=" .. card:getEffectiveId())
end
sgs.ai_skill_use_func.YinghuoCard=function(card,use,self)
	local tmp = sgs.Sanguosha:getCard((card:getSubcards():first()))
	if (tmp:isKindOf("Analeptic") or tmp:isKindOf("Peach")) then
		use.card = card
	else
		local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
		local slash=sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
		slash:setSkillName("yinghuo")
		slash:deleteLater()
		local target

		self:useBasicCard(slash, dummy_use)
		if not dummy_use.to:isEmpty() then
			for _, p in sgs.qlist(dummy_use.to) do
				if self:isEnemy(p) then
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
end

sgs.ai_use_priority.YinghuoCard = sgs.ai_use_priority.Slash + 0.4
function sgs.ai_cardsview_valuable.yinghuo(self, class_name, player)
	local card
	if  (class_name == "Analeptic"  and player:getMark("yinghuo_record_analeptic") == 0 )
	or (class_name == "Peach" and player:getMark("yinghuo_record_peach") == 0) then
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying then return nil end
		if self:isFriend(dying, player) then
			for _,c in sgs.qlist(self.player:getCards("h")) do
				if c:isKindOf(class_name) then
					card = c
					break
				end
			end
			if card then
				return "@YinghuoCard=" .. card:getEffectiveId()
			end
		end
	elseif (class_name == "Jink" and player:getMark("yinghuo_record_jink") == 0)
	or (class_name == "Slash" and player:getMark("yinghuo_record_slash") == 0) then
		for _,c in sgs.qlist(self.player:getCards("h")) do
			if c:isKindOf(class_name) then
				card = c
				break
			end
		end
		if card then
			return "@YinghuoCard=" .. card:getEffectiveId()
		end
	end
end

--[虫群]
sgs.ai_skill_playerchosen.chongqun = function(self, targets)
	targets=sgs.QList2Table(targets)
	self:sort(targets,"handcard")
	for _,p in pairs(targets)do
		if self:isEnemy(p) and self.player:canDiscard(p, "hs") and not self:touhouHandCardsFix(p) then
			return p
		end
	end
	return nil
end

sgs.ai_playerchosen_intention.chongqun =function(self, from, to)
	local intention = 50
	if self:touhouHandCardsFix(to)  then
		intention = 0
	end
	sgs.updateIntention(from, to, intention)
end

--[萤火 国]
sgs.ai_skill_invoke.yinghuo_hegemony = true
--[虫群 国]
sgs.ai_skill_playerchosen.chongqun_hegemony = function(self, targets)
	targets=sgs.QList2Table(targets)
	self:sort(targets,"handcard")
	for _,p in pairs(targets)do
		if self:isEnemy(p) and self.player:canDiscard(p, "hs") and not self:touhouHandCardsFix(p) then
			return p
		end
	end
	return nil
end

--白泽
--[创史]
sgs.ai_skill_playerchosen.chuangshi = function(self, targets)
	if self.player:getMark("chuangshi")>0 then return nil end


	if #self.friends_noself>0 then
		self:sort(self.friends_noself, "hp")
		for _,p in pairs(self.friends_noself) do
			if self:isWeak(p) and p:isWounded() then
				self.player:setTag("chuangshi_mode",sgs.QVariant("save"))
				return p
			end
		end
	end
	local weaker
	local attacker
	if #self.enemies>0 then
		self:sort(self.enemies, "hp")
		for _,p in pairs(self.enemies) do
			if self:isWeak(p) then
				weaker=p
				break
			end
		end
	end
	if weaker then
		local _data=sgs.QVariant()
		_data:setValue(weaker)
		self.player:setTag("chuangshi_victim",_data)
		self:sort(self.friends_noself, "handcard",true)
		if #self.friends_noself>0 then
			self.player:setTag("chuangshi_mode",sgs.QVariant("kill1"))
			_data=sgs.QVariant()
			_data:setValue(self.friends_noself[1])
			self.player:setTag("chuangshi_attacker",_data)
			return self.friends_noself[1]
		end
	end
	if #self.enemies>1 then
		self.player:setTag("chuangshi_mode",sgs.QVariant("kill2"))
		_data=sgs.QVariant()
		_data:setValue(self.enemies[2])
		self.player:setTag("chuangshi_attacker",_data)
		if not weaker then
			_data=sgs.QVariant()
			_data:setValue(self.enemies[1])
			self.player:setTag("chuangshi_victim",_data)
		end
		return self.enemies[2]
	end
	return nil
end

sgs.ai_skill_use["@@chuangshi"] = function(self, prompt)
	local str=self.player:getTag("chuangshi_mode"):toString()
	self.player:removeTag("chuangshi_mode")
	local user
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if (p:getMark("chuangshi_user") >0 ) then
				user= p
				break
			end
	end

	local cardname
	if str =="save" then cardname= "peach"
	elseif str =="kill1" then
		local killer=self.player:getTag("chuangshi_attacker"):toPlayer()
		local target=self.player:getTag("chuangshi_victim"):toPlayer()
		self.player:removeTag("chuangshi_attacker")
		if killer:canSlash(target,nil,true) then
			cardname="fire_slash"
		else
			cardname="duel"
		end
	elseif str =="kill2" then
		self.player:removeTag("chuangshi_attacker")
		cardname="duel"
	end

	local target
	target=self.player:getTag("chuangshi_victim"):toPlayer()
	--self.player:removeTag("chuangshi_victim")


	if cardname and user then
		self.room:setPlayerMark(self.player, "chuangshi", self.player:getMark("chuangshi")+1)
		--self.room:setPlayerMark(user, "chuangshi_user", 0)

		self.room:setPlayerFlag(self.player, "chuangshi")
		if target then
			return "@ChuangshiCard=.:" .. cardname .. "->" .. target:objectName()
		else
			return "@ChuangshiCard=.:" .. cardname .. "->" .. user:objectName()
		end
	end
	return "."
end
local chuangshi_filter = function(self, player, carduse)
	if carduse.card:getSkillName(true)=="chuangshi" then
		sgs.ai_chuangshi_effect = true
	end
end

--[望月]
sgs.ai_skill_invoke.wangyue = true
--[望月 国]
sgs.ai_skill_invoke.wangyue_hegemony =function(self,data)
	local target=data:toDamage().from
	if target and target:getHandcardNum() > self.player:getHandcardNum() then
		return true
	end
	return false
end


--SP自警队妹红
--[护卫]
sgs.ai_skill_invoke.huwei = true
sgs.ai_skill_playerchosen.huwei = function(self, targets)
	local target =self:touhouFindPlayerToDraw(false, 2)
	if target then return target end
	if #self.friends_noself>0 then return self.friends_noself[1] end
	return nil
end
function sgs.ai_cardsview_valuable.huwei(self, class_name, player)
	local tmpslash = sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
	if player:isCardLimited(tmpslash, sgs.Card_MethodUse) then  return nil end
	if self:touhouClassMatch(class_name, "Slash") and (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE)  then
		 if not player:hasFlag("Global_huweiFailed") and player:getPhase() ~=sgs.Player_Play then
			for _,p in sgs.qlist(player:getAliveSiblings()) do
				if (tmpslash:targetFilter(sgs.PlayerList(), p, player)) then
					return "@HuweiCard=."
				end
			end
		end
	end
end
sgs.ai_playerchosen_intention.huwei = -60
sgs.ai_no_playerchosen_intention.huwei =function(self, from)
	local lord =self.room:getLord()
	if lord  then
		if lord:getPhase() ==sgs.Player_NotActive then
			if not lord:hasSkills("gaoao|yongheng")  then
				sgs.updateIntention(from, lord, 10)
			end
		else
			sgs.updateIntention(from, lord, 10)
		end
	end
end
sgs.ai_trick_prohibit.huwei = function(self, from, to, card)
	if not card:isKindOf("Duel")  then return false end
	if self:isFriend(from,to) then return false end
	return true
end
sgs.ai_cardneed.huwei = function(to, card, self)
	return  card:isKindOf("Slash")
end

--[今昔]
local jinxi_skill = {}
jinxi_skill.name = "jinxi"
table.insert(sgs.ai_skills, jinxi_skill)
function jinxi_skill.getTurnUseCard(self)
	if self.player:getMark("@jinxi")==0 or not self.player:isWounded() then return nil end
	local value = self.player:getCards("hs"):length() + self.player:getHp()
	if value <=3 then
		return sgs.Card_Parse("@JinxiCard=.")
	end
	return nil
end
sgs.ai_skill_use_func.JinxiCard=function(card,use,self)
	use.card = card
end
sgs.ai_use_value.JinxiCard = 2
sgs.ai_use_priority.JinxiCard = 1

--SP米斯蒂娅
--[明目]
local mingmuvs_skill = {}
mingmuvs_skill.name = "mingmu_attach"
table.insert(sgs.ai_skills, mingmuvs_skill)
function mingmuvs_skill.getTurnUseCard(self)
		local cards = self.player:getCards("hes")
		if cards:isEmpty() then return nil end
		local source = self.room:findPlayerBySkillName("mingmu")
		if not source or source:hasFlag("mingmuInvoked") or not self:isFriend(source) then return nil end
		local give = self:getOverflow(self.player) > 0

		if not give and sgs.Slash_IsAvailable(self.player) then
			local slash = self:getCard("Slash")
			--攻击范围+1后能否够到敌人
			if slash then
				local new_range = self.player:getAttackRange() +1
				for _,p in ipairs(self.enemies) do
					if new_range == self.player:distanceTo(p) then
						give = true
						return
					end
				end
			end
		end

		if not give then return nil end
		cards = sgs.QList2Table(cards)
		self:sortByKeepValue(cards)
		local mingmu_cards={}
		table.insert(mingmu_cards, cards[1]:getEffectiveId())
		if #mingmu_cards>0 then
			local card_str= "@MingmuCard=" .. table.concat(mingmu_cards, "+")
			return sgs.Card_Parse(card_str)
		end
end
sgs.ai_skill_use_func.MingmuCard = function(card, use, self)
	local targets = {}
	for _,friend in ipairs(self.friends_noself) do
		if friend:hasSkill("mingmu") then
			if not friend:hasFlag("mingmuInvoked") then
				table.insert(targets, friend)
			end
		end
	end
	if #targets > 0 then
		use.card = card
		if use.to then
			use.to:append(targets[1])
			return
		end
	end
end

sgs.ai_skill_choice.mingmu = function(self, choices, data)
	local choice_table = choices:split("+")
	return choice_table[1]
end

