

local shouhui_skill = {}
shouhui_skill.name = "shouhui"
table.insert(sgs.ai_skills, shouhui_skill)
function shouhui_skill.getTurnUseCard(self)
	local ecards={}
	sgs.ai_use_priority.ShouhuiCard =  3
	for _,c in sgs.qlist(self.player:getCards("hs")) do
		if c:isKindOf("EquipCard") then
			if  sgs.ai_use_priority.ShouhuiCard <8  and self:getSameEquip(c) then
				sgs.ai_use_priority.ShouhuiCard = 8
			end
			table.insert(ecards,c)
		end
	end
	if #ecards==0 then return nil end
	self:sortByKeepValue(ecards)
	if #ecards>0 then
		if self:getSameEquip(ecards[1]) then
			local temp={}
			table.insert(temp, ecards[1])
			local equipped
			if ecards[1]:isKindOf("Weapon") then
				equipped= sgs.Sanguosha:getCard(self.player:getEquip(0):getEffectiveId())
			elseif ecards[1]:isKindOf("Armor") then
				equipped= sgs.Sanguosha:getCard(self.player:getEquip(1):getEffectiveId())
			end
			if equipped then
				table.insert(temp, equipped)
			end
			self:sortByUseValue(temp)
			return sgs.Card_Parse("@ShouhuiCard=" .. temp[1]:getEffectiveId())
		end
	end
	return nil
end
sgs.ai_skill_use_func.ShouhuiCard = function(card, use, self)
	use.card=card
	return
end
function sgs.ai_cardsview_valuable.shouhui(self, class_name, player)

	if class_name == "Peach"  then
		local dying = self.room:getCurrentDyingPlayer()
		if not dying or dying:objectName() ~= self.player:objectName() then
			return nil
		end
		local cards=self.player:getCards("hes")
		local ecards={}
		for _,c in sgs.qlist(cards) do
			if c:isKindOf("EquipCard") then
				table.insert(ecards,c)
			end
		end
		if #ecards==0 then return nil end
		self:sortByUseValue(ecards)

		return "@ShouhuiCard=".. ecards[1]:getId()
	end
end



local woyu_skill = {}
woyu_skill.name = "woyu"
table.insert(sgs.ai_skills, woyu_skill)
function woyu_skill.getTurnUseCard(self)
	if self.player:getMark("@woyu")==0 then return nil end
	if self.player:getRole() ~="lord" then return nil end
	return sgs.Card_Parse("@WoyuCard=.")
end
sgs.ai_skill_use_func.WoyuCard = function(card, use, self)
	local avails = {}
	for _, p in pairs (self.enemies) do
		if p:getMark("AI_RoleShown") == 0 then
			table.insert(avails, p)
		end
	end
	if #avails >0 then
		use.card = card
		if use.to then
			use.to:append(avails[1])
			if use.to:length() >= 1 then return end
		end
	end
end
sgs.ai_use_value.WoyuCard = 7
sgs.ai_use_priority.WoyuCard = 7
sgs.ai_skillProperty.woyu = function(self)
	return "noKingdom"
end


sgs.ai_skill_playerchosen.beisha = function(self, targets)
	local num= self.player:getHandcardNum()/2
	targets1={}
	targets2={}
	targets3={}
	local slash= sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
	local slash_eff
	for _,target in sgs.qlist(targets) do
		if self:isEnemy(target) then
			if self:slashIsEffective(slash, target, self.player) then
					local fakeDamage=sgs.DamageStruct()
					fakeDamage.card=slash
					fakeDamage.nature= self:touhouDamageNature(slash,self.player,target)
					fakeDamage.damage=1
					fakeDamage.from=self.player
					fakeDamage.to=target
					slash_eff= self:touhouDamage(fakeDamage,self.player,target).damage>0
			end
			if target:getHandcardNum()> num then
				if slash_eff  then
					table.insert(targets1,target)
				end
			else
				if not self:touhouHpLocked(target) then
					table.insert(targets2,target)
				end
				if sgs.ai_role[target:objectName()] == "rebel" and target:getHp()==1 then
					local shanghai =self.room:findPlayerBySkillName("zhancao")
					if target:hasArmorEffect("EightDiagram")
					and (not self.player:hasWeapon("QinggangSword") ) then
						continue
					end
					if  target:hasSkill("huanshi") then
						continue
					end
					if shanghai and shanghai:getCards("e"):length()>0 and self:isFriend(shanghai,target)
					and (shanghai:inMyAttackRange(target) or shanghai:objectName() == target:objectName()) then
						continue
					end
					if slash_eff then
						table.insert(targets3,target)
					end
				end
			end
		end
	end
	if #targets3>0  then
		self:sort(targets3, "value")
		self.player:setTag("beisha_waak_rebel",sgs.QVariant(true))
		return targets3[1]
	end
	if #targets2>0  then
		self:sort(targets2, "hp")
		return targets2[1]
	end
	if #targets1>0  then
		self:sort(targets2, "value")
		return targets1[1]
	end
	return nil
end
sgs.ai_playerchosen_intention.beisha = 80
sgs.ai_skill_choice.beisha= function(self)
	if self.player:getTag("beisha_waak_rebel")
	and self.player:getTag("beisha_waak_rebel"):toBool() then
		self.player:setTag("beisha_waak_rebel",sgs.QVariant(false))
		return "useslash"
	end
	return "losehp"
end

function keycard_xisan(card)
	if card:isKindOf("Peach") or card:isKindOf("Nullification") then
		return true
	end
	return false
end
sgs.ai_skill_invoke.xisan = function(self, data)
	if self.player:getHandcardNum()<3 then
		return true
	end
	local current=self.room:getCurrent()
	if self:isFriend(current) and current:hasSkill("souji") then
		return true
	end
	cards=self.player:getCards("hs")
	for _,card in sgs.qlist(cards) do
		if keycard_xisan(card) then
			return false
		end
	end
	return true
end
sgs.ai_skill_choice.xisan=function(self)
	local current=self.room:getCurrent()
	if self:isFriend(current) and current:hasSkill("souji") then
		return "b"
	end
	if self.player:getHandcardNum()<3 then
		return "a"
	else
		return "b"
	end
end

sgs.ai_skill_discard.xisan = function(self, discard_num, optional, include_equip)
	cards=self.player:getCards("hs")
	cards_table=sgs.QList2Table(cards)
	self:sortByCardNeed(cards_table)
	local d={}
	throwall=false
	local current=self.room:getCurrent()
	if self:isFriend(current) and current:hasSkill("souji") then
		throwall=true
	end
	for _,card in pairs(cards_table) do
		if not throwall then
			if not keycard_xisan(card) then
				table.insert(d,card:getId())
			end
		else
			table.insert(d,card:getId())
		end
	end

	if #d==0 then
		table.insert(d,cards_table[1]:getId())
	end
	return d
end


sgs.ai_skill_playerchosen.jubao = function(self, targets)
	local enemies={}
	local good_targets={}
	for _,target in sgs.qlist(targets) do
		if self:isEnemy(target) then
			table.insert(enemies,target)
			local  hasLoseEffect = self:getLeastHandcardNum(target) > target:getHandcardNum()-1
			if not hasLoseEffect  and not self:doNotDiscard(target, "hs")   then
				table.insert(good_targets,target)
			end
		end
	end

	local compare_func = function(a, b)
		if (a:getCards("e"):length()>0 or b:getCards("e"):length()>0) then
			return a:getCards("e"):length()> b:getCards("e"):length()
		else
			return a:getHandcardNum()<  b:getHandcardNum()
		end
	end

	if #good_targets>0 then
		table.sort(good_targets, compare_func)
		return good_targets[1]
	end
	if #enemies>0 then
		table.sort(enemies, compare_func)
		return enemies[1]
	end
	return nil
end
sgs.ai_playerchosen_intention.jubao = 60

sgs.ai_skill_invoke.haidi = true



--[[local shanji_skill = {}
shanji_skill.name = "shanji"
table.insert(sgs.ai_skills, shanji_skill)
shanji_skill.getTurnUseCard = function(self, inclusive)
		--如何防止胡乱顶掉武器？？？
		local shanji_cards={}
		local ids=self.player:getPile("piao")
		if ids:length()==0 then return false end
		for _,id in sgs.qlist(ids) do
			card =sgs.Sanguosha:getCard(id)
			if  card:isAvailable(self.player) then
				table.insert(shanji_cards,card)
			end
		end
		if #shanji_cards==0 then return false end
		self:sortByUseValue(shanji_cards, true)
		local acard =  shanji_cards[1]

		local suit =acard:getSuitString()
		local number = acard:getNumberString()
		local card_id = acard:getEffectiveId()
		local slash_str = (acard:objectName()..":shanji[%s:%s]=%d"):format(suit, number, card_id)

		local slash = sgs.Card_Parse(slash_str)

		assert(slash)
		return slash

end
]]

function sgs.ai_cardsview_valuable.shanji(self, class_name, player)
	if (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_UNKNOWN) then
		return nil
	end
	local ids=player:getPile("piao")

	local card
	for _,id in sgs.qlist(ids) do
		card1 =sgs.Sanguosha:getCard(id)
		if card1:isKindOf(class_name)then
			card=card1
			break
		end
	end
	if not card then return nil end
	local suit =card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	return (card:objectName()..":shanji[%s:%s]=%d"):format(suit, number, card_id)
end
sgs.ai_need_bear.shanji = function(self, card,from,tos)
	from = from or self.player
	if self:cautionChangshi() then return false end
	local isPiao = false
	for _,id in sgs.qlist(from:getPile("piao")) do
		if id == card:getEffectiveId() then
			isPiao = true
			break
		end
	end
	if isPiao then
		for _,p in sgs.qlist(tos) do
			if  self:isEnemy(from,p) or self:isFriend(from,p) then
				return false
			end
		end
		return true
	end
	return false
end

local yazhi_skill = {}
yazhi_skill.name = "yazhi"
table.insert(sgs.ai_skills, yazhi_skill)
function yazhi_skill.getTurnUseCard(self)

	if self.player:hasUsed("YazhiCard") then return nil end
	if self:cautionChangshi() then return nil end
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)

	if #cards==0  then
		return nil
	else
		self:sortByKeepValue(cards)
		return sgs.Card_Parse("@YazhiCard=" .. cards[1]:getEffectiveId() )
	end
end
sgs.ai_skill_use_func.YazhiCard=function(card,use,self)
	use.card = card
end

sgs.ai_skillProperty.tianxiang = function(self)
	return "noKingdom"
end



sgs.ai_skill_invoke.qingcang =  function(self,data)
	if #self.enemies==0 then return false end
	if self:willSkipPlayPhase(self.player)  then
		return false
	end
	return true
end


