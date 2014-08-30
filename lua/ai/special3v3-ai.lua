sgs.ai_skill_choice["3v3_direction"] = function(self, choices, data)
	local card = data:toCard()
	local aggressive = (card and card:isKindOf("AOE"))
	if self:isFriend(self.player:getNextAlive()) == aggressive then return "cw" else return "ccw" end
end

sgs.ai_skill_playerchosen.vsganglie = function(self, targets)
	self:sort(self.enemies)
	local prior_enemies = {}
	for _, enemy in ipairs(self.enemies) do
		if enemy:getHandcardNum() < 2 then table.insert(prior_enemies, enemy) end
	end
	for _, enemy in ipairs(prior_enemies) do
		if self:canAttack(enemy) then return enemy end
	end
	for _, enemy in ipairs(self.enemies) do
		if self:canAttack(enemy) or enemy:getHandcardNum() > 3 then return enemy end
	end
	if self.room:getMode() == "06_3v3" or self.room:getMode() == "06_XMode" then return nil end
	for _, friend in ipairs(self.friends_noself) do
		if self:damageIsEffective(friend, sgs.DamageStruct_Normal, friend) and not self:cantbeHurt(friend) and self:getDamagedEffects(friend, self.player) then
			sgs.ai_ganglie_effect = string.format("%s_%s_%d", self.player:objectName(), friend:objectName(), sgs.turncount)
			return friend
		end
	end
	return nil
end

sgs.ai_playerchosen_intention.vsganglie = function(self, from, to)
	if sgs.ai_ganglie_effect and sgs.ai_ganglie_effect == string.format("%s_%s_%d", from:objectName(), to:objectName(), sgs.turncount) then
		sgs.updateIntention(from, to, -10)
	elseif from:getState() == "online" then
		if not from:hasSkill("jueqing") and self:getDamagedEffects(to, from) then return end
		sgs.updateIntention(from, to, 40)
	else
		sgs.updateIntention(from, to, 80)
	end
end

sgs.ai_need_damaged.vsganglie = function(self, attacker, player)
	for _, enemy in ipairs(self.enemies) do
		if self:isEnemy(enemy, player) and enemy:getHp() + enemy:getHandcardNum() <= 3
			and not (enemy:hasSkills(sgs.need_kongcheng) and not hasBuquEffect(enemy) and enemy:getHandcardNum() > 1) and sgs.isGoodTarget(enemy, self.enemies, self) then
			return true
		end
	end
	return false
end

sgs.ai_skill_discard.vsganglie = function(self, discard_num, min_num, optional, include_equip)
	return ganglie_discard(self, discard_num, min_num, optional, include_equip, "vsganglie")
end

function sgs.ai_slash_prohibit.vsganglie(self, from, to)
	if self:isFriend(from, to) then return false end
	if from:hasSkill("jueqing") or (from:hasSkill("nosqianxi") and from:distanceTo(to) == 1) then return false end
	if from:hasFlag("NosJiefanUsed") then return false end
	if #(self:getEnemies(from)) > 1 then
		for _, p in ipairs(self:getFriends(from)) do
			if p:getHandcardNum() + p:getHp() < 4 then return true end
		end
	end
	return false
end

local zhongyi_skill = {}
zhongyi_skill.name = "zhongyi"
table.insert(sgs.ai_skills, zhongyi_skill)
zhongyi_skill.getTurnUseCard = function(self)
	if self.player:getMark("@loyal") <= 0 or self.player:isKongcheng() or sgs.turncount <= 1 then return end

	cards = sgs.QList2Table(self.player:getHandcards())
	local red_card
	self:sortByUseValue(cards, true)
	for _, card in ipairs(cards) do
		if card:isRed() and not isCard("Peach", card, self.player) and not isCard("ExNihilo", card, self.player) then
			red_card = card
			break
		end
	end

	if not red_card then return end

	local value = 0
	local friends = {}
	if self.room:getMode() == "06_3v3" then
		for _, friend in ipairs(self.friends) do
			if not friend:hasFlag("actioned") then table.insert(friends, friend) end
		end
	else
		friends = self.friends
	end
	self:sort(self.enemies)
	local slash = sgs.Sanguosha:cloneCard("slash")
	for _, friend in ipairs(friends) do
		local local_value = 0
		for _, enemy in ipairs(self.enemies) do
			if friend:canSlash(enemy) and not self:slashProhibit(slash, enemy) and self:slashIsEffective(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self) then
				local_value = local_value + 0.8
				if getCardsNum("Jink", enemy) < 1 then local_value = local_value + 0.5 end
				if friend:hasSkill("tieji")
					or (friend:hasSkill("liegong") and (enemy:getHandcardNum() <= friend:getAttackRange() or enemy:getHandcardNum() >= friend:getHp()))
					or (friend:hasSkill("kofliegong") and enemy:getHandcardNum() >= friend:getHp()) then
					local_value = local_value + 0.5
				end
				break
			end
		end
		if getCardsNum("Slash", friend) < 1 then local_value = local_value * 0.3
		elseif self:hasCrossbowEffect(friend) then local_value = local_value * getCardsNum("Slash", friend) end
		if friend:hasSkill("shensu") and not self:isWeak(friend) then local_value = local_value * 1.2
		elseif self:willSkipPlayPhase(friend) then local_value = local_value * 0.2 end
		value = value + local_value
	end
	local ratio = value / #self.enemies
	if ratio > 0.85 then return sgs.Card_Parse("@ZhongyiCard=" .. red_card:getEffectiveId()) end
end

sgs.ai_skill_use_func.ZhongyiCard = function(card, use, self)
	use.card = card
end

sgs.ai_use_priority.ZhongyiCard = 10

sgs.ai_skill_invoke.zhongyi = function(self, data)
	local damage = data:toDamage()
	return self:isEnemy(damage.to)
end

function sgs.ai_cardsview.jiuzhu(self, class_name, player)
	if class_name == "Peach" and player:getHp() > 1 and not player:isNude() then
		local dying = self.room:getCurrentDyingPlayer()
		if not dying then return nil end
		if (self.room:getMode() == "06_3v3" or self.room:getMode() == "06_XMode") and not self:isFriend(dying) then return nil end
		local must_save = false
		if self.room:getMode() == "06_3v3" then
			if dying:getRole() == "renegade" or dying:getRole() == "lord" then must_save = true end
		elseif dying:isLord() and (self.role == "loyalist" or (self.role == "renegade" and self.room:alivePlayerCount() > 2)) then
			must_save = true
		end
		if not must_save and self:isWeak(player) and not player:hasArmorEffect("SilverLion") then return nil end
		local to_discard = self:askForDiscard(player, "dummyreason", 1, 1, false, true)
		if #to_discard == 1 then return "@JiuzhuCard=" .. to_discard[1] .. "->." end
		return nil
	end
end

sgs.ai_card_intention.JiuzhuCard = sgs.ai_card_intention.Peach

sgs.ai_skill_invoke.zhanshen = function(self, data)
	local obj = data:toString():split(":")[2]
	local lvbu = findPlayerByObjectName(self.room, obj)
	return self:isFriend(lvbu)
end

sgs.ai_skill_use["@@zhenwei"] = function(self, prompt)
	local total = math.floor(sgs.Sanguosha:getPlayerCount(self.room:getMode()) / 2) - 1
	if total == 0 then return "." end
	local targets = {}
	if #self.friends_noself > 0 then
		self:sort(self.friends_noself, "defense")
		for _, friend in ipairs(self.friends_noself) do
			table.insert(targets, friend:objectName())
			if #targets == total then break end
		end
	elseif #self.enemies > 0 and sgs.turncount >= 2 then
		self:sort(self.enemies, "defense")
		for _, enemy in ipairs(self.enemies) do
			table.insert(targets, enemy:objectName())
			if #targets == total then break end
		end
	end
	if #targets == 0 then return "."
	else return "@ZhenweiCard=.->" .. table.concat(targets, "+")
	end
end

sgs.ai_card_intention.ZhenweiCard = function(self, card, from, tos)
	if #(self:getFriendsNoself(from)) > 0 then
		for _, to in ipairs(tos) do
			sgs.updateIntention(from, to, -50)
		end
	end
end

sgs.weapon_range.VSCrossbow = sgs.weapon_range.Crossbow
sgs.ai_use_priority.VSCrossbow = sgs.ai_use_priority.Crossbow
