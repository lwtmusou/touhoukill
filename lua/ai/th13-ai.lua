
sgs.ai_needToWake.shengge=function(self,player)
	return "Kongcheng","StartPhase"
end

local qingting_skill = {}
qingting_skill.name = "qingting"
table.insert(sgs.ai_skills, qingting_skill)
qingting_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("QingtingCard") then return nil end
	t=false
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if not p:isKongcheng() then
			t=true
		end
	end
	if t then
	return sgs.Card_Parse("@QingtingCard=.")
	end
	return nil
end
sgs.ai_skill_use_func.QingtingCard=function(card,use,self)
	use.card = card
end
sgs.ai_skill_discard.qingting_give = function(self)
	local target=self.player:getTag("qingting_give"):toPlayer()
	local to_discard = {}
	local cards = sgs.QList2Table(self.player:getHandcards())

	if self:isFriend(target) then
		self:sortByUseValue(cards, true)
	else
		self:sortByUseValue(cards)
	end

	local tmpCard = cards[1]
	if self:isEnemy(target) and tmpCard:isKindOf("Peach") then
		for var= 1, #cards, 1 do
			if not cards[var]:isKindOf("Peach") then
				tmpCard = cards[var]
				break
			end
		end
	end
	table.insert(to_discard, tmpCard:getEffectiveId())
	return to_discard
end
sgs.ai_skill_discard.qingting = function(self)
	local target = self.player:getTag("qingting_return"):toPlayer()
	local to_discard = {}

	local need_give = 1

	for _,p in sgs.qlist(self.room:getOtherPlayers(target)) do
		if p:getMark("@qingting")>0 then
			need_give = need_give + 1
		end
	end

	
	if target:hasSkill("chunxi") or target:hasSkill("xingyun") then
		local redcard
		for _,c in sgs.qlist(self.player:getHandcards())do
			if self:isFriend(target) then
				if c:getSuit()==sgs.Card_Heart then
					redcard=c
					break
				end
			else
				if c:getSuit()~=sgs.Card_Heart then
					redcard=c
					break
				end
			end
		end
		if  redcard then
			table.insert(to_discard, redcard:getEffectiveId())
			return to_discard
		end
	end
	

	local cards={}
	if self.player:getHandcards():length() > need_give then
		local tmpcards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(tmpcards)
		for var=1, need_give, 1 do
			table.insert(cards,tmpcards[var])
		end
	else
		cards = sgs.QList2Table(self.player:getHandcards())
	end

	if self:isFriend(target) then
		self:sortByUseValue(cards, true)
	else
		self:sortByUseValue(cards)
	end

	local tmpCard = cards[1]
	if self:isEnemy(target) and tmpCard:isKindOf("Peach") then
		for var= 1, #cards, 1 do
			if not cards[var]:isKindOf("Peach") then
				tmpCard = cards[var]
				break
			end
		end
	end
	table.insert(to_discard, tmpCard:getEffectiveId())
	return to_discard
end

sgs.ai_use_value.QingtingCard = 7
sgs.ai_use_priority.QingtingCard = 7



sgs.ai_skill_invoke.chiling = function(self,data)
	local isSlash=self.player:getTag("chiling_showslash"):toInt()
	local slasher=self.player:getTag("chiling_givener"):toPlayer()
	if isSlash==1 then
		return self:isFriend(slasher)
	else
		return self:isEnemy(slasher)
	end
	return false
end



measure_xihua = function(self,card)
	local pattern = card:objectName()
	if card:isKindOf("Slash") then
		pattern = "slash"
	elseif card:isKindOf("Peach") then
		pattern = "peach"
	elseif card:isKindOf("Jink") then
		pattern = "jink"
	elseif card:isKindOf("Analeptic") then
		pattern = "analeptic"	
	end
	local xihuaUsed = "xihua_record_" .. pattern
	if self.player:getMark(xihuaUsed) > 0 then
		return false
	end

	local success=0
	for _,c in sgs.qlist(self.player:getCards("hs")) do
		if card:objectName()==c:objectName() then
			return false
		end
		if card:isKindOf("BasicCard") and c:isKindOf("BasicCard") then
			success =success+1
		elseif  card:isKindOf("TrickCard") and c:isKindOf("TrickCard") then
			success =success+1
		elseif c:getNumber()>10 then
			success =success+1
		end
	end
	local num=self.player:getHandcardNum()
	if success< num and success>0 then
		local duxin=self.room:findPlayerBySkillName("duxin")
		if duxin and self:isFriend(duxin) then
			return true
		elseif duxin and self:isEnemy(duxin) and self.room:alivePlayerCount()==2 then
			return false
		end
	end
	num = num/2
	--但是危险时刻就顾不得成功率了。。。 比如要死的时候南蛮出杀 队友求桃 你总得试一下吧。。。
	return success > 0 and success >=  num
end
sgs.ai_skill_playerchosen.xihua = function(self, targets)--选择展示人
	local duxin=self.room:findPlayerBySkillName("duxin")
	if duxin then
		if self:isFriend(duxin) then
			return duxin
		end
		if targets:length()>1 then
			targets:removeOne(duxin)
		end
	end
	return targets:first()
end

local xihua_skill = {}
xihua_skill.name = "xihua"
table.insert(sgs.ai_skills, xihua_skill)
xihua_skill.getTurnUseCard = function(self)
	if self.player:isKongcheng() then return end
	local current = self.room:getCurrent()
	if not current or current:isDead() or current:getPhase() == sgs.Player_NotActive then return end

	local cards = sgs.QList2Table(self.player:getHandcards())
	local XihuaCards = {}

	local guhuo = "slash|jink|peach|ex_nihilo|snatch|dismantlement|amazing_grace|archery_attack|savage_assault"
	local ban = table.concat(sgs.Sanguosha:getBanPackages(), "|")
	if not ban:match("maneuvering") then guhuo = guhuo .. "|fire_attack|analeptic|thunder_slash|fire_slash" end
	if not ban:match("test_card") then guhuo = guhuo .. "|super_peach|magic_analeptic|light_slash|iron_slash|power_slash" end
	local guhuos = guhuo:split("|")
	for i = 1, #guhuos do
		local forbidden = guhuos[i]
		local forbid = sgs.cloneCard(forbidden)
		if not self.player:isLocked(forbid) and self:canUseXihuaCard(forbid, true) then
			table.insert(XihuaCards,forbid)
		end
	end

	self:sortByUseValue(XihuaCards, false)
	for _,XihuaCard in pairs (XihuaCards) do
		if measure_xihua(self,XihuaCard) then
			local dummyuse = { isDummy = true }
			if XihuaCard:isKindOf("BasicCard") then
				self:useBasicCard(XihuaCard, dummyuse)
			else
				self:useTrickCard(XihuaCard, dummyuse)
			end
			if dummyuse.card then
				fakeCard = sgs.Card_Parse("@XihuaCard=.:" .. XihuaCard:objectName())
				return fakeCard
			end
		end
	end
	return nil
end
sgs.ai_skill_use_func.XihuaCard=function(card,use,self)
	local userstring=card:toString()
	userstring=(userstring:split(":"))[3]
	local xihuacard=sgs.cloneCard(userstring)
	xihuacard:setSkillName("xihua")
	if xihuacard:getTypeId() == sgs.Card_TypeBasic then self:useBasicCard(xihuacard, use)
	else
		assert(xihuacard)
		self:useTrickCard(xihuacard, use)
	end
	if not use.card then return end
	use.card=card
end

--sgs.ai_use_priority.XihuaCard = 10
function sgs.ai_cardsview_valuable.xihua(self, class_name, player)
	if self.player:isKongcheng() then
		return nil
	end

	if (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_UNKNOWN) then
		return nil
	end
	local classname2objectname = {
		["Slash"] = "slash", ["Jink"] = "jink",
		["Peach"] = "peach", ["Analeptic"] = "analeptic",
		["Nullification"] = "nullification",
		["FireSlash"] = "fire_slash", ["ThunderSlash"] = "thunder_slash",
		["ChainJink"] = "chain_jink", ["LightJink"] = "light_jink",
		["MagicAnaleptic"] = "magic_analeptic",["SuperPeach"] = "super_peach"
	}

	if classname2objectname[class_name] then
		local viewcard = sgs.cloneCard(classname2objectname[class_name])
		if self.player:isLocked(viewcard) then
			return nil
		end
		if measure_xihua(self,viewcard) then
			return "@XihuaCard=.:".. classname2objectname[class_name]
		end
	end
end


function SmartAI:canUseXihuaCard(card, at_play)
	local player = self.player
	if at_play then
		if card:isKindOf("Peach") and not player:isWounded() then return false
		elseif card:isKindOf("Analeptic") and card:isAvailable(player) then return false
		elseif card:isKindOf("Slash") and not self:slashIsAvailable(player) then return false
		elseif card:isKindOf("Jink") or card:isKindOf("Nullification") then return false
		end
	else
		if card:isKindOf("Peach") and self.player:hasFlag("Global_PreventPeach") then return  false end
	end
	return   true --self:getxihuaViewCard(class_name)
end

sgs.ai_skill_cardchosen.xihua = function(self, who, flags)
	local cards = who:getHandcards()
	cards = sgs.QList2Table(cards)
	if self.player:hasSkill("duxin") then
		local choice=who:getTag("xihua_choice"):toString()
		local xihua = sgs.cloneCard(choice)

		self:sortByKeepValue(cards)

		local success={}
		local unsuccess={}
		for _,c in pairs(cards) do
			if xihua:isKindOf("BasicCard") and c:isKindOf("BasicCard") then
				table.insert(success,c)
			elseif  xihua:isKindOf("TrickCard") and c:isKindOf("TrickCard") then
				table.insert(success,c)
			elseif c:getNumber()>10 then
				table.insert(success,c)
			else
				table.insert(unsuccess,c)
			end
		end
		if self:isFriend(who) and #success>0 then
			return success[1]
		elseif self:isEnemy(who) and #unsuccess>0 then
			return unsuccess[#unsuccess]
		end
	end
	local j = math.random(1, #cards)
	return cards[j]
end




sgs.ai_cardneed.xihua = function(to, card, self)
	return card:getNumber() > 10
end



function sgs.ai_cardsview_valuable.shijie(self, class_name, player)
	if class_name == "Peach" then
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying or not self:isFriend(dying, player) or player:isKongcheng() then return nil end
		local cards = sgs.QList2Table(player:getCards("hs"))
		self:sortByKeepValue(cards)
		return "@ShijieCard="..cards[1]:getId()
	end
end
sgs.ai_skill_playerchosen.shijie = function(self, targets)
	for _,p in sgs.qlist(targets) do
		if self:isEnemy(p) then
			return p
		end
	end
	for _,p in sgs.qlist(targets) do
		if not self:isFriend(p) then
			return p
		end
	end
	return targets:first()
end
sgs.ai_skill_cardchosen.shijie = function(self, who, flags)
	local suit_id = self.player:getTag("shijie_suit"):toString()
	for _,c in sgs.qlist(who:getCards("e")) do
		if c:getSuitString() == suit_id then
			return c
		end
	end
	return who:getCards("e"):first()
end

sgs.ai_skill_invoke.fengshui = function(self,data)
	return true
end

sgs.ai_skill_invoke.fengshui_retrial = function(self,data)
	local id = self.player:getTag("fengshui_id"):toInt()
	self.player:removeTag("fengshui_id")
	local judge=data:toJudge()
	local cards={}
	table.insert(cards,judge.card)
	local ex_id=self:getRetrialCardId(cards, judge)
	--ex_id==-1
	if ex_id ==-1 then
		return true
	end
	return false
end


local leishi_skill = {}
leishi_skill.name = "leishi"
table.insert(sgs.ai_skills, leishi_skill)
leishi_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("LeishiCard") then return nil end
	local slash = sgs.cloneCard("thunder_slash")
	if self.player:isCardLimited(slash, sgs.Card_MethodUse) then return nil end
	return sgs.Card_Parse("@LeishiCard=.")
end
sgs.ai_skill_use_func.LeishiCard = function(card, use, self)
		self:sort(self.enemies,"handcard")
		local slash = sgs.cloneCard("thunder_slash", sgs.Card_NoSuit, 0)
		local targets={}
		for _, p in ipairs(self.enemies) do
			if not p:hasSkill("jingdian") then  
				if not p:isKongcheng() and  self.player:canSlash(p,slash,false) then
					if getCardsNum("Jink", p, self.player) < 1
					or sgs.card_lack[p:objectName()]["Jink"] == 1 or self:isWeak(p)  then
						table.insert(targets,p)
					end
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
		local slash = sgs.cloneCard("thunder_slash")
		local goodf
		for _,p in ipairs(self.friends_noself) do
			if self:isTargetForRedundantSlash(slash, p,self.player) then
				good_f =p
				break
			end
		end
		if good_f then
			use.card = card
			if use.to then
				use.to:append(good_f)
				if use.to:length() >= 1 then return end
			end
		end
end
sgs.dynamic_value.damage_card.LeishiCard = true



sgs.ai_skill_invoke.fenyuan = function(self,data)
	local damage = self.player:getTag("fenyuanDying"):toDying().damage
	if self.player:isLord() then
		return false
	end
	local current = self.room:getCurrent()
	if self:isFriend(current) then
		return false
	end
	local killer = self:findRealKiller(self.player, damage)
	if killer and  killer:hasLordSkill("tymhwuyu") then
		return false
	end

	local can_save = self:getAllPeachNum(self.player)+ self:getCardsNum("Analeptic")+self.player:getHp() >= 1
	if can_save then
		return false
	else
		local fakeDamage=sgs.DamageStruct()
		fakeDamage.nature= sgs.DamageStruct_Thunder
		fakeDamage.damage=2
		fakeDamage.to=current
		local damageEffect = self:touhouNeedAvoidAttack(fakeDamage,nil,current)
		if damageEffect then
			return true
		else
			return not current:hasSkill("jingdian")
		end
	end
end


local xiefa_skill = {}
xiefa_skill.name = "xiefa"
table.insert(sgs.ai_skills, xiefa_skill)
function xiefa_skill.getTurnUseCard(self)
	if self.player:hasUsed("XiefaCard") then return nil end
	local handcards = sgs.QList2Table(self.player:getHandcards())
	if #handcards==0 then return nil end
	self:sortByUseValue(handcards)
	return sgs.Card_Parse("@XiefaCard=" .. handcards[1]:getEffectiveId())
end
sgs.ai_skill_use_func.XiefaCard = function(card, use, self)

		local nojink_targets={}
		local good_targets={}
		local bad_targets={}
		for _, p in ipairs(self.enemies) do
			if getCardsNum("Jink", p, self.player) < 1
				or sgs.card_lack[p:objectName()]["Jink"] == 1 then
				table.insert(nojink_targets,p)
				if self:isWeak(p) then
					table.insert(good_targets,p)
				end
			else
				table.insert(bad_targets,p)
			end
		end

		local attacker
		local victim
		local slash = sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
		local slash_eff

		if  #good_targets>0 then
			for _, p in ipairs(good_targets) do
				for _,a in pairs (self.friends_noself) do
					if  a:inMyAttackRange(p) then  
						if a:canSlash(p,slash,true) then
							if self:slashIsEffective(slash, p, a) then
								local fakeDamage=sgs.DamageStruct()
								fakeDamage.card=slash
								fakeDamage.nature= self:touhouDamageNature(slash,a,p)
								fakeDamage.damage=1
								fakeDamage.from=a
								fakeDamage.to=p
								slash_eff= self:touhouDamage(fakeDamage,a,p).damage>0
							end
							if slash_eff then
								attacker=a
								victim=p
								break
							end
						end
					end
				end
			end
		end

		if not attacker  and #nojink_targets>0 then

			for _, p in ipairs(nojink_targets) do
				for _,a in pairs (self.friends_noself) do
					if  a:inMyAttackRange(p) then  
						if a:canSlash(p,slash,true) then
							attacker=a
							victim=p
							break
						end
					end
				end
			end
		end

		if not attacker  and #bad_targets>0 then
			for _,p in ipairs(bad_targets) do
				for _,a in pairs (self.enemies) do
					if  a:inMyAttackRange(p) then  
					if a:canSlash(p,slash,true) and self:slashIsEffective(slash, p, a) then
						attacker=a
						victim=p
						break
					end
					end
				end
			end
		end
		if attacker and victim then
			use.card = card
			if use.to then
				use.to:append(attacker)
				use.to:append(victim)
				if use.to:length() >= 2 then return end
			end
		end
end



sgs.ai_skill_invoke.chuanbi = function(self,data)
	local slash_source
	local strs=data:toStringList()
	if strs and #strs==2 then
		local str1=(strs[2]:split(":"))[1]
		local str2=(strs[2]:split(":"))[2]
		if str1=="slash-jink" then
			for _,p in sgs.qlist(self.room:getAlivePlayers()) do
				if p:objectName() == str2 then
					slash_source=p
					break
				end
			end
		end
	end
	--if slash_source and self:isFriend(slash_source)
	--and slash_source:getPhase() == sgs.Player_Play
	--and slash_source:hasSkill("sidie")   then
	--	return false
	--end
	return true
end
sgs.ai_slash_prohibit.chuanbi = function(self, from, to, card)
	if not self:isEnemy(from,to) then
		return false
	end
	if self:hasWeiya(to) then
		return false
	end

	local current =self.room:getCurrent()
	if not current or  current:isDead() then
		return false
	end
	if (current:getWeapon() ) then --and current:getMark("@tianyi_Weapon")==0
		return false
	end
	if from:hasWeapon("Axe") and from:getCards("hes"):length()>=2 then
		return false
	end
	return true
end



local duzhua_skill = {}
duzhua_skill.name = "duzhua"
table.insert(sgs.ai_skills, duzhua_skill)
duzhua_skill.getTurnUseCard = function(self, inclusive)

		if self.player:hasFlag("duzhua") then return false end
		local cards = self.player:getCards("hs")
		cards=self:touhouAppendExpandPileToList(self.player,cards)
		cards = sgs.QList2Table(cards)
		if #cards==0 then return false end
		self:sortByUseValue(cards, true)
		cards1={}
		for _,c in pairs (cards) do
			if c:isRed() then
			table.insert(cards1,c)
			end
		end
		if #cards1==0 then return false end
		local red_card=cards1[1]

		if red_card then
			local suit = red_card:getSuitString()
			local number = red_card:getNumberString()
			local card_id = red_card:getEffectiveId()
			local slash_str = ("slash:duzhua[%s:%s]=%d"):format(suit, number, card_id)
			local slash = sgs.Card_Parse(slash_str)

			assert(slash)
			return slash
		end
end
sgs.duzhua_suit_value = {
	heart = 3.9,
	diamond = 3.9
}
sgs.ai_cardneed.duzhua = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  card:isRed()
	end
end

sgs.ai_skill_invoke.taotie =  function(self)
	return self:invokeTouhouJudge()
end
sgs.ai_skillProperty.taotie = function(self)
	return "cause_judge"
end

--[[
sgs.ai_skill_use["@@huisheng"] = function(self, prompt)
	local use=self.room:getTag("huisheng_use"):toCardUse()
	local target = use.from
	local card = use.card
	if not target then return "."    end
	if not card then return "."  end

	local needHuisheng=false
	if self:isFriend(target) then
		if card:isKindOf("Peach") and target:isWounded() then
			needHuisheng= true
		end
	end
	if self:isEnemy(target) then
		if card:isKindOf("Duel") then
			needHuisheng = self:getCardsNum("Slash") >= getCardsNum("Slash", target, self.player)
		end
		if not card:isKindOf("Peach") then
			needHuisheng= true
		end
	end
	if not needHuisheng then return "."  end

	local victim
	if card:isKindOf("Collateral") then
		local others ={}
		for _,p in sgs.qlist(self.room:getOtherPlayers(target)) do
			if self:isEnemy(p) and target:canSlash(p) then
				victim= p
				break
			elseif target:canSlash(p) then
				table.insert(others,p)
			end
		end
		if not victim and #others>0 then
			victim=others[1]
		end
	end

	local targets={}
	table.insert(targets,target:objectName())
	if victim then
		table.insert(targets,victim:objectName())
	end
	return "@HuishengCard=.->" .. table.concat(targets, "+")
end
]]

sgs.ai_skill_invoke.songjing = true
sgs.ai_skill_cardask["@gongzhen"] = function(self, data, pattern, target)
	local damage =data:toDamage()
	if not self:isEnemy(damage.to) then return "." end
	
	local convert = { [".S"] = "spade", [".D"] = "diamond", [".H"] = "heart", [".C"] = "club"}
	local cards = sgs.QList2Table(self.player:getHandcards())
	local card
	self:sortByUseValue(cards, true)
	for _, acard in ipairs(cards) do
		if acard:getSuitString() == convert[pattern]  and not sel.player:isJilei(acard) then
			if not isCard("Peach", acard, self.player) then
				card = acard
				break
			end
		end
	end
	return card and card:getId() or "."
end
sgs.ai_choicemade_filter.cardResponded["@gongzhen"] = function(self, player, args)
	local to = findPlayerByObjectName(self.room, (args[2]:split(":"))[2])
	if to and args[#args] ~= "_nil_" then
		sgs.updateIntention(player, to, 80)
	end
end

sgs.ai_need_bear.chuixue = function(self, card,from,tos)
	from = from or self.player
	if self:getOverflow(from) ~=1 then return false end
	local targets
	if self:isFriend(from) then
		targets=self.enemies
	end
	if self:isEnemy(from) then
		targets=self.friends
	end
	if targets and #targets>0 then
		for _,p in pairs(targets) do
			if p:isKongcheng() and not self:touhouHpLocked(p) then
				return true
			end
		end
	end
	return false
end

sgs.ai_skill_playerchosen.chuixue = function(self, targets)
	target_table =sgs.QList2Table(targets)
	if #target_table==0 then return nil end
	self:sort(target_table,"handcard")
	for _,target in pairs(target_table) do
		if  self:isEnemy(target) and not self:touhouHpLocked(target) then
			return target
		end
	end
	return nil
end


sgs.ai_playerchosen_intention.chuixue = 50

--sgs.ai_card_intention.Slash
--sgs.ai_armor_value.EightDiagram
function SmartAI:lastEnemy(player,target)
	if not self:isEnemy(player,target) then return false end
	for _,p in sgs.qlist(self.room:getOtherPlayers(target)) do
		if not self:isFriend(player,target) then
			return false
		end
	end
	return true
end
sgs.ai_skill_invoke.wushou = true
sgs.ai_skill_discard.wushou = sgs.ai_skill_discard.gamerule
sgs.ai_slash_prohibit.wushou = function(self, from, to, card)
	if self:hasWeiya(to) then
		return false
	end

	if to:getHp()<=3 and to:hasSkill("wushou")
	and self:isEnemy(from,to)  then
		if to:hasArmorEffect("EightDiagram")
		and (not from:hasWeapon("QinggangSword") ) then
				return self:lastEnemy(from,to)
		elseif not self:lastEnemy(from,to) then
			if self:getAllPeachNum(to) > 0 then
				return true
			end
		end
	end
	return false
end
sgs.ai_trick_prohibit.wushou = function(self, from, to, card)
	if not card:isKindOf("Duel")  then return false end
	if self:isFriend(from,to) then return false end
	if self:hasWeiya(to) then return false end
	if not self:lastEnemy(from,to) then
		if self:getAllPeachNum(to) > 0 then
			return true
		end
	end
	if getCardsNum("Slash", from, self.player)> getCardsNum("Slash", to, self.player) then
		return false
	end
	return true
end


local function inBumingRange(bumingType,player,target)
	if bumingType==0 then
		return player:inMyAttackRange(target)
	end
	if bumingType==2 then
		local rangefix =1
		distance=player:distanceTo(target,rangefix)
		return  distance <=player:getAttackRange()
	end
	if bumingType==1 then
		local rangefix =0
		if player:getAttackRange() >player:getAttackRange(false) then
			rangefix = rangefix +player:getAttackRange() - player:getAttackRange(false)
		end
		distance=player:distanceTo(target,rangefix)
		return  distance <=player:getAttackRange()
	end
end
local function findBumingTarget(self,card)
	self:sort(self.enemies,"handcard")
	local rangefix=0
	bumingType=0
	if self.player:getWeapon() and self.player:getWeapon():getId() == card:getId() then
		if self.player:getAttackRange() >self.player:getAttackRange(false) then
			rangefix = rangefix +self.player:getAttackRange() - self.player:getAttackRange(false)
		end
		bumingType=1
	end
	if self.player:getOffensiveHorse() and self.player:getOffensiveHorse():getId() == card:getId() then
		rangefix=rangefix+1
		bumingType=2
	end


	for _, p in ipairs(self.enemies) do

		if inBumingRange(bumingType,self.player,p) then
			--if p:hasSkill("yemang") and bumingType==2 then
			--	rangefix=rangefix+1
			--end
			local slash = sgs.cloneCard("slash", sgs.Card_NoSuit, 0)
			local duel = sgs.cloneCard("duel", sgs.Card_NoSuit, 0)

			if (self.player:canSlash(p,slash,true,rangefix) and not(self.player:isCardLimited(slash, sgs.Card_MethodUse)))
			or ( not self.player:isProhibited(p, duel)and not self.player:isCardLimited(duel, sgs.Card_MethodUse))
			then

				num=self.player:getHandcardNum() - p:getHandcardNum()
				if self:isWeak(p) or num>2 or p:getHandcardNum()<3 then
					return p
				end
			end
		end
	end
	return nil
end

local buming_skill = {}
buming_skill.name = "buming"
table.insert(sgs.ai_skills, buming_skill)
buming_skill.getTurnUseCard = function(self)
	cards =sgs.QList2Table(self.player:getCards("hes"))
	if #cards==0 then return nil end
	self:sortByKeepValue(cards)
	if self.player:hasUsed("BumingCard") then return nil end
	target=findBumingTarget(self,cards[1])
	if not target then return nil end
	_data=sgs.QVariant()
	_data:setValue(target)
	self.player:setTag("buming_target",_data)
	return sgs.Card_Parse("@BumingCard=" .. cards[1]:getId())

end
sgs.ai_skill_use_func.BumingCard=function(card,use,self)
	use.card = card
	p= self.player:getTag("buming_target"):toPlayer()
	if use.to then
		use.to:append(p)
		if use.to:length() >= 1 then return end
	end
end
sgs.ai_skill_choice.buming=function(self)--因为有cardlimit  需要检测实际choice。。。
	cards =sgs.QList2Table(self.player:getCards("hs"))
	local s=0
	local j=0
	for _,card in pairs (cards) do
		if card:isKindOf("Slash") then
			s=s+1
		end
		if card:isKindOf("Jink") then
			j=j+1
		end
	end

	if j<2 and s>1 then
		return "duel_buming"
	end
	if s<2 and j>1 then
		return "slash_buming"
	end
	return "slash_buming"
end

sgs.ai_card_intention.BumingCard = 70

--[[
sgs.ai_skill_playerchosen.zhengti = function(self, targets)
	target_table =sgs.QList2Table(targets)
	for _,p in pairs (target_table) do
		if self:isEnemy(p) then
			return p
		end
	end
	return target_table[1]
end

--sgs.ai_playerchosen_intention.zhengti = 10
sgs.ai_damageInflicted.zhengti =function(self, damage)
	local can =false
	for _,p in sgs.qlist(self.room:getOtherPlayers(damage.to)) do
		if p:getMark("@zhengti")>0 then
			can=true
			break
		end
	end
	if can then
		damage.damage=0
	end
	return damage
end
function SmartAI:zhengtiParse(from,to)

	local weak_friends={}
	local friends={}
	local enemies={}
	local t_enemies={}
	local can_transfer=false
	if not to:hasSkill("zhengti") then return false end
	--if self:isFriend(from,to) then return false end
	for _,p in sgs.qlist(self.room:getOtherPlayers(to)) do
		if self:isEnemy(from,p) then
			table.insert(enemies,p)
		end
		if p:getMark("@zhengti")>0 then
			can_transfer=true
			if self:isEnemy(from,p) then
				table.insert(t_enemies,p)
			end
			if self:isFriend(from,p) then
				table.insert(friends,p)
				if self:isWeak(p) then
					table.insert(weak_friends,p)
				end
			end
		end
	end

	local result = true
	local target = nil
	if #weak_friends>0 then
		result = true
		target = weak_friends[1]
	elseif #enemies==0 then
		result = false
	elseif #friends==0  and  #t_enemies>0 then
		result = false
		target = t_enemies[1]
	end
	if #friends >0 and not target then
		target = friends[1]
	end
	return result, target
end
]]
sgs.ai_skill_invoke.qingyu = true
sgs.ai_skill_cardask["@qingyu-discard"] = function(self, data)
	local source=data:toPlayer()
	if self:isEnemy(source) and self.player:getHandcardNum()>2 then
		cards =sgs.QList2Table(self.player:getCards("hs"))
		self:sortByKeepValue(cards)
		return "$" .. cards[1]:getId()
	end
	return "."
end
sgs.ai_choicemade_filter.cardResponded["@qingyu-discard"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		local target =player:getTag("qingyu_source"):toPlayer()
		if not target then return end
		sgs.updateIntention(player, target, 20)
	end
end
function SmartAI:qingyuNum(player)
	local num =0
	for _,p in sgs.qlist(self.room:getOtherPlayers(player)) do
		if p:getHp()>=player:getHp() then
			num = num+1
		end
	end
	return num
end
sgs.ai_need_damaged.qingyu = function(self, attacker, player)
	if player:isKongcheng() then return false end
	local num = self:qingyuNum(player)
	if player:containsTrick("indulgence") or player:containsTrick("supply_shortage") then
		return false
	end
	if player:getLostHp()==0  then
		return num>=3
	elseif player:getLostHp()==1  then
		return num>=5
	elseif getCardsNum("Peach", player, self.player) >= 1 or getCardsNum("Analeptic", player, self.player) >= 1 then
		return num>=5
	end
	return false
end
sgs.ai_slash_prohibit.qingyu = function(self, from, to, card)
	if self:isFriend(from,to) then
		return false
	end
	local callback=sgs.ai_damage_prohibit["qingyu"]
	local damage = sgs.DamageStruct(card, from, to, 1, self:touhouDamageNature(card,from,to))
	return callback(self, from, to, damage)
end
sgs.ai_trick_prohibit.qingyu = function(self, from, to, card)
	if self:isFriend(from,to) then return false end
	local isDamage=false
	if ( card:isKindOf("Duel") or card:isKindOf("AOE") or card:isKindOf("FireAttack")
			or sgs.dynamic_value.damage_card[card:getClassName()]) then
		isDamage=true
	end
	if isDamage then
		local callback=sgs.ai_damage_prohibit["qingyu"]
		local damage = sgs.DamageStruct(card, from, to, 1, self:touhouDamageNature(card,from,to))
		return callback(self, from, to, damage)
	end
	return false
end
sgs.ai_damage_prohibit.qingyu = function(self, from, to, damage)

	if self:isFriend(from,to) then return false end
	local num = self:qingyuNum(to)
	if num < 5 then return false end
	local effect, willEffect = self:touhouDamageEffect(damage,from,to)
	if effect then
		return false
	end
	return true
end



sgs.ai_skill_invoke.guoke = true
sgs.ai_skill_choice.guoke= function(self, choices, data)
	--player:isSkipped(sgs.Player_Play)
	--self:willSkipPlayPhase()
	if self.player:isWounded() and (self:willSkipPlayPhase() or self.player:getPhase()==sgs.Player_Judge)  then
		return "recover"
	end
	return "draw"
end
sgs.ai_trick_prohibit.guoke = function(self, from, to, card)
	if card:isKindOf("DelayedTrick")  then
		if self:isFriend(from,to) then return false end
		if card:isKindOf("Indulgence") and from:getPhase() == sgs.Player_Play then
			if to:getHandcardNum() >= 6 then
				return false
			end
		end
		return true
	end
	return false
end