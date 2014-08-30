sgs.ai_skill_invoke.neo2013renwang = sgs.ai_skill_invoke.danlao

local neo2013xinzhan_skill={}
neo2013xinzhan_skill.name="neo2013xinzhan"
table.insert(sgs.ai_skills,neo2013xinzhan_skill)
neo2013xinzhan_skill.getTurnUseCard=function(self)
	if not self.player:hasUsed("Neo2013XinzhanCard") and self.player:getHandcardNum() > self.player:getLostHp() then
		return sgs.Card_Parse("@Neo2013XinzhanCard=.")
	end
end

sgs.ai_skill_use_func.Neo2013XinzhanCard=function(card,use,self)
	use.card = card
end

sgs.ai_use_value.Neo2013XinzhanCard = 4.5
sgs.ai_use_priority.Neo2013XinzhanCard = 9.5

sgs.ai_slash_prohibit.neo2013huilei = sgs.ai_slash_prohibit.huilei

sgs.ai_skill_invoke.neo2013yishi = sgs.ai_skill_invoke.yishi

function sgs.ai_cardsview.neo2013haoyin(self, class_name, player)
	if class_name == "Analeptic" and player:hasSkill("neo2013haoyin") and sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_PLAY then
		if player:getHp() < 2 or self:isWeak(player) then return nil end
		return ("analeptic:neo2013haoyin[no_suit:0]=.")
	end
end

sgs.ai_skill_invoke.neo2013zhulou = sgs.ai_skill_invoke.zhulou
sgs.ai_cardneed.neo2013zhulou = sgs.ai_cardneed.weapon
sgs.neo2013zhulou_keep_value = sgs.qiangxi_keep_value

local neo2013fanjian_skill = {}
neo2013fanjian_skill.name = "neo2013fanjian"
table.insert(sgs.ai_skills, neo2013fanjian_skill)
neo2013fanjian_skill.getTurnUseCard = function(self)
	if self.player:isKongcheng() or self.player:hasUsed("Neo2013FanjianCard") then return nil end
	return sgs.Card_Parse("@Neo2013FanjianCard=.")
end

sgs.ai_skill_use_func.Neo2013FanjianCard = function(card, use, self)
	self:sort(self.enemies, "defense")
	local target
	for _, enemy in ipairs(self.enemies) do
		if self:canAttack(enemy) and not self:hasSkills("qingnang|tianxiang", enemy) then
			target = enemy

			local wuguotai = self.room:findPlayerBySkillName("buyi")
			local care = (target:getHp() <= 1) and (self:isFriend(target, wuguotai))
			local ucard = nil
			local handcards = self.player:getCards("h")
			handcards = sgs.QList2Table(handcards)
			self:sortByKeepValue(handcards)
			for _,cd in ipairs(handcards) do
				local flag = not (cd:isKindOf("Peach") or cd:isKindOf("Analeptic"))
				local suit = cd:getSuit()
				if flag and care then
					flag = cd:isKindOf("BasicCard")
				end
				if flag and target:hasSkill("longhun") then
					flag = (suit ~= sgs.Card_Heart)
				end
				if flag and target:hasSkill("jiuchi") then
					flag = (suit ~= sgs.Card_Spade)
				end
				if flag and target:hasSkill("jijiu") then
					flag = (cd:isBlack())
				end
				if flag then
					ucard = cd
					break
				end
			end
			if ucard then
				local keep_value = self:getKeepValue(ucard)
				if ucard:getSuit() == sgs.Card_Diamond then keep_value = keep_value + 0.5 end
				if keep_value < 6 then
					use.card = sgs.Card_Parse("@Neo2013FanjianCard=" .. ucard:getEffectiveId())
					if use.to then use.to:append(target) end
					return
				end
			end
		end
	end
end

sgs.ai_card_intention.Neo2013FanjianCard = sgs.ai_card_intention.FanjianCard

function sgs.ai_skill_suit.neo2013fanjian(self)
	local map = {0, 0, 1, 2, 2, 3, 3, 3}
	local suit = map[math.random(1, 8)]
	if self.player:hasSkill("hongyan") and suit == sgs.Card_Spade then return sgs.Card_Heart else return suit end
end

sgs.ai_skill_playerchosen.neo2013fankui = function(self, targets)
	local to
	local player = self:findPlayerToDiscard("he", false, false, targets)
	targets = sgs.QList2Table(targets)
	self:sort(self.friends_noself, "threat")
	self:sort(self.enemies, "defense")
	for _, enemy in ipairs(self.enemies) do
		if not self:doNotDiscard(enemy) or self:getOverflow(fr) >= 0 then to = enemy break end
	end
	if not to then
		for _, fr in ipairs(self.friends_noself) do
			if self:doNotDiscard(fr) or self:getOverflow(fr) > 2 then to = fr break end
		end
	end
	return player or to or nil
end
sgs.ai_playerchosen_intention.neo2013fankui = 30


sgs.ai_skill_discard.neo2013yongyi = function(self, discard_num, min_num, optional, include_equip)
	local cards = self.player:getCards("h")
	if (cards and cards:length() < discard_num) or self:isWeak() then return {} end
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)
	for _,cd in ipairs(cards) do
		if not cd:isKindOf("Peach") and (self:getKeepValue(cd) < 4 or self:cardNeed(cd) < 7) then return {cd:getEffectiveId()} end
	end
	return {}
end

function sgs.ai_cardsview.neo2013yongyi(self, class_name, player)
	if class_name == "Slash" and player:hasSkill("neo2013yongyi") and (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_PLAY or sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
		if player:getPile("neoarrow"):length() == 0 then return nil end
		return ("@Neo2013YongyiCard=.:slash")
	end
end

sgs.ai_skill_use_func.Neo2013YongyiCard = function(card, use, self)
	use.card = card
end


sgs.ai_skill_discard.neo2013duoyi = function(self, discard_num, min_num, optional, include_equip)
	if self.player:isNude() then return {} end
	if self:needKongcheng(self.player, true) and self.player:getHandcardNum() == 1 then return {self.player:handCards():first()} end
	local cards = sgs.QList2Table(self.player:getCards("h"))
	self:sortByKeepValue(cards)
	for _,cd in ipairs(cards) do
		if not cd:isKindOf("Peach") and self:getKeepValue(cd) <= 2 then return {cd:getEffectiveId()} end
	end
	local id = JS_Card(self)
	if id then return {id} else return {} end
end
sgs.ai_skill_choice.neo2013duoyi = function(self, choices)
	local current = self.room:getCurrent()
	if not current then return "BasicCard" end
	if getCardsNum("TrickCard", current) - getCardsNum("Nullification", current) > 0 or getCardsNum("TrickCard", current) > 1 or getCardsNum("ExNihilo", current) > 0 then
		return "TrickCard"
	end
	if self:hasSkills("jizhi|nosjizhi|jilve", current) and getCardsNum("TrickCard", current) > 0 then return "TrickCard" end
	if self:isWeak(current) and getCardsNum("Peach", current) > 0 then return "BasicCard" end
	if self:hasCrossbowEffect(current) and getCardsNum("Slash", current) > 1 then return "BasicCard" end
	if (self:hasSkills(sgs.lose_equip_skill, current) or current:getEquips():isEmpty()) and current:getHandcardNum() > 2 then return "EquipCard" end
	if self:hasCrossbowEffect(current) and getCardsNum("Slash", current) > 0 then return "BasicCard" end
	local choice_table = choices:split("+")
	return choice_table[math.random(1, #choice_table)]
end


function PujiCard(self) --选择一张黑色牌
	local card_id
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByCardNeed(cards)
	local lightning = self:getCard("Lightning")
	if lightning and lightning:isBlack() and not self:willUseLightning(lightning) then card_id = lightning:getEffectiveId() end
	if not card_id then
		if self:needToThrowArmor() and self.player:getArmor():isBlack() then card_id = self.player:getArmor():getId()
		elseif self.player:getHandcardNum() >= self.player:getHp() then
			for _, acard in ipairs(cards) do
				if not acard:isBlack() then continue end
				if (acard:isKindOf("EquipCard") or acard:isKindOf("AmazingGrace") or acard:isKindOf("BasicCard"))
					and self:cardNeed(acard) < 7 and not acard:isKindOf("Peach") then card_id = acard:getEffectiveId() break
				elseif acard:getTypeId() == sgs.Card_TypeTrick then
					local dummy_use = { isDummy = true }
					self:useTrickCard(acard, dummy_use)
					if not dummy_use.card then card_id = acard:getEffectiveId() break end
				end
			end
		elseif not self.player:getEquips():isEmpty() then
			local equips=sgs.QList2Table(self.player:getEquips())
			self:sortByCardNeed(equips)
			for _, card in ipairs(equips) do
				if not card:isBlack() then continue end
				if card:getId() ~= self:getValuableCard(self.player) and not card:isKindOf("Armor") then
					card_id = card:getEffectiveId() break end
			end
		end
	end
	return card_id
end

local neo2013puji_skill = {}
neo2013puji_skill.name = "neo2013puji"
table.insert(sgs.ai_skills, neo2013puji_skill)
neo2013puji_skill.getTurnUseCard = function(self)
	if self.player:isNude() or self.player:hasUsed("Neo2013PujiCard") then return nil end
	local cardid = PujiCard(self) or JS_Card(self)
	if not cardid then return end
	local cardstr = ("@Neo2013PujiCard=%d"):format(cardid)
	local cardc = sgs.Card_Parse(cardstr)
	assert(cardc)
	return cardc
end

sgs.ai_skill_use_func.Neo2013PujiCard = function(card, use, self)
	self:sort(self.enemies, "threat")
	self:sort(self.friends_noself, "defense")
	if sgs.getDefense(self.friends_noself[1]) > 5 then self:sort(self.friends_noself, "hp") end
	for _, friend in ipairs(self.friends_noself) do
		if friend and self:needToThrowArmor(friend) and friend:getArmor():isBlack() then
			use.card = card
			if use.to then use.to:append(friend) end
			return
		end
	end

	for _, friend in ipairs(self.friends_noself) do
		if friend and not friend:getEquips():isEmpty() and friend:hasSkills(sgs.lose_equip_skill) then
			for _, card in sgs.list(friend:getEquips()) do
				if card:isBlack() then
					use.card = card
					if use.to then use.to:append(friend) end
					return
				end
			end
		end
	end

	for _, friend in ipairs(self.friends_noself) do
		local rednum = self:getSuitNum("diamond|heart", true, friend)
		local blacknum = self:getSuitNum("club|spade", true, friend)
		if blacknum >= rednum and not friend:isNude() and friend:isWounded() then
			use.card = card
			if use.to then use.to:append(friend) end
			return
		end
	end

	for _, friend in ipairs(self.friends_noself) do
		local blacknum = self:getSuitNum("club|spade", true, friend)
		if blacknum > 0 and not friend:isNude() and (friend:isWounded() or self:getOverflow(friend) > 0) then
			use.card = card
			if use.to then use.to:append(friend) end
			return
		end
	end

	for _, enemy in ipairs(self.enemies) do
		local rednum = self:getSuitNum("diamond|heart", true, enemy)
		if (rednum > 0 or self:getOverflow(enemy) > 0) and not enemy:isNude() then
			use.card = card
			if use.to then use.to:append(enemy) end
			return
		end
	end
	for _, enemy in ipairs(self.enemies) do
		local rednum = self:getSuitNum("diamond|heart", true, enemy)
		if rednum > 0 and not enemy:getEquips():isEmpty() then
			use.card = card
			if use.to then use.to:append(enemy) end
			return
		end
	end
	for _, enemy in ipairs(self.enemies) do
		local rednum = self:getSuitNum("diamond|heart", true, enemy)
		local cards = sgs.QList2Table(enemy:getCards("he"))
		if rednum > 0 and #cards > 2 then
			use.card = card
			if use.to then use.to:append(enemy) end
			return
		end
	end
end
sgs.ai_use_priority.Neo2013PujiCard = 4.3
sgs.ai_use_value.Neo2013PujiCard = 7
sgs.dynamic_value.lucky_chance.Neo2013PujiCard = true

sgs.ai_skill_cardchosen.neo2013puji  = function(self, who)
	if self:isFriend(who) then
		if self:needToThrowArmor(who) and who:getArmor():isBlack() then return who:getArmor() end
		if not who:getEquips():isEmpty() and who:hasSkills(sgs.lose_equip_skill) then
			local equips = sgs.QList2Table(who:getEquips())
			self:sortByCardNeed(equips)
			for _, card in ipairs(equips) do
				if card:isBlack() then return card end
			end
		end
		local cards = sgs.QList2Table(who:getCards("h"))
		self:sortByKeepValue(cards)
		if #cards > 0 then
			for _, card in ipairs(cards) do
				if not (card:isKindOf("Peach") or card:isKindOf("Analeptic")) and card:isBlack() then return card end
			end
		end
		local equips2 = sgs.QList2Table(who:getEquips())
		self:sortByCardNeed(equips2)
		if #equips2 > 0 then
			for _, card in ipairs(equips2) do
				if not card:isBlack() then continue end
				if card:getId() ~= self:getValuableCard(who) and not card:isKindOf("Armor") then return card end
			end
		end
	elseif self:isEnemy(who) then
		local equips3 = sgs.QList2Table(who:getEquips())
		self:sortByCardNeed(equips3, true)
		if #equips3 > 0 then
			for _, card in ipairs(equips3) do
				if not card:isRed() then continue end
				if card:getId() == self:getValuableCard(who) or card:isKindOf("Armor") or card:getId() == self:getDangerousCard(who) then return card end
			end
		end
		local cards2 = sgs.QList2Table(who:getCards("he"))
		self:sortByUseValue(cards2)
		if #cards2 > 0 then
			for _, card in ipairs(cards2) do
				if card:isRed() then return card end
			end
		end
	end
end

sgs.ai_skill_choice.neo2013puji = sgs.ai_skill_choice.yaowu

sgs.ai_skill_invoke.neo2013suishi = function(self, data)
	local tf = self.room:findPlayerBySkillName("neo2013suishi")
	local damage = data:toDamage()
	if damage then
		if tf:getPhase() == sgs.Player_NotActive and self:needKongcheng(tf, true) then return false end
		return self:isFriend(tf)
	end
	local death = data:toDeath()
	if death and death.damage then
		local player = self:findPlayerToDiscard("he", false)
		if tf and self:isFriend(tf) and player == tf then return true end
		return self:isEnemy(tf) and not self:doNotDiscard(tf)
	end
	return
end

sgs.ai_skill_playerchosen.neo2013shushen = function(self)
	return self:findPlayerToDraw(true) or self.player
end
sgs.ai_playerchosen_intention.neo2013shushen = -50

sgs.ai_skill_invoke.neo2013shenzhi = function(self, data)
	if self:getCardsNum("Peach") > 0 then return false end
	if self.player:getHandcardNum() >= 3 then return false end
	if self.player:isWounded() then return true end
	if self.player:hasSkill("beifa") and self.player:getHandcardNum() == 1 and self:needKongcheng() then return true end
	if self.player:hasSkill("sijian") and self.player:getHandcardNum() == 1 then return true end
	return false
end


sgs.ai_skill_cardask["@neo2013longyin"] = function(self, data, pattern)
	local function SameCard(cd)
		if pattern == ".|red" then return cd:isRed() end
		return cd:isBlack()
	end
	local function getLeastValueCard()
		local offhorse_avail, weapon_avail
		for _, enemy in ipairs(self.enemies) do
			if self:canAttack(enemy, self.player) then
				if not offhorse_avail and self.player:getOffensiveHorse() and self.player:distanceTo(enemy, 1) <= self.player:getAttackRange() then
					offhorse_avail = true
				end
				if not weapon_avail and self.player:getWeapon() and self.player:distanceTo(enemy) == 1 then
					weapon_avail = true
				end
			end
			if offhorse_avail and weapon_avail then break end
		end
		if self:needToThrowArmor() and SameCard(self.player:getArmor()) then return "$" .. self.player:getArmor():getEffectiveId() end
		if self.player:getPhase() > sgs.Player_Play then
			local cards = sgs.QList2Table(self.player:getHandcards())
			self:sortByKeepValue(cards)
			for _, c in ipairs(cards) do
				if self:getKeepValue(c) < 8 and not self.player:isJilei(c) and not self:isValuableCard(c) and SameCard(c) then return "$" .. c:getEffectiveId() end
			end
			if offhorse_avail and not self.player:isJilei(self.player:getOffensiveHorse()) and SameCard(self.player:getOffensiveHorse()) then return "$" .. self.player:getOffensiveHorse():getEffectiveId() end
			if weapon_avail and not self.player:isJilei(self.player:getWeapon()) and self:evaluateWeapon(self.player:getWeapon()) < 5 and SameCard(self.player:getWeapon()) then return "$" .. self.player:getWeapon():getEffectiveId() end
		else
			local slashc
			local cards = sgs.QList2Table(self.player:getHandcards())
			self:sortByUseValue(cards, true)
			for _, c in ipairs(cards) do
				if self:getUseValue(c) < 6 and not self:isValuableCard(c) and not self.player:isJilei(c) and SameCard(c) --[[and]] then
					if self:getCardsNum("Slash") < 3 then
						if not isCard("Slash", c, self.player) then return "$" .. c:getEffectiveId() end
					else
						return "$" .. c:getEffectiveId()
					end
					return "."
				end
			end
			if offhorse_avail and not self.player:isJilei(self.player:getOffensiveHorse()) and SameCard(self.player:getOffensiveHorse()) then return "$" .. self.player:getOffensiveHorse():getEffectiveId() end
		end
	end
	local use = data:toCardUse()
	local slash = use.card
	local slash_num = 0
	if use.from:objectName() == self.player:objectName() then slash_num = self:getCardsNum("Slash") else slash_num = getCardsNum("Slash", use.from) end
	if self:isEnemy(use.from) and use.m_addHistory and not self:hasCrossbowEffect(use.from) and slash_num > 0 then return "." end
	if (slash) or (use.m_reason == sgs.CardUseStruct_CARD_USE_REASON_PLAY and use.m_addHistory and self:isFriend(use.from) and slash_num >= 1) then
		local str = getLeastValueCard()
		if str then return str end
	end
	return "."
end


local neo2013duoshi_skill = {}
neo2013duoshi_skill.name = "neo2013duoshi"
table.insert(sgs.ai_skills, neo2013duoshi_skill)
neo2013duoshi_skill.getTurnUseCard = function(self, inclusive)
	if self.player:usedTimes("NeoDuoshiAE") >= 4 or sgs.turncount <= 1 then return nil end
	local cards = sgs.QList2Table(self.player:getCards("h"))
	local red_card
	self:sortByUseValue(cards, true)

	for _, card in ipairs(cards) do
		if card:isRed() then
			local shouldUse = true
			if card:isKindOf("Slash") then
				local dummy_use = { isDummy = true }
				if self:getCardsNum("Slash") == 1 then
					self:useBasicCard(card, dummy_use)
					if dummy_use.card then shouldUse = false end
				end
			end

			if self:getUseValue(card) > 3.5 and card:isKindOf("TrickCard") then
				local dummy_use = { isDummy = true }
				self:useTrickCard(card, dummy_use)
				if dummy_use.card then shouldUse = false end
			end

			if shouldUse and not card:isKindOf("Peach") then
				red_card = card
				break
			end

		end
	end

	if red_card then
		local suit = red_card:getSuitString()
		local number = red_card:getNumberString()
		local card_id = red_card:getEffectiveId()
		local card_str = ("await_exhausted:neo2013duoshi[%s:%s]=%d"):format(suit, number, card_id)
		local await = sgs.Card_Parse(card_str)

		assert(await)

		return await
	end
end

sgs.ai_cardneed.neo2013danji = function(to)
	return to:getMark("neo2013danji") == 0 and to:getHandcardNum() <= to:getHp()
end

sgs.ai_skill_invoke.neo2013huwei = function(self)
	local NeoDrowning = sgs.Sanguosha:cloneCard("neo_drowning", sgs.Card_NoSuit, 0)
	local dummy_use = { isDummy = true }
	self:useTrickCard(NeoDrowning, dummy_use)
	if dummy_use.card and self:getAoeValue(NeoDrowning) > 0 then return true end
	return
end

sgs.ai_skill_discard.neo2013qingcheng = function(self, discard_num, min_num, optional, include_equip)
	if self.player:isNude() or self:needBear() then return {} end
	if self.room:alivePlayerCount() == 2 then
		local only_enemy = self.room:getOtherPlayers(self.player):first()
		if only_enemy:getLostHp() < 3 then return {} end
	end
	local invoke = false
	local from = self.room:getCurrent()
	if self:isFriend(from) then
		if from:hasSkills("shiyong|benghuai") then invoke = true end
	end
	if self:isEnemy(from) then
		local skills = from:getVisibleSkillList()
		if from:hasSkill("shiyong") and skills:length() == 1 then invoke = false end
		if skills:length() > 0 then invoke = true end
	end
	if invoke then
		if self:needKongcheng(self.player, true) and self.player:getHandcardNum() == 1 then return {self.player:handCards():first()} end
		local id = JS_Card(self)
		if id then return {id}  end
		return self:askForDiscard("dummyreason", 1, 1, true, true)
	end
	return {}
end
sgs.ai_skill_choice.neo2013qingcheng = sgs.ai_skill_choice.qingcheng
sgs.ai_choicemade_filter.skillChoice.neo2013qingcheng = sgs.ai_choicemade_filter.skillChoice.qingcheng


local neo2013xiechan_skill = {}
neo2013xiechan_skill.name = "neo2013xiechan"
table.insert(sgs.ai_skills, neo2013xiechan_skill)
neo2013xiechan_skill.getTurnUseCard = function(self)
	if self:needBear() or self.player:getMark("@neo2013xiechan") == 0 then return nil end
	if not self.player:hasUsed("Neo2013XiechanCard") and not self.player:isKongcheng() then return sgs.Card_Parse("@Neo2013XiechanCard=.") end
end

sgs.ai_skill_use_func.Neo2013XiechanCard = function(card,use,self)
	if sgs.turncount == 0 then return nil end
	self:sort(self.enemies, "hp")
	local max_card = self:getMaxCard()
	if self:isWeak() and max_card and not max_card:isKindOf("Peach") then
		for _, enemy in ipairs(self.enemies) do
			local emaxcard = self:getMaxCard(enemy)
			if not enemy:isKongcheng() and not self:doNotDiscard(enemy, "h") and not emaxcard and self:canAttack(enemy) then
				use.card = card
				if use.to then use.to:append(enemy) end
				return
			end
		end
		for _, enemy in ipairs(self.enemies) do
			local emaxcard = self:getMaxCard(enemy)
			if not enemy:isKongcheng() and not self:doNotDiscard(enemy, "h") and emaxcard and max_card:getNumber() > emaxcard:getNumber() and self:canAttack(enemy) then
				use.card = card
				if use.to then use.to:append(enemy) end
				return
			end
		end
	elseif not self:isWeak() then
		for _, enemy in ipairs(self.enemies) do
			local emaxcard = self:getMaxCard(enemy)
			if not enemy:isKongcheng() and not self:doNotDiscard(enemy, "h") and not emaxcard and self:canAttack(enemy) and (self:isWeak(enemy) or enemy:getHp() < 3) then
				use.card = card
				if use.to then use.to:append(enemy) end
				return
			end
		end
		for _, enemy in ipairs(self.enemies) do
			if enemy:hasFlag("AI_HuangtianPindian") and enemy:getHandcardNum() == 1 and self:canAttack(enemy) then
				use.card = card
				if use.to then
					use.to:append(enemy)
					enemy:setFlags("-AI_HuangtianPindian")
				end
				return
			end
		end
		for _, enemy in ipairs(self.enemies) do
			if not enemy:isKongcheng() and not self:doNotDiscard(enemy, "h") and self:canAttack(enemy) and (self:isWeak(enemy) or enemy:getHp() < 3) then
				use.card = card
				if use.to then use.to:append(enemy) end
				return
			end
		end
		for _, enemy in ipairs(self.enemies) do
			if not enemy:isKongcheng() and self:canAttack(enemy) and (self:isWeak(enemy) or enemy:getHp() < 3) then
				use.card = card
				if use.to then use.to:append(enemy) end
				return
			end
		end
		for _, enemy in ipairs(self.enemies) do
			if not enemy:isKongcheng() and self:canAttack(enemy) and enemy:isWounded() then
				use.card = card
				if use.to then use.to:append(enemy) end
				return
			end
		end
		for _, enemy in ipairs(self.enemies) do
			if not enemy:isKongcheng() and self:isWeak(enemy) then
				use.card = card
				if use.to then use.to:append(enemy) end
				return
			end
		end
		for _, enemy in ipairs(self.enemies) do
			if enemy:hasFlag("AI_HuangtianPindian") and enemy:getHandcardNum() == 1 then
				use.card = card
				if use.to then
					use.to:append(enemy)
					enemy:setFlags("-AI_HuangtianPindian")
				end
				return
			end
		end
	end
	return nil
end
sgs.dynamic_value.damage_card.Neo2013XiechanCard = true
sgs.ai_use_value.Neo2013XiechanCard = 5
sgs.ai_use_priority.Neo2013XiechanCard = sgs.ai_use_priority.Slash + 0.1
sgs.ai_card_intention.Neo2013XiechanCard = sgs.ai_card_intention.Slash


sgs.ai_skill_invoke.neo2013chengxiang = sgs.ai_skill_invoke.chengxiang
sgs.ai_skill_askforag.neo2013chengxiang = sgs.ai_skill_askforag.chengxiang

local neo2013tongwu_skill = {}
neo2013tongwu_skill.name = "neo2013tongwu"
table.insert(sgs.ai_skills, neo2013tongwu_skill)
neo2013tongwu_skill.getTurnUseCard = function(self, inclusive)
	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)

	local red_card
	self:sortByUseValue(cards, true)
	for _, card in ipairs(cards) do
		if not card:isKindOf("Slash")
			and not isCard("Peach", card, self.player) and not isCard("ExNihilo", card, self.player)
			and card:isNDTrick() and (self:getUseValue(card) < sgs.ai_use_value.Slash or inclusive or sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, self.player, sgs.Sanguosha:cloneCard("slash")) > 0) then
			red_card = card
			break
		end
	end

	if red_card then
		local suit = red_card:getSuitString()
		local number = red_card:getNumberString()
		local card_id = red_card:getEffectiveId()
		local card_str = ("slash:neo2013tongwu[%s:%s]=%d"):format(suit, number, card_id)
		local slash = sgs.Card_Parse(card_str)

		assert(slash)
		return slash
	end
end

sgs.ai_view_as.neo2013tongwu = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place ~= sgs.Player_PlaceSpecial and card:isNDTrick() and not card:isKindOf("Peach") and not card:hasFlag("using") and sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE then
		return ("slash:neo2013tongwu[%s:%s]=%d"):format(suit, number, card_id)
	end
end

sgs.ai_skill_invoke.neo2013tongwu = function(self, data)
	local tc = data:toCard()
	local dummy_use = { isDummy = true }
	self:useTrickCard(tc, dummy_use)
	if dummy_use.card then return true end
	return
end

neo2013xiongyi_skill = {}
neo2013xiongyi_skill.name = "neo2013xiongyi"
table.insert(sgs.ai_skills, neo2013xiongyi_skill)
neo2013xiongyi_skill.getTurnUseCard = function(self)
	if self.player:getMark("@arise") < 1 then return end
	if (#self.friends <= #self.enemies and sgs.turncount > 2 and self.player:getLostHp() > 0) or (sgs.turncount > 1 and self:isWeak()) then
		return sgs.Card_Parse("Neo2013XiongyiCard=.")
	end
end
sgs.ai_skill_use_func.Neo2013XiongyiCard = sgs.ai_skill_use_func.XiongyiCard

sgs.ai_card_intention.Neo2013XiongyiCard = sgs.ai_card_intention.XiongyiCard
sgs.ai_use_priority.Neo2013XiongyiCard = sgs.ai_use_priority.XiongyiCard

sgs.ai_skill_discard.neo2013qijun = function(self, discard_num, min_num, optional, include_equip)
	local from = self.room:getCurrent()
	if self.player:isNude() or self:needBear() or self:isWeak() or from:getMark("qijun") > 0 then return {} end
	local idtable = {JS_Card(self)} or self:askForDiscard("dummyreason", 1, 1, true, true)
	if self.room:alivePlayerCount() == 2 then
		local only_enemy = self.room:getOtherPlayers(self.player):first()
		if only_enemy:canSlash(self.player) then return idtable end
	end
	if self:isFriend(from) then return idtable end
	return {}
end

local neo2013zhoufu_skill = {} --直接根据咒缚
neo2013zhoufu_skill.name = "neo2013zhoufu"
table.insert(sgs.ai_skills, neo2013zhoufu_skill)
neo2013zhoufu_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("Neo2013ZhoufuCard") or self.player:isKongcheng() or self:getOverflow() <= 0 then return end
	return sgs.Card_Parse("@Neo2013ZhoufuCard=.")
end

sgs.ai_skill_use_func.Neo2013ZhoufuCard = function(card, use, self)
	local cards = {}
	for _, card in sgs.qlist(self.player:getHandcards()) do
		table.insert(cards, sgs.Sanguosha:getEngineCard(card:getEffectiveId()))
	end
	self:sortByKeepValue(cards)
	self:sort(self.friends_noself)
	local zhenji
	for _, friend in ipairs(self.friends_noself) do
		local reason = getNextJudgeReason(self, friend)
		if reason then
			if reason == "luoshen" then
				zhenji = friend
			elseif reason == "indulgence" then
				for _, card in ipairs(cards) do
					if card:getSuit() == sgs.Card_Heart or (friend:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade)
						and (friend:hasSkill("tiandu") or not self:isValuableCard(card)) then
						use.card = sgs.Card_Parse("@Neo2013ZhoufuCard=" .. card:getEffectiveId())
						if use.to then use.to:append(friend) end
						return
					end
				end
			elseif reason == "supply_shortage" then
				for _, card in ipairs(cards) do
					if card:getSuit() == sgs.Card_Club and (friend:hasSkill("tiandu") or not self:isValuableCard(card)) then
						use.card = sgs.Card_Parse("@Neo2013ZhoufuCard=" .. card:getEffectiveId())
						if use.to then use.to:append(friend) end
						return
					end
				end
			elseif reason == "lightning" and not friend:hasSkills("hongyan|wuyan") then
				for _, card in ipairs(cards) do
					if (card:getSuit() ~= sgs.Card_Spade or card:getNumber() == 1 or card:getNumber() > 9)
						and (friend:hasSkill("tiandu") or not self:isValuableCard(card)) then
						use.card = sgs.Card_Parse("@Neo2013ZhoufuCard=" .. card:getEffectiveId())
						if use.to then use.to:append(friend) end
						return
					end
				end
			end
		end
	end
	if zhenji then
		for _, card in ipairs(cards) do
			if card:isBlack() and not (zhenji:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade) then
				use.card = sgs.Card_Parse("@Neo2013ZhoufuCard=" .. card:getEffectiveId())
				if use.to then use.to:append(zhenji) end
				return
			end
		end
	end
	self:sort(self.enemies)
	for _, enemy in ipairs(self.enemies) do
		local reason = getNextJudgeReason(self, enemy)
		if not enemy:hasSkill("tiandu") and reason then
			if reason == "indulgence" then
				for _, card in ipairs(cards) do
					if not (card:getSuit() == sgs.Card_Heart or (enemy:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade))
						and not self:isValuableCard(card) then
						use.card = sgs.Card_Parse("@Neo2013ZhoufuCard=" .. card:getEffectiveId())
						if use.to then use.to:append(enemy) end
						return
					end
				end
			elseif reason == "supply_shortage" then
				for _, card in ipairs(cards) do
					if not card:getSuit() == sgs.Card_Club and not self:isValuableCard(card) then
						use.card = sgs.Card_Parse("@Neo2013ZhoufuCard=" .. card:getEffectiveId())
						if use.to then use.to:append(enemy) end
						return
					end
				end
			elseif reason == "lightning" and not enemy:hasSkills("hongyan|wuyan") then
				for _, card in ipairs(cards) do
					if card:getSuit() == sgs.Card_Spade and card:getNumber() >= 2 and card:getNumber() <= 9 then
						use.card = sgs.Card_Parse("@Neo2013ZhoufuCard=" .. card:getEffectiveId())
						if use.to then use.to:append(enemy) end
						return
					end
				end
			end
		end
	end
end

sgs.ai_card_intention.Neo2013ZhoufuCard = 0
sgs.ai_use_value.Neo2013ZhoufuCard = 2
sgs.ai_use_priority.Neo2013ZhoufuCard = 1

sgs.ai_skill_invoke.neo2013kunxiang = function(self)
	local ts = sgs.Sanguosha:getTriggerSkill("neo2013kunxiang")
	local yz = self.room:findPlayerBySkillName("neo2013kunxiang")
	if ts:triggerable(self.player) then
		if #self.enemies > 2 then return false end
		if self:willSkipPlayPhase() then return true end
		if self.player:getHandcardNum() > 6 then return false end
		if self:needBear() or self:doNotDiscard() then return true end
		return (self.player:hasSkills(sgs.lose_equip_skill) and not self.player:getEquips():isEmpty()) or self:needToThrowArmor()
	else
		if self:isFriend(yz) then return true end
		if self:isEnemy(yz) then
			if self:doNotDiscard(yz) then return false end
			return true
		end
	end
	return
end


sgs.ai_skill_choice.neo2013kunxiang = function(self, choices)
	local yz = self.room:findPlayerBySkillName("neo2013kunxiang")
	if not yz or self:isEnemy(yz) then return "dismiss" end
	if self:needKongcheng(yz, true) then return "dismiss" end
	return "draw"
end


function sgs.ai_skill_invoke.neo2013zongxuan(self, data)
	local move = data:toMoveOneTime()
	local id = move.card_ids:at(0)
	local card = sgs.Sanguosha:getCard(id)
	if self:needToThrowArmor()  then return true end
	local hcards = sgs.QList2Table(self.player:getCards("h"))
	local ecards = sgs.QList2Table(self.player:getCards("e"))
	self:sortByCardNeed(ecards)
	self:sortByCardNeed(hcards)
	if self:hasSkills(sgs.lose_equip_skill) and not self.player:getEquips():isEmpty() then
		for _, ecard in ipairs(ecards) do
			if ecard then return true end
		end
	end
	if card:isKindOf("Slash") and not self:slashIsAvailable() then return false end
	for _, hcard in ipairs(hcards) do
		if self:getUseValue(hcard) < self:getUseValue(card) and not self:isValuableCard(hcard) then return true end
	end
	return
end
sgs.ai_skill_discard.neo2013zongxuan = function(self, discard_num, min_num, optional, include_equip)
	if self:needToThrowArmor()  then return {self.player:getArmor():getEffectiveId()} end
	local hcards = sgs.QList2Table(self.player:getCards("h"))
	local ecards = sgs.QList2Table(self.player:getCards("e"))
	self:sortByCardNeed(ecards)
	self:sortByCardNeed(hcards)
	if self:hasSkills(sgs.lose_equip_skill) and not self.player:getEquips():isEmpty() then
		for _, ecard in ipairs(ecards) do
			if ecard then return {ecard:getEffectiveId()} end
		end
	end
	if self.player:isNude() or self:needBear() or self:isWeak() then return {} end
	local idtable = {JS_Card(self)} or self:askForDiscard("dummyreason", 1, 1, true, true)
	return idtable
end

sgs.ai_skill_choice.neo2013zongxuan = function(self, choices, data)
	local card = data:toCard()
	local from = self.room:getCurrent()
	local near = from:getNextAlive()
	if not near then return "zongxuanup" end
	if self:isFriend(near) then
		if self:getUseValue(card) > 6 or card:isKindOf("Peach") or self:isValuableCard(card) then return "zongxuanup" end
		if near:containsTrick("indulgence") and not near:containsTrick("YanxiaoCard")
			and card:getSuit() == sgs.Card_Heart then return "zongxuanup" end
		return "zongxuandown"
	elseif self:isEnemy(near) then
		if self:getUseValue(card) < 6 or not self:isValuableCard(card, near) then return "zongxuanup" end
		if near:containsTrick("indulgence") and not near:containsTrick("YanxiaoCard")
			and card:getSuit() == sgs.Card_Heart then return "zongxuandown" end
		return "zongxuandown"
	end
end
