--伊吹萃香
--[醉月]
--酒貌似是使用就根本就不过useBasicCard。。。。不能通过turnuse插入card列表。。。
--而是在使用杀的usecard function里调用的 getCardId。。。这个函数丝毫不用到getTurnUse。。。

sgs.ai_cardneed.zuiyue = function(to, card, self)
	return card:isKindOf("Slash")  or card:isKindOf("TrickCard")
end

--[斗酒]
function SmartAI:cautionDoujiu(player,card)
	player = player or self.player
	--if player:getPhase() ~=sgs.Player_Play then
	--	return false
	--end
	local  zhan006 = self.room:findPlayerBySkillName("doujiu")
	if not zhan006
	or not self:isEnemy(zhan006)
	or zhan006:isKongcheng() then

		return false
	end
	local enemy_card=self:getMaxCard(zhan006)
	local f_card=self:getMaxCard()
	local enemy_number=0
	local f_number=0
	if enemy_card then
		enemy_number=enemy_card:getNumber()
	end
	if f_card then
		f_number=f_card:getNumber()
	end
	if f_number<7 then
		if self:getOverflow(player)>0 then
			return self:touhouIsPriorUseOtherCard(player,card)
		else
			return true
		end
	else
		return enemy_number >f_number
	end
	return false
end
sgs.ai_skill_invoke.doujiu =function(self,data)
	local target= data:toPlayer()
	if self:isEnemy(target) then
		if not self.player:isKongcheng() then
			self.doujiu_card=  self:getMaxCard():getEffectiveId()
		end
		return true
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.doujiu = function(self, player, args, data)
	local target =data:toPlayer()
	if target then
		if args[#args] == "yes" then
			sgs.updateIntention(player, target, 50)
		end
	end
end
function sgs.ai_skill_pindian.doujiu(minusecard, self, requester, maxcard)
	return self:getMaxCard()
end
sgs.ai_cardneed.doujiu = function(to, card, self)
	return card:getNumber() > 10
end

--[宴会]
local yanhuivs_skill = {}
yanhuivs_skill.name = "yanhui_attach"
table.insert(sgs.ai_skills, yanhuivs_skill)
function yanhuivs_skill.getTurnUseCard(self)
		if self.player:isKongcheng() then return nil end
		if self.player:getKingdom() ~="zhan" then return nil end
		local handcards = {}
		for _,c in sgs.qlist(self.player:getCards("hs")) do
			if c:isKindOf("Peach") or c:isKindOf("Analeptic") then
				table.insert(handcards, c)
			end
		end
		if #handcards  ==0 then return nil end
		self:sortByUseValue(handcards)

		return sgs.Card_Parse("@YanhuiCard=" .. handcards[1]:getEffectiveId())
end

sgs.ai_skill_use_func.YanhuiCard = function(card, use, self)
	local targets = {}
	for _,friend in ipairs(self.friends_noself) do
		if friend:hasLordSkill("yanhui") and friend:isWounded() then
			table.insert(targets, friend)
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


--四季映姬·夜摩仙那度
--[审判]
sgs.ai_skill_playerchosen.shenpan = function(self, targets)
	--需要check damage prohibit
	local target_table = self:getEnemies(self.player)
	if #target_table==0 then return nil end
	self:sort(target_table, "hp")
	local shenpan_targets={}
	local weak_targets={}
	local chain_targets={}
	local good_friends={}
	local x,y
	for _,target in pairs(target_table) do
		if  self:damageIsEffective(target, sgs.DamageStruct_Thunder, self.player) then
			local damage=sgs.DamageStruct("shenpan", self.player, target, 1, sgs.DamageStruct_Thunder)

			local final_damage=self:touhouDamage(damage,self.player,target)
			if not (final_damage.damage <=0) then
				x,y=self:touhouChainDamage(damage,self.player,target)
				if x>y then
					table.insert(chain_targets,target)
				end
				if self:isWeak(target)  then
					table.insert(weak_targets,target)
				end
				final_hp= math.max(0,target:getHp()-final_damage.damage)
				if target:getHandcardNum()>final_hp  then
					table.insert(shenpan_targets,target)
				end
			end
		end
	end
	for _,p in pairs (self.friends_noself)do
		if p:hasSkill("jingdian") then
			table.insert(good_friends,p)
		end
	end
	need_shenpan=true
	if self:isWeak(self.player) and self.player:getHandcardNum()<3
	and not self:willSkipPlayPhase(self.player)
	and #weak_targets==0 then
		need_shenpan=false
	end
	if need_shenpan then
		if #weak_targets>0 then
			if #chain_targets>0 then
				for _,p in pairs(weak_targets) do
					if p:isChained() then return p end
				end
			else
				return weak_targets[1]
			end
		end
		if #chain_targets>0 then
			if #shenpan_targets>0 then
				for _,p in pairs(shenpan_targets) do
					if p:isChained() then return p end
				end
			else
				return chain_targets[1]
			end
		end
		if #shenpan_targets>0 then
			self:sort(shenpan_targets, "hp")
			return shenpan_targets[1]
		end
		if #good_friends>0 then
			return good_friends[1]
		end
	end
	return nil
end
sgs.ai_skill_invoke.shenpan =true
sgs.ai_playerchosen_intention.shenpan =function(self, from, to)
	local intention = 0
	if to:hasSkill("jingdian") then
		intention = -30
	end
	sgs.updateIntention(from, to, intention)
end

--[悔悟]
function huiwu_judge(self,owner,card, invoker)
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
	if card:isKindOf("IronChain") then
		if target:isChained() then
			return 2
		else
			return 1
		end
	end
	if   card:isKindOf("Dismantlement") or card:isKindOf("Snatch") then
		if self:isFriend(owner, invoker) and owner:getCards("j"):length()>0 then
			return 2
		end
		if self:isEnemy(owner, invoker) and owner:getCards("j"):length()>0  and owner:isNude() then
			return 2
		end
		return 1
	end
	return 1
end
sgs.ai_skill_invoke.huiwu =function(self,data)
	local owner=self.player:getTag("huiwu_owner"):toPlayer()
	local card=data:toCardUse().card
	if not owner then return false end
	return self:isFriend(owner)
end

-- 先注释掉，这逻辑。。。。
sgs.ai_choicemade_filter.skillInvoke.huiwu = function(self, player, args, data)
	local card=data:toCardUse().card
	local to=player:getTag("huiwu_owner"):toPlayer()
	res=huiwu_judge(self,to,card)
	
	if args[#args] == "yes" then
		sgs.updateIntention(player, to, -20)
	else
		sgs.updateIntention(player, to, 20)
	end
end
sgs.ai_benefitBySlashed.huiwu = function(self, card,source,target)
	return true
end
sgs.ai_skill_invoke.huiwu_nullify = function(self, data)
	local use = data:toCardUse()
	local card= use.card
	return huiwu_judge(self, self.player, card, use.from) == 1
end

--[花冢]
sgs.ai_skill_invoke.huazhong = function(self, data)
	if not self:invokeTouhouJudge() then return false end
	local to =data:toPlayer()
	return self:isFriend(to)
end
sgs.ai_choicemade_filter.skillInvoke.huazhong = function(self, player, args, data)
	local to=data:toPlayer()
	if to then
		if args[#args] == "yes" then
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


--小野塚小町
--[冥途]
sgs.ai_skill_invoke.mingtu = true
--[薄命]
sgs.ai_skill_invoke.boming =  true
sgs.ai_damageCaused.boming = function(self, damage)
	if damage.card and not damage.chain and not damage.transfer then
		if  damage.card:isKindOf("Slash") and  damage.to:getHp() <= damage.to:dyingThreshold() then
			damage.damage=damage.damage+2
		end
	end
	return damage
end
sgs.ai_cardneed.boming = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return getCardsNum("Slash", to, self.player) <1
		and card:isKindOf("Slash")
	end
end

--风见幽香
--[威压]
sgs.ai_skill_invoke.weiya =  function(self,data)
	--AOE情况下，更复杂的判断情况，放弃了
	local use =data:toCardUse()
	if use and use.from and self:isFriend(use.from) then return false end
	local resp = data:toCardResponse()
	if resp and resp.m_from and self:isFriend(resp.m_from) then return false end
	return true
end
function SmartAI:hasWeiya(player)
	player=player or self.player
	local current=self.room:getCurrent()
	if current and current:isAlive() and current:hasSkill("weiya") then
		if player:objectName()== current:objectName() then
			return false
		else
			return true
		end
	end
	return false
end

sgs.ai_skill_cardask["@weiya"] = function(self, data, pattern, target)
	local Pattern2ClassName = {
		["slash"] = "Slash", ["jink"] = "Jink",
		["peach"] = "Peach", ["analeptic"] = "Analeptic",
		["nullification"] = "Nullification"
	}
	if Pattern2ClassName[pattern] then
		local cards = self.player:getHandcards()
		for _, card in sgs.qlist(cards) do
			if card:isKindOf(Pattern2ClassName[pattern]) then
				return "$" .. card:getEffectiveId()
			end
		end
	end
	return "."
end


--梅蒂欣·梅兰可莉
--[剧毒]
sgs.ai_skill_invoke.judu =function(self,data)
	if not self:invokeTouhouJudge() then return false end
	local target=data:toPlayer()
	if self:isEnemy(target) then
		return true
	end
end
sgs.ai_choicemade_filter.skillInvoke.judu = function(self, player, args, data)
	local target = data:toPlayer()
	if args[#args] == "yes" then
		sgs.updateIntention(player, target, 70)
	end
end
sgs.ai_skillProperty.judu = function(self)
	return "cause_judge"
end

--[恨意]
sgs.ai_skill_invoke.henyi =function(self,data)
	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	local card = sgs.cloneCard("archery_attack", sgs.Card_NoSuit, 0)
	card:setSkillName("henyi")
	card:deleteLater()
	self:useTrickCard(card, dummy_use)
	if  dummy_use.card then return true end
	return false
end

--文花帖SP文
--[偷拍]
function SmartAI:toupaiValue(player)
	if self:touhouHandCardsFix(player) then
		return 0
	end

	local value=0
	if player:hasSkills("xisan|yongheng|kongpiao") then
		value= - 20
	end
	for _, card in sgs.qlist(player:getHandcards()) do
		local flag = string.format("%s_%s_%s", "visible", global_room:getCurrent():objectName(), player:objectName())
		if  card:hasFlag("visible") or card:hasFlag(flag) then
			if card:isKindOf("BasicCard")  then
					value=value+10
				if card:isRed() and player:hasSkill("qucai") then
					value=value+20
				end
				if card:isKindOf("Peach") then
					value=value+10
				end
			end
		else
			value=value+5
		end
	end
	return value
end
sgs.ai_skill_use["@@toupai"] = function(self, prompt)
	self:sort(self.enemies,"handcard")
	self.enemies = sgs.reverse(self.enemies)
	local enemies = self.enemies
	for _,p in pairs(self.enemies) do
		if p:isKongcheng() or self:touhouHandCardsFix(p)  then
			table.removeOne(enemies, p)
		end
	end
	if #enemies==0 then return "." end
	enemy_table={}
	for _,e in pairs (enemies) do
		local array={player=e, value=self:toupaiValue(e)}
		table.insert(enemy_table,array)
	end
	local compare_func = function(a, b)
		return a.value > b.value
	end
	table.sort(enemy_table, compare_func)
	local target_objectname = {}
	for _, t in ipairs(enemy_table) do
		if t.value < 15 and #target_objectname == 0 then return "." end
		table.insert(target_objectname, t.player:objectName())
		if #target_objectname >=2 then break end
	end
	if #target_objectname >=0 then
		return "@ToupaiCard=.->" .. table.concat(target_objectname, "+")
	end
	return "."
end
sgs.ai_card_intention.ToupaiCard = 50
--[取材]
sgs.ai_skill_invoke.qucai = true

--比那名居天子
--[绯想]
sgs.ai_skill_playerchosen.feixiang = function(self, targets)
	local judge=self.player:getTag("feixiang_judge"):toJudge()
	local cards={}
	table.insert(cards,judge.card)
	local ex_id = self:getRetrialCardId(cards, judge)

	local judgecard_value = self:getUseValue(judge.card)
	--ex_id 不为-1 则代表 此牌作为判定牌生效的话，对天子而言是个好结果
	local retrial_targets={}
	for _,target in sgs.qlist(targets) do
        	local flag = "es"
        	if target:objectName() == self.player:objectName() then flag = "hes" end
		local cards1 = sgs.QList2Table(target:getCards(flag))
		local self_card =  target:objectName()== self.player:objectName()
		local new_id = self:getRetrialCardId(cards1, judge,self_card)
		--new_id 不为-1 代表 装备区的id去改判，可以得到好结果
        	if new_id == -1 then continue end
		--牌的基础使用价值
        	local new_value = self:getUseValue(sgs.Sanguosha:getCard(new_id))
        	local diff = judgecard_value - new_value
        	if not self:isEnemy(target) then
			diff = 0 - diff
        	end
                
        	--改判结果的价值修正
        	if ex_id == -1 and new_id ~= -1 then
			diff = diff + 15
        	elseif  new_id == -1 then
			if ex_id ~= -1 then
              			diff = diff - 15
			else
				diff = diff - 3
			end
        	end
        --还需要计算拔装备的价值？

		local array={player= target, value= diff}
		table.insert(retrial_targets,array)
	end
	
	
	local compare_func = function(a, b)
		return a.value > b.value
	end

    if #retrial_targets>0 then
		table.sort(retrial_targets, compare_func)
        for _, t in ipairs(retrial_targets) do
			if t.value >= 5 then return t.player end
        end
    end
	return nil
end

sgs.ai_skill_cardchosen.feixiang = function(self, who, flags)
	local flag = "es"
	local judge=self.player:getTag("feixiang_judge"):toJudge()
	local cards={}
	table.insert(cards,judge.card)
	--local ex_id = self:getRetrialCardId(cards, judge)
	
	if who:objectName() == self.player:objectName() or self.player:hasSkill("duxin") then
		flag  = "hes"
	end
	local cards1 = sgs.QList2Table(who:getCards(flag))
	local self_card =  who:objectName()== self.player:objectName()
	local new_id=self:getRetrialCardId(cards1, judge, self_card)
	if new_id ~= -1 then
		return sgs.Sanguosha:getCard(new_id)
	end

	return cards1[math.random(1, #cards1)]
end

sgs.ai_slash_prohibit.feixiang = function(self, from, to, card)
	if to:hasArmorEffect("EightDiagram")
		and (not from:hasWeapon("QinggangSword")) then
		if self:isEnemy(from,to) then
			local tianzi=self.room:findPlayerBySkillName("feixiang")
			if tianzi and self:isEnemy(from,tianzi) and #self.enemies>1 then
				return  true
			end
		end
	end

	return false
end
sgs.ai_skillProperty.feixiang = function(self)
	return "wizard_harm"
end

--[地震]
sgs.ai_skill_invoke.dizhen  = true


--[天人]
table.insert(sgs.ai_global_flags, "tianrenslashsource")
table.insert(sgs.ai_global_flags, "tianrenjinksource")
local tianren_slash_filter = function(self, player, carduse)
	if not carduse then self.room:writeToConsole(debug.traceback()) end
	if carduse.card:isKindOf("TianrenCard") then
		sgs.tianrenslashsource = player
	else
		sgs.tianrenslashsource = nil
	end
end
table.insert(sgs.ai_choicemade_filter.cardUsed, tianren_slash_filter)
sgs.ai_skill_invoke.tianren = function(self,data)
	local pattern = data:toStringList()[1]

	if pattern=="slash" then
		if not self.player:isLord() then return end
		if sgs.tianrenslashsource then return false end
		local asked = data:toStringList()
		local prompt = asked[2]
		if self:askForCard("slash", prompt, 1) == "." then return false end

		local current = self.room:getCurrent()
		if self:isFriend(current) and current:getKingdom() == "zhan" and self:getOverflow(current) > 2 and not self:hasCrossbowEffect(current) then
			return true
		end

		local cards = self.player:getHandcards()
		for _, card in sgs.qlist(cards) do
			if isCard("Slash", card, self.player) then
				return false
			end
		end

		local lieges = self.room:getLieges("zhan", self.player)
		if lieges:isEmpty() then return false end
		local has_friend = false
		for _, p in sgs.qlist(lieges) do
			if not self:isEnemy(p) then
				has_friend = true
				break
			end
		end
		return has_friend
	end

	if pattern=="jink" then
		local asked = data:toStringList()
		local prompt = asked[2]
		if self:askForCard("jink", prompt, 1) == "." then
			return false
		end
		local cards = self.player:getHandcards()
		if sgs.tianrenjinksource then return false end
		for _, friend in ipairs(self.friends_noself) do
			if friend:getKingdom() == "zhan" and self:hasEightDiagramEffect(friend) then return true end
		end

		local current = self.room:getCurrent()
		if self:isFriend(current) and current:getKingdom() == "zhan" and self:getOverflow(current) > 2 then
			return true
		end

		for _, card in sgs.qlist(cards) do
			if isCard("Jink", card, self.player) then
				return false
			end
		end
		local lieges = self.room:getLieges("zhan", self.player)
		if lieges:isEmpty() then return end
		local has_friend = false
		for _, p in sgs.qlist(lieges) do
			if self:isFriend(p) or sgs.evaluatePlayerRole(p) == "neutral" then
				has_friend = true
				break
			end
		end
		return has_friend
	end


	return false
end
sgs.ai_choicemade_filter.skillInvoke.tianren = function(self, player, args)
	if args[#args] == "yes" then
		if (self.room:getTag("tianren_slash"):toBool()) then
			sgs.tianrenslashsource = player
		else
			sgs.tianrenjinksource = player
		end
	end
end
sgs.ai_skill_use_func.TianrenCard = function(card, use, self)
	self:sort(self.enemies, "defenseSlash")

	if not sgs.tianrentarget then table.insert(sgs.ai_global_flags, "tianrentarget") end
	sgs.tianrentarget = {}

	local dummy_use = { isDummy = true }
	dummy_use.to = sgs.SPlayerList()
	if self.player:hasFlag("slashTargetFix") then
		for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if p:hasFlag("SlashAssignee") then
				dummy_use.to:append(p)
			end
		end
	end
	local slash = sgs.cloneCard("slash")
	self:useCardSlash(slash, dummy_use)
	if dummy_use.card and dummy_use.to:length() > 0 then
		use.card = card
		for _, p in sgs.qlist(dummy_use.to) do
			table.insert(sgs.tianrentarget, p)
			if use.to then use.to:append(p) end
		end
	end
end
sgs.ai_choicemade_filter.cardResponded["@tianren-slash"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		sgs.updateIntention(player, sgs.tianrenslashsource, -40)
		sgs.tianrenslashsource = nil
		sgs.tianrentarget = nil
	elseif sgs.tianrenslashsource and player:objectName() == player:getRoom():getLieges("zhan", sgs.tianrenslashsource):last():objectName() then
		sgs.tianrenslashsource = nil
		sgs.tianrentarget = nil
	end
end
sgs.ai_skill_cardask["@tianren-slash"] = function(self, data)
	if not sgs.tianrenslashsource or not self:isFriend(sgs.tianrenslashsource) then return "." end

	local tianrentargets = {}
	for _, player in sgs.qlist(self.room:getAllPlayers()) do
		if player:hasFlag("tianrenTarget") then
			if self:isFriend(player) and not (self:needToLoseHp(player, sgs.tianrenslashsource, true) or self:getDamagedEffects(player, sgs.tianrenslashsource, true)) then return "." end
			table.insert(tianrentargets, player)
		end
	end

	if #tianrentargets == 0 then
		return self:getCardId("Slash") or "."
	end

	self:sort(tianrentargets, "defenseSlash")
	local slashes = self:getCards("Slash")
	for _, slash in ipairs(slashes) do
		for _, target in ipairs(tianrentargets) do
			if not self:slashProhibit(slash, target, sgs.tianrenslashsource) and self:slashIsEffective(slash, target, sgs.tianrenslashsource) then
				return slash:toString()
			end
		end
	end
	return "."
end
function sgs.ai_cardsview_valuable.tianren(self, class_name, player, need_lord)
	if self:touhouClassMatch(class_name, "Slash") and sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE
		and not player:hasFlag("Global_tianrenFailed") and (need_lord == false or player:hasLordSkill("tianren")) then
		local current = self.room:getCurrent()
		if current:getKingdom() == "zhan" and self:getOverflow(current) > 2 and not self:hasCrossbowEffect(current) then
			self.player:setFlags("stack_overflow_jijiang")
			local isfriend = self:isFriend(current, player)
			self.player:setFlags("-stack_overflow_jijiang")
			if isfriend then return "@TianrenCard=." end
		end

		local cards = player:getHandcards()
		for _, card in sgs.qlist(cards) do
			if isCard("Slash", card, player) then return end
		end

		local lieges = self.room:getLieges("zhan", player)
		if lieges:isEmpty() then return end
		local has_friend = false
		for _, p in sgs.qlist(lieges) do
			self.player:setFlags("stack_overflow_jijiang")
			has_friend = self:isFriend(p, player)
			self.player:setFlags("-stack_overflow_jijiang")
			if has_friend then break end
		end
		if has_friend then return "@TianrenCard=." end
	end
end
sgs.ai_choicemade_filter.cardResponded["@tianren-jink"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		sgs.updateIntention(player, sgs.tianrenjinksource, -80)
		sgs.tianrenjinksource = nil
	elseif sgs.tianrenjinksource then
		local lieges = player:getRoom():getLieges("zhan", sgs.tianrenjinksource)
		if lieges and not lieges:isEmpty() then
			if player:objectName() == lieges:last():objectName() then
				sgs.tianrenjinksource = nil
			end
		end
	end
end
sgs.ai_skill_cardask["@tianren-jink"] = function(self)
	if not self.room:getLord() then return "." end
	local yuanshu = self.room:findPlayerBySkillName("weidi")
	if not sgs.tianrenjinksource and not yuanshu then sgs.tianrenjinksource = self.room:getLord() end
	if not sgs.tianrenjinksource then return "." end
	if not self:isFriend(sgs.tianrenjinksource) then return "." end
	return self:getCardId("Jink") or "."
end

--永江衣玖
--视为雷杀 懒了
sgs.ai_skill_invoke.leiyu = true

sgs.ai_damageInflicted.leiyu = function(self, damage)
	if damage.nature == sgs.DamageStruct_Thunder and damage.to:hasSkill("leiyu") then
			damage.damage=0
	end
	return damage
end
sgs.ai_benefitBySlashed.leiyu = function(self, card,source,target)
	if not card:isKindOf("ThunderSlash")  then return false end
	return true
end

local shizai_skill = {}
shizai_skill.name = "shizai"
table.insert(sgs.ai_skills, shizai_skill)
shizai_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("ShizaiCard") then
		return sgs.Card_Parse("@ShizaiCard=.")
	end
end
sgs.ai_skill_use_func.ShizaiCard = function(card, use, self)
	use.card = card
end
sgs.ai_use_priority.ShizaiCard = 4294967295

sgs.ai_skill_playerchosen.shizai = function(self, targets)
	local enemies = {}
	local friends = {}
	
	targets = sgs.QList2Table(targets)
	for _, t in ipairs(targets) do
		if self:isEnemy(t) then
			table.insert(enemies, t)
		else
			table.insert(friends, t)
		end
	end
	
	if #enemies > 0 then
		self:sort(enemies)
		return enemies[1]
	elseif #friends > 0 then
		self:sort(friends, "defense")
		return friends[1]
	end
	
	return targets[1]
end

--姬海棠果
--[快照]
function SmartAI:kuaizhaoValue(player)
	if player:isKongcheng() then return 0 end
	local value=0
	for _, card in sgs.qlist(player:getHandcards()) do
		local flag = string.format("%s_%s_%s", "visible", global_room:getCurrent():objectName(), player:objectName())
		if  card:hasFlag("visible") or card:hasFlag(flag) then
			if card:isKindOf("BasicCard") then
				value=value+10
			end
		else
			value=value+5
		end
	end
	return value
end

sgs.ai_skill_playerchosen.kuaizhao = function(self, targets)
	self:sort(self.enemies,"handcard")
	local enemies = self.enemies
	for _,p in pairs(self.enemies) do
		if not self.player:inMyAttackRange(p)
		or p:isKongcheng() then
			table.removeOne(enemies, p)
		end
	end

	if #enemies==0 then return nil end
	local compare_func = function(a, b)
		return self:kuaizhaoValue(a) > self:kuaizhaoValue(b)
	end
	table.sort(enemies, compare_func)
	if enemies[1] and self:kuaizhaoValue(enemies[1])>=15 then
		return enemies[1]
	end
	return nil
end
sgs.ai_playerchosen_intention.kuaizhao = 80

--[快照 国]
sgs.ai_skill_invoke.kuaizhao_hegemony = true
local kuaizhao_hegemony_skill = {}
kuaizhao_hegemony_skill.name = "kuaizhao_hegemony"
table.insert(sgs.ai_skills, kuaizhao_hegemony_skill)
function kuaizhao_hegemony_skill.getTurnUseCard(self)
	if self.player:hasUsed("KuaizhaoHegemonyCard") then return nil end
	return sgs.Card_Parse("@KuaizhaoHegemonyCard=.")
end
sgs.ai_skill_use_func.KuaizhaoHegemonyCard = function(card, use, self)
	self:sort(self.friends, "handcard")
	local to1, to2
	to1 = self.player --使用者锁定为自己。
	local dummy_use = { isDummy = true, to = sgs.SPlayerList()}
	local dummy_card=sgs.cloneCard("known_both_hegemony", sgs.Card_NoSuit, 0)
	dummy_card:setSkillName("_kuaizhao_hegemony")
	dummy_card:deleteLater()
	self:useTrickCard(dummy_card, dummy_use)
	
	if not dummy_use.card or dummy_use.to:isEmpty() then
		local targets = self.room:getOtherPlayers(self.player)
		local r = math.random(1, targets:length())
		targets=sgs.QList2Table(targets)
		to2 = targets[r]
		
	else
		to2 = dummy_use.to:first()
	end
	
	if to1 and to2 then
		use.card = card
		if use.to then
			use.to:append(to1)
			use.to:append(to2)
			if use.to:length() >= 2 then return end
		end
	end
end
sgs.ai_use_priority.KuaizhaoHegemonyCard = sgs.ai_use_priority.KnownBothHegemony

--秦心
--[能舞]
sgs.ai_skill_playerchosen.nengwudraw = function(self, targets)
	local target =self:touhouFindPlayerToDraw(true, 1, targets)
	if target then return target end
	return nil
end
sgs.ai_skill_playerchosen.nengwudiscard = function(self, targets)
	targets=sgs.QList2Table(targets)
	self:sort(targets,"handcard")
	for _,p in pairs(targets)do
		if self:isEnemy(p) and not self:touhouHandCardsFix(p) then
			return p
		end
	end
	return nil
end
sgs.ai_playerchosen_intention.nengwudraw = -40
sgs.ai_playerchosen_intention.nengwudiscard = 40
--[[sgs.ai_cardneed.nengwu = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  (not to:getWeapon() and  getCardsNum("Weapon",to,self.player)<1 and card:isKindOf("Weapon"))
		or (not to:getOffensiveHorse() and  getCardsNum("OffensiveHorse",to,self.player)<1 and card:isKindOf("OffensiveHorse"))
	end
end]]
--[希望]
sgs.ai_skill_cardask["@xiwang"] = function(self, data)
	local target = self.player:getTag("xiwang_target"):toPlayer()
	if not target or not self:isFriend(target) then return "." end

	local cards = self.player:getCards("hs")
	cards = sgs.QList2Table(cards)
	if #cards==0 then return "." end
	self:sortByUseValue(cards)
	return "$" .. cards[1]:getId()
end

sgs.ai_choicemade_filter.cardResponded["@xiwang"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		local target =player:getTag("xiwang_target"):toPlayer()
		if not target then return end
		sgs.updateIntention(player, target, -80)
	end
end

--宇佐见堇子
--[念力]
function SmartAI:nianliColor(cards)
	local black = 0
	local red = 0
	for _,c in pairs (cards) do
		if c:isBlack() then
			black = black +1
		else
			red = red + 1
		end
	end
	if black > red then
		return sgs.Card_Black
	else
		return sgs.Card_Red
	end
end

--念力: 使用念力技能卡
local nianli_skill = {}
nianli_skill.name = "nianli"
table.insert(sgs.ai_skills, nianli_skill)
nianli_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("NianliCard") then return nil end
	if self.player:getPhase() ~= sgs.Player_Play then return nil end


	local nianliCards = {}
	local nianli = "slash|snatch"
	local nianlis = nianli:split("|")
	for i = 1, #nianlis do
		local forbidden = nianlis[i]
		local forbid = sgs.cloneCard(forbidden)
		if not self.player:isLocked(forbid)  then --and self:canUseXihuaCard(forbid, true)
			table.insert(nianliCards,forbid)
		end
	end

	self:sortByUseValue(nianliCards, false)
	for _,nianliCard in pairs (nianliCards) do
		local dummyuse = { isDummy = true }
		if nianliCard:isKindOf("BasicCard") then
			self:useBasicCard(nianliCard, dummyuse)
		else
			self:useTrickCard(nianliCard, dummyuse)
		end
		if dummyuse.card then
			fakeCard = sgs.Card_Parse("@NianliCard=.:" .. nianliCard:objectName())
			return fakeCard
		end
	end
	return nil
end
--念力: 念力技能卡选择目标
sgs.ai_skill_use_func.NianliCard=function(card,use,self)
	local userstring=card:toString()
	userstring=(userstring:split(":"))[3]
	local nianlicard=sgs.cloneCard(userstring)
	nianlicard:setSkillName("nianli")
	if nianlicard:getTypeId() == sgs.Card_TypeBasic then self:useBasicCard(nianlicard, use)
	else
		assert(nianlicard)
		self:useTrickCard(nianlicard, use)
	end
	if not use.card then return end
	use.card=card
end


sgs.ai_use_priority.NianliCard = 9

--[深秘] 暂无想法
-- guanxing-ai.lua 里面有默认的代码，暂时先用着吧

--密封梦魇堇子

local mengxiangJudge = function(self, player)
	return self:isEnemy(player) and (not player:isKongcheng())
end

sgs.ai_skill_invoke.mengxiang = function(self)
	if self.player:getHandcardNum() <= self.player:getMaxCards() then return false end
	
	local others, enemies, enemy = self.room:getOtherPlayers(self.player), {}
	for _, enemy in sgs.qlist(others) do
		if mengxiangJudge(self, enemy) then table.insert(enemies, enemy) end
	end
	
	if self.player:getHandcardNum() > self.player:getMaxCards() + #enemies then return false end
	
	-- tentative
	return true
end

sgs.ai_skill_use["@@mengxiang-card1"] = function(self)
	local others, enemies, enemy = self.room:getOtherPlayers(self.player), {}
	for _, enemy in sgs.qlist(others) do
		if mengxiangJudge(self, enemy) then table.insert(enemies, enemy) end
	end
	
	self:sort(enemies, "threat")
	
	local enemyNames = {}
	for _ = 1, self.player:getMark("mengxiang") do
		table.insert(enemyNames, enemies[_]:objectName())
	end
	
	return "@MengxiangTargetCard=.->" .. table.concat(enemyNames, "+")
end

sgs.ai_skill_use["@@mengxiang-card2"] = function(self)
	local targetPlayer, t
	for _, t in sgs.qlist(self.room:getAllPlayers()) do
		if t:hasFlag("mengxiangtarget") then targetPlayer = t break end
	end
	
	if not targetPlayer then return "." end
	local cards = sgs.QList2Table(targetPlayer:getHandcards())
	self:sortByUseValue(cards)
	local c
	for _, c in ipairs(cards) do
		local use = {isDummy = true, to = sgs.SPlayerList()}
		self:useCardByClassName(c, use)
		if use.card then
			local targets = {}
			for _, t in sgs.qlist(use.to) do
				table.insert(targets, t:objectName())
			end
			-- we don't use MengxiangCard since it cause trouble.
			local mes = sgs.LogMessage()
			mes.type = "$mengxiang"
			mes.from = self.player
			mes.to:append(targetPlayer)
			mes.arg = "mengxiang"
			mes.card_str = use.card:toString()
			self.room:sendLog(mes)
			return tostring(use.card:getId()) .. "->" .. table.concat(targets, "+")
		end
	end
	
	-- 亏炸！
	return "."
end

sgs.ai_skill_invoke.jishi = true

sgs.ai_skill_use["@@jishi"] = function(self)
	local list = sgs.QList2Table(self.player:getTag("jishi_tempcards"):toIntList())
	-- 相当无脑了
	return "@JishiCard:" .. table.concat(list, "+") .. "->."
end

--依神女苑&依神紫苑
--[俭奢]
--俭奢:其他角色的结束阶段开始时，你可以弃置一张手牌，令其选择一项：将手牌弃置至一张，若如此做，其回复1点体力；或摸一张牌，然后失去1点体力。
--俭奢: 暂时无脑弃牌发动
sgs.ai_skill_cardask["@jianshe-discard"] = function(self, data)
	if not self.player:canDiscard(self.player, "hes") then return "." end
	local target = self.room:getCurrent()
	if not target or target:isDead() then return "." end
	if not self:isEnemy(target) then return "." end


	local cards = sgs.QList2Table(self.player:getCards("hes"))
	if #cards ==0 then return "." end
	self:sortByCardNeed(cards)

	return "$" .. cards[1]:getId()
end
sgs.ai_choicemade_filter.cardResponded["@jianshe-discard"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		local target =self.room:getCurrent()
		if not target then return end
		sgs.updateIntention(player, target, 30)
	end
end


--[[sgs.ai_skill_invoke.jianshe = function(self, data)
	local target = self.room:getCurrent()
	if not target or target:isDead() then return false end
	local effect=false
	local num =  target:getHandcardNum() - 1
	
	if self:isEnemy(target) and (num > 2 or num <=0) then
		effect=true
	elseif self:isFriend(target) and num <= 2 and num > 0  and target:isWounded() then
		effect=true
	end
	return effect 
end
]]

--俭奢: 执行选项
--[[sgs.ai_skill_choice.jianshe = function(self, choices, data)
	if self:isWeak(self.player) then
		return "jianshe_jian"
	end	
	local num =  self.player:getHandcardNum() - 1  
	
	if num > 2 then
		return "jianshe_she"
	else
		return "jianshe_jian"
	end
	return choices[1]
end]]

--俭奢: 暂时无脑给牌
sgs.ai_skill_discard.jianshe = function(self)
	local to_discard = {}
	local cards = self.player:getCards("hes")
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)

	table.insert(to_discard, cards[1]:getEffectiveId())

	return to_discard
	
	--暂时没考虑不给的情况
	--local skiller = self.player:getTag("jianshe_source"):toPlayer()
	--local to_discard = {}
	--if not skiller or not self:isFriend(skiller) then return to_discard end

end



--极厄:出牌阶段限一次，你可以令至少一名手牌数小于其已损失体力值的角色各失去1点体力。
local ysjie_skill = {}
ysjie_skill.name = "ysjie"
table.insert(sgs.ai_skills, ysjie_skill)
ysjie_skill.getTurnUseCard = function(self, inclusive)
	if self.player:hasUsed("YsJieCard") then return nil end
	return sgs.Card_Parse("@YsJieCard=.")
end

sgs.ai_skill_use_func.YsJieCard=function(card,use,self)
	local targets = {}
	for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self:isEnemy(p) and p:getHandcardNum() < p:getLostHp() then
			table.insert(targets, p)
		end
	end
	if #targets > 0 then
		use.card = card
		if use.to then
			for _,p in ipairs(targets) do
				use.to:append(p)
			end
			return
		end
	end
end
 sgs.ai_card_intention.YsJieCard = 50

--依神：锁定技，当你成为【杀】的目标后，若此牌使用者攻击范围内有其他战势力角色，其选择弃置一张牌或令【杀】无效。
sgs.ai_skill_invoke.yishen =  true
sgs.ai_skill_cardask["@yishen-discard"] = function(self, data)
	local target = self.player:getTag("yishen_target"):toPlayer()
	if self:isFriend(target)  then return "." end

	local cards = sgs.QList2Table(self.player:getCards("hes"))
	if #cards ==0 then return "." end
	self:sortByKeepValue(cards)
	return "$" .. cards[1]:getEffectiveId()
end


--SP丧面秦心
sgs.ai_skill_invoke.mianling = true
sgs.ai_skill_cardask["@mianling-exchange"] = function(self, data, pattern, target, target2, arg, arg2)
	local n = tonumber(arg)
	local cardIds, cards, card = self.player:getPile("qsmian"), {}
	for _, card in sgs.qlist(cardIds) do
		table.insert(cards, sgs.Sanguosha:getCard(card))
	end
	
	self:sortByUseValue(cards, true)
	local toThrow, i = {}
	for i = 1, n do
		table.insert(toThrow, cards[i]:getEffectiveId())
	end
	return "$" .. table.concat(toThrow, "+")
end

-- In fact I don't know if this can be used here
-- analaptic and peach will be dealt with later
local mlskill = {}
mlskill.name = "mianling"
table.insert(sgs.ai_skills, mlskill)
mlskill.getTurnUseCard = function(self)
	return sgs.Card_Parse("@MianlingCard=.")
end

sgs.ai_skill_use_func.MianlingCard=function(card,_use,self)
	local cardIds, cards, card = self.player:getPile("qsmian"), {}
	for _, card in sgs.qlist(cardIds) do
		cards:insert(sgs.Sanguosha:getCard(card))
	end
	self:sortByUseValue(cards)
	local c
	for _, c in ipairs(cards) do
		local use = {isDummy = true, to = sgs.SPlayerList()}
		self:useCardByClassName(c, use)
		if use.card then
			_use.card = sgs.Card_Parse("@MianlingCard=" .. use.card:getEffectiveId())
			if _use.to then
				for _, t in sgs.qlist(use.to) do
					_use.to:append(t)
				end
			end
			return
		end
	end
end

sgs.ai_skill_playerchosen.ximshang = sgs.ai_skill_playerchosen.damage
