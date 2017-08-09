
function sgs.ai_cardsview_valuable.skltkexue_attach(self, class_name, player)
	if class_name == "Peach" and player:getHp()> player:dyingThreshold() then
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying or not dying:hasSkill("skltkexue") or self:isEnemy(dying, player) or dying:objectName() == player:objectName() then return nil end

		if self:isFriend(dying, player) then
			if self.role == "renegade" then
				local need_hp = math.abs(1 - dying:getHp())
				local others_hp = 0
				for _,p in pairs(self.friends_noself)do
					if p:getHp()>1 then
						others_hp = others_hp + p:getHp() - 1
					end
				end
				if others_hp >= need_hp then return nil end
			end
			return "@SkltKexueCard=."
		end
		return nil
	end
end
sgs.ai_card_intention.SkltKexueCard = sgs.ai_card_intention.Peach
sgs.ai_use_priority.SkltKexueCard = sgs.ai_use_priority.Peach + 0.1
function SmartAI:canKexue(player)
	if not player:hasSkill("skltkexue") then
		return false
	end
	for _,p in sgs.qlist(self.room:getOtherPlayers(player)) do
		if self:isFriend(p, player) and p:getHp() > 1  then
			return true
		end
	end
	return false
end

function SmartAI:invokeTouhouJudge(player)
	player = player or self.player
	local wizard_type ,wizard = self:getFinalRetrial()
	local value = 0
	if wizard   then
		if self:isFriend(wizard,player) then
			value = value + 1
		elseif  self:isEnemy(wizard,player) then
			value = value - 1
		end
	end
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:hasSkills("feixiang|mingyun") then
			if self:isFriend(p,player) then
				value = value + 1
			else
				value = value - 1
			end
		end
	end
	return value>=0,value
end
function SmartAI:needtouhouDamageJudge(player)
	player = player or self.player
	local wizard_type ,wizard = self:getFinalRetrial()
	local value = 0
	if wizard  and self:isFriend(wizard,player) then
		value = value + 1
	end
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:hasSkills("feixiang|mingyun") then
			if self:isFriend(p,player) then
				value = value + 1
			else
				value = value - 1
			end
		end
	end
	return value>0
end
function SmartAI:slashProhibitToEghitDiagram(card,from,enemy)
	if self:isFriend(from,enemy) then return false end
	if self:hasEightDiagramEffect(enemy) then
		local invoke, value = self:invokeTouhouJudge(enemy)
		if value>0 then
			if not self:touhouIgnoreArmor(card,from,enemy)  then
				return true
			end
		end
	end
	return false
end

--sgs.ai_skill_invoke.EightDiagram
--sgs.ai_armor_value.EightDiagram
--SmartAI:getFinalRetrial
--SmartAI:canRetrial
sgs.ai_skill_invoke.mingyun = true
sgs.ai_skill_askforag.mingyun = function(self, card_ids)
	local judge = self.player:getTag("mingyun_judge"):toJudge()
	local mingyun={}
	local mingyun1={}
	local mingyun2={}

	judge.card = sgs.Sanguosha:getCard(card_ids[1])
	table.insert(mingyun1,judge.card)
	table.insert(mingyun,judge.card)
	local ex_id1=self:getRetrialCardId(mingyun1, judge)

	judge.card = sgs.Sanguosha:getCard(card_ids[2])
	table.insert(mingyun2,judge.card)
	table.insert(mingyun,judge.card)
	local ex_id2=self:getRetrialCardId(mingyun2, judge)

	--ex_id==-1 means the Retrial result are not good for remilia
	if ex_id1==ex_id2 or (ex_id1~=-1 and  ex_id2~=-1) then
		self:sortByKeepValue(mingyun,true)
		return mingyun[1]:getId()
	elseif ex_id1==-1 then
		return card_ids[1]
	elseif ex_id2==-1 then
		return card_ids[2]
	end
	return card_ids[1]
end
sgs.ai_skillProperty.mingyun = function(self)
	return "wizard_harm"
end



sgs.ai_skill_invoke.xueyi = function(self, data)
	local to =data:toPlayer()
	return self:isFriend(to)
end
sgs.ai_choicemade_filter.skillInvoke.xueyi = function(self, player, promptlist)
	local to = player:getTag("xueyi-target"):toPlayer()
	if to then
		if promptlist[#promptlist] == "yes" then
			sgs.updateIntention(player, to, -60)
		else
			sgs.updateIntention(player, to, 60)
		end
	end
end

function SmartAI:pohuaiBenefit(player)
	local value=0
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if not (player:distanceTo(p) > 1) then  
			local damage=sgs.DamageStruct("pohuai", player, p, 1, sgs.DamageStruct_Normal)
			local final_damage=self:touhouDamage(damage,player, p)
			if final_damage.damage>0 then
				if self:isFriend(p) then
					if self:getDamagedEffects(player) then
						value = value+1
					else
						value = value-1
					end
				elseif self:isEnemy(p) then
					if self:getDamagedEffects(player) then
						value = value-1
					else
						value = value+1
					end
				end
				if player:hasSkill("shengyan") then
					if self:isFriend(player) then
						value = value + 1
					else
						value = value - 1
					end
				end
			end
		end
	end
	return value
end
sgs.ai_cardneed.pohuai = function(to, card, self)
	if to:hasSkill("shengyan") then
		if not to:getOffensiveHorse() and getCardsNum("OffensiveHorse", to, self.player) < 1 then
			return  card:isKindOf("OffensiveHorse")
		end
	end
end
sgs.ai_skillProperty.pohuai = function(self)
	return "cause_judge"
end
sgs.ai_judge_model.pohuai = function(self, who)
	local judge = sgs.JudgeStruct()
	judge.who = who
	judge.pattern = "Slash"
	judge.good = true
	judge.reason = "pohuai"
	return judge
end

sgs.yuxue_keep_value = {
	Peach           = 5.5,
	Analeptic       = 5.5,
	Jink            = 4.2,
	FireSlash       = 5.6,
	Slash           = 5.4,
	ThunderSlash    = 5.5
}
sgs.ai_need_damaged.yuxue = function(self, attacker, player)
	local hasGoodTarget=false
	local hasGoodState=false
	local hasSlash=false
	for _,p in sgs.qlist(self.room:getOtherPlayers(player)) do
		if self:isEnemy(p) then
			if getCardsNum("Jink", p, player) < 1 or sgs.card_lack[p:objectName()]["Jink"] == 1 or self:isWeak(p) then
				hasGoodTarget=true
				break
			end
		end
	end

	if getCardsNum("Slash",player,self.player) > 0 then
		hasSlash=true
	end
	if player:getHp()>2 then
		hasGoodState=true
	elseif player:getHp()==2 then
		if getCardsNum("Peach",player,self.player) > 0 then
			hasGoodState=true
		end
	end

	if hasGoodTarget and hasGoodState and hasSlash then
		return true
	end
	return false
end
sgs.ai_cardneed.yuxue = function(to, card, self)
	return getCardsNum("Slash", to, self.player) <2
	 and card:isKindOf("Slash")
end

sgs.ai_skill_invoke.shengyan = function(self)
		return not self:needKongcheng(self.player, true)
end


local suoding_skill = {}
suoding_skill.name = "suoding"
table.insert(sgs.ai_skills, suoding_skill)
function suoding_skill.getTurnUseCard(self)
		if self.player:hasUsed("SuodingCard") then return nil end
		return sgs.Card_Parse("@SuodingCard=.")
end
sgs.ai_skill_use_func.SuodingCard = function(card, use, self)
		self:sort(self.enemies, "handcard")
		over=math.min(self:getOverflow(),3)
		enemy_check=false
		if over >0 then
			use.card = card

			if use.to then
			   for i=1,  over do
					use.to:append(self.player)
			   end
			end
		end
		if (use.to and use.to:length() < 3) then
			for _, p in ipairs(self.enemies) do
				if use.to:length() >= 3 then
					break
				end
				if not p:isKongcheng() and not self:touhouHandCardsFix(p)then
					local mincards=math.min(p:getHandcardNum(),3)
					use.card = card
					for i=1, mincards  do
						if use.to then
							use.to:append(p)
							if use.to:length() >= 3 then
								break
							end
						end
					end
				end
			end
		end

		use.card = card
		if  use.to  and enemy_check and use.to:length() >= 1 then return end
end

sgs.ai_use_value.SuodingCard = 8
sgs.ai_use_priority.SuodingCard =7
sgs.ai_card_intention.SuodingCard = 20


sgs.ai_skill_invoke.huisu = function(self)
	return self:invokeTouhouJudge()
end
sgs.ai_need_damaged.huisu = function(self, attacker, player)
	if self.player:getLostHp() < 1 and not self.player:hasFlag("huisu") then
		return self:needtouhouDamageJudge()
	end
	return false
end
sgs.ai_skillProperty.huisu = function(self)
	return "cause_judge"
end

sgs.ai_skill_invoke.bolan = true

function sgs.ai_cardsview_valuable.hezhou(self, class_name, player)
	if self:touhouClassMatch(class_name, "Peach") then
		if (sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
			return nil
		end
		if self.player:getPhase()~= sgs.Player_NotActive then return nil end
		if self.player:getMark("Global_PreventPeach")>0 then return nil end
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying or self.player:getLostHp() < 1 then return nil end

		local all ={}
		local real_peach={}
		local cards = self.player:getCards("hes")
		cards=self:touhouAppendExpandPileToList(self.player,cards)
		for _,c in sgs.qlist(cards) do
			if c:isKindOf("Peach") then
				table.insert(real_peach,c)
			else
				table.insert(all,c)
			end
		end

		if #real_peach<1 and  self:hasWeiya()  then
			return nil
		end

		local cards = {}
		self:sortByKeepValue(all)
		local function isCombine(card1, card2)
			if card1:getType() == card2:getType() then
				return false
			end
			if (card1:isKindOf("WoodenOx") and self.player:getPile("wooden_ox"):contains(card2:getId())) then
				return false
			elseif (card2:isKindOf("WoodenOx") and self.player:getPile("wooden_ox"):contains(card1:getId())) then
				return false
			end
			return true
		end

		for index, c in ipairs(all) do
			for index1 = index+1, #all, 1 do
				if (isCombine(c, all[index1])) then
					table.insert(cards,c)
					table.insert(cards,all[index1])
					break
				end
			end
			if #cards>0 then
				break
			end
		end


		if #cards == 2 then
			local card_id1 = cards[1]:getEffectiveId()
			local card_id2 = cards[2]:getEffectiveId()

			local card_str = ("peach:%s[%s:%s]=%d+%d"):format("hezhou", "to_be_decided", 0, card_id1, card_id2)
			return card_str
		end
		return nil
	end
end
sgs.hezhou_keep_value = {
	Peach = 10,
	TrickCard = 8
}
sgs.ai_cardneed.hezhou = function(to, card, self)
	local ClassName = {"TrickCard", "EquipCard", "BasicCard"}
	for _,class in ipairs(ClassName)  do
		if card:isKindOf(class) and getCardsNum(class, to, self.player) <1 then
			return true
		end
	end
end

sgs.ai_playerchosen_intention.hezhou = -70
sgs.ai_no_playerchosen_intention.hezhou =function(self, from)
	local lord =self.room:getLord()
	if lord  then
		sgs.updateIntention(from, lord, 10)
	end
end
sgs.ai_skill_playerchosen.hezhou = function(self, targets)
	local target =self:touhouFindPlayerToDraw(false, 1)
	if not target and #self.friends_noself>0 then
		target= self.friends_noself[1]
	end
	if target then
		return target
	end
	return nil
end

sgs.ai_skill_invoke.taiji = function(self, data)
	local use=self.player:getTag("taiji"):toCardUse()
	--默认只补给自己人
	if self:isFriend(use.to:first()) then
		if self.player:objectName() ~= use.to:first():objectName() then
			return true
		elseif self:isFriend(use.from) then
			return true
		else
			return getCardsNum("Slash", use.from, self.player) <= 1
		end
	end
	return false
end


local beishui_skill = {}
beishui_skill.name = "beishui"
table.insert(sgs.ai_skills, beishui_skill)
beishui_skill.getTurnUseCard = function(self)
	if self.player:getMark("beishui") >0 then return nil end
	local x = math.max(self.player:getHp(), 1)

	local cards = self.player:getCards("hes")
	cards=self:touhouAppendExpandPileToList(self.player,cards)
	cards = sgs.QList2Table(cards)
	if #cards < x then return nil end
	self:sortByKeepValue(cards)
	local ids = {}
	local count = 0
	for _,c in pairs(cards) do
		table.insert(ids, c:getEffectiveId())
		count = count + 1
		if (count >= x) then break end
	end

	local beishuiCards = {}
	local pattern = "slash|peach" --|analeptic
	local patterns = pattern:split("|")
	for i = 1, #patterns do
		local forbidden = patterns[i]
		local forbid = sgs.cloneCard(forbidden)
		if not self.player:isLocked(forbid) and forbid:isAvailable(self.player) then
			table.insert(beishuiCards,forbid)
		end
	end

	if #beishuiCards < 1 then return nil end
	self:sortByUseValue(beishuiCards, false)
	local choice = beishuiCards[1]:objectName()
	local card_str = (choice..":%s[%s:%s]="):format("beishui", "to_be_decided", -1)
	for _,id in pairs(ids) do
		if id == ids[#ids] then
			card_str = card_str .. id
		else
			card_str = card_str .. id .. "+"
		end
	end


	local parsed_card = sgs.Card_Parse(card_str)
	return parsed_card
end
function sgs.ai_cardsview_valuable.beishui(self, class_name, player)
	if (sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
		return nil
	end
	if self.player:getMark("beishui") > 0 then return nil end
	local x = math.max(self.player:getHp(), 1)
	local cards = self.player:getCards("hes")
	cards=self:touhouAppendExpandPileToList(self.player,cards)
	cards = sgs.QList2Table(cards)
	if #cards < x then return nil end
	self:sortByKeepValue(cards)
	local ids = {}
	local count = 0
	for _,c in pairs(cards) do
		table.insert(ids, c:getEffectiveId())
		count = count + 1
		if (count >= x) then break end
	end


	local card_str
	if class_name == "Peach" then
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying  then return nil end
		card_str =  ("peach:beishui[%s:%s]="):format("to_be_decided", -1)
	elseif class_name == "Jink" then
		card_str = ("jink:beishui[%s:%s]="):format("to_be_decided", -1)
	elseif class_name == "Slash" then
		card_str = ("slash:beishui[%s:%s]="):format("to_be_decided", -1)
	else
		return nil
	end
	for _,id in pairs(ids) do
		if id == ids[#ids] then
			card_str = card_str .. id
		else
			card_str = card_str .. id .. "+"
		end
	end
	return card_str
end


sgs.ai_skill_invoke.dongjie = function(self, data)
		local damage =self.player:getTag("dongjie"):toDamage()
		local to = damage.to
		local final_damage = self:touhouDamage(damage, self.player, to, 2)
		local needAvoidAttack = self:touhouNeedAvoidAttack(damage,self.player,to,true, 2)
		if self:isEnemy(to) and needAvoidAttack then
			if final_damage.damage > 1  or final_damage.damage >= to:getHp()  then return false end
		end
		return self:isFriend(to) ~= to:faceUp()
end
sgs.ai_choicemade_filter.skillInvoke.dongjie = function(self, player, args)
	local to=player:getTag("dongjie_damage"):toDamage().to
	if to then
		if to:faceUp() then
			if args[#args] == "yes" then
				sgs.updateIntention(player, to, 60)
			end
		else
			if args[#args] == "yes" then
				sgs.updateIntention(player, to, -60)
			else
				sgs.updateIntention(player, to, 60)
			end
		end
	end
end
sgs.ai_cardneed.dongjie = function(to, card, self)
	return getCardsNum("Slash", to, self.player) <1
	 and card:isKindOf("Slash")
end
sgs.dongjie_keep_value = {
	Peach           = 5.5,
	Slash           = 6.4
}


sgs.ai_damageInflicted.bingpo =function(self, damage)
	if damage.nature ~= sgs.DamageStruct_Fire then
		if damage.damage >= damage.to:getHp() then
		--if damage.damage>1 or damage.to:getHp()==1 then
			damage.damage=0
		end
	end
	return damage
end



sgs.ai_skill_playerchosen.zhenye = function(self, targets)
	local target_table= sgs.QList2Table(targets)
	self:sort(target_table,"hp")
	for _,p in pairs (target_table) do
		if self:isFriend(p) and not p:faceUp()then
			 return p
		end
		if self:isEnemy(p) and p:faceUp()then
			 return p
		end
	end
	return nil
end
sgs.ai_playerchosen_intention.zhenye =function(self, from, to)
	local intention = 0
	if to:faceUp() then
		intention = 80
	else
		intention = -30
	end
	sgs.updateIntention(from, to, intention)
end


sgs.ai_skill_invoke.anyu = true
sgs.ai_skill_choice.anyu= function(self, choices, data)
	if self.player:faceUp() then
		return "draw"
	end

	if self.player:containsTrick("indulgence") or self.player:containsTrick("supply_shortage") then
		return "draw"
	end

	--[[local damage =data:toDamage()
	local dongjie=false
	if damage.from and damage.from:hasSkill("dongjie") and self:isFriend(use.from)  then
		dongjie=true
	end
	if dongjie then
		return "draw"
	else]]
	if self:isWeak(self.player) and self:getOverflow() <2 then
		return "draw"
	elseif self:getOverflow() >=2 then
		return "turnover"
	end
	return "turnover"
end
sgs.ai_slash_prohibit.anyu = function(self, from, to, card)
	if not card:isBlack() or not to:hasSkill("zhenye") then return false end

	if self:isFriend(from, to) then return false end
	return not self:isWeak(to) and not to:faceUp()
end


--function SmartAI:getEnemyNumBySeat(from, to, target, include_neutral)
qiyue_find_righter = function(room,target)
	local righter
	for _,p in sgs.qlist(room:getOtherPlayers(target)) do
		if target:isAdjacentTo(p) then
			if p:getSeat()-target:getSeat()==1 then
				righter=p
			end
			if target:getSeat()-p:getSeat()==room:getOtherPlayers(target):length() then
				righter=p
			end
		end
	end
	return righter
end
sgs.ai_skill_invoke.qiyue = function(self,data)
	local current=self.room:getCurrent()
	if self:isEnemy(current) then
		if self.player:getMaxHp()== 1 then
			if current:hasSkill("weiya") then
				return false
			end
			local recover =  getCardsNum("Analeptic", self.player, self.player)
			for _,p in ipairs(self.friends) do
				recover = recover + getCardsNum("Peach", p, self.player)
			end
			return recover > 0
		end
		if self.player:getMaxHp()<3 or self.player:getHp()<3 then return true end
		if #self.enemies<3 then return false end
		enemyseat= current:getSeat()
		selfseat=self.player:getSeat()
		count=1
		a =qiyue_find_righter (self.room,current)
		while a:objectName()~=self.player:objectName() do
			if self:isEnemy(a) then
				count=count+1
			end
			a =qiyue_find_righter (self.room,a)
		end
		if count>=3 then
			return true
		end
	end
	return false
end
sgs.ai_skill_choice.qiyue=function(self)
	if self.player:getMaxHp()>self.player:getHp() then
		return "maxhp"
	else
		return "hp"
	end
end
sgs.ai_choicemade_filter.skillInvoke.qiyue = function(self, player, args)
	local to=self.room:getCurrent()
	if args[#args] == "yes" then
		if self:willSkipPlayPhase(to) and self:getOverflow(to,to:getMaxCards())>1 then
			sgs.updateIntention(player, to, -20)
		else
			sgs.updateIntention(player, to, 60)
		end
	end
end


sgs.ai_skill_invoke.juxian = true

sgs.string2suit = {
		spade = 0 ,
		club = 1 ,
		heart = 2 ,
		diamond = 3
}
sgs.ai_skill_suit.juxian = function(self)
		local cards = self.player:getTag("juxian_cards"):toIntList()
		local suits = {}
		for _, c in sgs.qlist(cards) do
				local suit = sgs.Sanguosha:getCard(c):getSuitString()
				if not suits[suit] then suits[suit] = 0 end
				suits[suit] = suits[suit] + 1
		end
		local maxsuit = sgs.Sanguosha:getCard(cards:at(0)):getSuitString()
		for s, n in pairs(suits) do
				if n > suits[maxsuit] then maxsuit = s end
		end
		return sgs.string2suit[maxsuit] or sgs.Card_Spade
end



local banyue_skill = {}
banyue_skill.name = "banyue"
table.insert(sgs.ai_skills, banyue_skill)
function banyue_skill.getTurnUseCard(self)
		if self.player:getHp() <= 2 and not self.player:hasSkill("juxian") then return nil end
		if self.player:getHp() == 2 and self.player:hasSkill("juxian") then return nil end
		if self.player:hasUsed("BanyueCard") then return nil end
		return sgs.Card_Parse("@BanyueCard=.")
end
sgs.ai_skill_use_func.BanyueCard = function(card, use, self)
		if #self.friends < 2 then return end
		self:sort(self.friends)
		for _, p in ipairs(self.friends) do
				use.card = card
				if use.to then
						use.to:append(p)
						if use.to:length() >= 3 then return end
				end
		end
end

sgs.ai_use_value.BanyueCard = 3
sgs.ai_use_priority.BanyueCard =6
sgs.ai_card_intention.BanyueCard = function(self, card, from, tos)
	sgs.updateIntentions(from, tos, -40)
	if #tos <3 then
		local lord = self.room:getLord()
		local targetList = sgs.SPlayerList()
		for _,to in pairs (tos) do
			targetList:append(to)
		end
		if lord and lord:isAlive() and not targetList:contains(lord) and not lord:hasSkills("yongheng|gaoao") then
			sgs.updateIntention(from,lord, 40)
		end
	end
end

sgs.ai_skill_playerchosen.mizong = function(self, targets)
	for _,target in sgs.qlist(targets) do
		if self:isWeak(target) and self:isEnemy(target) then
			return target
		end
	end
	if #self.enemies > 0 then
		self:sort(self.enemies, "hp")
		return self.enemies[1]
	end
	for _,target in sgs.qlist(targets) do
		if  not self:isFriend(target) then
			return target
		end
	end
	return targets:first()
end
sgs.ai_playerchosen_intention.mizong = 20

sgs.ai_skill_invoke.yinren = function(self, data)
	local target = data:toPlayer()
	return self:isEnemy(target)
end
sgs.ai_cardneed.yinren = function(to, card, self)
	return card:isKindOf("Slash") and card:isBlack()
end
sgs.ai_choicemade_filter.skillInvoke.yinren = function(self, player, promptlist)
	local to = player:getTag("yinren-target"):toPlayer()
	if to and promptlist[#promptlist] == "yes" then
		sgs.updateIntention(player, to, 60)
	end
end



sgs.ai_skill_invoke.xiaoyin = function(self, data)
	local target = self.room:getCurrent()
	return self:isEnemy(target) and self:isWeak(self.player)
end
sgs.ai_skill_use["@@xiaoyinVS!"] = function(self, prompt)
	local targets = {}
	for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if p:hasFlag("Global_xiaoyinFailed") then
			table.insert( targets, p:objectName())
			break
		end
	end
	local cards = self.player:getCards("hs")
	cards=self:touhouAppendExpandPileToList(self.player,cards)
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)

	local card = sgs.cloneCard("lure_tiger", sgs.Card_SuitToBeDecided, -1)
	card:addSubcard(cards[1])
	if #targets > 0 then
	    return card:toString() .. "->" .. table.concat(targets, "+")
	end
	return "."
end
