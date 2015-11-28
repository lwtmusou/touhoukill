--different from sgs.ai_skill_playerchosen.zero_card_as_slash
sgs.ai_skill_playerchosen.sidie = function(self, targets)
	local sidie =self.player:getTag("sidie_target"):toPlayer()
	local victim,sidieType = getSidieVictim(self, targets,sidie)
	if victim then
		return victim
	else
		if self:isFriend(sidie) then
			for _,p in pairs(self.enemies) do
				if sidie:canSlash(p,nil,false) then
					return p
				end
			end
		end
		return nil
	end
	
	
	--local dummy_damage=sgs.DamageStruct(slash, sidie, p, 1, sgs.DamageStruct_Normal)
	--local final_damage=self:touhouDamage(dummy_damage,sidie, p)
		
	return nil
end
sgs.ai_playerchosen_intention.sidie = 30
function getSidieVictim(self, targets,sidie)
	--local sidie =self.player:getTag("sidie_target"):toPlayer()
	local slash = sgs.cloneCard("slash")
	local targetlist = sgs.QList2Table(targets)
	local arrBestHp, canAvoidSlash, forbidden = {}, {}, {}
	self:sort(targetlist, "defenseSlash")

	for _, target in ipairs(targetlist) do
		if self:isEnemy(target) and not self:slashProhibit(slash,target,sidie) and sgs.isGoodTarget(target, targetlist, self) then
			if self:slashIsEffective(slash, target,sidie) then
				if self:getDamagedEffects(target, sidie, true)  then
					--or self:needLeiji(target, self.player)
					table.insert(forbidden, target)
				elseif self:needToLoseHp(target, sidie, true, true) then
					table.insert(arrBestHp, target)
				else
					return target, "good" 
				end
			else
				table.insert(canAvoidSlash, target)
			end
		end
	end
	--[[for i=#targetlist, 1, -1 do
		local target = targetlist[i]
		if not self:slashProhibit(slash, target,sidie) then
			if self:slashIsEffective(slash, target,sidie) then
				if self:isFriend(target) and (self:needToLoseHp(target, sidie, true, true)
					or self:getDamagedEffects(target, sidie, true) ) then --or self:needLeiji(target, self.player)
						--return target,"goodfriend"
				end
			else
				table.insert(canAvoidSlash, target)
			end
		end
	end]]

	if #canAvoidSlash > 0 then return canAvoidSlash[1], "canAvoid" end
	if #arrBestHp > 0 then return arrBestHp[1], "bestHp" end

	self:sort(targetlist, "defenseSlash")
	targetlist = sgs.reverse(targetlist)
	for _, target in ipairs(targetlist) do
		if target:objectName() ~= sidie:objectName()  and not self:isFriend(target) and not table.contains(forbidden, target) then
			return target, "other"
		end
	end
	return nil
end

function SmartAI:sidieEffect(from)
	if from:hasSkill("sidie") and from:getPhase() ==sgs.Player_Play and not from:hasFlag("sidie_used") then 
		return true
	end
	return false
end	
sgs.ai_cardneed.sidie = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return getCardsNum("Slash", to, self.player) <1
		and card:isKindOf("Slash")
	end
end
sgs.sidie_keep_value = {
	Slash = 7
}


-- sgs.ai_skill_playerchosen.wangxiang = function(self, targets)
	-- if not self:invokeTouhouJudge() then return nil end
	-- target_table =sgs.QList2Table(targets)
	-- if #target_table==0 then return false end
	-- for _,target in pairs(target_table) do	
		-- if  self:isFriend(target) then
			-- return target
		-- end
	-- end
	-- return nil
-- end
-- sgs.ai_playerchosen_intention.wangxiang = -80
-- sgs.ai_no_playerchosen_intention.wangxiang = function(self, from)
	-- local lord = self.room:getLord()
	-- if not lord then return end
	-- local wizard_harm = false
	-- for _, aplayer in sgs.qlist(self.room:getAlivePlayers()) do
		-- if self:hasSkills(sgs.wizard_harm_skill , aplayer) then
			-- wizard_harm = true
		-- end
	-- end
	-- if not wizard_harm then 
		-- sgs.updateIntention(from, lord, 10)
	-- end
-- end

sgs.ai_skill_invoke.wangxiang = function(self, data)
    if not self:invokeTouhouJudge() then return false end
    local to =data:toPlayer()
	return self:isFriend(to)
end
sgs.ai_choicemade_filter.skillInvoke.wangxiang = function(self, player, promptlist)
	local to=player:getTag("wangxiang-target"):toPlayer()
	if to then 
		if promptlist[#promptlist] == "yes" then
		    sgs.updateIntention(player, to, -60)
	    else
		    local wizard_harm = false
			for _, aplayer in sgs.qlist(self.room:getAlivePlayers()) do
				if self:hasSkills(sgs.wizard_harm_skill , aplayer) then
					wizard_harm = true
				end
			end
			if not wizard_harm then 
				sgs.updateIntention(player, to, 10)
			end
		end
	end
end



sgs.ai_skill_invoke.jingjie = true
sgs.ai_skill_discard.jingjie = function(self)
	local to_discard = {}
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)

	table.insert(to_discard, cards[1]:getEffectiveId())

	return to_discard
end


--sgs.ai_choicemade_filter.cardChosen.sisheng = -50
sgs.ai_skill_invoke.sisheng = function(self)
	local who= self.room:getCurrentDyingPlayer()
	
	if self:isFriend(who) then 
		if getCardsNum("Peach", who, self.player) >= 1 or getCardsNum("Analeptic", who, self.player) >= 1 then
			if self:hasWeiya(who)  then
				return true
			elseif who:getHandcardNum()>=3 then 
				return true
			end
			return false
		end
		return true 
	end
	return false 
end
sgs.ai_skill_askforag.sisheng = function(self, card_ids)
	return card_ids[1]
end
sgs.ai_choicemade_filter.skillInvoke.sisheng = function(self, player, promptlist)
	local who= player:getRoom():getCurrentDyingPlayer()
		if promptlist[#promptlist] == "yes" then
			sgs.updateIntention(player, who, -70)
		--else  --明桃不救的情况暂时不好排除 
		--	if player:getPile("jingjie"):length()>=2 then
		--		sgs.updateIntention(player, who, 60)
		--	end
		end
end
sgs.ai_skill_cardchosen.sisheng = function(self, who, flags)
	if flags == "he" then
		local who= self.player:getRoom():getCurrentDyingPlayer()
		if who:getEquip(1) and who:getEquip(1):isKindOf("SilverLion") then
			return who:getEquip(1)
		end
		local hcards = who:getCards("h")
		if hcards:length()>0 then
			return hcards:first()
		end
		local ecards = who:getCards("e")
		if ecards:length()>0 then
			return ecards:first()
		end
	end
end

sgs.ai_skill_invoke.jingdong = function(self)
	local target=self.player:getTag("jingdong_target"):toPlayer()
	if  target:hasSkill("huanmeng") or  target:hasSkill("zaozu") or target:hasSkill("yongheng")then
		return false 
	end
	num=target:getHandcardNum()-target:getMaxCards()
	if num==0 then return false end
	cards=self.player:getPile("jingjie")
	if cards:isEmpty() then return false end
	if not self:isFriend(target) then
		return false
	else
		if self:isWeak(target) and num>0 then
			return true
		end
		if num>1   then
			if self:cautionChangshi() or cards:length()>3 then
				return true
			else
				if num>3 then
					return true
				end
			end
		end
	end
	return false
end
sgs.ai_skill_askforag.jingdong = function(self, card_ids)
	return card_ids[1]
end
sgs.ai_choicemade_filter.skillInvoke.jingdong = function(self, player, promptlist)
	
	local to=self.room:getCurrent()
	if not (to:hasSkill("huanmeng") or  to:hasSkill("zaozu") or to:hasSkill("yongheng"))then
	num=to:getHandcardNum()-to:getMaxCards()
	if promptlist[#promptlist] == "yes" then
			sgs.updateIntention(player, to, -60)
	else
		if num>=3 then
			sgs.updateIntention(player, to, 30)
		end
	end
	end
end

sgs.ai_skill_cardask["@zhaoliao"] = function(self, data)
	local a=data:toDamage().to
	if not self:isFriend(a) then return "." end
	local ecards=self.player:getCards("e")
	if ecards:length()>0 then return "$" .. ecards:first():getId() end
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)
	if #cards==0 then return "." end
	self:sortByUseValue(cards)
	return "$" .. cards[1]:getId()
end
sgs.ai_skill_choice.zhaoliao=function(self)
	if self.player:isKongcheng() then return "zhaoliao1" end
	return "zhaoliao2"
end
sgs.ai_choicemade_filter.cardResponded["@zhaoliao"] = function(self, player, promptlist)
	if promptlist[#promptlist] ~= "_nil_" then
		local target =player:getTag("zhaoliao_target"):toPlayer()
		if not target then return end	
		sgs.updateIntention(player, target, -80)
	end
end
--[[sgs.ai_choicemade_filter.skillChoice.zhaoliao = function(self, player, promptlist)
	local choice = promptlist[#promptlist]
	local target =player:getTag("zhaoliao_target"):toPlayer()
	if not target then return end	
	if choice== "zhaoliao1" then  
		sgs.updateIntention(player, target, -40)
	else
		sgs.updateIntention(player, target, -80)
	end
end]]



sgs.ai_skill_invoke.jiaoxia = function(self)
	return self:invokeTouhouJudge()
end
sgs.ai_need_bear.jiaoxia = function(self, card,from,tos) 
	from =from or self.player
	if not card:isKindOf("EquipCard") then return false end
	if (from:getHp()==1 and from:getCards("e"):length()==0) then
		return true
	end
	if not self:isWeak(from) then
		if card:isKindOf("Armor") and not from:getArmor() then
			return false
		elseif  card:isKindOf("DefensiveHorse") and not from:getDefensiveHorse() then
			return false
		end	
	end
	if self:getOverflow(from) <= 0 then
		return true
	end
	return false
end

sgs.ai_skill_invoke.youming = function(self, data)
	local handcard = self.player:getHandcardNum()
	local max_card = self.player:getMaxCards()
	local target = 0
	local slashnum = 0

	for _, slash in ipairs(self:getCards("Slash")) do
		for _,enemy in ipairs(self.enemies) do
			if self.player:canSlash(enemy, slash) and self:slashIsEffective(slash, enemy) and self:slashIsEffective(slash, enemy)
			  and not self:slashProhibit(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self) then
				slashnum = slashnum + 1
				target = target + 1
				break
			end
		end
	end

	local prompt = data:toString()
	if prompt == "draw2play" then
		if self:needBear() then return false end
		if slashnum > 1 and target > 0 then return true end
		if self.player:isSkipped(sgs.Player_Play) and #(self:getTurnUse()) > 0 then return true end
		return false
	elseif prompt == "play2draw" then
		if self:needBear() then return true end
		if slashnum > 0 and target > 0 then return false end
		if #(self:getTurnUse()) == 0 then return true end
		return false
	end
end

function sgs.ai_cardneed.youming(to, card, self)
	return isCard("Slash", card, to) and getKnownCard(to, self.player, "Slash", true) < 2
end

--[[sgs.ai_cardneed.jianshu = function(to, card, self)
	if not to:getOffensiveHorse() and getCardsNum("OffensiveHorse", to, self.player) < 1 then
		return  card:isKindOf("OffensiveHorse")
	end
end


sgs.ai_skill_invoke.bailou =function(self,data)
	local target=data:toPlayer()
	if self:isEnemy(target) then
		return true
	end
end
sgs.ai_cardneed.bailou = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return card:isKindOf("Slash") and card:isRed()
	end
end
]]

sgs.ai_skill_use["@@xiezou"] = function(self, prompt)
	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	local cardname=self.player:property("xiezou_card"):toString()
	local card=sgs.cloneCard(cardname, sgs.Card_NoSuit, 0)
	card:setSkillName("xiezou")
	local target
	if card:isKindOf("TrickCard") then
		self:useTrickCard(card, dummy_use)
	else
		self:useBasicCard(card, dummy_use)
	end
	
	if not dummy_use.card then return false end
	if dummy_use.to:isEmpty() then
		if card:isKindOf("IronChain") then
			return "."
		end
		return dummy_use.card:toString()
	else
		local target_objectname = {}
		if card:isKindOf("IronChain") then
			for _, p in sgs.qlist(dummy_use.to) do
				if (self:isEnemy(p) and not p:isChained()) 
				or (self:isFriend(p) and p:isChained())then
					table.insert(target_objectname, p:objectName())
				end
				if #target_objectname==2 then break end
			end
		else
			for _, p in sgs.qlist(dummy_use.to) do
				if self:isEnemy(p) then
					table.insert(target_objectname, p:objectName())
					target=p
					break
				end
			end
		end
		
		if card:isKindOf("Collateral") then
			local victim
			for _,p in sgs.qlist(self.room:getOtherPlayers(target))do
				if self:isEnemy(p) and target:canSlash(p,nil,true) then
					table.insert(target_objectname, p:objectName())
					victim=p
					break
				end
			end
			if not victim then
				return "."
			end			
		end
		if #target_objectname>0 then
			return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
		end
	end
	return "."
end

sgs.ai_slash_prohibit.hesheng = function(self, from, to, card)
	local prevent=false
	if not to:hasSkill("hesheng") then return false end
	if self:isFriend(from,to) then return false end
	for _,p in sgs.qlist(self.room:getAlivePlayers())do
		if (p:getCards("j"):length()>0 ) then
			prevent=true
			break
		end
	end
	if prevent then
		local damage=sgs.DamageStruct(card, from, to,self:touhouDamageNature(card,from,to))
		local slash_effect=self:touhouDamageEffect(damage,from,to)
		if slash_effect then return false end
	end
	return prevent
end
sgs.ai_damageInflicted.hesheng =function(self, damage)
	local can =false
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:getCards("j"):length()>0 then
			can=true
			break
		end
	end
	if can then
		damage.damage=0
	end
	return damage
end



sgs.ai_skill_invoke.renou = true
--具体如何选人偶牌 尚没有策略。。。

sgs.ai_view_as.zhanzhen = function(card, player, card_place)
	if (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_UNKNOWN) then
		return false
	end
	local pattern=sgs.Sanguosha:getCurrentCardUsePattern()
	if not card:isKindOf("EquipCard") then return false end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place ~= sgs.Player_PlaceSpecial then
		if pattern=="jink" then
			return ("jink:zhanzhen[%s:%s]=%d"):format(suit, number, card_id)
		elseif pattern=="slash" then
			return ("slash:zhanzhen[%s:%s]=%d"):format(suit, number, card_id)
		end
	end
end

local zhanzhen_skill = {}
zhanzhen_skill.name = "zhanzhen"
table.insert(sgs.ai_skills, zhanzhen_skill)
zhanzhen_skill.getTurnUseCard = function(self, inclusive)
        if not sgs.Slash_IsAvailable(self.player)  then return false end
		local ecards={}
		local cards=self.player:getCards("he")
		cards=self:touhouAppendExpandPileToList(self.player,cards)
		for _,c in sgs.qlist(cards) do
			if c:isKindOf("EquipCard") then
				table.insert(ecards,c)
			end
		end
        if #ecards==0 then return false end
		self:sortByUseValue(ecards, true)
        local suit = ecards[1]:getSuitString()
        local number = ecards[1]:getNumberString()
        local card_id = ecards[1]:getEffectiveId()
        local slash_str = ("slash:zhanzhen[%s:%s]=%d"):format(suit, number, card_id)
        local slash = sgs.Card_Parse(slash_str)
                
        assert(slash)
        return slash	
end
sgs.ai_cardneed.zhanzhen = function(to, card, self)
	return card:isKindOf("EquipCard")
end

sgs.ai_skill_playerchosen.zhanzhen = function(self, targets)
	local c = self.player:getTag("zhanzhen"):toCard()
	local cards = {}
	table.insert(cards, c)
	
	local available_friends = {}
	if #self.friends_noself==0 then return nil end
	for _, friend in ipairs(self.friends_noself) do
		--需要check 永恒 高傲这种
		table.insert(available_friends, friend) 
	end
	
	
	local card, friend = self:getCardNeedPlayer(cards)
	if card and friend and table.contains(available_friends, friend) then return friend  end

	if #available_friends > 0 then
		self:sort(available_friends, "handcard")
		for _, afriend in ipairs(available_friends) do
			if not self:needKongcheng(afriend, true) then
				return afriend
			end
		end
		self:sort(available_friends, "defense")
		return available_friends[1]
	end
	return nil
end
sgs.ai_playerchosen_intention.zhanzhen = -60
sgs.ai_no_playerchosen_intention.zhanzhen = function(self, from)
	local lord = self.room:getLord()
	if lord then
		sgs.updateIntention(from, lord, 10)
	end
end


sgs.ai_skill_choice.shishen=function(self)
	if self.player:getPhase() == sgs.Player_Start then
		return "shishen1"
	end
	if self.player:hasFlag("shishen_choice") then
		local ran = self.room:findPlayerBySkillName("zhaoliao")
		if ran and ran:getCards("he")>=2 and self:isFriend(ran) then
			return "cancel"
		else
			return "shishen1"
		end
	end
	if self.player:getPhase() == sgs.Player_Play  and self.player:getMark("@shi")==0    then
		for _,card in sgs.qlist(self.player:getCards("h")) do
			if card:isNDTrick() and not card:isKindOf("Nullification") then
				local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
				self:useTrickCard(card, dummy_use)
				if  dummy_use.card then
					if card:isKindOf("AOE") or card:isKindOf("GlobalEffect") then
						return "shishen2"
					elseif not dummy_use.to:isEmpty() then
						return "shishen2"
					end
				end
			end
		end
	end
	return "cancel"
end
sgs.ai_slash_prohibit.yexing = function(self, from, to, card)
	if to:hasSkill("yexing") and to:getMark("@shi") == 0 then
		if not card:isKindOf("NatureSlash") or  from:hasSkill("here") then 
			return true
		end
	end
	return false
end


sgs.ai_skill_use["@@yaoshu"] = function(self, prompt)
	
	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	local cardname=self.player:property("yaoshu_card"):toString()
	local card=sgs.cloneCard(cardname, sgs.Card_NoSuit, 0)
	card:setSkillName("yaoshu")
	local target
	
	self:useTrickCard(card, dummy_use)
	if not dummy_use.card then return false end
	
	if dummy_use.to:isEmpty() then
		if card:isKindOf("IronChain") then
			return "."
		end
		return dummy_use.card:toString()
	else
		local target_objectname = {}
		if card:isKindOf("IronChain") then
			for _, p in sgs.qlist(dummy_use.to) do
				if (self:isEnemy(p) and not p:isChained()) 
				or (self:isFriend(p) and p:isChained())then
					table.insert(target_objectname, p:objectName())
				end
				if #target_objectname==2 then break end
			end
		else
			for _, p in sgs.qlist(dummy_use.to) do
				if self:isEnemy(p) then
					table.insert(target_objectname, p:objectName())
					target=p
					break
				end
			end
		end
		
		if card:isKindOf("Collateral") then
			local victim
			for _,p in sgs.qlist(self.room:getOtherPlayers(target))do
				if self:isEnemy(p) and target:canSlash(p,nil,true) then
					table.insert(target_objectname, p:objectName())
					victim=p
					break
				end
			end
			if not victim then
				return "."
			end			
		end
		if #target_objectname>0 then
			return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
		end
	end
	return "."
end
sgs.ai_cardneed.yaoshu = function(to, card, self)
	return card:isNDTrick()
end

sgs.ai_skill_invoke.jiyi = true
sgs.ai_skill_askforyiji.jiyi = function(self, card_ids)
	local available_friends = {}
	if #self.friends_noself==0 then return nil, -1 end
	for _, friend in ipairs(self.friends_noself) do
		--if not friend:hasSkill("manjuan") and not self:isLihunTarget(friend) then 
		table.insert(available_friends, friend) 
		--end
	end
    if self.player:getHandcardNum()<=2 then  return nil, -1 end
	local toGive, allcards = {}, {}
	local keep
	for _, id in ipairs(card_ids) do
		local card = sgs.Sanguosha:getCard(id)
		if not keep and (isCard("Jink", card, self.player) or isCard("Analeptic", card, self.player)) then
			keep = true
		else
			table.insert(toGive, card)
		end
		table.insert(allcards, card)
	end

	local cards = #toGive > 0 and toGive or allcards
	self:sortByKeepValue(cards, true)
	local id = cards[1]:getId()

	local card, friend = self:getCardNeedPlayer(cards)
	if card and friend and table.contains(available_friends, friend) then return friend, card:getId() end

	if #available_friends > 0 then
		self:sort(available_friends, "handcard")
		for _, afriend in ipairs(available_friends) do
			if not self:needKongcheng(afriend, true) then
				return afriend, id
			end
		end
		self:sort(available_friends, "defense")
		return available_friends[1], id
	end
	return nil, -1
end
sgs.ai_Rende_intention.jiyi= -30

sgs.ai_skill_playerchosen.baochun = function(self, targets)
	local target =self:touhouFindPlayerToDraw(true, self.player:getLostHp())
	if target then return target end
	if #self.friends>0 then return self.friends[1] end
	return nil
end
sgs.ai_playerchosen_intention.baochun = -80

sgs.ai_need_damaged.baochun = function(self, attacker, player)
	local x= player:getLostHp()+1 
	if x>=3 and player:getHp()>1 then
		return true
	end
	return false
end


sgs.ai_skill_invoke.zhancao = function(self,data)
	local use=self.player:getTag("zhancao_carduse"):toCardUse()
	local target =self.player:getTag("zhancao_target"):toPlayer()
	if not self:isFriend(target) then return false end
	if self:touhouCardUseEffectNullify(use,target) then return false end
	--if self:isFriend(use.from) and use.from:hasSkills("shenyin|lizhi") then return false end
	if self:isFriend(use.from) then
		local fakeDamage=sgs.DamageStruct()
		fakeDamage.card=use.card
        fakeDamage.nature= self:touhouDamageNature(use.card, use.from, target)
        fakeDamage.damage=1
        fakeDamage.from=use.from
        fakeDamage.to= target
		if self:touhouDamageEffect(fakeDamage,use.from,target) then  return false  end
	end
	
	local hasEquip=false
	cards =self.player:getCards("he") 
	for _,card in sgs.qlist(cards) do
		if card:isKindOf("EquipCard") then
			hasEquip=true
			break
		end
	end
	if self:isWeak(target)
		and ((not self:isWeak(self.player)) or hasEquip) then
		return true
	end
	if getCardsNum("Jink", target, self.player) < 1 or sgs.card_lack[target:objectName()]["Jink"] >0 then
		if target:objectName()==self.player:objectName() then
			return true
		else
			if  (hasEquip or self.player:getHp()>1) then
				if self:hasHeavySlashDamage(use.from, use.card, target) then
					return true
				end
				if target:isChained() and (use.card:isKindOf("NatureSlash") ) then
					for _,p in pairs (self.friends) do
						if self:isWeak(p) and p:isChained() then
							return true
						end
					end
				end
			end
		end
	end
	return false
end
sgs.ai_skill_cardask["@zhancao-discard"] = function(self, data)
	
	cards =self.player:getCards("he") 
	cards=sgs.QList2Table(cards)
	ecards={}
	for _,card in pairs(cards) do
		if card:isKindOf("EquipCard") then
			table.insert(ecards,card)
		end
	end
	
	if #ecards==0 then return "." end
	self:sortByCardNeed(ecards)
	return "$" .. ecards[1]:getId()
end	
sgs.ai_choicemade_filter.skillInvoke.zhancao = function(self, player, promptlist)
	local target =player:getTag("zhancao_target"):toPlayer()
	if target then
		if promptlist[#promptlist] == "yes" then
			sgs.updateIntention(player, target, -50)
		end	
	end
end
sgs.ai_cardneed.zhancao = function(to, card, self)
	return card:isKindOf("EquipCard")
end
sgs.zhaocao_keep_value = { 
	EquipCard = 7
}
sgs.ai_need_bear.zhaocao = function(self, card,from,tos) 
	from =from or self.player
	if not card:isKindOf("EquipCard") then return false end
	if self:getSameEquip(card,from) then
		if card:isKindOf("Weapon") then
			if self:getOverflow(from) >= 0 then
				local old_range=sgs.weapon_range[from:getWeapon():getClassName()] or 0
				local new_range = sgs.weapon_range[card:getClassName()] or 0
				if new_range<=old_range then
					return true
				end
			end
		else
			return true
		end
	end
	return false
end


local mocao_skill = {}
mocao_skill.name = "mocao"
table.insert(sgs.ai_skills, mocao_skill)
mocao_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("MocaoCard") then return nil end
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if  p:getCards("e"):length()>0 then
			t=true
			break
		end
	end
	if t then
		return sgs.Card_Parse("@MocaoCard=.")
	end
	return nil
end
sgs.ai_skill_use_func.MocaoCard = function(card, use, self)
	local targets={}
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if  p:getCards("e"):length()>0 then
			table.insert(targets,p)
		end
	end
	--需要一个全新的sort
	self:sort(targets,"value")
	for _, p in ipairs(targets) do
        if (p:getLostHp()<2 and self:isEnemy(p)) or (p:getLostHp()>1 and self:isFriend(p))  then
			use.card = card
			if use.to then
				use.to:append(p)
				if use.to:length() >= 1 then return end
			end
		end
    end
end
sgs.ai_use_value.MocaoCard = 9
sgs.ai_use_priority.MocaoCard = 6
sgs.ai_card_intention.MocaoCard = function(self, card, from, tos)
	if tos[1]:getLostHp()>=2  then
		sgs.updateIntention(from, tos[1], -30)
	else
		sgs.updateIntention(from, tos[1], 30)
	end
end

--function SmartAI:getDamagedEffects(to, from, slash)
sgs.ai_skill_invoke.shenyin = function(self,data)
	local target=data:toPlayer()
	local damage  = self.player:getTag("shenyin_damage"):toDamage()
	local isSlash = true
	if not damage.card or not damage.card:isKindOf("Slash") then
		isSlash = false
	end
	if  self:isEnemy(target) then
		local canDamage = self:touhouNeedAvoidAttack(damage,self.player,target,true)
		if not canDamage then return true end
		if self:needToLoseHp(target, self.player, isSlash, true) 
		or self:getDamagedEffects(target, self.player, isSlash) then
			return true
		end
		if self:isWeak(target) then
			return false
		end
	end
	if  self:isFriend(target) then
		if self:needToLoseHp(target, self.player, isSlash, true) 
		or self:getDamagedEffects(target, self.player, isSlash) then
			return false
		end
	end
	return true
end

function checkXijianPile(target)
	local skillNeed =false
	for _,pile in pairs(target:getPileNames()) do
		if  pile == "shende" and target:hasSkill("shende") then
			skillNeed = true
		elseif pile == "jingjie" and 
		(target:hasSkills("sisheng|jingdong|luanying") 
		or (target:hasSkill("mengxian") and target:getMark("mengxian") == 0)) then
			skillNeed =true
		elseif pile == "piao" and target:hasSkill("shanji") then
			skillNeed =true
		end
	end
	return skillNeed
end
sgs.ai_skill_playerchosen.xijian = function(self, targets)
	local target_willobtain = self.player:getTag("xijian_target"):toPlayer()
	if not self:isFriend(target_willobtain) then return  nil end
	if self:isFriend(target_willobtain) then
		local target_table =sgs.QList2Table(targets)
		self:sort(target_table,"value")
		for _,p in pairs (target_table) do
			if self:isFriend(p) then
				if not checkXijianPile(p) then
					return p
				end
			else
				return p
			end
		end
	end
	return nil
end
sgs.ai_skill_askforag.xijian = function(self, card_ids)
	local cards={}
	local target = self.player:getTag("xijian_target"):toPlayer()
	for _,card_id in pairs(card_ids) do
		local card=sgs.Sanguosha:getCard(card_id)
		table.insert(cards,card)
	end
	self:sortByCardNeed(cards, true)
	if self:isFriend(target)  then
		return cards[1]:getId()
	end
end
sgs.ai_no_playerchosen_intention.xijian =function(self, from)
	local target_willobtain = from:getTag("xijian_target"):toPlayer()
	if target_willobtain  and sgs.ai_role[target_willobtain:objectName()]  ~= "netural" then
		for _,p in sgs.qlist(self.room:getAlivePlayers()) do
			if checkXijianPile(p) and   sgs.ai_role[p:objectName()]  ~= "netural" and not self:isFriend(p, target_willobtain) then
				sgs.updateIntention(from, p, -10)
			end
		end
	end
end
sgs.ai_playerchosen_intention.xijian =function(self, from, to)
	local target = from:getTag("xijian_target"):toPlayer()
	if target then
		sgs.updateIntention(from, target, -50)
	end
end



sgs.ai_skill_choice.youqu=function(self)
	local yukari=self.player:getRoom():findPlayerBySkillName("xijian")	
	if yukari then
		if self:isFriend(yukari) then
			return "siling3"
		else
			return "siling1"
		end
	end
	local peach_num = self:getCardsNum("Peach")
	if not self.player:isWounded() and peach_num>0  then
		return "siling3"
	end
	if self.player:isLord() and sgs.current_mode_players["rebel"]==0 then
		return "siling1"
	end
	local enemy_num =0
	if self.player:isLord() or self.player:getRole() == "loyalist" then
		enemy_num = sgs.current_mode_players["rebel"]
	elseif self.player:getRole() == "rebel" then
		enemy_num = sgs.current_mode_players["loyalist"]
	else
		enemy_num = #self.enemies
	end
	local cards=self.player:getPile("siling")
	local good_count = 1 
	for i=1, 3, 1 do
		if cards:length()+i >= enemy_num then
			good_count = i
			break
		end
	end
	if good_count == 1 then
		return "siling1"
	elseif good_count == 2 then
		return "siling2"
	elseif good_count == 3 then
		return "siling3"
	end
	return "siling1"
end
sgs.ai_choicemade_filter.skillChoice.youqu = function(self, player, promptlist)
	sgs.siling_lack[player:objectName()]["Red"] = 0
	sgs.siling_lack[player:objectName()]["Black"] = 0
end

sgs.ai_skill_invoke.wangwu = function(self,data)
	local target=self.player:getTag("wangwu_use"):toCardUse().from
	if not target then return false end
	if self:isEnemy(target) then
		return true
	end
	return false
end
sgs.ai_skill_askforag.wangwu = function(self, card_ids)
	local wangwu_card=self.player:getTag("wangwu_card"):toCard()
	local cards={}
	for _,card_id in pairs(card_ids) do
		local card=sgs.Sanguosha:getCard(card_id)
		table.insert(cards,card)
	end
	self:sortByCardNeed(cards)
	
	for _,card in pairs(cards) do
        if card:sameColorWith(wangwu_card) then
            return cards[1]:getId()
        end
    end
end
sgs.ai_choicemade_filter.skillInvoke.wangwu = function(self, player, promptlist)
	local use = player:getTag("wangwu_use"):toCardUse()
	
	if use.card and (use.card:isRed() or  use.card:isBlack()) then
		local str 
		if use.card:isRed() then
			str = "Red"
		end
		if use.card:isBlack() then
			str = "Black"
		end
		if use.from and self:isEnemy(use.from,player) then
			if promptlist[#promptlist] == "yes" then
				local findSame =false
				for _,id in sgs.qlist(player:getPile("siling")) do
					if sgs.Sanguosha:getCard(id):sameColorWith(use.card) then
						findSame =true
						break
					end
				end
				if not findSame then
					sgs.siling_lack[player:objectName()][str] = 1
				end
			else
				sgs.siling_lack[player:objectName()][str] = 1
			end	
		end
	end
end
sgs.ai_slash_prohibit.wangwu = function(self, from, to, card)
	if self:isFriend(from,to) then
		return false
	end
	if to:getPile("siling"):length()==0 then return false end
	if card:isRed() then
		if sgs.siling_lack[to:objectName()]["Red"] > 0 then
			return false
		end
	elseif card:isBlack() then
		if sgs.siling_lack[to:objectName()]["Black"] > 0 then
			return false
		end
	else
		return false
	end
	local fakeDamage=sgs.DamageStruct()
	fakeDamage.nature= sgs.DamageStruct_Normal
	fakeDamage.damage=1
	fakeDamage.from=to
	fakeDamage.to=from
	local damageEffect = self:touhouNeedAvoidAttack(fakeDamage,to,from) 
	return damageEffect
end
sgs.ai_trick_prohibit.wangwu = function(self, from, to, card)
	if self:isFriend(from,to) then
		return false
	end
	if to:getPile("siling"):length()==0 then return false end
	if card:isRed() then
		if sgs.siling_lack[to:objectName()]["Red"] > 0 then
			return false
		end
	elseif card:isBlack() then
		if sgs.siling_lack[to:objectName()]["Black"] > 0 then
			return false
		end
	else
		return false
	end
	
	local fakeDamage=sgs.DamageStruct()
	fakeDamage.nature= sgs.DamageStruct_Normal
	fakeDamage.damage=1
	fakeDamage.from=to
	fakeDamage.to=from
	local damageEffect = self:touhouNeedAvoidAttack(fakeDamage,to,from) 
	return damageEffect
end

sgs.ai_slash_prohibit.hpymsiyu = function(self, from, to, card)
	if self:isFriend(from,to) then
		return false
	end
	local callback=sgs.ai_damage_prohibit["hpymsiyu"]
	return callback(self, from, to, card)
end
sgs.ai_trick_prohibit.hpymsiyu = function(self, from, to, card)
	if self:isFriend(from,to) then return false end
	local isDamage=false
	if ( card:isKindOf("Duel") or card:isKindOf("AOE") or card:isKindOf("FireAttack") 
			or sgs.dynamic_value.damage_card[card:getClassName()]) then
		isDamage=true
	end
	if isDamage then
		local callback=sgs.ai_damage_prohibit["hpymsiyu"]
		return callback(self, from, to, card)
	end
	return false
end
sgs.ai_damage_prohibit.hpymsiyu = function(self, from, to, card)
	if not to:hasSkills("hpymsiyu+juhe") then return false end
	if to:getPhase() ~=sgs.Player_NotActive then return false end
	if self:isFriend(from,to) then return false end
	--强制不去集火
	for _,p in sgs.qlist(self.room:getOtherPlayers(to)) do
		if not self:isFriend(from,p) then
			return true
		end
	end
	--后面的暂且不管了。。。。
	local fakeDamage=sgs.DamageStruct()
	fakeDamage.card=card
	fakeDamage.nature= self:touhouDamageNature(card,from,to)
	fakeDamage.damage=1
	fakeDamage.from=from
	fakeDamage.to=to
	if self:touhouDamage(fakeDamage,from,to).damage < to:getHp() then 
		return false
	end
	if to:containsTrick("indulgence") or to:containsTrick("supply_shortage") then
		return true
	end
	if not to:faceUp() then
		return true
	end
	local recoverNum = self:getAllPeachNum(to) + getCardsNum("Analeptic", to, self.player)
	return recoverNum>0 
end


sgs.ai_skill_invoke.juhe = true
--sgs.ai_skill_discard.juhe = sgs.ai_skill_discard.gamerule
sgs.ai_skill_discard.juhe = function(self,discard_num)
	local Weapon=self.player:getWeapon()  
	local Distance=self.player:getOffensiveHorse()
	if  Distance and Weapon then
		local gamerule = sgs.QList2Table(self.player:getCards("h"))
		self:sortByKeepValue(gamerule,false)
		local gamerule_discard={}
		for var=1, discard_num ,1 do
			table.insert(gamerule_discard,gamerule[var]:getId())
		end
		return gamerule_discard
	end
	local cards = {}
	local tmp_dis
	local weapons={}
	for _,c in sgs.qlist(self.player:getCards("h")) do
		if not Distance and c:isKindOf("OffensiveHorse") and not tmp_dis then
			tmp_dis=c
		elseif not Weapon and c:isKindOf("Weapon") then
				table.insert(weapons,c)
		else
			table.insert(cards,c)
		end
	end
	if #weapons>1 then
		self:sortByUseValue(weapons,false)
		for var=2, #weapons ,1 do
			table.insert(cards,weapons[var])
		end
	end
	
	self:sortByKeepValue(cards)
	local to_discard = {}
	for var=1, discard_num ,1 do
		table.insert(to_discard,cards[var]:getId())
	end
	return to_discard
end
function SmartAI:touhouBreakDamage(damage,to)
	if to:hasSkill("hpymsiyu") and to:getPhase()==sgs.Player_NotActive then
		if to:getHp()>0   then
			return damage.damage>= to:getHp()
		else
			return damage.damage>0
		end
	end
	return false
end
