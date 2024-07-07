--年代记SP妹红
--[死斗]
sgs.ai_skill_playerchosen.sidou = function(self, targets)
	if self.player:getHp()== 1 then
		return nil
	end

	local fakeDamage=sgs.DamageStruct()
	fakeDamage.nature= sgs.DamageStruct_Fire
	fakeDamage.damage=1
	fakeDamage.from = self.player
	fakeDamage.to = self.player
	local final_damage=self:touhouDamage(fakeDamage,self.player, self.player)
        local friend_damage = final_damage.damage
        local enemy_damage = 0
        if self.player:isChained() then
        	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
				if p:isChained() then
					fakeDamage.to = p
					local tmp_damage=self:touhouDamage(fakeDamage,self.player, self.player)
					if self:isEnemy(p) then
						enemy_damage = enemy_damage  + tmp_damage.damage
					elseif self:isFriend(p) then
						friend_damage = friend_damage  + tmp_damage.damage
				end
            end
	    end
	end

	if friend_damage > enemy_damage and friend_damage >=2 then
		return nil
    end

	local wizard=self:invokeTouhouJudge()
	for _,p in sgs.qlist(targets) do
		if not wizard and p:containsTrick("lightning") then
			return p
		end
		if self:isFriend(p) then
			if p:containsTrick("indulgence") or p:containsTrick("supply_shortage") then
				return p
			end
		end
	end
	for _,p in sgs.qlist(targets) do
		if self:isEnemy(p) and (p:getEquip(1) or p:getEquip(2) or p:getEquip(3) or p:getEquip(4)) then
			return p
		end
	end
	return nil
end
sgs.ai_skill_cardchosen.sidou = function(self, who, flags)
	local wizard=self:invokeTouhouJudge()
	if self:isFriend(who) then
		cards=who:getCards("j")
		cards = sgs.QList2Table(cards)
		if #cards>0 then
			return cards[1]
		end
	end
	if self:isEnemy(who) then
		cards=who:getCards("e")
		cards = sgs.QList2Table(cards)
		ecards={}
		for _,c in pairs (cards) do
			if not c:isKindOf("Weapon") then
				table.insert(ecards,c)
			end
		end
		if #ecards>0 then
			return ecards[1]
		end
	end
end
sgs.ai_choicemade_filter.cardChosen.sidou = sgs.ai_choicemade_filter.cardChosen.dismantlement

--[物语]
function SmartAI:findRealKiller(victim,damage)
	local mouko = self.room:getLord()
	if mouko and mouko:isAlive() and mouko:hasLordSkill("tymhwuyu") then
		return mouko
	end
	if damage then
		return damage.from
	else
		return nil
	end
end
sgs.ai_skillProperty.tymhwuyu = function(self)
	return "noKingdom"
end

--年代记SP辉夜
--[幻月]
sgs.ai_skill_invoke.huanyue = true
sgs.ai_skill_cardask["@huanyue-keep"] = function(self, data)
	return "$" .. self.player:getPile("huanyue_pile"):first()
end
sgs.ai_skill_cardask["@huanyue"] = function(self, data)
	local damage = data:toDamage()
	if self:isEnemy(damage.to) then
		return "$" .. self.player:getPile("huanyue_pile"):first()
	end
	return "."
end
sgs.ai_choicemade_filter.cardResponded["@huanyue"] = function(self, player, args, data)
	if args[#args] ~= "_nil_" then
		local target =data:toDamage().to
		if not target then return end
		sgs.updateIntention(player, target, 80)
	end
end
--[网购]
sgs.ai_skill_invoke.wanggou = true

--年代记SP紫
--[援护]
sgs.ai_skill_invoke.yuanhu = function(self,data)
	local source = self.player:getTag("yuanhu"):toPlayer()
	return source and self:isFriend(source)
end
sgs.ai_choicemade_filter.skillInvoke.yuanhu = function(self, player, args)
	local source = player:getTag("yuanhu"):toPlayer()
	if source and args[#args] == "yes" then
		sgs.updateIntention(player, source, -30)
	end
end

sgs.ai_skill_cardask["@yuanhu"] = function(self, data, pattern, target)
	local yukari = self.player:getTag("yuanhu_drawers"):toPlayer()
	if self:isFriend(yukari) then
		local handcards = sgs.QList2Table(self.player:getHandcards())
		if #handcards==0 then return "." end
		self:sortByUseValue(handcards)
		return "$" .. handcards[1]:getId()
	end
	return "."
end

sgs.ai_choicemade_filter.cardResponded["@yuanhu"] = function(self, player, args)
	local source = player:getTag("yuanhu_drawers"):toPlayer()
	if source and args[#args] == "yes" then
		sgs.updateIntention(player, source, -30)
	end
end

sgs.ai_skill_discard.yuanhu = function(self,discard_num, min_num)
	local target = self.player:getTag("yuanhu_target"):toPlayer()
	local to_discard = {}
	if not target or not self:isFriend(target) then return to_discard end

	local toGive, allcards = {}, {}
	local keep
	for _, card in sgs.qlist(self.player:getCards("hs")) do
		if not keep and (isCard("Jink", card, self.player) or isCard("Analeptic", card, self.player)) then
			keep = true
		else
			table.insert(toGive, card)
		end
		table.insert(allcards, card)
	end
	local cards = #toGive > 0 and toGive or allcards
	self:sortByKeepValue(cards, true)
	table.insert(to_discard, cards[1]:getId())
	--默认分配一张 其他情况暂时不计
	return to_discard
end
sgs.ai_choicemade_filter.cardExchange.yuanhu = function(self, player, args)
	local target = player:getTag("yuanhu_target"):toPlayer()
	if target and args[#args] ~= "_nil_" then
		sgs.updateIntention(player, target, -30)
	end
end

--年代记SP妖梦
--[魂魄]
local hunpo_skill = {}
hunpo_skill.name = "hunpo"
table.insert(sgs.ai_skills, hunpo_skill)
function hunpo_skill.getTurnUseCard(self)
	if self.player:getMaxHp()>= 4 then return nil end
	if self.player:isNude() then return nil end
	local cards={}
	if self:getOverflow() >0 then
		cards= sgs.QList2Table(self.player:getHandcards())
	else
		cards= sgs.QList2Table(self.player:getCards("hes"))
	end
	self:sortByKeepValue(cards)
	if #cards>0 then
		return sgs.Card_Parse("@HunpoCard="..cards[1]:getId())
	end
	return nil
end
sgs.ai_skill_use_func.HunpoCard = function(card, use, self)
	use.card=card
end

--[反击]
sgs.ai_skill_invoke.fanji = function(self,data)
	local target=self.player:getTag("fanji_damage"):toDamage().from
	local buff =self.player:getTag("fanji_damage"):toDamage().to
	if target and buff then
		if self:isEnemy(target) then
			if buff:objectName()==self.player:objectName() then
				return true
			else
				if self.player:getMaxHp()>=self.player:getHp() then
					return true
				end
			end
		end
	end
	return false
end
sgs.ai_skill_choice.fanji= function(self, choices, data)
	if self.player:isWounded() then
		return "maxhp"
	end
	return "hp"
end

--年代记SP玛艾露贝莉
--[裂隙]
sgs.ai_skill_use["@@liexi"] = function(self, prompt)
	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	local card=sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
	card:setSkillName("liexi")
	local target

	self:useBasicCard(card, dummy_use)
	if not dummy_use.card then return false end
	if dummy_use.to:isEmpty() then
		return "."
	else
		local target_objectname = {}

		for _, p in sgs.qlist(dummy_use.to) do
			if self:isEnemy(p) then
				table.insert(target_objectname, p:objectName())
				target=p
				break
			end
		end

		if #target_objectname>0 then
			return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
		end
	end
	return "."
end

sgs.ai_skill_playerchosen.liexi = function(self, targets)
	local use = self.room:getTag("liexi_extra"):toCardUse()
	for _,p in sgs.qlist(targets) do
		if self:isFriend(p) then
			if not self:slashIsEffective(use.card, p, use.from) then
			return true
			end
			if getCardsNum("Jink", p, self.player) > 0  then return p end
		end
	end
	return targets:first()
end
sgs.ai_skill_invoke.liexi_extra = function(self,data)
	local use = self.room:getTag("liexi_extra"):toCardUse()
	if (use.from and self:isFriend(use.from)) then
		if not self:slashIsEffective(use.card, self.player, use.from) then
			return true
		end
		if  self:getCardsNum("Jink") > 0  then return true end
	end
	return false
end
--[梦违]
sgs.ai_skill_playerchosen.mengwei = function(self, targets)
	local use = self.room:getTag("mengwei_extra"):toCardUse()
	for _,p in sgs.qlist(targets) do
		if self:playerGetRound(p) <= self:playerGetRound(self.player) and self:isFriend(p) then
			if not self:slashIsEffective(use.card, p, use.from) then
				return p
			end
			if  self:getCardsNum("Jink") > 0  then return p end
		end
	end
	return targets:first()
end
sgs.ai_skill_invoke.mengwei_extra = function(self,data)
	local use = self.room:getTag("mengwei_extra"):toCardUse()
	local source
	for _,p in sgs.qlist(use.to) do
		if p:hasSkill("mengwei") then
			source = p
			break
		end
	end
	if (source and self:isFriend(source)) then
		if not self:slashIsEffective(use.card, self.player, use.from) then
			return true
		end
		if  self:getCardsNum("Jink") > 0  then return true end
		if self:playerGetRound(self.player) >= self:playerGetRound(source) then return true end
	end
	return false
end

sgs.ai_skill_invoke.zaiwu = function(self,data)
	local target = self.player:getTag("zaiwu"):toPlayer()
	if not target then return false end
	local plus = self.player:getHp() > target:getHp()
	if plus then
		return self:isFriend(target)
	end
	return self:isEnemy(target)
end
sgs.ai_choicemade_filter.skillInvoke.zaiwu = function(self, player, args)
	local target = player:getTag("zaiwu"):toPlayer()
	if target  then
		local plus = player:getHp() > target:getHp()
		if plus and args[#args] == "yes" then
			sgs.updateIntention(player, target, -30)
		elseif not plus and args[#args] == "yes" then
			sgs.updateIntention(player, target, 50)
		end
	end
end
--[[
sgs.ai_skill_invoke.mengwei = function(self,data)
	local target = data:toPlayer()
	if not target then return false end
	--太复杂 先默认
	return self:getOverflow() <= 0 and self:isFriend(target)
end
sgs.ai_choicemade_filter.skillInvoke.mengwei = function(self, player, args)
	local target = player:getTag("mengwei"):toPlayer()
	if target  and args[#args] == "yes" then
		sgs.updateIntention(player, target, -30)
	end
end
]]

--年代记SP莲子
-- 考虑神灵梦 连着为啥要亮将解开
sgs.ai_skill_invoke.liangzi = function(self)
	return not self.player:isChained()
end

--年代记SP早苗
--[修补]
sgs.ai_skill_cardask["@xiubu-self"] = function(self, data)
	if sgs.ai_skill_invoke.xiubu(self, data) then
		local dis = self:askForDiscard("Dummy", 1, 1, false, false)
		return tostring(dis[1])
	end
end

sgs.ai_skill_invoke.xiubu =function(self,data)
	local target = data:toPlayer()
	if not target then return false end
	if self:isEnemy(target) then
		if target:getHandcardNum() <= 1 or target:getHp() ==1 then
			return true
		end
	elseif self:isFriend(target) then
		if target:getHandcardNum() >3 and target:getHp() > 1 then
			return true
		end
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.xiubu = function(self, player, args)
	local target = self.room:getCurrent()
	if target and target:getHandcardNum() <= 1 and args[#args] == "yes" then
		sgs.updateIntention(player, target, 50)
	end
end

sgs.ai_cardneed.xiubu = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  (not to:getWeapon() and  getCardsNum("Weapon",to,self.player)<1 and card:isKindOf("Weapon"))
		or (not to:getOffensiveHorse() and  getCardsNum("OffensiveHorse",to,self.player)<1 and card:isKindOf("OffensiveHorse"))
	end
end

--年代记SP文
--[机能]
sgs.ai_skill_invoke.jineng =function(self,data)
	--if not self:invokeTouhouJudge() then return false end
	return true
end
local jineng_skill = {}
jineng_skill.name = "jineng"
table.insert(sgs.ai_skills, jineng_skill)
jineng_skill.getTurnUseCard = function(self)

	local current = self.room:getCurrent()
	if not current or current:isDead() or current:getPhase() == sgs.Player_NotActive then return end

	local ids = self.player:getPile("jinengPile")
	local spade, heart, club, diamond -- = {}, {}, {}, {}
	local jinengCards = {}
	for _, id in sgs.qlist(ids) do
		local forbid
		local c = sgs.Sanguosha:getCard(id)
		local suit = c:getSuit()
		if  suit == sgs.Card_Spade then
			forbid = sgs.cloneCard("slash")
		elseif suit == sgs.Card_Club then
			forbid = sgs.cloneCard("known_both")
		elseif suit == sgs.Card_Diamond then
			forbid = sgs.cloneCard("analeptic")
		end

		if forbid then
			forbid:addSubcard(c)
			forbid:setSkillName("jineng")
			if not self.player:isLocked(forbid)  then
				table.insert(jinengCards,forbid)
			end
		end
	end

	self:sortByUseValue(jinengCards, false)
	for _,jinengCard in pairs (jinengCards) do
		local dummyuse = { isDummy = true }
		if jinengCard:isKindOf("BasicCard") then
			self:useBasicCard(jinengCard, dummyuse)
		else
			self:useTrickCard(jinengCard, dummyuse)
		end

		if dummyuse.card then
			local suit = jinengCard:getSuitString()
			local number = jinengCard:getNumberString()
			local card_id = jinengCard:getEffectiveId()
			local jineng_str = (jinengCard:objectName() .. ":jineng[%s:%s]=%d"):format(suit, number, card_id)
			local jineng_card = sgs.Card_Parse(jineng_str)

			assert(jineng_card)
			if (jinengCard:isKindOf("KnownBoth")) then--known_both
				jineng_card:setCanRecast(false)
			end
			return jineng_card
		end
	end

	return nil
end

-- 这是啥？
sgs.ai_skill_use_func.HuaxiangCard=function(card,use,self)
	local userstring=card:toString()
	userstring=(userstring:split(":"))[3]
	local huaxiangcard=sgs.cloneCard(userstring)
	huaxiangcard:setSkillName("huaxiang")
	self:useBasicCard(huaxiangcard, use)
	if not use.card then return end
	use.card=card
end

function sgs.ai_cardsview_valuable.jineng(self, class_name, player)
	if (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_UNKNOWN) then
		return nil
	end

	local ids = self.player:getPile("jinengPile")
	local spade, heart, club, diamond -- = {}, {}, {}, {}

	for _, id in sgs.qlist(ids) do
		local c = sgs.Sanguosha:getCard(id)
		local suit = c:getSuit()
		if  suit == sgs.Card_Spade then
			spade =  c
		elseif suit == sgs.Card_Heart then
			heart = c
		elseif suit == sgs.Card_Diamond then
			diamond = c
		end
	end

	local cardname, card
	if self:touhouClassMatch(class_name, "Analeptic") then
		card = diamond
		cardname = "analeptic"
	elseif self:touhouClassMatch(class_name, "Jink") then
		card = heart
		cardname = "jink"
	--elseif self:touhouClassMatch(class_name, "KnownBoth")  then
	elseif self:touhouClassMatch(class_name, "Slash") then
		card = spade
		cardname = "slash"
	end

	if not card then return nil end

	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	return (cardname .. ":jineng[%s:%s]=%d"):format(suit, number, card_id)

end
--[快报]
sgs.ai_skill_invoke.kuaibao =function(self,data)
	local current = self.room:getCurrent()
	if self:isEnemy(current) and not self:needToLoseHp(current, self.player, false, false) then
		return true
	end
	return false
end

--年代记SP天子
--[忧乐]
-- 忧乐："一名角色的非额外回合结束时，若你于此回合内造成或受到过伤害，你可以弃置一名此回合内未造成过伤害的角色有牌区域各一张牌，令其获得一个额外的回合，然后若此次你仅弃置了其他角色一张牌，你失去1点体力。"
--忧乐：选择发动技能的目标 @todo:sort targets
sgs.ai_skill_playerchosen.youle = function(self, targets)
	for _,t in sgs.qlist(targets) do
		if self:isFriend(t) then
			local num = 0
			if t:getHandcardNum() > 0 then num = num + 1 end
			if not t:getEquips():isEmpty() then num = num + 1 end
			if t:containsTrick("supply_shortage") or t:containsTrick("indulgence") then
				if (not self:isWeak(self.player)) or num >= 1 then return t end
			end
		end
	end
	for _,t in sgs.qlist(targets) do
		if self:isEnemy(t) then
			local num = 0
			if t:getHandcardNum() > 0 then num = num + 1 end
			if not t:getEquips():isEmpty() then num = num + 1 end
			if t:containsTrick("spring_breath") or t:containsTrick("saving_energy") then
				if num >= 1 then return t end
			elseif t:getJudgingArea():isEmpty() then
				if num > 1 then return t end
			end
		end
	end
	return nil
end

-- 忧乐：选择牌
-- 直接用默认AI算了，默认AI反正也会拆乐兵电

--年代记SP永琳
local yaoliCardType = function(originalCard)
	card = originalCard
	if type(originalCard) == "number" then card = sgs.Sanguosha:getCard(originalCard) end
	-- 1：基本 2：非延时锦囊 3：延时锦囊或装备
	if card:getTypeId() == sgs.Card_TypeBasic then return 1 end
	if card:isNDTrick() then return 2 end
	if card:getTypeId() == sgs.Card_TypeEquip then return 3 end
	if card:getTypeId() == sgs.Card_TypeTrick and not card:isNDTrick() then return 3 end
	-- nil: 未知（？？？？）
end
sgs.ai_event_callback[sgs.EventPhaseStart].yaoli = function(self, player)
	if player:getPhase() == sgs.Player_Play then
		sgs.ai_use_priority.YaoliCard = 6
	end
end
local yaoli_skill, yaoli_attach_skill = {}, {}
yaoli_skill.name = "yaoli"
yaoli_attach_skill.name = "yaoli_attach"
table.insert(sgs.ai_skills, yaoli_skill)
table.insert(sgs.ai_skills, yaoli_attach_skill)
yaoli_skill.getTurnUseCard = function(self)
	local discard
	if self.player:isWounded() and self.player:getArmor() and self.player:getArmor():isKindOf("SliverLion") then
		discard = self.player:getArmor()
	end
	if not discard then
		local cards = self.player:getHandcards()
		local cards_without_peach = {}
		local cards_sort = {{}, {}, {}}
		for _, card in sgs.qlist(cards) do
			if not card:isKindOf("Peach") then
				local cardType = yaoliCardType(card)
				if cardType then table.insert(cards_sort[cardType], card) end
				table.insert(cards_without_peach, card)
			end
		end

		-- 延时锦囊/装备太厉害了，拆装备/判定比较猛，但是延时锦囊普遍usevalue比较高，而装备的延时收益也要考虑，权衡一下的话可能不是好方案
		-- 基本牌比较多，容易触发，但是也是效果最不靠谱的（桃除外）
		-- 普通锦囊如果priority低的话，可能不如药理扔了好
		-- 如果简单考虑的话，普通锦囊 / 基本 / 装备吧（懒）
		local priority = {2, 1, 3}
		for _, prio in ipairs(priority) do
			local collection = cards_sort[prio]
			if #collection > 1 then
				self:sortByUseValue(collection, true)
				discard = collection[1]
				break
			end
		end

		-- 因为ai使用YaoliCard的目标一定是队友（ai_skill_use_func中有判断），为了能让队友制衡一张，扔牌肯定不亏
		if not discard then
			-- 最后再扔
			sgs.ai_use_priority.YaoliCard = 0
			self:sortByKeepValue(cards_without_peach, true)
			discard = cards_without_peach[1]
		end
	end
	return discard and sgs.Card_Parse("@YaoliCard=" .. tostring(discard:getEffectiveId()))
end
yaoli_attach_skill.getTurnUseCard = yaoli_skill.getTurnUseCard
sgs.ai_skill_use_func.YaoliCard = function(card, use, self)
	local eirin
	for _, p in sgs.qlist(self.room:getAllPlayers()) do
		if (p:objectName() == self.player:objectName() or self:isFriend(p)) and p:hasSkill("yaoli") and not p:hasFlag("yaoliselected") then
			if eirin then
				-- 目前状态下不可能
				return
			end
			eirin = p
		end
	end

	if not eirin then return end
	local effects = (self.player:getMark("yaolieffect1") > 0 and not self.player:hasFlag("yaolieffect1")) or
					(self.player:getMark("yaolieffect2") > 0 and not self.player:hasFlag("yaolieffect2")) or
					(self.player:getMark("yaolieffect3") > 0 and not self.player:hasFlag("yaolieffect3"))

	if effects then
		-- 有没用完的药理，先用完了再发动下一个
		return
	end

	-- if self.player:objectName() == eirin:objectName() then
	-- 加个什么限制才能防止永林自己过劳导致没牌？
	-- end

	-- 用吧用吧，不太亏
	use.card = card
	if use.to then use.to:append(eirin) end
end

local yaoli_lasteffect
sgs.ai_skill_invoke["yaoli_draw"] = function(self, data)
	yaoli_lasteffect = data:toCardEffect()
	local from = yaoli_lasteffect.from

	-- 如果是队友，发动就完事了，之后再想扔什么牌
	if (from:objectName() == self.player:objectName()) or self:isFriend(from) then return true end

	-- 如果不是，要判断自己手头有没有能扔的，没啥用的，和原牌类型不同的牌。
	-- 如果有能扔的没啥用的牌，就扔

	local id = yaoli_lasteffect.card:getSubcards():first()
	local discardedType = yaoliCardType(id)

	-- 优先度：装备区白银狮子
	-- 手牌非桃
	if self.player:getArmor() and self.player:getArmor():isKindOf("SilverLion") and (discardedType ~= 3) then
		return true
	end

	local cards = self.player:getHandcards()
	local cards_without_peach = {}
	for _, card in sgs.qlist(cards) do
		if not card:isKindOf("Peach") then
			local cardType = yaoliCardType(card)
			if cardType ~= discardedType then
				table.insert(cards_without_peach, card)
			end
		end
	end

	if #cards_without_peach > 0 then
		self:sortByKeepValue(cards_without_peach, true)
		if self:getKeepValue(cards_without_peach[1]) >= sgs.ai_keep_value.Slash then return false end
		return true
	end
end
sgs.ai_skill_cardask["@yaoli-discard"] = function(self, data)
	local from = yaoli_lasteffect.from

	local isfriend = function() return self.player:objectName() == from:objectName() or  self:isFriend(from) end
	local isenemy  = function() return self.player:objectName() ~= from:objectName() and self:isEnemy (from) end

	local id = yaoli_lasteffect.card:getSubcards():first()
	local discardedType = yaoliCardType(id)

	if self.player:getArmor() and self.player:getArmor():isKindOf("SilverLion") and ((isfriend() and discardedType == 3) or (isenemy() and discardedType ~= 3)) then
		return "$" .. tostring(self.player:getArmor():getEffectiveId())
	end

	local cards = self.player:getHandcards()
	local cards_without_peach = {}
	for _, card in sgs.qlist(cards) do
		if not card:isKindOf("Peach") then
			local cardType = yaoliCardType(card)
			if ((isfriend() and discardedType == cardType) or (isenemy() and discardedType ~= cardType)) then
				table.insert(cards_without_peach, card)
			end
		end
	end

	if #cards_without_peach > 0 then
		self:sortByKeepValue(cards_without_peach, true)
		return "$" .. tostring(cards_without_peach[1]:getEffectiveId())
	end

	-- 砸锅卖铁也得拿出来一张牌。。。。
	return "$" .. tostring(self.player:getCards("hes"):first():getEffectiveId())
end
-- update intention
-- 永林不响应药理 = use.from -> eirin +10
-- 永林用不同类型的牌响应药理 = use.from -> eirin +50
-- 永林用相同类型的牌响应药理 = use.from -> eirin -30

sgs.ai_skill_playerchosen.yaolitrick = function(self, targets)
	local canadd, cancancel = {}

	local use = self.player:getTag("yaolitrick"):toCardUse()
	cancancel = sgs.QList2Table(use.to)
	for _, target in sgs.qlist(targets) do
		local flag = false
		for _, c in ipairs(cancancel) do
			if target:objectName() == c:objectName() then
				flag = true
				break
			end
		end
		if not flag then table.insert(canadd, target) end
	end

	-- canadd 是可以添加的目标
	-- cancancel 是可以取消的目标 （就是use.to啦）

	-- 基本上是aoe / GlobalEffect才会要取消目标，其余都是添加
	-- 添加目标原则？每个牌可能不一样

	if use.card:isKindOf("AmazingGrace") then
		-- 五谷：敌方秋穰子取消 / 座次最近的取消
		local cancancelEnemy = sgs.SPlayerList()
		for _, t in ipairs(cancancel) do
			if self:isEnemy(t) then
				if t:hasSkill("shouhuo", true) then return t end -- 敌方秋穰子，不能让她白嫖，最优先
				if not self:needKongcheng(t) and t:isKongcheng() then -- 敌方河童，不能取消，让她摸
					cancancelEnemy:append(t)
				end
			end
			if self:isFriend(t) then
				if self:needKongcheng(t) and t:isKongcheng() then
					-- 我方没手牌河童。取消吗？
				end
			end
		end
		if cancancelEnemy.length() > 0 then
			self.room:sortByActionOrder(cancancelEnemy)
			return cancancelEnemy:first()
		end
		-- canadd? 五谷一般都是全局，canadd一般为空
	elseif use.card:isKindOf("GlobalEffect") then
		-- 桃园 / 联军盛宴：找到敌方hp最低的取消 / 找没效果的（优先敌方）取消
		local cancancelEnemy = {}
		local cancancelEnemyNoEffect = {}
		for _, t in ipairs(cancancel) do
			if self:isEnemy(t) then
				if t:isWounded() or (use.card:isKindOf("AllianceFeast") and (not t:isDebuffStatus())) then
					table.insert(cancancelEnemy, t)
				else
					table.insert(cancancelEnemyNoEffect, t)
				end
			end
		end
		if #cancancelEnemy > 0 then
			self:sort(cancancelEnemy, "hp")
			return cancancelEnemy[1]
		end
		if #cancancelEnemyNoEffect > 0 then
			self:sort(cancancelEnemyNoEffect, "hp")
			return cancancelEnemyNoEffect[1]
		end
		-- canadd? 桃园/联军一般都是全局，canadd一般为空
	elseif use.card:isKindOf("AOE") then
		-- 南蛮/万箭/水淹：找到我方defense最低的（如果是水淹，有装备且无狮子的 ，如果是南蛮万箭，无藤甲的）取消
		local cancancelFriend = {}
		local cancancelFriendNoEffect = {}
		for _, t in ipairs(cancancel) do
			if self:isFriend(t) then
				if use.card:isKindOf("Drowning") then
					if t:getEquips():isEmpty() then
						table.insert(cancancelFriendNoEffect, t)
					elseif t:getArmor() and t:getArmor():isKindOf("SilverLion") and t:isWounded() then
						-- 有狮子！千万别取消！！取消了就扔不了狮子回血了！！！
					else
						table.insert(cancancelFriend, t)
					end
				else
					if t:getArmor() and t:getArmor():isKindOf("Vine") then
						table.insert(cancancelFriendNoEffect, t)
					else
						table.insert(cancancelFriend, t)
					end
				end
			end
		end
		if #cancancelFriend > 0 then
			self:sort(cancancelFriend, "defense")
			return cancancelFriend[1]
		end
		if #cancancelFriendNoEffect > 0 then
			self:sort(cancancelFriendNoEffect, "defense")
			return cancancelFriendNoEffect[1]
		end
	else
		-- 源码耦合，让牌可以多选择一个目标，让默认AI尝试着用一用
		use.card:setFlags("yaoli_Fsu0413AiHelperDoNotRemoveThisSkillIsToComplicatedOhMyGod")
		local dummyuse = { to = sgs.SPlayerList(), isDummy = true }
		self:useCardByClassName(use.card, dummyuse)
		use.card:setFlags("-yaoli_Fsu0413AiHelperDoNotRemoveThisSkillIsToComplicatedOhMyGod")
		-- 然后看看AI默认使用这张牌有没有增加的目标。如果有的话，就直接增加。如果没有的话，就随缘了
		if dummyuse.card then
			for _, target in sgs.qlist(dummyuse.to) do
				local flag = false
				for _, add in ipairs(canadd) do
					if target:objectName() == add:objectName() then
						flag = true
						break
					end
				end
				if flag then
					return target
				end
			end
		end
	end

	-- 随缘啦
	return targets:first()
end
-- update intention
-- 五谷：取消的目标 target -> use.from +20 / （秋穰子) +200
-- 桃园 / 联军：若被取消的目标有损血 target -> use.from +50
-- 南蛮 / 万箭：target -> use.from -40
-- 水淹：若被取消的目标没有狮子 target -> use.from -30，有狮子 target -> use.from +40
-- 其余用牌自带intention应该就行 毕竟用的是默认AI


sgs.ai_skill_playerchosen.yaoliequip = function(self, targets)
	-- 敌方有超魔理沙 - 无脑扔，一张也要扔
	-- 友方有乐或兵且没有春息，没装备或只有一张狮子且受伤，
	-- 敌方装备最多的，且装备>2，且没有乐或兵

	for _, t in sgs.qlist(targets) do
		if self:isEnemy(t) and t:hasSkill("baoyi") then return t end
	end

	for _, t in sgs.qlist(targets) do
		if self:isFriend(t) then
			if (t:containsTrick("Indulgence") or t:containsTrick("SupplyShortage")) and not t:containsTrick("SpringBreath") and (t:getEquips():length() == 0 or (t:getEquips():length() == 1 and t:getArmor() and t:getArmor():isKindOf("SilverLion") and t:isWounded())) then
				return t
			end
		end
	end

	local enemyTarget, enemyTargetLength
	for _, t in sgs.qlist(targets) do
		if self:isEnemy(t) then
			if t:getEquips():length() > 0 and not (t:containsTrick("Indulgence") or t:containsTrick("SupplyShortage")) then
				local l = t:getEquips():length()
				if (not enemyTarget) or (l > enemyTargetLength) then
					enemyTarget = t enemyTargetLength = l
				end
			end
		end
	end

	if enemyTarget then return enemyTarget end

	-- 马丹 又要强制选择，我吐了
	return targets:first()
end
