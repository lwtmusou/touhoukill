
function sgs.ai_cardsview_valuable.skltkexue_attach(self, class_name, player)
	if class_name == "Peach" and player:getHp()>1 then
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
	local to=player:getTag("xueyi-target"):toPlayer()
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
		if player:distanceTo(p) > 1 then continue end
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
	Peach 			= 5.5,
	Analeptic 		= 5.5,
	Jink 			= 4.2,
	FireSlash 		= 5.6,
	Slash 			= 5.4,
	ThunderSlash 	= 5.5
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
		enemy_check=false;
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
	if self.player:getLostHp() < 1 then 
		return self:needtouhouDamageJudge()
	end
	return false
end
sgs.ai_skillProperty.huisu = function(self)
	return "cause_judge"
end

sgs.ai_skill_invoke.bolan = function(self)
	local current = self.room:getCurrent()
	if current and current:hasSkill("souji") and not self:isFriend(current) then
		return self.player:getPile("yao_mark"):length() + self.player:getHandcardNum() <= self.player:getMaxHp()
	end
	if self.player:getPile("yao_mark"):length()>0 then
		return true
	else
		if self.player:getHandcardNum()> 5 then
			return false
		end
	end
	return true
end


sgs.ai_skill_discard.qiyao_got = sgs.ai_skill_discard.gamerule


function sgs.ai_cardsview_valuable.qiyao(self, class_name, player)
	if class_name == "Peach" then
		if (sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
			return nil
		end
		if self.player:getPhase()~= sgs.Player_NotActive then return nil end
		if self.player:getMark("Global_PreventPeach")>0 then return nil end
		
		
		local hand_trick={}
		local real_peach={}
		local cards = self.player:getHandcards()
		cards=self:touhouAppendExpandPileToList(self.player,cards)
	
		for _,c in sgs.qlist(cards) do
			if c:isKindOf("Peach") then
				table.insert(real_peach,c)
			elseif c:isNDTrick()then
				table.insert(hand_trick,c)
			end
		end
		
		if #real_peach<1 and  self:hasWeiya()  then
			return nil
		end
	
		local card
		if #hand_trick>0 then
			self:sortByKeepValue(hand_trick)
			card=hand_trick[1]
		end
		if card then
			local suit = card:getSuitString()
			local number = card:getNumberString()
			local card_id = card:getEffectiveId()
			return ("peach:qiyao[%s:%s]=%d"):format(suit, number, card_id)	
		end
		return nil
	end
end
sgs.qiyao_keep_value = {
	Peach = 10, 
	TrickCard = 8
}
sgs.ai_cardneed.qiyao = function(to, card, self)
	return getCardsNum("TrickCard", to, self.player) <1
	 and card:isKindOf("TrickCard")
end


sgs.ai_skill_invoke.neijin = function(self)
	local current = self.room:getCurrent() 
	if current then
		return self:isFriend(current)
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.neijin = function(self, player, promptlist)
	local current = self.room:getCurrent() 
	if  current and  promptlist[#promptlist] == "yes" then
		sgs.updateIntention(player, current, -40)
	end
end

sgs.ai_skill_playerchosen.taiji = function(self, targets)
	if #self.enemies > 0 then
		self:sort(self.enemies, "hp")
		return self.enemies[1]
	end
    return nil	
end


--[[
sgs.ai_skill_cardask["douhun-slash"]  = function(self, data, pattern, target)
	local effect = data:toSlashEffect()
	local p
	if effect.from:objectName()==self.player:objectName() then
		p=effect.to
	else
		p=effect.from
	end
	if self:isFriend(p) then return "." end
	if self.player:hasSkill("zhanyi") then
		return self:getCardId("Slash")
	elseif p:hasSkill("zhanyi") then
		local rate=1
		if self.player:hasSkill("weiya") and self:hasWeiya(p) then
			rate=2
		end
		if self:getCardsNum("Slash") > (getCardsNum("Slash", p, self.player)+p:getPile("qi"):length())*rate then
			return self:getCardId("Slash")
		end
	end
	return "."
end
sgs.ai_cardneed.douhun = function(to, card, self)
	return getCardsNum("Slash", to, self.player) <2
	 and card:isKindOf("Slash")
end

sgs.ai_slash_prohibit.douhun = function(self, from, to, card)
	
	if self.player:objectName()~=from:objectName() then return false end
	if to:hasSkills("douhun+zhanyi") then
		local rate=1
		if from:hasSkill("weiya") and self:hasWeiya() then
			rate=2
		end
		if self:getCardsNum("Slash") <= 
		(getCardsNum("Slash", to, from)+to:getPile("qi"):length())*rate then
			return true
		end
	end
	return false
end	
]]
--[[sgs.ai_view_as.zhanyi = function(card, player, card_place)
        local suit = card:getSuitString()
        local number = card:getNumberString()
        local card_id = card:getEffectiveId()
        if card_place == sgs.Player_PlaceHand and not card:isKindOf("Peach") and not card:hasFlag("using") then
                return ("slash:zhanyi[%s:%s]=%d"):format(suit, number, card_id)
        end
end]]
--[[
sgs.ai_skill_cardask["@zhanyi"] = function(self, data)
	
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards)
	
	local maxNum = #cards - self.player:getMaxCards()
	local qis={}
	for _,c in pairs(cards) do
		if  not c:isKindOf("Peach") and not c:isKindOf("Slash")  then
			table.insert(qis,c:getEffectiveId())
		end
		if #qis >= maxNum then
			break
		end
	end
	local canAddSlash = #qis < maxNum
	if canAddSlash then
		for _,c in sgs.qlist(cards) do
			if  c:isKindOf("Slash")  then
				table.insert(qis,c:getEffectiveId())
			end
			if #qis >= maxNum then
				break
			end
		end
	end

	if #qis==0 then return "." end
	return "$" .. table.concat(qis, "+")
end
]]
--[[
function sgs.ai_cardsview_valuable.zhanyi(self, class_name, player)
	if class_name == "Slash" then
		if (sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE) then
			return nil
		end
		if self.player:getPile("qi"):isEmpty() then return nil end
		local ids=self.player:getPile("qi")
		local card= sgs.Sanguosha:getCard(ids:first())
		local suit = card:getSuitString()
        local number = card:getNumberString()
        local card_id = card:getEffectiveId()
		return ("slash:zhanyi[%s:%s]=%d"):format(suit, number, card_id)
	end
end]]

sgs.ai_cardneed.zhanyi = function(to, card, self)
	return card:isKindOf("Slash")
	--return  card:isRed()
end
sgs.ai_skill_invoke.zhanyi = function(self)
	return true
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
	Peach 			= 5.5,
	Slash 			= 6.4
}


sgs.ai_slash_prohibit.bingpo = function(self, from, to, card)
	if card:isKindOf("FireSlash") or from:hasSkill("here") then
		if not self:isFriend(to) then
			return false
		else
			return true
		end
	end
	if to:getHp()==1 then
		if not self:isFriend(to) then
			return true
		else
			return false
		end
	end
	if self:hasHeavySlashDamage(from, card, to)then
		if not self:isFriend(to) then
			return true
		else
			return false
		end
	end
	return false
end
sgs.ai_damageInflicted.bingpo =function(self, damage)
	if damage.nature ~= sgs.DamageStruct_Fire then
		if damage.damage>1 or damage.to:getHp()==1 then
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



sgs.ai_skill_choice.anyu= function(self, choices, data)	
	if self.player:faceUp() then
		return "draw"
	end
	
	if self.player:containsTrick("indulgence") or self.player:containsTrick("supply_shortage") then
		return "draw"
	end
	
	local use=data:toCardUse()
	local dongjie=false
	if use.from and use.card:isKindOf("Slash") 
	and use.from:hasSkill("dongjie") and self:isFriend(use.from)  then
		dongjie=true
	end
	if dongjie then
		return "draw"
	elseif self:isWeak(self.player) and self:getOverflow() <2 then
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

	--local turnFriend =false
	--for _,p in pairs (self.friends) do
	--	if not p:faceUp() then
	--		turnFriend = true
	--	end
	--end
	--return not self:isWeak(to) and turnFriend
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

--嘲讽值设定
--[[sgs.ai_chaofeng.hmx001 = 2
sgs.ai_chaofeng.hmx002 = -2
sgs.ai_chaofeng.hmx003 = -2
sgs.ai_chaofeng.hmx004 = 3
sgs.ai_chaofeng.hmx005 = -1
sgs.ai_chaofeng.hmx006 = 0
sgs.ai_chaofeng.hmx007 = 0
sgs.ai_chaofeng.hmx008 = 4
sgs.ai_chaofeng.hmx009 = 3]]

