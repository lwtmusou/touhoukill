--酒貌似是使用就根本就不过useBasicCard。。。。不能通过turnuse插入card列表。。。
--而是在使用杀的usecard function里调用的 getCardId。。。这个函数丝毫不用到getTurnUse。。。

sgs.ai_cardneed.zuiyue = function(to, card, self)
	return card:isKindOf("Slash")  or card:isKindOf("TrickCard")
end


function SmartAI:cautionDoujiu(player,card)
	player = player or self.player
	if player:getPhase() ~=sgs.Player_Play then
		return false
	end
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
sgs.ai_choicemade_filter.skillInvoke.doujiu = function(self, player, args)
	local target =player:getTag("doujiu_target"):toPlayer()
	if target then
		if args[#args] == "yes" then
			sgs.updateIntention(player, target, 50)
		end
	end
end
function sgs.ai_skill_pindian.doujiu(minusecard, self, requestor, maxcard)
	return self:getMaxCard()
end
sgs.ai_cardneed.doujiu = function(to, card, self)
	return card:getNumber() > 10
end

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

function huiwu_judge(self,target,card)
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
		if self:isFriend(target) and target:getCards("j"):length()>0 then
			return 2
		end
		if self:isEnemy(target) and target:getCards("j"):length()>0  and target:isNude() then
			return 2
		end
		return 1
	end
	return 1
end
sgs.ai_skill_invoke.huiwu =function(self,data)
	local target=self.player:getTag("huiwu"):toPlayer()
	local card=self.room:getTag("huiwu_use"):toCardUse().card
	if not target then  return false end
	local res=huiwu_judge(self,target,card)
	if res==1 then--杀等危害性牌
		return self:isFriend(target)
	end
	if res==2  then
		return self:isEnemy(target)
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.huiwu = function(self, player, args)
	local card=self.room:getTag("huiwu_use"):toCardUse().card
	local to=player:getTag("huiwu"):toPlayer()
	res=huiwu_judge(self,to,card)
	if res==1 then
		if args[#args] == "yes" then
			sgs.updateIntention(player, to, -20)
		else
			sgs.updateIntention(player, to, 20)
		end
	end
	if res==2 then
		if args[#args] == "yes" then
			sgs.updateIntention(player, to, 20)
		else
			sgs.updateIntention(player, to, -20)
		end
	end
end
sgs.ai_benefitBySlashed.huiwu = function(self, card,source,target)
	return true
end


sgs.ai_skill_invoke.huazhong = function(self, data)
	if not self:invokeTouhouJudge() then return false end
	local to =data:toPlayer()
	return self:isFriend(to)
end
sgs.ai_choicemade_filter.skillInvoke.huazhong = function(self, player, args)
	local to=player:getTag("huazhong-target"):toPlayer()
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



sgs.ai_skill_invoke.mingtu = true

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

sgs.ai_skill_invoke.judu =function(self,data)
	if not self:invokeTouhouJudge() then return false end
	local target=data:toPlayer()
	if self:isEnemy(target) then
		return true
	end
end
sgs.ai_choicemade_filter.skillInvoke.judu = function(self, player, args)
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	if args[#args] == "yes" then
		sgs.updateIntention(damage.from, damage.to, 70)
	end
end
sgs.ai_skillProperty.judu = function(self)
	return "cause_judge"
end


sgs.ai_skill_invoke.henyi =function(self,data)
	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
	local card = sgs.cloneCard("archery_attack", sgs.Card_NoSuit, 0)
	card:setSkillName("henyi")
	card:deleteLater()
	self:useTrickCard(card, dummy_use)
	if  dummy_use.card then return true end
	return false
end


--[[function SmartAI:toupaiValue(player)
	if self:touhouHandCardsFix(player) or player:hasSkill("heibai") then
		return 0
	end

	local value=0
	if player:hasSkills("xisan|yongheng|kongpiao") then
		value= - 20
	end
	for _, card in sgs.qlist(player:getHandcards()) do
		local flag = string.format("%s_%s_%s", "visible", global_room:getCurrent():objectName(), player:objectName())
		if  card:hasFlag("visible") or card:hasFlag(flag) then
			if card:isRed() then
					value=value+10
				if card:isKindOf("BasicCard") then
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
end]]
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
--[[sgs.ai_skill_playerchosen.toupai = function(self, targets)
	self:sort(self.enemies,"handcard")
	self.enemies = sgs.reverse(self.enemies)
	local enemies = self.enemies
	for _,p in pairs(self.enemies) do
		if self:touhouHandCardsFix(p) or p:hasSkill("heibai") then
			table.removeOne(enemies, p)
		end
	end
	if #enemies==0 then return nil end

	--考虑计算量 应该采用的算法
	--因为遍历table（玩家数不多）本身的时间比较少，而计算value的时间比较多
	--不过前提是value取值算法固定..不会因为比较的玩家发生变化而改变

	enemy_table={}
	for _,e in pairs (enemies) do
		local array={player=e, value=self:toupaiValue(e)}
		table.insert(enemy_table,array)
	end
	local compare_func = function(a, b)
		return a.value > b.value
	end
	table.sort(enemy_table, compare_func)
	if enemy_table[1].value >=15 then
		return enemy_table[1].player
	end
	return nil
end]]
--sgs.ai_playerchosen_intention.toupai = 60
--sgs.ai_skill_askforag.toupai = function(self, card_ids)
sgs.ai_skill_invoke.qucai = true


sgs.ai_skill_playerchosen.feixiang = function(self, targets)
	local judge=self.player:getTag("feixiang_judge"):toJudge()
	local cards={}
	table.insert(cards,judge.card)
	local ex_id = self:getRetrialCardId(cards, judge)
	--ex_id 不为-1 则代表 当前判定对天子而言是个好结果
	local retrial_targets={}
	for _,target in sgs.qlist(targets) do
		local e_value = 0
		local cards1 = sgs.QList2Table(target:getCards("e"))
		local self_card =  target:objectName()== self.player:objectName()
		local new_id = self:getRetrialCardId(cards1, judge,self_card)
		--new_id 不为-1 代表 装备区的id去改判，可以得到好结果

		--装备判断
		if ex_id == -1 and new_id ~= -1 then
			if self:isEnemy(target) then
				e_value = e_value + 100
			elseif self:isFriend(target) then
				e_value = e_value + 20
			else
				e_value = e_value + 50
			end
		elseif ex_id == -1 and new_id == -1 then
			if self:isEnemy(target) and #cards1 > 0 then
				e_value = e_value + 10
			end
		--elseif ex_id ~= -1 and new_id == -1 then
		elseif ex_id ~= -1 and new_id ~= -1 then
			if self:isEnemy(target) then
				e_value = e_value + 50
			end
		end


		local array={player= target, value= e_value}
		table.insert(retrial_targets,array)

		if not (e_value > 0  or self:touhouHandCardsFix(target) or target:isKongcheng()) then  
			--手牌判断
			if ex_id == -1 then
				if self:isEnemy(target) then
					local array={player= target, value = 5 - target:getHandcards():length()}
					table.insert(retrial_targets,array)
				elseif (self.player:objectName() == target:objectName()) then
					local cards1 = sgs.QList2Table(target:getHandcards())
					local new_id=self:getRetrialCardId(cards1, judge, true)
					if new_id ~= -1 then
						local array={player= target, value = 30}
						table.insert(retrial_targets,array)
					end
				end
			elseif self:isEnemy(target) then --敌人的已知手牌
				local count=0
				for _, card in sgs.qlist(target:getHandcards()) do
					local flag = string.format("%s_%s_%s", "visible", global_room:getCurrent():objectName(), target:objectName())
					if  card:hasFlag("visible") or card:hasFlag(flag) then
						local cards1={}
						table.insert(cards1,card)
						local new_id=self:getRetrialCardId(cards1, judge, false)
						if new_id ~=-1 then
							count= count + 1
						end
					end
				end
				local array={player= target, value = 5 - target:getHandcards():length() + count}
				table.insert(retrial_targets,array)
			end
		end
	end
	local compare_func = function(a, b)
		return a.value > b.value
	end
	table.sort(retrial_targets, compare_func)
	if #retrial_targets>0 then
		return retrial_targets[1]
	end
	return nil
end
sgs.ai_skill_cardchosen.feixiang = function(self, who, flags)
	local flag = "e"
	local judge=self.player:getTag("feixiang_judge"):toJudge()
	local cards={}
	table.insert(cards,judge.card)
	local ex_id = self:getRetrialCardId(cards, judge)
	if ex_id == -1 then
		if who:objectName() == self.player:objectName() or self.player:hasSkill("duxin") then
			flag  = "hes"
		end
		local cards1 = sgs.QList2Table(who:getCards(flag))
		local self_card =  who:objectName()== self.player:objectName()
		local new_id=self:getRetrialCardId(cards1, judge, self_card)
		if new_id ~= -1 then
			return sgs.Sanguosha:getCard(new_id)
		end
		if #cards1 > 0 then
			return cards1[math.random(1, #cards1)]
		end
	else
		local cards1 = sgs.QList2Table(who:getCards("e"))
		local self_card =  who:objectName()== self.player:objectName()
		local new_id=self:getRetrialCardId(cards1, judge, self_card)
		if new_id ~= -1 then
			return sgs.Sanguosha:getCard(new_id)
		else
			flag = "hs"
		end
	end
	local cards2 = sgs.QList2Table(who:getCards("hes"))
	return cards2[math.random(1, #cards2)]
end

sgs.ai_playerchosen_intention.feixiang = 50

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


sgs.ai_skill_invoke.dizhen =function(self,data)
	local target=self.player:getTag("dizhen_judge"):toJudge().who
	if self:isEnemy(target) then
		return true
	end
end


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
	if self:needBear() then return "." end
	return self:getCardId("Jink") or "."
end



sgs.ai_damageInflicted.jingdian = function(self, damage)
	if damage.nature == sgs.DamageStruct_Thunder and damage.to:hasSkill("jingdian") then
			damage.damage=0
	end
	return damage
end
sgs.ai_benefitBySlashed.jingdian = function(self, card,source,target)
	if not card:isKindOf("ThunderSlash")  then return false end
	return true
end

local leiyun_skill = {}
leiyun_skill.name = "leiyun"
table.insert(sgs.ai_skills, leiyun_skill)
leiyun_skill.getTurnUseCard = function(self, inclusive)
		local cards = self.player:getCards("hs")
		cards=self:touhouAppendExpandPileToList(self.player,cards)
		cards = sgs.QList2Table(cards)
		self:sortByUseValue(cards, true)

		local heart_card
		for _, card in ipairs(cards) do
				if  (card:getSuit()==sgs.Card_Heart or card:getSuit()==sgs.Card_Spade) and  not isCard("Peach", card, self.player)
				and not isCard("ExNihilo", card, self.player) then
						heart_card = card
						break
				end
		end
		if heart_card then
				local suit = heart_card:getSuitString()
				local number = heart_card:getNumberString()
				local card_id = heart_card:getEffectiveId()
				local lightning_str = ("lightning:leiyun[%s:%s]=%d"):format(suit, number, card_id)
				local lightning = sgs.Card_Parse(lightning_str)

				assert(lightning)
				return lightning
		end
end
sgs.ai_cardneed.leiyun = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return card:getSuit()==sgs.Card_Heart or card:getSuit()==sgs.Card_Spade
	end
end
sgs.leiyun_suit_value = {
	spade = 3.9,
	heart = 3.9
}
sgs.ai_skillProperty.leiyun = function(self)
	return "use_delayed_trick"
end


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
sgs.ai_cardneed.nengwu = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  (not to:getWeapon() and  getCardsNum("Weapon",to,self.player)<1 and card:isKindOf("Weapon"))
		or (not to:getOffensiveHorse() and  getCardsNum("OffensiveHorse",to,self.player)<1 and card:isKindOf("OffensiveHorse"))
	end
end

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
