
sgs.ai_skill_cardask["@jiexiandamage"] = function(self, data)
	local target=data:toDamage().to
	local damage = data:toDamage()
	if not self:isFriend(target) then return "." end
	local isSlash = true
	if not damage.card or not damage.card:isKindOf("Slash") then
		isSlash = false
	end
	if self:needToLoseHp(target, damage.from, isSlash, true)
	or self:getDamagedEffects(target, damage.from, isSlash) then
		return "."
	end
	if damage.damage == 1 then
		if not target:isWounded() then
			local overflow = false
			if target:objectName() == self.player:objectName() then
				overflow = self:getOverflow()+1 >0
			else
				overflow = self:getOverflow() >1
			end
			if not overflow then
				return "."
			end
		end
	end

	local cards=sgs.QList2Table(self.player:getCards("hes"))
	local ecards={}
	for _,card in pairs(cards) do
		if card:getSuit()==sgs.Card_Heart then
			table.insert(ecards,card)
		end
	end

	if #ecards==0 then return "." end
	self:sortByKeepValue(ecards)
	return "$" .. ecards[1]:getId()
end
sgs.ai_skill_cardask["@jiexianrecover"] = function(self, data)
	local target=data:toPlayer()
	if not self:isEnemy(target) then return "." end
	cards =self.player:getCards("hes")
	cards=sgs.QList2Table(cards)
	ecards={}
	for _,card in pairs(cards) do
		if card:getSuit()==sgs.Card_Spade then
			table.insert(ecards,card)
		end
	end

	if #ecards==0 then return "." end
	self:sortByCardNeed(ecards)
	return "$" .. ecards[1]:getId()
end
sgs.jiexian_suit_value = {
	spade = 6,
	heart = 6
}
sgs.ai_cardneed.jiexian = function(to, card, self)
	return card:getSuit()==sgs.Card_Heart or card:getSuit()==sgs.Card_Spade
end
sgs.ai_choicemade_filter.cardResponded["@jiexiandamage"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		local target =player:getTag("jiexian_target"):toPlayer()
		if not target then return end
		sgs.updateIntention(player, target, -80)
	end
end
sgs.ai_choicemade_filter.cardResponded["@jiexianrecover"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		local target =player:getTag("jiexian_target"):toPlayer()
		if not target then return end
		sgs.updateIntention(player, target, 80)
	end
end


local hongwu_skill = {}
hongwu_skill.name = "hongwu"
table.insert(sgs.ai_skills, hongwu_skill)
function hongwu_skill.getTurnUseCard(self)
	if self.player:getMark("@ye")>0 then return nil end
	local handcards = sgs.QList2Table(self.player:getCards("hes"))
	if #handcards==0 then return nil end
	self:sortByUseValue(handcards)
	local hearts={}
	local diamonds={}
	for _,c in pairs(handcards) do
		if c:getSuit()== sgs.Card_Heart then
			table.insert(hearts,c)
		elseif c:getSuit()== sgs.Card_Diamond then
			table.insert(diamonds,c)
		end
	end
	if #hearts + #diamonds<2 then return nil end
	local use_cards={}
	if #diamonds>0 then
		local n1=1
		while (#use_cards < #diamonds and #use_cards<2)do
			table.insert(use_cards,diamonds[n1]:getId())
			n1=n1+1
		end
	end
	if #use_cards<2  then
		local n2=1
		while (#use_cards<2)do
			table.insert(use_cards,hearts[n2]:getId())
			n2=n2+1
		end
	end
	return sgs.Card_Parse("@HongwuCard=" .. table.concat(use_cards, "+") )
end
sgs.ai_skill_use_func.HongwuCard=function(card,use,self)
	use.card = card
end
sgs.ai_use_priority.HongwuCard = 7.6
sgs.ai_cardneed.hongwu = function(to, card, self)
	return card:isRed()
end

local shenqiang_skill = {}
shenqiang_skill.name = "shenqiang"
table.insert(sgs.ai_skills, shenqiang_skill)
function shenqiang_skill.getTurnUseCard(self)
	if self.player:getMark("@ye")==0 then return nil end
	local handcards = sgs.QList2Table(self.player:getCards("hes"))
	if #handcards==0 then return nil end
	self:sortByUseValue(handcards)
	reds={}
	for _,c in pairs(handcards) do
		if c:getSuit()==sgs.Card_Heart or c:isKindOf("Weapon") then
			table.insert(reds,c)
		end
	end
	if #reds==0 then return nil end
	return sgs.Card_Parse("@ShenqiangCard=" .. reds[1]:getId())
end
sgs.ai_skill_use_func.ShenqiangCard = function(card, use, self)
		if #self.enemies==0 then return false end
		self:sort(self.enemies,"hp")
		local enemies = sgs.reverse(self.enemies)
		local target
		for _,p in pairs (enemies)do
			local fakeDamage=sgs.DamageStruct()
			fakeDamage.card=nil
			fakeDamage.nature= sgs.DamageStruct_Normal
			fakeDamage.damage=1
			fakeDamage.from=self.player
			fakeDamage.to=p
			if self:touhouNeedAvoidAttack(fakeDamage,self.player,p) then
				target = p
			end
		end
		if target then
			use.card = card
			if use.to then
				use.to:append(target)
				if use.to:length() >= 1 then return end
			end
		end
end
sgs.ai_cardneed.shenqiang = function(to, card, self)
	return card:getSuit()==sgs.Card_Heart or card:isKindOf("Weapon")
end
sgs.ai_use_priority.ShenqiangCard = 7.6

sgs.ai_damageInflicted.yewang=function(self,damage)
	if damage.to:getMark("@ye") then
		damage.damage=damage.damage-1
	end
	return damage
end



sgs.ai_damageCaused.bingfeng = function(self, damage)
	if damage.card then
		if  damage.to:getMark("@ice") > 0 then
			damage.damage = math.max(0, damage.damage -1)
		end
	end
	return damage
end


sgs.ai_cardneed.shikong = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  (not to:getWeapon() and  getCardsNum("Weapon",to,self.player)<1 and card:isKindOf("Weapon"))
		or (not to:getOffensiveHorse() and  getCardsNum("OffensiveHorse",to,self.player)<1 and card:isKindOf("OffensiveHorse"))
	end
end

function SmartAI:isShikongTarget(to, card, self)

end

sgs.ai_skill_askforag.cuixiang = function(self, card_ids)
	local ids = card_ids
	local cards = {}
	for _, id in ipairs(ids) do
		table.insert(cards, sgs.Sanguosha:getCard(id))
	end
	local needcards ={}
	for _, card in ipairs(cards) do
		if self.player:hasSkill("xuying") and sgs.ai_cardneed.xuying(self.player, card, self) then
			return card:getEffectiveId()
		end
		if sgs.dynamic_value.damage_card[card:getClassName()] then
			table.insert(needcards, card)
		end

		if card:isKindOf("Peach") then
			lord = self.room:getLord()
			if lord and lord:isAlive() and self:isFriend(lord) and not self.player:isLord() then
				table.insert(needcards, card)
			end
		end
		if card:isKindOf("ExNihilo") then
			table.insert(needcards, card)
		end
		if card:isKindOf("Weapon") and self:getCardsNum("Weapon")==0 then
			table.insert(needcards, card)
		end
		if card:isKindOf("Armor") and not card:isKindOf("SilverLion")
		and self:getCardsNum("Armor")==0 then
			table.insert(needcards, card)
		end
	end

	if #needcards>0 then
		self:sortByCardNeed(needcards)
		return needcards[#needcards]:getEffectiveId()
	end
	self:sortByCardNeed(cards)
	return cards[#cards]:getEffectiveId()
end


sgs.ai_skill_discard.xuying = sgs.ai_skill_discard.gamerule
sgs.ai_cardneed.xuying = function(to, card, self)
	return getCardsNum("Jink", to, self.player) <2
	 and card:isKindOf("Jink")
end



sgs.ai_skill_invoke.kuangyan = function(self,data)
	if self.player:getMark("@kinki")>0 then return true end
	current=self.room:getCurrent()
	if current and self:isFriend(current) and current:getHp()<3 then
		return false
	end
	return true
end
sgs.ai_slash_prohibit.kuangyan = function(self, from, to, card)
	if to:hasSkill("kuangyan")  and to:getHp()==1 then
		local current=self.room:getCurrent()
		if  current and current:isAlive()  and self:isEnemy(current,to)  then
			if self:isFriend(from,to) then
				return false
			else
				return true
			end
		end
	end
	return false
end

local huimie_skill = {}
huimie_skill.name = "huimie"
table.insert(sgs.ai_skills, huimie_skill)
function huimie_skill.getTurnUseCard(self)
	if self.player:hasUsed("HuimieCard") then return nil end
	return sgs.Card_Parse("@HuimieCard=.")
end
sgs.ai_skill_use_func.HuimieCard = function(card, use, self)
		self:sort(self.enemies,"hp",true)
		local targets={}
		for _, p in ipairs(self.enemies) do
			if not p:isChained()  then
				local fakeDamage=sgs.DamageStruct()
				fakeDamage.card=nil
				fakeDamage.nature= sgs.DamageStruct_Fire
				fakeDamage.damage=1
				fakeDamage.from=self.player
				fakeDamage.to=p
				local isEffective = self:touhouNeedAvoidAttack(fakeDamage, self.player, p)
				if not isEffective then continue end
				table.insert(targets,p)
			end
		end
		if #targets == 0 then
			for _, p in ipairs(self.enemies) do
				if p:isChained()  then
					local fakeDamage=sgs.DamageStruct()
					fakeDamage.card=nil
					fakeDamage.nature= sgs.DamageStruct_Fire
					fakeDamage.damage=1
					fakeDamage.from=self.player
					fakeDamage.to=p
					local isEffective = self:touhouNeedAvoidAttack(fakeDamage, self.player, p)
					if not isEffective then continue end
					table.insert(targets,p)
				end
			end
		end


		if #targets >0 then
			use.card = card
			if use.to then
				use.to:append(targets[1])
				if use.to:length() >= 1 then return end
			end
		end
end

--function SmartAI:isWeak
sgs.ai_skill_discard.jinguo = function(self, discard_num, min_num, optional, include_equip)
	if optional then
		return {}
	end
	local flag = "hs"
	local equips = self.player:getEquips()
	if include_equip and not (equips:isEmpty() or self.player:isJilei(equips:first())) then flag = flag .. "e" end
	local cards = self.player:getCards(flag)
	local to_discard = {}
	cards = sgs.QList2Table(cards)
	local aux_func = function(card)
		local place = self.room:getCardPlace(card:getEffectiveId())
		if place == sgs.Player_PlaceEquip then
			if card:isKindOf("SilverLion") then
				local players = self.room:getOtherPlayers(self.player)
				for _,p in sgs.qlist(players) do
					local blade = p:getWeapon()
					if blade and blade:isKindOf("GudingBlade") then
						if p:inMyAttackRange(self.player) then
							if self:isEnemy(p, self.player) then
								return 6
							end
						else
							break --因为只有一把古锭刀，检测到有人装备了，其他人就不会再装备了，此时可跳出检测。
						end
					end
				end
				if self.player:isWounded() then
					return -2
				end
			elseif card:isKindOf("Weapon") and self.player:getHandcardNum() < discard_num + 2 and not self:needKongcheng() then return 0
			elseif card:isKindOf("OffensiveHorse") and self.player:getHandcardNum() < discard_num + 2 and not self:needKongcheng() then return 0
			elseif card:isKindOf("OffensiveHorse") then return 1
			elseif card:isKindOf("Weapon") then return 2
			elseif card:isKindOf("DefensiveHorse") then return 3
			elseif card:isKindOf("Armor") then
				return 4
			end
		elseif self:hasSkills(sgs.lose_equip_skill) then
			return 5
		else
			return 0
		end
		return 0
	end
	local compare_func = function(a, b)
		if aux_func(a) ~= aux_func(b) then return aux_func(a) < aux_func(b) end
		return self:getKeepValue(a) < self:getKeepValue(b)
	end

	table.sort(cards, compare_func)
	local least = min_num
	if discard_num - min_num > 1 then
		least = discard_num -1
	end
	for _, card in ipairs(cards) do
		if not self.player:isJilei(card) then
			table.insert(to_discard, card:getId())
		end
		if #to_discard >= discard_num then
			break
		end
	end
	return to_discard
end

function shijian_attack(self)
	for _,p in pairs (self.enemies) do
		if self:isWeak(p) and self:canAttack(p) then
			return true
		end
	end
	return false
end

sgs.ai_skill_invoke.shiting = function(self,data)
	local current = self.room:getCurrent()
	local nexter =  self.room:nextPlayer(current) --current:getNextAlive()
	--shicao_find_adjacent(self.room,self.player)
	if nexter:hasSkill("changshi") then return true end
	if self.player:objectName()== nexter:objectName() then
		return true
	end
	if self:isWeak(self.player) then return true end
	return shijian_attack(self)
end

sgs.ai_skill_invoke.huanzai = function(self,data)
	if self.player:getMark("@clock") > 0 then return false end
	if self:isWeak(self.player) then return true end
	return shijian_attack(self)
end

sgs.ai_skill_invoke.shanghun = function(self,data)
	if self.player:getMark("@clock") > 0 then return false end
	if self:isWeak(self.player) then return true end
	return shijian_attack(self)
end


sgs.ai_skill_choice.banling_plus=function(self, choices)
	local x=self.player:getLingHp()
	local y=self.player:getRenHp()
	if x > y then
		return "rentili"
	end
	return "lingtili"
end

sgs.ai_skill_choice.banling_minus=function(self, choices)
	local x=self.player:getLingHp()
	local y=self.player:getRenHp()
	if x > y then
		return  "lingtili"
	end
	return "rentili"
end

--function getBestHp(player)
sgs.ai_skill_invoke.rengui = true
sgs.ai_skill_playerchosen.renguidiscard = function(self, targets)
	target_table = sgs.QList2Table(targets)
	if #target_table==0 then return nil end
	self:sort(target_table, "handcard")
	for _,target in pairs(target_table) do
		if self:isEnemy(target) then
			return target
		end
	end
	return nil
end
sgs.ai_skill_playerchosen.renguidraw = function(self, targets)
	self:sort(self.friends,"handcard")
	return self.friends[1]
end
sgs.ai_playerchosen_intention.renguidraw = -60
sgs.ai_playerchosen_intention.renguidiscard = 60




sgs.ai_trick_prohibit.gaoao = function(self, from, to, card)
	if not card:isKindOf("DelayedTrick")  then return false end
	if self:isFriend(from,to) then return false end
	--回合内可能被【船难】【葛笼】
	return to:getPhase() == sgs.Player_NotActive
end

sgs.ai_skill_property.gaoao = { effect = {{"AntiDrawEffect"}},
	trigger =       {{"NotActive"}},
	target =        {{"SkillOwner"}},
}


--sgs.ai_card_intention.Slash
sgs.ai_skill_choice.shenshou=function(self, choices)
	local x=self.player:getTag("shenshou_x"):toInt()
	local y=self.player:getTag("shenshou_y"):toInt()
	local z=self.player:getTag("shenshou_z"):toInt()
	local target=self.player:getTag("shenshou_target"):toPlayer()
	local e={}
	local e1={}
	for _,p in sgs.qlist(self.room:getOtherPlayers(target)) do
		if not self:isFriend(p) and target:inMyAttackRange(p) then
			table.insert(e,p)
			if target:canSlash(p,nil,true) then
				table.insert(e1,p)
			end
		end
	end
	if z>0 then
		return "shenshou_draw"
	end
	if y>0 then
		if #e>0 then return "shenshou_obtain" end
	end
	if x>0 then
		if #e1>0 then return "shenshou_slash" end
	end
	return "cancel"
end
local shenshou_skill = {}
shenshou_skill.name = "shenshou"
table.insert(sgs.ai_skills, shenshou_skill)
function shenshou_skill.getTurnUseCard(self)
	if self.player:hasUsed("ShenshouCard") then return nil end
	local handcards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(handcards)
	local cards={}
	for _,card in pairs (handcards) do
		if card:getNumber()<10 and card:getNumber()>4 then
			table.insert(cards,card)
		end
	end
	for _,card in pairs (handcards) do
		if (card:isKindOf("Slash") or card:getSuit()==sgs.Card_Spade)and not table.contains(cards,card) then
			table.insert(cards,card)
		end
	end
	if #cards>0 then
		sgs.ai_use_priority.ShenshouCard = 9.2
		return sgs.Card_Parse("@ShenshouCard=" .. cards[1]:getEffectiveId() )
	end
	if #handcards>0 then
		if #handcards == 1 then
			sgs.ai_use_priority.ShenshouCard = 9.2
		end
		return sgs.Card_Parse("@ShenshouCard=" .. handcards[1]:getEffectiveId())
	end
	return nil
end
sgs.ai_skill_use_func.ShenshouCard = function(card, use, self)
		self:sort(self.friends_noself,"handcard")
		if #self.friends_noself >0 then
			use.card = card
			if use.to then
				use.to:append(self.friends_noself[1])
				if use.to:length() >= 1 then return end
			end
		end
end
sgs.ai_skill_playerchosen.shenshou = function(self, targets)
	targets=sgs.QList2Table(targets)
	self:sort(targets,"hp")
	local shenshou
	for _,p in pairs(targets) do
		if self:isEnemy(p) then
			shenshou=p
		end
	end
	if shenshou then
		return shenshou
	end
	return nil
end
sgs.shenshou_suit_value = {
	spade = 5
}
sgs.ai_playerchosen_intention.shenshou = 40
sgs.ai_use_value.ShenshouCard = 9.2
sgs.ai_use_priority.ShenshouCard = 0.2
sgs.ai_cardneed.shenshou = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  card:isKindOf("Slash") or card:getSuit()==sgs.Card_Spade
		or (card:getNumber()>4 and card:getNumber()<9)
	end
end



function SmartAI:isRealFriend(player)
	if self.player:getRole() == "renegade" then
		return false
	end

	if not player:hasShownRole() then
		return false
	end

	local role1 = self.player:getRole()
	local role2 = player:getRole()
	if (role1 == role2) then
		return true
	elseif (role1 == "loyalist" and role2 == "lord") or (role1 == "lord" and role2 == "loyalist")  then
		return true
	end
	return false
end

function SmartAI:gameBalance()
	local rene_num = 0
	local rebel_num = 0
	local loyal_num = 0
	local banlance_table={}


	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:hasShownRole() then
			local role = p:getRole()
			if (role == "loyalist" or  role2 == "lord") then

				loyal_num = loyal_num + 1
			elseif role == "rebel" then
				rebel_num = rebel_num + 1
			else
				rene_num = rene_num + 1
			end
		end
	end
	local diffLN = loyal_num - rene_num
	local diffLR = loyal_num - rebel_num
	local diffRN = rebel_num - rene_num

	local canBalance = {}

	if loyal_num == rene_num and rebel_num == rene_num then
		table.insert(canBalance, "rebel")
		table.insert(canBalance, "renegade")
		table.insert(canBalance, "loyalist")
	else
		if  math.abs(diffLN) == 1 then
			if diffLN > 0 then
				table.insert(canBalance, "loyalist")
			else
				table.insert(canBalance, "renegade")
			end
		end
		if  math.abs(diffLR) == 1 then
			if diffLR > 0 then
				table.insert(canBalance, "loyalist")
			else
				table.insert(canBalance, "rebel")
			end
		end
		if  math.abs(diffRN) == 1 then
			if diffRN > 0 then
				table.insert(canBalance, "rebel")
			else
				table.insert(canBalance, "renegade")
			end
		end
	end

	return canBalance

end

local fengyin_skill = {}
fengyin_skill.name = "fengyin"
table.insert(sgs.ai_skills, fengyin_skill)
function fengyin_skill.getTurnUseCard(self)
	if self.player:hasUsed("FengyinCard") then return nil end
	local cards={}
	for _,card in sgs.qlist(self.player:getCards("hes")) do
		if card:getSuit() == sgs.Card_Heart then
			table.insert(cards,card)
		end
	end
	self:sortByUseValue(cards, true)
	if #cards>0 then
		return sgs.Card_Parse("@FengyinCard=" .. cards[1]:getEffectiveId() )
	end
	return nil
end
sgs.ai_skill_use_func.FengyinCard = function(card, use, self)
	local roles = {"rebel", "renegade", "loyalist"}
	if self:hasSkill("huanxiang") then
		roles = self:gameBalance()
	end

	local avails ={}
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:hasShownRole() then
			if table.contains(roles, p:getRole()) or (p:isLord() and table.contains(roles, "loyalist")) then
				if self:isEnemy(p)  and p:faceUp() then
					table.insert(avails, p)
				end
				if  self:isFriend(p)  and not p:faceUp() then
					table.insert(avails, p)
				end
			end
		end
	end

	self:sort(avails,"handcard")
	if #avails >0 then
		use.card = card
		if use.to then
			use.to:append(avails[1])
			if use.to:length() >= 1 then return end
		end
	end
end
sgs.ai_card_intention.FengyinCard = function(self, card, from, tos)
	local to = tos[1]
	if to:faceUp() then
		sgs.updateIntention(from, to, 100)
	else
		sgs.updateIntention(from, to, -30)
	end
end


sgs.ai_cardneed.fengyin = function(to, card, self)
	--应该判断若到了残局，不是那么需要红桃
	return  card:getSuit() == sgs.Card_Heart
end

 sgs.ai_skill_invoke.yibian = function(self,data)
	if self.player:getRole() == "renegade" then
		return true
	 end

	local rebel_num = sgs.current_mode_players["rebel"]
	 if self.player:getRole() == "rebel" and rebel_num >1 then
		return true
	end

	 local loyalist_num = sgs.current_mode_players["loyalist"]
	 if (self.player:getRole() == "loyalist" or self.player:isLord()) and rebel_num >1 then
		 return true
	 end
	return false
 end

 sgs.ai_skill_playerchosen.yibian = function(self, targets)
	for _,p in sgs.qlist(targets) do
	if p:getRole() == "renegade" or (not self:isEnemy(p))  then
			 return p
		end
	end
	return targets:first()
 end

sgs.ai_skill_askforyiji.yibian = function(self, card_ids)
	if self.player:getRole() == "renegade" then return  nil, -1 end

	local available_friends = {}

	 for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		 if not p:hasShownRole() then continue end
		if self:isRealFriend(p) then
			table.insert(available_friends, p)
		end
	 end

	 if #available_friends == 0 then return nil, -1 end

	 local toGive, allcards = {}, {}
	 local keep
	 for _, id in ipairs(card_ids) do
		 local card = sgs.Sanguosha:getCard(id)
		 if not keep and (isCard("Jink", card, self.player) or isCard("Analeptic", card, self.player)) then
			 keep = true
		 else
			 table.insert(toGive, card)
		end
		 table.insert(allcards, card)
	 end

	 local cards = #toGive > 0 and toGive or allcards
	 self:sortByKeepValue(cards, true)
	 local id = cards[1]:getId()

	local card, friend = self:getCardNeedPlayer(cards)
	if card and friend and table.contains(available_friends, friend) then return friend, card:getId() end

	if #available_friends > 0 then
		 self:sort(available_friends, "handcard")
		 for _, afriend in ipairs(available_friends) do
			if not self:needKongcheng(afriend, true) then
			 return afriend, id
			 end
		 end
		 self:sort(available_friends, "defense")
		 return available_friends[1], id
	end
	 return nil, -1
end
sgs.ai_Yiji_intention.yibian = -30




sgs.ai_skill_invoke.quanjie = function(self,data)
	local target=data:toPlayer()
	if self:isEnemy(target) then
		if target:getHandcardNum()<5 then
			for _,p in sgs.qlist(self.room:getAlivePlayers()) do
				if self:isFriend(p) and self:isWeak(p) and target:inMyAttackRange(p) then
					return true
				end
			end
		end
	end
	return false
end
sgs.ai_skill_cardask["@quanjie-discard"] = function(self, data)
	local num = getCardsNum("Slash", self.player, self.player)
	if num < 2 then return "." end
	local slashs = {}
	for _,c in sgs.qlist(self.player:getCards("hs")) do
		if c:isKindOf("Slash") then
			table.insert(slashs, c)
		end
	end
	if #slashs > 0 then
		self:sortByKeepValue(slashs)
		return "$" .. slashs[1]:getId()
	end
	return "."
end


sgs.ai_skill_invoke.duanzui = true


local huaxiang_skill = {}
huaxiang_skill.name = "huaxiang"
table.insert(sgs.ai_skills, huaxiang_skill)
huaxiang_skill.getTurnUseCard = function(self)
	local current = self.room:getCurrent()
	if not current or current:isDead() or current:getPhase() == sgs.Player_NotActive then return end

	local cards = self.player:getCards("hs")
	--cards=self:touhouAppendExpandPileToList(self.player,cards)
	local validCards = {}
	for _,c in sgs.qlist(cards) do
		local can = true
		for _,id in sgs.qlist(self.player:getPile("rainbow")) do
			if c:getSuit() == sgs.Sanguosha:getCard(id):getSuit() then
				can = false
				break
			end
		end
		if can then
			table.insert( validCards, c)
		end
	end

	if #validCards == 0 then return nil end
	self:sortByKeepValue(validCards)




	local huaxiangCards = {}
	local huaxiang = "slash|analeptic"
	local ban = table.concat(sgs.Sanguosha:getBanPackages(), "|")
	if not ban:match("maneuvering") then huaxiang = huaxiang .. "|analeptic|thunder_slash|fire_slash" end
	if not ban:match("test_card") then huaxiang = huaxiang .. "|magic_analeptic|light_slash|iron_slash|power_slash" end
	if self.player:getMaxHp() <= 2 then
		huaxiang = huaxiang .. "|peach"
		if not ban:match("test_card") then huaxiang = huaxiang .. "|super_peach" end
	end


	local huaxiangs = huaxiang:split("|")
	for i = 1, #huaxiangs do
		local forbidden = huaxiangs[i]
		local forbid = sgs.cloneCard(forbidden)
		if not self.player:isLocked(forbid) and self:canUseXihuaCard(forbid, true) then
			table.insert(huaxiangCards,forbid)
		end
	end



	self:sortByUseValue(huaxiangCards, false)
	for _,huaxiangCard in pairs (huaxiangCards) do
		local dummyuse = { isDummy = true }
		self:useBasicCard(huaxiangCard, dummyuse)
		if dummyuse.card then
			fakeCard = sgs.Card_Parse("@HuaxiangCard=" .. validCards[1]:getId() .. ":" .. huaxiangCard:objectName())
			return fakeCard
		end
	end
	return nil
end
sgs.ai_skill_use_func.HuaxiangCard=function(card,use,self)
	local userstring=card:toString()
	userstring=(userstring:split(":"))[3]
	local huaxiangcard=sgs.cloneCard(userstring)
	huaxiangcard:setSkillName("huaxiang")
	self:useBasicCard(huaxiangcard, use)
	if not use.card then return end
	use.card=card
end


function sgs.ai_cardsview_valuable.huaxiang(self, class_name, player)
	if (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_UNKNOWN) then
		return nil
	end
	if self.player:getPile("rainbow"):length() > 3 then
		return nil
	end


	local cards = self.player:getCards("hs")
	--cards=self:touhouAppendExpandPileToList(self.player,cards)
	local validCards = {}
	for _,c in sgs.qlist(cards) do
		local can = true
		for _,id in sgs.qlist(self.player:getPile("rainbow")) do
			if c:getSuit() == sgs.Sanguosha:getCard(id):getSuit() then
				can = false
				break
			end
		end
		if can then
			table.insert( validCards, c)
		end
	end

	if #validCards == 0 then return nil end
	self:sortByKeepValue(validCards)

	local classname2objectname = {
		["Slash"] = "slash",  ["Analeptic"] = "analeptic", ["MagicAnaleptic"] = "magic_analeptic", 
		["FireSlash"] = "fire_slash", ["ThunderSlash"] = "thunder_slash"
		, ["LightSlash"] = "light_slash", ["PowerSlash"] = "power_slash", ["IronSlash"] = "iron_slash"
	}
	if self.player:getMaxHp() <= 3 then
		classname2objectname["Jink"] = "jink"
		classname2objectname["ChainJink"] = "chain_jink"
		classname2objectname["LightJink"] = "light_jink"
	end
	if self.player:getMaxHp() <= 2 then
		classname2objectname["Peach"] = "peach"
		classname2objectname["SuperPeach"] = "super_peach"
	end
	if self.player:getMaxHp() <= 1 then
		classname2objectname["Nullification"] = "nullification"
	end
	if classname2objectname[class_name] then
		local viewcard = sgs.cloneCard(classname2objectname[class_name])
		if self.player:isLocked(viewcard) then
			return nil
		end
		return "@HuaxiangCard=" .. validCards[1]:getEffectiveId() .. ":" .. classname2objectname[class_name]
	end
end




sgs.ai_skill_invoke.caiyu = function(self,data)
	local prompt = data:toString()
	if prompt:match("discard") then
		return true
	elseif prompt:match("loseMaxHp") then
		return self.player:getMaxHp() > 1
	end
	return false
end

function caiyuValue(self, card)
	local hands = self.player:getCards("hs")
	local num = 0
	for _,c in sgs.qlist(hands) do
		if c:getSuit() == card:getSuit() then
			num = num + 1
		end
	end
	local value = 1/num
	return self:getKeepValue(card)* value
end
sgs.ai_skill_discard.caiyu = function(self,discard_num)
	local hands = self.player:getCards("hs")
	local card_table={}
	for _,c in sgs.qlist(hands) do
		local array={card = c, value= caiyuValue(self, c)}
		table.insert(card_table,array)
	end
	local compare_func = function(a, b)
		return a.value > b.value
	end
	table.sort(card_table, compare_func)

	local to_discard = {}
	for var=1, discard_num ,1 do
		table.insert(to_discard, card_table[var].card:getId())
	end
	return to_discard
end


sgs.ai_skill_invoke.xuanlan = true

sgs.ai_choicemade_filter.cardResponded["@qinlue-discard"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		local current=self.room:getCurrent()
		sgs.updateIntention(player, current, 80)
	end
end
sgs.ai_skill_cardask["@qinlue-discard"] = function(self, data)
	local current = data:toPlayer()

	if not self:isEnemy(current)  then return "." end
	local cards={}

	for _,c in sgs.qlist(self.player:getCards("hes")) do
		if c:isKindOf("Slash") or c:isKindOf("Weapon") then
			if self.player:canDiscard(self.player,c:getId()) then
				table.insert(cards,c)
			end
		end
	end
	if #cards==0 then return "." end
	self:sortByKeepValue(cards)
	return "$" .. cards[1]:getId()
end

sgs.ai_cardneed.qinlue = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  card:isKindOf("Slash") or card:isKindOf("EquipCard")
	end
end
sgs.qinlue_keep_value = {
	Slash           = 7.4,
	EquipCard = 6.5
}

local chaoren_skill = {}
chaoren_skill.name = "chaoren"
table.insert(sgs.ai_skills, chaoren_skill)
chaoren_skill.getTurnUseCard = function(self, inclusive)
		local drawpile = self.room:getDrawPile()
		if drawpile:isEmpty() then return false end
		local acard = sgs.Sanguosha:getCard(drawpile:first())
		if not acard:isAvailable(self.player) then return false end
		local suit =acard:getSuitString()
		local number = acard:getNumberString()
		local card_id = acard:getEffectiveId()
		local slash_str = (acard:objectName()..":chaoren[%s:%s]=%d"):format(suit, number, card_id)

		local slash = sgs.Card_Parse(slash_str)

		assert(slash)
		return slash

end

function sgs.ai_cardsview_valuable.chaoren(self, class_name, player)
	local acard = sgs.Sanguosha:getCard(self.room:getDrawPile():first())
	if not acard then return nil end
	local suit =acard:getSuitString()
	local number = acard:getNumberString()
	local card_id = acard:getEffectiveId()

	if class_name == "Slash" and acard:isKindOf("Slash") then
		return (acard:objectName()..":chaoren[%s:%s]=%d"):format(suit, number, card_id)
	end
	if acard:isKindOf(class_name) then
		return (acard:objectName()..":chaoren[%s:%s]=%d"):format(suit, number, card_id)
	end
end



sgs.ai_needToWake.yizhi=function(self,player)
	return "Dying","Unknown"
end

local ziwo_skill = {}
ziwo_skill.name = "ziwo"
table.insert(sgs.ai_skills, ziwo_skill)
ziwo_skill.getTurnUseCard = function(self)
	if not self.player:isWounded() then return nil end
	cards =sgs.QList2Table(self.player:getCards("hs"))
	if #cards<2 then return nil end
	self:sortByKeepValue(cards)
	use_cards={}
	table.insert(use_cards,cards[1]:getId())
	table.insert(use_cards,cards[2]:getId())
	return sgs.Card_Parse("@ZiwoCard=" .. table.concat(use_cards, "+") )

end
sgs.ai_skill_use_func.ZiwoCard=function(card,use,self)
	use.card = card
end

sgs.ai_skill_invoke.benwo = function(self,data)
	local target=self.player:getTag("benwo_target"):toPlayer()
	if not target then return true end
	if self:isFriend(target) then
		return false
	else
		return true
	end
end

sgs.ai_skill_discard.benwo = function(self, discard_num, min_num, optional, include_equip)
	local to_discard = {}
	local cards = self.player:getCards("hes")
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)
	local num=math.min(#cards,discard_num)
	for var=1, num ,1 do
		table.insert(to_discard, cards[var]:getEffectiveId())
	end
	return to_discard
end



sgs.ai_skill_use["@@chaowo"] = function(self, prompt)
	if self.player:isKongcheng() then return nil end
	local target
	target_table = self:getFriends(self.player)
	if #target_table==0 then return "." end
	self:sort(target_table, "value")
	for _,p in pairs(target_table) do
		if  p:getMaxHp()==3 then
			 target=p
		end
		if self:isWeak(p) and not self:isWeak(self.player) then
			 target=p
		end
	end
	if not target then
		target= self.player
	end
	if target  then
		local cards = self.player:getHandcards()
		cards = sgs.QList2Table(cards)
		self:sortByKeepValue(cards)
		return "@ChaowoCard=" ..cards[1]:getEffectiveId().. "->" .. target:objectName()
	end
	return "."
end
sgs.ai_card_intention.ChaowoCard = -60




sgs.ai_skill_invoke.zuosui = function(self,data)
	local damage =self.player:getTag("zuosui_damage"):toDamage()
	local isSlash = true
	if not damage.card or not damage.card:isKindOf("Slash") then
		isSlash = false
	end
	local need_damage =  self:getDamagedEffects(damage.to, damage.from, isSlash)
	if self:isFriend(damage.to) then
		if need_damage and damage.damage <= 1 then
			return false
		end
		return true
	elseif self:isEnemy(damage.to) then
		if  damage.damage <= 1 then

			return need_damage
		else
			return false
		end
	end
	return false
end
sgs.ai_skill_choice.zuosui= function(self, choices, data)
	if choices:match("losehp") then
		local target = self.player:getTag("zuosui_damage"):toDamage().to
		local number = self.player:getTag("zuosui_number"):toInt()
		local discard_num = target:getCards("hes"):length() - number
		if self:isEnemy(target)then
			if discard_num >= 2*number then
				return "discard"
			end
		else
			if discard_num < 2 or number >= 2 then
				return "discard"
			end
		end

		return "losehp"
	else
		local suwako =self.player:getTag("zuosui_source"):toPlayer()
		local minus_discard_num = self.player:getCards("hes"):length() - 4
		if self:isFriend(suwako) then
			if minus_discard_num <=1 then
				return "4"
			else
				return "1"
			end
		else
			return "1"
		end
		return "1"
	end
end

sgs.ai_skill_invoke.worao = function(self,data)
	return true
end
sgs.ai_skill_discard.worao = function(self)
	local use =self.player:getTag("worao_use"):toCardUse()
	local target = use.from
	if not target then return true end

	local woraoGive = {}
	local toGive, allcards = {}, {}
	local keep
	for _, card in sgs.qlist(self.player:getHandcards()) do
		if not keep and (isCard("Jink", card, self.player) or isCard("Analeptic", card, self.player)) then
			keep = true
		else
			table.insert(toGive, card)
		end
		table.insert(allcards, card)
	end

	local cards = #toGive > 0 and toGive or allcards
	if self:isFriend(target) then
		self:sortByKeepValue(cards, true)
		local card, friend = self:getCardNeedPlayer(cards)
		if card and friend and target:objectName() == friend:objectName() then
			table.insert(woraoGive, card:getId())
			return  woraoGive
		end
	else
		self:sortByKeepValue(cards)
	end

	table.insert(woraoGive, cards[1]:getId())
	return woraoGive
end
sgs.ai_skill_choice.shenhua = function(self, choices, data)
	return "discardMark"
end


sgs.ai_skill_invoke.hongfo = function(self,data)
	return true
end
sgs.ai_skill_playerchosen.hongfo = function(self, targets)
	for _,p in sgs.qlist(targets) do
		if self:isFriend(p) then
			return p
		end
	end
	return targets:first()
end

sgs.ai_skill_cardask["@junwei-discard"] = function(self, data)
	local target = self.player:getTag("junwei_target"):toPlayer()
	if self:isFriend(target)  then return "." end
	local cards = {}
	for _, card in sgs.qlist(self.player:getCards("hes")) do
		if card:isRed() then continue end
		table.insert(cards,card)
	end
	if #cards == 0 then return "." end
	self:sortByKeepValue(cards)
	return "$" .. cards[1]:getEffectiveId()
end

function sgs.ai_slash_prohibit.junwei(self, from, to)
	if self:isFriend(to, from) or from:getKingdom() == to:getKingdom() then return false end

	if from:objectName() == self.player:objectName() then
		local num =self:getSuitNum("black", true)
		return num < 2
	end
	if self.player:getHandcardNum() == 2 then
		local needkongcheng = self:needKongcheng()
		if needkongcheng then
			return  false
		end
	end
	return true
end

sgs.ai_skill_use["@@wendao"] = function(self, prompt)
	if #self.enemies== 0    then return "." end
	self:sort(self.enemies,"handcard")

	local kingdoms = {}
	local  targets={}
	for _,p in pairs (self.enemies) do
		if self.player:canDiscard(p, "hs") then
			if not table.contains(kingdoms,p:getKingdom()) then
				table.insert(targets,p:objectName())
				table.insert(kingdoms,p:getKingdom())
			end
		end
	end
	if #targets >=1 then
		return "@WendaoCard=.->" .. table.concat(targets, "+")
	end
	return "."
end


local shenbao_skill = {}
shenbao_skill.name = "shenbao_attach"
table.insert(sgs.ai_skills, shenbao_skill)

shenbao_skill.getTurnUseCard = function(self, inclusive)
	local spear = function(self, inclusive)
		if not self.player:hasWeapon("Spear") then return nil end
		local weapon = self.player:getWeapon()
		if weapon and weapon:isKindOf("Spear") then return nil end
		return turnUse_spear(self, inclusive, "Spear")
	end
	
	local jadeSeal = function(self, inclusive)
		if not self.player:hasTreasure("JadeSeal") then return nil end
		local treasure = self.player:getWeapon()
		if treasure and treasure:isKindOf("JadeSeal") then return nil end
		if self.player:hasFlag("JadeSeal_used") then
			return nil
		end
		local c = sgs.cloneCard("known_both", sgs.Card_NoSuit, 0)
		c:setSkillName("JadeSeal")
		c:setCanRecast(false)
		return c
	end
	
	return jadeSeal(self, inclusive) or spear(self, inclusive)
end

function sgs.ai_cardsview.shenbao_attach(self, class_name, player)
	if not self.player:hasWeapon("Spear") then return nil end
	local weapon = self.player:getWeapon()
	if weapon and weapon:isKindOf("Spear") then return nil end
	if class_name == "Slash" then
		return cardsView_spear(self, player, "Spear")
	end
end

sgs.ai_view_as.shenbao_attach = function(card, player, card_place)
	if not player:hasFlag("Pagoda_used") then
		local suit = card:getSuitString()
		local number = card:getNumberString()
		local card_id = card:getEffectiveId()
		if card_place == sgs.Player_PlaceHand then
			if card:isBlack() then
				return ("nullification:Pagoda[%s:%s]=%d"):format(suit, number, card_id)
			end
		end
	end
end

function SmartAI:executorRewardOrPunish(victim,damage)
	local komachi = self.room:findPlayerBySkillName("yindu")
	local killer = self:findRealKiller(victim,damage)
	local executor
	if komachi then
		if komachi:objectName() == victim:objectName() then
			return killer
		end
		if not killer then
			return komachi
		else
			if sgs.evaluatePlayerRole(victim) == "rebel" then
					if self:isFriend(killer,komachi) then
						return killer
					else
						return komachi
					end
			elseif  sgs.evaluatePlayerRole(victim) == "neutral" then
				return komachi
			elseif  sgs.evaluatePlayerRole(victim) == "renegade" then
				return komachi
			else
				if killer:isLord() then
					if self:isFriend(komachi,killer) then
						return komachi
					else
						if killer:getCards("hes"):length() + killer:getPile("wooden_ox"):length() > 3 then
							return killer
						else
							return komachi
						end
					end
				else
					return komachi
				end
			end
		end

	else
		return killer
	end
end
sgs.ai_skill_invoke.yindu = function(self,data)
	local death = self.player:getTag("yindu_death"):toDeath()
	local executor = self:executorRewardOrPunish(death.who,death.damage)
	if executor then
		if executor:objectName() == self.player:objectName() then
			return true
		else
			return not self:isFriend(executor)
		end
	else
		return true
	end
end


sgs.ai_skill_invoke.huanming = function(self,data)
	local target = data:toPlayer()
	if self:isEnemy(target) then
		local selfrecover = math.min(target:getHp(),self.player:getMaxHp()) - self.player:getHp()
		local enemylose = target:getHp() - math.min(self.player:getHp(),target:getMaxHp())
		local dif =  selfrecover + enemylose
		if self:isWeak() then
			return dif >=2
		else
			return dif > 2
		end
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.huanming = function(self, player, args)
	local damage = player:getTag("huanming_damage"):toDamage()
	local to=damage.to
	if  to  and args[#args] == "yes" then
		if player:getHp()> to:getHp() then
			sgs.updateIntention(player, to, -50)
		elseif player:getHp() < to:getHp() then
			sgs.updateIntention(player, to, 100)
		end
	end
end


--心花
local xinhua_skill = {}
xinhua_skill.name = "xinhua"
table.insert(sgs.ai_skills, xinhua_skill)
xinhua_skill.getTurnUseCard = function(self)
	local XinhuaCards = {}
	for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		for _, id in sgs.qlist(p:getShownHandcards()) do
		    local card = sgs.Sanguosha:getCard(id)
			if not self.player:isLocked(card) then
				table.insert(XinhuaCards, card)
			end
		end
	end
    if #XinhuaCards == 0 then return end

	self:sortByUseValue(XinhuaCards, false)
	for _,c in pairs (XinhuaCards) do
		local dummyuse = { isDummy = true, to = sgs.SPlayerList()}
		if c:isKindOf("BasicCard") then
			self:useBasicCard(c, dummyuse)
		elseif c:isKindOf("TrickCard") then
			self:useTrickCard(c, dummyuse)
		elseif c:isKindOf("EquipCard") then
			self:useEquipCard(c, dummyuse)
		end
		if dummyuse.card then
		    --防止重铸
		    if dummyuse.card:isKindOf("IronChain") or dummyuse.card:isKindOf("KnownBoth") then
				if not dummyuse.to or dummyuse.to:isEmpty() then return nil end 
			end
		    return sgs.Card_Parse("@XinhuaCard="..c:getEffectiveId())
		end
	end
	return nil
end
sgs.ai_skill_use_func.XinhuaCard=function(card,use,self)
	local xinhua = sgs.Sanguosha:getCard(card:getSubcards():first())
	if xinhua:getTypeId() == sgs.Card_TypeBasic then 
		self:useBasicCard(xinhua, use)
	elseif xinhua:getTypeId() == sgs.Card_TypeTrick then
			self:useTrickCard(xinhua, use)
    elseif xinhua:getTypeId() == sgs.Card_TypeEquip then
		self:useEquipCard(xinhua, use)
	end
	if not use.card then return end
	use.card=card
end


sgs.ai_use_priority.XinhuaCard = 10
function sgs.ai_cardsview_valuable.xinhua(self, class_name, player)
	if (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_UNKNOWN) then
		return nil
	end

    local XinhuaCards = {}
	for _, p in sgs.qlist(self.room:getOtherPlayers(player)) do
		for _, id in sgs.qlist(p:getShownHandcards()) do
		    local card = sgs.Sanguosha:getCard(id)
			if card:isKindOf(class_name) then
			    if sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE then
					if not player:isCardLimited(card, sgs.Card_MethodResponse) then
						table.insert(XinhuaCards, card)
					end
				else
				    if not player:isLocked(card) then
						table.insert(XinhuaCards, card)
					end
				end
			end
		end
	end
    if #XinhuaCards == 0 then return end
	return "@XinhuaCard=" .. XinhuaCards[1]:getEffectiveId() .. ":" .. XinhuaCards[1]:objectName()
end

local badSkills={"mokai","guangji","xinghui","bendan","moxue","sisheng","jingdong","shishen","chunmian",
"wangwu","shouye","zhouye","hongwu","shenqiang","yewang","jinguo","rengui","gaoao","caiyu","shenhua"}
local key_skills={"feixiang","mingyun","yongheng","qiuwen","xiangqi","jiushu","hpymsiyu"}
sgs.ai_skill_choice.tongling = function(self, choices)
	local choice_table = choices:split("+")
	table.removeOne(choice_table, "cancel")
	for index, achoice in ipairs(choice_table) do
		if table.contains(key_skills,achoice)  then return achoice end
	end
	for index, achoice in ipairs(choice_table) do
		if not table.contains(badSkills,achoice)  then return achoice end
	end
	return "cancel"
end

function rumoNum(player)
    local loyalist, rebel, renegade = 0, 0 , 0
    for _,role in ipairs(player:getRoom():aliveRoles()) do
        if (role == "rebel") then
            rebel =rebel + 1
        elseif (role == "renegade") then
            renegade = renegade + 1
        elseif (role == "loyalist" or role == "lord") then
            loyalist = loyalist + 1
		end
    end
    renegade = math.min(renegade, 1)
    local num = math.max(renegade, loyalist);
    num = math.max(num, rebel)
	return num
end

local rumo_skill = {}
rumo_skill.name = "rumo"
table.insert(sgs.ai_skills, rumo_skill)
function rumo_skill.getTurnUseCard(self)
	if self.player:getMark("@rumo")==0 then return nil end
	local num = rumoNum(self.player)
	if num > 1 then
		return sgs.Card_Parse("@RumoCard=.")
	end
	return nil
end
sgs.ai_skill_use_func.RumoCard=function(card,use,self)
	local num =  rumoNum(self.player)
	local targets={}
	table.insert(targets, self.player)
	for _,p in ipairs (self.enemies) do
		if not p:isChained() and #targets < num then
			table.insert(targets, p)
		end
	end

	if #targets > 1 then
		use.card = card
		if use.to then
			for _,t in ipairs(targets) do
				use.to:append(t)
			end
			if use.to:length() >= 1 then return end
		end
	end
end
