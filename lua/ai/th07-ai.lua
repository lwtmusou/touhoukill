--西行寺幽幽子
--[死蝶]
sgs.ai_skill_playerchosen.sidie = function(self, targets)
	local target = sgs.ai_skill_playerchosen.zero_card_as_slash(self, targets)
	if (target and self:isEnemy(target)) then
		return target
	end
	return nil
end

--[役灵]
sgs.ai_skill_invoke.yiling = function(self, data)
	local current = self.room:getCurrent()
	if self:isFriend(current) then
		for _,p in sgs.qlist(self.room:getOtherPlayers(current)) do
			if (self:isEnemy(p) and  current:getHandcardNum() > p:getHandcardNum() and current:canSlash(p, false)) then
				return true
			end
		end
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.yiling = function(self, player, promptlist)
	local current = self.room:getCurrent()
	if promptlist[#promptlist] == "yes" then
		sgs.updateIntention(player, current, -60)
	end
end

--[墨染]
sgs.ai_skill_invoke.moran = function(self, data)
	if not self:invokeTouhouJudge() then return false end
	local to =data:toPlayer()
	return self:isFriend(to)
end
sgs.ai_choicemade_filter.skillInvoke.moran = function(self, player, promptlist, data)
	local to=data:toPlayer()
	if to then
		if promptlist[#promptlist] == "yes" then
			sgs.updateIntention(player, to, -60)
		else
			local wizard_harm = false
			for _, aplayer in sgs.qlist(self.room:getAlivePlayers()) do
				if self:hasSkills(sgs.wizard_harm_skill , aplayer) then
					wizard_harm = true
				end
			end
			if not wizard_harm then
				sgs.updateIntention(player, to, 10)
			end
		end
	end
end

--八云紫
--[神隐]
sgs.ai_skill_invoke.shenyin = function(self, data)
	local target = self.player:getTag("shenyin-target"):toPlayer()
	if self.player:isCurrent() then
		return target:objectName() ~= self.player:objectName()
	else
		return target:objectName() == self.player:objectName()
	end
	return  false
end
--[隙间]
sgs.ai_skill_use["@@xijian"] = function(self, prompt)
	local targets={}
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if not self:isEnemy(p) then continue end
		for _,t in sgs.qlist(self.room:getOtherPlayers(p)) do
			if not self:isFriend(t) then continue end
			if not t:inMyAttackRange(p) and not p:inMyAttackRange(t) then
				if not p:isKongcheng() then
					table.insert(targets, p:objectName())
					table.insert(targets, t:objectName())
					break
				elseif not p:getCards("e"):isEmpty() then
					local can_equip = false
					for _,c in sgs.qlist(p:getCards("e")) do
						local equip = c:getRealCard():toEquipCard()
						if not t:getEquip(equip:location()) then
							can_equip = true
							break
						end
					end
					if can_equip then
						table.insert(targets, p:objectName())
						table.insert(targets, t:objectName())
						break
					end
				elseif not p:getCards("j"):isEmpty() then
					--挪延时暂时不管。。。
				end
			end
		end
		if #targets >= 2 then
			break
		end
	end
	if #targets == 2 then
		return "@XijianCard=.->" .. table.concat(targets, "+")
	end
	return "."
end

sgs.ai_card_intention.XijianCard = function(self, card, from, tos)
	sgs.updateIntention(from, tos[2], -30)
end

--八云蓝
--[式辉]
sgs.ai_skill_invoke.shihui = function(self, data)
	local to =data:toPlayer()
	local num = math.max(to:getEquips():length(), 1)
	if self:isFriend(to) then
		return num <= 2
	end
	if self:isEnemy(to) then
		return num > 2
	end
	return false
end

sgs.ai_skill_invoke.shihui_hegemony = function(self, data)
	return true
end
sgs.ai_skill_use["@@shihui_hegemonyVS"] = function(self, prompt, method)

	local cards = self.player:getCards("hes")
	cards=self:touhouAppendExpandPileToList(self.player, cards)
	cards = sgs.QList2Table(cards)
	local ecards = {}
	for _, c in ipairs(cards) do
		if c:isKindOf("EquipCard") then
			table.insert(ecards, c)
		end
	end

	if #ecards == 0 then return "." end

	self:sortByUseValue(ecards, false)

	local card = sgs.cloneCard("ex_nihilo", sgs.Card_SuitToBeDecided, -1)
	local maxNum = math.max(self.player:getEquips():length(), 1)
	card:addSubcard(ecards[1])
	card:deleteLater()

	--table.insert(ids, cards[1]:getId())

	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	card:setSkillName("_shihui")

	self:useTrickCard(card, dummy_use)

	if not dummy_use.card then return "." end
	return dummy_use.card:toString()
end

sgs.ai_choicemade_filter.cardExchange.shihui = function(self, player, args)
	local who= player:getTag("shihui_target"):toPlayer()
	if args[#args] ~= "_nil_"  and who then
		sgs.updateIntention(player, who, -20)
	end
end

sgs.ai_skill_use["@@shihuiVS"] = function(self, prompt, method)

	local cards = self.player:getCards("hes")
	cards=self:touhouAppendExpandPileToList(self.player, cards)
	cards = sgs.QList2Table(cards)
	if #cards == 0 then return "." end

	self:sortByUseValue(cards, false)

	local card = sgs.cloneCard("ex_nihilo", sgs.Card_SuitToBeDecided, -1)
	local maxNum = math.max(self.player:getEquips():length(), 1)
	--local ids = {}
	for i = 1, maxNum, 1 do
		card:addSubcard(cards[i])
	end
	card:deleteLater()

	--table.insert(ids, cards[1]:getId())

	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	card:setSkillName("_shihui")

	self:useTrickCard(card, dummy_use)

	if not dummy_use.card then return "." end
	return dummy_use.card:toString()
end
--sgs.ai_card_intention.ShihuiCard = -50

--[幻葬]
sgs.ai_skill_invoke.huanzang = function(self, data)
	local target = data:toDying().who
	if self:isEnemy(target) and  target:hasShownSkills("shijie+nengwu") then return false end
	if self:isFriend(target) then
		if  not target:getCards("je"):isEmpty() then return true end
		num = getCardsNum("Analeptic", target, self.player) + getCardsNum("Peach", target, self.player)
		if num == 0 then return true end
	end
	if self:isEnemy(target) then
		num = getCardsNum("Analeptic", target, self.player) + getCardsNum("Peach", target, self.player)
		if num > 0 then return true end
		--if target:getEquips():isEmpty() then
		--for _,p in pairs(self.enemies) do
		--getCardsNum("Peach", p, from)
	end
	return false
end

--[[sgs.ai_choicemade_filter.skillInvoke.huanzang = function(self, player, args)
	local target = self.room:getCurrentDyingPlayer()
	if target and args[#args] == "yes" then
		sgs.updateIntention(player, target, -60)
	end
end]]
sgs.ai_skill_choice.huanzang=function(self, choices, data)
	local who = data:toDying().who
	local choice_table = choices:split("+")

	if self:isFriend(who) then
		if choices:match("e") then return "e" end
		if choices:match("hs") then
			num = getCardsNum("Analeptic", target, self.player) + getCardsNum("Peach", target, self.player)
			if num == 0 then return true end
		end
		if choices:match("j") then return "j" end
	end
	if self:isEnemy(who) then--已经判定有桃酒
		if choices:match("hs") then return "hs" end
	end
	return choice_table[1]
end

--魂魄妖梦
--[斩妄]
sgs.ai_skill_invoke.zhanwang = function(self)
	local damage = self.player:getTag("zhanwang"):toDamage()
	return self:isEnemy(damage.to)
end
sgs.ai_skill_invoke.zhanwang_discard = function(self)
	local damage = self.player:getTag("zhanwang"):toDamage()
	damage.damage = damage.damage + 1
	return self:touhouNeedAvoidAttack(damage, damage.from, damage.to)
end
sgs.ai_choicemade_filter.skillInvoke.zhanwang = function(self, player, args)
	local damage = player:getTag("zhanwang"):toDamage()
	if damage.to and args[#args] == "yes" then
		sgs.updateIntention(player, damage.to, 60)
	end
end

--普莉兹姆利巴三姐妹
--[协奏]
sgs.ai_skill_use["@@xiezou"] = function(self, prompt)
	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	local cardname=self.player:property("xiezou_card"):toString()
	local card=sgs.cloneCard(cardname, sgs.Card_NoSuit, 0)
	card:setSkillName("xiezou")
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

		--[[if card:isKindOf("Collateral") then
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
		end]]
		if #target_objectname>0 then
			return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
		end
	end
	return "."
end
--[和声]
sgs.ai_skill_invoke.hesheng = true

--国战部分
--[露娜萨·普莉兹姆利巴]
--[合奏]
sgs.ai_view_as.hezou_hegemony = function(card, player, card_place)
	if (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_UNKNOWN) then
		return false
	end
	local pattern=sgs.Sanguosha:getCurrentCardUsePattern()
	if card:isKindOf("BasicCard") then return false end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place ~= sgs.Player_PlaceSpecial and not card:hasFlag("using") then
		if pattern=="slash" then
			return ("slash:hezou_hegemony[%s:%s]=%d"):format(suit, number, card_id)
		end
	end
end

local hezou_hegemony_skill = {}
hezou_hegemony_skill.name = "hezou_hegemony"
table.insert(sgs.ai_skills, hezou_hegemony_skill)
hezou_hegemony_skill.getTurnUseCard = function(self, inclusive)
		if not sgs.Slash_IsAvailable(self.player)  then return false end
		local ecards={}
		local cards=self.player:getCards("hes")
		cards=self:touhouAppendExpandPileToList(self.player,cards)
		for _,c in sgs.qlist(cards) do
			if not c:isKindOf("BasicCard") then
				table.insert(ecards,c)
			end
		end
		if #ecards==0 then return false end
		self:sortByUseValue(ecards, true)
		local suit = ecards[1]:getSuitString()
		local number = ecards[1]:getNumberString()
		local card_id = ecards[1]:getEffectiveId()
		local slash_str = ("slash:hezou_hegemony[%s:%s]=%d"):format(suit, number, card_id)
		local slash = sgs.Card_Parse(slash_str)

		assert(slash)
		return slash
end
sgs.ai_cardneed.hezou_hegemony = function(to, card, self)
	return not card:isKindOf("BasicCard")
end

--[梅露兰·普莉兹姆利巴]
--[激奏]
sgs.ai_skill_invoke.jizou_hegemony = function(self, data)
	local target = data:toPlayer()
	return target and self:isEnemy(target)
end

--[莉莉卡·普莉兹姆利巴]
--[键灵]
sgs.ai_skill_invoke.jianling_hegemony = true

--爱丽丝·玛格特洛依德
--[战阵]
sgs.ai_view_as.zhanzhen = function(card, player, card_place)
	if (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_UNKNOWN) then
		return false
	end
	local pattern=sgs.Sanguosha:getCurrentCardUsePattern()
	if not card:isKindOf("EquipCard") then return false end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place ~= sgs.Player_PlaceSpecial and not card:hasFlag("using") then
		if pattern=="jink" then
			return ("jink:zhanzhen[%s:%s]=%d"):format(suit, number, card_id)
		elseif pattern=="slash" then
			return ("slash:zhanzhen[%s:%s]=%d"):format(suit, number, card_id)
		end
	end
end

local zhanzhen_skill = {}
zhanzhen_skill.name = "zhanzhen"
table.insert(sgs.ai_skills, zhanzhen_skill)
zhanzhen_skill.getTurnUseCard = function(self, inclusive)
		if not sgs.Slash_IsAvailable(self.player)  then return false end
		local ecards={}
		local cards=self.player:getCards("hes")
		cards=self:touhouAppendExpandPileToList(self.player,cards)
		for _,c in sgs.qlist(cards) do
			if c:isKindOf("EquipCard") then
				table.insert(ecards,c)
			end
		end
		if #ecards==0 then return false end
		self:sortByUseValue(ecards, true)
		local suit = ecards[1]:getSuitString()
		local number = ecards[1]:getNumberString()
		local card_id = ecards[1]:getEffectiveId()
		local slash_str = ("slash:zhanzhen[%s:%s]=%d"):format(suit, number, card_id)
		local slash = sgs.Card_Parse(slash_str)

		assert(slash)
		return slash
end
sgs.ai_cardneed.zhanzhen = function(to, card, self)
	return card:isKindOf("EquipCard")
end

sgs.ai_skill_playerchosen.zhanzhen = function(self, targets)
	local c = self.player:getTag("zhanzhen"):toCard()
	local cards = {}
	table.insert(cards, c)

	local available_friends = {}
	if #self.friends_noself==0 then return nil end
	for _, friend in ipairs(self.friends_noself) do
		--需要check 永恒 高傲这种
		table.insert(available_friends, friend)
	end

	local card, friend = self:getCardNeedPlayer(cards)
	if card and friend and table.contains(available_friends, friend) then return friend  end

	if #available_friends > 0 then
		self:sort(available_friends, "handcard")
		for _, afriend in ipairs(available_friends) do
			if not self:needKongcheng(afriend, true) then
				return afriend
			end
		end
		self:sort(available_friends, "defense")
		return available_friends[1]
	end
	return nil
end
sgs.ai_playerchosen_intention.zhanzhen = -60
sgs.ai_no_playerchosen_intention.zhanzhen = function(self, from)
	local lord = self.room:getLord()
	if lord then
		sgs.updateIntention(from, lord, 10)
	end
end

--[人偶]
sgs.ai_skill_invoke.renou = true
--具体如何选人偶牌 尚没有策略。。。

--橙
--[奇门]
sgs.ai_skill_use["@@qimen"] = function(self, prompt)
	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	local cardname=self.player:property("qimen_card"):toString()
	local card=sgs.cloneCard(cardname, sgs.Card_NoSuit, 0)
	card:setSkillName("qimen")
	card:deleteLater()

	maxNum = 0
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if (p:getEquips():length() > maxNum) then
			maxNum = p:getEquips():length()
		end
	end

	local f, e = {}, {}
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:getEquips():length() >= maxNum and not self.player:isProhibited(p, card) then --card:targetFilter(sgs.SPlayerList(), p, self.player)
			if (self:isFriend(p)) then
				table.insert(f, p)
			end
			if (self:isEnemy(p)) then
				table.insert(e, p)
			end
		end
	end

	--Dummyuse
	local target_objectname = {}
	if card:isKindOf("TrickCard") then
		self:useTrickCard(card, dummy_use)
	else
		self:useBasicCard(card, dummy_use)
	end

	--if not dummy_use.card then return false end
	if dummy_use.to:isEmpty() then
		if card:isKindOf("IronChain") or card:isKindOf("KnownBoth") then
			return "."
		end

		if not f:isEmpty() and (card:isKindOf("Peach") or card:isKindOf("ExNihilo") or card:isKindOf("GlobalEffect")) then
			table.insert(target_objectname, f:first():objectName())
		elseif not e:isEmpty()
		and (card:isKindOf("AOE")) then
			table.insert(target_objectname, e:first():objectName())
		end
	else
	    for _,p in sgs.qlist(dummy_use.to) do
		    if table.contains(e, p)  or table.contains(f, p) then
				table.insert(target_objectname, p:objectName())
				break
			end
		end
	end
	if #target_objectname>0 then
		card = sgs.cloneCard(cardname, sgs.Card_NoSuit, 0)
		card:setSkillName("qimen")
		card:deleteLater()
		return card:toString() .. "->" .. table.concat(target_objectname, "+")
	end
	return "."
end

--[遁甲]
sgs.ai_skill_invoke.dunjia = function(self, data)
	local to = data:toPlayer()
	local num1 = self.player:getEquips():length()
	local num2 = to:getEquips():length()

	if self:isEnemy(to) and num2 > num1 then
		 return true
	end
	return false
end

--蕾蒂·霍瓦特洛克
--[记忆]
sgs.ai_skill_invoke.jiyi = true
sgs.ai_skill_askforyiji.jiyi = function(self, card_ids)
	local available_friends = {}
	if #self.friends_noself==0 then return nil, -1 end
	for _, friend in ipairs(self.friends_noself) do
		table.insert(available_friends, friend)
	end
	if self.player:getHandcardNum()<=2 then  return nil, -1 end
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
sgs.ai_Rende_intention.jiyi= -30

--[寒波 国]
sgs.ai_skill_invoke.hanbo_hegemony = function(self, data)
	local target = data:toPlayer()
	if target then
		return self:isEnemy(target)
		--if self:isFriend(target) and not target:faceup() then return true end
	end
	return false
end

--[冬至 国]
local dongzhi_hegemony_skill={}
dongzhi_hegemony_skill.name="dongzhi_hegemony"
table.insert(sgs.ai_skills, dongzhi_hegemony_skill)
dongzhi_hegemony_skill.getTurnUseCard=function(self)
	if self.player:getMark("@dongzhi") == 0 then return nil end
	return sgs.Card_Parse("@DongzhiHegemonyCard=.")
end

sgs.ai_skill_use_func.DongzhiHegemonyCard = function(card, use, self)
	local self_role = self.player:getRole()

	--[[local players = { wei = , shu = 1, wu = 1, qun = 1}

	table.removeOne(players, self_role)

	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if p:hasShownOneGeneral() or self.room:getTag(p:objectName() + "_RoleConfirmed"):toBool() then
			role = p:getRole()
			if players[role] then
				players[role] = players[role] + 1
			end
		end
	end]]

	local kingdoms = { "wei", "shu", "wu", "qun"}
	table.removeOne(kingdoms, self_role)
	table.insert(kingdoms, "careerist")

	--[[local cmp = function(a, b)
		return players[a] > players[b]
	end
	table.sort(kingdoms, cmp)
	]]
	--local p = sgs.gameProcess(true)
	local upperlimit = math.min(2, math.floor(self.room:getPlayers():length() / 2))
	local target_role = ""
	for i = 1, #kingdoms do
		local playerNum = self.player:getPlayerNumWithSameKingdom("AI", kingdoms[i])
		if playerNum >= upperlimit then
			target_role = kingdoms[i]
			break
		end
	end

	if target_role ~= "" then
		use.card = card
		if use.to then
			for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
				if p:hasShownOneGeneral() and not p:isNude()  then --or self.room:getTag(p:objectName() + "_RoleConfirmed"):toBool()
					if target_role == p:getRole() then
						use.to:append(p)
						--return  --只指定一个
					end
				end
			end
			return  --指定全部的时候
		end
	end
end

sgs.ai_use_value.DongzhiHegemonyCard = 9
sgs.ai_use_priority.DongzhiHegemonyCard = 8

--莉莉霍瓦特
--[春意]
sgs.ai_skill_invoke.chunyi =  true
--[报春]
sgs.ai_skill_playerchosen.baochun = function(self, targets)
	local target =self:touhouFindPlayerToDraw(true, self.player:getLostHp())
	if target then return target end
	if #self.friends>0 then return self.friends[1] end
	return nil
end
sgs.ai_playerchosen_intention.baochun = -80

sgs.ai_need_damaged.baochun = function(self, attacker, player)
	local x= player:getLostHp()+1
	if x>=2 and player:getHp()>1 then
		return true
	end
	return false
end

--[报春 国]
sgs.ai_skill_invoke.baochun_hegemony =  function(self, data)
	if self.player:hasFlag("Global_baochunAIFailed") then
		return true
	end
	local damage = data:toDamage()
	if  damage.to and damage.to:objectName() == self.player:objectName() then
		return true
	end
	return false
end

--[春痕 国]
sgs.ai_skill_use["@@chunhen_hegemony"] = function(self, prompt)

	local tmp = self.player:getTag("chunhen_cards"):toIntList()
	local cards = {}
	for _, card_id in sgs.qlist(tmp) do
		table.insert(cards, sgs.Sanguosha:getCard(card_id))
	end
	if #cards == 0 then return "." end

	local card, friend = self:getCardNeedPlayer(cards, self.friends_noself)--麻痹 死人不马上更新self.friends_noself
	if card and friend and friend:isAlive() then return "@ChunhenHegemonyCard=" .. card:getEffectiveId() .. "->" .. friend:objectName() end
	if #self.friends_noself == 0 then return "." end
	self:sort(self.friends_noself, "handcard")
	for _, afriend in ipairs(self.friends_noself) do
		if afriend:isAlive() and not self:needKongcheng(afriend, true) then
			return "@ChunhenHegemonyCard=" .. cards[1]:getEffectiveId() .. "->" .. afriend:objectName()
		end
	end
	return "."
end

--上海人形
--[战操]
sgs.ai_skill_cardask["@zhancao-discard"] = function(self, data)
	local use = data:toCardUse()
	local target =self.player:getTag("zhancao_target"):toPlayer()
	if not self:isFriend(target) then return "." end
	if not self:slashIsEffective(use.card, target, use.from) then return "." end
	if self:touhouCardUseEffectNullify(use,target) then return "." end
	if self:isFriend(use.from) then
		local fakeDamage=sgs.DamageStruct()
		fakeDamage.card=use.card
		fakeDamage.nature= self:touhouDamageNature(use.card, use.from, target)
		fakeDamage.damage=1
		fakeDamage.from=use.from
		fakeDamage.to= target
		if not self:touhouNeedAvoidAttack(fakeDamage, use.from,target) then  return "."  end
	end

	local cards =self.player:getCards("hes")
	cards=sgs.QList2Table(cards)
	local ecards={}
	for _,card in pairs(cards) do
		if card:isKindOf("EquipCard") then
			table.insert(ecards,card)
		end
	end

	if #ecards==0 then return "." end
	if self:isWeak(target) or getCardsNum("Jink", target, self.player) < 1 or sgs.card_lack[target:objectName()]["Jink"] >0 then
		self:sortByCardNeed(ecards)
		return "$" .. ecards[1]:getId()
	end
	return "."
end
--[[sgs.ai_choicemade_filter.skillInvoke.zhancao = function(self, player, args)
	local target =player:getTag("zhancao_target"):toPlayer()
	if target then
		if args[#args] == "yes" then
			sgs.updateIntention(player, target, -50)
		end
	end
end]]
sgs.ai_cardneed.zhancao = function(to, card, self)
	return card:isKindOf("EquipCard")
end
sgs.zhancao_keep_value = {
	EquipCard = 7
}
sgs.ai_need_bear.zhancao = function(self, card,from,tos)
	from =from or self.player
	if not card:isKindOf("EquipCard") then return false end
	if self:getSameEquip(card,from) then
		if card:isKindOf("Weapon") then
			if self:getOverflow(from) >= 0 then
				local old_range=sgs.weapon_range[from:getWeapon():getClassName()] or 0
				local new_range = sgs.weapon_range[card:getClassName()] or 0
				if new_range<=old_range then
					return true
				end
			end
		else
			return true
		end
	end
	return false
end
sgs.ai_choicemade_filter.cardResponded["@zhancao-discard"] = function(self, player, args)
	local target = player:getTag("zhancao_target"):toPlayer()
	if target and args[#args] ~= "_nil_" then
		sgs.updateIntention(player, target, -60)
	end
end

--[魔操]
local mocao_skill = {}
mocao_skill.name = "mocao"
table.insert(sgs.ai_skills, mocao_skill)
mocao_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("MocaoCard") then return nil end
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if  p:getCards("e"):length()>0 then
			t=true
			break
		end
	end
	if t then
		return sgs.Card_Parse("@MocaoCard=.")
	end
	return nil
end
sgs.ai_skill_use_func.MocaoCard = function(card, use, self)
	local targets={}
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if  p:getCards("e"):length()>0 then
			table.insert(targets,p)
		end
	end
	--需要一个全新的sort
	self:sort(targets,"value")
	for _, p in ipairs(targets) do
		if (p:getLostHp()<2 and self:isEnemy(p)) or (p:getLostHp()>1 and self:isFriend(p))  then
			use.card = card
			if use.to then
				use.to:append(p)
				if use.to:length() >= 1 then return end
			end
		end
	end
end
sgs.ai_use_value.MocaoCard = 9
sgs.ai_use_priority.MocaoCard = 6
sgs.ai_card_intention.MocaoCard = function(self, card, from, tos)
	if tos[1]:getLostHp()>=2  then
		sgs.updateIntention(from, tos[1], -30)
	else
		sgs.updateIntention(from, tos[1], 30)
	end
end

--SP千年紫
--[境界]
sgs.ai_skill_invoke.jingjie = true
sgs.ai_skill_discard.jingjie = function(self)
	local to_discard = {}
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)

	table.insert(to_discard, cards[1]:getEffectiveId())

	return to_discard
end

--[死生]
sgs.ai_skill_cardask["@sisheng-invoke"] = function(self)
	local who = self.room:getCurrentDyingPlayer()

	local getReturn = function()
		return "$" .. self.player:getPile("jingjie"):first()
	end

	if self:isFriend(who) then
		if getCardsNum("Peach", who, self.player) >= 1 or getCardsNum("Analeptic", who, self.player) >= 1 then
			if self:hasWeiya(who)  then
				return getReturn()
			elseif who:getHandcardNum()>=3 then
				return getReturn()
			end
			return "."
		end
		return getReturn()
	end
	return "."
end

sgs.ai_choicemade_filter.cardResponded["@sisheng-invoke"] = function(self, player, args)
	local who= player:getRoom():getCurrentDyingPlayer()
		if args[#args] ~= "_nil_"  and who then
			sgs.updateIntention(player, who, -70)
		--else  --明桃不救的情况暂时不好排除
		--  if player:getPile("jingjie"):length()>=2 then
		--      sgs.updateIntention(player, who, 60)
		--  end
		end
end

sgs.ai_skill_cardchosen.sisheng = function(self, who, flags)
	if flags == "hes" then
		local who= self.player:getRoom():getCurrentDyingPlayer()
		if who:getEquip(1) and who:getEquip(1):isKindOf("SilverLion") then
			return who:getEquip(1)
		end
		local hcards = who:getCards("hs")
		if hcards:length()>0 then
			return hcards:first()
		end
		local ecards = who:getCards("e")
		if ecards:length()>0 then
			return ecards:first()
		end
	end
end

--[静动]
sgs.ai_skill_cardask["@jingdong-target"] = function(self, data)
	local target = data:toPlayer()
	if target:hasSkill("huanmeng") or target:hasSkill("zaozu") or target:hasSkill("yongheng") then
		return "."
	end
	num = target:getHandcardNum() - target:getMaxCards()
	if num == 0 then return "." end
	if self.player:getPile("jingjie"):isEmpty() then return "." end
	if not self:isFriend(target) then
		return "."
	else
		local getReturn = function()
			return "$" .. self.player:getPile("jingjie"):first()
		end
		if self:isWeak(target) and num > 0 then
			return getReturn()
		end
		if num > 1 then
			if  self.player:getPile("jingjie"):length() > 3 then --self:cautionChangshi() or
				return getReturn()
			else
				if num > 3 then
					return getReturn()
				end
			end
		end
	end
	return "."
end

sgs.ai_choicemade_filter.cardResponded["@jingdong-target"] = function(self, player, args)
	local to = self.room:getCurrent()
	if not (to:hasSkill("huanmeng") or to:hasSkill("zaozu") or to:hasSkill("yongheng"))then
		num = to:getHandcardNum() - to:getMaxCards()
		if args[#args] ~= "_nil_" then
			sgs.updateIntention(player, to, -60)
		else
			if num >= 3 then
				sgs.updateIntention(player, to, 30)
			end
		end
	end
end

--SP千年幽幽子
--[幽曲]
sgs.ai_choicemade_filter.skillChoice.youqu = function(self, player, args)
	sgs.siling_lack[player:objectName()]["Red"] = 0
	sgs.siling_lack[player:objectName()]["Black"] = 0
end
--[亡舞]
sgs.ai_skill_cardask["@wangwu-invoke"] = function(self, data, pattern)
	local target = data:toCardUse().from
	if not target then return "." end
	if self:isEnemy(target) then
		local card = data:toCardUse().card
		local cards = {}
		for _, id in sgs.qlist(self.player:getHandCards()) do
			if id:getColor() == card:getColor() and id:getTypeId() == card:getTypeId() and not id:isKindOf("Peach") then
				table.insert(id)
			end
		end
		if #cards <2 then return "." end
		self:sortByKeepValue(cards)
		return "$" .. cards[1]:getId() .. "+" .. cards[2]:getId()
	end
	return "."
end

--九尾妖狐SP蓝
--[示兆]
sgs.ai_skill_invoke.shizhao = true
sgs.ai_skill_askforag.shizhao = function(self, card_ids)
	local cards = {}
	for _,id in pairs (card_ids) do
		table.insert(cards, sgs.Sanguosha:getCard(id))
	end
	local current = self.room:getCurrent()
	local inverse = not self:isFriend(current)
	self:sortByUseValue(cards, inverse)

	local value = self:getUseValue(cards[1])
	if (self:isFriend(current) and value < 6 ) then
		return cards[1]:getId()
	elseif (not self:isFriend(current) and value >= 6) then
		return cards[1]:getId()
	end
	return -1
end

--[吉凶]
sgs.ai_skill_invoke.jixiong1 = function(self,data)
	local player = data:toPlayer()
	if player then
		return self:isFriend(player)
	end
	return false
end
sgs.ai_skill_invoke.jixiong2 = function(self,data)
	local player = data:toPlayer()
	if player then
		return self:isEnemy(player)
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.jixiong1 = function(self, player, args)
	local current = self.room:getCurrent()
	if current then
		if args[#args] == "yes" then
			sgs.updateIntention(player, current, -50)
		elseif sgs.evaluatePlayerRole(current) ~= "neutral" then
			sgs.updateIntention(player, current, 20)
		end
	end
end
sgs.ai_choicemade_filter.skillInvoke.jixiong2 = function(self, player, args)
	local current = self.room:getCurrent()
	if current then
		if args[#args] == "yes" then
			sgs.updateIntention(player, current, 50)
		end
	end
end

--魂魄妖忌
--[御剑]
local yujian_skill = {}
yujian_skill.name = "yujian"
table.insert(sgs.ai_skills, yujian_skill)
yujian_skill.getTurnUseCard = function(self)
	if self.player:isKongcheng() or self.player:hasUsed("YujianCard") then return nil end
	return sgs.Card_Parse("@YujianCard=.")
end
sgs.ai_skill_use_func.YujianCard = function(card, use, self)
	local target
	self:sort(self.enemies, "defenseSlash")
	for _,p in ipairs (self.enemies) do
		if not p:isKongcheng() then
			target = p
		end
	end
	if target then
		use.card = card
		if use.to then
			use.to:append(target)
			self.yujian_card = self:getMaxCard():getEffectiveId()
			if use.to:length() >= 1 then return end
		end
	end
end
sgs.ai_use_priority.YujianCard = sgs.ai_use_priority.Slash + 0.5
sgs.ai_card_intention.YujianCard = 30

--[授术]
sgs.ai_skill_playerchosen.shoushu = function(self, targets)
	local target = self:touhouFindPlayerToDraw(false, 1)
	if not target and #self.friends_noself>0 then
		target= self.friends_noself[1]
	end
	if target then
		return target
	end
	return nil
end
sgs.ai_playerchosen_intention.shoushu = -70

--蕾拉·普莉兹姆利巴
--[华音]
function SmartAI:canHuayin(player)
	for _,c in sgs.qlist(player:getCards("hs")) do
		if not c:isKindOf("BasicCard") then
			return true
		end
	end
	return false
end
function turnUse_huayin(self)
	if self.player:getMark("Global_PreventPeach")>0  or self.player:hasFlag("Global_huayinFailed") then return nil end
	if self:canHuayin(self.player) then
		return "@HuayinCard=."
	end
	return nil
end

local huayin_skill = {}
huayin_skill.name = "huayin"
table.insert(sgs.ai_skills, huayin_skill)
huayin_skill.getTurnUseCard = function(self)
	local str = turnUse_huayin(self)
	if not str then return nil end
	return sgs.Card_Parse(str)
end

function sgs.ai_cardsview_valuable.huayin(self, class_name, player)
	if class_name == "sqchuangshi" then
		return turnUse_huayin(self)
	end
	if class_name == "Peach" then
		if (sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
			return nil
		end
		if self.player:getMark("Global_PreventPeach")>0  or player:hasFlag("Global_huayinFailed") then return nil end
		local dying = player:getRoom():getCurrentDyingPlayer()
		if self:canHuayin(player) and dying and self:isFriend(dying, player)  then
			return "@HuayinCard=."
		end
		--if not dying or dying:objectName() == player:objectName() then return nil end
		return nil
	end
end

sgs.ai_skill_use_func.HuayinCard=function(card,use,self)
	if (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_PLAY) then
		local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
		local peach = sgs.cloneCard("peach", sgs.Card_NoSuit, 0)
		peach:setSkillName("huayin")
		peach:deleteLater()
		if self.player:isWounded() and not self.player:isCardLimited(peach, sgs.Card_MethodUse, true) then
			use.card = card
		end
		--响应【宴会】没达成
		--self:useBasicCard(card, dummy_use)
		--if dummy_use.to:isEmpty() then
		--  dummy_use.to:append(self.player)
		--end

		--[[if use.to then
			use.to:append(dummy_use.to:first())
			if use.to:length() >= 1 then return end
		end]]
	else
		use.card=card
	end
end
sgs.ai_use_priority.HuayinCard = sgs.ai_use_priority.ExNihilo - 0.3
--[唤灵]
sgs.ai_skill_invoke.huanling = true
