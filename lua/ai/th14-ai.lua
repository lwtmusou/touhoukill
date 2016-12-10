sgs.ai_skill_cardask["@baochui"] = function(self, data)
	local target = self.room:getCurrent()
	if not self:isFriend(target) then return "." end
	local cards=self.player:getCards("hes")
	cards = sgs.QList2Table(cards)
	if #cards==0 then return "." end
	self:sortByKeepValue(cards)
	return "$" .. cards[1]:getId()
end
sgs.ai_choicemade_filter.cardResponded["@baochui"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		local target = self.room:getCurrent()
		if not target then return end
		sgs.updateIntention(player, target, -80)
	end
end


function SmartAI:yicunEffective(card, to, from)
	if not to:hasSkill("yicun")  then return false end
	if card:isKindOf("Slash")  then
		local id = card:getEffectiveId()
		local minus = 0
		if id ~=-1 and self.room:getCardPlace(id) == sgs.Player_PlaceHand then
			if card:isVirtualCard() then
				for _,cd in sgs.qlist(card:getSubcards()) do
					if self.room:getCardPlace(cd) == sgs.Player_PlaceHand then
						minus = minus + 1
					end
				end
			else
				minus = 1
			end
		end
		if from:getHandcardNum() - minus >= to:getHandcardNum() then
			return true
		end
	end
	return false
end



sgs.ai_skill_invoke.moyi = function(self, data)
		local to =data:toPlayer()
		return self:isFriend(to)
end
sgs.ai_choicemade_filter.skillInvoke.moyi = function(self, player, args)
	local to=player:getTag("moyi-target"):toPlayer()
	if to then
		if args[#args] == "yes" then
			sgs.updateIntention(player, to, -60)
		end
	end
end



--todo：雷霆弃牌丢出了已经装入木牛且移给他人的牌
local leiting_skill = {}
leiting_skill.name = "leiting"
table.insert(sgs.ai_skills, leiting_skill)
function leiting_skill.getTurnUseCard(self)
	if self.player:hasUsed("LeitingCard") then return nil end
	return sgs.Card_Parse("@LeitingCard=.")
end
sgs.ai_skill_use_func.LeitingCard = function(card, use, self)
	local temp
	local hearts={}
	local spades={}
	local cards = self.player:getHandcards()
	for _,c in sgs.qlist(cards) do
		if c:getSuit()==sgs.Card_Heart then
			table.insert(hearts,c)
		elseif c:getSuit()==sgs.Card_Spade then
			table.insert(spades,c)
		end
	end
	cards = sgs.QList2Table(cards)
	if #cards>0 then
		self:sortByKeepValue(cards)
		temp= cards[1]
		self.player:setTag("temp_leiting",sgs.QVariant(temp:getId()))
	end
	local slash = sgs.cloneCard("thunder_slash")
	if temp and temp:getSuit() == sgs.Card_Spade then
		for _,p in pairs (self.friends_noself) do
			for _,e in sgs.qlist(self.room:getOtherPlayers(p)) do
				if self:isEnemy(e) and p:inMyAttackRange(e) and
				p:canSlash(e, slash, true) then
					use.card = card
					if use.to then
						use.to:append(p)
						if use.to:length() >= 1 then return end
					end
				end
			end
		end
	end
	if #self.enemies>0 then
		self:sort(self.enemies, "hp")
		use.card = card
		if use.to then
			use.to:append(self.enemies[1])
			if use.to:length() >= 1 then return end
		end
	end
	use.card = card
	if use.to then
		local temp_target = self.room:getOtherPlayers(self.player):first()
		use.to:append(temp_target)
		if use.to:length() >= 1 then return end
	end
end
sgs.ai_skill_playerchosen.leiting = function(self, targets)
	local slash = sgs.cloneCard("thunder_slash")
	for _,p in pairs (self.enemies) do
		if self.player:inMyAttackRange(p) and
			self.player:canSlash(p, slash, true) then
			return p
		end
	end
	return targets:first()
end
sgs.ai_skill_cardask["@leiting"] = function(self, data)
	local temp =self.player:getTag("temp_leiting"):toInt()
	self.player:removeTag("temp_leiting")
	if temp > -1 and self.room:getCardOwner(temp):objectName() == self.player:objectName()
		and self.room:getCardPlace(temp) == sgs.Player_PlaceHand  then
		return "$" .. temp
	end

	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)
	return "$" .. cards[1]:getId()
end
sgs.ai_cardneed.leiting = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return card:getSuit()==sgs.Card_Heart or card:getSuit()==sgs.Card_Spade
	end
end

sgs.ai_skill_cardask["@nizhuan-self"] = function(self, data)
	if (sgs.ai_skill_invoke.nizhuan(self, data)) then
		local dis = self:askForDiscard("Dummy", 1, 1, false, false)
		return tostring(dis[1])
	end
end
sgs.ai_skill_invoke.nizhuan =function(self,data)
	local use = self.player:getTag("nizhuan_carduse"):toCardUse()
	local to=use.to:first()
	if to then
		return self:isFriend(to)
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.nizhuan = function(self, player, args)
	local use = player:getTag("nizhuan_carduse"):toCardUse()
	local to=use.to:first()
	if  to and  args[#args] == "yes" then
		sgs.updateIntention(player, to, -80)
	end
end
function SmartAI:canNizhuan(player, attacker)
	local seija = self.room:findPlayerBySkillName("nizhuan")
	if not player:isWounded() then return false end
	if player:getLostHp() <= attacker:getLostHp() then return false end
	if seija and self:isFriend(seija,player) and not self:isFriend(player,attacker) and seija:canDiscard(player,"hs") then
		return true
	end
	return false
end


sgs.ai_skill_cardask["@guizha"] = function(self, data)
	for _,card in sgs.qlist(self.player:getCards("hs") ) do
		if card:isKindOf("Peach") then
			return "$" .. card:getId()
		end
	end
	return "."
end



sgs.ai_skill_playerchosen.yuyin = function(self, targets)
	for _,p in sgs.qlist(targets) do
		if self:isEnemy(p) then
			return p
		end
	end
end
sgs.ai_choicemade_filter.cardChosen.yuyin = sgs.ai_choicemade_filter.cardChosen.dismantlement

sgs.ai_skill_invoke.wuchang =function(self,data)
	local current = self.room:getCurrent()
	return self:isEnemy(current)
end
sgs.ai_choicemade_filter.skillInvoke.wuchang = function(self, player, args)
	local current = self.room:getCurrent()
	if current then
		if args[#args] == "yes" then
			sgs.updateIntention(player, current, 10)
		end
	end
end


sgs.ai_skill_playerchosen.canxiang = function(self, targets)
	for _,p in sgs.qlist(targets) do
		if self:isEnemy(p) then
			return p
		end
	end
end
sgs.ai_choicemade_filter.cardChosen.canxiang = sgs.ai_choicemade_filter.cardChosen.dismantlement

sgs.ai_skill_invoke.juwang =function(self,data)
	local current = self.room:getCurrent()
	return self:isEnemy(current)
end
sgs.ai_choicemade_filter.skillInvoke.juwang = function(self, player, args)
	local current = self.room:getCurrent()
	if current and self.player:objectName()~= current:objectName() then
		if args[#args] == "yes" then
			sgs.updateIntention(player, current, 10)
		else
			sgs.updateIntention(player, current, -10)
		end
	end
end




sgs.ai_skill_invoke.langying =function(self,data)
	local slash_source
	local strs=data:toStringList()
	if strs and #strs==2 then
		local str1=(strs[2]:split(":"))[1]
		local str2=(strs[2]:split(":"))[2]
		if str1=="slash-jink" then
			for _,p in sgs.qlist(self.room:getAlivePlayers()) do
				if p:objectName() == str2 then
					slash_source=p
					break
				end
			end
		end
	end
	if slash_source and self:isFriend(slash_source)
	and slash_source:getPhase() == sgs.Player_Play
	and slash_source:hasSkill("sidie")   then
		return false
	end

	local hasDenfense=self.player:getArmor() or self.player:getDefensiveHorse()
	if not hasDenfense then
		return true
	end
	local hasJink= self:getCardsNum("Jink") > 0
	if not hasJink then
		return true
	end
	local current=self.room:getCurrent()
	local enemy_count=0
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if not self:isFriend(p) and self:playerGetRound(p) < self:playerGetRound(self.player)
		and self:playerGetRound(current) < self:playerGetRound(p) then
			enemy_count=enemy_count+1
		end
	end
	if enemy_count>1 then
		return not hasJink
	end
	return true
end


function SmartAI:yuanfeiValue(player)
	local value = 0
	local cards=self.player:getCards("hs")
	cards = self:touhouAppendExpandPileToList(self.player,cards)
	local attackCard
	for _,c in sgs.qlist(cards) do
		if sgs.dynamic_value.damage_card[c:getClassName()] then
			if c:isKindOf("TrickCard") and self:hasTrickEffective(c, player, self.player) then
				attackCard = c
				break
			elseif c:isKindOf("Slash") and self:slashIsEffective(c, player, self.player)  then
				attackCard = c
				break
			end
		end
	end
	if attackCard then
		value = 5 * (10 - player:getHp())
		value = value + (2 * player:getHandcardNum())
		value = value + 2 * player:getPile("wooden_ox"):length()
		if not self.player:inMyAttackRange(player) then
			local distance = self.player:distanceTo(player)
			local equipRange = 1
			for _,c in sgs.qlist(cards) do
				if c:isKindOf("Weapon") and sgs.weapon_range[c:getClassName()] >= distance  then
					value = value + 5
					break
				elseif c:isKindOf("OffensiveHorse") and not self.player:getOffensiveHorse() and distance == 2 then
					value = value + 5
					break
				end
			end
		end
	end
	return value, attackCard
end

local yuanfei_skill = {}
yuanfei_skill.name = "yuanfei"
table.insert(sgs.ai_skills, yuanfei_skill)
function yuanfei_skill.getTurnUseCard(self)
	if self.player:hasUsed("YuanfeiCard") then return nil end
	if #self.enemies==0 then return nil end
	local enemy_table={}
	for _,e in pairs (self.enemies) do
		local array={player = e, value = self:yuanfeiValue(e)}
		table.insert(enemy_table,array)
	end
	local compare_func = function(a, b)
		return a.value > b.value
	end
	table.sort(enemy_table, compare_func)
	if enemy_table[1].value >= 5 then
		local target = enemy_table[1].player
		local _data = sgs.QVariant()
		_data:setValue(target)
		self.player:setTag("yuanfeiAI",_data)
		if not self.player:inMyAttackRange(target) then
			return sgs.Card_Parse("@YuanfeiCard=.")
		else
			local value, card = self:yuanfeiValue(target)
			local cards=self.player:getCards("hs")
			cards = sgs.QList2Table(cards)
			self:sortByUseValue(cards, true)
			for _, c in pairs(cards) do
				if c:getEffectiveId() ~= card:getEffectiveId() then
					return sgs.Card_Parse("@YuanfeiCard="  .. c:getEffectiveId())
				end
			end
		end
	end
	return nil
end
sgs.ai_skill_use_func.YuanfeiCard = function(card, use, self)
	local target = self.player:getTag("yuanfeiAI"):toPlayer()
	if target then
		use.card = card
		if use.to then
			use.to:append(target)
			if use.to:length() >= 1 then return end
		end
	end
end
sgs.ai_skill_use_func.YuanfeiNearCard = function(card, use, self)
	local cards = self.player:getCards("hs")
	local yuanfei= sgs.Card_Parse("@YuanfeiNearCard="..cards:first():getEffectiveId())
	self:sort(self.enemies, "defense")
	for _, p in ipairs(self.enemies) do
		if (self.player:inMyAttackRange(p)) then
			use.card = yuanfei
			if use.to then
				use.to:append(p)
				if use.to:length() >= 1 then return end
			end
		end
	end
end
sgs.ai_use_priority.YuanfeiCard =7
sgs.ai_card_intention.YuanfeiCard = 60
sgs.ai_card_intention.YuanfeiNearCard = 60



sgs.ai_skill_invoke.feitou = true
local feitou_skill = {}
feitou_skill.name = "feitou"
table.insert(sgs.ai_skills, feitou_skill)
feitou_skill.getTurnUseCard = function(self, inclusive)
	if self.player:getPile("feitou"):isEmpty() then return false end
	local ids=self.player:getPile("feitou")
	local card= sgs.Sanguosha:getCard(ids:first())
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local slash_str = ("slash:feitou[%s:%s]=%d"):format(suit, number, card_id)
	local slash = sgs.Card_Parse(slash_str)

	assert(slash)
	return slash
end
function sgs.ai_cardsview_valuable.feitou(self, class_name, player)
	if self:touhouClassMatch(class_name, "Slash") then
		if self.player:getPile("feitou"):isEmpty() then return nil end
		local ids=self.player:getPile("feitou")
		local card= sgs.Sanguosha:getCard(ids:first())
		local suit = card:getSuitString()
		local number = card:getNumberString()
		local card_id = card:getEffectiveId()
		return ("slash:feitou[%s:%s]=%d"):format(suit, number, card_id)
	end
end


sgs.ai_skill_invoke.shizhu =function(self,data)
	return self:getCardsNum("Peach") == 0
end



local liange_skill = {}
liange_skill.name = "liange"
table.insert(sgs.ai_skills, liange_skill)
function liange_skill.getTurnUseCard(self)
	if self.player:hasUsed("LiangeCard") then return nil end
	local cards = sgs.QList2Table(self.player:getCards("hes"))
	if #cards==0 then return nil end
	self:sortByKeepValue(cards)
	local peach

	for _,c in pairs (cards) do
		if c:isKindOf("Peach") then
			peach = c
			break
		end
	end
	if peach and self.player:hasSkill("shizhu") then
		return sgs.Card_Parse("@LiangeCard=" .. peach:getEffectiveId())
	else
		return sgs.Card_Parse("@LiangeCard=" .. cards[1]:getEffectiveId())
	end
end
sgs.ai_skill_use_func.LiangeCard = function(card, use, self)
	local target =self:touhouFindPlayerToDraw(false, 1)
	if not target and #self.friends_noself>0 then
		target= self.friends_noself[1]
	end
	if target then
		use.card = card
		if use.to then
			use.to:append(target)
			if use.to:length() >= 1 then return end
		end
	end
end

sgs.ai_use_value.LiangeCard = 7
sgs.ai_use_priority.LiangeCard = sgs.ai_use_priority.Peach + 0.2
sgs.ai_card_intention.LiangeCard = -70