
sgs.ai_skill_choice.heg_nullification = function(self, choice, data)
	local effect = data:toCardEffect()
	if effect.card:isKindOf("AOE") or effect.card:isKindOf("GlobalEffect") then
		if self:isFriendWith(effect.to) then return "all"
		elseif self:isFriend(effect.to) then return "single"
		elseif self:isEnemy(effect.to) then return "all"
		end
	end
	local targets = sgs.SPlayerList()
	local players = self.room:getTag("targets" .. effect.card:toString()):toList()
	for _, q in sgs.qlist(players) do
		targets:append(q:toPlayer())
	end
	if effect.card:isKindOf("FightTogether") then
		local ed, no = 0
		for _, p in sgs.qlist(targets) do
			if p:objectName() ~= targets:at(0):objectName() and p:isChained() then
				ed = ed + 1
			end
			if p:objectName() ~= targets:at(0):objectName() and not p:isChained() then
				no = no + 1
			end
		end
		if targets:at(0):isChained() then
			if no > ed then return "single" end
		else
			if ed > no then return "single" end
		end
	end
	return "all"
end



sgs.ai_nullification.ArcheryAttack = function(self, card, from, to, positive, keep)
	local targets = sgs.SPlayerList()
	local players = self.room:getTag("targets" .. card:toString()):toList()
	for _, q in sgs.qlist(players) do
		targets:append(q:toPlayer())
	end

	if positive then
		if self:isFriend(to) then
			if keep then
				for _, p in sgs.qlist(targets) do
					if self:isFriend(p) and self:aoeIsEffective(card, p, from)
						and not self:hasEightDiagramEffect(p) and self:getDamagedEffects(p, from) and self:isWeak(p)
						and getKnownCard(p, self.player, "Jink", true, "he") == 0 then
						keep = false
					end
				end
			end
			if keep then return false end

			local heg_null_card = self:getCard("HegNullification") or (self.room:getTag("NullifyingTimes"):toInt() > 0 and self.room:getTag("NullificatonType"):toBool())
			if heg_null_card and self:aoeIsEffective(card, to, from) then
				targets:removeOne(to)
				for _, p in sgs.qlist(targets) do
					if to:isFriendWith(p) and self:aoeIsEffective(card, p, from) then return true, false end
				end
			end

			if not self:aoeIsEffective(card, to, from) then
				return
			elseif self:getDamagedEffects(to, from) then
				return
			elseif to:objectName() == self.player:objectName() and self:canAvoidAOE(card) then
				return
			elseif (getKnownCard(to, self.player, "Jink", true, "he") >= 1 or self:hasEightDiagramEffect(to)) and to:getHp() > 1 then
				return
			elseif not self:isFriendWith(to) and self:playerGetRound(to) < self:playerGetRound(self.player) and self:isWeak() then
				return
			else
				return true, true
			end
		end
	else
		if not self:isFriend(from) or not(self:isEnemy(to) and from:isFriendWith(to)) then return false end
		if keep then
			for _, p in sgs.qlist(targets) do
				if self:isEnemy(p) and self:aoeIsEffective(card, p, from)
					and not self:hasEightDiagramEffect(p) and self:getDamagedEffects(p, from) and self:isWeak(p)
					and getKnownCard(p, self.player, "Jink", true, "he") == 0 then
					keep = false
				end
			end
		end
		if keep or not self:isEnemy(to) then return false end
		local nulltype = self.room:getTag("NullificatonType"):toBool()
		if nulltype then
			targets:removeOne(to)
			local num = 0
			local weak
			for _, p in sgs.qlist(targets) do
				if to:isFriendWith(p) and self:aoeIsEffective(card, p, from) then
					num = num + 1
				end
				if self:isWeak(to) or self:isWeak(p) then
					weak = true
				end
			end
			return num > 1 or weak, true
		else
			if self:isWeak(to) then return true, true end
		end		
	end
	return
end

sgs.ai_nullification.SavageAssault = function(self, card, from, to, positive, keep)
	local targets = sgs.SPlayerList()
	local players = self.room:getTag("targets" .. card:toString()):toList()
	for _, q in sgs.qlist(players) do
		targets:append(q:toPlayer())
	end

	if positive then
		if self:isFriend(to) then
			local menghuo = nil --sgs.findPlayerByShownSkillName("huoshou")
			local zhurong = nil --sgs.findPlayerByShownSkillName("juxiang")
			if menghuo then targets:removeOne(menghuo) end
			if zhurong then targets:removeOne(zhurong) end

			if keep then
				for _, p in sgs.qlist(targets) do
					if self:isFriend(p) and self:aoeIsEffective(card, p, menghuo or from)
						and self:getDamagedEffects(p, menghuo or from) and self:isWeak(p)
						and getKnownCard(p, self.player, "Slash", true, "he") == 0 then
						keep = false
					end
				end
			end
			if keep then return false end

			local heg_null_card = self:getCard("HegNullification") or (self.room:getTag("NullifyingTimes"):toInt() > 0 and self.room:getTag("NullificatonType"):toBool())
			if heg_null_card and self:aoeIsEffective(card, to, menghuo or from) then
				targets:removeOne(to)
				for _, p in sgs.qlist(targets) do
					if to:isFriendWith(p) and self:aoeIsEffective(card, p, menghuo or from) then return true, false end
				end
			end

			if not self:aoeIsEffective(card, to, menghuo or from) then
				return
			elseif self:getDamagedEffects(to, menghuo or from) then
				return
			elseif to:objectName() == self.player:objectName() and self:canAvoidAOE(card) then
				return
			elseif getKnownCard(to, self.player, "Slash", true, "he") >= 1 and to:getHp() > 1 then
				return
			elseif not self:isFriendWith(to) and self:playerGetRound(to) < self:playerGetRound(self.player) and self:isWeak() then
				return
			else
				return true, true
			end
		end
	else
		if not self:isFriend(from) or not(self:isEnemy(to) and from:isFriendWith(to)) then return false end
		if keep then
			for _, p in sgs.qlist(targets) do
				if self:isEnemy(p) and self:aoeIsEffective(card, p, menghuo or from)
					and self:getDamagedEffects(p, menghuo or from) and self:isWeak(p)
					and getKnownCard(p, self.player, "Slash", true, "he") == 0 then
					keep = false
				end
			end
		end
		if keep or not self:isEnemy(to) then return false end
		local nulltype = self.room:getTag("NullificatonType"):toBool()
		if nulltype then
			targets:removeOne(to)
			local num = 0
			local weak
			for _, p in sgs.qlist(targets) do
				if to:isFriendWith(p) and self:aoeIsEffective(card, p, menghuo or from) then
					num = num + 1
				end
				if self:isWeak(to) or self:isWeak(p) then
					weak = true
				end
			end
			return num > 1 or weak, true
		else
			if self:isWeak(to) then return true, true end
		end		
	end
	return
end



function SmartAI:useCardAwaitExhaustedHegemony(AwaitExhausted, use)
	if not AwaitExhausted:isAvailable(self.player) then return end
	use.card = AwaitExhausted
	return
end
sgs.ai_use_priority.AwaitExhaustedHegemony = 2.8
sgs.ai_use_value.AwaitExhaustedHegemony = 4.9
sgs.ai_keep_value.AwaitExhaustedHegemony = 1
sgs.ai_card_intention.AwaitExhaustedHegemony = function(self, card, from, tos)
	for _, to in ipairs(tos) do
		sgs.updateIntention(from, to, -50)
	end
end
sgs.ai_nullification.AwaitExhaustedHegemony = function(self, card, from, to, positive, keep)
	local targets = sgs.SPlayerList()
	local players = self.room:getTag("targets" .. card:toString()):toList()
	for _, q in sgs.qlist(players) do
		targets:append(q:toPlayer())
	end
	if keep then return false end
	if positive then
		local hegnull = self:getCard("HegNullification") or (self.room:getTag("NullifyingTimes"):toInt() > 0 and self.room:getTag("NullificatonType"):toBool())
		if self:isEnemy(to) and targets:length() > 1 and hegnull then
			for _, p in sgs.qlist(targets) do
				if (p:hasShownSkills(sgs.lose_equip_skill) and p:getEquips():length() > 0)
					or (p:getArmor() and self:needToThrowArmor(p)) then
					return true, false
				end
			end
		else
			if self:isEnemy(to) and self:evaluateKingdom(to) ~= "unknown" then
				--if self:getOverflow() > 0 or self:getCardsNum("Nullification") > 1 then return true, true end
				if to:hasShownSkills(sgs.lose_equip_skill) and to:getEquips():length() > 0 then return true, true end
				if to:getArmor() and self:needToThrowArmor(to) then return true, true end
			end
		end
	else
		if self:isFriend(to) and (self:getOverflow() > 0 or self:getCardsNum("Nullification") > 1) then return true, true end
	end
	return
end



function SmartAI:useCardBefriendAttacking(BefriendAttacking, use)
	if not BefriendAttacking:isAvailable(self.player) then return end
	local targets = sgs.PlayerList()
	local players = sgs.QList2Table(self.room:getOtherPlayers(self.player))
	self:sort(players)
	for _, to_select in ipairs(players) do
		if to_select:hasShownSkill("luanji") and to_select:getHandcardNum() > 3 then continue end
		if self:isFriend(to_select) and BefriendAttacking:targetFilter(targets, to_select, self.player) and not targets:contains(to_select)
			and self:hasTrickEffective(BefriendAttacking, to_select, self.player) then
			targets:append(to_select)
			if use.to then use.to:append(to_select) end
		end
	end

	if targets:isEmpty() then															--对野心家使用
		for _, to_select in ipairs(players) do
			--if to_select:hasShownSkill("luanji") and to_select:getHandcardNum() > 3 then continue end
			if BefriendAttacking:targetFilter(targets, to_select, self.player) and not targets:contains(to_select)
				and self:hasTrickEffective(BefriendAttacking, to_select, self.player) and to_select:getRole() == "careerist" then
				targets:append(to_select)
				if use.to then use.to:append(to_select) end
			end
		end
	end

	local kingdoms = {"wei","shu","wu","qun"}											--计算人数最少的势力
	if self.player:getRole() ~= "careerist" then
		table.removeOne(kingdoms, self.player:getRole())--self.player:getKingdom()
	end
	local num = 10
	local targetKingdom
	for _, kingdom in ipairs(kingdoms) do
		if self.player:getPlayerNumWithSameKingdom("ai", kingdom) < num and self.player:getPlayerNumWithSameKingdom("ai", kingdom) > 0 then
			num = self.player:getPlayerNumWithSameKingdom("ai", kingdom)
			targetKingdom = kingdom
		end
	end

	if targets:isEmpty() then															--对该势力使用
		for _, to_select in ipairs(players) do
			--if to_select:hasShownSkill("luanji") and to_select:getHandcardNum() > 3 then continue end
			if BefriendAttacking:targetFilter(targets, to_select, self.player) and not targets:contains(to_select)
				and self:hasTrickEffective(BefriendAttacking, to_select, self.player) and to_select:getRole() == targetKingdom then
				targets:append(to_select)
				if use.to then use.to:append(to_select) end
			end
		end
	end
	if targets:isEmpty() then
		for _, to_select in ipairs(players) do
			--if to_select:hasShownSkill("luanji") and to_select:getHandcardNum() > 3 then continue end
			if BefriendAttacking:targetFilter(targets, to_select, self.player) and not targets:contains(to_select)
				and self:hasTrickEffective(BefriendAttacking, to_select, self.player) then
				targets:append(to_select)
				if use.to then use.to:append(to_select) end
			end
		end
	end
	if not targets:isEmpty() then
		use.card = BefriendAttacking
		return
	end
end
sgs.ai_use_priority.BefriendAttacking = 9.28
sgs.ai_use_value.BefriendAttacking = 8.9
sgs.ai_keep_value.BefriendAttacking = 3.88


sgs.ai_nullification.BefriendAttacking = function(self, card, from, to, positive, keep)
	if keep then return false end
	if positive then
		if not self:isFriend(to) and self:isEnemy(from) and self:isWeak(from) then return true, true end
	else
		if self:isFriend(from) then return true, true end
	end
	return
end

sgs.ai_skill_invoke.DoubleSwordHegemony = function(self, data)
	local FirstShowRewarded = self.room:getTag("fahua_use"):toBool()
	return FirstShowRewarded
end

sgs.ai_skill_choice.DoubleSwordHegemony = function(self, choices, data)
	local choice_table = choices:split("+")
	return choice_table[math.random(1, #choice_table)]
end

sgs.weapon_range.DoubleSwordHegemony = 2
sgs.weapon_range.SixSwords = 2


function SmartAI:useCardKnownBothHegemony(KnownBoth, use)
	self.knownboth_choice = {}
	if not KnownBoth:isAvailable(self.player) then return false end
	local targets = sgs.PlayerList()
	local total_num = 1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, KnownBoth)
	for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if KnownBoth:targetFilter(targets, player, self.player) and sgs.isAnjiang(player) and not targets:contains(player)
			and player:getMark(("KnownBoth_%s_%s"):format(self.player:objectName(), player:objectName())) == 0 and self:hasTrickEffective(KnownBoth, player, self.player) then
			use.card = KnownBoth
			targets:append(player)
			if use.to then use.to:append(player) end
			self.knownboth_choice[player:objectName()] = "showhead"
		end
	end

	if total_num > targets:length() then
		self:sort(self.enemies, "handcard")
		sgs.reverse(self.enemies)
		for _, enemy in ipairs(self.enemies) do
			if KnownBoth:targetFilter(targets, enemy, self.player) and enemy:getHandcardNum() - self:getKnownNum(enemy, self.player) > 3 and not targets:contains(enemy)
				and self:hasTrickEffective(KnownBoth, enemy, self.player) then
				use.card = KnownBoth
				targets:append(enemy)
				if use.to then use.to:append(enemy) end
				self.knownboth_choice[enemy:objectName()] = "showcard"
			end
		end
	end
	if total_num > targets:length() and not targets:isEmpty() then
		self:sort(self.friends_noself, "handcard")
		self.friends_noself = sgs.reverse(self.friends_noself)
		for _, friend in ipairs(self.friends_noself) do
			if self:getKnownNum(friend, self.player) ~= friend:getHandcardNum() and card:targetFilter(targets, friend, self.player) and not targets:contains(friend)
				and self:hasTrickEffective(KnownBoth, friend, self.player) then
				targets:append(friend)
				if use.to then use.to:append(friend) end
				self.knownboth_choice[friend:objectName()] = "showcard"
			end
		end
	end

	if not use.card then
		targets = sgs.PlayerList()
		local canRecast = KnownBoth:targetsFeasible(targets, self.player)
		if canRecast then
			use.card = KnownBoth
			if use.to then use.to = sgs.SPlayerList() end
		end
	end
end


sgs.ai_skill_choice.known_both_hegemony = function(self, choices, data)
	local target = data:toPlayer()
	if target and self.knownboth_choice and self.knownboth_choice[target:objectName()] then return self.knownboth_choice[target:objectName()] end
	return "showcard"
end

sgs.ai_use_priority.KnownBothHegemony = 9.1
sgs.ai_use_value.KnownBothHegemony = 5.5
sgs.ai_keep_value.KnownBothHegemony = 3.33

sgs.ai_choicemade_filter.skillChoice.known_both_hegemony = function(self, from, promptlist)  --function(self, player, args, data)
	local choice = promptlist[#promptlist]
	if choice ~= "showcard" then
		for _, to in sgs.qlist(self.room:getOtherPlayers(from)) do
			if to:hasFlag("KnownBothTarget") then
				to:setMark(("KnownBoth_%s_%s"):format(from:objectName(), to:objectName()), 1)
				local names = {}
				if from:getTag("KnownBoth_" .. to:objectName()):toString() ~= "" then
					names = from:getTag("KnownBoth_" .. to:objectName()):toString():split("+")
				else
					if to:hasShownGeneral() then
						table.insert(names, to:getGeneralName()) --getActualGeneralName
					else
						table.insert(names, "anjiang")
					end
					if to:hasShownGeneral2() then
						table.insert(names, to:getGeneral2Name())
					else
						table.insert(names, "anjiang")
					end
				end
				local generals = self.room:getTag(to:objectName()).toList();
				
				if choice == "showhead" then
					names[1] = generals:first()
				else
					names[2] = generals:last()
				end
				from:setTag("KnownBoth_" .. to:objectName(), sgs.QVariant(table.concat(names, "+")))
				break
			end
		end
	end
end
