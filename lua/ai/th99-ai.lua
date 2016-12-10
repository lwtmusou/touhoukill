
local qiuwen_skill = {}
qiuwen_skill.name = "qiuwen"
table.insert(sgs.ai_skills, qiuwen_skill)
qiuwen_skill.getTurnUseCard = function(self)
	local can_use=false
	if self.player:hasUsed("QiuwenCard") then return nil end
	return sgs.Card_Parse("@QiuwenCard=.")
end
sgs.ai_skill_use_func.QiuwenCard=function(card,use,self)
	use.card = card
end
sgs.ai_use_value.QiuwenCard = 7
sgs.ai_use_priority.QiuwenCard = 7




local dangjiavs_skill = {}
dangjiavs_skill.name = "dangjia_attach"
table.insert(sgs.ai_skills, dangjiavs_skill)
dangjiavs_skill.getTurnUseCard = function(self)
	if self.player:getKingdom()~="wai" then return nil end
	if self.player:isKongcheng() or self:needBear()  or self.player:hasFlag("Forbiddangjia") then return nil end
	return sgs.Card_Parse("@DangjiaCard=.")
end
sgs.ai_skill_use_func.DangjiaCard = function(card, use, self)
	local lord
	for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if player:hasLordSkill("dangjia") and not player:isKongcheng()
		and not player:hasFlag("dangjiaInvoked") and player:isWounded()
		then
			lord=player
			break
		end
	end
	if not lord then return nil end

	if self:isEnemy(lord) then
		return nil
	end

	local cards = self.player:getHandcards()

	local max_num = 0, max_card
	local min_num = 14, min_card
		for _, hcard in sgs.qlist(cards) do
			if hcard:isKindOf("Peach") then continue end
			if hcard:getNumber() > max_num then
				max_num = hcard:getNumber()
				max_card = hcard
			end

			if hcard:getNumber() <= min_num then
				if hcard:getNumber() == min_num then
					if min_card and self:getKeepValue(hcard) > self:getKeepValue(min_card) then
						min_num = hcard:getNumber()
						min_card = hcard
					end
				else
					min_num = hcard:getNumber()
					min_card = hcard
				end
			end
		end
	if not min_card then return nil end

	if min_card:getNumber()>=12 then return nil end
	if min_card:getNumber()>9 and lord:getHandcardNum()<=4 then
		local lord_card = self:getMaxCard(lord)
		if not lord_card or lord_card:getNumber() < min_card:getNumber() then
			return nil
		end
	end
	if self:isFriend(lord) then
		self.dangjia_card = min_card:getEffectiveId()
			use.card = card
			if use.to then use.to:append(lord) end
			return
	end
end
function sgs.ai_skill_pindian.dangjia(minusecard, self, requestor, maxcard)
	return self:getMaxCard()
end

sgs.ai_choicemade_filter.pindian.dangjia = function(self, from, args)
	local number = sgs.Sanguosha:getCard(tonumber(args[3])):getNumber()
	local lord = findPlayerByObjectName(self.room, args[4])
	if not lord then return end

	if number < 6 then sgs.updateIntention(from, lord, -60)
	elseif number > 8 and lord:getHandcardNum()<=4 and self:isEnemy(lord,from) then
		sgs.updateIntention(from, lord, 60)
	end
end
function SmartAI:dangjiaIntention(player)
	local lord=self.room:getLord()
	if lord and lord:hasLordSkill("dangjia") and lord:isWounded() and lord:getHandcardNum()>=5
		and not lord:hasFlag("dangjiaInvoked")
		and player:getKingdom()=="wai" and not  player:hasFlag("PlayPhaseTerminated") then

		local overflow  =self:getOverflow(player)>0
		if player:hasSkill("shoucang") then
			overflow  =self:getOverflow(player)>1
		end
		if overflow then
			sgs.updateIntention(player, lord, 30)
		end
	end
end


function choose_xiufuId(self, card_ids)
	self.player:removeTag("xiufu_equipid")
	self.player:removeTag("xiufu_target")
	local weapons={}
	local armors={}
	local dhorses={}
	local ohorses={}
	local treasures={}
	local weapon_targets={}
	local armor_targets={}
	local dhorse_targets={}
	local ohorse_targets={}
	local treasure_targets={}
	local wounded_silverlion


	local spades={}
	local baoyi
	local baoyi_id


	local chosen_cards_id=-1
	local target

	for _,card_id in pairs(card_ids) do
		local card=sgs.Sanguosha:getCard(card_id)
		if card:getSuit()==sgs.Card_Spade  then
			table.insert(spades,card_id)
		end
		if card:isKindOf("Weapon") then
			table.insert(weapons,card_id)
		elseif card:isKindOf("Armor") then
			table.insert(armors,card_id)
		elseif card:isKindOf("DefensiveHorse") then
			table.insert(dhorses,card_id)
		elseif card:isKindOf("OffensiveHorse") then
			table.insert(ohorses,card_id)
		elseif card:isKindOf("Treasure") then
			table.insert(treasures,card_id)
		end
	end

	for  _,p in pairs(self.friends) do
		if not p:getEquip(0) then
			table.insert(weapon_targets,p)
		end
		if not p:getEquip(1) then
			table.insert(armor_targets,p)
		end
		if not p:getEquip(2) then
			table.insert(dhorse_targets,p)
		end
		if not p:getEquip(3) then
			table.insert(ohorse_targets,p)
		end
		if not p:getEquip(4) then
			table.insert(treasure_targets,p)
		end
		if p:hasSkills("baoyi|jiezou") then
			for _,id in  pairs(spades) do
				local card=sgs.Sanguosha:getCard(id):getRealCard():toEquipCard()
				local n=card:location()
				if not p:getEquip(n) then
					baoyi_id=id
					break
				end
			end
			if not baoyi_id then
				if not p:getEquip(0) then
					baoyi_id=weapons[1]
				elseif not p:getEquip(1) then
					baoyi_id=armors[1]
				elseif not p:getEquip(2) then
					baoyi_id=dhorses[1]
				elseif not p:getEquip(3) then
					baoyi_id=ohorses[1]
				end
			end
			if baoyi_id then
				baoyi=p
			end
		end
		if p:isWounded() and p:getEquip(1) and p:getEquip(1):isKindOf("SilverLion") then
			wounded_silverlion=p
		end
	end
	if baoyi and baoyi_id then
		chosen_cards_id=baoyi_id
		target=baoyi
	elseif #armors>0 and wounded_silverlion then
		chosen_cards_id=armors[1]
		target=wounded_silverlion
	elseif #armors>0 and #armor_targets>0 then
		chosen_cards_id=armors[1]
		self:sort(armor_targets,"hp")
		target=armor_targets[1]
	elseif #dhorses>0 and #dhorse_targets>0 then
		chosen_cards_id=dhorses[1]
		self:sort(dhorse_targets,"hp")
		target=dhorse_targets[1]
	elseif #treasures>0 and #treasure_targets>0 then
		chosen_cards_id=treasures[1]
		target=treasure_targets[1]
	elseif #ohorses>0 and #ohorse_targets>0 then
		chosen_cards_id=ohorses[1]
		target=ohorse_targets[1]
	elseif #weapons>0 and #weapon_targets>0 then
		chosen_cards_id=weapons[1]
		target=weapon_targets[1]
	end
	if chosen_cards_id >-1  and target then
		self.player:setTag("xiufu_equipid",sgs.QVariant(chosen_card_id))
		local _data = sgs.QVariant()
		_data:setValue(target)
		self.player:setTag("xiufu_target",_data)

	end
	self.player:setTag("xiufu_equipid",sgs.QVariant(card_ids[1]) )
end
function equip_in_discardpile(self)
	local card_ids ={}
	local discardpile = self.room:getDiscardPile()
	for _, id in sgs.qlist(discardpile) do
		local tmp_card = sgs.Sanguosha:getCard(id)
		if (tmp_card:isKindOf("EquipCard")) then
			table.insert(card_ids, id)
		end
	end
	return card_ids
end


local xiufu_skill = {}
xiufu_skill.name = "xiufu"
table.insert(sgs.ai_skills, xiufu_skill)
xiufu_skill.getTurnUseCard = function(self)

	--if self.player:hasUsed("XiufuCard") then return nil end
	if self.player:hasFlag("xiufu_used") then return nil end
	--or self.player:hasFlag("Global_xiufuFailed")
	local ids = equip_in_discardpile(self)
	if #ids == 0 then return nil end
	choose_xiufuId(self, ids)
	local card_id=self.player:getTag("xiufu_equipid"):toInt()
	local target=self.player:getTag("xiufu_target"):toPlayer()
	if target then
		return sgs.Card_Parse("@XiufuCard=.")
	end
	return nil
end

sgs.ai_skill_askforag.xiufu = function(self, card_ids)
	local data =self.player:getTag("xiufu_equipid")
	if not data then
		return card_ids[1]
	end
	local id = data:toInt()
	return id
end
sgs.ai_skill_playerchosen.xiufu = function(self, targets)
	local target = self.player:getTag("xiufu_target"):toPlayer()
	if not target then target = self.friends[1] end
	return target
end


sgs.ai_skill_use_func.XiufuCard=function(card,use,self)
	 use.card = card
end


sgs.ai_use_value.xiufu = 7
--sgs.ai_card_intention.XiufuCard = -70
sgs.ai_playerchosen_intention.xiufu = -70

sgs.ai_skill_invoke.fandu = true
sgs.ai_skill_playerchosen.fandu = function(self, targets)
	if #self.enemies==0  then return targets:first() end
	local target_table={}
	local return_table={}
	local maybereturn_table={}
	self:sort(self.enemies,"handcard")
	for _,p in pairs(self.enemies) do
		if p:getHandcardNum()==1 then
			table.insert(target_table,p)
		end
		if p:getHandcardNum()==2 then
			table.insert(return_table,p)
		end
		if p:getHandcardNum()>2 then
			table.insert(maybereturn_table,p)
		end
	end
	if #target_table==0 and #return_table==0 and #maybereturn_table==0 then
		return targets:first()
	end
	local card_number=self:getMaxCard():getNumber()
	local target
	if #return_table>0 and card_number>7 then
		target = return_table[1]
	elseif #maybereturn_table>0 and card_number>10 then
		target = maybereturn_table[1]
	elseif #target_table>0 then
			target = target_table[1]
	elseif #return_table>0 then
		target = return_table[1]
	elseif #maybereturn_table>0 then
		target = maybereturn_table[1]
	end
	if target then
		return target
	end
	return targets:first()
end


sgs.ai_skill_invoke.taohuan = function(self, data)
	local target = data:toPlayer()
	if self:isFriend(target) then
		return false
	end
	self.taohuan_card= self:getMaxCard():getEffectiveId()
	return true
end

function sgs.ai_skill_pindian.taohuan(minusecard, self, requestor, maxcard)
	return self:getMaxCard()
end
sgs.ai_choicemade_filter.skillInvoke.taohuan = function(self, player, args)
	local to = player:getTag("taohuantarget"):toPlayer()
	if  to and player:objectName()~= to:objectName() then
		if args[#args] == "yes" then
			sgs.updateIntention(player, to, 50)
		else
			sgs.updateIntention(player, to, -10)
		end
	end
end

sgs.ai_skill_playerchosen.shitu = function(self, targets)
	local target_table = self:getFriends(self.player)
	if #target_table==0 then return false end
	local shitu_target
	self:sort(target_table, "handcard")
	for _,target in pairs(target_table) do
		if not shitu_target then
			shitu_target=target
		end
		if self:isWeak(target) then
			shitu_target=target
		end
	end
	if shitu_target then
	return shitu_target
	end
	return nil
end
sgs.ai_playerchosen_intention.shitu = -60

sgs.ai_skill_property.shitu = { effect = {{"DrawEffect"}, {"ExtraDrawPhase"}},
	trigger =       {{"Current"}, {"Current"}},--option:if trigger2 has same node with trigger1, we can only record it once.
	target =        {{"AlivePlayers"}, {"AlivePlayers"}},
}


sgs.ai_needToWake.yueshi=function(self,player)
	return "Damage","StartPhase"
end
sgs.ai_need_damaged.mengxian = function(self, attacker, player)
	if player:hasSkills("mengxian+jingjie") and player:getMark("mengxian") == 0  then
		if player:getPile("jingjie"):length()<=3 then
			return not self:isWeak(player)
		end
	end
	return false
end

function SmartAI:canLuanying(player, card )
	if not player:hasSkill("luanying") or  player:getPile("jingjie"):isEmpty()  then
		return false
	end
	if not card:isBlack() and not card:isRed() then return false end
	for _,id in sgs.qlist(player:getPile("jingjie")) do
		if sgs.Sanguosha:getCard(id):sameColorWith(card) then
			return true
		end
	end
	return false
end
sgs.ai_skill_cardask["@luanying-invoke"] = function(self, data)
	local use = data:toCardUse()
	local target
	local card
	if use.to then
		target = use.from
		card = use.card
	else
		local resp = data:toCardResponse()
		target = resp.m_from
		card = resp.m_card
	end

	if not target then return "." end

	if self:isEnemy(target) then
		local getReturn = function()
			local cards = {}
			for _, id in sgs.qlist(self.player:getPile("jingjie")) do
				local silingcard = sgs.Sanguosha:getCard(id)
				if (silingcard:sameColorWith(card)) then
					table.insert(cards, silingcard)
				end
			end
			if #cards == 0 then return "." end
			self:sortByCardNeed(cards)
			return "$" .. cards[1]:getId()
		end

		if use.to then
			for _,p in sgs.qlist(use.to) do
				if card:isKindOf("Slash") and self:isFriend(p)then
					return getReturn()
				end
				if (card:isKindOf("Analeptic") or card:isKindOf("Peach"))and self:isEnemy(p)then
					return getReturn()
				end
			end
			return "."
		end
		return getReturn()
	end
	return "."
end


sgs.ai_skill_use["@@lianxi"] = function(self, prompt)
	local need_recast=false
	if self:getOverflow() <=0 then
		need_recast=true
	end
	if self:getCardsNum("Slash")==0 then
		need_recast=true
	end
	local dummy_use = { isDummy = true, to = sgs.SPlayerList() }

	local card=sgs.cloneCard("iron_chain", sgs.Card_NoSuit, 0)
	card:setSkillName("lianxi")
	local target

	self:useTrickCard(card, dummy_use)
	if not dummy_use.card then return false end
	if dummy_use.to:isEmpty()  or need_recast then
		return dummy_use.card:toString()
	else
		local target_objectname = {}
		for _, p in sgs.qlist(dummy_use.to) do
			table.insert(target_objectname, p:objectName())
			if #target_objectname==2 then break end
		end
		return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
	end
	return dummy_use.card:toString()
end
sgs.ai_cardneed.lianxi = function(to, card, self)
	return  card:isKindOf("Slash")
end


function SmartAI:needWakeYueshi(player)
	player = player or self.player
	if not player:hasSkill("yueshi") then return false end
	if player:getMark("yueshi")>0 then return false end


	local current=self.room:getCurrent()
	local enemy_num=0
	local current_seat=current:getSeat()
	local player_seat=player:getSeat()
	if player:objectName()==current:objectName() then
		for _,p in sgs.qlist(self.room:getOtherPlayers(player))do
				if not self:isFriend(p,player) then
					enemy_num=enemy_num+1
				end
		end
	elseif player_seat>current_seat then
		for _,p in sgs.qlist(self.room:getOtherPlayers(player))do
			if p:getSeat()>=current_seat and
			p:getSeat()<player_seat then
				if not self:isFriend(p,player) then
					enemy_num=enemy_num+1
				end
			end
		end
	elseif player_seat>current_seat then
		for _,p in sgs.qlist(self.room:getOtherPlayers(player))do
			if p:getSeat()>=current_seat or
			p:getSeat()<player_seat then
				if not self:isFriend(p,player) then
					enemy_num=enemy_num+1
				end
			end
		end
	end
	return enemy_num<=1
end
sgs.ai_needToWake.yueshi=function(self,player)
	return "Chained","StartPhase"
end

local function pingyi_skillname(player)
	local skillname
	local room=player:getRoom()
	for _,p in sgs.qlist(room:getOtherPlayers(player,true))do
			if (p:getMark("pingyi")>0)then
				local pingyi_record =player:objectName().."pingyi"..p:objectName()
				skillname =room:getTag(pingyi_record):toString()
				if (skillname and skillname ~="" ) then
					return skillname
				end
			end
	end
	return nil
end

local badSkills={"mokai","guangji","xinghui","bendan","moxue","sisheng","jingdong","shishen","chunmian",
"wangwu","shouye","zhouye","hongwu","shenqiang","yewang","jinguo","rengui","gaoao","caiyu","shenhua"}
local key_skills={"feixiang","mingyun","yongheng","qiuwen","xiangqi","jiushu","hpymsiyu"}

local function pingyi_change(player,attacker)
	if player:getMark("pingyi_steal")>0 then
		skillname=pingyi_skillname(player)
		if skillname and table.contains(key_skills,skillname) then
			return false
		end
	end
	for _,skill in sgs.qlist(attacker:getVisibleSkillList()) do
		if table.contains(key_skills,skill:objectName()) then
			return true
		end
	end
	if player:getMark("pingyi_steal")==0 then
		for _,skill in sgs.qlist(attacker:getVisibleSkillList()) do
			if not table.contains(badSkills,skill:objectName()) then
				return true
			end
		end
	end
	return false
end


sgs.ai_skill_choice.pingyi=function(self, choices)
	local target =self.player:getTag("pingyi_target"):toPlayer()
	if self:isEnemy(target) then
		if not pingyi_change(self.player,target) then
			return "cancel"
		end
		local choice_table = choices:split("+")
		table.removeOne(choice_table, "cancel")
		for index, achoice in ipairs(choice_table) do
			if table.contains(key_skills,achoice)  then return achoice end
		end
		for index, achoice in ipairs(choice_table) do
			if not table.contains(badSkills,achoice)  then return achoice end
		end
	end
	return "cancel"
end
sgs.ai_skill_cardask["@pingyi-discard"] = function(self, data)
	local cards = self.player:getCards("hes")
	cards = sgs.QList2Table(cards)
	if #cards==0 then return "." end
	self:sortByUseValue(cards)
	return "$" .. cards[1]:getId()
end
sgs.ai_need_damaged.pingyi = function(self, attacker, player)
	if self:isWeak(player) or player:isNude() then return false end
	if not self:getDamageSource(attacker) then return false end
	if not self:isEnemy(attacker,player) then return false end
	for _,p in sgs.qlist(self.room:getAlivePlayers())do
		if p:containsTrick("lightning") then
			if  attacker:hasSkills("feixiang|mingyun") and  not player:hasSkills("feixiang|mingyun") then
				return true
			end
		end
	end
	return pingyi_change(player,attacker)
end
sgs.ai_choicemade_filter.cardResponded["@pingyi-discard"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		local target = player:getTag("pingyi_target"):toPlayer()
		if not target then return end
		sgs.updateIntention(player, target, 80)
	end
end

sgs.ai_skill_use["@@zheshe"] = function(self, data, method)
	if not method then method = sgs.Card_MethodDiscard end
	local friend_lost_hp = 10
	local friend_hp = 0
	local card_id
	local target
	local cant_use_skill
	local dmg

	if data == "@zheshe" then
		dmg = self.player:getTag("zhesheDamage"):toDamage()
	else
		dmg = data
	end

	if not dmg then self.room:writeToConsole(debug.traceback()) return "." end

	local cards = self.player:getCards("hs")
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)
	for _, card in ipairs(cards) do
		if not self.player:isCardLimited(card, method)
		and not card:isKindOf("Peach") then
			card_id = card:getId()
			break
		end
	end
	if not card_id then return "." end

	self:sort(self.enemies, "hp")

	for _, enemy in ipairs(self.enemies) do
		if (enemy:getHp() <= dmg.damage and enemy:isAlive()) then
			if (enemy:getHandcardNum() <= 2 or enemy:hasSkills("guose|leiji|ganglie|enyuan|qingguo|wuyan|kongcheng") or enemy:containsTrick("indulgence"))
				and self:canAttack(enemy, dmg.from or self.room:getCurrent(), dmg.nature)
				and not (dmg.card and dmg.card:getTypeId() == sgs.Card_TypeTrick and enemy:hasSkill("wuyan")) then
				return "@ZhesheCard=" ..card_id.. "->" .. enemy:objectName()
			end
		end
	end

	for _, friend in ipairs(self.friends) do
		if (friend:getLostHp() + dmg.damage > 1 and friend:isAlive()) then
			if friend:isChained() and dmg.nature ~= sgs.DamageStruct_Normal and not self:isGoodChainTarget(friend, dmg.from, dmg.nature, dmg.damage, dmg.card) then
			elseif friend:getHp() >= 2 and dmg.damage < 2
					and (friend:hasSkills("yiji|buqu|nosbuqu|shuangxiong|zaiqi|yinghun|jianxiong|fangzhu")
						or self:getDamagedEffects(friend, dmg.from or self.room:getCurrent())
						or self:needToLoseHp(friend)
						or (friend:getHandcardNum() < 3 and (friend:hasSkill("nosrende") or (friend:hasSkill("rende") and not friend:hasUsed("RendeCard"))))) then
				return "@ZhesheCard=" ..card_id.. "->" .. friend:objectName()
				elseif dmg.card and dmg.card:getTypeId() == sgs.Card_TypeTrick and friend:hasSkill("wuyan") and friend:getLostHp() > 1 then
					return "@ZhesheCard=" ..card_id.. "->" .. friend:objectName()
			elseif hasBuquEffect(friend) then
				return "@ZhesheCard=" ..card_id.. "->" .. friend:objectName()
			end
		end
	end

	for _, enemy in ipairs(self.enemies) do
		if (enemy:getLostHp() <= 1 or dmg.damage > 1) and enemy:isAlive() then
			if (enemy:getHandcardNum() <= 2)
				or enemy:containsTrick("indulgence") or enemy:hasSkills("guose|leiji|vsganglie|ganglie|enyuan|qingguo|wuyan|kongcheng")
				and self:canAttack(enemy, (dmg.from or self.room:getCurrent()), dmg.nature)
				and not (dmg.card and dmg.card:getTypeId() == sgs.Card_TypeTrick and enemy:hasSkill("wuyan")) then
				return "@ZhesheCard=" ..card_id.. "->" .. enemy:objectName()
			end
		end
	end

	for i = #self.enemies, 1, -1 do
		local enemy = self.enemies[i]
		if not enemy:isWounded() and not self:hasSkills(sgs.masochism_skill, enemy) and enemy:isAlive()
			and self:canAttack(enemy, dmg.from or self.room:getCurrent(), dmg.nature)
			and (not (dmg.card and dmg.card:getTypeId() == sgs.Card_TypeTrick and enemy:hasSkill("wuyan") and enemy:getLostHp() > 0) or self:isWeak()) then
			return "@ZhesheCard=" ..card_id.. "->" .. enemy:objectName()
		end
	end

	return "."
end
sgs.ai_card_intention.ZhesheCard = function(self, card, from, tos)
	local to = tos[1]
	if self:getDamagedEffects(to) or self:needToLoseHp(to) then return end
	local intention = 10
	if hasBuquEffect(to) then intention = 0
	elseif (to:getHp() >= 2 and self:hasSkills("yiji|shuangxiong|zaiqi|yinghun|jianxiong|fangzhu", to))
		or (to:getHandcardNum() < 3 and (to:hasSkill("nosrende") or (to:hasSkill("rende") and not to:hasUsed("RendeCard")))) then
		intention = -10
	end
	sgs.updateIntention(from, to, intention)
end

function SmartAI:touhouDamageTransfer(player)
	if player:hasSkill("zheshe") and not player:isKongcheng() then
		return true
	end
	if player:hasSkill("zhengti") then
		for _,p in sgs.qlist(self.room:getOtherPlayers(player))do
			if p:getMark("@zhengti")>0 then
				return true
			end
		end
	end
	return false
end



local zhuonong_skill = {}
zhuonong_skill.name = "zhuonong"
table.insert(sgs.ai_skills, zhuonong_skill)
zhuonong_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ZhuonongCard") then return nil end
	return sgs.Card_Parse("@ZhuonongCard=.")
end
sgs.ai_skill_use_func.ZhuonongCard = function(card, use, self)
	local weaks={}
	local notWounded={}
	local chained={}
	local goodTargets={}
	local cankill = {}
	local f =0
	local e=0

	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:isChained() then
			table.insert(chained,p)
			local n= p:hasArmorEffect("Vine") and 2 or 1
			if self:isEnemy(p) then
				e=e+n
			elseif self:isFriend(p) then
				f=f+n
			end
		end
	end
	local chain = e>f and  true or false
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if p:isChained() then
			continue
		end
		if self:isEnemy(p) then
			if self:touhouDamageTransfer(p) then
				continue
			end
			if self:touhouDamageProhibit(sgs.DamageStruct(nil, self.player, p, 1, sgs.DamageStruct_Fire), self.player, p) then
				continue
			end
			if p:hasSkill("ganying")  and self.player:distanceTo(p) >1 then
				continue
			end
			if not p:isWounded() then
				table.insert(notWounded,p)
			end
			if p:getHp()<=2 and self:slashIsAvailable()
			and self:getCardsNum("Slash") > 0
			and  self.player:canSlash(p,nil,true) then
				table.insert(cankill,p)
			end
			if p:getHp()==1 then
				table.insert(weaks,p)
			end
			if  p:hasArmorEffect("Vine") then
				table.insert(goodTargets,p)
			end
		elseif self:isFriend(p) then
			if p:hasArmorEffect("Vine") then
				continue
			end
			if p:isWounded() and self:touhouDamageTransfer(p) then
				table.insert(goodTargets,p)
			end
			if p:hasSkill("ganying") and self.player:distanceTo(p) >1 then
				table.insert(goodTargets,p)
			end
		end
	end
	sgs.ai_use_priority.ZhuonongCard = #cankill >0 and 0 or 5
	local target
	if chain then
		self:sort(chained,"hp")
		for _,p in pairs(chained)do
			if self:isFriend(p) then
				target=p
				break
			end
		end
		if not target then
			target=chained[1]
		end
	elseif #goodTargets>0 then
		self:sort(goodTargets,"hp")
		target= goodTargets[1]
	elseif #weaks>0 then
		self:sort(weaks,"hp")
		target= weaks[1]
	elseif #notWounded>0 then
		self:sort(notWounded,"hp")
		target= notWounded[1]
	end
	if target then
		use.card = card
		if use.to then use.to:append(target) end
	end
end
sgs.ai_skill_choice.zhuonong=function(self, choices)
	local target=self.player:getTag("zhuonong_target"):toPlayer()
	if self:isFriend(target) then
		if target:isWounded() and target:getHp()<2 then
			return "rd"
		else
			return "dr"
		end
	else
		if target:isWounded() then
			return "dr"
		else
			return "rd"
		end
	end
end
sgs.ai_use_value.ZhuonongCard = sgs.ai_use_value.Slash+0.5
sgs.ai_choicemade_filter.skillChoice.zhuonong = function(self, player, args)
	local choice = args[#args]
	local target =player:getTag("zhuonong_target"):toPlayer()
	if not target then return end
	local friendly =false
	if self:isFriend(player,target) then
		if target:isWounded() and target:getHp()<2 then
			if choice== "rd" then friendly =true end
		else
			if choice== "dr" then friendly =true end
		end
	end
	if friendly then
		sgs.updateIntention(player, target, -20)
	else
		sgs.updateIntention(player, target, 40)
	end
end




sgs.ai_skill_playerchosen.ganying = function(self, targets)
	if self:isWeak(self.player) then return self.player end
	for _,p in sgs.qlist(targets) do
		if self:isEnemy(p)  then
			return p
		end
	end
	return nil
end
sgs.ai_playerchosen_intention.ganying = 60
sgs.ai_skill_invoke.ganying = true
sgs.ai_cardneed.ganying = function(to, card, self)
	if not self:willSkipPlayPhase(to) then
		return  (not to:getDefensiveHorse() and  getCardsNum("DefensiveHorse",to,self.player)<1 and card:isKindOf("DefensiveHorse"))
		or (not to:getOffensiveHorse() and  getCardsNum("OffensiveHorse",to,self.player)<1 and card:isKindOf("OffensiveHorse"))
	end
end



sgs.ai_skill_cardask["@zhujiu"] = function(self, data)
	local target = self.player:getTag("zhujiu_target"):toPlayer()
	if not target or not self:isFriend(target)  then return "." end
	local cards= self.player:getCards("hes")
	cards = sgs.QList2Table(cards)
	if #cards==0 then return "." end
	self:sortByUseValue(cards, true)
	return "$" .. cards[1]:getId()
end
sgs.ai_choicemade_filter.cardResponded["@zhujiu"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		local target = player:getTag("zhujiu_target"):toPlayer()
		if not target then return end
		sgs.updateIntention(player, target, -80)
	end
end


function SmartAI:yushouValue(from ,to)
	if self:isFriend(from , to) then
		return 0
	elseif self:isEnemy(from, to) then
		local basic = -1
		if self:isWeak(to) then
			basic = basic + 2
		end
		if self:isEnemy(from) then
			basic = -basic
		end
		return basic
	elseif self:isFriend(from) then
		return -1
	elseif self:isFriend(to) then
		return 1
	end
	return 0
end
local yushou_skill = {}
yushou_skill.name = "yushou"
table.insert(sgs.ai_skills, yushou_skill)
yushou_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("YushouCard") then return nil end
	return sgs.Card_Parse("@YushouCard=.")
end
sgs.ai_skill_use_func.YushouCard = function(card, use, self)
	local target_table = {}
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		local ecards = p:getCards("e")
		if ecards:isEmpty() then continue end
		for _,t in sgs.qlist(self.room:getOtherPlayers(p)) do
			for _,c in sgs.qlist(ecards) do
				local equip = c:getRealCard():toEquipCard()
				if not t:getEquip(equip:location()) then
					local array={from = p, to = t, value=self:yushouValue(p, t)}
					table.insert(target_table,array)
					break
				end
			end
		end
	end
	local compare_func = function(a, b)
		return a.value > b.value
	end

	if #target_table > 0 then
		table.sort(target_table, compare_func)
		if target_table[1].value > 0 then

			sgs.ai_use_priority.YushouCard = self:isFriend(target_table[1].from) and 2 or 8
			use.card = card
			if use.to then
				use.to:append(target_table[1].from)
				use.to:append(target_table[1].to)
				if use.to:length() >= 2 then return end
			end
		end
	end
end

sgs.ai_skill_cardchosen.yushou = function(self, who, flags)
	local to = self.player:getTag("yushou_target"):toPlayer()
	local cards =  {}
	for _,c in sgs.qlist(who:getCards("e")) do
		local equip = c:getRealCard():toEquipCard()
		if not to:getEquip(equip:location()) then
			table.insert(cards, c)
		end
	end
	local inverse = self:isFriend(who) and true or false

	self:sortByUseValue(cards, inverse)
	return cards[1]
end

sgs.ai_skill_invoke.yushou_damage = function(self,data)
	local target =self.player:getTag("yushou_target"):toPlayer()
	if not target then return false end

	local fakeDamage=sgs.DamageStruct(nil, self.player, target, 1, sgs.DamageStruct_Normal)
	local effect, willEffect = self:touhouDamageEffect(fakeDamage, self.player, target)
	if effect then
		return true
	end
	if target and self:isEnemy(target) then
		if not self:getDamagedEffects(target, self.player, false) then
			return true
		end
	end
	return false
end

sgs.ai_use_priority.YushouCard = 8

local pandu_skill = {}
pandu_skill.name = "pandu"
table.insert(sgs.ai_skills, pandu_skill)
pandu_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("PanduCard") then return nil end
	return sgs.Card_Parse("@PanduCard=.")
end
sgs.ai_skill_use_func.PanduCard=function(card,use,self)
	local targets={}
	local good_targets={}
	self:sort(self.enemies, "handcard",false)
	for _,p in pairs(self.enemies) do
		if p:isKongcheng() then continue end
		if p:hasSkill("taohuan") or self:touhouHandCardsFix(p) then
			continue
		end
		if getCardsNum("Slash", p, self.player) < 1 or sgs.card_lack[p:objectName()]["Slash"] == 1 then
			table.insert(good_targets,p)
		else
			local rate= getCardsNum("Slash", p, self.player)/ p:getHandcardNum()
			if rate <= 1/2 then
				table.insert(targets,p)
			end
		end
	end
	if #good_targets>0 or  #targets>0 then
		use.card = card
		if use.to then
			if #good_targets>0 then
				use.to:append(good_targets[1])
				if use.to:length() >= 1 then return end
			elseif #targets>0 then
				use.to:append(targets[1])
				if use.to:length() >= 1 then return end
			end
		end
	end
end
sgs.ai_card_intention.PanduCard = 50
sgs.ai_use_priority.PanduCard = sgs.ai_use_priority.Dismantlement -0.4

sgs.ai_skill_playerchosen.bihuo = function(self, targets)
	if #self.enemies ==0 then return nil end
	local weak_targets={}
	local normal_targets={}
	local temp_targets={}
	local has_heart=false
	for _,c in sgs.qlist(self.player:getCards("hs")) do
		if c:getSuit()== sgs.Card_Heart then
			has_heart=true
			break
		end
	end

	for _,p in sgs.qlist(targets) do
		if self:isEnemy(p) then
			if p:hasSkills("chunxi|xingyun") and has_heart then
				continue
			elseif  self:isWeak(p) or getCardsNum("Slash", p, self.player) < 1 or sgs.card_lack[p:objectName()]["Slash"] == 1 then
				table.insert(weak_targets,p)
			else
				table.insert(normal_targets,p)
			end
		elseif not self:isFriend(p) then
			if self.player:getRole() == "renegade" and p:isLord() then
				continue
			end
			table.insert(temp_targets,p)
		end
	end
	if #weak_targets>0 then
		return weak_targets[1]
	elseif #normal_targets>0 then
		return normal_targets[1]
	elseif #temp_targets>0 then

	end
	return nil
end

sgs.ai_playerchosen_intention.bihuo =function(self, from, to)
	local intention = 50
	sgs.updateIntention(from, to, intention)

	sgs.ai_bihuo_effect =true
end
sgs.ai_slash_prohibit.bihuo = function(self, from, to, card)
	if to:isKongcheng()  or to:getMark("bihuo") > 0 then
		return false
	end
	if self:isFriend(from,to) then
		return false
	end
	if self.player:hasSkill("lizhi") then
		return false
	end
	if self.player:hasSkill("sidie") and self.player:getPhase() ==sgs.Player_Play then
		return false
	end
	for _,p in pairs (self.friends_noself)do
		local fakeDamage=sgs.DamageStruct()
		fakeDamage.card=card
		fakeDamage.nature= sgs.DamageStruct_Normal
		fakeDamage.damage=1
		fakeDamage.from=self.player
		fakeDamage.to=p
		local effect= self:touhouDamage(fakeDamage,self.player,p).damage>0
		if  self:slashIsEffective(card, p, from)  and effect then
			return true
		end
	end
	return false
end




sgs.ai_skill_cardask["jidong-discard"] = function(self, data)
	local use = data:toCardUse()
	if self:touhouCardUseEffectNullify(use, self.player) then return "." end

	local cards = {}
	for _,c in sgs.qlist(self.player:getCards("hs")) do
		--if c:getTypeId() == use.card:getTypeId() then
		if c:isKindOf("BasicCard") then
			table.insert(cards, c)
		end
	end
	if #cards <= 0 then
		return "."
	end


	local compare_func = function(a, b)
		return a:getNumber() < b:getNumber()
	end

	table.sort(cards, compare_func)

	if self:isFriend(use.from) or not use.from:canDiscard(use.from, "hs") then
		return "$" .. cards[1]:getId()
	else
		return "$" .. cards[#cards]:getId()
	end
end

sgs.ai_skill_cardask["jidong-confirm"] = function(self, data)
	local target = self.player:getTag("jidong_target"):toPlayer()
	if not self:isEnemy(target) then  return "." end

	local card = target:getTag("jidong_card"):toCard()
	local cards = {}
	for _,c in sgs.qlist(self.player:getCards("hs")) do
		if c:getNumber() > card:getNumber() then
			table.insert(cards, c)
		end
	end
	if #cards <= 0 then
		return "."
	end
	self:sortByUseValue(cards)
	return "$" .. cards[1]:getId()
end

sgs.ai_choicemade_filter.cardResponded["jidong-confirm"] = function(self, player, args)
	if args[#args] ~= "_nil_" then
		local target = player:getTag("jidong_target"):toPlayer()
		if not target then return end
		sgs.updateIntention(player, target, 50)
	end
end



sgs.ai_skill_invoke.zhangmu = function(self, data)
	local use = self.player:getTag("zhangmu"):toCardUse()
	if use.card:isKindOf("Slash") then
		if self:getCardsNum("Jink") > 0 then
			return self.player:getHandcardNum() < 3
		elseif self.player:getHp() == 1 and (self:getCardsNum("Peach") > 0 or self:getCardsNum("Analeptic") > 0) then
			return self.player:getHandcardNum() < 3
		else
			return self.player:getHandcardNum() <= 1
		end
	else
		if self:isFriend(use.from) then
			if self.player:getHandcardNum() > 1 then
				return self.player:getCards("j"):isEmpty()
			else
				return true
			end
		else
			return self.player:getCards("e"):isEmpty() or self.player:getHandcardNum() <= 1
		end
	end
	return false
end
sgs.ai_skill_discard.zhangmu = function(self)
	local use = self.player:getTag("zhangmu"):toCardUse()
	local to_discard = {}
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)
	local keep
	if use.card:isKindOf("Slash") then
		for _, hcard in ipairs(cards) do
			if hcard:isKindOf("Jink") then
				keep = hcard
				break
			elseif self.player:getHp() == 1 and (hcard:isKindOf("Peach") or hcard:isKindOf("Analeptic")) then
				keep = hcard
				break
			end
		end

		if not keep then
			keep = cards[1]
		end
	else
		if self:isFriend(use.from) then
			for _, hcard in ipairs(cards) do
				for _, askill in sgs.qlist(use.from:getVisibleSkillList()) do
					local callback = sgs.ai_cardneed[askill:objectName()]
					if type(callback)=="function" and callback(use.from, hcard, self) then
						keep = hcard
						break
					end
				end
				if keep then break end
			end
			if not keep then
				keep = cards[1]
			end
		else
			keep = cards[#cards]
		end
	end

	for _, hcard in ipairs(cards) do
		if hcard ~= keep then
			table.insert(to_discard, hcard:getEffectiveId())
		end
		if #to_discard >= 2 then break end
	end
	return to_discard
end

sgs.ai_skill_playerchosen.liyou = function(self, targets)
	local cards = self.player:getCards("hes")
	cards = sgs.QList2Table(cards)
	local card, p = self:getCardNeedPlayer(cards)
	if card and p and p:objectName() ~= self.player:objectName() and self:isFriend(p) then
		return p
	elseif #self.friends_noself > 0 then
		return self.friends_noself[1]
	end
	return nil
end
sgs.ai_playerchosen_intention.liyou = -30


sgs.ai_slash_prohibit.sixiang = function(self, from, to, card)
	if not card:isKindOf("NatureSlash") then return false end
	if self:isFriend(from, to) or not to:isChained() then return false end
	local fakeDamage= sgs.DamageStruct()
	fakeDamage.card = card
	fakeDamage.nature = self:touhouDamageNature(card,from,to)
	fakeDamage.damage=1
	fakeDamage.from= from
	fakeDamage.to = to
	finalDamage = self:touhouDamage(fakeDamage, from, to)
	if finalDamage.damage >= to:getHp() then
		return false
	else
		for _,p in sgs.qlist(self.room:getOtherPlayers(to)) do
			if self:isEnemy(p) then
				return false
			end
		end
	end

	return true
end

sgs.ai_skill_choice.daoyao= function(self, choices, data)
	local target=self.player:getTag("daoyao-target"):toPlayer()
	if choices:match("discard") and self:isEnemy(target) then
		return "discard"
	end
	if choices:match("draw") and self:isFriend(target) then
		return "draw"
	end
	local invokes = choices:split("+")
	return invokes[1]
end
sgs.ai_choicemade_filter.skillChoice.daoyao = function(self, player, args)
	local choice = args[#args]
	local target=self.player:getTag("daoyao-target"):toPlayer()
	if  choice == "discard"  and target then
		sgs.updateIntention(player, target, 40)
	elseif choice == "draw" and target then
		sgs.updateIntention(player, target, -40)
	end
end
sgs.ai_skill_playerchosen.daoyao = function(self, targets)
	local target_table = sgs.QList2Table(targets)
	self:sort(target_table, "handcard")
	for _,p in ipairs(target_table) do
		if self:isFriend(p) and p:isChained() then
			return p
		elseif p:isCurrent() and  self:isEnemy(p) and p:canDiscard(p, "hes") then
			return p
		end
	end
	return nil
end
--sgs.ai_playerchosen_intention.daoyao = -40
sgs.ai_slash_prohibit.sixiang = function(self, from, to, card)
	if self:isFriend(from,to) then
		return false
	end
	local callback=sgs.ai_damage_prohibit["sixiang"]
	local damage = sgs.DamageStruct(card, from, to, 1, self:touhouDamageNature(card,from,to))
	return callback(self, from, to, damage)
end
sgs.ai_damage_prohibit.sixiang = function(self, from, to, damage)
	if not to:hasSkill("daoyao") then return false end
	if self:isFriend(from,to) then return false end
	if not to:isChained() then return false end
	if damage.nature == sgs.DamageStruct_Normal then return false end
	return self:touhouDamage(damage,from,to).damage < to:getHp()
	--暂不考虑连弩
end

