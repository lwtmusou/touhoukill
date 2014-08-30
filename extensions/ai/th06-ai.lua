--【渴血】ai
--function SmartAI:isWeak
function sgs.ai_cardsview_valuable.skltkexuepeach(self, class_name, player)
	if class_name == "Peach" and  player:getHp()>1 then
		if player:getMark("Global_PreventPeach")>0 then return false end
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying or not dying:hasSkill("skltkexue") or self:isEnemy(dying, player) or dying:objectName() == player:objectName() then return nil end
		if self:isFriend(dying, player) then return "#skltkexuepeach:.:" end
		return nil
	end
end
sgs.ai_card_intention.skltkexuepeach = sgs.ai_card_intention.Peach

function SmartAI:invokeTouhouJudge()
	local tianzi = self.room:findPlayerBySkillName("feixiang")
	local leimi=self.room:findPlayerBySkillName("skltmingyun")
	if leimi and self:isEnemy(leimi) then
		if tianzi and self:isFriend(tianzi) then
			return true
		else
			return false
		end
	end 
	if tianzi and self:isEnemy(tianzi) then
		return false
	end
	return true
end
function SmartAI:needtouhouDamageJudge()
	local tianzi = self.room:findPlayerBySkillName("feixiang")
	local leimi=self.room:findPlayerBySkillName("skltmingyun")
	if leimi and self:isFriend(leimi) then
		if tianzi then
			return self:isFriend(tianzi)
		end
	end 
	return false
end
--【命运】ai 如果对此判定的好坏无法判断 此AI可能会导致闪退
--sgs.ai_skill_invoke.EightDiagram 
--sgs.ai_armor_value.EightDiagram 
--SmartAI:getFinalRetrial
--SmartAI:canRetrial
sgs.ai_skill_invoke.skltmingyun = true 
--[[sgs.ai_skill_cardask["@skltmingyun"] = function(self, data)
        local cards = sgs.QList2Table(self.player:getCards("h"))
		local judge = self.player:getTag("mingyun_judge"):toJudge()
        judge.card = cards[1]
        table.removeOne(cards, judge.card) --这一条不能少??
        id =self:getRetrialCardId(cards, judge)
		if id==-1 then
			--打出默认牌也不对。。。 有时候闪电电中不明身份的。。。不如电明反。。。
			return "$" .. judge.card:getEffectiveId()
		else
			return "$" .. id
		end
		
		--return "$" .. self:getRetrialCardId(cards, judge) or judge.card:getEffectiveId()  --tostring()
end]]
sgs.ai_skill_discard.skltmingyun = function(self)
	local cards = sgs.QList2Table(self.player:getCards("h"))
	local judge = self.player:getTag("mingyun_judge"):toJudge()
    judge.card = cards[1]
    table.removeOne(cards, judge.card) --这一条不能少??
    id =self:getRetrialCardId(cards, judge)
	local final_id
	if id==-1 then
	--打出默认牌也不对。。。 有时候闪电电中不明身份的。。。不如电明反。
		final_id=judge.card:getEffectiveId()
	else
		final_id=id
	end
	local to_discard = {}
	table.insert(to_discard, final_id)
	return to_discard
end
sgs.skltmingyun_suit_value = {
	heart = 3.9,
	club = 3.9,
	spade = 3.5
}
--相应的一些改判该如何改还没有写
--function sgs.ai_cardneed.skltmingyun(to, card, self)  


--【血裔】ai  具体策略不好想。。。。
--这仅仅是打出杀  还有回合外借刀使用杀。。。
sgs.ai_skill_invoke.skltxueyi = function(self,data)
	local pattern = self.player:getTag("skltxueyi_pattern"):toStringList()[1]
	cards =self.player:getCards("h")
	cardname="Slash"
	if pattern=="jink" then
		cardname="Jink"
	end
	for _,card in sgs.qlist(cards) do
		if card:isKindOf(cardname) then
			return false
		end
	end
	if self:isWeak(self.player) then
		return true
	else
		self:sort(self.friends_noself)
		for _,p in pairs (self.friends_noself) do
			if not self:isWeak(p) then
				return true
			end
		end
	end
	return false
end 
function sgs.ai_cardsview_valuable.skltxueyi(self, class_name, player, need_lord)
	if class_name == "Slash" and sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE
		and not player:hasFlag("Global_skltxueyiFailed") and (need_lord == false or player:hasLordSkill("skltxueyi")) then
		local current = self.room:getCurrent()
		if current:getKingdom() == "hmx" and self:getOverflow(current) > 2 and not self:hasCrossbowEffect(current) then
			self.player:setFlags("stack_overflow_jijiang")
			local isfriend = self:isFriend(current, player)
			self.player:setFlags("-stack_overflow_jijiang")
			if isfriend then return "#skltxueyi:.:" end
		end

		local cards = player:getHandcards()
		for _, card in sgs.qlist(cards) do
			if isCard("Slash", card, player) then return end
		end

		local lieges = self.room:getLieges("hmx", player)
		if lieges:isEmpty() then return end
		local has_friend = false
		for _, p in sgs.qlist(lieges) do
			self.player:setFlags("stack_overflow_jijiang")
			has_friend = self:isFriend(p, player)
			self.player:setFlags("-stack_overflow_jijiang")
			if has_friend then break end
		end
		if has_friend then return "#skltxueyi:.:" end
	end
end

--严格的话，应该改成hujiasource一类的global flag
sgs.ai_skill_cardask["@skltxueyi-slash"]=function(self,data)
	local lord=data:toPlayer()
	local slashs={}
	if not self:isFriend(lord) then return "." end
	for _,card in sgs.qlist(self.player:getCards("h")) do
		if card:isKindOf("Slash") then
			table.insert(slashs,card)
		end
	end
	if #slashs ==0 then return "." end
	if #slashs ==1 then 
		if self:isWeak(lord) then
			return "$" .. slashs[1]:getId()
		else
			return "."
		end
	end
	if #slashs >1 then return "$" .. slashs[1]:getId() end
end
sgs.ai_skill_cardask["@skltxueyi-jink"]=function(self,data)
	local lord=data:toPlayer()
	local jinks={}
	if not self:isFriend(lord) then return "." end
	if  self:hasEightDiagramEffect() then
		return self:getCardId("Jink") or "."
	end
	local num=self:getCardsNum("Jink")
	if  num==0  then return "." end
	
	if num ==1 then 
		if self:isWeak(lord) then
			return self:getCardId("Jink") or "."
			--return "$" .. jinks[1]:getId()
		else
			return "."
		end
	end
	
	if num >1 then 
		return self:getCardId("Jink") or "."
		--return "$" .. jinks[1]:getId() 
	end
end
sgs.ai_choicemade_filter.cardResponded["@skltxueyi-jink"] = function(self, player, promptlist)
	local lord=self.room:getLord()
	if not lord then return false end
	if promptlist[#promptlist] ~= "_nil_" then
		sgs.updateIntention(player, lord, -80)
	end
end
sgs.ai_choicemade_filter.cardResponded["@skltxueyi-slash"] = function(self, player, promptlist)
	local lord=self.room:getLord()
	if not lord then return false end
	if promptlist[#promptlist] ~= "_nil_" then
		sgs.updateIntention(player, lord, -80)
	end
end

--【破坏】ai  无需AI锁定技
--function SmartAI:filterEvent(event, player, data)

--【浴血】ai  pattern为slash的askForUseCard无需AI
sgs.ai_cardneed.yuxue = function(to, card, self)
	return  isCard("Slash", card, to) 
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
	if not attacker then return end
	--if self:getDamagedEffects(attacker, player) then return self:isFriend(attacker, player) end
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
	
	if self:getCardsNum("Slash") > 0 then
		hasSlash=true
	end
	if player:getHp()>2 then
		hasGoodState=true
	elseif player:getHp()==2 then
			if self:getCardsNum("Peach") > 0 then
				hasGoodState=true
			end
	end
	
	if hasGoodTarget and hasGoodState and hasSlash then
		return true
	end
	return false
end
--【盛宴】ai
sgs.ai_skill_invoke.shengyan = function(self)
        return not self:needKongcheng(self.player, true)
end

--【锁定】ai
--没考虑锁定为自己保牌的情况。。。
local suoding_skill = {}
suoding_skill.name = "suoding"
table.insert(sgs.ai_skills, suoding_skill)
function suoding_skill.getTurnUseCard(self)
        if self.player:hasUsed("#suoding") then return nil end
        return sgs.Card_Parse("#suoding:.:")
end
sgs.ai_skill_use_func["#suoding"] = function(card, use, self)
        self:sort(self.enemies, "handcard")
		if self:getOverflow() >0 then
			use.card = card
            if use.to then
               use.to:append(self.player)
               if use.to:length() >= 1 then return end
            end	
		else
			for _, p in ipairs(self.enemies) do
                if not p:isKongcheng() 
				--and not self:hasLoseHandcardEffective(p) 
				and not self:touhouHandCardsFix(p) then
						use.card = card
                        if use.to then
                                use.to:append(p)
                                if use.to:length() >= 1 then return end
                        end
                end
			end
		end
		
end
sgs.ai_skill_playerchosen.suoding = function(self, targets)
	if self:getOverflow() >0 then
			return self.player
	else
			target_table =sgs.QList2Table(targets)
		if #target_table==0 then return nil end
		self:sort(target_table, "handcard")
		for _, p in ipairs(target_table) do
			if self:isEnemy(p) 
			--and not self:hasLoseHandcardEffective(p) 
			and not self:touhouHandCardsFix(p) then --【永恒】一类
				return p
			end
		end
	end
	
	return nil
end

sgs.ai_use_value.suoding = 8
sgs.ai_use_priority.suoding =7
sgs.ai_playerchosen_intention.suoding = 20--对于永恒类仇恨和对自身使用应该不一样。。。
sgs.ai_card_intention.suoding = 20

--【回溯】ai

sgs.ai_skill_invoke.huisu = function(self)
	return self:invokeTouhouJudge()
end
sgs.ai_need_damaged.huisu = function(self, attacker, player)
	if self.player:getHp()>1 then 
		return self:needtouhouDamageJudge()
	end
	return false
end

--【博览】ai
sgs.ai_skill_invoke.bolan = function(self)
	if self.player:getPile("yao_mark"):length()>0 then
		return true
	else
		if self.player:getHandcardNum()> 5 then
			return false
		end
	end
	return true
end

--丢拍策略要重新考虑。。。要保留锦囊牌。。。
sgs.ai_skill_discard.qiyao_got = sgs.ai_skill_discard.gamerule

--【七曜】ai
--七曜留牌策略 依靠1 SmartAI:assignKeep(num, start)
--2 还是keepvalue呢？？
--七曜的viewas如何早于桃子？ 非得靠cardsview_valuable？？？
sgs.ai_view_as.qiyao = function(card, player, card_place)
	if player:getPhase()~= sgs.Player_NotActive then return false end
	if player:getMark("Global_PreventPeach")>0 then return false end
	
	local hand_trick={}
	local real_peach={}
	local others={}
	for _,c in sgs.qlist(player:getCards("h")) do
		if c:isKindOf("Peach") then
			table.insert(real_peach,c)
		
		elseif c:isKindOf("TrickCard") then
			table.insert(hand_trick,c)
		else
			table.insert(others,c)
		end
	end
	local current= player:getRoom():getCurrent()
	--self:hasWeiya(player) 拿不到self?
	if #real_peach<1 and current:hasSkill("weiya")
	and not player:hasFlag("weiya_ask") then--威压救不活
		return false
	end
	local ids=player:getPile("yao_mark")
	--其实还该考量先用手牌里的锦囊还是曜里的锦囊
	if card:isKindOf("Peach") then
		return false 
	elseif card:isKindOf("TrickCard") then 
	elseif ids:length()==0  then
		return false
	end
	if card_place ~= sgs.Player_PlaceSpecial then
		local suit = card:getSuitString()
		local number = card:getNumberString()
		local card_id = card:getEffectiveId()
		return ("peach:qiyao[%s:%s]=%d"):format(suit, number, card_id)	
	end
end
sgs.qiyao_keep_value = {
	Peach = 10, 
	TrickCard = 8
}

--（旧）【七曜】ai

--[[nljqiyaoable = function(nlj, suit)
        if suit == sgs.Card_SuitToBeDecided then return nil end
        local pile = nlj:getPile("yao_mark")
        local result = {}
        result.enabled = sgs.IntList()
        result.disabled = sgs.IntList()
        if suit == sgs.Card_NoSuit then
                if pile:isEmpty() then return nil end
                result.enabled = pile
                return result
        end
        local isred = (suit == sgs.Card_NoSuitRed)
        for _, id in sgs.qlist(pile) do
                if sgs.Sanguosha:getCard(id):isRed() ~= isred then
                        result.enabled:append(id)
                else
                        result.disabled:append(id)
                end
        end
        if result.enabled:isEmpty() then return nil end
        return result
end
sgs.ai_view_as.nljqiyao = function(card, player, card_place)
        local can_use = false
        if (card_place == sgs.Player_PlaceSpecial) and player:getPile("yao_mark"):contains(card:getEffectiveId()) then
                can_use = true
        elseif card_place == sgs.Player_PlaceHand and card:isKindOf("TrickCard") then
                can_use = true
        end
        if not can_use then return end
        local suit = player:property("nljqiyao"):toInt()
        if card_place == sgs.Player_PlaceSpecial then
                local result = nljqiyaoable(player, suit)
                if result == nil then return end
                if result.enabled:contains(card:getEffectiveId()) then
                        return ("jink:nljqiyao[%s:%s]=%d"):format(card:getSuit(), card:getNumber(), card:getEffectiveId())
                end
        else
                local pattern2 = "."
                if suit == sgs.Card_NoSuitRed then pattern2 = "black"
                elseif suit == sgs.Card_NoSuitBlack then pattern2 = "red"
                elseif suit == sgs.Card_NoSuit then pattern2 = "."
                end
                local pattern = "TrickCard|" .. pattern2
                if sgs.Sanguosha:matchExpPattern(pattern, player, card) then
                        return ("jink:nljqiyao[%s:%s]=%d"):format(card:getSuitString(), card:getNumberString(), card:getEffectiveId())
                end
        end
end
]]

--【斗魂】ai
sgs.ai_skill_cardask["douhun-slash"]  = function(self, data, pattern, target)
	local effect = data:toSlashEffect()
	local p
	if effect.from:objectName()==self.player:objectName() then
		p=effect.to
	else
		p=effect.from
	end
	if self:isFriend(p) then return "." end
	if self.player:hasSkill("hmlzhanyi") then
		return self:getCardId("Slash")
	elseif p:hasSkill("hmlzhanyi") then
		local rate=1
		if self.player:hasSkill("weiya") and self:hasWeiya(p) then
			rate=2
		end
		if self:getCardsNum("Slash") > p:getHandcardNum()*rate then
			return self:getCardId("Slash")
		end
	end
	return "."
end
--function(self, data,pattern,target)
sgs.ai_slash_prohibit.douhun = function(self, from, to, card)
	
	if self.player:objectName()~=from:objectName() then return false end
	if to:hasSkills("douhun+hmlzhanyi") then
		local rate=1
		if from:hasSkill("weiya") and self:hasWeiya() then
			rate=2
		end
		if self:getCardsNum("Slash") <= to:getHandcardNum()*rate then
			return true
		end
	end
	return false
end	
--【战意】ai
--[[相关ai
sgs.ai_skill_cardask["duel-slash"]
]]
--[[sgs.ai_view_as.hmlzhanyi = function(card, player, card_place)
        local suit = card:getSuitString()
        local number = card:getNumberString()
        local card_id = card:getEffectiveId()
        if card_place == sgs.Player_PlaceHand and not card:isKindOf("Peach") and not card:hasFlag("using") then
                return ("slash:hmlzhanyi[%s:%s]=%d"):format(suit, number, card_id)
        end
end]]
function sgs.ai_cardsview_valuable.hmlzhanyi(self, class_name, player)
	if class_name == "Slash" then
		if (sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE) then
			return nil
		end
		if self.player:isKongcheng() then return nil end
		local card
		
		local peaches={}
		local blacks={}
		local reds={}
		for _,c in sgs.qlist(self.player:getCards("h")) do
			if c:isKindOf("Peach") then
				table.insert(peaches,c)
			elseif c:isRed() then
				table.insert(reds,c)
			elseif c:isBlack() then
				table.insert(blacks,c)
			end
		end
		if #reds>0 then
			card=reds[1]
		elseif #blacks>0 then
			card=blacks[1]
		elseif #peaches>0 then
			card=peaches[1]
		end
		if not card then return nil end
		local suit = card:getSuitString()
        local number = card:getNumberString()
        local card_id = card:getEffectiveId()
		return ("slash:hmlzhanyi[%s:%s]=%d"):format(suit, number, card_id)
	end
end
--【冻结】ai 
--1修改函数isPriorFriendOfSlash  主动杀队友
--2function SmartAI:slashProhibit(card, enemy, from)
--3 sgs.ai_card_intention.Slash 杀翻面队友仇恨为0
sgs.ai_skill_invoke.dongjie = function(self, data)
        local damage =self.player:getTag("dongjie_damage"):toDamage()
        local to =data:toPlayer()
		if damage.damage > 1 and self:isEnemy(to) then return false end 
        return self:isFriend(to) ~= to:faceUp()
end
sgs.ai_choicemade_filter.skillInvoke.dongjie = function(self, player, promptlist)
	local to=player:getTag("dongjie_damage"):toDamage().to
	if to then 
		if to:faceUp() then
			if promptlist[#promptlist] == "yes" then
				sgs.updateIntention(player, to, 60)
			end
		else
			if promptlist[#promptlist] == "yes" then
				sgs.updateIntention(player, to, -60)
			else
				sgs.updateIntention(player, to, 60)
			end
		end
	end
end
sgs.dongjie_keep_value = {
	Peach 			= 5.5,
	Slash 			= 6.4
}
--【冰魄】ai 无需AI锁定技
--function SmartAI:needRetrial(judge)
--sgs.ai_skill_cardask["slash-jink"]
--function SmartAI:damageIsEffective(to, nature, from)
sgs.ai_slash_prohibit.bingpo = function(self, from, to, card)
	if card:isKindOf("FireSlash") or from:hasSkill("ldlkhere") then
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
--【笨蛋】ai 无需AI锁定技
--function SmartAI:getRetrialCardId(cards, judge, self_card)

--【真夜】ai
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

--【暗域】ai 
--sgs.ai_card_intention.Slash = function(self, card, from, tos)
--function SmartAI:isPriorFriendOfSlash
sgs.ai_skill_invoke.anyu = function(self,data)
	local use=self.room:getTag("anyu_use"):toCardUse()
	weiya_current=self.room:getCurrent()
	--self.player:hasFlag("weiya_target") 
	
	if use.card:isKindOf("Slash") then
		if self:isEnemy(use.from)  and use.from:hasWeapon("Axe") and use.from:getCards("he"):length()>=2 then
			return false
		end
		if weiya_current:isAlive() and  weiya_current:objectName() ~=self.player:objectName()
		and weiya_current:hasSkill("weiya") 
		and not self.player:hasFlag("weiya_ask")then
			return false
		end
		if self:getCardsNum("Jink") > 0 then
			return true
		end
		if use.from:hasSkill("sidie") and self:isFriend(use.from)  then
			return true
		end
	end
	if use.card:isKindOf("Duel") then
		if self:isEnemy(use.from) then
			if self:getCardsNum("Slash") > 2 and not self:isWeak(self.player) then
				return true
			end
		else
			return true
		end
	end
	return false 
end

--【契约】ai 技能非常复杂 没想好细节  ai可能会变成猪队友
--契约帮助队友没想好
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
	--没有考虑没有魔血 只有契约时。。。
	local current=self.room:getCurrent()
	--先得到敌人数目
	if self:isEnemy(current) then
		if self.player:getMaxHp()==1 then return false end
		if self.player:getMaxHp()<3 or self.player:getHp()<3 then return true end 
		--等敌人全出来？
		if #self.enemies<3 then return false end
		--计算起始第一次契约
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
		return "maxhp_moxue"
	else
		return "hp_moxue"
	end
end
sgs.ai_choicemade_filter.skillInvoke.qiyue = function(self, player, promptlist)
	local to=self.room:getCurrent()
	if promptlist[#promptlist] == "yes" then
		if self:willSkipPlayPhase(to) and self:getOverflow(to,to:getMaxCards())>1 then
			sgs.updateIntention(player, to, -20)
		else
			sgs.updateIntention(player, to, 60)
		end
	end
end
--【魔血】ai 无需AI锁定技

--【具现】ai
--function SmartAI:isWeak
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

--【半月】ai
--ai本身可以执行 但是此策略不赞同  有时还需要主动卖血配合【具现】求爆发。

local banyue_skill = {}
banyue_skill.name = "banyue"
table.insert(sgs.ai_skills, banyue_skill)
function banyue_skill.getTurnUseCard(self)
        if self.player:getHp() <= 2 and not self.player:hasSkill("juxian") then return nil end
        --还是让大妖精可以主动卖血 具现吧
		if self.player:getHp() == 2 and self.player:hasSkill("juxian") then return nil end
		if self.player:hasUsed("#banyue") then return nil end
        return sgs.Card_Parse("#banyue:.:")
end
sgs.ai_skill_use_func["#banyue"] = function(card, use, self)
        self:sort(self.friends_noself)
        if #self.friends_noself == 1 then return end
        for _, p in ipairs(self.friends) do
                use.card = card
                if use.to then
                        use.to:append(p)
                        if use.to:length() >= 3 then return end
                end
        end
end

sgs.ai_use_value.banyue = 3
sgs.ai_use_priority.banyue =6
sgs.ai_card_intention.banyue = -40


--嘲讽值设定
sgs.ai_chaofeng.hmx001 = 2
sgs.ai_chaofeng.hmx002 = -2
sgs.ai_chaofeng.hmx003 = -2
sgs.ai_chaofeng.hmx004 = 3
sgs.ai_chaofeng.hmx005 = -1
sgs.ai_chaofeng.hmx006 = 0
sgs.ai_chaofeng.hmx007 = 0
sgs.ai_chaofeng.hmx008 = 4
sgs.ai_chaofeng.hmx009 = 3

