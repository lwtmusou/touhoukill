
--SmartAI:needRetrial(judge)
function SmartAI:lingqiParse(self,target,use)
	use = use or self.player:getTag("lingqicarduse"):toCardUse()
	--if not use or not use.card  or not use.from then return 3 end
	if not use or not use.card   then return 3 end
	local card=use.card
	local from=use.from

	if self.player:objectName() == target:objectName() then
		--simulate standard-ai with a fakeEffect
		local pattern = nil
		if card:isKindOf("Slash") then
			local _data=sgs.QVariant()
			local fakeEffect =sgs.SlashEffectStruct()
			fakeEffect.slash = card
			fakeEffect.from = from
			fakeEffect.to = target
			_data:setValue(fakeEffect)
			if sgs.ai_skill_cardask["slash-jink"] (self, _data, pattern, from) == "." then
				return 1
			end
		elseif card:isKindOf("AOE") then
			local _data=sgs.QVariant()

			local fakeEffect =sgs.CardEffectStruct()
			fakeEffect.card = card
			fakeEffect.from = from
			fakeEffect.to = target
			_data:setValue(fakeEffect)
			if card:isKindOf("SavageAssault") then
				pattern = "slash"
				if sgs.ai_skill_cardask.aoe(self, _data, pattern, from, "savage_assault") == "." and not self:hasWeiya() then
					--should fix weiya
					return 1
				end
			end
			if card:isKindOf("ArcheryAttack") then
				pattern = "jink"
				if sgs.ai_skill_cardask.aoe(self, _data, pattern, from, "archery_attack") == "." and not self:hasWeiya()  then
					--should fix weiya
					return 1
				end
			end
		end
	end
	if sgs.dynamic_value.damage_card[card:getClassName()] then
		return 2
	end
	if card:isKindOf("ExNihilo") then
		return 1
	end
	if card:isKindOf("GodSalvation") and target:isWounded() then
		return 1
	end
	if card:isKindOf("IronChain")  then
		if target:isChained() then
			return 1
		else
			return 2
		end
	end

	if card:isKindOf("Snatch") or card:isKindOf("Dismantlement") then
		local cards=target:getCards("j")
		local throw_delay=false
		for _,card in sgs.qlist(cards) do
			if card:isKindOf("Indulgence") or card:isKindOf("SupplyShortage") then
				throw_delay=true
			end
		end
		if  throw_delay and not self:isEnemy(from,target) then
			return 1
		else
			return 2
		end
	end
	return 3
end


sgs.ai_skill_invoke.lingqi =function(self,data)
	if not self:invokeTouhouJudge() then return false end

	local parse=self:lingqiParse(self,self.player)
	if parse==2 then
		return true
	end
	return false
end
sgs.ai_skillProperty.lingqi = function(self)
	return "cause_judge"
end

sgs.ai_skill_invoke.qixiang =function(self,data)
	local target=self.player:getTag("qixiang_judge"):toJudge().who
	if target and self:isFriend(target) then
		return true
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.qixiang = function(self, player, args)
	local target=player:getTag("qixiang_judge"):toJudge().who
	if target then
		if args[#args] == "yes" then
			sgs.updateIntention(player, target, -50)
		elseif args[#args] == "no" and sgs.ai_role[target:objectName()] ~= "netural" then
			if target:isLord() then
				sgs.updateIntention(player, target, 10)
			elseif sgs.ai_role[target:objectName()] == "loyalist" and not self:cautionRenegade(player,target) then
				sgs.updateIntention(player, target, 10)
			elseif sgs.ai_role[target:objectName()] == "rebel" then
				sgs.updateIntention(player, target, 10)
			end
		end
	end
end

table.insert(sgs.ai_global_flags, "bolisource")
sgs.ai_skill_invoke.boli = function(self,data)
	local judge=self.player:getTag("boli_judge"):toJudge()
	--防止没有队友时ai还无聊地发动技能
	local onlyEnemy = true
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if not self:isEnemy(p) then
			onlyEnemy = false
			break
		end
	end
	return not onlyEnemy and self:needRetrial(judge)
end
sgs.ai_choicemade_filter.skillInvoke.boli = function(self, player, args)
	if args[#args] == "yes" then
			sgs.bolisource = player
	end
end
sgs.ai_skill_cardask["@boli-retrial"] = function(self, data)
		local lord = getLord(self.player)
		if not sgs.bolisource  then sgs.bolisource = lord end
		if not sgs.bolisource then return "." end
		if not self:isFriend(lord) then return "." end
		local cards = sgs.QList2Table(self.player:getCards("hs"))
		local cards1={}
		for _,card in pairs(cards) do
			if card:getSuit()==sgs.Card_Heart then
				table.insert(cards1,card)
			end
		end
		local judge = data:toJudge()

		if #cards1==0 then return "." end
		return "$" .. self:getRetrialCardId(cards1, judge) or judge.card:getEffectiveId()  --tostring()
end

sgs.ai_choicemade_filter.cardResponded["@boli-retrial"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		sgs.updateIntention(player, sgs.bolisource, -80)
		sgs.bolisource = nil
	elseif sgs.bolisource then
		local lieges = player:getRoom():getOtherPlayers(sgs.bolisource)
		if lieges and not lieges:isEmpty() then
			if player:objectName() == lieges:last():objectName() then
				sgs.bolisource = nil
			end
		end
	end
end

sgs.ai_slash_prohibit.lingqi = function(self, from, to, card)
	if self:isFriend(from,to) then
		return false
	end
	local wizard_type ,wizard = self:getFinalRetrial()
	return  (wizard  and self:invokeTouhouJudge(to))
end


sgs.ai_skillProperty.boli = function(self)
	return "noKingdom"
end

--function SmartAI:hasHeavySlashDamage(from, slash, to, getValue)
local mofa_skill = {}
mofa_skill.name = "mofa"
table.insert(sgs.ai_skills, mofa_skill)
function mofa_skill.getTurnUseCard(self)

	if self.player:hasFlag("mofa_invoked") then return nil end
	if self.player:isKongcheng() then return nil end
	if #self.enemies==0 then return false end


	local cards = self.player:getCards("hs")
	local key_names={"Duel","SavageAssault","ArcheryAttack"}
	local key_ids={}
	local slashes={}
	local slashe_ids={}
	for _,name in pairs (key_names) do
		for _,c in sgs.qlist(cards) do
			if c:isKindOf(name) and  c:isAvailable(self.player) then
				table.insert(key_ids,c:getId())
			elseif c:isKindOf("Slash") and  c:isAvailable(self.player) then
				table.insert(slashe_ids,c:getId())
				table.insert(slashes,c)
			end
		end
	end


	local temp_slash
	for _,p in pairs (self.enemies) do
		for _,s in pairs (slashes) do
			if self.player:canSlash(p,s,true) then
				if temp_slash and temp_slash:getSuit() ~=sgs.Card_Spade then
					break
				else
					temp_slash=s
				end
			end
		end
		if temp_slash and temp_slash:getSuit() ~=sgs.Card_Spade then
			break
		end
	end

	if #key_ids==0 and not temp_slash  then
		return nil
	end




	local scards={}
	local scards_without_key={}
	local others_without_key={}
	local keycard_func = function(card)
		return   table.contains(key_ids,card:getId())
			or (temp_slash and card:getId() == temp_slash:getId())
	end
	for _,card in sgs.qlist(cards) do
		if  card:getSuit()==sgs.Card_Spade and not keycard_func(card) then
				table.insert(scards_without_key,card)
		end
		if card:getSuit()==sgs.Card_Spade then
				table.insert(scards,card)
		end
		if  card:getSuit() ~=sgs.Card_Spade and not keycard_func(card)  then
				table.insert(others_without_key,card)
		end
	end



	if #scards_without_key>0 then
		self:sortByCardNeed(scards_without_key)
		return sgs.Card_Parse("@MofaCard="..scards_without_key[1]:getEffectiveId())
	elseif #others_without_key>0 then
		self:sortByCardNeed(others_without_key)
		return sgs.Card_Parse("@MofaCard="..others_without_key[1]:getEffectiveId())
	elseif #scards>0 and (#key_ids>1 or (#key_ids>0 and temp_slash)) then
			self:sortByUseValue(scards)
			return sgs.Card_Parse("@MofaCard="..scards[1]:getEffectiveId())
	end
	return false
end
sgs.ai_skill_use_func.MofaCard = function(card, use, self)
	use.card=card
end

sgs.ai_use_value.MofaCard = 4
sgs.ai_use_priority.MofaCard = 4
sgs.ai_cardneed.mofa = function(to, card, self)
	return card:getSuit()==sgs.Card_Spade
end



local wuyuvs_skill = {}
wuyuvs_skill.name = "wuyu_attach"
table.insert(sgs.ai_skills, wuyuvs_skill)
wuyuvs_skill.getTurnUseCard = function(self)

	if self.player:hasFlag("Forbidwuyu") then return nil end


	local cards = self.player:getCards("hs")
	cards = sgs.QList2Table(cards)
	local card
	self:sortByUseValue(cards,true)
	for _,acard in ipairs(cards)  do
		if acard:getSuit()==sgs.Card_Spade then
			card = acard
			break
		end
	end
	if not card then return nil end

	local card_id = card:getEffectiveId()
	local card_str ="@WuyuCard="..card_id
	local skillcard = sgs.Card_Parse(card_str)

	assert(skillcard)

	return skillcard
end

sgs.ai_skill_use_func.WuyuCard = function(card, use, self)

	local targets = {}
	for _,friend in ipairs(self.friends_noself) do
		if friend:hasLordSkill("wuyu") then
			if not friend:hasFlag("wuyuInvoked") then
				table.insert(targets, friend)
			end
		end
	end

	if #targets > 0 then
		use.card = card
		if use.to then
			use.to:append(targets[1])
			if use.to:length()>=1 then return end
		end
	end

end

sgs.ai_card_intention.WuyuCard = -40
sgs.ai_skillProperty.wuyu = function(self)
	return "noKingdom"
end

function SmartAI:hasSkillsForSaiqian(player)
	player = player or self.player
	if player:hasSkills("xisan|yongheng|kongpiao") then
		return true
	end
	if player:hasSkills("zaozu+qiuwen") then
		return true
	end
	return false
end
local saiqianvs_skill = {}
saiqianvs_skill.name = "saiqian_attach"
table.insert(sgs.ai_skills, saiqianvs_skill)
function saiqianvs_skill.getTurnUseCard(self)
		if self.player:isKongcheng() then return nil end
		if self.player:hasFlag("ForbidSaiqian") then return nil end
		local handcards = sgs.QList2Table(self.player:getHandcards())
		local saiqian_cards={}
		self:sortByUseValue(handcards)
		if self.player:isWounded()then
			for _,c in pairs(handcards)do
				if c:getSuit()==sgs.Card_Heart then
					table.insert(saiqian_cards,c:getId())
					break
				end
			end
		end
	   if self:getOverflow() >0 or self:hasSkillsForSaiqian(self.player)  then
			local overflow=math.min(3,self:getOverflow())
			if self:hasSkillsForSaiqian() then
				overflow= math.min(3,self.player:getHandcardNum())
			end
			for var=1, overflow, 1 do
				if #saiqian_cards<overflow
					and not table.contains(saiqian_cards,handcards[var]:getId()) then
					table.insert(saiqian_cards,handcards[var]:getId())
				end
			end
	   end
		if #saiqian_cards>0 then
			local card_str= "@SaiqianCard=" .. table.concat(saiqian_cards, "+")
			return sgs.Card_Parse(card_str)
		end
		return nil
end
sgs.ai_skill_use_func.SaiqianCard = function(card, use, self)
	local targets = {}
	for _,friend in ipairs(self.friends_noself) do
		if friend:hasSkill("saiqian") then
			if not friend:hasFlag("saiqianInvoked") then
				table.insert(targets, friend)
			end
		end
	end

	if #targets > 0 then
		use.card = card
		if use.to then
			use.to:append(targets[1])
			return
		end
	end
end
sgs.ai_card_intention.SaiqianCard = -60

sgs.ai_skill_choice.saiqian= function(self, choices, data)
	local source= self.room:getCurrent()
	if not source or not source:isWounded() or not self:isFriend(source) then return "cancel_saiqian" end
	if  source:getHp()+1 > getBestHp(source) then return "cancel_saiqian" end
	if choices:match("discard_saiqian") then
		for _,c in sgs.qlist(self.player:getHandcards())do
			if c:getSuit()==sgs.Card_Heart  then
				return "discard_saiqian"
			end
		end
	end
	if self.player:getHp()>3 and choices:match("losehp_saiqian") then
		return "losehp_saiqian"
	end
	return "cancel_saiqian"
end
sgs.ai_choicemade_filter.skillChoice.saiqian = function(self, player, args)
	local choice = args[#args]
	local target = self.room:getCurrent()

	if not target or not target:isWounded() then return end
	if  choice == "losehp_saiqian" then
		sgs.updateIntention(player, target, -80)
	end
end

sgs.ai_skill_cardask["@saiqian-discard"] = function(self,data)
		local cards = sgs.QList2Table(self.player:getCards("hs"))
		if #cards==0 then return "." end
		self:sortByCardNeed(cards)
		for _,card in pairs (cards) do
			if card:getSuit()==sgs.Card_Heart then
				return "$" .. card:getId()
			end
		end
		return "."
end
sgs.ai_choicemade_filter.cardResponded["@saiqian-discard"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		local target = self.room:getCurrent()
		if not target or not target:isWounded() then return end
		sgs.updateIntention(player, target, -80)
	end
end

sgs.saiqian_suit_value = {
	heart = 4.9
}

--find cost for jiezou
function jiezouSpade(self,player)
	local cards
	if player:objectName() == self.player:objectName() then
		cards =self.player:getCards("hes")
	else
		cards=player:getCards("e")
	end
	for _,card in sgs.qlist(cards) do
		if card:getSuit()==sgs.Card_Spade then
			return true
		end
	end
	return false
end
local jiezou_skill = {}
jiezou_skill.name = "jiezou"
table.insert(sgs.ai_skills, jiezou_skill)
jiezou_skill.getTurnUseCard = function(self)
	local targets=self:getEnemies(self.player)
	local can_use= jiezouSpade(self,self.player)
	if not can_use then
		for _,target in pairs(targets) do
			can_use = jiezouSpade(self,target)
			if can_use then
				break
			end
		end
	end
	if can_use then
		return sgs.Card_Parse("@JiezouCard=.")
	end
	return nil
end
sgs.ai_skill_use_func.JiezouCard = function(card, use, self)
		local friends={}
		local enemies_spade={}
		local enemies ={}

		for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if not p:isAllNude() then
				if self:isFriend(p) then
					local jcards=p:getCards("j")
					for _,card in sgs.qlist(jcards) do
						if card:getSuit()==sgs.Card_Spade and not card:isKindOf("Lightning") then
							table.insert(friends,p)
						end
					end
				end
				if self:isEnemy(p) then
					if jiezouSpade(self,p) then
						table.insert(enemies_spade,p)
					elseif not p:isNude() then
						table.insert(enemies,p)
					end
				end
			end
		end
		if #friends>0 then
			use.card = card
			if use.to then
				self:sort(friends,"value")
				use.to:append(friends[1])
				if use.to:length() >= 1 then return end
			end
		elseif #enemies_spade>0 then
			use.card = card
			if use.to then
				self:sort(enemies_spade,"value")
				use.to:append(enemies_spade[1])
				if use.to:length() >= 1 then return end
			end
		elseif #enemies>0 and jiezouSpade(self,self.player) then
			use.card = card
			if use.to then
				self:sort(enemies,"value")
				use.to:append(enemies[1])
				if use.to:length() >= 1 then return end
			end
		end
end
sgs.ai_skill_cardchosen.jiezou = function(self, who, flags)
	if self:isFriend(who) then
		local jcards=who:getCards("j")
		for _,card in sgs.qlist(jcards) do
			if card:getSuit()==sgs.Card_Spade and not card:isKindOf("Lightning") then
				return card
			end
		end
	else
		local ecards=who:getCards("e")
		for _,card in sgs.qlist(ecards) do
			if card:getSuit()==sgs.Card_Spade then
				return card
			end
		end
		local id = self:askForCardChosen(who, "hes", "snatch", sgs.Card_MethodNone)
		return sgs.Sanguosha:getCard(id)
	end
end
sgs.ai_skill_cardask["@jiezou_spadecard"] = function(self, data)
		local cards = sgs.QList2Table(self.player:getCards("hes"))
		if #cards==0 then return "." end
		self:sortByKeepValue(cards)
		for _,card in pairs (cards) do
			if card:getSuit()==sgs.Card_Spade then
				return "$" .. card:getId()
			end
		end
		return "."
end


sgs.ai_choicemade_filter.cardChosen.jiezou = sgs.ai_choicemade_filter.cardChosen.snatch
sgs.ai_use_value.JiezouCard = 8
sgs.ai_use_priority.JiezouCard =6
sgs.dynamic_value.control_card.JiezouCard = true
sgs.ai_cardneed.jiezou = function(to, card, self)
	return card:getSuit()==sgs.Card_Spade
end
sgs.jiezou_suit_value = {
	spade = 4.9
}


function keycard_shoucang(card)
	if card:isKindOf("Peach") or card:isKindOf("SavageAssault")
	or card:isKindOf("ArcheryAttack") or card:isKindOf("ExNihilo") then
		return true
	end
	return false
end
sgs.ai_skill_use["@@shoucang"] = function(self, prompt)
	if self:getOverflow() <= 0 then
		return "."
	end
	local needNum=math.min(4,self:getOverflow())
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	local suits = {}
	local show={}
	for _,c in pairs(cards)do
		local suit = c:getSuitString()
		if not suits[suit] then
			suits[suit] = 1
			table.insert(show, tostring(c:getId()))
		end
		if #show>=needNum then
			break
		end
	end
	if #show>0 then
		return "@ShoucangCard=" ..table.concat(show, "+").."->."
	end
	return "."
end




sgs.ai_need_bear.baoyi = function(self, card,from,tos)
	from = from or self.player
	local Overflow = self:getOverflow(from) >1
	if not card:isKindOf("EquipCard") then return false end
	if self:getSameEquip(card,from) and not Overflow then
		return true
	end
	return false
end
function SmartAI:needBaoyiEquip(card,player)
	player = player or self.player
	if not player:hasSkill("baoyi") then return false end
	t =false
	Overflow = self:getOverflow() <2
	if not card:isKindOf("EquipCard") then return false end
	if self:getSameEquip(card) and Overflow then
		return true
	end
	return false
end


sgs.ai_skill_use["@@baoyi"] = function(self, prompt)
	local target_table = self:getEnemies(self.player)
	if #target_table==0 then return "." end

	local delay_num= self.player:getCards("j"):length()
	local delay_ids=self.player:getJudgingAreaID()
	delay_ids = sgs.QList2Table(delay_ids)
	local has_spade=false
	for _,c in sgs.qlist(self.player:getCards("j")) do
		if c:getSuit()== sgs.Card_Spade then
			has_spade=true
		end
	end
	if has_spade then
		return "@BaoyiCard=" .. table.concat(delay_ids, "+")
	end
	local target = sgs.ai_skill_playerchosen.zero_card_as_slash(self, self.room:getOtherPlayers(self.player))
	if not  target then
		return "@BaoyiCard="..table.concat(delay_ids, "+")
	end
	local equips={}
	for _,c in sgs.qlist(self.player:getCards("hes")) do
		if c:isKindOf("EquipCard") then
			if c:getSuit()== sgs.Card_Spade then
				if delay_num>0 then
					return "@BaoyiCard="..c:getEffectiveId().."+"..table.concat(delay_ids, "+")
				else
					return "@BaoyiCard="..c:getEffectiveId()
				end
			end
			table.insert(equips,c)
		end
	end
	if delay_num==0 then
		if #equips>0  then
			self:sortByKeepValue(equips)
			return "@BaoyiCard="..equips[1]:getEffectiveId()
		end
	else
		return "@BaoyiCard="..table.concat(delay_ids, "+")
	end
	return "."
end

sgs.ai_skill_playerchosen.baoyi = function(self, targets)
	local target = sgs.ai_skill_playerchosen.zero_card_as_slash(self, targets)
	if target then
		return target
	end
	return nil
	--[[local target_table = sgs.QList2Table(targets)
	self:sort(target_table, "hp")
	local baoyi_target
	local slash= sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
	for _,target in pairs(target_table) do
		if  self:isEnemy(target) and self:slashIsEffective(slash, target, self.player)  then
			if not baoyi_target  then
				baoyi_target=target
			else
				if self:isWeak(target) and not self:isWeak(baoyi_target) then
					baoyi_target=target
					break
				end
			end
		end
	end

	if baoyi_target then
		return baoyi_target
	end
	return nil]]
end

sgs.ai_cardneed.baoyi = function(to, card, self)
	return  card:isKindOf("EquipCard")
end
sgs.baoyi_suit_value = {
	spade = 4.9
}
sgs.ai_trick_prohibit.baoyi = function(self, from, to, card)
	if not card:isKindOf("DelayedTrick")  then return false end
	if self:isFriend(from,to) then return false end
	if self.room:alivePlayerCount()==2 then
		return not from:hasArmorEffect("Vine")
	else
		return true
	end
	return true
end



function SmartAI:zhizeValue(player)
	if self:touhouHandCardsFix(player) or player:hasSkill("heibai") then
		return 0
	end
	local value=0
	for _, card in sgs.qlist(player:getHandcards()) do
		local flag = string.format("%s_%s_%s", "visible", global_room:getCurrent():objectName(), player:objectName())
		if  card:hasFlag("visible") or card:hasFlag(flag) then
			if card:isKindOf("Peach") then
				value=value+20
			end
			if card:getSuit()==sgs.Card_Heart then
				value=value+10
			end
		else
			value=value+5
		end
	end
	return value
end
sgs.ai_skill_playerchosen.zhize = function(self, targets)

	local pre_zhize=self.player:getTag("pre_zhize"):toPlayer()
	local target_table = sgs.QList2Table(targets)
	local zhize_target
	local targets={}
	local badtargets={}
	local ftargets={}
	for _,target in pairs(target_table) do
		if  self:isEnemy(target)  then
			if target:hasSkills("yongheng|kongpiao") then
				table.insert(badtargets,target)
			else
				table.insert(targets,target)
			end
		else
			if target:hasSkills("yongheng|kongpiao") then
				table.insert(ftargets,target)
			end
		end
	end
	if #targets==0 then
		for _,target in pairs(target_table) do
			if  not self:isFriend(target)  then
				table.insert(targets,target)
			end
		end
	end

	local compare_func = function(a, b)
		return self:zhizeValue(a) > self:zhizeValue(b)
	end

	local tt
	if #targets>0  then
		table.sort(targets, compare_func)
		tt= targets[1]
	elseif #badtargets>0 then
		tt= badtargets[1]
	elseif #ftargets>0 then
		tt=ftargets[1]
	else
		tt=target_table[1]
	end
	local _data = sgs.QVariant()
	 _data:setValue(tt)
	self.player:setTag("pre_zhize",_data)
	return tt
end

sgs.ai_playerchosen_intention.zhize =function(self, from, to)
	local intention = 30
	if self:isFriend(from,to)   then
		local t=true
		for _,p in pairs (self:getEnemies(from)) do
			if not p:isKongcheng() then
				t=false
				break
			end
		end
		if t then intention = 0 end
	end
	sgs.updateIntention(from, to, intention)
end
sgs.ai_skill_cardchosen.zhize = function(self, who, flags)
	local hearts={}
	local others={}
	for _,c in sgs.qlist(who:getCards("hs")) do
		if self.player:hasSkill("chunxi") and c:getSuit() ==sgs.Card_Heatrt then
			table.insert(hearts,c)
		else
			table.insert(others,c)
		end
	end
	local inverse = not self:isFriend(who)
	if #hearts>0 then
		self:sortByKeepValue(hearts, inverse)
		return hearts[1]
	else
		self:sortByKeepValue(others, inverse)
		return others[1]
	end
end


sgs.ai_skill_use["@@chunxi"] = function(self, prompt)
	local move = self.player:getTag("chunxi_move"):toMoveOneTime()
	local ids = {}
	for _, id in sgs.qlist(move.card_ids) do
		if sgs.Sanguosha:getCard(id):getSuit() == sgs.Card_Heart
		  and self.room:getCardPlace(id) == sgs.Player_PlaceHand then
			table.insert(ids, id)
		end
	end
	if #ids > 0 then
		return "@ChunxiCard=" .. table.concat(ids, "+")
	end
	return "."
end
sgs.ai_skill_playerchosen.chunxi = function(self, targets)
	local preheart=false
	local pre_zhize
	if self.player:getPhase()==sgs.Player_Draw then
		if self.player:getTag("pre_heart") and self.player:getTag("pre_heart"):toBool() then
			preheart=true
		end
		if preheart then
			pre_zhize=self.player:getTag("pre_zhize"):toPlayer()
		end
	end
	if pre_zhize then
		self.player:setTag("pre_heart",sgs.QVariant(false))
		return pre_zhize
	end
	local target_table = self:getEnemies(self.player)
	if #target_table == 0 then return false end
	local chunxi_target
	self:sort(target_table, "value")
	for _,target in pairs(target_table) do
		if not chunxi_target and (not target:isKongcheng()) then
			chunxi_target=target
		else
			if (not target:isKongcheng()) and (self:isWeak(target) or target:getHandcardNum()<2) then
				chunxi_target=target
				break
			end
		end
	end
	if chunxi_target  then
		return chunxi_target
	end
end
sgs.ai_playerchosen_intention.chunxi = 30
sgs.ai_cardneed.chunxi = function(to, card, self)
 return card:getSuit()==sgs.Card_Heart
end
sgs.chunxi_suit_value = {
	heart = 4.0
}



function bllmwuyu_discard(player)
	local cards = sgs.QList2Table(player:getCards("hes"))
	local all_hearts={}
	for _,c in pairs(cards) do
		if c:getSuit()==sgs.Card_Heart
		and not (c:isKindOf("Peach") or c:isKindOf("Slash") or c:isKindOf("DefensiveHorse"))
		and not c:hasFlag("AIGlobal_SearchForAnaleptic") then
			table.insert(all_hearts,c:getId())
		end
	end

	return all_hearts
end
function needSkipJudgePhase(self,player)
	if (player:containsTrick("supply_shortage") or player:containsTrick("indulgence")) then
		return true
	elseif player:containsTrick("lightning") then
		return not self:invokeTouhouJudge()
	end
	return false
end



sgs.ai_skill_cardask["@bllm-discard"] = function(self, data)
	local prompt=self.player:getTag("wuyu_prompt"):toString()
	local all_hearts=bllmwuyu_discard(self.player)
	if #all_hearts==0 then return "." end
	self:sortByKeepValue(all_hearts)

	if prompt=="bllmcaiyu" then
		return "$" .. all_hearts[1]
	elseif prompt=="bllmmingyu" then
		if not needSkipJudgePhase(self,self.player) then return "." end
		return "$" .. all_hearts[1]
	elseif prompt=="bllmshuiyu" then
		if self:getOverflow() >0 and self.player:getMaxCards()<4 then
			local num=self.player:getHandcardNum()-self.player:getMaxCards()
			if num<2 then return "." end
			return "$" .. all_hearts[1]
		end
	elseif prompt=="bllmseyu" then
		return "$" .. all_hearts[1]
	elseif prompt=="bllmshiyu" then  -- 杀的ai 于 searchForAnaleptic 后 会调用 这时必须返回真值，否则ai会有卡死
		--local mustuse=false
		--if self.player:hasFlag("Global_Dying") then
		--  mustuse=true
		--elseif sgs.Slash_IsAvailable(self.player) then --and getCardsNum("Slash", self.player, self.player) > 0
		--  mustuse=true
		--end
		--if not mustuse then return "." end
		return "$" .. all_hearts[1]
	end
	return "."
end
sgs.ai_skill_cardask["@bllmshiyu-basics"] = function(self, data)
	local cards = self.player:getHandcards()
	cards=self:touhouAppendExpandPileToList(self.player,cards)
	local anals ={}
	for _,c in sgs.qlist(cards) do
		if not c:hasFlag("AIGlobal_SearchForAnaleptic") then
			table.insert(anals, c)
		end
	end
	if #anals==0 then return "." end
	self:sortByKeepValue(anals)
	return "$" .. anals[1]:getId()
end
sgs.ai_skill_invoke.bllmwuyu = function(self,data)
	local prompt=self.player:getTag("wuyu_prompt"):toString()

	if prompt=="bllmcaiyu" or prompt=="bllmwuyu" then
		return true
	elseif prompt=="bllmmingyu" then
		return  needSkipJudgePhase(self,self.player)
	elseif prompt=="bllmshuiyu" then
		if self:getOverflow() >0 then
			if  self.player:getMark("@yu")>1 or self:isWeak(self.player) then
				return true
			end
		end
		return false
	elseif prompt=="bllmshiyu" then -- 杀的ai 于 searchForAnaleptic 后 会调用 这时必须返回真值，否则ai会有卡死
		local who=self.room:getCurrentDyingPlayer()
		if who and who:objectName()==self.player:objectName() then
			return true
		else
			return true
			--return sgs.Slash_IsAvailable(self.player) --and getCardsNum("Slash", self.player, self.player) > 0
		end
	elseif prompt=="bllmseyu" then
		--return  getCardsNum("Slash", self.player, self.player) > 0  and not sgs.Slash_IsAvailable(self.player)
		return true
	end
	return false
end

function sgs.ai_cardsview_valuable.bllmwuyu(self, class_name, player)
	if self:touhouClassMatch(class_name, "Analeptic")  then
		if player:isKongcheng() then return nil end
		if player:getMark("@yu")==0 then
			local hearts_e={}
			local hearts_h={}
			local others = {}
			for _,c in sgs.qlist(player:getCards("e")) do
				if c:getSuit()==sgs.Card_Heart and not c:hasFlag("AIGlobal_SearchForAnaleptic") then
					table.insert(hearts_e,c)
				end
			end
			for _,c in sgs.qlist(player:getCards("hs")) do
				if c:getSuit()==sgs.Card_Heart and not c:hasFlag("AIGlobal_SearchForAnaleptic") then
					table.insert(hearts_h,c)
				elseif not c:hasFlag("AIGlobal_SearchForAnaleptic") then
					table.insert(others,c)
				end
			end
			for _,id in sgs.qlist(player:getHandPile()) do
				local c = sgs.Sanguosha:getCard(id)
				if not c:hasFlag("AIGlobal_SearchForAnaleptic") then
					table.insert(others,c)
				end
			end

			if #hearts_e+#hearts_h==0 then
				return nil
			elseif #hearts_e+#hearts_h==1 and #others==0 then
				return nil
			end
		end
		return "@BllmShiyuCard=."
	end
end

local bllmwuyu_skill = {}
bllmwuyu_skill.name = "bllmwuyu"
table.insert(sgs.ai_skills, bllmwuyu_skill)
function bllmwuyu_skill.getTurnUseCard(self)
--此处只发动色欲
--主动食欲吃酒的ai 由slash主动触发 此处不发动
	local hearts=bllmwuyu_discard(self.player)
	if #hearts==0 and self.player:getMark("@yu")==0 then return nil end
	if sgs.Slash_IsAvailable(self.player) then return nil end
	local atLeast = 0
	if self.player:getMark("@yu")==0 then atLeast = atLeast + 1 end
	--其实还应该详细排查hearts内含有杀和不含杀的情况。
	local slash_num=getCardsNum("Slash", self.player, self.player)

	if  slash_num <= atLeast then return nil end
	return sgs.Card_Parse("@BllmWuyuCard=.")
end
sgs.ai_skill_use_func.BllmWuyuCard = function(card, use, self)
	use.card=card
end

sgs.ai_skill_choice.bllmwuyu= function(self, choices, data)
	return "bllmseyu"
end

sgs.ai_cardneed.bllmwuyu = function(to, card, self)
 return card:getSuit()==sgs.Card_Heart
end
sgs.bllmwuyu_suit_value = {
	heart = 4.9
}
sgs.ai_use_priority.BllmWuyuCard =sgs.ai_use_priority.Slash +0.2


sgs.ai_skill_invoke.qiangyu = true
sgs.ai_skill_cardask["qiangyu-discard"] = function(self, data)
	local blacks = {}
	for _,c in sgs.qlist(self.player:getHandcards()) do
		if c:getSuit() == sgs.Card_Spade then
			table.insert(blacks, c)
		end
	end
	if #blacks > 0 then
		self:sortByUseValue(blacks)
		return "$" .. blacks[1]:getId()
	end

	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards)
	local ids = {}
	for _, c in ipairs (cards) do
		table.insert(ids, c:getId())
		if #ids >= 2 then break end
	end
	return "$" .. table.concat(ids, "+")
end


sgs.ai_cardneed.qiangyu = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  card:getSuit()==sgs.Card_Spade
	end
end
sgs.qiangyu_suit_value = {
	spade = 4.9
}

sgs.ai_skill_cardask["@mokai"] = function(self, data)
	local cards = {}
	for _,c in sgs.qlist(self.player:getCards("hes")) do
		if c:isKindOf("EquipCard") then
			table.insert(cards,c)
		end
	end
	if #cards == 0 then
		return "."
	end
	if self.player:getPhase() == sgs.Player_NotActive then
		self:sortByCardNeed(cards)
	else
		self:sortByUseValue(cards)
	end
	return "$" .. cards[1]:getId()
end

sgs.ai_cardneed.mokai = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return card:isKindOf("TrickCard") or card:isKindOf("EquipCard")
	end
end

sgs.ai_skill_cardask["@mokai-dis"] = function(self)
	local pile = self.player:getPile("tianyi")
	return "$" .. pile:at(math.random(0, pile:length() - 1))
end


sgs.ai_skill_cardask["@guangji-invoke"] =function(self,data)
   local use = data:toCardUse()
   if self:touhouCardUseEffectNullify(use,self.player) then return "." end --此杀已经无效
   --check whether player need providing jink

	local pattern = nil
	local _data=sgs.QVariant()

	local fakeEffect =sgs.SlashEffectStruct()
	fakeEffect.slash = use.card
	fakeEffect.from = use.from
	fakeEffect.to = self.player
	_data:setValue(fakeEffect)
	if sgs.ai_skill_cardask["slash-jink"] (self, _data, pattern, use.from) ~= "." then
		local pile = self.player:getPile("tianyi")
		return "$" .. pile:at(math.random(0, pile:length() - 1))
	end
	return "."
end
sgs.ai_skill_invoke.xinghui = true







local dfgzmsiyu_skill = {}
dfgzmsiyu_skill.name = "dfgzmsiyu"
table.insert(sgs.ai_skills, dfgzmsiyu_skill)
function dfgzmsiyu_skill.getTurnUseCard(self)
	if self.player:hasUsed("DfgzmSiyuCard") then return nil end
	local cards=self.player:getCards("hs")

	local can_qishu=true
	if #self.enemies<2 then
		can_qishu=false
	end
	if not self.player:hasSkill("qishu") then
		can_qishu=false
	end
	local qishu_card
	if can_qishu then
		for _,c in sgs.qlist(cards) do
			if c:isKindOf("Jink") then
				qishu_card=c
				break
			end
		end
	end
	if can_qishu and cards:length()==2 and not qishu_card then
		if cards:first():isKindOf("Dismantlement") or  cards:first():isKindOf("Snatch") then
			qishu_card=cards:last()
		elseif cards:last():isKindOf("Dismantlement") or  cards:last():isKindOf("Snatch") then
			qishu_card=cards:first()
		elseif cards:first():isKindOf("Slash") and sgs.Slash_IsAvailable(self.player) then
			qishu_card=cards:last()
		elseif cards:last():isKindOf("Slash") and sgs.Slash_IsAvailable(self.player) then
			qishu_card=cards:first()
		end
	end
	sgs.ai_use_priority.DfgzmSiyuCard = (qishu_card and can_qishu) and 8 or 0
	if not qishu_card then
		cards = sgs.QList2Table(cards)
		self:sortByKeepValue(cards)
		qishu_card=cards[1]
	end
	if (qishu_card) then
		return sgs.Card_Parse("@DfgzmSiyuCard=" .. qishu_card:getEffectiveId())
	end
	return nil
end
sgs.ai_skill_use_func.DfgzmSiyuCard = function(card, use, self)
	local target
	self:sort(self.enemies,"handcard")
	for var=#self.enemies, 1, -1 do
		if self.enemies[var]:hasSkills("chunxi|xingyun") then
			local heart=sgs.Sanguosha:getCard(card:getSubcards():first())
			if heart:getSuit()~=sgs.Card_Heart then
				target=self.enemies[var]
				break
			end
		else
			target=self.enemies[var]
			break
		end
	end
	if target then
		use.card=card
		if use.to then
			use.to:append(target)
			if use.to:length()>=1 then return end
		end
	end
end
sgs.ai_skill_cardchosen.dfgzmsiyu = function(self, who, flags)
	local cards= sgs.QList2Table(who:getCards("hs"))
	local inverse = not self:isFriend(who)
	self:sortByKeepValue(cards, inverse)
	return cards[1]
end







sgs.ai_slash_prohibit.hpymsiyu = function(self, from, to, card)
	if self:isFriend(from,to) then
		return false
	end
	local callback=sgs.ai_damage_prohibit["hpymsiyu"]
	local damage = sgs.DamageStruct(card, from, to, 1, self:touhouDamageNature(card,from,to))
	return callback(self, from, to, damage)
end
sgs.ai_trick_prohibit.hpymsiyu = function(self, from, to, card)
	if self:isFriend(from,to) then return false end
	local isDamage=false
	if ( card:isKindOf("Duel") or card:isKindOf("AOE") or card:isKindOf("FireAttack")
			or sgs.dynamic_value.damage_card[card:getClassName()]) then
		isDamage=true
	end
	if isDamage then
		local callback=sgs.ai_damage_prohibit["hpymsiyu"]
		local damage = sgs.DamageStruct(card, from, to, 1, self:touhouDamageNature(card,from,to))
		return callback(self, from, to, damage)
	end
	return false
end
sgs.ai_damage_prohibit.hpymsiyu = function(self, from, to, damage)
	if not to:hasSkills("hpymsiyu+juhe") then return false end
	if to:getPhase() ~=sgs.Player_NotActive then return false end
	if self:isFriend(from,to) then return false end
	--强制不去集火
	for _,p in sgs.qlist(self.room:getOtherPlayers(to)) do
		if not self:isFriend(from,p) then
			return true
		end
	end
	--后面的暂且不管了。。。。
	if self:touhouDamage(damage,from,to).damage < to:getHp() then
		return false
	end
	if to:containsTrick("indulgence") or to:containsTrick("supply_shortage") then
		return true
	end
	if not to:faceUp() then
		return true
	end
	local recoverNum = self:getAllPeachNum(to) + getCardsNum("Analeptic", to, self.player)
	return recoverNum>0
end


sgs.ai_skill_invoke.juhe = true
--sgs.ai_skill_discard.juhe = sgs.ai_skill_discard.gamerule
sgs.ai_skill_discard.juhe = function(self,discard_num)
	local Weapon=self.player:getWeapon()
	local Distance=self.player:getOffensiveHorse()
	if  Distance and Weapon then
		local gamerule = sgs.QList2Table(self.player:getCards("hs"))
		self:sortByKeepValue(gamerule,false)
		local gamerule_discard={}
		for var=1, discard_num ,1 do
			table.insert(gamerule_discard,gamerule[var]:getId())
		end
		return gamerule_discard
	end
	local cards = {}
	local tmp_dis
	local weapons={}
	for _,c in sgs.qlist(self.player:getCards("hs")) do
		if not Distance and c:isKindOf("OffensiveHorse") and not tmp_dis then
			tmp_dis=c
		elseif not Weapon and c:isKindOf("Weapon") then
				table.insert(weapons,c)
		else
			table.insert(cards,c)
		end
	end
	if #weapons>1 then
		self:sortByUseValue(weapons,false)
		for var=2, #weapons ,1 do
			table.insert(cards,weapons[var])
		end
	end

	self:sortByKeepValue(cards)
	local to_discard = {}
	for var=1, discard_num ,1 do
		table.insert(to_discard,cards[var]:getId())
	end
	return to_discard
end
function SmartAI:touhouBreakDamage(damage,to)
	if to:hasSkill("hpymsiyu") and to:getPhase()==sgs.Player_NotActive then
		if to:getHp()>0   then
			return damage.damage>= to:getHp()
		else
			return damage.damage>0
		end
	end
	return false
end




local yinyang_skill = {}
yinyang_skill.name = "yinyang"
table.insert(sgs.ai_skills, yinyang_skill)
yinyang_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("YinyangCard") then return nil end
	return sgs.Card_Parse("@YinyangCard=.")
end
sgs.ai_skill_use_func.YinyangCard=function(card,use,self)
	--local heart = getKnownCard(self.player, self.player, "heart", viewas, "hs", false)

	local red = getKnownCard(self.player, self.player, "red", viewas, "hs", false)
	local black = getKnownCard(self.player, self.player, "black", viewas, "hs", false)
	if red > 0 and black > 0 then
		local targets = {}
		for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if not p:isKongcheng() then
				if self:isFriend(p) and p:isWounded() then
					table.insert(targets, p)
				elseif self:isEnemy(p) and not self:isWeak(self.player) then
					table.insert(targets, p)
				end
			end
		end
		self:sort(targets,"hp")
		if #targets >0 then
			use.card = card
			if use.to then
				use.to:append(targets[1])
				if use.to:length() >= 1 then return end
			end
		end
	end
end
sgs.ai_skill_cardask["@yinyang_discard"] = function(self, data)
	local cards = sgs.QList2Table(self.player:getHandcards())
	if self.player:getPhase() == sgs.Player_Play then
		local target = self.player:getTag("yinyang_target"):toPlayer()
		local card = self.player:getTag("yinyang_card"):toCard()

		local same = target and not self:isFriend(target)
		if card then
			local tmp = {}
			local hearts = {}
			for _,c in pairs (cards) do
				if same == c:sameColorWith(card) then
					table.insert(tmp, c)
					if c:getSuit() == sgs.Card_Heart and self.player:hasSkill("lingji") then
						table.insert(hearts, c)
					end
				end
			end

			self:sortByCardNeed(hearts)
			if #hearts > 0 then return "$" .. hearts[1]:getId() end
			self:sortByCardNeed(tmp)
			if #tmp > 0 then  return "$" .. tmp[1]:getId() end
		end
	end

	self:sortByCardNeed(cards)
	return "$" .. cards[1]:getId()
end


sgs.ai_skill_playerchosen.lingji = function(self, targets)
	local target_table =sgs.QList2Table(targets)
	self:sort(target_table, "hp")
	for _,p in pairs (target_table) do
		if self:isEnemy(p) then
			local fakeDamage=sgs.DamageStruct()
			fakeDamage.card=nil
			fakeDamage.nature= sgs.DamageStruct_Normal
			fakeDamage.damage=1
			fakeDamage.from=self.player
			fakeDamage.to=p
			local willDamage = self:touhouNeedAvoidAttack(fakeDamage, self.player, p)
			if willDamage then
				return p
			end
		end
	end
	return nil
end
sgs.ai_playerchosen_intention.lingji = 60
sgs.ai_cardneed.lingji = function(to, card, self)
	return card:getSuit()==sgs.Card_Heart
end
sgs.lingji_suit_value = {
	heart = 4.8
}


sgs.ai_skill_use["@@toushi"] = function(self, prompt)
	local cards = {}
	for _,c in sgs.qlist(self:touhouAppendExpandPileToList(self.player, self.player:getCards("hes"))) do
		if c:isKindOf("BasicCard") or c:getSuit() == sgs.Card_Spade then
			table.insert(cards, c)
		end
	end
	if #cards == 0 then return "." end
	self:sortByUseValue(cards)


	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	local cardname=self.player:property("toushi_card"):toString()
	local card=sgs.cloneCard(cardname, cards[1]:getSuit(), cards[1]:getNumber())
	card:addSubcard(cards[1])
	card:setSkillName("toushi")
	card:deleteLater()
	local target
	if card:isKindOf("TrickCard") then
		self:useTrickCard(card, dummy_use)
	else
		self:useBasicCard(card, dummy_use)
	end

	if not dummy_use.card then return false end
	if dummy_use.to:isEmpty() then
		if card:isKindOf("IronChain") then
			return "."
		end
		return dummy_use.card:toString()
	else
		local target_objectname = {}
		if card:isKindOf("IronChain") then
			for _, p in sgs.qlist(dummy_use.to) do
				if (self:isEnemy(p) and not p:isChained())
				or (self:isFriend(p) and p:isChained())then
					table.insert(target_objectname, p:objectName())
				end
				if #target_objectname==2 then break end
			end
		else
			for _, p in sgs.qlist(dummy_use.to) do
				if self:isEnemy(p) then
					table.insert(target_objectname, p:objectName())
					target=p
					break
				end
			end
		end

		if card:isKindOf("Collateral") then
			local victim
			for _,p in sgs.qlist(self.room:getOtherPlayers(target))do
				if self:isEnemy(p) and target:canSlash(p,nil,true) then
					table.insert(target_objectname, p:objectName())
					victim=p
					break
				end
			end
			if not victim then
				return "."
			end
		end
		if #target_objectname>0 then
			return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
		end
	end
	return "."
end
sgs.ai_cardneed.toushi = function(to, card, self)
	return card:isKindOf("BasicCard") or card:getSuit() == sgs.Card_Spade
end


sgs.ai_skill_playerchosen.moli = function(self, targets)
	targets=sgs.QList2Table(targets)
	self:sort(targets,"hp")
	for _,p in pairs(targets) do
		if (self:isFriend(p) and p:isWounded() and getBestHp(p) > p:getHp()) then
			return p
		end
	end
	return nil
end
sgs.ai_playerchosen_intention.moli = -70
sgs.ai_no_playerchosen_intention.moli =function(self, from)
	local lord =self.room:getLord()
	if lord and  lord:isWounded() and getBestHp(lord) > lord:getHp()  then
		sgs.updateIntention(from, lord, 10)
	end
end


local bodong_skill = {}
bodong_skill.name = "bodong"
table.insert(sgs.ai_skills, bodong_skill)
function bodong_skill.getTurnUseCard(self)
		if self.player:hasUsed("BodongCard") then return nil end
		if not self.player:canDiscard(self.player, "hs") then return nil end
		local cards = sgs.QList2Table(self.player:getCards("hs"))
		self:sortByUseValue(cards)
		return sgs.Card_Parse("@BodongCard="..cards[1]:getEffectiveId())
end
sgs.ai_skill_use_func.BodongCard = function(card, use, self)
		self:sort(self.enemies, "defense")
        local targets = {}
		
			for _, p in ipairs(self.enemies) do
				if #targets >= 3 then
					break
				end
				local num = p:getCards("e"):length() - p:getBrokenEquips():length()
				local available = math.min(num,3)
				if num > 0 then 
					for i=1, num  do
						table.insert(targets, p)	
						if #targets >= 3 then
							break
						end
					end
				end
			end
		
		if #targets > 0 then 
			use.card = card
			if  use.to then
				for _,p in ipairs (targets) do
					use.to:append(p)
				end
				return 
			end  --use.to:length() >= 1 
		end
end

sgs.ai_use_value.BodongCard = 8
sgs.ai_use_priority.BodongCard =7
sgs.ai_card_intention.BodongCard = 20

sgs.ai_skill_invoke.huanlong =function(self,data)
	local target = data:toPlayer()
	if target and self:isEnemy(target) then return true end
	return  false
end
sgs.ai_choicemade_filter.cardChosen.huanlong = sgs.ai_choicemade_filter.cardChosen.dismantlement
