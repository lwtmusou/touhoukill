
-- 【缮形】当一名角色失去装备区里的牌后，你可以令其摸一张牌，然后其可以交给你一张手牌。
-- 思路：对队友发动，然后如果你处于回合内弃牌阶段前，队友给usevalue最高的牌，否则给keepvalue最高的牌
sgs.ai_skill_invoke.shanxing = function(self, data)
	return self:isFriend(data:toPlayer())
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

-- 【灵守】结束阶段开始时，你可以观看一名角色的手牌并展示其中至多两张花色相同的牌，其须选择将这些牌重铸或当【杀】使用。
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
		-- 先想好弃置啥
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

-- 【狱守】 其他角色的准备阶段开始时，你可以弃置一张手牌，若如此做，当其于此回合内使用第一张有点数的牌时，若点数不大于之，此牌无效，反之此牌不计入限制的使用次数。
-- 需要大量调整出牌顺序（比如对手用无效的的五谷桃园，队友先用果酒、酒和杀之类的，不过因为果酒和酒的出牌阶段选牌是耦合的，不好做。。。。。。），出牌顺序调整搁置，这里只做本体
-- 本体思路：只留桃子其余全扔，如果有未发动的零度且其他区域有牌，桃子也扔（反正可以捡回来），对队友的话，扔小于等于7的牌，对对手扔大于等于7的牌

sgs.ai_skill_cardask["@yvshou-discard"] = function(self, data)
	local current = data:toPlayer()

	local sortfunc = function(a,b)
		return a:getNumber() < b:getNumber()
	end
	
	local cards = {}
	for _, p in sgs.qlist(self.player:getHandcards()) do
		local isNotPeach = --[[ (self.player:hasSkill("lingdu") and (not current:hasFlag("lingdu")) and (not (self.player:getJudgingArea():isEmpty() and self.player:getEquips():isEmpty()))) and ]] (not p:isKindOf("Peach"))
		if isNotPeach then table.insert(cards, p) end
	end
	
	if #cards == 0 then return "." end
	table.sort(cards, sortfunc)
	if self:isEnemy(current) then
		if cards[#cards]:getNumber() < 7 then return "." end
		return tostring(cards[#cards]:getId())
	elseif self:isFriend(current) then
		if cards[1]:getNumber() > 7 then return "." end
		return tostring(cards[1]:getId())
	end
	
	return "."
end

-- 【灵渡】 当你区域里的牌于回合外置入弃牌堆后（包括牌使用或打出结算完毕后），你可以用你区域里另一张牌替换之，若以此法失去了一个区域里最后的一张牌，你摸一张牌。每回合限一次。
-- 思路：捡桃子，扔兵乐电和装备（除母牛）和手牌（除桃）

local lingdu_pick = nil

sgs.ai_skill_cardask["@lingdu-discard"] = function(self,data)
	local ids = data:toIntList()
	local cards = {}
	for _, id in sgs.qlist(ids) do
		table.insert(cards, sgs.Sanguosha:getCard(id))
	end
	
	lingdu_pick = nil
	
	-- 桃子
	for _, c in ipairs(cards) do
		if c:isKindOf("Peach") then lingdu_pick = c:getId() break end
	end
	
	-- 其余牌，按需保留
	if not lingdu_pick then
		self:sortByCardNeed(cards)
		lingdu_pick = cards[#cards]:getId()
	end
	
	-- 要扔的牌，优先判定区
	local j = self.player:getJudgingArea()
	-- 优先度：乐兵电
	for _, c in sgs.qlist(j) do
		if c:isKindOf("Indulgence") then return "$" .. c:getId() end
	end
	for _, c in sgs.qlist(j) do
		if c:isKindOf("SupplyShortage") then return "$" .. c:getId() end
	end
	for _, c in sgs.qlist(j) do
		if c:isKindOf("Lightning") then return "$" .. c:getId() end
	end
	-- 然后若只有一张养精蓄锐也扔。春息不扔
	if j:length() == 1 and j:first():isKindOf("SavingEnergy") then
		return "$" .. j:first():getId()
	end
	
	-- 然后是装备，除母牛不扔之外，其他全部扔，优先扔没用的Camouflage
	local uselessCamouflage = function(room, player)
		for _, p in sgs.qlist(room:getOtherPlayers(player)) do
			if p:getArmor() then return true end
		end
		return false
	end
	if self.player:getArmor() and self.player:getArmor():getClassName() == "Camouflage" and uselessCamouflage(self.room, self.player) then
		return "$" .. self.player:getArmor():getId()
	end
	-- 然后是优先度：-1，武器，+1，宝物（除母牛），防具
	if self.player:getOffensiveHorse() then return "$" .. self.player:getOffensiveHorse():getId() end
	if self.player:getWeapon() then return "$" .. self.player:getWeapon():getId() end
	if self.player:getDefensiveHorse() then return "$" .. self.player:getDefensiveHorse():getId() end
	if self.player:getTreasure() and (self.player:getTreasure():getClassName() ~= "WoodenOx") then return "$" .. self.player:getTreasure():getId() end
	if self.player:getArmor() then return "$" .. self.player:getArmor():getId() end
	
	-- 手牌（除桃）
	local hc = self.player:getHandcards()
	local hcs = {}
	for _, c in sgs.qlist(hc) do
		if not c:isKindOf("Peach") then table.insert(hcs, c) end
	end
	
	if #hcs == 0 then return "." end
	
	self:sortByKeepValue(hcs)
	return "$" .. hcs[1]:getId()
end

sgs.ai_skill_askforag.lingdu = function()
	return lingdu_pick
end

-- 【夺志】锁定技 当你使用牌结算完毕后，你令所有其他角色于当前回合内不能使用或打出牌。
-- 本体无需AI，出牌顺序可能需要微调，比如先挂装备等，防御上调（防二刀，防锦囊，太强了），出牌顺序搁置

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

-- 【劲疾】锁定技 你与其他角色的距离-X（X为你装备区里的牌数），其他角色与你的距离+Y（Y为你装备区里横置的牌数）。
-- None needed

-- 【天行】一名其他角色的准备阶段开始时，你可以横置装备区里的一张牌，视为对其使用【杀】；当你使用【杀】对一名角色造成伤害后，其于此回合内不能使用以你为唯一目标的牌。 
-- 用默认杀的策略，只是杀换成了视为的

--[[sgs.ai_skill_cardask["@tianxing-discard"] = function(self, data)
	local current = data:toPlayer()
	if self:isFriend(current) then return "." end
	
	local viewAsSlash = sgs.Sanguosha:cloneCard("Slash", sgs.Card_SuitToBeDecided, -1)
	viewAsSlash:setSkillName("_tianxing")
	local use = {isDummy = true, to = sgs.SPlayerList()}
	use.to:append(current)
	self:useCardSlash(viewAsSlash, use)
	viewAsSlash:deleteLater()
	if not use.card then return "." end
	
	
	-- 横置优先级： -1，武器，+1，宝物，防具
	local __first = true
	local id
	while __first do
		__first = false
		if self.player:getOffensiveHorse() and not self.player:isBrokenEquip(self.player:getOffensiveHorse():getId()) then id = self.player:getOffensiveHorse():getId() break end
		if self.player:getWeapon() and not self.player:isBrokenEquip(self.player:getWeapon():getId()) then id = self.player:getWeapon():getId() break end
		if self.player:getDefensiveHorse() and not self.player:isBrokenEquip(self.player:getDefensiveHorse():getId()) then id = self.player:getDefensiveHorse():getId() break end
		if self.player:getTreasure() and not self.player:isBrokenEquip(self.player:getTreasure():getId()) then id = self.player:getTreasure():getId() break end
		if self.player:getArmor() and not self.player:isBrokenEquip(self.player:getArmor():getId()) then id = self.player:getArmor():getId() break end
	end
	
	if not id then return "." end
	
	return "$" .. tostring(id)
end
]]