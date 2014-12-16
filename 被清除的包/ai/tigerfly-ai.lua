--技能：奢靡
local shemi_skill = {}
shemi_skill.name = "shemi" --只保证能否发动
table.insert(sgs.ai_skills, shemi_skill)
shemi_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ShemiAG") then return end
	local usenum = 2
	local usecard
	local hp = self.player:getHp()
	if hp <= 2 then usenum = 1 end
	local first_card, second_card
	if self.player:getCards("he"):length() >= usenum then
		local flag = "he"
		if self:needToThrowArmor() and self.player:getArmor() then flag = "eh"
			elseif self:getOverflow() > 0 then flag = "h"
		end
		local cds = sgs.QList2Table(self.player:getCards(flag))
		local eid = self:getValuableCard(self.player)
		self:sortByCardNeed(cds)
		for _, fcard in ipairs(cds) do
			if  not (fcard:isKindOf("Peach") or fcard:isKindOf("ExNihilo") or fcard:isKindOf("AmazingGrace")) and (fcard:getEffectiveId() ~= eid or not eid) then
				first_card = fcard
				if hp <= 2 then break end
				for _, scard in ipairs(cds) do
					if hp <= 2 then break end
					if first_card ~= scard and not (scard:isKindOf("Peach") or scard:isKindOf("ExNihilo") or scard:isKindOf("AmazingGrace")) and (scard:getEffectiveId() ~= eid or not eid) then
						second_card = scard
						local am = sgs.Sanguosha:cloneCard("amazing_grace", sgs.Card_NoSuit, 0)
						am:addSubcard(first_card)
						am:addSubcard(second_card)
						local dummy_use = {isDummy = true}
						self:useTrickCard(am, dummy_use)
						if not dummy_use.card then
							first_card=nil
							second_card=nil
							break
						end
					end
				end
				if first_card and second_card then break end
			end
		end
	end
	if usenum == 2 then
		if first_card and second_card then
		local first_id, second_id =  first_card:getId(), second_card:getId()
		local amazing = sgs.Card_Parse(("amazing_grace:shemi[to_be_decided:0]=%d+%d"):format(first_id, second_id))
		assert(amazing)
		return amazing
		end
	else
		 usecard = first_card
		 if usecard then
			local usecardid = usecard:getId()
			local amazing = sgs.Card_Parse(("amazing_grace:shemi[to_be_decided:0]=%d"):format(usecardid))
			assert(amazing)
			return amazing
		end
	end
end

--技能：宽惠
local function need_kuanhui(self, who)
	local card = sgs.Card_Parse(self.room:getTag("kuanhui_user"):toString())
	if card == nil then return false end
	local from = self.room:getCurrent()
	if self:isEnemy(who) then
		if card:isKindOf("GodSalvation") and who:isWounded() and self:hasTrickEffective(card, who, from) then
			if who:hasSkill("manjuan") and who:getPhase() == sgs.Player_NotActive then return true end
			if self:isWeak(who) then return true end
			if self:hasSkills(sgs.masochism_skill, who) then return true end
		end
		if card:isKindOf("AmazingGrace") and self:hasTrickEffective(card, who, from) then return true end
		return false
	elseif self:isFriend(who) then
		if who:hasSkill("noswuyan") and from:objectName() ~= who:objectName() then return true end
		if card:isKindOf("GodSalvation") and who:isWounded() and self:hasTrickEffective(card, who, from) then
			if self:needToLoseHp(who, nil, nil, true, true) then return true end
			return false
		end
		if card:isKindOf("IronChain") and (self:needKongcheng(who, true) or (who:isChained() and self:hasTrickEffective(card, who, from))) then
			return false
		end
		if card:isKindOf("AmazingGrace") and self:hasTrickEffective(card, who, from) and self:needKongcheng(who, true) then return true end
		if card:isKindOf("AmazingGrace") and self:hasTrickEffective(card, who, from) and not self:needKongcheng(who, true) then return false end
		if sgs.dynamic_value.benefit[card:getClassName()] == true then return false end
		return true
	end
end

function sgs.ai_skill_invoke.kuanhui(self, data)
	local effect = data:toCardUse()
	local tos = effect.to
	local invoke = false
	for _, to in sgs.qlist(tos) do
		if to and need_kuanhui(self, to) then invoke = true break end
	end
	return invoke
end

sgs.ai_skill_playerchosen.kuanhui = function(self, targets)
	targets = sgs.QList2Table(targets)
	self:sort(targets, "defenseSlash")
	for _, player in ipairs(targets) do
	if player and need_kuanhui(self, player) then return player end
	end
end
sgs.ai_playerchosen_intention.kuanhui = function(self, from, to)
	local intention = -77
	local cardx = sgs.Card_Parse(self.room:getTag("kuanhui_user"):toString())
	if not cardx then return end
	if cardx:isKindOf("GodSalvation") and to:isWounded() and not self:needToLoseHp(to, nil, nil, true, true) then intention = 50 end
	if self:needKongcheng(to, true) then intention = 0 end
	if cardx:isKindOf("AmazingGrace") and self:hasTrickEffective(cardx, to) then intention = 0 end
	sgs.updateIntention(from, to, intention)
end


--技能：宏量
sgs.ai_skill_cardask["@HongliangGive"] = function(self, data)
	local damage = data:toDamage()
	local target = damage.from
	local eid = self:getValuableCard(self.player)
	if not target then return "." end
	if self:needKongcheng(self.player, true) and self.player:getHandcardNum() <= 2 then
		if self.player:getHandcardNum() == 1 then
			local card = self.player:getHandcards():first()
			return (isCard("Jink", card, self.player)) and "." or ("$" .. card:getEffectiveId())
		end
		if self.player:getHandcardNum() == 2 then
			local first = self.player:getHandcards():first()
			local last = self.player:getHandcards():last()
			local jink = isCard("Jink", first, self.player) and first or (isCard("Jink", last, self.player) and last)
			if jink then
				return first:getEffectiveId() == jink:getEffectiveId() and ("$"..last:getEffectiveId()) or ("$"..first:getEffectiveId())
			end
		end
	end
	if self:needToThrowArmor() and self.player:getArmor() then
		return "$"..self.player:getArmor():getEffectiveId()
	end
	if self:isFriend(target) then
		local cards = sgs.QList2Table(self.player:getCards("he"))
		self:sortByCardNeed(cards)
		if self:isWeak(target) then return "$"..cards[#cards]:getEffectiveId() end
		if self:isWeak() then
			for _, card in ipairs(cards) do
				if  card:isKindOf("Peach") or (card:getEffectiveId() == eid) then return "." end
			end
		else
			return "$"..cards[1]:getEffectiveId()
		end
	else
		local flag = "h"
		if self:hasSkills(sgs.lose_equip_skill) then flag = "eh" end
		local card2s = sgs.QList2Table(self.player:getCards(flag))
		self:sortByUseValue(card2s, true)
			for _, card in ipairs(card2s) do
				if not self:isValuableCard(card) and (card:getEffectiveId() ~= eid) then return "$"..card:getEffectiveId() end
			end
		end
	return "."
end


--技能：破阵
local pozhen_skill = {}
pozhen_skill.name = "pozhen"
table.insert(sgs.ai_skills, pozhen_skill)
pozhen_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("PozhenCard") then return sgs.Card_Parse("@PozhenCard=.") end
end
sgs.ai_skill_use_func.PozhenCard = function(card,use,self)
	self:sort(self.enemies, "defense")
	self:sort(self.friends, "threat")
	for _, friend in ipairs(self.friends) do
		if friend and not friend:getEquips():isEmpty() and (friend:hasSkills(sgs.lose_equip_skill) or self:needToThrowArmor(friend)) and not self:willSkipPlayPhase(friend) then
			use.card = card
			if use.to then
				use.to:append(friend)
			end
			return
		end
	end
	for _, friend in ipairs(self.friends) do
		if friend and not friend:getEquips():isEmpty() and friend:hasSkills(sgs.lose_equip_skill) then
			use.card = card
			if use.to then
				use.to:append(friend)
			end
			return
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if (not self.player:canSlash(enemy) or not self:needToThrowArmor(enemy) or self.player:distanceTo(enemy) > 1) and not enemy:getEquips():isEmpty() and enemy:getHandcardNum() < 3 then
				use.card = card
				if use.to then use.to:append(enemy) end
				return
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if enemy:getEquips():isEmpty() then continue end
		if (not self.player:canSlash(enemy) or not self:needToThrowArmor(enemy) or self.player:distanceTo(enemy) > 1) and not enemy:hasSkills("kofxiaoji|xiaoji") then
			if not self:hasSuit("spade", true, enemy) then
				sgs.suit_for_ai = 0
				use.card =  card
				if use.to then use.to:append(enemy) end
				return
			end
			if not self:hasSuit("club", true, enemy) then
				sgs.suit_for_ai = 1
				use.card =  card
				if use.to then use.to:append(enemy) end
				return
			end
			if not self:hasSuit("diamond", true, enemy) then
				sgs.suit_for_ai = 3
				use.card =  card
				if use.to then use.to:append(enemy) end
				return
			end
			if not self:hasSuit("heart", true, enemy) then
				sgs.suit_for_ai = 2
				use.card =  card
				if use.to then use.to:append(enemy) end
				return
			end
		end
	end
	local zhugeliang = self.room:findPlayerBySkillName("kongcheng")
	if zhugeliang and not self:isFriend(zhugeliang) and zhugeliang:getHandcardNum() == 0 and not zhugeliang:getEquips():isEmpty() then
		use.card =  card
		if use.to then use.to:append(zhugeliang) end
			return
	end
	if self.player and self.player:isAlive() and not self.player:getEquips():isEmpty() then
		use.card =  card
		if use.to then use.to:append(self.player) end
		return
	end
end
sgs.ai_use_value.PozhenCard = 6.5
sgs.ai_use_priority.PozhenCard = 8

function sgs.ai_skill_suit.pozhen(self)
	local map = {0, 1, 2, 2, 2, 3}
	local suit = map[math.random(1, 6)]
	if sgs.suit_for_ai ~= nil and type(sgs.suit_for_ai) == "number" then return sgs.suit_for_ai end
	return suit
end

sgs.ai_skill_cardask["@pozhen"] = function(self, data, pattern)
	local source = self.room:getTag("pozhen_source"):toPlayer()
	if not source or source:isDead() then return "." end
	if self:isFriend(source) then return "." end
	if source == self.player then return "." end
	if self:isEnemy(source) then
		if not self:isWeak(source) and (self.player:hasSkills(sgs.lose_equip_skill) or self:needToThrowArmor()) then
			return "."
		end
	end
	local suits = pattern:split("|")[2]:split(",")
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByCardNeed(cards)
	for _, card in ipairs(cards) do
		if not self:isValuableCard(card) then
			for _, obname in ipairs(suits) do
				if card:getSuitString() == obname then return "$" .. card:getEffectiveId() end
			end
		end
	end
	return "."
end

--技能：化名
sgs.ai_skill_invoke["huaming"] = function(self,data)
	local source = self.room:getTag("huamingkiller"):toPlayer()
	return source
end

sgs.ai_skill_choice["huaming"] = function(self, choices, data)
	local source = data:toPlayer()
	local orirole = self.role
	local sourole = source:getRole()
	if self:isFriend(source) then
		if sourole == "renegade" then return math.random(0, 1) == 1 and "renegade" or "loyalist" end
		return "rebel"
	else
		if source:isLord() then
			return "loyalist"
		else
			return math.random(0, 1) == 1 and "renegade" or "loyalist"
		end
	end
	return orirole
end

--技能：迫离

sgs.ai_skill_choice.poli = function(self, choices)
	local willdis = false
	local lightning = self:getCard("Lightning")
	if self.player:getMaxHp() == 1 or (lightning and not self:willUseLightning(lightning)) then willdis = true end
	if self:needToThrowArmor() or self:doNotDiscard(self.player) then willdis = true end
	if self:hasSkills(sgs.lose_equip_skill, self.player) and not self.player:getEquips():isEmpty() then willdis = true end
	return willdis and "discard" or "changehero"
end

--技能：追袭


sgs.ai_skill_invoke.zhuixi = true

--技能：急思

function sgs.ai_cardsview_valuable.jisi(self, class_name, player)
	if class_name == "Nullification" then
		local nullnum = 0
		local max_card = self:getMaxCard()
		local cc = self.room:getCurrent()
		if not max_card or player:getMark("jisiused") ~= 0 or not cc then return nil end
		local ccmaxcard = self:getMaxCard(cc)
		if max_card and ccmaxcard and max_card:getNumber() <= ccmaxcard:getNumber() and not self:isFriend(cc) then return nil end
		if self:doNotDiscard(cc, "h") and not self:isFriend(cc) and not self:isWeak() then return nil end
		for _,idx in sgs.qlist(player:handCards()) do
			local card = sgs.Sanguosha:getCard(idx)
			if card and card:objectName() == "nullification" then nullnum = nullnum + 1 end
		end
		if nullnum > 0 then return nil end
		local null = sgs.Sanguosha:cloneCard("nullification")
		if player:isLocked(null) then return nil end
		return "@JisiCard=."
	end
end

--技能：傲才

sgs.ai_skill_invoke.neoaocai = function(self) --没细想
	if self.player:getMark("@neoaocai") <= 0 then return false end
	local lord = self.room:getLord()
	if self.role ~= "rebel" and self:isWeak(lord) then return false end
	for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self:isFriend(player) and self:isWeak(player) and self:getAllPeachNum() == 0 then return false end
	end
	for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if (not self:isFriend(player)) and self:isWeak(player) then return true end
	end
	if self.player:getHp() <= 2 then return true end
	if #self.friends*2 > self.room:alivePlayerCount() then
		 if self:getCardsNum("Peach") > 0 then return true end
	end
	if #self.friends*2 == self.room:alivePlayerCount() then
		 if self:getAllPeachNum() > 0 then return true end
		if self.room:alivePlayerCount() <= 3 then return true end
	end
		return false
end

sgs.ai_skill_choice.neoaocai = function(self, choices)
	local from = self.room:getCurrent()
	if self:isFriend(from) then return "give"
	elseif self:isEnemy(from) then
		local cards = self.player:getCards("he")
		for _, card in sgs.qlist(cards) do
			if (card:isKindOf("AmazingGrace") or self:cardNeed(card) < 3) and not from:hasSkills("qingnang|jijiu|longhun") then return "give" end
		end
		if self:isWeak(from) and not self:isWeak() and self.player:getHandcardNum() >= 4 then return "discard" end
		if self:isWeak(from) and self.player:getHandcardNum() > 7 then return "discard" end
		if self:doNotDiscard(self.player) or self:hasLoseHandcardEffective() or self:getOverflow() > 0 then return "discard" end
		if self.player:hasSkill("kongcheng") or self:needKongcheng(nil, true) then return "discard" end
	end
	local choice_table = choices:split("+")
	return choice_table[math.random(1, #choice_table)]
end
sgs.ai_skill_discard["neoaocai-give"] = function(self, discard_num, min_num, optional, include_equip)
	local to_id
	local from = self.room:getCurrent()
	local cards = sgs.QList2Table(self.player:getCards("he"))
	if #cards < discard_num then return {} end
	self:sortByCardNeed(cards)
	if self:isFriend(from) then to_id = cards[math.random(1, #cards)]:getEffectiveId()
	elseif self:isEnemy(from) then
		if #cards > 0 and #cards <= 2 then to_id = cards[1]:getEffectiveId()
		elseif #cards > 2 then to_id = cards[math.random(1, 2)]:getEffectiveId() end
	end
	local discard = {to_id}
	return discard
end

--技能：专权

function sgs.ai_skill_invoke.zhuanquan(self, data)
	local current = self.room:getCurrent()
	local erzhang = self.room:findPlayerBySkillName("guzheng")
	if erzhang and self:isEnemy(erzhang) then return false end
	if self:isFriend(current) and self:doNotDiscard(current, "h") then return true end
	return self:isEnemy(current) and not self:doNotDiscard(current, "h")
end

--技能：图守

sgs.ai_skill_choice.tushou = function(self, choices)
	local lightning = self:getCard("Lightning")
	local sp = self.player
	local others = self.room:getOtherPlayers(sp)
	if sp:isWounded() and sp:getHandcardNum() == 2 and self:getCardsNum("Peach") == 0 and not self:needToLoseHp() then return "discard" end
	if self:isWeak() and sp:getHandcardNum() > 2 and not self:needToLoseHp() then return "discard" end
	if sp:isWounded() and not self:needToLoseHp() and sp:getCardCount(true) > 2 and (self:needToThrowArmor() or self:doNotDiscard(self.player)) then return "discard" end
	if sp:getMaxHp() > 2 and sp:getMaxHp()-sp:getHandcardNum() > 0 and lightning and sp:aliveCount() < 3 and not self:willSkipPlayPhase() then others:at(0):setFlags("AI_tushouTarget") return "give" end
	if self:isWeak() and sp:getCardCount(true) > 1 and self:willSkipPlayPhase() then return "discard" end
	for _, enemy in ipairs(self.enemies) do
		if (enemy:inMyAttackRange(sp) and self:canAttack(enemy)) or self:canAttack(enemy) then return "cancel" end
	end
	local m = {}
	self:sort(self.friends_noself, "defense")
	for _,p in sgs.qlist(others) do table.insert(m, p:getHp()) end
	local maxhp = math.max(unpack(m))
	for _,p2 in ipairs(self.friends_noself) do
		if p2:getHp() == maxhp then p2:setFlags("AI_tushouTarget") return "give" end
	end
	return "cancel"
end

function SmartAI:getTSCard()
	local card_id
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)
	local lightning = self:getCard("Lightning")
	if lightning and not self:willUseLightning(lightning) then card_id = lightning:getEffectiveId() end
	if not card_id then
	if self:needToThrowArmor() then card_id = self.player:getArmor():getId()
	elseif self.player:getHandcardNum() >= self.player:getHp() then
		for _, acard in ipairs(cards) do
			if (acard:isKindOf("EquipCard") or acard:isKindOf("AmazingGrace") or acard:isKindOf("BasicCard"))
				and self:cardNeed(acard) <= 6 then card_id = acard:getEffectiveId() break
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
		if (card:isKindOf("Armor") and self:needToThrowArmor()) or (card:getId()~=self:getValuableCard(self.player) and not card:isKindOf("Armor")) then
			card_id = card:getEffectiveId() break end
		end
	end
end
	return card_id
end
sgs.ai_skill_use["@@tushou"]=function(self, prompt)

	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)
	local cdid
	local others = self.room:getOtherPlayers(self.player)
	local tar
	for _,card in ipairs(cards) do
		if not isCard("Peach", card, self.player) and not isCard("ExNihilo", card, self.player) then cdid = card:getEffectiveId() break end
	end
	local card_id = self:getTSCard() or cdid

	if not card_id or self.player:isNude() then return "." end
	for _,p in sgs.qlist(others) do
		if not p:hasFlag("AI_tushouTarget") then continue end
		if self:isFriend(p) and not self:needKongcheng(p, true) and not p:hasSkill("manjuan") then tar = p break end
		if self:isFriend(p) and not self:needKongcheng(p, true) then tar = p break end
		if p then tar = p break end
	end
	if tar then return  "@TushouGiveCard="..card_id.."->"..tar:objectName() end
	return "."
end


function sgs.ai_skill_invoke.kangdao(self, data)
	local move = data:toMoveOneTime()
	if not move.from then return false end
	local from = findPlayerByObjectName(self.room, move.from:objectName())
	--local card = sgs.Sanguosha:getCard(move.card_ids:first())
	if not from then return false end
	return self:isFriend(from)
end
sgs.ai_skill_choice.kangdao = function(self)
	local zc = self.room:findPlayerBySkillName("kangdao")
	if zc and self:isFriend(zc) then
		if zc:hasSkill("kongcheng") and zc:isKongcheng() then return "kangdaocancel" end
		if self:needKongcheng(zc, true) then return "kangdaocancel" end
		if self:isWeak() or self:hasSkills(sgs.lose_equip_skill, zc) then return "kangdaogain" end
		if self.player:getPhase() == sgs.Player_NotActive then return "kangdaogain" end
	end
	return math.random(0, 1) == 0 and "kangdaogain" or "kangdaocancel"
end

sgs.ai_skill_cardask["@bushi-discard"] = function(self, data)
	if self.player:isNude() then return "." end
	local aim = data:toPlayer()
	local cards = sgs.QList2Table(self.player:getCards("he"))
	local cards2 = aim:getCards("he")
	self:sortByCardNeed(cards)
	local card = cards[1]
	local rednum = 0
	local blacknum = 0
	local reds = {}
	local blacks = {}
	for _, scard in sgs.qlist(cards2) do
		if scard:isRed() then rednum = rednum + 1 end
	end
	for _, scard in sgs.qlist(cards2) do
		if scard:isBlack() then blacknum = blacknum + 1 end
	end
	for _, acard in ipairs(cards) do
		if acard:isBlack() then table.insert(blacks, acard:getEffectiveId()) end
	end
	for _, acard in ipairs(cards) do
		if acard:isRed() then return table.insert(reds, acard:getEffectiveId()) end
	end
	local str = rednum > blacknum and "$" .. reds[1] or "$" .. blacks[1]
	return  str or "$" .. card:getId()
end

sgs.ai_skill_choice.bushi = function(self, choices, data)
	local aim = data:toPlayer()
	if self:isFriend(aim) or aim:getSeat() == self.player:getSeat() then return "bushiinc" end
	return "bushidec"
end

--[[
--choudu的AI有问题暂时注释掉，此AI会导致杨仪自杀。
--原因分析：choudu获得牌的角色会拥有一个flag，没获得牌就不会有，最早的askForYiji是问一张给一张，换用askForRende之后选完一起给，而AI还是一张一张的问。
--有可能AI里面判断了这个给牌的flag，而askForRende给出去之前多次询问时均没有那个给牌的flag导致问题。
sgs.ai_skill_use["@@choudu"] = function(self, prompt)
	local targets = {}
	local tarnum = self.player:getMark("chouduuse")
	local others = self.room:getOtherPlayers(self.player)
	for _, tar in sgs.qlist(others) do
		if tar:isKongcheng() then continue end
		if self:isFriend(tar) then
			if self:doNotDiscard(tar, "h") and not tar:hasSkill("manjuan") then table.insert(targets, tar:objectName()) end
		else
			if not self:doNotDiscard(tar, "h") then table.insert(targets, tar:objectName()) end
		end
		if #targets >= tarnum then break end
	end
	if #targets == 0 then return "." else return "@ChouduCard=.->" .. table.concat(targets, "+") end
end
sgs.ai_choicemade_filter.cardChosen.choudu = sgs.ai_choicemade_filter.cardChosen.snatch
sgs.ai_skill_askforyiji.choudu = function(self, card_ids)
	local cards = {}
	local target = sgs.SPlayerList()
	for _, card_id in ipairs(card_ids) do table.insert(cards, sgs.Sanguosha:getCard(card_id)) end
	local players = self.room:getTag("choudutargets"):toString():split("+")
	local marknum = self.player:getMark("choudutargets")
	for _,name in ipairs(players) do target:append(findPlayerByObjectName(self.room, name)) end
	for _,player in sgs.qlist(target) do
		if player:hasFlag("chouduselected") then continue end
		if self:isFriend(player) then
			self:sortByCardNeed(cards)
			return player, cards[#cards]:getEffectiveId()
		else
			self:sortByKeepValue(cards)
			if marknum < 2 and self.player:getHp() > 3 then return nil, -1 else return player, cards[1]:getEffectiveId() end
		end
	end
	return nil, -1
end
]]


sgs.ai_skill_cardask["@xuedian"] = function(self, data)
	local x = self.player:getLostHp()
	if self:needToThrowArmor() and self.player:getArmor():isRed() then
		return "$"..self.player:getArmor():getEffectiveId()
	end
	local hcards = sgs.QList2Table(self.player:getCards("h"))
	local ecards = sgs.QList2Table(self.player:getCards("e"))
	self:sortByCardNeed(ecards)
	self:sortByCardNeed(hcards)
	if self:hasSkills(sgs.lose_equip_skill) and not self.player:getEquips():isEmpty() then
		for _, ecard in ipairs(ecards) do
			if ecard:isRed() then return "$"..ecard:getEffectiveId() end
		end
	end
	if x == 1 then
		local card = sgs.Sanguosha:getCard(self.room:getDrawPile():at(0))
		if card:isKindOf("Slash") and not self:slashIsAvailable() then return "." end
		for _, hcard in ipairs(hcards) do
			if hcard:isRed() and self:getUseValue(hcard) < self:getUseValue(card) and not self:isValuableCard(hcard) then return "$"..hcard:getEffectiveId() end
		end
	else
		for _, hcard in ipairs(hcards) do
			if hcard:isRed() and not self:isValuableCard(hcard) then return "$"..hcard:getEffectiveId() end
		end
	end
	return "."
end

function sgs.ai_cardsview.duanhun(self, class_name, player)
	if class_name == "Slash" then
		local cards = player:getCards("he")
		cards = sgs.QList2Table(cards)
		for _, acard in ipairs(cards) do
			if isCard("Slash", acard, player) then return end
		end
		local flag = "he"
		if self:needToThrowArmor() then flag = "eh"  elseif self:getOverflow() > 0 then flag = "h" end
		local cds = sgs.QList2Table(player:getCards(flag))
		local eid = self:getValuableCard(player)
		local newcards = {}
		for _, card in ipairs(cds) do
			if not isCard("Slash", card, player) and not isCard("Peach", card, player) and not (isCard("ExNihilo", card, player) and player:getPhase() == sgs.Player_Play) and (card:getEffectiveId() ~= eid or not eid) then
				table.insert(newcards, card)
			end
		end
		if #newcards < 2 then return end
		self:sortByCardNeed(newcards)
		local card_id1 = newcards[1]:getEffectiveId()
		local card_id2 = newcards[2]:getEffectiveId()
		local card_str = ("slash:duanhun[to_be_decided:0]=%d+%d"):format(card_id1, card_id2)
		return card_str
	end
end
local duanhun_skill = {}
duanhun_skill.name = "duanhun"
table.insert(sgs.ai_skills, duanhun_skill)
duanhun_skill.getTurnUseCard = function(self, inclusive)
	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)
	for _, acard in ipairs(cards) do
		if isCard("Slash", acard, self.player) then return end
	end
	local flag = "he"
	if self:needToThrowArmor() then flag = "eh"  elseif self:getOverflow() > 0 then flag = "h" end
	local cds = sgs.QList2Table(self.player:getCards(flag))
	local eid = self:getValuableCard(self.player)
	self:sortByCardNeed(cds)
	local newcards = {}
	for _, card in ipairs(cds) do
		if not isCard("Slash", card, self.player) and not isCard("Peach", card, self.player) and not (isCard("ExNihilo", card, self.player) and self.player:getPhase() == sgs.Player_Play) and (card:getEffectiveId() ~= eid or not eid) then
			table.insert(newcards, card)
		end
	end
	if #newcards <= self.player:getHp() - 1 and self.player:getHp() <= 4 and not self:hasHeavySlashDamage(self.player)
		and not self.player:hasSkills("kongcheng|lianying|paoxiao|shangshi|noshangshi")
		and not (self.player:hasSkill("zhiji") and self.player:getMark("zhiji") == 0) then return end
	if #newcards < 2 then return end

	local card_id1 = newcards[1]:getEffectiveId()
	local card_id2 = newcards[2]:getEffectiveId()

	if newcards[1]:isBlack() and newcards[2]:isBlack() then
		local black_slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuitBlack)
		local nosuit_slash = sgs.Sanguosha:cloneCard("slash")

		self:sort(self.enemies, "defenseSlash")
		for _, enemy in ipairs(self.enemies) do
			if self.player:canSlash(enemy) and not self:slashProhibit(nosuit_slash, enemy) and self:slashIsEffective(nosuit_slash, enemy)
				and self:canAttack(enemy) and self:slashProhibit(black_slash, enemy) and self:isWeak(enemy) then
				local redcards, blackcards = {}, {}
				for _, acard in ipairs(newcards) do
					if acard:isBlack() then table.insert(blackcards, acard) else table.insert(redcards, acard) end
				end
				if #redcards == 0 then break end

				local redcard, othercard

				self:sortByUseValue(blackcards, true)
				self:sortByUseValue(redcards, true)
				redcard = redcards[1]

				othercard = #blackcards > 0 and blackcards[1] or redcards[2]
				if redcard and othercard then
					card_id1 = redcard:getEffectiveId()
					card_id2 = othercard:getEffectiveId()
					break
				end
			end
		end
	end
	local card_str = ("slash:duanhun[to_be_decided:0]=%d+%d"):format(card_id1, card_id2)
	local slash = sgs.Card_Parse(card_str)
	assert(slash)
	return slash
end



local gudan_skill = {}
gudan_skill.name = "gudan"
table.insert(sgs.ai_skills, gudan_skill)
gudan_skill.getTurnUseCard = function(self)
	local cards = self.player:getHandcards()
	local allcard = {}
	cards = sgs.QList2Table(cards)
	local i
	local qicetrick = "slash|fire_slash|thunder_slash|peach|analeptic"
	local qicetricks = qicetrick:split("|")
	for i=1, #qicetricks do
		local forbiden = qicetricks[i]
		forbid = sgs.Sanguosha:cloneCard(forbiden)
		if self.player:isLocked(forbid) then return end
	end
	if self.player:isKongcheng() then return end

	for _,card in ipairs(cards)  do
		table.insert(allcard, card:getId())
	end

	if self.player:getHandcardNum() <= 3 and self:getCardsNum("Analeptic")==0 and self:slashIsAvailable() and sgs.Analeptic_IsAvailable(self.player) and not self:hasLoseHandcardEffective() then
		local parsed_card = sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "analeptic")
			return parsed_card
	end
	if self.player:getHandcardNum() <= 3 and self:getCardsNum("Jink") == 0 and self:getCardsNum("Peach") == 0 and self:slashIsAvailable() and self:getCardsNum("Slash") == 0 then
		local slashcard = sgs.Sanguosha:cloneCard("slash")
		local slashcard2 = sgs.Sanguosha:cloneCard("fire_slash")
		local slashcard3 = sgs.Sanguosha:cloneCard("thunder_slash")
		local dummyuse = { isDummy = true }
		self:useBasicCard(slashcard, dummyuse)
		if dummyuse.card then return sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "slash") end
		local dummyuse2 = { isDummy = true }
		self:useBasicCard(slashcard2, dummyuse2)
		if dummyuse2.card then return sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "fire_slash") end
		local dummyuse3 = { isDummy = true }
		self:useBasicCard(slashcard3, dummyuse3)
		if dummyuse3.card then return sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "thunder_slash") end
	end


	if not self:hasLoseHandcardEffective() and self:getCardsNum("Peach") == 0 and (not self:needToLoseHp() or self:isWeak()) and self.player:isWounded() then
		local cardx = sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "peach")
		local peachcard = sgs.Sanguosha:cloneCard("peach", cardx:getSuit(), cardx:getNumber())
		local dummy_use = { isDummy = true }
		self:useBasicCard(peachcard, dummy_use)
		if dummy_use.card then return cardx end
	end
	if self.player:getHandcardNum() < 3 and self:getCardsNum("Peach") == 0 and self:isWeak() and self.player:isWounded() then
		local cardx = sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "peach")
		local peachcard = sgs.Sanguosha:cloneCard("peach", cardx:getSuit(), cardx:getNumber())
		local dummy_use = { isDummy = true }
		self:useBasicCard(peachcard, dummy_use)
		if dummy_use.card then return cardx end
	end
	if self:getCardsNum("Peach") == 0 and self.player:getHp() < 2 and not self:needToLoseHp() and self.player:isWounded() then
		local cardx = sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "peach")
		local peachcard = sgs.Sanguosha:cloneCard("peach", cardx:getSuit(), cardx:getNumber())
		local dummy_use = { isDummy = true }
		self:useBasicCard(peachcard, dummy_use)
		if dummy_use.card then return cardx end
	end
	if self:getCardsNum("Peach") == 0 and self.player:getHandcardNum() < 2 and not self:needToLoseHp() and self.player:isWounded() then
		local cardx = sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "peach")
		local peachcard = sgs.Sanguosha:cloneCard("peach", cardx:getSuit(), cardx:getNumber())
		local dummy_use = { isDummy = true }
		self:useBasicCard(peachcard, dummy_use)
		if dummy_use.card then return cardx end
	end

	if self.player:hasSkill("kongcheng") and self:getCardsNum("Peach") == 0 and self.player:isWounded() then
		local cardx = sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "peach")
		local peachcard = sgs.Sanguosha:cloneCard("peach", cardx:getSuit(), cardx:getNumber())
		local dummy_use = { isDummy = true }
		self:useBasicCard(peachcard, dummy_use)
		if dummy_use.card then return cardx end
	end
	if self.player:hasSkill("kongcheng") and self:getCardsNum("Peach") == 0 and sgs.Analeptic_IsAvailable(self.player) then
		local cardx = sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "analeptic")
		local peachcard = sgs.Sanguosha:cloneCard("analeptic", cardx:getSuit(), cardx:getNumber())
		local dummy_use = { isDummy = true }
		self:useBasicCard(peachcard, dummy_use)
		if dummy_use.card then return cardx end
	end
	if self:getCardsNum("Jink") == 0 and self:getCardsNum("Peach") == 0 and self:getCardsNum("Slash") == 0 and self:getCardsNum("Analeptic") == 0 then
		local slashcard = sgs.Sanguosha:cloneCard("slash")
		local slashcard2 = sgs.Sanguosha:cloneCard("fire_slash")
		local slashcard3 = sgs.Sanguosha:cloneCard("thunder_slash")
		local dummyuse = { isDummy = true }
		self:useBasicCard(slashcard, dummyuse)
		if dummyuse.card then return sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "slash") end
		local dummyuse2 = { isDummy = true }
		self:useBasicCard(slashcard2, dummyuse2)
		if dummyuse2.card then return sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "fire_slash") end
		local dummyuse3 = { isDummy = true }
		self:useBasicCard(slashcard3, dummyuse3)
		if dummyuse3.card then return sgs.Card_Parse("@GudanCard=" .. table.concat(allcard, "+") .. ":" .. "thunder_slash") end
	end
	return nil
end

sgs.ai_skill_use_func.GudanCard=function(card,use,self)
	local userstring=card:toString()
	userstring=(userstring:split(":"))[3]
	local guhuocard=sgs.Sanguosha:cloneCard(userstring, card:getSuit(), card:getNumber())
	 self:useBasicCard(guhuocard, use)
	if not use.card then return end
	use.card=card
end
sgs.ai_use_priority.GudanCard = sgs.ai_use_priority.Indulgence - 0.05
sgs.ai_skill_choice.gudan_slash = function(self, choices)
	local str = choices
	choices = str:split("+")
	local slashtables = {}
	if table.contains(choices, "fire_slash") then
		slashtables = {"slash", "fire_slash", "thunder_slash", "fire_slash", "thunder_slash"}
	else
		slashtables = {"slash"}
	end
	sgs_gudan_slash = slashtables[math.random(1, #choices)]
	return sgs_gudan_slash
end
sgs.ai_skill_choice.gudan_saveself = function(self, choices)
	local str = choices
	choices = str:split("+")
	if self.player:hasFlag("Global_PreventPeach") then return "analeptic" end
	local anal = sgs.Sanguosha:cloneCard("analeptic")
	local peach = sgs.Sanguosha:cloneCard("peach")
	if self.player:isLocked(anal) then return "peach" end
	if self.player:isLocked(peach) then return "analeptic" end
	return choices[math.random(1, #choices)]
end
sgs.ai_view_as.gudan = function(card, player, card_place, class_name)
	local allcard = {}
	local cards = sgs.QList2Table(player:getHandcards())
	if #cards == 0 then return nil end
	for _,card in ipairs(cards)  do
		table.insert(allcard, card:getEffectiveId())
	end
	local idstr = table.concat(allcard, "+")
	local jinknum = 0
	local slashnum = 0
	local peachnum = 0
	local analnum = 0
	for _,card in ipairs(cards)  do
		if card:isKindOf("Jink") then jinknum = jinknum + 1
		elseif card:isKindOf("Slash") then slashnum = slashnum + 1
		elseif card:isKindOf("Peach") then peachnum = peachnum + 1
		elseif card:isKindOf("Analeptic") then analnum = analnum + 1 end
	end
	if class_name == "Slash" then
		if player:getHp() < 3 and player:getHandcardNum() < 3 and slashnum == 0 and peachnum == 0 then
			if sgs_gudan_slash then return (sgs_gudan_slash..":".."gudan[to_be_decided:0]".."="..idstr)
			else
				return ("slash:gudan[to_be_decided:0]".."="..idstr)
			end
		end
	elseif	class_name == "Jink" then
		if (player:getHp() < 2 or player:getHandcardNum() < 4) and jinknum == 0 and peachnum == 0 and analnum == 0 then
			return ("jink:gudan[to_be_decided:0]".."="..idstr)
		end
	elseif (class_name == "Peach" and not player:hasFlag("Global_PreventPeach")) then
		if peachnum == 0 and analnum == 0 then
			return ("peach:gudan[to_be_decided:0]".."="..idstr)
		end
	elseif	class_name == "Analeptic" then
		if peachnum == 0 and analnum == 0 then
			return ("analeptic:gudan[to_be_decided:0]".."="..idstr)
		end
	end
end

--勇节 by Fsu0413（没写过几次AI的渣）
sgs.ai_skill_invoke.yongjie = function(self, data)
	local use = data:toCardUse()

	if self.player:getRole() == "rebel" then
		if use.from and use.from:isLord() then
			if self.player:getMaxHp() == 1 then return true end
		end
	end

	if not self:slashIsEffective(use.card or sgs.Sanguosha:cloneCard("Slash", sgs.Card_NoSuit, 0), self.player, false, use.from) then return false end

	local jinknum = 0
	local expectedjink = self:getExpectedJinkNum(use)
	for _, c in sgs.qlist(self.player:getCards("he")) do
		if isCard("Jink", c, self.player) then
			jinknum = jinknum + 1
		end
	end

	if jinknum < expectedjink then --typo
		if self.player:hasSkills("leiji|nosleiji") and self.player:hasSkills(sgs.wizard_harm_skill) then
			if jinknum > 0 then return false end
			if self:hasEightDiagramEffect(self.player) then return false end
		end
		if use.from and (not self:isFriend(use.from)) and self:getAllPeachNum(self.player) < 1 and self.player:getMaxHp() == 1 then
			return true
		end
		return self.player:getLostHp() > 0
	end

	return false
end

sgs.ai_skill_use["@@shangjian"]=function(self, prompt)
	local cards = sgs.QList2Table(self.player:getHandcards())
	local maxs = self.player:getMaxCards()
	local cha = self.player:getHandcards():length() - maxs
	local friends = self:getFriendsNoself()
	self:sortByKeepValue(cards)
	local alls = {}
	local allcard = {}
	local tar
	for _,card in ipairs(cards) do
		table.insert(alls, card)
		if #alls >= cha then break end
	end
	if #alls == 0 or #friends == 0 then return "." end
	for _,card in ipairs(alls)  do
		table.insert(allcard, card:getId())
	end
	self:sort(self.friends_noself, "defense")
	for _, friend in ipairs(self.friends_noself) do
		if (not (self:needKongcheng(friend) or friend:hasSkill("manjuan"))) and friend:isAlive() then tar = friend break end
	end
	local target = tar or self.friends_noself[1]
	if target then return "@ShangjianCard="..table.concat(allcard, "+").."->"..target:objectName() end
	return "."
end

sgs.ai_skill_invoke.manwu = function(self)
	return self:willSkipPlayPhase()
end

sgs.ai_skill_playerchosen.manwu = function(self, targets)
	targets = sgs.QList2Table(targets)
	self:sort(targets, "defense")
	for _, en in ipairs(targets) do
		if not self:doNotDiscard(en,"ej") and self:isEnemy(en) then return en end
		if self:doNotDiscard(en,"ej") and self:isFriend(en) then return en end
	end
	for _, en2 in ipairs(targets) do
		if self:isEnemy(en2) then return en2 end
	end
	return nil
end
sgs.ai_playerchosen_intention.manwu = 30

sgs.ai_skill_invoke.annei = function(self, data)
	local damage = data:toDamage()
	local from = damage.from
	local target = damage.to
	if self:isFriend(target) and (self:isWeak(target) or target:getHp() < 3) and not self:needToLoseHp(target, from) then return true end
	if self:isFriend(target) and self:isFriend(from) and not self:needToLoseHp(target, from) then return true end
	return false
end


sgs.ai_skill_askforag.annei = function(self, card_ids)
	local cards = {}
	for _, card_id in ipairs(card_ids) do
		table.insert(cards, sgs.Sanguosha:getCard(card_id))
	end
	self:sortByCardNeed(cards)
	return cards[1]:getEffectiveId()
end

-- dangliang AI by Fsu0413
sgs.ai_skill_cardask["@dangliang-discard"] = function(self, data)
	local _cards = self.player:getCards("he")
	local cards = {}
	for _, c in sgs.qlist(_cards) do
		if not c:isKindOf("BasicCard") then
			table.insert(cards, c)
		end
	end

	if #cards == 0 then return "." end
	self:sortByKeepValue(cards)
	local use_card = cards[1]
	local to_return = "$" .. cards[1]:getEffectiveId()

	local target = data:toPlayer()
	local invoke = 0 -- 0: not invoke, 1:d2f, -1:d2p

	if self:isFriend(target) then
		if self:willSkipDrawPhase(target) then
			return "."
		end
		if target:hasSkills("yongsi|jiangchi") and target:getHandcardNum() < 4 then
			invoke = 1
		elseif target:hasSkill("longhun") and target:getHp() == 1 then
			invoke = 1
		elseif self:isWeak(target) then
			invoke = 1
		end
	elseif self:isEnemy(target) then
		if self:willSkipPlayPhase(target) then
			return "."
		end
		if self:needKongcheng(target, true) then
			invoke = -1
		elseif target:getHp() == 1 and target:getHandcardNum() <= 3 then
			invoke = -1
		elseif self:isWeak(target) then
			invoke = -1
		end
	else
		return "."
	end

	if (invoke == 0) then
		return "."
	elseif invoke == 1 then
		sgs.ai_skill_choice.dangliang = "d2f"
	elseif invoke == -1 then
		sgs.ai_skill_choice.dangliang = "d2p"
	end

	return to_return
end

function JS_Card(self) --选择一张手牌或装备区的牌
	local card_id = nil
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByCardNeed(cards)
	local lightning = self:getCard("Lightning")
	if lightning and not self:willUseLightning(lightning) then card_id = lightning:getEffectiveId() end
	if not card_id then
		if self:needToThrowArmor() then card_id = self.player:getArmor():getId()
		elseif self.player:getHandcardNum() >= self.player:getHp() then
			for _, acard in ipairs(cards) do
				if (acard:isKindOf("EquipCard") or acard:isKindOf("AmazingGrace") or acard:isKindOf("BasicCard"))
					and self:cardNeed(acard) <= 6 then card_id = acard:getEffectiveId() break
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
				if card:getId() ~= self:getValuableCard(self.player) and not card:isKindOf("Armor") then
					card_id = card:getEffectiveId() break end
			end
		end
	end
	return card_id
end

function DoBeweak(self)
if self.player:getCards("h"):length() - self.player:getMaxCards() > 1 then return false end
local _cards = self.player:getCards("he"):length()
return self:isWeak() and self.player:getHp() < 2 and _cards < 4
end

sgs.ai_compare_funcs.value_sha = function(a, b)
return sgs.getDefenseSlash(a) > sgs.getDefenseSlash(b) end

local jingshang={}
jingshang.name="jingshang"
table.insert(sgs.ai_skills,jingshang)
jingshang.getTurnUseCard=function(self)
	if DoBeweak(self) or self:needBear() then return nil end
	local card_id
	local cardc
	local powers = self.player:getPile("zi")
	if self:getOverflow(target) > 1 then card_id = JS_Card(self)
	elseif powers:length() > 4 then card_id = nil
	else
		card_id = JS_Card(self)
	end
	if card_id then
		local cardstr = ("@JingshangCard=%d"):format(card_id)
		cardc = sgs.Card_Parse(cardstr)
	elseif not card_id and powers:length() > 1 then
		cardc = sgs.Card_Parse("@JingshangCard=.")
	else return nil end
	assert(cardc)
	return cardc
end
sgs.ai_skill_use_func.JingshangCard = function(card,use,self)
if self.player:hasUsed("JingshangCard") then return end
self:sort(self.enemies, "defense")
self:sort(self.friends_noself, math.random(0,1) == 0 and "threat" or "value_sha")
local dingenemy = nil
local dingfriend = self.friends_noself[1]
for index=1,#self.enemies,1 do
local fr = self.enemies[index]
if fr and not fr:isKongcheng() and fr:isAlive() then dingenemy = fr break end
end

for _, enemy3 in ipairs(self.enemies) do
local max1 = self:getMaxCard(dingfriend)
local max2 = self:getMaxCard(enemy3)
local enemy_max_point = max2 and max2:getNumber() or 13
if not max1 then return end
if dingfriend and not dingfriend:isKongcheng() and not enemy3:isKongcheng()
and self:getMaxCard(dingfriend):getNumber() > enemy_max_point then
use.card = card
if use.to then use.to:append(enemy3)
use.to:append(dingfriend) end
return end
end

for _, friend in ipairs(self.friends_noself) do
if not friend:isKongcheng() and self:needKongcheng(friend, true) and dingenemy and not dingenemy:isKongcheng() then
use.card = card
if use.to then use.to:append(dingenemy)
use.to:append(friend) end
return end
end

if dingenemy and dingfriend and not dingenemy:isKongcheng() and not dingfriend:isKongcheng() then
use.card = card
if use.to then use.to:append(dingenemy)
use.to:append(dingfriend) end
return
end

for _, enemy in ipairs(self.enemies) do
if dingenemy and not dingenemy:isKongcheng() and not enemy:isKongcheng() and enemy ~= dingenemy then
use.card = card
if use.to then use.to:append(dingenemy)
use.to:append(enemy) end
return
end
end
end

sgs.ai_use_value.JingshangCard = 6
sgs.ai_use_priority.JingshangCard = sgs.ai_use_priority.Slash + 0.5

sgs.card_ids_length = 0

sgs.ai_skill_askforag.zijun = function(self, card_ids)
	local cards = {}
	for _, card_id in ipairs(card_ids) do
		table.insert(cards, sgs.Sanguosha:getCard(card_id))
	end
	self:sortByCardNeed(cards)
	return cards[1]:getEffectiveId()
end

sgs.ai_skill_choice.zijun = function(self, choices, data)
	local aim = data:toPlayer()
	local powers = self.player:getPile("zi")
	local option = choices:split("+")
	if powers:length() == 0 then return "dismiss" end
	if not self:isFriend(aim) and aim:hasSkill("kongcheng") and aim:isKongcheng() and powers:length() > 1 then return option[2] end
	if self:isFriend(aim) then
		sgs.card_ids_length = sgs.card_ids_length + 1
		if sgs.card_ids_length > 2 then sgs.card_ids_length = 0 return "dismiss" end
		return option[2]
	end
	return "dismiss"
end

sgs.ai_skill_playerchosen["huaiju-rob"] = function(self, targets)
	targets = sgs.QList2Table(targets)
	self:sort(targets, "defense")
	local friends = self:getFriendsNoself()
	if #friends == 0 and self.player:getPile("ju"):length() == 2 then return nil end
	for _, tar in ipairs(targets) do
		if self:isFriend(tar) and not tar:isKongcheng() and self:doNotDiscard(tar, "h") then return tar end
	end
	for _, tar in ipairs(targets) do
		if self:isEnemy(tar) and not tar:isKongcheng() and not self:doNotDiscard(tar, "h") then return tar end
	end
	return nil
end
sgs.ai_playerchosen_intention["huaiju-rob"] = 50

sgs.ai_skill_playerchosen["huaiju-give"] = function(self, targets)
	targets = sgs.QList2Table(targets)
	self:sort(targets, "defense")
	for _, tar in ipairs(targets) do
		if self:isFriend(tar) then return tar end
	end
	for _, tar in ipairs(targets) do
		if self:isEnemy(tar) and self:doNotDiscard(tar, "h") then return tar end
	end
	return targets[1]
end
sgs.ai_playerchosen_intention["huaiju-give"] = -50

sgs.ai_skill_use["@@xingsuan"]=function(self, prompt)
	local clubcards={}
	local justarmor = false
	local loseequip = false
	local maxs = self.player:getMaxCards()
	local cha = self.player:getHandcards():length() - maxs
	if self:needToThrowArmor() then justarmor = true end
	if self:hasSkills(sgs.lose_equip_skill, self.player) then loseequip = true end
	local cards = sgs.QList2Table(self.player:getCards("h"))
	local cards2 = sgs.QList2Table(self.player:getCards("e"))
	self:sortByUseValue(cards, true)
	self:sortByCardNeed(cards2)
	for _,card in ipairs(cards) do
		if card:isKindOf("EquipCard") or card:isKindOf("Slash") then table.insert(clubcards, card:getId()) end
		if #clubcards >= cha then break end
	end
	if justarmor then
		for _, card in ipairs(cards2) do
			if card:isKindOf("Armor") then table.insert(clubcards, card:getId()) break end
		end
	end
	if loseequip then
		for _, card in ipairs(cards2) do
			if (card:isKindOf("Armor") and self:needToThrowArmor()) or card:getId()~=self:getValuableCard(self.player) then
				table.insert(clubcards, card:getId())
			end
		end
	end
	local willput = true
	if not self.player:getPile("tu"):isEmpty() and math.random(0, 4) == 2 then willput = false end
	if #clubcards > 0 and willput then return "@XingsuanCard="..table.concat(clubcards, "+").."->".."." end
	return "."
end
sgs.ai_chaofeng.luji = 4


sgs.ai_skill_discard.fuji = function(self, discard_num, min_num, optional, include_equip)
local source = self.room:getTag("fujiplayer"):toPlayer()
if not source then return {} end
if self:isFriend(source) and not self:getDamagedEffects(source, self.player) then return {} end
if self:isFriend(source) and self:isWeak(source) then return {} end
if self:isEnemy(source) and self:getDamagedEffects(source, self.player) and not self:isWeak(source) then return {} end
if not self.player:canSlash(source, sgs.Sanguosha:cloneCard("slash"), false) then return {} end
if not self:slashIsEffective(sgs.Sanguosha:cloneCard("slash"), source) then return {} end
if self:needToThrowArmor() then return {self.player:getArmor():getId()} end
local to_id
local flag = "h"
if self:hasSkills(sgs.lose_equip_skill, self.player) or self:doNotDiscard(self.player, "e") then local flag = "he" end
local cards = sgs.QList2Table(self.player:getCards(flag))
if #cards < discard_num then return {} end
self:sortByCardNeed(cards)
if cards[1]:isKindOf("Peach") and self.player:isWounded() then return {} end
if #cards > 0 and #cards <= 2 then to_id = cards[1]:getEffectiveId()
elseif #cards > 2 then to_id = cards[math.random(1,2)]:getEffectiveId() end
local discard = {to_id}
return discard
end

function sgs.ai_cardsview_valuable.chanyu(self, class_name, player) --使用此技能导致程序退出？
	if class_name == "Nullification" then
		local nullnum = 0
		for _,idx in sgs.qlist(player:handCards()) do
			local card = sgs.Sanguosha:getCard(idx)
			if card and card:objectName() == "nullification" then nullnum = nullnum + 1 end
		end
		if nullnum > 0 then return nil end
		if player:getHp() > 3 and player:getEquips():isEmpty() then return nil end
		local null = sgs.Sanguosha:cloneCard("nullification")
		if player:isLocked(null) then return nil end
		return "@ChanyuCard=."
	end
end

sgs.ai_skill_use["@@suoshi"]=function(self, prompt)
	if self:isWeak() or self.player:isKongcheng() then return "." end
	local list = self.player:property("suoshitarget"):toString():split("+")
	local targets = {}
	local stargets = {}
	for _,name in ipairs(list) do table.insert(targets, findPlayerByObjectName(self.room, name)) end
	local enemy
	local friend
	local tox
	self:sort(targets, "chaofeng")
	for _,player in ipairs(targets) do
		if self:isFriend(player) then friend = player break end
	end
	for _,player in ipairs(targets) do
		if not self:isFriend(player) then enemy = player break end
	end
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if not table.contains(list, p:objectName()) then table.insert(stargets, p) end
	end
	local cards = sgs.QList2Table(self.player:getCards("h"))
	self:sortByKeepValue(cards)
	local toy
	local cardid = cards[2]:getEffectiveId() or cards[1]:getEffectiveId()
	if friend then
		for _,player in ipairs(stargets) do
			if self:isFriend(player) and not self:isWeak(player) and self:getDamagedEffects(player, self.player) then
				return "@SuoshiCard="..cardid.."->"..player:objectName()
			end
		end
	end
	local willsave = false
	if enemy then
		for _, askill in sgs.qlist(enemy:getVisibleSkillList()) do
			if sgs.ai_slash_prohibit[askill:objectName()] then willsave = true end
		end
		if self.role == "rebel" then
			for _,player in ipairs(stargets) do
				if not self:isFriend(player) and player:isLord() and not self:getDamagedEffects(player, self.player) and not cards[1]:isKindOf("Peach") then
					return "@SuoshiCard="..cards[1]:getEffectiveId().."->"..player:objectName()
				end
			end
			for _,player in ipairs(stargets) do
				if not self:isFriend(player) and player:isLord() and self:isWeak(enemy) and not cards[1]:isKindOf("Peach") then
					return "@SuoshiCard="..cards[1]:getEffectiveId().."->"..player:objectName()
				end
			end
		elseif self.role == "loyalist" then
			for _,player in ipairs(stargets) do
				if self:isFriend(player) or player:isLord() and self:isWeak(enemy) and not self:isWeak(player) and self:getDamagedEffects(player, self.player) then
					return "@SuoshiCard="..cardid.."->"..player:objectName()
				end
			end
		else
			for _,player in ipairs(stargets) do
				if not player then continue end
				if willsave and self:isEnemy(player) and not self:getDamagedEffects(player, self.player) and self:cardNeed(cards[1]) < 7 and not cards[1]:isKindOf("Peach")then
					return "@SuoshiCard="..cards[1]:getEffectiveId().."->"..player:objectName()
				end
			end
			for _,player in ipairs(stargets) do
				if willsave and not self:isEnemy(player) and self:getDamagedEffects(player, self.player) and self:cardNeed(cards[1]) <= 7 then
					return "@SuoshiCard="..cardid.."->"..player:objectName()
				end
			end
		end
		if self:getDamagedEffects(enemy, self.player) then
			for _,player in ipairs(stargets) do
				if self:canAttack(enemy, player) and player:hasSkill("jueqing") and self:isFriend(player) and self:cardNeed(cards[1]) <= 7 then
					return "@SuoshiCard="..cardid.."->"..player:objectName()
				end
			end
			for _,player in ipairs(stargets) do
				if self:canAttack(enemy, player) and player:hasSkill("jueqing") and self:isEnemy(player) and self:cardNeed(cards[1]) <= 6 and not cards[1]:isKindOf("Peach") then
					return "@SuoshiCard="..cards[1]:getEffectiveId().."->"..player:objectName()
				end
			end
		end
		return "."
	end
	return "."
end


sgs.ai_skill_choice.kuxing = function(self, choices, data)
	local damage = data:toDamage()
	local from = damage.from
	if self:isFriend(from) then
		if from:getPhase() ~= sgs.Player_NotActive then return "draw" end
		if from:getPhase() == sgs.Player_NotActive and not self:needKongcheng(from, true) then return "draw" end
		if not self:doNotDiscard(self.player) then return "draw" end
		return "discard"
	else
		local slash_num = getCardsNum("Slash", from)
		if from:getPhase() == sgs.Player_NotActive and self:needKongcheng(from, true) then return "draw" end
		if self:isWeak(from) and not self:isWeak() then return "discard" end
		if self.player:getHandcardNum() > 3 and self.player:getEquips():isEmpty() then return "discard" end
		if self.player:getHandcardNum() > 4 and self.player:getEquips():length() > 1 then return "discard" end
		if (slash_num < 2 or from:getHandcardNum() < 4) and from:hasWeapon("Crossbow") then return "discard" end
		if (self.player:getHandcardNum() > 2 or not self:isWeak()) and self:hasSkills(sgs.cardneed_skill, from) then return "discard" end
		if self:doNotDiscard(self.player) then return "discard" else return "draw" end
	end
	local choice_table = choices:split("+")
	return choice_table[math.random(1, #choice_table)]
end

sgs.ai_skill_invoke.baozheng = function(self)
	if self.player:getMark("@baozheng") <= 0 then return end
	if self:isWeak() and self:getCardsNum("Peach") == 0 then return true end
	local lord = self.room:getLord()
	if self.role == "loyalist" and (sgs.isLordInDanger() or (lord and self:isWeak(lord))) then return end
	if sgs.turncount == 0 then return false end
	local value = 0
	for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self:isFriend(player) then value = value - getFenchengValue(self, player)
		elseif self:isEnemy(player) then value = value + getFenchengValue(self, player) end
	end
	if #self.friends_noself >= #self.enemies and value > 0 then return true end
	for _,enemy in ipairs(self.enemies) do
		if enemy:getWeapon() and enemy:getWeapon():isKindOf("Crossbow") and self:getCardsNum("Slash") > 1 then
			return not self:willSkipPlayPhase()
		elseif self.player:getWeapon() and self.player:getWeapon():isKindOf("Crossbow") and self.player:inMyAttackRange(enemy) and self.player:canSlash(enemy) and self:getCardsNum("Slash") > 1 then
			return not self:willSkipPlayPhase()
		end
	end
	return false
end

sgs.ai_skill_choice.baozheng = function(self, choices)
	local lord = self.room:findPlayerBySkillName("baozheng")
	if not lord then return "rob" end
	if self:needToLoseHp(self.player, lord) or self:getDamagedEffects(self.player, lord) then return "damage" end
	if self:isFriend(lord) then
		if self:getOverflow() >= 0 or self:doNotDiscard(self.player) then return "rob" end
		if not self:damageIsEffective(self.player, sgs.DamageStruct_Normal, lord) then return "damage" end
		return "rob"
	else
		if not self:damageIsEffective(self.player, sgs.DamageStruct_Normal, lord) then return "damage" end
		if self:doNotDiscard(self.player) then return "rob" end
		if self.player:getDefensiveHorse() and not self:isWeak() then return "damage" end
		if (self:getOverflow() > 1 and self.player:getEquips():length() > 1) then return "rob" end
		if self.player:getHp() > 3 and not self.player:isKongcheng() then return "damage" end
		if self:isWeak(lord) and not self:isWeak() and self.player:getHp() >= 3 then return "damage" end
	end
	return "rob"
end


sgs.ai_skill_choice.zongjiu = function(self, choices)
	local lord
	if self.room:getLord() and self.room:getLord():hasSkill("zongjiu") then lord = self.room:getLord() end
	if not lord or self:isEnemy(lord) then return "cancel" end
	if self:needToLoseHp(self.player, lord) or self:getDamagedEffects(self.player, lord) then return "damage" end
	if self:getOverflow() > 0 or self:doNotDiscard(self.player) then return "rob" end
	if not self:damageIsEffective(self.player, sgs.DamageStruct_Normal, lord) then return "damage" end
	if (self.player:getHandcardNum() > 7 and not self:isWeak()) or self.player:getHandcardNum() > 10 then return "rob" end
	if self:hasSkills(sgs.lose_equip_skill) and not self.player:getEquips():isEmpty() then return "rob" end
	return "cancel"
end


function sgs.ai_cardsview.zongjiu(self, class_name, player)
	if class_name == "Analeptic" and player:hasLordSkill("zongjiu") and not player:hasFlag("Global_ZongjiuFailed") and not player:hasUsed("ZongjiuCard") then
		if sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_PLAY and player:getPhase() == sgs.Player_Play then
			return "@ZongjiuCard=.->."
		end
	end
end
sgs.ai_skill_use_func.ZongjiuCard = function(card,use,self)
	use.card = card
end
sgs.dynamic_value.benefit.ZongjiuCard = true

duyi_skill = {}
duyi_skill.name = "duyi"
table.insert(sgs.ai_skills, duyi_skill)
duyi_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("DuyiCard") then return end
	return sgs.Card_Parse("@DuyiCard=.")
end

sgs.ai_skill_use_func.DuyiCard = function(card,use,self)
	use.card = card
end

sgs.ai_skill_playerchosen.duyi = function(self, targets)
	if sgs.ai_duyi then
		local tg = sgs.ai_duyi.tg
		local card = sgs.ai_duyi.id and sgs.Sanguosha:getCard(sgs.ai_duyi.id)
		sgs.ai_duyi = nil
		if card and card:isBlack() and tg then return tg end
	end
	if self:needBear() then return self.player end
	local to
	if self:getOverflow() < 0 then
		to = self:findPlayerToDraw(true)
	else
		to = self:findPlayerToDraw(false)
	end
	if to then return to
	else return self.player
	end
end

sgs.ai_skill_invoke.duanzhi = function(self, data)
	local use = data:toCardUse()
	if self:isEnemy(use.from) and use.card:getSubtype() == "attack_card" and self.player:getHp() == 1 and not self:getCard("Peach")
		and not self:getCard("Analeptic") and not isLord(self.player) and self:getAllPeachNum() == 0 then
		self.player:setFlags("AI_doNotSave")
		return true
	end
	return use.from and self:isEnemy(use.from) and not self:doNotDiscard(use.from, "he", true, 2) and self.player:getHp() > 2
end

sgs.ai_skill_choice.duanzhi = function(self, choices)
	return "discard"
end

