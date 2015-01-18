--【渴血】ai
--function SmartAI:isWeak
--SmartAI:ableToSave-- 【远吠】的问题
function sgs.ai_cardsview_valuable.skltkexuepeach(self, class_name, player)
	if class_name == "Peach" and player:getHp()>1 then
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying or not dying:hasSkill("skltkexue") or self:isEnemy(dying, player) or dying:objectName() == player:objectName() then return nil end
		--if self:isFriend(dying, player) then return "#skltkexuepeach:.:" end
		if self:isFriend(dying, player) then return "@skltkexueCard=." end
		return nil
	end
end
sgs.ai_card_intention.skltkexueCard = sgs.ai_card_intention.Peach

function SmartAI:invokeTouhouJudge(player)
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
	return value>=0
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
	if self:hasEightDiagramEffect(enemy) and self:invokeTouhouJudge(enemy) then
		if not self:touhouIgnoreArmor(card,from,enemy) 
		or not (from:hasSkill("guaili") and from:getHandcardNum()>=3 and from:getPhase()== sgs.Player_Play) then
			return true
		end
	end
	return false
end
--【命运】ai 如果对此判定的好坏无法判断 此AI可能会导致闪退
--sgs.ai_skill_invoke.EightDiagram 
--sgs.ai_armor_value.EightDiagram 
--SmartAI:getFinalRetrial
--SmartAI:canRetrial
sgs.ai_skill_invoke.mingyun = true 
sgs.ai_skill_askforag.mingyun = function(self, card_ids)
	--card_ids为table
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
	
	--local result1 = self:needRetrial(judge)  很难用 效果不好。。。
	--ex_id==-1 说明预设的判定不符合雷米的利益
	if ex_id1==ex_id2 or (ex_id1~=-1 and  ex_id2~=-1) then
		--哪一个都无所谓
		self:sortByKeepValue(mingyun,true)
		return mingyun[1]:getId()
	elseif ex_id1==-1 then
		return card_ids[1]
	elseif ex_id2==-1 then
		return card_ids[2]
	end
	return card_ids[1]
end

--【血裔】ai 
sgs.ai_skill_playerchosen.xueyi = function(self, targets)
	target_table =sgs.QList2Table(targets)
	if #target_table==0 then return false end
	for _,target in pairs(target_table) do	
		if  self:isFriend(target) then
			return target
		end
	end
	return nil
end
sgs.ai_playerchosen_intention.xueyi = -80

--【破坏】ai
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
--【浴血】ai  pattern为slash的askForUseCard无需AI
--[[sgs.ai_cardneed.yuxue = function(to, card, self)
	return  isCard("Slash", card, to) 
end]]
sgs.yuxue_keep_value = {
	Peach 			= 5.5,
	Analeptic 		= 5.5,
	Jink 			= 4.2,
	FireSlash 		= 5.6,
	Slash 			= 5.4,
	ThunderSlash 	= 5.5
}
sgs.ai_need_damaged.yuxue = function(self, attacker, player)
	--if not attacker then return end
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
--【盛宴】ai
sgs.ai_skill_invoke.shengyan = function(self)
        return not self:needKongcheng(self.player, true)
end

--【锁定】ai
local suoding_skill = {}
suoding_skill.name = "suoding"
table.insert(sgs.ai_skills, suoding_skill)
function suoding_skill.getTurnUseCard(self)
        if self.player:hasUsed("suodingCard") then return nil end
        --return sgs.Card_Parse("#suoding:.:")
		return sgs.Card_Parse("@suodingCard=.")
end
sgs.ai_skill_use_func.suodingCard = function(card, use, self)
--sgs.ai_skill_use_func["#suoding"] = function(card, use, self)
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

sgs.ai_use_value.suodingCard = 8
sgs.ai_use_priority.suodingCard =7
sgs.ai_card_intention.suodingCard = 20

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
function sgs.ai_cardsview_valuable.qiyao(self, class_name, player)
	if class_name == "Peach" then
		if (sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
			return nil
		end
		if self.player:getPhase()~= sgs.Player_NotActive then return nil end
		if self.player:getMark("Global_PreventPeach")>0 then return nil end
		
		
		local hand_trick={}
		local real_peach={}
		local others={}
		local cards = self.player:getHandcards()
		cards=self:touhouAppendExpandPileToList(self.player,cards)
	
		for _,c in sgs.qlist(cards) do
			if c:isKindOf("Peach") then
				table.insert(real_peach,c)
			elseif c:isNDTrick()then
				table.insert(hand_trick,c)
			else
				table.insert(others,c)
			end
		end
		
		if #real_peach<1 and  self:hasWeiya()  then
			return nil
		end
	
		local ids=self.player:getPile("yao_mark")
		local card
		if #hand_trick>0 then
			self:sortByKeepValue(hand_trick)
			card=hand_trick[1]
		elseif #others>0 and ids:length()>0 then
			self:sortByKeepValue(others)
			card=others[1]
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
	if self.player:hasSkill("zhanyi") then
		return self:getCardId("Slash")
	elseif p:hasSkill("zhanyi") then
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
sgs.ai_cardneed.douhun = function(to, card, self)
	return getCardsNum("Slash", to, self.player) <2
	 and card:isKindOf("Slash")
end
--function(self, data,pattern,target)
sgs.ai_slash_prohibit.douhun = function(self, from, to, card)
	
	if self.player:objectName()~=from:objectName() then return false end
	if to:hasSkills("douhun+zhanyi") then
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
--[[sgs.ai_view_as.zhanyi = function(card, player, card_place)
        local suit = card:getSuitString()
        local number = card:getNumberString()
        local card_id = card:getEffectiveId()
        if card_place == sgs.Player_PlaceHand and not card:isKindOf("Peach") and not card:hasFlag("using") then
                return ("slash:zhanyi[%s:%s]=%d"):format(suit, number, card_id)
        end
end]]
function sgs.ai_cardsview_valuable.zhanyi(self, class_name, player)
	if class_name == "Slash" then
		if (sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE) then
			return nil
		end
		local card
		local peaches={}
		local blacks={}
		local reds={}
		local cards = self.player:getHandcards()
		cards=self:touhouAppendExpandPileToList(self.player,cards)
		for _,c in sgs.qlist(cards) do
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
		return ("slash:zhanyi[%s:%s]=%d"):format(suit, number, card_id)
	end
end
sgs.ai_cardneed.zhanyi = function(to, card, self)
	return  card:isRed()
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
sgs.ai_cardneed.dongjie = function(to, card, self)
	return getCardsNum("Slash", to, self.player) <1
	 and card:isKindOf("Slash")
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
sgs.ai_playerchosen_intention.zhenye =function(self, from, to)
	local intention = 0
	if to:faceUp() then
		intention = 80
	else
		intention = -30
	end
	sgs.updateIntention(from, to, intention)
end

--【暗域】ai 
--sgs.ai_card_intention.Slash = function(self, card, from, tos)
--function SmartAI:isPriorFriendOfSlash
--[[sgs.ai_skill_invoke.anyu = function(self,data)
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
]]
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
		if self.player:hasUsed("banyueCard") then return nil end
        --return sgs.Card_Parse("#banyue:.:")
		return sgs.Card_Parse("@banyueCard=.")
end
sgs.ai_skill_use_func.banyueCard = function(card, use, self)
--sgs.ai_skill_use_func["#banyue"] = function(card, use, self)
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

sgs.ai_use_value.banyueCard = 3
sgs.ai_use_priority.banyueCard =6
sgs.ai_card_intention.banyueCard = -40


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

