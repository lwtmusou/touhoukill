
-- 【缮形】当一名角色失去装备区里的牌后，你可以令其摸一张牌，然后其可以交给你一张手牌。
-- 思路：对队友发动，然后如果你处于回合内弃牌阶段前，队友给usevalue最高的牌，否则给keepvalue最高的牌
sgs.ai_skill_invoke.shanxing = function(self, data)
	return self:isFriend(data:toPlayer())
end
sgs.ai_choicemade_filter.skillInvoke.shanxing = function(self, player, args, data)
	sgs.updateIntention(data:toPlayer(), player, -20)
end
sgs.ai_skill_discard.shanxing = function(self)
	local target = self.player:getTag("shanxing"):toPlayer()
	if self:isFriend(target) and not self.player:isCurrent() then
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(cards)
		if target:isCurrent() then
			local targetphase = target:getPhases()
			local targetCurrentPhase = target:getPhasesIndex()
			if targetphase.at(targetCurrentPhase) <= sgs.Player_Play and not self:willSkipPlayPhase(target) then
				self:sortByUseValue(cards)
			end
		end
		return cards[#cards]:getId()
	end

	return {}
end

-- 灵守: 结束阶段开始时，你可以观看一名其他角色的手牌并展示其中一张牌，令其选择将此牌和其装备区里所有同花色的牌当【杀】使用或重铸之。
-- 思路：找个牌最多的拆个桃子啥的
sgs.ai_skill_playerchosen.lingshou = function(self, targets)
	local n = {}
	for _, p in sgs.qlist(targets) do
		if self:isEnemy(p) then table.insert(n, p) end
	end

	if #n == 0 then return nil end

	local mostp =1
	for _, p in ipairs(n) do
		if p:getHandcardNum() > n[mostp]:getHandcardNum() then mostp = _ end
	end

	return n[mostp]
end
sgs.ai_playerchosen_intention.lingshou = 50
sgs.ai_skill_askforag.lingshou = function(self, ids)
	local cards = {}
	for _, id in ipairs(ids) do
		table.insert(cards, sgs.Sanguosha:getCard(id))
	end

	self:sortByKeepValue(cards)
	return cards[#cards]:getId()
end
-- 用默认的使用杀算法选择目标杀
-- 拆白银狮子回血
-- 如果牌 > 2张，重铸可能更赚？
sgs.ai_skill_use["@@LingshouOtherVS"] = function(self)
--	local idstrings = string.split(self.player:property("lingshouSelected"):toString(), "+")
	local id = tonumber(self.player:property("lingshouSelected"):toString())
	local ids = {id}
	local originalCard = sgs.Sanguosha:getCard(id)
	local equips = self.player:getEquips()
	for _, equip in sgs.qlist(equips) do
		if equip:getSuit() == originalCard:getSuit() then
			table.insert(ids, equip:getId())
			if equip:isKindOf("SilverLion") and self.player:isWounded() then return "." end
		end
	end

	if #ids > 2 then return "." end

	local slash = sgs.Sanguosha:cloneCard("Slash", sgs.Card_SuitToBeDecided, -1)
	slash:setSkillName("_lingshou")
	for _, s in ipairs(ids) do
		slash:addSubcard(s)
	end

	local use = {isDummy = true, to = sgs.SPlayerList()}
	self:useCardSlash(slash, use)
	slash:deleteLater()
	if use.card and not use.to:isEmpty() then
		local slist = {}
		for _, s in sgs.qlist(use.to) do
			table.insert(slist, s:objectName())
		end
		return slash:toString() .. "->" .. table.concat(slist, "+")
	end

	return "."
end

-- 【祈绝】主公技，当其他鬼势力角色杀死一名角色后或死亡时，其可以令你回复1点体力。
-- 对队友发动
sgs.ai_skill_invoke.qijue = function(self,data)
	return self:isFriend(data:toPlayer())
end
sgs.ai_choicemade_filter.skillInvoke.qijue = function(self, player, args, data)
	sgs.updateIntention(data:toPlayer(), player, -60)
end

-- 【善垒】锁定技，准备阶段开始时，若你的手牌数大于你的手牌上限，你将手牌弃置至上限；结束阶段开始时，若你不是手牌数唯一最多的角色，你将手牌摸至X张（X为手牌最多的角色的手牌数+1）。
-- None needed

-- 【崩落】一名角色的结束阶段开始时，若你于此回合内不因使用而失去过手牌，你可以使用一张非锦囊牌。
-- 照抄sqchuangshi
-- shanlei+bengluo = 延时卖血，保守的话暂时先卖剩2血，之后再看看怎么调
sgs.ai_skill_use["BasicCard+^Jink,EquipCard|.|.|sqchuangshi"] = function(self, prompt, method)
	local cards =  self:getCards("sqchuangshi", "hs")
	self:sortByUseValue(cards)
	for _, card in ipairs(cards) do
		if card:getTypeId() == sgs.Card_TypeBasic and not card:isKindOf("Jink") then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			self:useBasicCard(card, dummy_use)
			if dummy_use.card then
				if dummy_use.to:isEmpty() then
					return dummy_use.card:toString()
				else
					local target_objectname = {}
					for _, p in sgs.qlist(dummy_use.to) do
						table.insert(target_objectname, p:objectName())
					end
					return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
				end
			end
		elseif card:getTypeId() == sgs.Card_TypeEquip then
			local dummy_use = { isDummy = true }
			self:useEquipCard(card, dummy_use)
			if dummy_use.card then
				return dummy_use.card:toString()
			end
		end
	end
	return "."
end

-- 【沦溺】其他角色的出牌阶段开始时，你可以将你装备区里的一张牌置入其装备区（若已有同类型的牌则替换之），若如此做，此阶段结束时，其获得其装备区里所有的牌。
-- 看注释吧

-- 出牌阶段开始的询问
sgs.ai_skill_use["@@lunni"] = function(self)
	local w, a, oh, dh, t = {},{},{},{},{}
	for _, p in sgs.qlist(self.player:getCards("e")) do
		    if p:isKindOf("Weapon") then table.insert(w, p:getId())
		elseif p:isKindOf("Armor") then table.insert(a, p:getId())
		elseif p:isKindOf("OffensiveHorse") then table.insert(oh, p:getId())
		elseif p:isKindOf("DefensiveHorse") then table.insert(dh, p:getId())
		elseif p:isKindOf("Treasure") then table.insert(t, p:getId())
		end
	end

	local uselessCamouflage = function(room, player)
		for _, p in sgs.qlist(room:getOtherPlayers(player)) do
			if p:getArmor() then return true end
		end
		return false
	end

	local id

	local __first = true
	while __first do -- do {} while (0);
		__first = false
		local current = self.room:getCurrent()
		if self:isFriend(current) then
			-- 队友的话
			-- 送狮子回血
			if (not current:getArmor()) or ((current:getArmor():getClassName() == "Camouflage") and uselessCamouflage(self.room,current)) then
				local silverlion
				for _, p in ipairs(a) do
					if sgs.Sanguosha:getCard(p):getClassName() == "SilverLion" then silverlion = p break end
				end
				if silverlion then
					if self:isWeak() and self.player:getArmor() and self.player:getArmor():getId() == silverlion and current:getCards("hes"):length() <= current:getMaxCards() then
						id = silverlion
						break
					end
					if self:isWeak(current) then
						id = silverlion
						break
					end
				end
			end
			-- 送连弩
			if getKnownCard(current, self.player, "Slash", true) > 2 then
				if not current:getWeapon() and #w > 0 then
					local crossbow
					for _, p in ipairs(w) do
						if sgs.Sanguosha:getCard(p):getClassName() == "Crossbow" then crossbow = p break end
					end
					if crossbow then
						id = crossbow
						break
					end
				end
			end
			-- 不要让队友弃牌太多
			if current:getCards("hes"):length() <= current:getMaxCards() then
				-- 送距离
				if getKnownCard(current, self.player, "Slash", true) > 0 then
					if not current:getWeapon() and #w > 0 then
						local maxDist = 1
						for _, id in ipairs(w) do
							local weapon = sgs.Sanguosha:getCard(id):getRealCard():toWeapon()
							if sgs.Sanguosha:getCard(w[maxDist]):getRealCard():toWeapon():getRange() < weapon:getRange() then maxDist = _ end
						end
						id = w[maxDist]
						break
					elseif not current:getOffensiveHorse() and #of > 0 then
						id = of[1]
						break
					end
				end
				-- 送玉玺
				if not current:getTreasure() and #t > 0 then
					local jadeseal
					for _, p in ipairs(t) do
						if sgs.Sanguosha:getCard(p):getClassName() == "JadeSeal" then jadeseal = p break end
					end
					if jadeseal then
						id = jadeseal
						break
					end
				end
			end
		elseif self:isEnemy(current) then
			-- 如果是对手，优先拆母牛
			if current:getTreasure() and (current:getTreasure():getClassName() == "WoodenOx") and #t > 0 then
				-- 给个宝塔镇一镇（
				local pagoda
				for _, p in ipairs(t) do
					if sgs.Sanguosha:getCard(p):getClassName() == "Pagoda" then pagoda = p break end
				end
				if pagoda then
					id = pagoda
					break
				end
			end
			-- 弄掉对面的防御装备，限制对面的母牛
			if (not ((not current:getArmor()) or ((current:getArmor():getClassName() == "Camouflage") and uselessCamouflage(self.room,current)))) or current:getDefensiveHorse() or (current:getTreasure() and (current:getTreasure():getClassName() == "WoodenOx")) then
				if #a > 0 then
					-- 可以给的防具，除了白银狮子都能给？
					local theArmor
					for _, p in ipairs(a) do
						-- 给手牌
						if sgs.Sanguosha:getCard(p):getClassName() ~= "SilverLion" and (not self.player:getArmor() or (self.player:getArmor():getId() ~= p)) then theArmor = p break end
					end
					if theArmor then
						id = theArmor
						break
					end
				end
				if #dh > 0 then
					-- +1马
					local theDh
					for _, p in ipairs(dh) do
						-- 给手牌
						if not self.player:getDefensiveHorse() or (self.player:getDefensiveHorse():getId() ~= p) then theDh = p break end
					end
					if theDh then
						id = theDh
						break
					end
				end
			end
			-- 给个连弩限制距离，但是前提是他得没有杀和母牛
			if current:getWeapon() --[[ or (current:getTreasure() and (current:getTreasure():getClassName() == "WoodenOx")) ]] then
				if getKnownCard(current, self.player, "Slash", true) == 0 and (not (current:getTreasure() and (current:getTreasure():getClassName() == "WoodenOx"))) then
					local crossbow
					for _, p in ipairs(w) do
						if sgs.Sanguosha:getCard(p):getClassName() == "Crossbow" then crossbow = p break end
					end
					if crossbow then
						id = crossbow
						break
					end
				end
			end

			if current:getCards("hes"):length() > current:getMaxCards() then
				-- 拆迁，送-1
				if #oh > 0 then
					local theOh
					for _, p in ipairs(oh) do
						-- 优先给手牌
						if not self.player:getOffensiveHorse() or (self.player:getOffensiveHorse():getId() ~= p) then theOh = p break end
					end
					if theOh then
						id = theOh
						break
					end
					-- 装备也给
					id = oh[1]
					break
				end
				-- 送宝塔
				if #t > 0 then
					local pagoda
					for _, p in ipairs(t) do
						if sgs.Sanguosha:getCard(p):getClassName() == "Pagoda" then pagoda = p break end
					end
					if pagoda then
						id = pagoda
						break
					end
				end
			end
		end
	end

	if id then
		local c = sgs.Sanguosha:cloneSkillCard("LunniCard")
		c:addSubcard(id)
		c:deleteLater()
		return c:toString() .. "->."
	end

	return "."
end

-- 【撩罟】当其他角色于其弃牌阶段弃置牌后，你可以使用这些牌中的一张牌，然后若你于回合内未曾失去过牌，你须弃置一张牌。
-- 思路：
-- 如果没有失去过牌：（如果没有lunni）狮子 -> 桃以外的手牌 -> （如果没有lunni）-1
-- 如果有lunni：拿不重样的装备
-- 然后使用usevalue最大的
local liaogu_discard

sgs.ai_skill_discard["liaogu"] = function(self)
	local owner = self.room:getCardOwner(liaogu_discard)
	if owner and owner:objectName() == self.player:objectName() then return liaogu_discard end

	-- return nil to let default AI dealing with unexpected condition
end
sgs.ai_skill_use["@@liaogu"] = function(self)
	if not self.player:hasFlag("liaogulost") then
		-- 先想好弃置啥 -- 这段逻辑感觉有点问题。。。？？？
		repeat
			liaogu_discard = nil

			if not self.player:hasSkill("lunni", true) then
				if self.player:getArmor() and self.player:getArmor():isKindOf("SilverLion") then
					liaogu_discard = self.player:getArmor():getId()
					break
				end
			end

			local hc = self.player:getHandcards()
			local cl = {}
			for _, c in sgs.qlist(hc) do
				if not c:isKindOf("Peach") then table.insert(cl, c) end
			end
			if #cl > 0 then
				self:sortByKeepValue(cl)
				liaogu_discard = cl[1]:getId()
				break
			end

			if not self.player:hasSkill("lunni", true) then
				if self.player:getOffensiveHorse() then
					liaogu_discard = self.player:getOffensiveHorse():getId()
					break
				end
			end
		until true

		-- 想不好了，不聊了，拜拜了您内！
		return "."
	end

	local liaoguCardIds = self.player:getTag("liaogu_tempmove"):toIntList()
	local liaoguCards = {}
	for _, id in sgs.qlist(liaoguCardIds) do
		table.insert(liaoguCards, sgs.Sanguosha:getCard(id))
	end

	if self.player:hasSkill("lunni", true) then
		local w, a, oh, dh, t
		for _, p in sgs.qlist(self.player:getCards("e")) do
				if p:isKindOf("Weapon") then w = p:getId()
			elseif p:isKindOf("Armor") then a = p:getId()
			elseif p:isKindOf("OffensiveHorse") then oh = p:getId()
			elseif p:isKindOf("DefensiveHorse") then dh = p:getId()
			elseif p:isKindOf("Treasure") then t = p:getId()
			end
		end
		local selected
		local selectedLevel = 8
		for _, c in ipairs(liaoguCards) do
			-- 顺序：狮子 - 母牛 - +1 - 防具 - 宝物 - 武器 - -1
			local level
			    if c:isKindOf("SliverLion") and not a then level = 1
			elseif c:isKindOf("WoodenOx") and not t then level = 2
			elseif c:isKindOf("DefensiveHorse") and not dh then level = 3
			elseif c:isKindOf("Armor") and not a then level = 4
			elseif c:isKindOf("Treasure") and not t then level = 5
			elseif c:isKindOf("Weapon") and not w then level = 6
			elseif c:isKindOf("OffensiveHorse") and not oh then level = 7
			else level = 9
			end

			if level < selectedLevel then selectedLevel = level selected = c end
		end
		if selected then
			-- 装备牌都是直接用的，不用加目标
			local card = sgs.Sanguosha:cloneSkillCard("LiaoguCard")
			card:addSubcard(selected)
			card:deleteLater()
			return card:toString() .. "->."
		end
	end

	-- 崩服务器 暂且注释掉。。。。
--	self:sortByUseValue(liaoguCards)
--	for _, c in ipairs(liaoguCards) do
--		-- 如果是装备，要确认是否这个装备会顶掉discard的牌
--		local willReplaceEquip
--		if c:isKindOf("EquipCard") and (not self.player:hasFlag("liaogulost")) then
--			local ce = c:getRealCard():toEquipCard()
--			local discardce = sgs.Sanguosha:getCard(liaogu_discard):getRealCard():toEquipCard()
--			willReplaceEquip = (ce:location() == discardce:location())
--		end
--		if (not willReplaceEquip) then
--			self.player:speak(c:toString())
--			local use = {isDummy = true, to = sgs.SPlayerList()}
--			self:useCardByClassName(c, use)
--			if use.card then
--				local card = sgs.Sanguosha:cloneSkillCard("LiaoguCard")
--				card:addSubcard(selected)
--				card:deleteLater()
--				local targetsStr = "."
--				if not use.to:isEmpty() then
--					local targets = {}
--					for _, to in sgs.qlist(use.to) do table.insert(targets, to:objectName()) end
--					targetsStr = table.concat(targets, "+")
--				end
--				return card:toString() .. "->" .. targetsStr
--			end
--		end
--	end

	return "."
end

-- 血岭: 当你于出牌阶段外不以此法而失去牌后，你可以明置、横置或弃置你区域里的一张牌，然后若你各区域里不处于异常状态的牌数均不大于1，你摸两张牌。
local xueling_S = function(player, place)
	if place == sgs.Player_PlaceHand then
		return (player:getHandcardNum() - player:getShownHandcards():length()) <= 1
	elseif place == sgs.Player_PlaceEquip then
		return (player:getEquips():length() - player:getBrokenEquips():length()) <= 1
	elseif place == sgs.Player_PlaceDelayedTrick then
		return (player:getJudgingAreaID():length()) <= 1
	end
end
local xueling_O = function(player, place)
	if place == sgs.Player_PlaceHand then
		return (player:getHandcardNum() - player:getShownHandcards():length()) > 0
	elseif place == sgs.Player_PlaceEquip then
		return (player:getEquips():length() - player:getBrokenEquips():length()) > 0
	elseif place == sgs.Player_PlaceDelayedTrick then
		return (player:getJudgingAreaID():length()) > 0
	end
end
local xueling_choice
sgs.ai_skill_cardask["@xueling-select"] = function(self)
	-- 弃置有害延时
	local badDelayedTricks = {"Indulgence", "SupplyShortage", "Lightning"}
	for _, dt in sgs.qlist(self.player:getJudgingArea()) do
		for _, b in ipairs(badDelayedTricks) do
			if dt:isKindOf(b) then
				xueling_choice = "discard"
				return "$" .. tostring(dt:getEffectiveId())
			end
		end
	end

	local S, O = {}
	for _, p in ipairs({sgs.Player_PlaceHand, sgs.Player_PlaceEquip, sgs.Player_PlaceDelayedTrick}) do
		if not xueling_S(self.player, p) then table.insert(S, p) end
		if xueling_O(self.player, p) then table.insert(O, p) end
	end

	if #S == 0 then
		-- find operable place
		for _, p in ipairs({sgs.Player_PlaceDelayedTrick, sgs.Player_PlaceHand, sgs.Player_PlaceEquip}) do
			local contains = false
			for _, o in ipairs(O) do
				if o == p then contains = true break end
			end
			if contains then table.insert(S, p) break end
		end
		if #S == 0 then return "." end
	end

	if #S == 1 then
		if S[1] == sgs.Player_PlaceDelayedTrick then
			for _, dt in sgs.qlist(self.player:getJudgingArea()) do
				if dt:isKindOf("SpringBreath") then -- 赌春息希望渺茫，不如换两张牌来的实在，优先
					xueling_choice = "discard"
					return "$" .. tostring(dt:getEffectiveId())
				end
			end
			for _, dt in sgs.qlist(self.player:getJudgingArea()) do
				if not dt:isKindOf("SavingEnergy") then -- 未知锦囊优先。养精蓄锐见效快
					xueling_choice = "discard"
					return "$" .. tostring(dt:getEffectiveId())
				end
			end
			for _, dt in sgs.qlist(self.player:getJudgingArea()) do
				xueling_choice = "discard"
				return "$" .. tostring(dt:getEffectiveId())
			end
		elseif S[1] == sgs.Player_PlaceHand then
			local handcards = sgs.QList2Table(self.player:getHandcards())
			if #handcards > 0 then
				self:sortByKeepValue(handcards)
				for _, c in ipairs(handcards) do
					if not self.player:isShownHandcard(c:getEffectiveId()) then
						xueling_choice = "showHandcard"
						return "$" .. tostring(c:getEffectiveId())
					end
				end
				xueling_choice = "discard"
				return "$" .. tostring(handcards[1]:getEffectiveId())
			end
		elseif S[1] == sgs.Player_PlaceEquip then
			if self.player:getArmor() and not self.player:isBrokenEquip(self.player:getArmor():getEffectiveId()) then
				xueling_choice = "breakEquip"
				return "$" .. tostring(self.player:getArmor():getEffectiveId())
			end
			if self.player:getDefensiveHorse() and not self.player:isBrokenEquip(self.player:getDefensiveHorse():getEffectiveId()) then
				xueling_choice = "breakEquip"
				return "$" .. tostring(self.player:getDefensiveHorse():getEffectiveId())
			end
			if self.player:getTreasure() and not self.player:isBrokenEquip(self.player:getTreasure():getEffectiveId()) then
				xueling_choice = "breakEquip"
				return "$" .. tostring(self.player:getTreasure():getEffectiveId())
			end
			if self.player:getOffensiveHorse() and not self.player:isBrokenEquip(self.player:getOffensiveHorse():getEffectiveId()) then
				xueling_choice = "breakEquip"
				return "$" .. tostring(self.player:getOffensiveHorse():getEffectiveId())
			end
			if self.player:getWeapon() and not self.player:isBrokenEquip(self.player:getWeapon():getEffectiveId()) then
				xueling_choice = "breakEquip"
				return "$" .. tostring(self.player:getWeapon():getEffectiveId())
			end
			local equips = sgs.QList2Table(self.player:getEquips())
			if #equips > 0 then
				self:sortByUseValue(equips) -- 这个其实很粗糙就是了
				xueling_choice = "discard"
				return "$" .. tostring(equips[#equips]:getEffectiveId())
			end
		end

		-- 正常来讲走到这里就已经意味着没啥可干的了吧
		return "."
	end

	if self.player:getArmor() and not self.player:isBrokenEquip(self.player:getArmor():getEffectiveId()) then
		xueling_choice = "breakEquip"
		return "$" .. tostring(self.player:getArmor():getEffectiveId())
	end
	if self.player:getDefensiveHorse() and not self.player:isBrokenEquip(self.player:getDefensiveHorse():getEffectiveId()) then
		xueling_choice = "breakEquip"
		return "$" .. tostring(self.player:getDefensiveHorse():getEffectiveId())
	end
	if self.player:getTreasure() and not self.player:isBrokenEquip(self.player:getTreasure():getEffectiveId()) then
		xueling_choice = "breakEquip"
		return "$" .. tostring(self.player:getTreasure():getEffectiveId())
	end
	local handcards = sgs.QList2Table(self.player:getHandcards())
	if #handcards > 0 then
		self:sortByKeepValue(handcards)
		for _, c in ipairs(handcards) do
			if not self.player:isShownHandcard(c:getEffectiveId()) then
				xueling_choice = "showHandcard"
				return "$" .. tostring(c:getEffectiveId())
			end
		end
	end
	if self.player:getOffensiveHorse() and not self.player:isBrokenEquip(self.player:getOffensiveHorse():getEffectiveId()) then
		xueling_choice = "breakEquip"
		return "$" .. tostring(self.player:getOffensiveHorse():getEffectiveId())
	end
	if self.player:getWeapon() and not self.player:isBrokenEquip(self.player:getWeapon():getEffectiveId()) then
		xueling_choice = "breakEquip"
		return "$" .. tostring(self.player:getWeapon():getEffectiveId())
	end
	if #handcards > 0 then
		xueling_choice = "discard"
		return "$" .. tostring(handcards[1]:getEffectiveId())
	end
	local equips = sgs.QList2Table(self.player:getEquips())
	if #equips > 0 then
		self:sortByUseValue(equips) -- 这个其实很粗糙就是了
		xueling_choice = "discard"
		return "$" .. tostring(equips[#equips]:getEffectiveId())
	end
	for _, dt in sgs.qlist(self.player:getJudgingArea()) do
		if dt:isKindOf("SpringBreath") then -- 赌春息希望渺茫，不如换两张牌来的实在，优先
			xueling_choice = "discard"
			return "$" .. tostring(dt:getEffectiveId())
		end
	end
	for _, dt in sgs.qlist(self.player:getJudgingArea()) do
		if not dt:isKindOf("SavingEnergy") then -- 未知锦囊优先。养精蓄锐见效快
			xueling_choice = "discard"
			return "$" .. tostring(dt:getEffectiveId())
		end
	end
	for _, dt in sgs.qlist(self.player:getJudgingArea()) do
		xueling_choice = "discard"
		return "$" .. tostring(dt:getEffectiveId())
	end

	-- 啊？啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊？
	return "."
end
sgs.ai_skill_choice.xueling = function(self, choices)
	if #self.friends_noself == 0 and xueling_choice == "breakEquip" then return "discard" end
	return xueling_choice
end
-- 遗翎: 当你的手牌被暗置，或装备牌被重置时，你可以将之交给一名其他角色。
sgs.ai_skill_playerchosen.weiling = function(self, targets)
	if #self.friends_noself == 0 then return end
	local ids = self.player:getTag("weiling"):toIntList()
	local card = sgs.Sanguosha:getCard(ids[1])
	self:sort(self.friends_noself, "handcard")
	if card:isKindOf("EquipCard") then
		return self.friends_noself[#self.friends_noself]
	else
		return self.friends_noself[1]
	end
end
sgs.ai_playerchosen_intention.weiling = -30

-- 伪仪: 出牌阶段限一次，你可以令一名有牌的其他角色选择一项（TurnUse）：
local weiyi_t = {}
weiyi_t.name = "weiyi"
table.insert(sgs.ai_skills, weiyi_t)
weiyi_t.getTurnUseCard = function(self)
	return sgs.Card_Parse("@WeiyiCard=.")
end
sgs.ai_skill_use_func.WeiyiCard = function(card, use, self)
	-- 有1血的敌人处于自己和队友攻击范围内，且该队友至少有三张手牌，则技能对该队友释放，出杀目标指向该敌人；
	for _, p in ipairs(self.friends_noself) do
		for _, e in ipairs(self.enemies) do
			if self.player:inMyAttackRange(e) and p:inMyAttackRange(e) and self:isWeak(e) and p:getHandcardNum() >= 3 then
				use.card = card
				if use.to then
					use.to:append(p)
					use.to:append(e)
				end
				return
			end
		end
	end

	local enemiesh = {}
	for _, p in ipairs(self.enemies) do
		if not p:isKongcheng() then table.insert(enemiesh, p) end
	end

	if #enemiesh >= 1 then
		self:sort(enemiesh, "defense")
		self:sort(self.enemies, "defense")
		local target, slashtarget = enemiesh[1]
		for _, p in ipairs(self.enemies) do
			if p:objectName() ~= target:objectName() then
				slashtarget = p
				break
			end
		end

		use.card = card
		if use.to then
			use.to:append(target)
			use.to:append(slashtarget or self.player)
		end
		return
	end
end
-- 2.对你选择的另一名角色使用【杀】（无距离限制）。 （默认，askForUseSlashTo，这个有默认ai，返回 "." fallback到1）
-- 1.将一张牌交给你（ai_skill_discard.weiyi），你获得后，你可以将之当【杀】对其使用（ai_skill_use["@@weiyi"]）；
sgs.ai_skill_discard.weiyi = function(self)
	-- 选项1根据牌的价值决定杀还是留
	local use = self.player:getTag("weiyi"):toCardUse()
	local handcard = sgs.QList2Table(self.player:getHandcards())
	self:sortByCardNeed(handcard)
	return self:isFriend(use.from) and {handcard[1]:getEffectiveId()} or {handcard[#handcard]:getEffectiveId()}
end
sgs.ai_skill_use["@@weiyi"] = function(self)
	local viewAsSlash = sgs.Sanguosha:cloneCard("Slash", sgs.Card_SuitToBeDecided, -1)
	viewAsSlash:addSubCard(self.player:getTag("weiyiSelected"):toInt())
	viewAsSlash:setSkillName("_weiyi")
	local use = {isDummy = true, to = sgs.SPlayerList()}
	self:useCardSlash(viewAsSlash, use)
	viewAsSlash:deleteLater()
	if use.card then
		-- 杀！
		local realUse = sgs.CardUseStruct()
		realUse.card = viewAsSlash
		realUse.to = use.to
		return realUse:toString()
	end
end
sgs.ai_card_intention.WeiyiCard = function(self, card, from, to)
	sgs.updateIntention(to[#to], from, 10)
end

-- 夺志: 一名角色的准备阶段开始时，若其手牌数不小于你，你可以令一名手牌数小于你的角色摸一张牌（ai_skill_playerchosen.duozhi），然后令后者于此回合内不能使用或打出【闪】。
sgs.ai_skill_playerchosen.duozhi = function(self, targets)
	local current = self.room:getCurrent()
	if not current then return end
	if self:isFriend(current) then
		local friends = {}
		for _, t in sgs.qlist(targets) do
			if self:isEnemy(t) and current:inMyAttackRange(t) and t:getHp() == t:getDyingFactor() then
				return t
			end
			if self:isFriend(t) then
				table.insert(friends, t)
			end
		end

		if #friends > 0 then
			self:sort(friends, "handcard")
			return friends[1]
		end
	end
	if self:isEnemy(current) then
		local friends = {}
		for _, t in sgs.qlist(targets) do
			if self:isFriend(t) and not current:inMyAttackRange(t) then
				table.insert(friends, t)
			end
		end

		if #friends > 0 then
			self:sort(friends, "handcard")
			return friends[1]
		end
	end
end

-- 【领军】当你于一个回合内使用的第一张【杀】结算完毕后，你可以选择此牌的一个目标，令攻击范围内有其的其他角色各选择是否将一张基本牌当【杀】对其使用（除其外的角色不是合法目标），然后若此次被以此法转化的牌均是【杀】，你视为对其使用【杀】。
-- 只要不是杀队友无脑发动，目标的对手手牌中有杀的话用杀当杀，没杀的话看剩余目标数和目标血量手牌数决定是否用其他牌（除桃外）当杀，目标的队友不响应

sgs.ai_skill_playerchosen.lingjun = function(self, targets)
	local ts = {}
	for _, t in sgs.qlist(targets) do
		if self:isEnemy(t) then table.insert(ts, t) end
	end

	if #ts == 0 then return nil end
	self:sortEnemies(ts)
	return ts[1]
end

sgs.ai_skill_use["@@LingjunOtherVS"] = function(self)
	local target = nil
	local from = nil
	for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if p:hasFlag("SlashAssignee") then target = p end
		if self.player:hasFlag("SlashRecorder_Lingjun_" .. p:objectName()) then from = p end
		if target and from then break end
	end

	if not target then return "." end
	if self:isFriend(target) then return "." end

	-- 是否已经有人用杀以外的牌当杀了？需要判断lingjun的发动者
	local useSlashAsSlash = from and (not from:hasFlag("lingjunNotSlash"))
	if useSlashAsSlash then
		-- 没有人用过杀以外的牌当杀
		local slashes = {}
		for _, c in sgs.qlist(self.player:getHandcards()) do
			if c:isKindOf("Slash") then table.insert(slashes, c) end
		end
		if #slashes > 0 then
			-- 有杀
			self:sortByKeepValue(slashes)
			local viewAsSlash = sgs.Sanguosha:cloneCard("Slash", sgs.Card_SuitToBeDecided, -1)
			viewAsSlash:setSkillName("_lingjun")
			local use = {isDummy = true, to = sgs.SPlayerList()}
			use.to:append(target)
			for _, sl in ipairs(slashes) do
				viewAsSlash:clearSubcards()
				viewAsSlash:addSubcard(sl)
				-- 判断一下要不要用
				self:useCardSlash(viewAsSlash, use)
				viewAsSlash:deleteLater()
				if use.card then
					-- 杀！
					return viewAsSlash:toString() .. "->" .. target:objectName()
				end
			end

			-- 这么怂
			return "."
		end

		-- 判断还剩余多少队友没响应，是否值得用非杀转换杀他
		local op = self.room:getOtherPlayers(from)
		local flag = false
		local n = 0
		for _, p in sgs.qlist(op) do
			if p:objectName() == self.player:objectName() then
				flag = true
			elseif flag then
				if p:inMyAttackRange(target) and self:isFriend(p) then n = n + 1 end
			end
		end

		-- 先不搞他，反正他还有点血
		if sgs.getDefense(target) > n then return "." end
	end

	-- 用所有的基本牌（除桃）转化
	local basics = {}
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if c:isKindOf("BasicCard") and (not c:isKindOf("Peach")) then table.insert(basics, c) end
	end
	if #basics > 0 then
		-- 有基本牌
		self:sortByKeepValue(basics)
		local viewAsSlash = sgs.Sanguosha:cloneCard("Slash", sgs.Card_SuitToBeDecided, -1)
		viewAsSlash:setSkillName("_lingjun")
		local use = {isDummy = true, to = sgs.SPlayerList()}
		use.to:append(target)
		for _, sl in ipairs(basics) do
			viewAsSlash:clearSubcards()
			viewAsSlash:addSubcard(sl)
			-- 判断一下要不要用
			self:useCardSlash(viewAsSlash, use)
			viewAsSlash:deleteLater()
			if use.card then
				-- 杀！
				return viewAsSlash:toString() .. "->" .. target:objectName()
			end
		end
	end

	-- 怂
	return "."
end

-- 【瓷偶】锁定技 当你受到伤害时，若受到的是【杀】造成的无属性伤害，此伤害结算结束后你失去1点体力，否则此伤害值-1。
-- 本体无需AI，防御要调整

--劲疾: 锁定技，你与其他角色的距离-1，且其他角色与你的距离+1；当你你使用或打出基本牌时，此技能于此回合内无效。
-- None needed

--天行: 一名角色的出牌阶段结束时，若你攻击范围内的角色数与此阶段开始时不同，你可以视为使用无视距离的幻【杀】。
sgs.ai_skill_use["@@tianxing"] = function(self)
	local viewAsSlash = sgs.Sanguosha:cloneCard("Slash", sgs.Card_SuitToBeDecided, -1)
	viewAsSlash:setSkillName("tianxing")
	viewAsSlash:setShowSkill("tianxing")
	local use = {isDummy = true, to = sgs.SPlayerList()}
	self:useCardSlash(viewAsSlash, use)
	viewAsSlash:deleteLater()
	if use.card then
		-- 杀！
		local realUse = sgs.CardUseStruct()
		realUse.card = viewAsSlash
		realUse.to = use.to
		return realUse:toString()
	end
end
