
function SmartAI:useCardAwaitExhausted(card, use)
	if #self.friends_noself == 0 and not self:isWeak() then return end
	if self.player:getCardCount(false) <= 2 or self:needBear() then return end
	use.card = card
	for _, player in ipairs(self.friends_noself) do
		if use.to and not player:hasSkill("manjuan") and not self.room:isProhibited(self.player, player, card) then
			use.to:append(player)
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if use.to and enemy:hasSkill("manjuan") and not self.room:isProhibited(self.player, enemy, card) then
			use.to:append(enemy)
		end
	end
end
sgs.ai_use_value.AwaitExhausted = sgs.ai_use_value.DuoshiCard
sgs.ai_use_priority.AwaitExhausted = sgs.ai_use_priority.DuoshiCard
sgs.ai_card_intention.AwaitExhausted = sgs.ai_card_intention.DuoshiCard

function SmartAI:useCardBefriendAttacking(card, use)
	local targets_num = 1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, card)
	local target = sgs.SPlayerList()
	self:sort(self.friends_noself, "defense")
	self:sort(self.enemies, "threat")
	local distance = 0
	local siblings = self.room:getOtherPlayers(self.player)
	for _, p in sgs.list(siblings) do
		local dist = self.player:distanceTo(p)
		if (dist > distance) then distance = dist end
	end
	for _, friend in ipairs(self.friends_noself) do
		if not self.room:isProhibited(self.player, friend, card) and self:hasTrickEffective(card, friend) and
			self.player:distanceTo(friend) == distance then target:append(friend) end
		if target:length() >= targets_num then break end
	end
	if target:length() == 0 then
	for _, en in ipairs(self.enemies) do
		if not self.room:isProhibited(self.player, en, card) and self:hasTrickEffective(card, en) and
			self.player:distanceTo(en) == distance and en:hasSkill("manjuan") then target:append(en) break end
		if not self.room:isProhibited(self.player, en, card) and self:hasTrickEffective(card, en) and
			self.player:distanceTo(en) == distance then target:append(en) break end
	end
	end
	if target:length() > 0 then
		use.card = card
		if use.to then use.to = target end
		return
	end
end
sgs.ai_card_intention.BefriendAttacking = -50
sgs.ai_use_value.BefriendAttacking = 7.8
sgs.ai_use_priority.BefriendAttacking = 4.35

function SmartAI:useCardKnownBoth(card, use)
	use.card = card
	return
end
sgs.ai_use_priority.KnownBoth = 8
sgs.ai_use_value.KnownBoth = sgs.ai_use_value.AmazingGrace - 1


sgs.weapon_range.SixSwords = 2
sgs.weapon_range.Triblade = 3
sgs.weapon_range.DragonPhoenix = 2

sgs.ai_skill_use["@@SixSwords"] = function(self, prompt)
	local targets = {}
	self:sort(self.friends_noself, "defense")
	for _, friend in ipairs(self.friends_noself) do table.insert(targets, friend:objectName()) end
	if #targets == 0 then return "." else return "@SixSwordsCard=.->" .. table.concat(targets, "+") end
end

sgs.ai_skill_use["@@Triblade"]=function(self,prompt)

	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)
	self:sortByCardNeed(cards)
	local cdid
	local tar
	local lightnings = self:getCards("Lightning", "h")
	for _,lightning in ipairs(lightnings) do
		if lightning and not self:willUseLightning(lightning) then cdid = lightning:getEffectiveId() break end
	end
	if not cdid and self.player:getHandcardNum() >= self.player:getHp() - 1 then
		for _,card in ipairs(cards) do
			if not isCard("Peach", card, self.player) and not isCard("Analeptic", card, self.player) then cdid = card:getEffectiveId() break end
		end
	end
	if not cdid or self.player:isKongcheng() then return "." end
	self:sort(self.enemies, "hp")
	self:sort(self.friends, "threat")
	for _,e in ipairs(self.enemies) do
		if e:hasFlag("TribladeFilter") and not self:getDamagedEffects(e, self.player) and self:canAttack(e) then tar = e break end
	end
	if not tar then
		for _,f in ipairs(self.friends) do
			if f:hasFlag("TribladeFilter") and self:getDamagedEffects(f, self.player) and not self:isWeak(f) then tar = f break end
		end
	end
	if tar then return "@TribladeCard="..cdid.."->"..tar:objectName() end
	return "."
end
sgs.ai_card_intention.TribladeCard = 30


sgs.ai_skill_invoke.DragonPhoenix = function(self, data)
	local use = data:toCardUse()
	if use then
		for _,to in sgs.qlist(use.to) do
			if self:isFriend(to) and self:doNotDiscard(to) then return true end
			if self:isEnemy(to) and not self:doNotDiscard(to) then return true end
		end
		return false
	end
	return false
end

function SmartAI:useCardDrowning(card, use)
	if self.player:hasSkill("noswuyan") or (self.player:hasSkill("wuyan") and not self.player:hasSkill("jueqing")) then return end
	self:sort(self.enemies)
	local targets, equip_enemy = {}, {}
	for _, enemy in ipairs(self.enemies) do
		if (not use.current_targets or not table.contains(use.current_targets, enemy:objectName()))
			and self:hasTrickEffective(card, enemy) and self:damageIsEffective(enemy) and self:canAttack(enemy)
			and not self:getDamagedEffects(enemy, self.player) and not self:needToLoseHp(enemy, self.player) then
			if enemy:hasEquip() then table.insert(equip_enemy, enemy)
			else table.insert(targets, enemy)
			end
		end
	end
	if not (self.player:hasSkill("wumou") and self.player:getMark("@wrath") < 7) then
		if #equip_enemy > 0 then
			local function cmp(a, b)
				return a:getEquips():length() >= b:getEquips():length()
			end
			table.sort(equip_enemy, cmp)
			for _, enemy in ipairs(equip_enemy) do
				if not self:needToThrowArmor(enemy) then table.insert(targets, enemy) end
			end
		end
		for _, friend in ipairs(self.friends_noself) do
			if not (not use.current_targets or not table.contains(use.current_targets, friend:objectName())) and self:needToThrowArmor(friend) then
				table.insert(targets, friend)
			end
		end
	end
	if #targets > 0 then
		local targets_num = 1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, card)
		local lx = self.room:findPlayerBySkillName("huangen")
		use.card = card
		if use.to then
			for i = 1, targets_num, 1 do
				if not (use.to:length() > 0 and targets[i]:hasSkill("danlao"))
					and not (use.to:length() > 0 and lx and self:isFriend(lx, targets[i]) and self:isEnemy(lx) and lx:getHp() > targets_num / 2) then
					use.to:append(targets[i])
					if #targets == i then break end
				end
			end
		end
	end
end

sgs.ai_skill_choice.neo_drowning = sgs.ai_skill_choice.drowning