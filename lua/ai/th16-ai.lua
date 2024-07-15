
--天空璋：摩多罗
-- 崩服务器，待查，先注释掉

--[==[
--[秘神]
local mishen_skill={}
mishen_skill.name="mishen"
table.insert(sgs.ai_skills,mishen_skill)
mishen_skill.getTurnUseCard=function(self)
	local avail = {}
	for _,c in sgs.qlist(self.player:getCards("e")) do
        if (not self.player:isBrokenEquip(c:getEffectiveId())) then
            table.insert(avail, c)
		end
    end

	self:sortByUseValue(avail,true)

	if #avail == 0 then
		if not self.player:isChained() then
			return sgs.Card_Parse("@MishenCard=.")
		end
	else
		return sgs.Card_Parse("@MishenCard=" .. avail[1]:getEffectiveId())
	end
end
sgs.ai_skill_use_func.MishenCard = function(card, use, self)
	local lt =sgs.cloneCard("lure_tiger")
	lt:setSkillName("mishen")

	assert(lt)
	self:useTrickCard(lt, use)

	if not use.card then return end
	use.card=card
end

sgs.ai_use_priority.MishenCard =  sgs.ai_use_priority.LureTiger

--[里季]
--武器：【知己知彼】；防具：【铁索连环】；进攻马：火【杀】；防御马：【桃】；宝物：【调虎离山】
local liji_skill={}
liji_skill.name="liji"
table.insert(sgs.ai_skills,liji_skill)
liji_skill.getTurnUseCard=function(self)
	return sgs.Card_Parse("@LijiCard=.")
end
sgs.ai_skill_use_func.LijiCard = function(card, use, self)
	local nextTarget = self.room:findPlayerByObjectName(self.player:getNextAlive():objectName())
	local lastTarget = self.room:findPlayerByObjectName(self.player:getLastAlive():objectName())

	local function goodTarget(target)
		local can = false
		for _,c in sgs.qlist(target:getEquips()) do
			if (not target:isBrokenEquip(c:getEffectiveId())) then
				if c:isKindOf("Weapon") and self:isEnemy(target) then
					can = true
					break
				elseif c:isKindOf("Armor") and self:isFriend(target) and target:isChained()  then
					can = true
					break
				elseif c:isKindOf("Armor") and self:isEnemy(target) and not target:isChained()  then
					can = true
					break
				elseif c:isKindOf("OffensiveHorse") and self:isEnemy(target)  then
					can = true
					break
				elseif c:isKindOf("DefensiveHorse") and self:isFriend(target)  then
					can = true
					break
				elseif c:isKindOf("Treasure") then
					can = true
					break
				end
			end
		end

		return can
	end

	if nextTarget and self.player:objectName() ~= nextTarget:objectName() then
		local can = goodTarget(nextTarget)
		if can then
			use.card=card
			if use.to then
				use.to:append(nextTarget)
				return
			end
		end
	end

	if lastTarget and self.player:objectName() ~= lastTarget:objectName() then
		local can = goodTarget(lastTarget)
		if can then
			use.card=card
			if use.to then
				use.to:append(lastTarget)
				return
			end
		end
	end
end

sgs.ai_use_priority.LijiCard =  sgs.ai_use_priority.LureTiger + 0.5

sgs.ai_skill_cardchosen.liji = function(self, who, flags)

	for _,c in sgs.qlist(who:getCards("e")) do
		if (not who:isBrokenEquip(c:getEffectiveId())) then
			if c:isKindOf("Weapon") and self:isEnemy(who) then
				return c
			elseif c:isKindOf("Armor") and self:isFriend(who) and who:isChained()  then
				return c
			elseif c:isKindOf("Armor") and self:isEnemy(who) and not who:isChained()  then
				return c
			elseif c:isKindOf("OffensiveHorse") and self:isEnemy(who)  then
				return c
			elseif c:isKindOf("DefensiveHorse") and self:isFriend(who)  then
				return c
			elseif c:isKindOf("Treasure") then
				return c
			end
		end
	end

	return who:getCards("e"):first()
end

--[后光]
sgs.ai_skill_invoke.houguang = true
sgs.ai_skill_invoke.houguanghide =function(self,data)
	local user = self.room:getLord()
	if not user then  return false end
	return self:isFriend(user)
end
]==]

--天空璋：爱塔妮缇拉尔瓦
--[鳞洒]
sgs.ai_skill_invoke.linsa = function(self, data)
	local to = data:toPlayer()
	return self:isEnemy(to) -- and not self:touhouHandCardsFix(to)
end
local linsa_skill={}
linsa_skill.name="linsa"
table.insert(sgs.ai_skills,linsa_skill)
linsa_skill.getTurnUseCard=function(self)
	local cards = self.player:getCards("hs")
	cards=self:touhouAppendExpandPileToList(self.player, cards)
	cards=sgs.QList2Table(cards)
	local card
	self:sortByUseValue(cards,true)
	for _,acard in ipairs(cards) do
		if acard:getSuit() == sgs.Card_Diamond and not acard:isKindOf("Peach") and (self:getDynamicUsePriority(acard) < sgs.ai_use_value.FireAttack or self:getOverflow() > 0) then
			if acard:isKindOf("Slash") and self:getCardsNum("Slash") == 1 then
				local keep
				local dummy_use = { isDummy = true , to = sgs.SPlayerList() }
				self:useBasicCard(acard, dummy_use)
				if dummy_use.card and dummy_use.to and dummy_use.to:length() > 0 then
					for _, p in sgs.qlist(dummy_use.to) do
						if p:getHp() <= 1 then keep = true break end
					end
					if dummy_use.to:length() > 1 then keep = true end
				end
				if keep then sgs.ai_use_priority.Slash = sgs.ai_use_priority.KnownBoth + 0.1
				else
					sgs.ai_use_priority.Slash = 2.6
					card = acard
					break
				end
			else
				card = acard
				break
			end
		end
	end
	if not card then return nil end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local card_str = ("known_both:linsa[%s:%s]=%d"):format(suit, number, card_id)
	local skillcard = sgs.Card_Parse(card_str)
	assert(skillcard)
	return skillcard
end
sgs.ai_cardneed.linsa = function(to, card, self)
	return card:getSuit() == sgs.Card_Diamond
end

--天空璋：坂田合欢
--[磨刀]
local modao_skill={}
modao_skill.name="modao"
table.insert(sgs.ai_skills,modao_skill)
modao_skill.getTurnUseCard=function(self)
	local cards = self.player:getCards("hes")
	cards=self:touhouAppendExpandPileToList(self.player, cards)
	cards=sgs.QList2Table(cards)
	local card
	self:sortByUseValue(cards,true)
	for _,acard in ipairs(cards) do
		if acard:getSuit() == sgs.Card_Club then
			card = acard
			break
		end
	end
	if not card then return nil end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local card_str = ("bone_healing:modao[%s:%s]=%d"):format(suit, number, card_id)
	local skillcard = sgs.Card_Parse(card_str)
	assert(skillcard)
	return skillcard
end
sgs.ai_cardneed.modao = function(to, card, self)
	return card:getSuit() == sgs.Card_Club
end

--天空璋：高丽野阿吽
--[寻佛]
sgs.ai_skill_invoke.xunfo = true
--[镇社]
local zhenshe_skill={}
zhenshe_skill.name="zhenshe"
table.insert(sgs.ai_skills,zhenshe_skill)
zhenshe_skill.getTurnUseCard=function(self)
	if self.player:hasFlag("zhenshe") then return nil end
	local cards = self.player:getCards("hes")
	cards=self:touhouAppendExpandPileToList(self.player, cards)
	cards=sgs.QList2Table(cards)
	local card
	self:sortByUseValue(cards,true)
	for _,acard in ipairs(cards) do
		if acard:getSuit() == sgs.Card_Heart then
			card = acard
			break
		end
	end
	if not card then return nil end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local card_str = ("saving_energy:zhenshe[%s:%s]=%d"):format(suit, number, card_id)
	local skillcard = sgs.Card_Parse(card_str)
	assert(skillcard)
	return skillcard
end

--天空璋：矢田寺成美
--[菩提]
sgs.ai_skill_invoke.puti = true

sgs.ai_skill_discard.puti = function(self,discard_num)
	local gamerule_discard={}
	if (getCardsNum("Slash", self.player, self.player) == 0) then
		return gamerule_discard
	end

	gamerule_discard = sgs.ai_skill_discard.juhe(self,  1)

	return gamerule_discard
end

--[藏法]
sgs.ai_skill_playerchosen.zangfa = function(self, targets)
	local use = self.player:getTag("zangfa_use"):toCardUse()
	for _,p in sgs.qlist(targets) do
		local res = wunian_judge(self,p, use.card)
		if res==1 and self:isEnemy(p) then
			return p
		elseif res==2 and self:isFriend(p)  then
			return p
		end
	end

	return nil
end

sgs.ai_playerchosen_intention.zangfa = function(self, from, to)
	local use = from:getTag("zangfa_use"):toCardUse()
	local res = wunian_judge(self, to, use.card)
	if res==1 then
		sgs.updateIntention(from, to, 30)
	elseif res== 2 then
		sgs.updateIntention(from, to, -30)
	end
end

function kuangwu_judge(self,target,use)
    local card = use.card
	if card:isKindOf("Peach") or card:isKindOf("Analeptic") then
		return 2
	end
	if card:isKindOf("Slash") then
		return 1
	end
	if card:isKindOf("AmazingGrace") then
		return 3
	end
	if card:isKindOf("GodSalvation") then
		if target:isWounded() then
			return 2
		else
			return 1
		end
	end
	if card:isKindOf("AwaitExhausted") then
		return 2
	end

	if card:isKindOf("IronChain") then
		if target:isChained() then
			return 2
		else
			return 1
		end
	end
	if   card:isKindOf("Dismantlement") or card:isKindOf("Snatch") then
		if self:isFriend(use.from ,target) and target:getCards("j"):length()>0 then
			return 2
		end
		if self:isEnemy(use.from, target) and target:getCards("j"):length()>0  and target:isNude() then
			return 2
		end
		return 1
	end
	return 1
end

--天空璋：尔子田里乃
--[鼓舞]
sgs.ai_skill_use["@@guwu"] = function(self, prompt)
	local  l =self.player:property("guwuavailability"):toString():split("+")
	local targets =sgs.QList2Table(self.room:getAlivePlayers())
	self:sort(targets,"defense")
	local avail_targets = {}
	local use = self.player:getTag("guwu"):toCardUse()

	for _,p in ipairs(targets) do
		if table.contains(l, p:objectName()) then
			local res = kuangwu_judge(self,p, use)
			if res == 2 and self:isFriend(p) then
				table.insert(avail_targets, p:objectName())
			elseif res == 1 and self:isEnemy(p) then
				table.insert(avail_targets, p:objectName())
			end
		end
	end

	if #avail_targets == 0 then return "." end

	local avail = {}
	for _,c in sgs.qlist(self.player:getCards("e")) do
		if (not self.player:isBrokenEquip(c:getEffectiveId())) then
            table.insert(avail, c)
		end
	end

	if #avail > 0 then
		return "@GuwuCard=".. avail[1]:getId()  .. "->" .. avail_targets[1]
	elseif not self.player:isChained() then
		return "@GuwuCard=.->" .. avail_targets[1]
	end
	return "."
end

--[茗荷]
sgs.ai_skill_use["@@minghe"] = function(self, prompt)
	local targets={}
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:isChained() or not p:getShownHandcards():isEmpty() or not p:getBrokenEquips():isEmpty() then
			if self:isEnemy(p) and p:getHp() > self.player:getHp() then
				table.insert(targets,p:objectName())
			elseif self:isFriend(p) and p:getHp() <= self.player:getHp() then
				table.insert(targets,p:objectName())
			end
		end
	end
	if #targets >1 then
		return "@MingheCard=.->" .. table.concat(targets, "+")
	end
	return "."
end

--天空璋：丁礼田舞
--[狂舞]
sgs.ai_skill_use["@@kuangwu"] = function(self, prompt)
	local  l =self.player:property("kuangwuavailability"):toString():split("+")
	local targets =sgs.QList2Table(self.room:getAlivePlayers())
	self:sort(targets,"defense")
	local avail_targets = {}
	local use = self.player:getTag("kuangwu"):toCardUse()

	for _,p in ipairs(targets) do
		if table.contains(l, p:objectName()) then
			local res = kuangwu_judge(self,target, use)
			if res == 2 and self:isFriend(p) then
				table.insert(avail_targets, p:objectName())
			elseif res == 1 and self:isEnemy(p) then
				table.insert(avail_targets, p:objectName())
			end
		end
	end

	if #avail_targets == 0 then return "." end

	local avail = {}
	for _,c in sgs.qlist(self.player:getCards("e")) do
		if (not self.player:isBrokenEquip(c:getEffectiveId())) then
            table.insert(avail, c)
		end
	end

	if #avail > 0 then
		return "@KuangwuCard=".. avail[1]:getId()  .. "->" .. avail_targets[1]
	elseif not self.player:isChained() then
		return "@KuangwuCard=.->" .. avail_targets[1]
	end
	return "."
end

--[竹屉]
sgs.ai_skill_use["@@zhuti"] = function(self, prompt)
	local targets={}
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:isChained() or not p:getShownHandcards():isEmpty() or not p:getBrokenEquips():isEmpty() then
			if self:isEnemy(p) and p:getHp() > self.player:getHp() then
				table.insert(targets,p:objectName())
			elseif self:isFriend(p) and p:getHp() <= self.player:getHp() then
				table.insert(targets,p:objectName())
			end
		end
	end
	if #targets >1 then
		return "@ZhutiCard=.->" .. table.concat(targets, "+")
	end
	return "."
end

--天空璋：SP摩多罗隐岐奈
--[门扉]
local menfei_skill = {}
menfei_skill.name = "menfei"
table.insert(sgs.ai_skills, menfei_skill)
menfei_skill.getTurnUseCard = function(self)
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:getMark("@door") > 0 then return nil end
	end
	return sgs.Card_Parse("@MenfeiCard=.")
end

sgs.ai_skill_use_func.MenfeiCard = function(card, use, self)
	use.card = card
	if use.to then
		use.to:append(self.player)
		if use.to:length() >= 1 then return end
	end
end

--[后户]
sgs.ai_skill_invoke.houhu =function(self,data)
	local use = data:toCardUse()
	local target
	for _,p in sgs.qlist(self.room:getAlivePlayers())do
		if p:getMark("@door") > 0 then
			target = p
			break
		end
	end

	if use.to:contains(target) then
		return true
	else
		local res = wunian_judge(self,target, use.card)
		if res==1 and self:isEnemy(target) then
			return true
		elseif res==2 and self:isFriend(target)  then
			return true
		end
	end
	return false
end

--天空璋：SP莉莉霍瓦特
--[春腾]
sgs.ai_skill_use["@@chunteng"] = function(self, prompt)
	if self.friends_noself == 0 then return nil end
	local use = self.player:getTag("chunteng_use"):toCardUse()
	local l = {}
	for _, id in sgs.qlist(self.player:getPile("spring")) do
		local c = sgs.Sanguosha:getCard(id)
		table.insert(l, c)
	end
	self:sortByUseValue(l,true)
	return "@ChuntengCard=".. l[1]:getId() .."->" .. self.friends_noself[1]:objectName()
end

sgs.ai_skill_invoke.chunteng_discard = function(self,data)
	local effect = data:toCardEffect()
	return self:isEnemy(effect.from) -- 判定啃腚是不够的，但是先这样
end

--[花朝]
sgs.ai_skill_invoke.huazhao = true
sgs.ai_skill_use["@@huazhao"] = function(self, prompt)
	local l = self.player:getPile("spring")
	return "@HuazhaoCard=".. l:first() .."->."
end
