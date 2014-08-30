module("extensions.th08", package.seeall)
extension = sgs.Package("th08")
local function touhou_logmessage(logtype,logfrom,logarg,logto,logarg2,logtotype)
	local alog = sgs.LogMessage()
	 alog.type = logtype
	 alog.from = logfrom
	if logtotype and logtotype==1 then  
		alog.to=logto
	else
		if logto then
			alog.to:append(logto)	
		end
	end
	if logarg then
		alog.arg = logarg
	end
	if logarg2 then
		alog.arg2 = logarg2
	end
	local room = logfrom:getRoom()
	room:sendLog(alog)
end
 -----------------------------------------------------------------------------------------------------------
--技能代码   永夜抄
------------------------------------------------------------------------------------------------------------	
--【永远与须臾的罪人——蓬莱山辉夜】 编号：08001 
yyc001 = sgs.General(extension,"yyc001$","yyc",4,false) --08001
function adjustHandcardNum(player, card_num, reason)
        local room = player:getRoom()
        local hc_num = player:getHandcardNum()
		if card_num ~= hc_num then
			touhou_logmessage("#TriggerSkill",player,"yongheng")
			room:notifySkillInvoked(player, "yongheng")
		end
		if card_num > hc_num then
                player:drawCards(card_num-hc_num, reason)
        elseif card_num < hc_num then
                room:askForDiscard(player, reason, hc_num-card_num, hc_num-card_num)
        end
		
end
yongheng = sgs.CreateTriggerSkill{
        name = "yongheng",
        events = {sgs.EventPhaseChanging, sgs.CardsMoveOneTime,sgs.EventAcquireSkill},
        frequency = sgs.Skill_Compulsory,
	
		on_trigger = function(self, event, player, data)
                local room = player:getRoom()
				if event == sgs.EventPhaseChanging then
                    local change = data:toPhaseChange()
						if change.to == sgs.Player_Discard then
								touhou_logmessage("#TriggerSkill",player,"yongheng")
								room:notifySkillInvoked(player, self:objectName())
								player:skip(change.to)
								
                        elseif change.to == sgs.Player_NotActive then
                                adjustHandcardNum(player, 4, self:objectName())
                        end
                end
				if event==sgs.CardsMoveOneTime then
						if player:getPhase() ~= sgs.Player_NotActive then return false end
                        local move = data:toMoveOneTime()
                        if (move.from and move.from:objectName() == player:objectName() and move.from_places:contains(sgs.Player_PlaceHand))
                                        or (move.to and move.to:objectName() == player:objectName() and move.to_place == sgs.Player_PlaceHand) then
                                
								adjustHandcardNum(player, 4, self:objectName())
                        end
                end
				if event ==sgs.EventAcquireSkill and data:toString() == "yongheng" then
					if player:getPhase() ~= sgs.Player_NotActive then return false end
					adjustHandcardNum(player, 4, self:objectName())
				end
                return false
        end
}
yongheng1 = sgs.CreateTriggerSkill{ --处理【常识】导致【永恒】可能出现的问题
        name = "#yongheng",
        events = { sgs.EventPhaseChanging},
        frequency = sgs.Skill_Compulsory,
		can_trigger=function(self,palyer)
			return player
		end,
		on_trigger = function(self, event, player, data)
                local room = player:getRoom()
				if event == sgs.EventPhaseChanging then
					local phase = data:toPhaseChange()
					local source = room:findPlayerBySkillName("yongheng")
					local current = room:getCurrent()
					if not source then return false end
					if phase.to==sgs.Player_Start and current:objectName() ~= source:objectName() then
						adjustHandcardNum(source, 4, "yongheng")
					end
				end
        end
}
zhuqu = sgs.CreateTriggerSkill{
                name = "zhuqu$",
        events = sgs.FinishJudge,
        can_trigger = function(self, target)
                if not target or target:isDead() then return false end
                local room = target:getRoom()
                for _,p in sgs.qlist(room:getOtherPlayers(target)) do
                        if p:hasLordSkill(self:objectName()) and p:isWounded() then
                                return true
                        end
                end
                return false
        end,
        on_trigger = function(self, event, player, data)
                if player:getKingdom() ~= "yyc" then return false end
                local room = player:getRoom()
                local judge = data:toJudge()
				if judge.card:getSuit() ~= sgs.Card_Diamond then return false end
                local lords = sgs.SPlayerList()
                for _,p in sgs.qlist(room:getOtherPlayers(player)) do
                        if p:hasLordSkill(self:objectName()) and p:isWounded() then
                                lords:append(p)
                        end
                end
                while not lords:isEmpty() do
                        local target = room:askForPlayerChosen(player, lords, self:objectName(), "@"..self:objectName(), true,true)
                        if not target then return false end
                        room:notifySkillInvoked(target, self:objectName())
						lords:removeOne(target)
                        local recov = sgs.RecoverStruct()
                        recov.recover = 1
                        recov.who = player
                        room:recover(target, recov)
                end
                return false
        end
}

yyc001:addSkill(yongheng)
yyc001:addSkill(yongheng1)
yyc001:addSkill(zhuqu)
extension:insertRelatedSkills("yongheng", "#yongheng")


--【月之头脑——八意永琳】 编号：08002
yyc002 = sgs.General(extension,"yyc002", "yyc",4,false)
ruizhi = sgs.CreateTriggerSkill{
        name = "ruizhi",
        frequency = sgs.Skill_NotFrequent,
        events = {sgs.CardEffected,sgs.PostCardEffected},
        on_trigger = function(self,event,player,data)
            local room = player:getRoom()
            local effect = data:toCardEffect()
			if event==sgs.CardEffected then
				--for ai
				if effect.card and effect.card:isNDTrick() then
					player:setTag("ruizhi_effect",data)
				end
			end
			if event == sgs.PostCardEffected then 
                if effect.card and effect.card:isNDTrick() then
                        if effect.to:objectName() == player:objectName() then
                                if player:isWounded() then
                                        local prompt="invoke:"..effect.card:objectName()
										if room:askForSkillInvoke(player,self:objectName(),sgs.QVariant(prompt)) then
                                                local judge = sgs.JudgeStruct()
                                                judge.who = player
                                                judge.pattern = ".|red"
                                                judge.good = true
                                                judge.reason = self:objectName()
                                                room:judge(judge)
                                                if judge:isGood() then
                                                        local recover = sgs.RecoverStruct()
                                                        recover.recover = 1
                                                        room:recover(player,recover)
                                                end
											
                                        end
                                end
                        end
                end
			end
        end
}

miyaoCard = sgs.CreateSkillCard{
        name = "miyao",
        target_fixed = false,
        will_throw = true,

        filter = function(self,selected,to_select)
                return #selected < 1 and sgs.Self:objectName()~=to_select:objectName() and (not to_select:isKongcheng())
        end,

        on_effect = function (self,effect)
                local room = effect.from:getRoom()
                if effect.to:isWounded() then
					local recover = sgs.RecoverStruct()
					recover.recover = 1
					recover.who=effect.from
					room:recover(effect.to,recover)
				end
                local cards=room:askForExchange(effect.to, self:objectName(), 1, false, "miyao_cardchosen")
				room:throwCard(cards,effect.to)
				
        end
}
miyao = sgs.CreateViewAsSkill{
        name = "miyao",
        n = 0,
        view_as = function (self,cards)
                return miyaoCard:clone()
        end,

        enabled_at_play = function(self, player)
                return not player:hasUsed("#miyao")
        end


}

yyc002:addSkill(ruizhi)
yyc002:addSkill(miyao)


--【蓬莱人之形——藤原妹红】 编号：08003 
yyc003 = sgs.General(extension,"yyc003", "yyc",4,false) 
bumie = sgs.CreateTriggerSkill{
        name = "bumie" ,
        frequency = sgs.Skill_Compulsory ,
        events = {sgs.DamageInflicted, sgs.PreHpLost},--sgs.PreDamageDone, return true 也不行了。。。
        priority = -100 , --优先级需要最低
        on_trigger = function(self, event, player, data)
                local will_losehp = (event == sgs.DamageInflicted) and data:toDamage().damage or data:toInt()
                local hp = player:getHp()
                local room = player:getRoom()
                
				local damage_lose =will_losehp
                if (hp - will_losehp < 1) then
						will_losehp = hp - 1
                end
				touhou_logmessage("#bumie01",player,"bumie",nil,will_losehp)
				room:notifySkillInvoked(player, self:objectName())
                if will_losehp <= 0 then 
					if event == sgs.DamageInflicted then
						damage=data:toDamage()
					end
					return true 
				end
				
				if (event == sgs.DamageInflicted) then
                        local damage = data:toDamage()
                        damage.damage = will_losehp
                        data:setValue(damage)
                else
                        data:setValue(will_losehp) 
                end
                return false
        end
}
bumie1 = sgs.CreateTriggerSkill{
        name = "#bumie1" ,
        events = {sgs.HpChanged,sgs.CardsMoveOneTime} ,
        frequency = sgs.Skill_Compulsory,
on_trigger = function(self, event, player, data)
        local room = player:getRoom()
		local judge=sgs.JudgeStruct()
        judge.who=player
        judge.reason=self:objectName()
        judge.pattern=".|diamond"
        judge.good = true
        judge.negative = true
        
        if event==sgs.HpChanged then
                if player:getHp()==1 and player:isKongcheng() then
                        touhou_logmessage("#TriggerSkill",player,"bumie")
						room:notifySkillInvoked(player, "bumie")
						room:judge(judge)
                        player:obtainCard(judge.card)
                        if not judge:isGood() then
							room:loseMaxHp(player)
							if player:isWounded() then
                                local recover=sgs.RecoverStruct()
                                recover.recover=player:getLostHp()
                                room:recover(player,recover)
							end
						end
				end
        elseif event==sgs.CardsMoveOneTime then
                local move = data:toMoveOneTime()
                if (player:getHp() == 1)
                        and (move.from and move.from:objectName() == player:objectName() and move.from_places:contains(sgs.Player_PlaceHand))and ((not move.to) or (move.to:objectName() ~= player:objectName()) or (move.to_place ~= sgs.Player_PlaceHand))and move.is_last_handcard then
						
						touhou_logmessage("#TriggerSkill",player,"bumie")
                        room:judge(judge)
                        player:obtainCard(judge.card)
                        if not judge:isGood() then
                                room:loseMaxHp(player)
								if player:isWounded() then
									local recover=sgs.RecoverStruct()
									recover.recover=player:getLostHp()
									room:recover(player,recover)
                                end
                        end
                end
        end
        end
}

lizhan = sgs.CreateTriggerSkill{
        name = "lizhan" ,
        events = {sgs.EventPhaseEnd,sgs.CardsMoveOneTime,sgs.EventPhaseChanging},
        can_trigger=function(self,palyer)
			return true
		end,
		on_trigger = function(self, event, player, data)
            local room=player:getRoom()
			local current =room:getCurrent()
			local source=room:findPlayerBySkillName(self:objectName())
			if not source then return false end
			
			--if player:objectName() ~=current:objectName() then return false end 	
			if source:objectName()==current:objectName() then return false end
			if event == sgs.CardsMoveOneTime  then
				
				if current:getPhase() ~=sgs.Player_Discard then return false end
				local move = data:toMoveOneTime()				
				local reason = move.reason
				if  move.to_place == sgs.Player_DiscardPile then	
					--move.from and move.from:objectName() ==current:objectName() 试想一下旋风				
					--单纯排除历战slash本身的话 reason有用
					--if bit32.band(reason.m_reason, sgs.CardMoveReason_S_MASK_BASIC_REASON) == sgs.CardMoveReason_S_REASON_DISCARD then						 
						if player:objectName() ~=current:objectName() then return false end
						local ids =sgs.IntList()
						local ids1=source:getTag("lizhan"):toIntList()
						for _,id in sgs.qlist(move.card_ids) do							
							local card = sgs.Sanguosha:getCard(id)
							if (card:isKindOf("Slash")) and not ids1:contains(id)  then 								
								ids1:append(id)
							end
						end
						local _data=sgs.QVariant()
						_data:setValue(ids1)
						source:setTag("lizhan",_data)
					--end
				end 
			end
			if event== sgs.EventPhaseEnd  and current:getPhase() ==sgs.Player_Discard then
				
					local ids=source:getTag("lizhan"):toIntList()
					if ids:length()==0 then return false end
					local a_data=sgs.QVariant()
					a_data:setValue(current)
					source:setTag("lizhan_target",a_data)
					local prompt="target:"..current:objectName()
					if room:askForSkillInvoke(source,self:objectName(),sgs.QVariant(prompt)) then
						local able=sgs.IntList()
						local disabled=sgs.IntList()
						for _,id in sgs.qlist(ids) do
                            local slash=sgs.Sanguosha:getCard(id)
							
							if source:isCardLimited(slash, sgs.Card_MethodUse) or  not source:canSlash(current,slash,false) then
								disabled:append(id)
							else
								able:append(id)
							end	
						end
						if able:length()==0 then return false end
						room:fillAG(ids,source,disabled)
						local id=room:askForAG(source,able,false,self:objectName())
						room:clearAG()
						local lizhan_slash=sgs.Sanguosha:getCard(id)
						
						source:obtainCard(lizhan_slash,true)
						--lizhan_slash:setSkillName(self:objectName())
						room:useCard(sgs.CardUseStruct(lizhan_slash, source, current),false)
					end
				
			end
			if event== sgs.EventPhaseChanging then
				source:setTag("lizhan",sgs.QVariant())
			end
        end

}

yyc003:addSkill(bumie)
yyc003:addSkill(bumie1)
yyc003:addSkill(lizhan)
extension:insertRelatedSkills("bumie", "#bumie1")


--【狂气的月兔——铃仙•优昙华院•稻叶】 编号：08004
yyc004 = sgs.General(extension,"yyc004", "yyc",3,false) --08004
kuangzaocard=sgs.CreateSkillCard{
name="kuangzao",
target_fixed=false,
will_throw=true,
filter=function(self,targets,to_select)
        return #targets==0 and to_select:inMyAttackRange(sgs.Self) and to_select:objectName()~=sgs.Self:objectName()
end,
on_use = function(self, room, source, targets)
        local prompt = string.format("@kuangzao-slash:%s", source:objectName())
        if not room:askForUseSlashTo(targets[1], source, prompt) then
                local damage=sgs.DamageStruct()
                damage.from=nil
                damage.to=targets[1]
                room:damage(damage)
        end
end
}
kuangzao=sgs.CreateViewAsSkill{
name="kuangzao",
n=0,
view_as=function(self,cards)
        return kuangzaocard:clone()
end,
enabled_at_play=function(self,player)
        return not player:hasUsed("#kuangzao")
end
}

huanshicard=sgs.CreateSkillCard{
name="easthuanshi",
target_fixed=false,
will_throw=false,
handling_method = sgs.Card_MethodNone ,
filter = function(self, targets, to_select, player)
    local huanshiTargets = player:property("huanshi"):toString():split("+")
	return #targets < 10 and table.contains(huanshiTargets, to_select:objectName())
end,
on_use = function(self, room, source, targets)
	for _,p in pairs(targets) do
		p:addMark("huanshi_target")
	end
end
}
huanshivs = sgs.CreateViewAsSkill{
name = "easthuanshi",
n =0,
view_filter = function(self, selected, to_select)
        return true
end,
view_as = function(self, cards)
    local card=huanshicard:clone()
    return card
end,
enabled_at_play=function(self, player)
        return false
end,
enabled_at_response = function(self, player, pattern)
        return pattern=="@@easthuanshi"
end
}
easthuanshi=sgs.CreateTriggerSkill{
name="easthuanshi",
events=sgs.TargetConfirming,
view_as_skill=huanshivs,
on_trigger=function(self,event,player,data)
        local use=data:toCardUse()
        if use.card:isKindOf("Slash") and use.from:objectName()~=player:objectName() and use.to:contains(player) then
                local room=player:getRoom()
                
				local huanshiTargets = {}
                for _, p in sgs.qlist(room:getOtherPlayers(player)) do
					if use.from:canSlash(p,use.card,true) and (not use.to:contains(p)) 
					and use.from:inMyAttackRange(p)  then
						table.insert(huanshiTargets, p:objectName()) 
					end
                end
                if #huanshiTargets > 0 then 
                        room:setPlayerProperty(player, "huanshi", sgs.QVariant(table.concat(huanshiTargets, "+")))
						--for ai
						local _data=sgs.QVariant()
						_data:setValue(use)
						player:setTag("huanshi_source",_data)
						--end
						room:askForUseCard(player, "@@easthuanshi", "@easthuanshi:"..use.from:objectName())
						for _,p in sgs.qlist(room:getOtherPlayers(player)) do
							if p:getMark("huanshi_target")>0 then
								--room:setPlayerMark(p, "huanshi_target", 0)
								use.to:append(p)
							end
						end
						room:setPlayerProperty(player, "huanshi", sgs.QVariant())
                        room:sortByActionOrder(use.to)
                        data:setValue(use)
						--必须重新触发TargetConfirming 
						for _,p in sgs.qlist(room:getOtherPlayers(player)) do
							if p:getMark("huanshi_target")>0 then
								room:setPlayerMark(p, "huanshi_target", 0)
								room:getThread():trigger(sgs.TargetConfirming, room, p, data);
							end
						end
						--player:removeTag("huanshi_source")
						
                end
				
        end
end
}

yyc004:addSkill(kuangzao)
yyc004:addSkill(easthuanshi)


--  【知识与历史的半兽——上白泽慧音】  编号：08005 by三国有单
yyc005 = sgs.General(extension,"yyc005", "yyc",3,false) --08005
timing_xushi = function(room,source,target,xushi_card) --使用时机判断函数
	local result =true
	if source:isProhibited(target, xushi_card) then--都要加禁止使用的判断。。。
		result =false
		return result
	end
	if xushi_card:isKindOf("TrickCard") then
		if source:getPhase()  ~= sgs.Player_Play then  
			result =false
			return result
		end         
	end
	if xushi_card:isKindOf("Peach") then
		if not target:isWounded() then   
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("Analeptic") then
		if target:getHp()>0 or source:getPhase()  ~= sgs.Player_Play then   
			result =false
			return result
		end
	end
	return result
end
count_xushi = function(room,source,target,xushi_card) --使用次数判断函数
	local result =true
	if xushi_card:isKindOf("Slash") then
		if not sgs.Slash_IsAvailable(source) then   
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("Analeptic") then
		if not sgs.Analeptic_IsAvailable(source) then   
			result =false
			return result
		end
	end
	return result
end
distance_xushi = function(room,source,target,xushi_card) --使用距离判断函数
	local result=true
	if xushi_card:isKindOf("Slash") then
		if not source:inMyAttackRange(target) then   
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("Snatch") or xushi_card:isKindOf("SupplyShortage")  then
		if not source:hasSkill("qicai") then
			if source:distanceTo(target) >1 then   
				result =false
				return result
			end
		end
	end
	return result
end
target_xushi = function(room,source,target,xushi_card) --使用目标判断函数
	local result=true
	if xushi_card:isKindOf("Analeptic") or xushi_card:isKindOf("ExNihilo") or xushi_card:isKindOf("Lightning") then
		if source:objectName() ~= target:objectName()  then   
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("Collateral") then
		local ecard =target:getEquip(0) --确认武器
		if ecard  then
		else
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("FireAttack") then
		if target:isKongcheng()  then
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("Dismantlement") or xushi_card:isKindOf("Snatch") then
		if target:isNude() and target:getCards("j"):length() == 0 then
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("Dismantlement") or xushi_card:isKindOf("Snatch") or xushi_card:isKindOf("Duel") 
	  or xushi_card:isKindOf("Slash") or xushi_card:isKindOf("Collateral") or xushi_card:isKindOf("ArcheryAttack")
	or xushi_card:isKindOf("SavageAssault") or xushi_card:isKindOf("SupplyShortage") or xushi_card:isKindOf("Indulgence") then
		if source:objectName() == target:objectName() then
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("TrickCard") and not xushi_card:isNDTrick() then
		for _,card in sgs.qlist(target:getCards("j")) do
            if card:objectName() == xushi_card:objectName() then
                result =false
				return result
            end
        end
	end
	return result
end

--[[sbzhy_xushi = sgs.CreateTriggerSkill{
	name = "sbzhy_xushi",
	events = {sgs.PreCardUsed},
	priority=-1, --注意优先度
	can_trigger = function(self, player)
		return true
	end,
	
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		if event == sgs.PreCardUsed then --还是改成targetconfriming?
			local use = data:toCardUse()
			local skillowner = room:findPlayerBySkillName(self:objectName())
			if not skillowner then return false end
			if use.from:objectName() ~= skillowner:objectName() and  use.to:length()==1  then
				if use.card:isKindOf("TrickCard") or use.card:isKindOf("BasicCard") and (not use.card:isKindOf("Nullification"))  then
					if use.card:hasFlag("xushi") then return false end
					if skillowner:getHandcardNum() ==0 then return false end
					ai_data =sgs.QVariant()				
					local ask_card = room:askForCard(skillowner, "^EquipCard","@xushi",ai_data, sgs.Card_MethodDiscard, nil, true, self:objectName())
					if ask_card then 
						local log=sgs.LogMessage()
						--use.to=sgs.SPlayerList()
						--data:setValue(use)
						
						log.type = "$CancelTargetNoUser"
						log.to = use.to
						log.arg = use.card:objectName()
						room:sendLog(log)
						
						
						log.type = "#xushi_effect"
						log.from = player
						
						log.arg = self:objectName()
						room:notifySkillInvoked(skillowner, self:objectName())
						--room:setCardFlag(use.card, "xushi_nullification")
						--if use.card:getSubcards():length()>0 then
						--	room:throwCard(use.card,use.from, use.from)
						--end
						--targets =sgs.PlayerList()
						if timing_xushi(room,use.from,use.to:first(),ask_card) and
							count_xushi(room,use.from,use.to:first(),ask_card) and
							--ask_card:targetFilter(targets, use.to, use.from)
							distance_xushi(room,use.from,use.to:first(),ask_card) and
							target_xushi(room,use.from,use.to:first(),ask_card) and
							not (use.from:isCardLimited(ask_card, sgs.Card_MethodUse)) then 
						
							--use.card=ask_card
							--data:setValue(use)
							room:sendLog(log)
							
							local carduse=sgs.CardUseStruct()
							ask_card:setFlags("xushi")
							--虚史会掩盖一些原有的skillName 奇迹 导致奇迹的mark加不上去？？ carduse时机在后。
							
							carduse.card=ask_card
							carduse.from=use.from
							carduse.to:append(use.to:first())
							room:useCard(carduse,true)
							
							--return true
						else
							log.type = "#xushi_effect1"
							room:sendLog(log)
						end
						
						--return true
					end
				end
			end
		end
	end
}
]]


sbzhy_xushi = sgs.CreateTriggerSkill{
	name = "sbzhy_xushi",
	events = {sgs.PreCardUsed},
	priority=-1, --注意优先度
	can_trigger = function(self, player)
		return player
	end,
	
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		if event == sgs.PreCardUsed then --还是改成targetconfriming?
			local use = data:toCardUse()
			local skillowner = room:findPlayerBySkillName(self:objectName())
			if not skillowner then return false end
			if use.from:objectName() ~= skillowner:objectName() and  use.to:length()==1  then
				if use.card:isKindOf("TrickCard") or use.card:isKindOf("BasicCard") and (not use.card:isKindOf("Nullification"))  then
					if use.card:hasFlag("xushi") then return false end
					if skillowner:getHandcardNum() ==0 then return false end
					ai_data =sgs.QVariant()				
					local ask_card = room:askForCard(skillowner, "^EquipCard","@xushi",ai_data, sgs.Card_MethodDiscard, nil, true, self:objectName())
					if ask_card then 
						local log=sgs.LogMessage()
						log.type = "#xushi_effect"
						log.from = player
						log.arg = self:objectName()
						room:notifySkillInvoked(skillowner, self:objectName())
						room:setCardFlag(use.card, "xushi_nullification")
						if use.card:getSubcards():length()>0 then
							room:throwCard(use.card,use.from, use.from)
						end
						--targets =sgs.PlayerList()
						if timing_xushi(room,use.from,use.to:first(),ask_card) and
							count_xushi(room,use.from,use.to:first(),ask_card) and
							--ask_card:targetFilter(targets, use.to, use.from)
							distance_xushi(room,use.from,use.to:first(),ask_card) and
							target_xushi(room,use.from,use.to:first(),ask_card) and
							not (use.from:isCardLimited(ask_card, sgs.Card_MethodUse)) then 
						
							--use.card=ask_card
							--data:setValue(use)
							room:sendLog(log)
							
							local carduse=sgs.CardUseStruct()
							ask_card:setFlags("xushi")
							--虚史会掩盖一些原有的skillName 奇迹 导致奇迹的mark加不上去？？ carduse时机在后。
							
							carduse.card=ask_card
							carduse.from=use.from
							carduse.to:append(use.to:first())
							room:useCard(carduse,true)
							
							return true
						else
							log.type = "#xushi_effect1"
							room:sendLog(log)
						end
						
						return true
					end
				end
			end
		end
		--同xushi_eff
		--[[if event == sgs.CardUsed then --sgs.CardEffected
			--local effect = data:toCardEffect()
			local use = data:toCardUse()
			if use.card:hasFlag("xushi_nullification") then
				return true
			end
		end
		if event == sgs.BeforeCardsMove then
			local move = data:toMoveOneTime() 
			if move.to_place==sgs.Player_PlaceDelayedTrick  then
				for _,id in sgs.qlist(move.card_ids) do							
					if sgs.Sanguosha:getCard(id):hasFlag("xushi_nullification") then
						move.to_place=sgs.Player_DiscardPile
						move.to=nil
						data:setValue(move)
						return true
					end
				end	
			end
		end]]
	end
}
sbzhy_xushi_effect = sgs.CreateTriggerSkill{
	name = "#sbzhy_xushi",
	events = {sgs.CardUsed,sgs.BeforeCardsMove},
	priority=3,
	can_trigger = function(self, player)
		return player
	end,
	
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		if event == sgs.CardUsed then 
			local use = data:toCardUse()
			if use.card:hasFlag("xushi_nullification") then
				return true
			end
		end
		if event == sgs.BeforeCardsMove then
			local move = data:toMoveOneTime() 
			if move.to_place==sgs.Player_PlaceDelayedTrick  then
				for _,id in sgs.qlist(move.card_ids) do							
					if sgs.Sanguosha:getCard(id):hasFlag("xushi_nullification") then
						move.to_place=sgs.Player_DiscardPile
						move.to=nil
						data:setValue(move)
						return true
					end
				end	
			end
		end
	end
}

yyc005:addSkill(sbzhy_xushi)
yyc005:addSkill(sbzhy_xushi_effect)
extension:insertRelatedSkills("sbzhy_xushi", "#sbzhy_xushi")

 
 --【幸运的白兔——因幡帝】 编号：08006 
yyc006 = sgs.General(extension,"yyc006", "yyc",3,false) --08006
qizhaCard = sgs.CreateSkillCard{
        name = "qizha" ,
        will_throw = false ,
        handling_method = sgs.Card_MethodNone ,
        filter = function(self, players, to_select,player)
                return (#players < 2) and (not to_select:isKongcheng()) and player:objectName() ~= to_select:objectName()
        end ,
        feasible = function(self, players)
                return #players == 2
        end ,
        on_use = function(self, room, player, players)
                room:moveCardTo(self, nil, sgs.Player_DrawPile)
                local _data = sgs.QVariant()
                _data:setValue(player)
                room:setTag("qizha", _data)
                players[1]:pindian(players[2], "qizha")
                room:setTag("qizha", sgs.QVariant())
        end ,
}
qizhaVS = sgs.CreateOneCardViewAsSkill{
        name = "qizha" ,
        filter_pattern = ".|.|.|hand",
        view_as = function(self, card)
                local c = qizhaCard:clone()
                c:addSubcard(card)
                return c
        end ,
        enabled_at_play = function(self, player)
                return false--not player:hasUsed("#qizha")
        end ,
		enabled_at_response = function(self, player, pattern)  
			return pattern == "@@qizha" 
        end ,
}
qizha = sgs.CreateTriggerSkill{
        name = "qizha" ,
        view_as_skill = qizhaVS ,
        events = {sgs.EventPhaseStart,sgs.Damaged} ,

        on_trigger = function(self, event, player, data)
            if event==sgs.Damaged or (event==sgs.EventPhaseStart and player:getPhase() == sgs.Player_Play) then 
			local room = player:getRoom()
				if not player:isKongcheng() then
					room:askForUseCard(player, "@@qizha", "@qizha") 
				end
			end			
        end ,
}
qizha_effect = sgs.CreateTriggerSkill{
        name = "#qizha" ,
        events = {sgs.Pindian} ,
        can_trigger = function(self, player)
                return player --cost付出后，效果必须执行，不需要验证 player:isAlive()
        end,
        on_trigger = function(self, event, player, data)
                local pindian = data:toPindian()
                if (pindian.reason ~= "qizha") then return false end
                local room = player:getRoom()
                local yinpandi = room:getTag("qizha"):toPlayer()
                
                        local bigger = nil
                        if (pindian.from_number > pindian.to_number) then
                                bigger = pindian.from
                        elseif (pindian.to_number > pindian.from_number) then
                                bigger = pindian.to
                        end
                        if bigger then
                                bigger:drawCards(1)
                                room:damage(sgs.DamageStruct(self:objectName(), nil, bigger))
                        else
                                if yinpandi then
									yinpandi:drawCards(1)
								end
                        end

                return false
        end ,
}

yfd_xingyun = sgs.CreateTriggerSkill{
        name = "yfd_xingyun" ,
        events = {sgs.CardsMoveOneTime} ,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if (room:getTag("FirstRound"):toBool()) then return false end
                
				local move = data:toMoveOneTime()
                if ((not move.from) or (move.from:objectName() ~= player:objectName()))
                                and (move.to and (move.to:objectName() == player:objectName()) and ((move.to_place == sgs.Player_PlaceHand) or (move.to_place == sgs.Player_PlaceEquip))) then
                        for _, id in sgs.qlist(move.card_ids) do
                                if (sgs.Sanguosha:getCard(id):getSuit() == sgs.Card_Heart) then
                                        if player:askForSkillInvoke(self:objectName(), sgs.QVariant(id)) then
                                                local choice = "letdraw"
                                                room:showCard(player, id)
                                                if player:isWounded() then choice = room:askForChoice(player, self:objectName(), "letdraw+recover", sgs.QVariant(id)) end
                                                if (choice == "letdraw") then
                                                        local target = room:askForPlayerChosen(player, room:getAlivePlayers(), self:objectName(), "@xingyun-select")
                                                        target:drawCards(1)
                                                elseif (choice == "recover") then
                                                        local recover = sgs.RecoverStruct()
                                                        recover.who = player
                                                        room:recover(player, recover)
                                                else
                                                        assert(nil)
                                                end
											
                                        end
										
                                end
                        end
                end
                return false
        end ,
}

yyc006:addSkill(qizha)
yyc006:addSkill(qizha_effect)
yyc006:addSkill(yfd_xingyun)
extension:insertRelatedSkills("qizha", "#qizha")
	
	
--【夜雀妖怪——米斯蒂娅•萝蕾拉】 编号：08007 
yyc007 = sgs.General(extension,"yyc007", "yyc",3,false) 
--[[geshengdis = sgs.CreateSkillCard{
        name = "gesheng" ,
        target_fixed = true ,
        on_use = function(self, room, player)
        local realcardid = self:getSubcards():at(0)
        room:setPlayerMark(player, "gesheng", sgs.Sanguosha:getCard(realcardid):isRed() and 1 or 2)
        end ,
}
geshengVS = sgs.CreateOneCardViewAsSkill{
        name = "gesheng" ,
        view_filter = function(self, card)
                if not sgs.Self:hasUsed("#gesheng") then return true end
                if sgs.Self:getMark("gesheng") == 0 then assert(nil) return false end
                local isred = (sgs.Self:getMark("gesheng") == 1)
                return card:isRed() == isred
                end ,
        view_as = function(self, card)
        if not sgs.Self:hasUsed("#gesheng") then
                local song = geshengdis:clone()
                song:addSubcard(card)
                return song
        else
                local indl = sgs.Sanguosha:cloneCard("indulgence", sgs.Card_SuitToBeDecided, -1)
                indl:addSubcard(card)
                indl:setSkillName("gesheng")
                return indl
        end
        return nil
        end ,
}
gesheng = sgs.CreateTriggerSkill{--应该要改为changing 等技能整体调整
        name = "gesheng" ,
        event = {sgs.EventPhaseEnd} ,
        view_as_skill = geshengVS ,
        on_trigger = function(self, event, player, data)
                room:setPlayerMark(player, "gesheng", 0)
        end ,
}
]]

--[[geshengCard = sgs.CreateSkillCard {
	name = "gesheng",

	filter = function(self, targets, to_select, player)
		if #targets == 0 then
			local targets2 = sgs.PlayerList()
			local card = sgs.Sanguosha:getCard(self:getSubcards():first())
			local indulgence = sgs.Sanguosha:cloneCard("indulgence", card:getSuit(), card:getNumber())
			indulgence:addSubcard(card)
			indulgence:setSkillName("gesheng")
			return (indulgence:isAvailable(player) and not player:isProhibited(to_select, indulgence) and indulgence:targetFilter(targets2, to_select, player)) 
		else
			return false
		end
	end,
	
	on_validate = function(self, use)
		if use.to:length() == 1 and not use.to:first():containsTrick("indulgence") then
			local card = sgs.Sanguosha:getCard(self:getSubcards():first())
			local indulgence = sgs.Sanguosha:cloneCard("indulgence", card:getSuit(), card:getNumber())
			indulgence:addSubcard(card)
			indulgence:setSkillName("gesheng")
			use.to:first():drawCards(1)
			return indulgence
		end
		return use.card
	end,
}
]]
geshengVS = sgs.CreateOneCardViewAsSkill{
        name = "gesheng" ,
		filter_pattern = ".|.|.|.!",
        
        view_as = function(self, card)
			--local indl =geshengCard:clone()
			local indl = sgs.Sanguosha:cloneCard("indulgence", card:getSuit(), card:getNumber())
            indl:addSubcard(card)
            indl:setSkillName("gesheng")
            return indl
        end ,
		 enabled_at_play = function(self, player)
                return not player:isKongcheng() 
				--and not player:hasUsed("#gesheng")
				and not player:hasFlag("gesheng")
        end ,
}

gesheng = sgs.CreateTriggerSkill{
    name = "gesheng",
    events = {sgs.EventPhaseChanging,sgs.PreCardUsed,sgs.TargetConfirmed} ,
    view_as_skill = geshengVS ,
		
     on_trigger = function(self, event, player, data)
        local room = player:getRoom()
			 if event==sgs.PreCardUsed then
				local use = data:toCardUse()
				if use.card and use.card:getSkillName()=="gesheng" then
					room:setPlayerFlag(use.from,"gesheng")
				end
			 end
			 if event==sgs.TargetConfirmed then
				local use = data:toCardUse()
				
				if use.card and use.card:getSkillName()=="gesheng" then
					if use.to:length()>0 then
						use.to:first():drawCards(1)
					end
				end
			 end
			 if event==sgs.EventPhaseChanging then
					local change = data:toPhaseChange()
					if change.from ==sgs.Player_Play  then
						room:setPlayerFlag(player,"-gesheng")
					end
			end
    end ,
}
-- 杀使用 -1马的时候要注意
yemang = sgs.CreateProhibitSkill{
        name = "yemang" ,
        is_prohibited = function(self, from, to, card)
            if card:isKindOf("Slash") then
				t=false
				card_ids=card:getSubcards() 
				if card_ids:length()>0 then
					for _,id in sgs.qlist(card_ids) do
						if from:getOffensiveHorse() and from:getOffensiveHorse():getId() ==id then
							t=true
							break
						end
					end
				end
				
				if t then
					return to:hasSkill(self:objectName()) and (from:distanceTo(to,1) >= to:getHp())
				else
					return to:hasSkill(self:objectName()) and (from:distanceTo(to) >= to:getHp())
				end
			end
		end ,
}

yyc007:addSkill(gesheng)
yyc007:addSkill(yemang)

	
--【在黑暗中蠢动的光虫——莉格露•奈特巴格】 编号：08008 
yyc008 = sgs.General(extension,"yyc008", "yyc",3,false) --08008
yingguang_card = sgs.CreateSkillCard{
	name = "yingguang",
	target_fixed = true,
	will_throw = false,
	
	on_use = function(self, room, source, targets)
		local cards= self:getSubcards() 
		local card = sgs.Sanguosha:getCard(cards:first())
		local id=cards:first()
		room:showCard(source,id)
        room:getThread():delay()
		local tag = sgs.QVariant()
		local ids 
		ids=source:getTag("enternal_show"):toIntList()
		if ids:length()>0 then
			ids:append(id)
		else
			ids=sgs.IntList()
			ids:append(id)
		end
        tag:setValue(ids)
        source:setTag("enternal_show", tag)
		source:gainMark("@yingguang")
		
		if not source:hasSkill("#eternalshow_handle") then
			room:acquireSkill(source,"#eternalshow_handle")
		end
		for _,p in sgs.qlist(room:getAllPlayers()) do
			if not p:hasSkill("eternalshow") then
				room:attachSkillToPlayer(p, "eternalshow",true)
			end
		end
	end,
}
yingguang_vs = sgs.CreateViewAsSkill{
        name = "yingguang",
        n = 1,
		view_filter = function(self, selected, to_select)
			return (not to_select:isEquipped())  and to_select:getSuit() ==sgs.Card_Diamond
		end,
		
        view_as = function (self,cards)
            if #cards==1 then    
				local card =yingguang_card:clone()
				card:addSubcard(cards[1])
				card:setSkillName(self:objectName())
				return card
			end
        end,

        enabled_at_play = function(self, player)
           -- ids =player:getTag("enternal_show"):toIntList()
			if player:getMark("@yingguang")==0 then
			--if ids:length()==0 then
				return true
			end
        end
}
yingguang =sgs.CreateTriggerSkill{
	name="yingguang",
	events = {sgs.CardAsked},
	view_as_skill=yingguang_vs,

	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local pattern = data:toStringList()[1]
		if pattern ~= "jink" then return false end
		if player:getMark("@yingguang")==0 then return false end
		local prompt="invoke"
		if not player:askForSkillInvoke("yingguang", sgs.QVariant(prompt)) then return false end
		room:getThread():delay()
		local Jink = sgs.Sanguosha:cloneCard("jink",sgs.Card_NoSuit, 0)
		Jink:setSkillName("_yingguang")
		room:provide(Jink)
	end,
}

--[源码改动]为持续展示  修改room.cpp的函数askforcardchosen
eternalshow_card = sgs.CreateSkillCard{
	name = "eternalshow",
	target_fixed = false,
	will_throw = false,
	filter = function(self, targets, to_select, player)
		 return to_select:getMark("@yingguang")>0 or  to_select:getMark("@eyun")>0
	end,
	
	on_use = function(self, room, source, targets)
		local target = targets[1]
		local ids=target:getTag("enternal_show"):toIntList()
		if ids:length()>0 then
			room:fillAG(ids, source)
			local card_id = room:askForAG(source, ids, true, "eternalshow_chose")
			room:clearAG(source)
		end
	end,
}
eternalshow= sgs.CreateViewAsSkill{
        name = "eternalshow",
        n = 0,

        view_as = function (self,cards) 
			local card =eternalshow_card:clone()
			return card
        end,

        enabled_at_play = function(self, player)
				return true
        end
}
eternalshow_handle =sgs.CreateTriggerSkill{
	name="#eternalshow_handle",
	events = {sgs.CardsMoveOneTime},--sgs.BeforeCardsMove

	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local move = data:toMoveOneTime()
		
		if  move.from_places:contains(sgs.Player_PlaceHand) then
			if not player:hasSkill("#eternalshow_handle") then return false end
			if move.from:objectName() ~=player:objectName() then return false end
			local ids=player:getTag("enternal_show"):toIntList()
			
			if ids:length()==0 then return false end --减少计算量
			local can_yingguang =false 
			if player:hasSkill("yingguang") then
				can_yingguang =true
			end
			
			
			local hina = room:findPlayerBySkillName("zaihuo")
			local can_zaihuo=false
			local e_ids
			local current =room:getCurrent()
			if hina then
				e_ids=hina:getTag("e_ids"):toIntList()
				if e_ids:length()>0 then
					can_zaihuo =true
				end
			end
			
			local count =0--移走后的方块数
			local delete_ids=sgs.IntList()
			local delete_eids=sgs.IntList()
			for _,cardid in sgs.qlist(move.card_ids) do
				for _,id in sgs.qlist(ids) do
					if cardid ==id then
						delete_ids:append(id)
					end
				end
			end
			if can_zaihuo then
				e_ids=hina:getTag("e_ids"):toIntList()
				for _,cardid in sgs.qlist(move.card_ids) do
					for _,eid in sgs.qlist(e_ids) do
						if cardid ==eid then
							delete_eids:append(eid)
						end
					end
				end
			end
			for _,delete_id in sgs.qlist(delete_ids) do
				ids:removeOne(delete_id)
			end
			local tag = sgs.QVariant()
			tag:setValue(ids)
			player:setTag("enternal_show", tag)
			for _,id in sgs.qlist(ids) do
				local card = sgs.Sanguosha:getCard(id)
				if card and card:getSuit()==sgs.Card_Diamond then
					count=count+1
				end
			end
			--取消荧光标记
			if count==0 and player:getMark("@yingguang")>0 then
				room:setPlayerMark(player,"@yingguang",0)
			end
			
			--执行【灾祸】效果
			if can_zaihuo then
				local ecount=0
				for _,delete_eid in sgs.qlist(delete_eids) do
					e_ids:removeOne(delete_eid)
					ecount=ecount+1
				end
				local tag1 = sgs.QVariant()
				tag1:setValue(e_ids)
				hina:setTag("e_ids", tag1)
				if ecount>0 then	
					room:setPlayerMark(player,"@eyun",player:getMark("@eyun")-ecount)				
				end
				if  current:objectName()==player:objectName() then
					for var=1, ecount, 1 do
						local _data = sgs.QVariant()
						_data:setValue(player)
						hina:setTag("zaihuo_target",_data)
						local prompt="invoke:"..player:objectName()
						if not hina:askForSkillInvoke("zaihuo", sgs.QVariant(prompt)) then break end
						local damage=sgs.DamageStruct()
						damage.from=nil
						damage.to=player
						room:damage(damage)
					end
				end
			end
		end
	end,
}

yangchong=sgs.CreateTriggerSkill{
	name="yangchong",
	events = {sgs.CardResponded},

	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local card_star = data:toCardResponse().m_card
		if (card_star:isKindOf("Jink")) then
			local target = room:askForPlayerChosen(player, room:getOtherPlayers(player), self:objectName(), "@yangchong-invoke", true, true) 
			if target then
				if  target:getHandcardNum()<2 or (not room:askForDiscard(target,"yangchong",2,2,true,false,"yangchong_discard:"..player:objectName())) then
					local damage=sgs.DamageStruct()
                    damage.from=nil
                    damage.to=target
					damage.reason="yangchong"
                    room:damage(damage)
				end
			end
		end
	end
}
 
if not sgs.Sanguosha:getSkill("#eternalshow_handle") then
        local skillList=sgs.SkillList()
        skillList:append(eternalshow_handle)
        sgs.Sanguosha:addSkills(skillList)
end
if not sgs.Sanguosha:getSkill("eternalshow") then
        local skillList=sgs.SkillList()
        skillList:append(eternalshow)
        sgs.Sanguosha:addSkills(skillList)
end
 yyc008:addSkill(yingguang)
 yyc008:addSkill(yangchong)
 
 
--【治世之兆——白泽】 编号：08009  --by三国有单
yyc009 = sgs.General(extension,"yyc009", "yyc",3,false) 
function targetsTable2QList(thetable)
        local theqlist = sgs.PlayerList()
        for _, p in ipairs(thetable) do
                theqlist:append(p)
        end
        return theqlist
end
chuangshi_choice =function(target)
	local choice_str={"cancel"}
	local room=target:getRoom()
	local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
    if not target:isCardLimited(slash, sgs.Card_MethodUse) then
		local t=false
		for _,p in sgs.qlist(room:getOtherPlayers(target)) do
			if target:canSlash(p,slash,true) then
				t=true
				break
			end
		end
		if t then
			table.insert(choice_str,"slash")		
			table.insert(choice_str,"fire_slash")		
			table.insert(choice_str,"thunder_slash")
		end
	end
	slash = sgs.Sanguosha:cloneCard("peach", sgs.Card_NoSuit, 0)
	if target:isWounded() and not target:isCardLimited(slash, sgs.Card_MethodUse)  then--桃
		table.insert(choice_str,"peach")
	end
	local trick_strs={"amazing_grace","duel","ex_nihilo","snatch","dismantlement","collateral","iron_chain","fire_attack"}
	for _,trick_str in pairs(trick_strs) do
		slash = sgs.Sanguosha:cloneCard(trick_str, sgs.Card_NoSuit, 0)
        if not target:isCardLimited(slash, sgs.Card_MethodUse) then
			if trick_str =="amazing_grace" or trick_str =="ex_nihilo" then
				table.insert(choice_str,trick_str)
			else	
				
				local listt= sgs.PlayerList()
				for _,p in sgs.qlist(room:getAlivePlayers()) do
					if slash:targetFilter(listt, p, target) then
						table.insert(choice_str,trick_str)
						break
					end
				end
			end
		end
	end
	local choices=table.concat(choice_str, "+")
	return 	choices
end
chuangshiCard = sgs.CreateSkillCard{
    name = "chuangshiCard",
    target_fixed = false,
    will_throw = false,
    filter = function(self, targets, to_select, player)
       local str = player:property("chuangshi"):toString()
       local card =sgs.Sanguosha:cloneCard(str, sgs.Card_NoSuit, 0)		
	   local users=player:getAliveSiblings()
	   local user
	   for _, p in sgs.qlist(users) do
			if p:getMark("chuangshi_user") >0 then 
				user=p
				break
			end
	   end
	   if not user then return false end
	   if sgs.Sanguosha:isProhibited(user, to_select, card) then return false end
	   return card:targetFilter(targetsTable2QList(targets), to_select, user)
    end,
	
	feasible = function(self, targets, player)
        if #targets == 0 then return false end
        local str = player:property("chuangshi"):toString()
		local card =sgs.Sanguosha:cloneCard(str, sgs.Card_NoSuit, 0)
        local users=player:getAliveSiblings()
	   local user
	   for _, p in sgs.qlist(users) do
			if p:getMark("chuangshi_user") >0 then 
				user=p
				break
			end
	   end
		if not user then return false end
		for _, p in ipairs(targets) do
            if sgs.Sanguosha:isProhibited(user, p, card) then return false end
        end
        return card:targetsFeasible(targetsTable2QList(targets), user)
    end,
	
	about_to_use = function(self, room, cardUse)
    --on_use里target顺序被sort，不能反映UI时选择顺序
		local from = cardUse.from
		local str = from:property("chuangshi"):toString()
		local card =sgs.Sanguosha:cloneCard(str, sgs.Card_NoSuit, 0)
		if  card:isKindOf("Collateral") then 
			local users=room:getOtherPlayers(from)
			local user
			for _, p in sgs.qlist(users) do
				if p:getMark("chuangshi_user") >0 then 
					user=p
					break
				end
			end
			if not user then return false end 
			room:setPlayerMark(user, "chuangshi_user", 0)
			touhou_logmessage("#ChoosePlayerWithSkill",from,"chuangshi",cardUse.to,nil,1)
	
			card:setSkillName("_chuangshi")--ai用得着
			local use=sgs.CardUseStruct()
			use.from=user
			use.to=cardUse.to
			use.card=card
			room:useCard(use)
		else
			self:cardOnUse(room, cardUse)--声明about to use 后需要引用onuse
		end
	end,
	--借刀的处理放在about_to_use里
    on_use = function(self, room, source, targets)
        local str = source:property("chuangshi"):toString()
		local card =sgs.Sanguosha:cloneCard(str, sgs.Card_NoSuit, 0)
        if card:isKindOf("Collateral") then return false end
		local users=room:getOtherPlayers(source)
	   local user
	   for _, p in sgs.qlist(users) do
			if p:getMark("chuangshi_user") >0 then 
				user=p
				break
			end
	   end
	   if not user then return false end
	   room:setPlayerMark(user, "chuangshi_user", 0)
	   local carduse=sgs.CardUseStruct()
		card:setSkillName("_chuangshi")--ai用得着
		carduse.card=card
		carduse.from=user
		for var=1, #targets,1 do
			carduse.to:append(targets[var])
		end
		
		room:sortByActionOrder(carduse.to)
		room:useCard(carduse,true)
    end
}
chuangshiVS = sgs.CreateViewAsSkill{
                name = "chuangshi",
                n = 0,
                view_filter = function(self, selected, to_select)
					return  true
                end,
                view_as = function(self, cards)          
                    local card = chuangshiCard:clone()
                    return card
                end,
				
                enabled_at_play = function(self, player)
                    return false
                end,
				
				enabled_at_response = function(self, player, pattern)
					return pattern == "@@chuangshi"
				end,
}
use_chuangshi = function(room,player)
		local target = room:askForPlayerChosen(player, room:getOtherPlayers(player), "chuangshi","@chuangshi_target:"..tostring(player:getMark("chuangshi")+1),true,true)
		if target then
			local ask_str=chuangshi_choice(target)
			
			local choice = room:askForChoice(player,"chuangshi",ask_str)
			if choice=="cancel" then
				use_chuangshi(room,player)
			else
				local card =sgs.Sanguosha:cloneCard(choice, sgs.Card_NoSuit, 0)
				local carduse=sgs.CardUseStruct()
				card:setSkillName("_chuangshi")--ai用得着
				carduse.card=card
				carduse.from=target
				if choice =="amazing_grace" or choice =="ex_nihilo" or choice =="peach"then
					--for ai
					local _data = sgs.QVariant()
					_data:setValue(player)
					room:setTag("chuangshi_source",_data)
					--
					room:useCard(carduse,true)
					return true
				else
					room:setPlayerProperty(player, "chuangshi", sgs.QVariant(card:objectName()))
					room:setPlayerMark(target, "chuangshi_user", 1)
						
					local c_card =room:askForUseCard(player,"@@chuangshi", "@chuangshi_victim:"..target:objectName()..":"..choice)
					if c_card then
						return true
					else
						room:setPlayerMark(target, "chuangshi_user", 0)
						use_chuangshi(room,player)
					end
					return false
				end	
			end
			return false
		end
	return false
end
chuangshi=sgs.CreateTriggerSkill{
	name="chuangshi",
	events={sgs.DrawNCards,sgs.EventPhaseEnd},
	view_as_skill = chuangshiVS,
	on_trigger = function(self, event, player, data)
    local room=player:getRoom()
		if event==sgs.DrawNCards then
			
			if use_chuangshi(room,player) then
				player:addMark("chuangshi",1)
				data:setValue(data:toInt()-1)
				if use_chuangshi(room,player) then
					player:addMark("chuangshi",1)
					data:setValue(data:toInt()-1)
				end
			end
		end
		if event==sgs.EventPhaseEnd and player:getPhase()  == sgs.Player_Draw then
			if player:getMark("chuangshi")>1 then
				room:loseHp(player,2)
			end
			if player:getMark("chuangshi")>0 and player:hasSkill("chuanghuan") then
				room:handleAcquireDetachSkills(player,"-chuangshi")
			end
			room:setPlayerMark(player, "chuangshi", 0)
		end
	end
}

wangyue = sgs.CreateTriggerSkill{
	name = "wangyue", 
	events = {sgs.Damaged},
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		local damage = data:toDamage()
		local from = data:toDamage().from
		if from and from:isAlive() then
			if from:getHandcardNum() > player:getHp() then
				local _data = sgs.QVariant()
                _data:setValue(from)
				player:setTag("wangyue_target",_data)
				prompt="target:"..from:objectName().."::"..tostring(player:getHp())
				if room:askForSkillInvoke(player,self:objectName(),sgs.QVariant(prompt)) then	
					local x =from:getHandcardNum() - player:getHp()
					room:askForDiscard(from,"gamerule",x,x,false,false)
				end
			end
		end				
	end
}
 yyc009:addSkill(chuangshi)
 yyc009:addSkill(wangyue)


 --【红之自警队——藤原妹红】 编号：08010 --by三国有单
yyc010 = sgs.General(extension,"yyc010", "yyc",4,false) 
targetChoiceForHuwei = function(room,player,skillname,num) 
	local target = room:askForPlayerChosen(player, room:getOtherPlayers(player), skillname,"@tymh_huwei_targetdraw:"..tostring(num),true,true)--选择的目标
	if target then
		target:drawCards(2)
		return true
	else 
		return false
	end
	
end
tymh_huwei_card = sgs.CreateSkillCard{ 
	name = "tymh_huwei",
	target_fixed = true,
	will_throw = true,	

	 on_validate = function(self, cardUse)
        local mouko = cardUse.from
        local room = cardUse.from:getRoom()
		touhou_logmessage("#InvokeSkill",mouko,"tymh_huwei")
		room:notifySkillInvoked(mouko, self:objectName())
		mouko:drawCards(2)
		room:setPlayerFlag(mouko, "Global_huweiFailed")
		
        return nil
    end ,
}
tymh_huweivs = sgs.CreateViewAsSkill{
	name = "tymh_huwei" ,
	n = 0,
	
	view_as = function(self, cards)
		local qcard = tymh_huwei_card:clone()
		qcard:setSkillName(self:objectName())
		return qcard
	end,
			
	enabled_at_play = function(self, player)
		return false
	end ,
	enabled_at_response = function(self, player, pattern)
		return (not player:hasFlag("Global_huweiFailed"))and (pattern == "slash") and (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) 		
	end ,
}
tymh_huwei = sgs.CreateTriggerSkill{ 
	name = "tymh_huwei",
	events = {sgs.CardAsked,sgs.CardUsed,sgs.CardsMoveOneTime,sgs.BeforeCardsMove,sgs.CardResponded},
	view_as_skill = tymh_huweivs,
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		if (player:getPhase()  == sgs.Player_Play)then return false end --要求出牌阶段以外
		if event ==sgs.CardAsked  then
			local pattern = data:toStringList()[1]
			if(pattern == "slash") then
				if(not room:askForSkillInvoke(player, self:objectName(),data)) then return false end
				player:drawCards(2)				
			end
		end
		--由此开始时让别人摸牌部分
		if event ==sgs.CardUsed then
			local card_use = data:toCardUse()
			if (not card_use.card:isKindOf("Slash"))  then return false end
			targetChoiceForHuwei(room,player,self:objectName(),1) --自定义函数
		end  --
		if event ==sgs.CardResponded then
			local card_use = data:toCardResponse()
			if (not card_use.m_card:isKindOf("Slash"))  then return false end
			targetChoiceForHuwei(room,player,self:objectName(),1) --自定义函数
		end
		if event == sgs.CardsMoveOneTime or event == sgs.BeforeCardsMove then
			if event == sgs.BeforeCardsMove then
				local move = data:toMoveOneTime()				
				reason = move.reason
				if  move.from and move.to_place == sgs.Player_DiscardPile and move.from:objectName() ==player:objectName()then					
					if bit32.band(reason.m_reason, sgs.CardMoveReason_S_MASK_BASIC_REASON) == sgs.CardMoveReason_S_REASON_DISCARD then						 
						
						for _,id in sgs.qlist(move.card_ids) do							
							local card = sgs.Sanguosha:getCard(id)
							if (card:isKindOf("Slash"))  then 								
								player:addMark(self:objectName())
								
							end	
						end
					end
				end 
			else
				local n = player:getMark(self:objectName())
				while player:getMark(self:objectName())>0 do 	
					player:removeMark(self:objectName())
					if targetChoiceForHuwei(room,player,self:objectName(),n) then--自定义函数
						n=n-1
					else
						room:setPlayerMark(player, self:objectName(), 0)
						break
					end
				end
			end
		end
	end
}

jinxi_card = sgs.CreateSkillCard{
	name = "jinxi",
	target_fixed = true,
	will_throw = true,	
	
	on_use = function(self, room, source, targets)
		local target = targets[1]
		local x = source:getMaxHp() - source:getHp()   
		local recov = sgs.RecoverStruct()
		recov.card = self
		recov.recover = x
		recov.who = source
		room:recover(source, recov)
		if source:getHandcardNum()<4 then source:drawCards(4- source:getHandcardNum()) end
		
		room:removePlayerMark(source, "@jinxi")
	end,

}
jinxivs= sgs.CreateZeroCardViewAsSkill {
	name = "jinxi",
	
	enabled_at_play = function(self,player)
		return  player:getMark("@jinxi")>0 and  player:isWounded()
	end,
	view_as = function(self, cards)
		local qcard = jinxi_card:clone()
		qcard:setSkillName(self:objectName())
		return qcard
	end
}
jinxi = sgs.CreateTriggerSkill{
	name = "jinxi", 
	frequency = sgs.Skill_Limited,
	view_as_skill = jinxivs,
	limit_mark = "@jinxi",
	events = {sgs.GameStart},  
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if event ==sgs.GameStart then
		end
	end
}

 yyc010:addSkill(tymh_huwei)
 yyc010:addSkill(jinxi)


