sgs.ai_cardneed.yongheng = function(to, card, self)
	return  card:isKindOf("Spear")
end


sgs.ai_skill_invoke.zhuqu = function(self, data)
		local to =data:toPlayer()
		return self:isFriend(to)
end
sgs.ai_choicemade_filter.skillInvoke.zhuqu = function(self, player, args)
	local to=player:getTag("zhuqu-target"):toPlayer()
	if to then
		if args[#args] == "yes" then
			sgs.updateIntention(player, to, -60)
		else
			sgs.updateIntention(player, to, 60)
		end
	end
end



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

sgs.ai_need_bear.bumie = function(self, card,from,tos)
	from =from or self.player

	if from:getHp() >1 then return false end

	if not card  then return false end
	local num = 0
	local realId = card:getEffectiveId()
	for _,c in sgs.qlist(from:getCards("hs")) do
		if num >= from:getHandcardNum() then break end
		if realId ~= -1 then
			if c:getEffectiveId() == realId then
				return from:getHandcardNum() == 1
			end
		else
			for _,id in sgs.qlist(card:getSubcards()) do
				if c:getEffectiveId() == id then
					num = num + 1
				end
			end
		end
	end
	return num >= from:getHandcardNum()
end
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


function lizhan_slash(player,objectName)
	local ids=player:getTag("lizhan"):toIntList()
	for _,id in sgs.qlist(ids) do
		local card = sgs.Sanguosha:getCard(id)
		if objectName=="notNatureSlash" then
			if not card:isKindOf("NatureSlash") then
				return id
			end
		else
			if (card:isKindOf(objectName)) then
				return id
			end
		end

	end
	return -1
end
sgs.ai_skill_invoke.lizhan = function(self)
	return true
end
sgs.ai_skill_askforag.lizhan = function(self, card_ids)
	local id= lizhan_slash(self.player,"FireSlash")
	if id ==-1 then
		id= lizhan_slash(self.player,"ThunderSlash")
	end
	if id ==-1 then
		id = lizhan_slash(self.player,"notNatureSlash")
	end
	if id ~=-1 then
		return id
	end
	return card_ids[1]
end



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


sgs.ai_skill_playerchosen.shouye = function(self, targets)
	local target =self:touhouFindPlayerToDraw(false, 1)
	if not target and #self.friends_noself>0 then
		target= self.friends_noself[1]
	end
	if  target then
		return target
	end
	return nil
end
sgs.ai_playerchosen_intention.shouye = -80
sgs.ai_no_playerchosen_intention.shouye = function(self, from)
	local lord = self.room:getLord()
	if lord  then
		sgs.updateIntention(from, lord, 10)
	end
end

--[[
sgs.ai_skill_invoke.xinyue =function(self,data)
	local target=self.player:getTag("wangyue_target"):toPlayer()
	if target and self:isEnemy(target) then
		return true
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.xinyue = function(self, player, args)
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	local from=damage.from
	if from:getHandcardNum() > player:getCards("s"):length() then
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
		if not self:isWeak(self.player) and (attacker:getHandcardNum() - player:getCards("s"):length()) >= 3  
			and not self:touhouDrawCardsInNotActive(attacker) then
			return true
		end
	end
	return false
end
]]

sgs.ai_skill_invoke.xinyue =function(self,data)
	local target=self.player:getTag("xinyue_target"):toPlayer()
	if target and self:isEnemy(target) then
		return true
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.xinyue = function(self, player, args)
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
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

sgs.ai_skill_use["@@buxian"] = function(self, prompt)
	local handcards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(handcards)
	if #handcards==0 then return "." end

	self:sort(self.enemies,"handcard")
	targets={}
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


sgs.ai_skill_invoke.gesheng = function(self, data)
	local target = data:toPlayer()
	if (self:isEnemy(target)) then
		local tmpindl = sgs.cloneCard("indulgence", sgs.Card_NoSuit, 0)
		return not self:touhouDelayTrickBadTarget(tmpindl, target, self.player)
	end
	return false
end
sgs.ai_skill_use["@@gesheng"] = function(self, prompt)
	local cards = self.player:getHandcards()
	cards=self:touhouAppendExpandPileToList(self.player,cards)
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards)
	if #cards==0 then return "." end
	local diamonds = {}
	local others ={}
	for _,c in pairs(cards) do
		if (c:getSuit() == sgs.Card_Diamond) then
			table.insert(diamonds,c)
		else
			table.insert(others,c)
		end
	end


	local current = self.room:getCurrent()
	if #diamonds >0 then
		return "@GeshengCard=".. diamonds[1]:getId() .."->" .. current:objectName()
	else
		return "@GeshengCard=".. others[1]:getId() .."->" .. current:objectName()
	end

end
sgs.ai_skill_use["@@yege"] = function(self, prompt)
	local current = self.room:getCurrent()
	if self:isEnemy(current) then
		local cards = self.player:getHandcards()
		cards=self:touhouAppendExpandPileToList(self.player,cards)
		cards = sgs.QList2Table(cards)
		self:sortByUseValue(cards)
		if #cards == 0 then return "." end	
		return "@YegeCard=".. cards[1]:getId() .."->" .. current:objectName()
	end
	return "."
end

local mingmuvs_skill = {}
mingmuvs_skill.name = "mingmu_attach"
table.insert(sgs.ai_skills, mingmuvs_skill)
function mingmuvs_skill.getTurnUseCard(self)
		local cards = self.player:getCards("hes") 
		if cards:isEmpty() then return nil end
		local source = self.room:findPlayerBySkillName("mingmu")
		if not source or source:hasFlag("mingmuInvoked") or not self:isFriend(source) then return nil end
		local give = self:getOverflow(self.player) > 0
		if not give and sgs.Slash_IsAvailable(self.player) and source:inMyAttackRange(self.player) and self.player:getAttackRange() > 1  then
			local slash = self:getCard("Slash")
			--其实要比较距离1以内/以外的敌人的防御值。 先偷懒了。
			if slash then 
				for _,p in ipairs(self.enemies) do
					if self.player:distanceTo(p) > 1 then
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


sgs.ai_skill_cardask["@yinghuo"] = function(self, data)
	local ecards=self.player:getCards("hs")
	if ecards:length()==0 then return "." end
	for _,c in sgs.qlist(ecards) do
		if c:isKindOf("Jink")then
			 return "$" .. c:getId()
		end
	end
	return "."
end



sgs.ai_cardneed.yinghuo = function(to, card, self)
	return card:getTypeId() == sgs.Card_TypeBasic
end
local yinghuo_skill = {}
yinghuo_skill.name = "yinghuo"
table.insert(sgs.ai_skills, yinghuo_skill)
function yinghuo_skill.getTurnUseCard(self)
	local slashes = {}
	local anas = {}
	for _, c in sgs.qlist(self.player:getCards("h")) do  
		if c:isKindOf("Slash") then
			table.insert(slashes, c)
		elseif c:isKindOf("Analeptic") then
			table.insert(anas, c)
		end
	end

	local card
	if #slashes>0 and #anas >0   then  --and self:shouldUseAnaleptic(self.player, slashes[1])
		
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
	if (tmp:isKindOf("Analeptic")) then
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
	if class_name == "Analeptic"  or class_name == "Peach" then
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
	elseif class_name == "Jink" or class_name == "Slash" then
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


--[[sgs.ai_slash_prohibit.yinghuo = function(self, from, to, card)
	if self:isFriend(from,to) then
		return false
	end
	if getCardsNum("Jink", to, from) >0  then
		if from:hasWeapon("Axe") and from:getCards("hes"):length()>=3 then
			return false
		end
		return true
	end
	return false
end]]

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
	self.player:removeTag("chuangshi_victim")


	if cardname and user then
		self.room:setPlayerMark(self.player, "chuangshi", self.player:getMark("chuangshi")+1);
		self.room:setPlayerMark(user, "chuangshi_user", 0);
		local chuangshicard = sgs.Sanguosha:cloneCard(cardname)
		chuangshicard:setSkillName("_chuangshi")
		local carduse=sgs.CardUseStruct()
		carduse.card=chuangshicard
		carduse.from=user
		if target then
			carduse.to:append(target)
		end
		self.room:useCard(carduse,false)
		self.room:setPlayerFlag(self.player, "chuangshi")
	end
	return "."
end
local chuangshi_filter = function(self, player, carduse)
	if carduse.card:getSkillName(true)=="chuangshi" then
		sgs.ai_chuangshi_effect = true
	end
end
sgs.ai_skill_use_func.ChuangshiCard=function(card,use,self)
	local userstring=card:toString()
	local target=self.player:getTag("chuangshi_victim"):toPlayer()
	use.card=card
	local target=self.player:getTag("chuangshi_victim"):toPlayer()
	if use.to and  target then
		use.to:appnd(target)
		return
	end
end

sgs.ai_skill_invoke.wangyue = true



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


