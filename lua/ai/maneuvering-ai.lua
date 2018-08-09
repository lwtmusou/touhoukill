function SmartAI:useCardIceSlash(...)
	self:useCardSlash(...)
end

sgs.ai_card_intention.IceSlash = sgs.ai_card_intention.Slash

function SmartAI:useCardThunderSlash(...)
	self:useCardSlash(...)
end

sgs.ai_card_intention.ThunderSlash = sgs.ai_card_intention.Slash

sgs.ai_use_value.ThunderSlash = 4.55
sgs.ai_keep_value.ThunderSlash = 3.66
sgs.ai_use_priority.ThunderSlash = 2.5
sgs.dynamic_value.damage_card.ThunderSlash = true

function SmartAI:useCardFireSlash(...)
	self:useCardSlash(...)
end

sgs.ai_card_intention.FireSlash = sgs.ai_card_intention.Slash

sgs.ai_use_value.FireSlash = 4.6
sgs.ai_keep_value.FireSlash = 3.63
sgs.ai_use_priority.FireSlash = 2.5
sgs.dynamic_value.damage_card.FireSlash = true

sgs.weapon_range.Fan = 4
sgs.ai_use_priority.Fan = 2.655
sgs.ai_use_priority.Vine = 0.95

sgs.ai_skill_invoke.Fan = function(self, data)
	local use = data:toCardUse()
	if use.card:isKindOf("FireSlash") then
		return true
	end
	for _, target in sgs.qlist(use.to) do
		if self:isFriend(target) then
			if not self:damageIsEffective(target, sgs.DamageStruct_Fire) then return true end
			if target:isChained() and self:isGoodChainTarget(target, nil, nil, nil, use.card) then return true end
			--local card = self:copyHereSlash(use.card)
			if self:slashIsEffective(use.card, target, use.from) then
				return true
			end
		else
			if not self:damageIsEffective(target, sgs.DamageStruct_Fire) then return false end
			if target:isChained() and not self:isGoodChainTarget(target, nil, nil, nil, use.card) then return false end
			if target:hasArmorEffect("IronArmor") then return false end
			if target:isChained() and self:isGoodChainTarget(target, nil, nil, nil, use.card) then return true end
			if target:hasArmorEffect("Vine") then
				return true
			end
			
		end
	end
	return false
end
sgs.ai_view_as.Fan = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE
		and card_place ~= sgs.Player_PlaceSpecial and card:objectName() == "slash" then
		return ("fire_slash:fan[%s:%s]=%d"):format(suit, number, card_id)
	end
end

local fan_skill={}
fan_skill.name="Fan"
table.insert(sgs.ai_skills,fan_skill)
fan_skill.getTurnUseCard=function(self)
	local cards = self.player:getCards("hs")
	cards=self:touhouAppendExpandPileToList(self.player,cards)
	cards=sgs.QList2Table(cards)
	local slash_card
	--冰杀出问题
	for _,card in ipairs(cards)  do
		if card:isKindOf("Slash") and not card:isKindOf("NatureSlash") then
		--if card:isKindOf("Slash") and not (card:isKindOf("FireSlash") or card:isKindOf("ThunderSlash")) then
			slash_card = card
			break
		end
	end

	if not slash_card  then return nil end
	local suit = slash_card:getSuitString()
	local number = slash_card:getNumberString()
	local card_id = slash_card:getEffectiveId()
	local card_str = ("fire_slash:Fan[%s:%s]=%d"):format(suit, number, card_id)
	local fireslash = sgs.Card_Parse(card_str)
	assert(fireslash)

	return fireslash

end

function sgs.ai_weapon_value.Fan(self, enemy)
	if enemy and enemy:hasArmorEffect("Vine") then return 6 end
end

function sgs.ai_armor_value.Vine(player, self)
	if self:needKongcheng(player) and player:getHandcardNum() == 1 then
		return player:hasSkill("kongcheng") and 5 or 3.8
	end
	if self:hasSkills(sgs.lose_equip_skill, player) then return 3.8 end
	if not self:damageIsEffective(player, sgs.DamageStruct_Fire) then return 6 end
	if self.player:hasSkill("sizhan") then return 4.9 end
	if player:hasSkill("jujian") and not player:getArmor() and #(self:getFriendsNoself(player)) > 0 and player:getPhase() == sgs.Player_Play then return 3 end
	if player:hasSkill("diyyicong") and not player:getArmor() and player:getPhase() == sgs.Player_Play then return 3 end

	local fslash = sgs.cloneCard("fire_slash")
	local tslash = sgs.cloneCard("thunder_slash")
	if player:isChained() and (not self:isGoodChainTarget(player, self.player, nil, nil, fslash) or not self:isGoodChainTarget(player, self.player, nil, nil, tslash)) then return -2 end

	for _, enemy in ipairs(self:getEnemies(player)) do
		if (enemy:canSlash(player) and enemy:hasWeapon("Fan")) or self:hasSkills("huoji|zonghuo", enemy) then return -2 end
		if getKnownCard(enemy, player, "FireSlash", true) >= 1 or getKnownCard(enemy, player, "FireAttack", true) >= 1 or
			getKnownCard(enemy, player, "Fan") >= 1 then return -2 end
	end

	if (#self.enemies < 3 and sgs.turncount > 2) or player:getHp() <= 2 then return 5 end
	return 0
end

function SmartAI:shouldUseAnaleptic(target, slash)
	if sgs.turncount <= 1 and self.role == "renegade" and sgs.isLordHealthy() and self:getOverflow() < 2 then return false end
	if target:hasSkill("xuying") then return false end
	--默认杀的effective 已经check过了？

	--造伤估值  关联技能 例如冰魄
	--还缺判断是否需要主动使用damageEffect 如神隐
	local fakeDamage = sgs.DamageStruct(slash, self.player, target, 2, self:touhouDamageNature(slash,self.player,target))
	if not self:touhouNeedAvoidAttack(fakeDamage,self.player,target) or fakeDamage.damage<2 then
		return false
	end

	if target:hasArmorEffect("SilverLion") and not self.player:hasWeapon("QinggangSword") then
		return
	end

	if target:hasSkill("guangji") and not target:getPile("tianyi"):isEmpty() then
		return false
	end
	--【战操对策】
	local shrx=self.room:findPlayerBySkillName("zhancao")
	if shrx and self:isEnemy(shrx) then
		if self:isFriend(shrx,target) and (shrx:inMyAttackRange(target) or shrx:objectName() == target:objectName()) 
		and shrx:canDiscard(shrx, "e") then
			return false
		end
	end
	--【噬史对策】
	local huiyin = self.room:findPlayerBySkillName("shishi")
	if huiyin  and huiyin:getPile("lishi"):isEmpty() and self:isEnemy(huiyin) then
		return false
	end
	--【乱影对策】
	local merry = self.room:findPlayerBySkillName("luanying")
	if merry and self:isFriend(merry, target) and self:canLuanying(merry, slash) then
		return false
	end
	if fakeDamage.damage >= target:getHp() and target:hasArmorEffect("BreastPlate") and target:getArmor() and target:getArmor():objectName() == "BreastPlate"  then
		return false
	end
	if target:hasSkill("zheshe") and target:canDiscard(target, "hs") then return false end
	if target:hasSkill("xiangle") then
		local basicnum = 0
		for _, acard in sgs.qlist(self.player:getHandcards()) do
			if acard:getTypeId() == sgs.Card_TypeBasic and not acard:isKindOf("Peach") then basicnum = basicnum + 1 end
		end
		if basicnum < 3 then return false end
	end
	if target:hasSkill("junwei") and target:getKingdom() ~=self.player:getKingdom() then
		local blacknum = self:getSuitNum("black", true)
		if blacknum < 3 then return false end
	end

	if self.player:hasWeapon("Blade") and self:invokeTouhouJudge() then return true end
	if self.player:hasFlag("zuiyue") then return true end
	--勇仪主动吃酒
	if  self:canGuaili(slash) then
		return true
	end
	if self.player:hasSkill("jiuhao") and not self.player:hasFlag("jiuhao") and #self.enemies >0 then
		local jiuhaoTarget = sgs.ai_skill_playerchosen.zero_card_as_slash(self, self.room:getOtherPlayers(self.player))
		if jiuhaoTarget and self:isEnemy(self.player, jiuhaoTarget) then
			return true
		end
	end

	if self.player:hasWeapon("Axe") and self.player:getCards("hes"):length() > 4 then return true end

	if ((self.player:hasSkill("roulin") and target:isFemale()) or (self.player:isFemale() and target:hasSkill("roulin"))) or self.player:hasSkill("wushuang") then
		if getKnownCard(target, player, "Jink", true, "hes") >= 2 then return false end
		return getCardsNum("Jink", target, self.player) < 2
	end

	if getKnownCard(target, self.player, "Jink", true, "hes") >= 1 and not (self:getOverflow() > 0 and self:getCardsNum("Analeptic", nil, nil, "MagicAnaleptic") > 1) then return false end
	return self:getCardsNum("Analeptic", nil, nil, "MagicAnaleptic") > 1 or getCardsNum("Jink", target) < 1 or sgs.card_lack[target:objectName()]["Jink"] == 1
end

function SmartAI:useCardAnaleptic(card, use)
	if self:cautionDoujiu(self.player,card) then
		return
	end
	if not self.player:hasEquip(card) and not self:hasLoseHandcardEffective() and not self:isWeak()
		and sgs.Analeptic_IsAvailable(self.player, card) then
		use.card = card
	end
end

function SmartAI:searchForAnaleptic(use, enemy, slash)

	if not self.toUse then return nil end
	if not use.to then return nil end

	--使用酒不过getTurnUseCard。。。 而是过getCardId getCardId无法处理0牌转化。 只好暂时耦合
	if use.from:hasFlag("zuiyue") and use.from:hasSkill("zuiyue") then
		local ana = sgs.cloneCard("analeptic", sgs.Card_NoSuit, 0)
		ana:setSkillName("zuiyue")
		if sgs.Analeptic_IsAvailable(self.player, ana) then
			return ana
		end
	end
	
	local analeptic = self:getCard("Analeptic", nil, "MagicAnaleptic")
	if not analeptic then return nil end

	local analepticAvail = 1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, self.player, analeptic)
	local slashAvail = 0

	for _, card in ipairs(self.toUse) do
		if analepticAvail == 1 and card:getEffectiveId() ~= slash:getEffectiveId() and card:isKindOf("Slash") then return nil end
		if card:isKindOf("Slash") then slashAvail = slashAvail + 1 end
	end

	if analepticAvail > 1 and analepticAvail < slashAvail then return nil end
	if not sgs.Analeptic_IsAvailable(self.player) then return nil end
	local shouldUse = false
	for _, p in sgs.qlist(use.to) do
		if not p:hasSkill("zhenlie") and not (p:hasSkill("anxian") and not p:isKongcheng()) then
			shouldUse = true
			break
		end
	end
	if not shouldUse then return nil end

	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:fillSkillCards(cards)



	local card_str = self:getCardId("Analeptic", nil, nil, "MagicAnaleptic")
	if card_str then  return sgs.Card_Parse(card_str) end

	for _, anal in ipairs(cards) do
		if (anal:getClassName() == "Analeptic" and not anal:isKindOf("MagicAnaleptic")) and not (anal:getEffectiveId() == slash:getEffectiveId()) then
			return anal
		end
	end
end

sgs.dynamic_value.benefit.Analeptic = true

sgs.ai_use_value.Analeptic = 5.98
sgs.ai_keep_value.Analeptic = 4.1
sgs.ai_use_priority.Analeptic = 3.0

sgs.ai_card_intention.Analeptic = function(self, card, from, tos)
	for _, to in ipairs(tos) do
		if to:hasFlag("Global_Dying") then 
			sgs.updateIntention(from, to, -120)
		end
	end
end

local function handcard_subtract_hp(a, b)
	local diff1 = a:getHandcardNum() - a:getHp()
	local diff2 = b:getHandcardNum() - b:getHp()

	return diff1 < diff2
end

function SmartAI:useCardSupplyShortage(card, use)
	local enemies = self:exclude(self.enemies, card)

	--诵经
	for _, e in pairs(enemies) do
		if e:hasSkill("songjing") then
			return
		end
	end

	local zhanghe = self.room:findPlayerBySkillName("qiaobian")
	local zhanghe_seat = zhanghe and zhanghe:faceUp() and not zhanghe:isKongcheng() and not self:isFriend(zhanghe) and zhanghe:getSeat() or 0

	local sb_daqiao = self.room:findPlayerBySkillName("yanxiao")
	local yanxiao = sb_daqiao and not self:isFriend(sb_daqiao) and sb_daqiao:faceUp() and
					(getKnownCard(sb_daqiao, self.player, "diamond", nil, "hes") > 0
					or sb_daqiao:getHandcardNum() + self:ImitateResult_DrawNCards(sb_daqiao, sb_daqiao:getVisibleSkillList()) > 3
					or sb_daqiao:containsTrick("YanxiaoCard"))
	--东方杀中类张郃的角色
	local mouko = self.room:findPlayerBySkillName("sidou")
	local mouko_seat = mouko and mouko:faceUp() and not self:isFriend(mouko) and mouko:getSeat() or 0
	local marisa = self.room:findPlayerBySkillName("jiezou")
	local marisa_seat = marisa and card:getSuit()==sgs.Card_Spade and marisa:faceUp()  and not self:isFriend(marisa) and marisa:getSeat() or 0



	local getvalue = function(enemy)
		if enemy:containsTrick("supply_shortage") or enemy:containsTrick("YanxiaoCard") then return -100 end
		if self:touhouDelayTrickBadTarget(card, enemy, self.player) then return -100 end
		if enemy:getMark("juao") > 0 then return -100 end
		if enemy:hasSkill("qiaobian") and not enemy:containsTrick("supply_shortage") and not enemy:containsTrick("indulgence") then return -100 end
		if zhanghe_seat > 0 and (self:playerGetRound(zhanghe) <= self:playerGetRound(enemy) and self:enemiesContainsTrick() <= 1 or not enemy:faceUp()) then
			return - 100 end
		if mouko_seat > 0 and (self:playerGetRound(mouko) <= self:playerGetRound(enemy) and self:enemiesContainsTrick() <= 1 or not enemy:faceUp()) then
			return - 100 end
		if marisa_seat > 0 and (self:playerGetRound(marisa) < self:playerGetRound(enemy) and self:enemiesContainsTrick() <= 1 or not enemy:faceUp()) then
			return - 100 end
		if yanxiao and (self:playerGetRound(sb_daqiao) <= self:playerGetRound(enemy) and self:enemiesContainsTrick(true) <= 1 or not enemy:faceUp()) then
			return -100 end

		local value = 0 - enemy:getHandcardNum()

		if self:hasSkills("yongsi|haoshi|tuxi|noslijian|lijian|fanjian|neofanjian|dimeng|jijiu|jieyin|manjuan|beige",enemy)
		  or (enemy:hasSkill("zaiqi") and enemy:getLostHp() > 1)
			then value = value + 10
		end
		if self:hasSkills(sgs.cardneed_skill,enemy) or self:hasSkills("zhaolie|qinyin|yanxiao|zhaoxin|toudu|renjie",enemy) --tianxiang
			then value = value + 5
		end
		if self:hasSkills("yingzi|shelie|xuanhuo|buyi|jujian|jiangchi|mizhao|hongyuan|duoshi",enemy) then value = value + 1 end
		if enemy:hasSkill("zishou") then value = value + enemy:getLostHp() end
		if self:isWeak(enemy) then value = value + 5 end
		if enemy:isLord() then value = value + 3 end
		--这一句作何解。。。我根本不相信现在的objectivelevel。。。。
		if self:objectiveLevel(enemy) < 3 then value = value - 10 end
		if not enemy:faceUp() then value = value - 10 end
		if self:hasSkills("keji|shensu|qingyi", enemy) then value = value - enemy:getHandcardNum() end
		if self:hasSkills("guanxing|xiuluo|tiandu|guidao|noszhenlie", enemy) then value = value - 5 end
		if not sgs.isGoodTarget(enemy, self.enemies, self) then value = value - 1 end
		if self:needKongcheng(enemy) then value = value - 1 end
		if enemy:getMark("@kuiwei") > 0 then value = value - 2 end
		return value
	end

	local getSeatWeight = function(enemies,enemy)
		local weight = 0
		for _,s in pairs(enemies) do
			if (self:playerGetRound(s) < self:playerGetRound(enemy)) then
				weight=weight-10
			end
		end
		return weight
	end

	local cmp = function(a,b)
		return (getvalue(a)+getSeatWeight(enemies,a)) >  (getvalue(b)+getSeatWeight(enemies,b))
	end

	if #enemies > 0 then
		table.sort(enemies, cmp)

		local target = enemies[1]
		if getvalue(target) > -100 then
			use.card = card
			if use.to then use.to:append(target) end
			return
		end
	end

	--use to friends
	local friends  = self:exclude(self.friends_noself, card)
	local friendNames ={}
	for _,p in pairs(friends) do
		table.insert(friendNames,p:objectName())
	end
	local baoyi = self.room:findPlayerBySkillName("baoyi")

	if baoyi and self:isFriend(baoyi) and table.contains(friendNames,baoyi:objectName())  then
		use.card = card
		if use.to then use.to:append(baoyi) end
		return
	end
end

sgs.ai_use_value.SupplyShortage = 7
sgs.ai_keep_value.SupplyShortage = 3.48
sgs.ai_use_priority.SupplyShortage = 0.5
--sgs.ai_card_intention.SupplyShortage = 120
sgs.ai_card_intention.SupplyShortage = function(self, card, from, tos)
	if not tos[1]:hasSkill("baoyi") then
		sgs.updateIntentions(from, tos, 120)
	end

end
sgs.dynamic_value.control_usecard.SupplyShortage = true
sgs.ai_judge_model.supply_shortage = function(self, who)
	local judge = sgs.JudgeStruct()
	judge.who = who
	judge.pattern = ".|club"
	judge.good = true
	judge.reason = "supply_shortage"
	return judge
end


function SmartAI:getChainedFriends(player)
	player = player or self.player
	local chainedFriends = {}
	for _, friend in ipairs(self:getFriends(player)) do
		if friend:isChained() then
			table.insert(chainedFriends, friend)
		end
	end
	return chainedFriends
end

function SmartAI:getChainedEnemies(player)
	player = player or self.player
	local chainedEnemies = {}
	for _, enemy in ipairs(self:getEnemies(player)) do
		if enemy:isChained() then
			table.insert(chainedEnemies,enemy)
		end
	end
	return chainedEnemies
end

function SmartAI:isGoodChainPartner(player)
	player = player or self.player
	if hasBuquEffect(player) or (self.player:hasSkill("niepan") and self.player:getMark("@nirvana") > 0) or self:needToLoseHp(player)
		or self:getDamagedEffects(player) or (player:hasSkill("fuli") and player:getMark("@laoji") > 0) then
		return true
	end
	return false
end

function SmartAI:isGoodChainTarget(who, source, nature, damagecount, slash)
	if not who:isChained() then return false end
	source = source or self.player
	nature = nature or sgs.DamageStruct_Fire

	damagecount = damagecount or 1
	if slash and slash:isKindOf("Slash") then
		nature = slash:isKindOf("FireSlash") and sgs.DamageStruct_Fire
					or slash:isKindOf("ThunderSlash") and sgs.DamageStruct_Thunder
					or sgs.DamageStruct_Normal
		damagecount = self:hasHeavySlashDamage(source, slash, who, true)
	elseif nature == sgs.DamageStruct_Fire then
		if who:hasArmorEffect("Vine") then damagecount = damagecount + 1 end
	end


	if not self:damageIsEffective(who, nature, source) then return end

	if who:hasArmorEffect("SilverLion") then damagecount = 1 end

	local kills, killlord, the_enemy = 0
	local good, bad, F_count, E_count = 0, 0, 0, 0
	local peach_num = self.player:objectName() == source:objectName() and self:getCardsNum("Peach") or getCardsNum("Peach", source)

	local function getChainedPlayerValue(target, dmg)
		local newvalue = 0
		if self:isGoodChainPartner(target) then newvalue = newvalue + 1 end
		if self:isWeak(target) then newvalue = newvalue - 1 end
		if dmg and nature == sgs.DamageStruct_Fire then
			if target:hasArmorEffect("Vine") then dmg = dmg + 1 end
		end
		if self:cantbeHurt(target, source, damagecount) then newvalue = newvalue - 100 end
		if damagecount + (dmg or 0) >= target:getHp() then
			newvalue = newvalue - 2
			if target:isLord() and not self:isEnemy(target, source) then killlord = true end
			if self:isEnemy(target, source) then kills = kills + 1 end
		else
			if self:isEnemy(target, source) and source:getHandcardNum() < 2 and target:hasSkills("ganglie|neoganglie") and source:getHp() == 1
				and self:damageIsEffective(source, nil, target) and peach_num < 1 then newvalue = newvalue - 100 end
			if target:hasSkill("vsganglie") then
				local can
				for _, t in ipairs(self:getFriends(source)) do
					if t:getHp() == 1 and t:getHandcardNum() < 2 and self:damageIsEffective(t, nil, target) and peach_num < 1 then
						if t:isLord() then
							newvalue = newvalue - 100
							if not self:isEnemy(t, source) then killlord = true end
						end
						can = true
					end
				end
				if can then newvalue = newvalue - 2 end
			end
		end

		if target:hasArmorEffect("SilverLion") then return newvalue - 1 end
		return newvalue - damagecount - (dmg or 0)
	end


	local value = getChainedPlayerValue(who)
	if self:isFriend(who) then
		good = value
		F_count = F_count + 1
	elseif self:isEnemy(who) then
		bad = value
		E_count = E_count + 1
	end

	if nature == sgs.DamageStruct_Normal then return good >= bad end

	for _, player in sgs.qlist(self.room:getAllPlayers()) do
		if player:objectName() ~= who:objectName() and player:isChained() and self:damageIsEffective(player, nature, source) then
			local getvalue = getChainedPlayerValue(player, 0)
			if kills == #self:getEnemies(source) and not killlord and sgs.getDefenseSlash(player, self) < 2 then
				if slash then self.room:setCardFlag(slash, "AIGlobal_KillOff") end
				return true
			end
			if self:isFriend(player) then
				good = good + getvalue
				F_count = F_count + 1
			elseif self:isEnemy(player) then
				bad = bad + getvalue
				E_count = E_count + 1
				the_enemy = player
			end
		end
	end

	if killlord and sgs.evaluatePlayerRole(source) == "rebel" then return true end

	if slash and F_count == 1 and E_count == 1 and the_enemy and the_enemy:isKongcheng() and the_enemy:getHp() == 1 then
		for _, c in ipairs(self:getCards("Slash")) do
			if not c:isKindOf("NatureSlash") and self:slashProhibit(slash, the_enemy, source) then return end
		end
	end

	if F_count > 0 and E_count <= 0 then return end

	return good >= bad
end

--铁索能不能有其他目标时，不要锁卖血流啊
--静电对策需要详细写
function SmartAI:useCardIronChain(card, use)
	local needTarget = (card:getSkillName() == "guhuo" or card:getSkillName() == "nosguhuo"
		or card:getSkillName() == "qice" or card:getSkillName() == "chaoren")
	if not needTarget then
		needTarget = self.player:getPile("wooden_ox"):contains(card:getEffectiveId())
		--getSubcards()为empty。。。 什么鬼
		--[[for _, id in sgs.qlist(self.player:getPile("wooden_ox")) do
			if card:getSubcards():contains(id) then
				needTarget = true
				break
			end
		end]]
	end
	if not needTarget then use.card = card end
	local toyohime =self.room:findPlayerBySkillName("yueshi")
	if not needTarget then
		if self.player:isLocked(card) then return end

		if #self.enemies == 1 and #(self:getChainedFriends()) <= 1 then
			if toyohime and self:isFriend(toyohime) and self:needWakeYueshi(toyohime) then
			else
				return
			end
		end
		if self:needBear() then return end
	end

	local friendtargets = {}
	local otherfriends = {}
	local enemytargets = {}

	self:sort(self.friends, "defense")
	for _, friend in ipairs(self.friends) do
		if not (use.current_targets and table.contains(use.current_targets, friend:objectName())) then
			if friend:isChained() and not self:isGoodChainPartner(friend) and self:hasTrickEffective(card, friend) then
				if not self:needWakeYueshi(friend) then
					table.insert(friendtargets, friend)
				end
			else
				if self:needWakeYueshi(friend) then
					table.insert(friendtargets, friend)
				else
					table.insert(otherfriends, friend)
				end
			end
		end
	end

	-- add enemies
	self:sort(self.enemies, "defense")
	for _, enemy in ipairs(self.enemies) do
		if (not use.current_targets or not table.contains(use.current_targets, enemy:objectName()))
			and not enemy:isChained() and not self.room:isProhibited(self.player, enemy, card)
			and not enemy:hasSkill("jingdian")
			and self:hasTrickEffective(card, enemy) and not (self:objectiveLevel(enemy) <= 3)
			and not self:getDamagedEffects(enemy) and not self:needToLoseHp(enemy) and sgs.isGoodTarget(enemy, self.enemies, self) then
			if not self:needWakeYueshi(enemy) then
				table.insert(enemytargets, enemy)
			end
		end
	end


	local chainSelf = (not use.current_targets or not table.contains(use.current_targets, self.player:objectName()))
						and (self:needToLoseHp(self.player) or self:getDamagedEffects(self.player)) and not self.player:isChained()
						and (self:getCardId("FireSlash") or self:getCardId("ThunderSlash") or
							(self:getCardId("Slash") and (self.player:hasWeapon("Fan") or self.player:hasSkill("lihuo")))
						or (self:getCardId("FireAttack") and self.player:getHandcardNum() > 2))
	if self:needWakeYueshi() then
		chainSelf=true
	end
	local targets_num = 2 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, card)
	if #friendtargets > 1 then
		if use.to then
			for _, friend in ipairs(friendtargets) do
				use.to:append(friend)
				if use.to:length() == targets_num then return end
			end
		end
	elseif #friendtargets == 1 then
		if #enemytargets > 0 then
			if use.to then
				use.to:append(friendtargets[1])
				for _, enemy in ipairs(enemytargets) do
					use.to:append(enemy)
					if use.to:length() == targets_num then return end
				end
			end
		elseif chainSelf then
			if use.to then use.to:append(friendtargets[1]) end
			if use.to then use.to:append(self.player) end
		elseif use.current_targets then
			if use.to then use.to:append(friendtargets[1]) end
		end
	elseif #enemytargets > 1 then
		if use.to then
			for _, enemy in ipairs(enemytargets) do
				use.to:append(enemy)
				if use.to:length() == targets_num then return end
			end
		end
	elseif #enemytargets == 1 then
		if chainSelf then
			if use.to then use.to:append(enemytargets[1]) end
			if use.to then use.to:append(self.player) end
		elseif use.current_targets then
			if use.to then use.to:append(enemytargets[1]) end
		end
	end

	if use.to then assert(use.to:length() < targets_num + 1) end
	if needTarget and use.to and use.to:isEmpty() then use.card = nil end
end

sgs.ai_card_intention.IronChain = function(self, card, from, tos)
	for _, to in ipairs(tos) do
		if not (to:hasSkill("xunshi") and #tos > 1) then
			if not to:isChained() then
				if not self:needWakeYueshi(to) then
					sgs.updateIntention(from, to, 60)
				end
			else
				sgs.updateIntention(from, to, -60)
			end
		end
	end
end

sgs.ai_use_value.IronChain = 5.4
sgs.ai_keep_value.IronChain = 3.34
sgs.ai_use_priority.IronChain = 9.1

sgs.ai_skill_cardask["@fire-attack"] = function(self, data, pattern, target)
	local cards = sgs.QList2Table(self.player:getHandcards())
	local convert = { [".S"] = "spade", [".D"] = "diamond", [".H"] = "heart", [".C"] = "club"}
	local card
	--寻事被火攻砸惨了。先强行不砸队友。。。反正使用火攻本身现在也没考虑队友
	target = data:toCardEffect().to
	if target:objectName() ~= self.player:objectName() and self:isFriend(target) then return "." end

	self:sortByUseValue(cards, true)
	local lord = self.room:getLord()
	if sgs.GetConfig("EnableHegemony", false) then lord = nil end

	for _, acard in ipairs(cards) do
		if  pattern:find(acard:getSuitString())  then --or acard:getSuitString() == convert[pattern]
			if not isCard("Peach", acard, self.player) then
				card = acard
				break
			else
				local needKeepPeach = true
				if (self:isWeak(target) and not self:isWeak()) or target:getHp() == 1
						or self:isGoodChainTarget(target) or target:hasArmorEffect("Vine") then
					needKeepPeach = false
				end
				if (self.player:hasSkill("fenxiang") and self.player:getCards("h"):contains(acard)) then
					needKeepPeach = false
				end
				if lord and not self:isEnemy(lord) and sgs.isLordInDanger() and self:getCardsNum("Peach") == 1 and self.player:aliveCount() > 2 then
					needKeepPeach = true
				end
				if not needKeepPeach then
					card = acard
					break
				end
			end
		end
	end
	return card and card:getId() or "."
end

function SmartAI:useCardFireAttack(fire_attack, use) --尼玛 吃酒+火攻+丢杀 然后断杀。。。
	local lack = {
		spade = true,
		club = true,
		heart = true,
		diamond = true,
	}

	local cards = self.player:getHandcards()
	local canDis = {}
	for _, card in sgs.qlist(cards) do
		if card:getEffectiveId() ~= fire_attack:getEffectiveId() then
			table.insert(canDis, card)
			lack[card:getSuitString()] = false
		end
	end

	if self.player:hasSkill("hongyan") then
		lack.spade = true
	end

	local suitnum = 0
	for suit,islack in pairs(lack) do
		if not islack then suitnum = suitnum + 1  end
	end


	self:sort(self.enemies, "defense")
    local fire_Kongcheng = function(enemy)
		if (enemy:objectName() == self.player:objectName())  then
			local unShowns = enemy:getCards("h")
            if (fire_attack:isVirtualCard()) then
				local num = 0
				for _, id in sgs.qlist(fire_attack:getSubcards()) do
					local card = sgs.sanguosha:getCard(id)
					if unShowns:contains(card) then
						num = num+1
					end
				end
				return num >= unShowns:length()
			else
				if unShowns:length() == 1 and unShowns:contains(fire_attack) then
					return true
				end
			end
        end
		return enemy:getCards("h"):length() == 0
	end
	local can_attack = function(enemy)
		if self.player:hasFlag("FireAttackFailed_" .. enemy:objectName()) then
			return false
		end
		local damage = 1
		if not enemy:hasArmorEffect("SilverLion") then
			if enemy:hasArmorEffect("Vine") then damage = damage + 1 end
		end
		return self:objectiveLevel(enemy) > 3 and damage > 0 and not fire_Kongcheng(enemy) --not enemy:isKongcheng() 
				and not self.room:isProhibited(self.player, enemy, fire_attack)
				and self:damageIsEffective(enemy, sgs.DamageStruct_Fire, self.player) and not self:cantbeHurt(enemy, self.player, damage)
				and self:hasTrickEffective(fire_attack, enemy)
				and sgs.isGoodTarget(enemy, self.enemies, self)
				and (not (enemy:hasSkill("jianxiong") and not self:isWeak(enemy))
						and not (self:getDamagedEffects(enemy, self.player))
						and not (enemy:isChained() and not self:isGoodChainTarget(enemy)))
	end

	local enemies, targets = {}, {}
	for _, enemy in ipairs(self.enemies) do
		if (not use.current_targets or not table.contains(use.current_targets, enemy:objectName())) and can_attack(enemy) then
			if not self:touhouCardAttackWaste(fire_attack,self.player,enemy)
			and not self:trickProhibit(fire_attack, enemy, self.player)  then
				table.insert(enemies, enemy)
			end
		end
	end

	local can_FireAttack_self
	for _, card in ipairs(canDis) do
		if (not isCard("Peach", card, self.player) or self:getCardsNum("Peach") >= 3) and not self.player:hasArmorEffect("IronArmor")
			and (not isCard("Analeptic", card, self.player) or self:getCardsNum("Analeptic") >= 2) then
			can_FireAttack_self = true
		end
	end

	if (not use.current_targets or not table.contains(use.current_targets, self.player:objectName()))
		and self.role ~= "renegade" and can_FireAttack_self and self.player:isChained() and self:isGoodChainTarget(self.player)
		and	fire_Kongcheng(self.player)--and self.player:getHandcardNum() > 1
		and not self.room:isProhibited(self.player, self.player, fire_attack)
		and self:damageIsEffective(self.player, sgs.DamageStruct_Fire, self.player) and not self:cantbeHurt(self.player)
		and self:hasTrickEffective(fire_attack, self.player)
		and (self.player:getHp() > 1 or self:getCardsNum("Peach") >= 1 or self:getCardsNum("Analeptic") >= 1 or self.player:hasSkill("buqu")
			or (self.player:hasSkill("niepan") and self.player:getMark("@nirvana") > 0)) then

		table.insert(targets, self.player)
	end

	for _, enemy in ipairs(enemies) do
		if (not use.current_targets or not table.contains(use.current_targets, enemy:objectName())) and enemy:getHandcardNum() == 1 then
			local handcards = sgs.QList2Table(enemy:getCards("h"))--enemy:getHandcards()
			local flag = string.format("%s_%s_%s", "visible", self.player:objectName(), enemy:objectName())
			if (handcards[1]:hasFlag("visible") or handcards[1]:hasFlag(flag)) then
				local suitstring = handcards[1]:getSuitString()
				if not lack[suitstring] and not table.contains(targets, enemy) then
					table.insert(targets, enemy)
				end
			end
		end
	end

	if ((suitnum == 2 and lack.diamond == false) or suitnum <= 1)
		and self:getOverflow() <= (self.player:hasSkills("jizhi|nosjizhi") and -2 or 0)
		and #targets == 0 then return end

	for _, enemy in ipairs(enemies) do
		local damage = 1
		if not enemy:hasArmorEffect("SilverLion") then
			if enemy:hasArmorEffect("Vine") then damage = damage + 1 end
		end
		if (not use.current_targets or not table.contains(use.current_targets, enemy:objectName()))
			and self:damageIsEffective(enemy, sgs.DamageStruct_Fire, self.player) and damage > 1 then
			if not table.contains(targets, enemy) then table.insert(targets, enemy) end
		end
	end
	for _, enemy in ipairs(enemies) do
		if (not use.current_targets or not table.contains(use.current_targets, enemy:objectName())) and not table.contains(targets, enemy) then table.insert(targets, enemy) end
	end

	if #targets > 0 then
		local godsalvation = self:getCard("GodSalvation")
		if godsalvation and godsalvation:getId() ~= fire_attack:getId() and self:willUseGodSalvation(godsalvation) then
			local use_gs = true
			for _, p in ipairs(targets) do
				if not p:isWounded() or not self:hasTrickEffective(godsalvation, p, self.player) then break end
				use_gs = false
			end
			if use_gs then
				use.card = godsalvation
				return
			end
		end

		local targets_num = 1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, fire_attack)
		use.card = fire_attack
		for i = 1, #targets, 1 do
			if use.to then
				use.to:append(targets[i])
				if use.to:length() == targets_num then return end
			end
		end
	end
end

sgs.ai_cardshow.fire_attack = function(self, requestor)
	local cards = sgs.QList2Table(self.player:getHandcards())
	if requestor:objectName() == self.player:objectName() then
		self:sortByUseValue(cards, true)
		return cards[1]
	end

	local priority = { heart = 4, spade = 3, club = 2, diamond = 1 }
	if requestor:hasSkill("hongyan") then priority = { spade = 10, club = 2, diamond = 1, heart = 0 } end
	local index = -1
	local result
	for _, card in ipairs(cards) do
		if priority[card:getSuitString()] > index then
			result = card
			index = priority[card:getSuitString()]
		end
	end

	return result
end
sgs.ai_skill_cardask["@fire_attack_show"] = function(self, data)
	local cards = sgs.QList2Table(self.player:getCards("h"))
	requestor = data:toCardEffect().to
	if requestor:objectName() == self.player:objectName() then
		self:sortByUseValue(cards, true)
		return cards[1]
	end

	local priority = { heart = 4, spade = 3, club = 2, diamond = 1 }
	if requestor:hasSkill("hongyan") then priority = { spade = 10, club = 2, diamond = 1, heart = 0 } end
	local index = -1
	local result
	for _, card in ipairs(cards) do
		if priority[card:getSuitString()] > index then
			result = card
			index = priority[card:getSuitString()]
		end
	end
	return "$" .. result:getId()
end

sgs.ai_use_value.FireAttack = 4.8
sgs.ai_keep_value.FireAttack = 3.3
sgs.ai_use_priority.FireAttack = sgs.ai_use_priority.Dismantlement + 0.1

sgs.dynamic_value.damage_card.FireAttack = true

sgs.ai_card_intention.FireAttack = 80

--IronArmor
function sgs.ai_armor_value.IronArmor(player, self)
	if self:isWeak(player) then
		for _, p in sgs.qlist(self.room:getOtherPlayers(player)) do
			if p:hasSkill("zhence") and self:isEnemy(player, p) then
				return 5
			end
		end
	end
	return 2.5
end

sgs.ai_use_priority.IronArmor = 0.82
