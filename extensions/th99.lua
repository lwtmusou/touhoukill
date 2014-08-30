module("extensions.th99", package.seeall)
extension = sgs.Package("th99")
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
--技能代码   外传
------------------------------------------------------------------------------------------------------------	
--【幻想乡的记忆——稗田阿求】 编号：99001 
wai001 = sgs.General(extension,"wai001$","wai",3,false) 
qiuwenCard = sgs.CreateSkillCard{
        name = "qiuwen",
        target_fixed = true,
        will_throw = true,

        on_use = function(self,room,source,targets)
                room:drawCards(source,3)
        end
}

qiuwen = sgs.CreateViewAsSkill{
        name = "qiuwen",
        n = 0,
        view_as = function (self,card)
                local card = qiuwenCard:clone()
                return card
        end,
        enabled_at_play=function(self,target)
                return not target:hasUsed("#qiuwen")
        end
}

zaozu = sgs.CreateTriggerSkill{
        name = "zaozu" ,
        frequency = sgs.Skill_Compulsory ,
        events = {sgs.EventPhaseChanging, sgs.EventPhaseStart} ,
        on_trigger = function(self, event, player, data)
                local room=player:getRoom()
				if event == sgs.EventPhaseChanging then
                        local change = data:toPhaseChange()
                        if change.to == sgs.Player_Discard then
										room:notifySkillInvoked(player, self:objectName())

										touhou_logmessage("#TriggerSkill",player,self:objectName())
								player:skip(sgs.Player_Discard)
                        end
                else
                        if player:getPhase() == sgs.Player_Finish then
                                if player:getHandcardNum() > player:getMaxHp() then

										room:broadcastSkillInvoke(self:objectName())
										room:notifySkillInvoked(player, self:objectName())
										
										touhou_logmessage("#TriggerSkill",player,self:objectName())
										player:getRoom():loseHp(player)
                                end
                        end
                end
                return false
        end ,
}
dangjia_card = sgs.CreateSkillCard{
	name = "dangjiavs",
	target_fixed = false,
	will_throw = false,
	
	filter = function(self, targets, to_select, player)
		return #targets == 0 and (not to_select:hasFlag("Forbiddangjia") )and to_select:hasLordSkill("dangjia") and (not to_select:isKongcheng() ) and to_select:isWounded()
	end,
	
	on_use = function(self, room, source, targets)
		local target = targets[1]
		if (target:hasLordSkill("dangjia")) then
			if (source:pindian(target, "dangjia", nil)) then
			else	
				local recov = sgs.RecoverStruct()
				recov.card = nil
				recov.who = source
				room:recover(target, recov)			
			end
			room:setPlayerFlag(target, "Forbiddangjia")
			room:setPlayerFlag(source, "dangjiaInvoked")
		end
	end,
	
}

dangjiavs = sgs.CreateViewAsSkill{
	name = "dangjiavs",
	n = 0,
	
	enabled_at_play = function(self,target)
		return not target:isKongcheng()  and not target:hasFlag("dangjiaInvoked") and target:getKingdom()=="wai" 
	end,
	
	view_filter = function(self, selected, to_select)
		return false
	end,
	
	view_as = function(self, cards)
		local qcard = dangjia_card:clone()
		qcard:setSkillName(self:objectName())
		return qcard
	end
}
dangjia = sgs.CreateTriggerSkill{
	name = "dangjia$",
	events = {sgs.GameStart, sgs.EventPhaseChanging,sgs.EventAcquireSkill ,sgs.EventLoseSkill},
	can_trigger = function(self, target)
        return target
    end,
	
	on_trigger = function(self, event, player, data)
		--if not player then return false end
		local room = player:getRoom()
		
		if event == sgs.GameStart or (event == sgs.EventAcquireSkill and data:toString() == self:objectName()) then
                    local lords=sgs.SPlayerList()
					for _,p in sgs.qlist(room:getAlivePlayers()) do
						if p:hasLordSkill(self:objectName()) then
							lords:append(p)
						end
					end
					if lords:length()==0 then return false end
					local others=sgs.SPlayerList()
					if lords:length()==1 then
						others= room:getOtherPlayers(lords:first())
					else
						others= room:getAlivePlayers()
					end
					for _,p in sgs.qlist(others) do
						if not p:hasSkill("dangjiavs") then
							room:attachSkillToPlayer(p, "dangjiavs",true)													
						end
					end         
		end
		if (event == sgs.EventLoseSkill and data:toString() == "dangjia") then
					local room = player:getRoom()
					local lords=sgs.SPlayerList()
				
					for _,p in sgs.qlist(room:getAlivePlayers()) do
						if p:hasLordSkill(self:objectName()) then
							lords:append(p)
						end
					end
					if lords:length()>2 then return false end
					local others=sgs.SPlayerList()
					if lords:length()==0 then
						others= room:getAlivePlayers()
					else
						others:append(lords:first())
					end	
					for _,p in sgs.qlist(others) do
						if  p:hasSkill("dangjiavs") then
							room:detachSkillFromPlayer(p, "dangjiavs",true)													
						end
					end
		end

		if event == sgs.EventPhaseChanging then
            local phase_change = data:toPhaseChange()
				if phase_change.from == sgs.Player_Play then
					local players = room:getAlivePlayers()
					
                    for _,p in sgs.qlist(players) do
                        if p:hasFlag("dangjiaInvoked") then
                           room:setPlayerFlag(p, "-dangjiaInvoked")
                        end
						if p:hasFlag("Forbiddangjia") then
							room:setPlayerFlag(p, "-Forbiddangjia")
						end
                    end
				end
		end
	end
}
if not sgs.Sanguosha:getSkill("dangjiavs") then
        local skillList=sgs.SkillList()
        skillList:append(dangjiavs)
        sgs.Sanguosha:addSkills(skillList)
end
wai001:addSkill(qiuwen)
wai001:addSkill(zaozu)
wai001:addSkill(dangjia)
  
  
 --【古道具店主——森近霖之助】 编号：99002 
wai002 = sgs.General(extension, "wai002", "wai", 4) 
sjlzzxiufuCard = sgs.CreateSkillCard{
        name = "sjlzzxiufu" ,
        target_fixed = true ,
        about_to_use = function(self, room, cardUse)
                local discardpile = room:getDiscardPile()
                for _, id in sgs.qlist(discardpile) do
                        if sgs.Sanguosha:getCard(id):isKindOf("EquipCard") then
                                self:cardOnUse(room, cardUse)
                                return
                        end
                end
        end ,
        on_use = function(self, room, source)
                local able = sgs.IntList()
                local discardpile = room:getDiscardPile()
                for _, id in sgs.qlist(discardpile) do
                        tmp_card=sgs.Sanguosha:getCard(id)
						if tmp_card:isKindOf("EquipCard")  then
                                able:append(id)
                        end
						--and not tmp_card:objectName():startsWith("renou")
                end
                if able:isEmpty() then return end
                room:fillAG(able, source)
                local equipid = room:askForAG(source, able, false, self:objectName())
                local equip = sgs.Sanguosha:getCard(equipid):getRealCard():toEquipCard()
                local target = room:askForPlayerChosen(source, room:getAlivePlayers(), self:objectName(), "@sjlzzxiufu-select:" .. equip:objectName(), false, true)
                room:clearAG(source)

                local equipped_id = -1
                if target:getEquip(equip:location()) then
                        equipped_id = target:getEquip(equip:location()):getEffectiveId()
                end
                local exchangeMove = sgs.CardsMoveList()
                if equipped_id ~= -1 then
                        local move2 = sgs.CardsMoveStruct(equipped_id, nil, sgs.Player_DiscardPile,
                                        sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_CHANGE_EQUIP, target:objectName()))
                        exchangeMove:append(move2)
				end
                local move1 = sgs.CardsMoveStruct(equipid, target, sgs.Player_PlaceEquip,
                                sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_TRANSFER, source:objectName()))	
				--S_REASON_PUT
                exchangeMove:append(move1)

                room:moveCardsAtomic(exchangeMove, true)
        end ,
}
sjlzzxiufu = sgs.CreateZeroCardViewAsSkill{
        name = "sjlzzxiufu" ,
        view_as = function(self)
                return sjlzzxiufuCard:clone()
        end ,
        enabled_at_play = function(self, player)
                return not player:hasUsed("#sjlzzxiufu")
        end ,
}

wai002:addSkill(sjlzzxiufu)                
	
	
--【无名的读书妖怪——朱鹭子】 编号：99003 by三国有单
wai003 = sgs.General(extension,"wai003","wai",3,false) --99003
fandu_func = function(room,player)	
	player:drawCards(2)
	local target = room:askForPlayerChosen(player, room:getOtherPlayers(player), "fandu","@fandu-select")--, true, true
	if target then
		
		local to_throw = room:askForCardChosen(target, player, "h", "fandu",false,sgs.Card_MethodDiscard)		
		room:throwCard(to_throw,player, target)
	end
end
fandu = sgs.CreateTriggerSkill{
	name = "fandu", 
	events = {sgs.EventPhaseStart,sgs.Damaged},  
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		local damage =data:toDamage()
		if(player:getPhase() == sgs.Player_Start)  then	
			if(not room:askForSkillInvoke(player, "fandu")) then return false end--选择是否发动技能
			fandu_func(room,player) 
		end
		if (event == sgs.Damaged) then
			for var = 1, damage.damage, 1 do
				if(not room:askForSkillInvoke(player, "fandu")) then return false end--选择是否发动技能
				fandu_func(room,player)
			end
		end
	end
}
--讨还的mark可能和死欲冲突吗？
taohuan = sgs.CreateTriggerSkill{
	name = "taohuan",
	events = {sgs.CardsMoveOneTime},

	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		local tag = sgs.QVariant()
		if  event ==sgs.CardsMoveOneTime then 
			if  player:hasFlag("pindian") then return false end	--排除正在讨还拼点中。	
			
			move = data:toMoveOneTime()
			if not player:hasSkill(self:objectName()) then return false end

			--1:牌被人弃置的情况
			if bit32.band(move.reason.m_reason, sgs.CardMoveReason_S_MASK_BASIC_REASON) == sgs.CardMoveReason_S_REASON_DISCARD then
				if not move.from then return false end
				if move.from:objectName() ~=player:objectName()then return false end	
				local target
				for _,p in sgs.qlist(room:getAlivePlayers()) do
					if p:objectName() == move.reason.m_playerId then
						target=p
						break
					end
				end				
				if target and target:objectName() ~=player:objectName() then
					for _,id in sgs.qlist(move.card_ids)  do
						if move.from and move.from:objectName() ~=player:objectName() then return false end
						if move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceHand or 
							move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceEquip
							then
							local card = sgs.Sanguosha:getCard(id)	
							if not card:isKindOf("BasicCard") then  
								markname="taohuantarget"..player:objectName()
								room:setPlayerMark(target, markname, target:getMark(markname)+1)
								room:setPlayerMark(player, "taohuan"..target:objectName(),1)
							end
						end
					end
				end
			end
			--2:牌被别人获得的情况
			--move.to不能触发突袭。。。
			--move.origin_to兼顾顺手和突袭
			--move.to_place == sgs.Player_PlaceTable and
			if  move.origin_to and move.origin_to:objectName() ~= player:objectName() and move.origin_to_place == sgs.Player_PlaceHand  then	
				if not move.from then return false end
				if move.from:objectName() ~=player:objectName()  then return false end    
				for _,p in sgs.qlist(room:getOtherPlayers(player)) do
					if p:objectName() == move.origin_to:objectName() then
						target=p
						break
					end
				end	
				
				local show_list=player:getTag("taohuan"..move.origin_to:objectName()):toIntList()
				for _,id in sgs.qlist(move.card_ids) do																		
					if room:getCardPlace(id) == sgs.Player_PlaceHand then
					--到了对面的手牌里
						if move.from_places:at(move.card_ids:indexOf(id)) ==sgs.Player_PlaceTable
						or move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceHand 
						or	move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceEquip
							then	
							
							local card = sgs.Sanguosha:getCard(id)
							if not card:isKindOf("BasicCard") then
								markname="taohuantarget"..player:objectName()
								room:setPlayerMark(target, markname, target:getMark(markname)+1)
								room:setPlayerMark(player, "taohuan"..target:objectName(),1)
							
								if move.from_places:at(move.card_ids:indexOf(id)) ~= sgs.Player_PlaceEquip then
									show_list:append(id)
								end
							end
						end
					end
				end
				tag:setValue(show_list)
				player:setTag("taohuan"..target:objectName(), tag)

			end
			
			
			
			--执行讨还
			for _,p in sgs.qlist(room:getOtherPlayers(player)) do
				if player:getMark("taohuan"..p:objectName())>0 then
					target=p
					room:setPlayerMark(player, "taohuan"..p:objectName(),0)
					show_ids=player:getTag("taohuan"..p:objectName()):toIntList()
					show_ids1=sgs.IntList()
					tag:setValue(show_ids1)
					player:setTag("taohuan"..p:objectName(), tag)
					--ai
					local _data = sgs.QVariant()
					_data:setValue(target)
					player:setTag("taohuantarget", _data)
					--
					prompt="show:"..target:objectName()
					if show_ids:length()>0 then
						if (not room:askForSkillInvoke(player,"taohuanshow", sgs.QVariant(prompt))) then 
							return false
						end
						for _,id in sgs.qlist(show_ids) do
							room:showCard(target,id)  
						end
					end
					
					
					while target:getMark("taohuantarget"..player:objectName())>0 do
						
						if target:getHandcardNum() >0 and player:getHandcardNum() >0 then
							
							if (not room:askForSkillInvoke(player, "taohuan",_data)) then break end
							--
							player:setFlags("pindian")
							room:setPlayerMark(target, "taohuantarget"..player:objectName(),target:getMark("taohuantarget"..player:objectName())-1)
							if player:pindian(target, "taohuan", nil) then
								player:setFlags("-pindian")
								if not target:isNude()  then
								local to_obtain = room:askForCardChosen(player, target, "he", self:objectName())
									room:obtainCard(player,to_obtain,room:getCardPlace(to_obtain) ~= sgs.Player_PlaceHand)
								end
							end
						else
							break
						end
					end
					room:setPlayerMark(target, "taohuantarget"..player:objectName(),0)
				end
			end
		end
	end
}

 wai003:addSkill(fandu)
 wai003:addSkill(taohuan)
 

 --【窥知时空之目——宇佐见莲子】 编号：99004
wai004 = sgs.General(extension,"wai004", "wai",4,false) --99004
--识途之后，四季的审判会被算作伤害
shituDetect = sgs.CreateTriggerSkill{
        name = "#shituDetect",
        frequency = sgs.Skill_Compulsory,
        events = {sgs.Damaged,sgs.Death},
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local source = room:findPlayerBySkillName("shitu")
                if not source then return false end
                if room:getCurrent():objectName() == source:objectName() then
                    if source:getMark("touhou-extra")>0 then return false end
						if event == sgs.Damaged then
                                --和防止伤害类return true没有矛盾
								source:setMark("shituDamage",1)
                        end
                        if event == sgs.Death then
                                source:setMark("shituDeath",1)
                        end
                end
        end,
                
        can_trigger = function(self,player)
                return player
        end
}

shitu1 = sgs.CreateTriggerSkill{
        name = "shitu1",
        events = sgs.EventPhaseEnd, --sgs.EventPhaseChanging,, --为什么用pahsechanging就会出错呢？？？
        --can_trigger=function(self,player)
		--	return player:hasSkill(self:objectName()) and (player:getMark("touhou-extra")==0) 
		--end,
		on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                --local change = data:toPhaseChange()
				if player:getPhase() == sgs.Player_Finish then
				--if change.to == sgs.Player_NotActive then
                        if player:getMark("shituDamage") > 0 or player:getMark("shituDeath") > 0 then                 
                                player:removeMark("shituDamage")
                                player:removeMark("shituDeath")
                                return false
                        end
						
						if player:getMark("touhou-extra")>0 then
							
							player:removeMark("touhou-extra")
							return false
						end
                        local target = room:askForPlayerChosen(player,room:getAlivePlayers(),self:objectName(),"@shitu-playerchoose",true,true)
                        
						if target then
								room:notifySkillInvoked(player, self:objectName())
								local p = sgs.PhaseList()
								room:setPlayerMark(target,"touhou-extra",1)
                                p:append(sgs.Player_Draw)
								current=room:getCurrent()
								
								room:setCurrent(target)--不要忘记设置current
								target:play(p)
								room:setCurrent(current)
								
								room:setPlayerMark(target,"touhou-extra",0)
								
								--change.from=sgs.Player_Draw
								--data:setValue(change)
								--current:gainMark("@nimei",2)
								--local thread=room:getThread()
								--thread:trigger(sgs.EventPhaseChanging, room, current, data)

						end
								
								
                end
        end                
}

shitu = sgs.CreateTriggerSkill{
        name = "shitu",
        events = {sgs.EventPhaseChanging,sgs.EventPhaseStart},
        
		on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                if event==sgs.EventPhaseChanging then
					local change = data:toPhaseChange()
					if change.to == sgs.Player_NotActive then
						if player:getMark("shituDamage") > 0 or player:getMark("shituDeath") > 0 then                 
                                player:removeMark("shituDamage")
                                player:removeMark("shituDeath")
                                return false
                        end
						if player:getMark("touhou-extra")>0 then
							player:removeMark("touhou-extra")
							return false
						end
						player:setTag("ShituInvoke",sgs.QVariant(true))
					end
				end
				if event==sgs.EventPhaseStart and  player:getPhase()==sgs.Player_NotActive  then
					local shitu=player:getTag("ShituInvoke"):toBool()
					player:setTag("ShituInvoke",sgs.QVariant(false))
					if shitu then
						local target = room:askForPlayerChosen(player,room:getAlivePlayers(),self:objectName(),"@shitu-playerchoose",true,true)
                        
						if target then
							room:notifySkillInvoked(player, self:objectName())
							local p = sgs.PhaseList()
								room:setPlayerMark(target,"touhou-extra",1)
                                p:append(sgs.Player_Draw)
								current=room:getCurrent()
								
								room:setCurrent(target)--不要忘记设置current
								target:play(p)
								room:setCurrent(current)
								
								room:setPlayerMark(target,"touhou-extra",0)
							
							--[[target:setPhase(sgs.Player_Draw)
							room:broadcastProperty(target, "phase")
							local thread=room:getThread()
							if not thread:trigger(sgs.EventPhaseStart, room, target) then
								thread:trigger(sgs.EventPhaseProceeding, room, target)
							end	
							thread:trigger(EventPhaseEnd, room, target)

							target:setPhase(sgs.Player_NotActive)
							room:broadcastProperty(target, "phase")]]
						end
					end
				end
        end,                
}
wai004:addSkill(shitu)
wai004:addSkill(shituDetect)
extension:insertRelatedSkills("shitu", "#shituDetect")
	
	
--【看穿境界之目——玛艾露贝莉•赫恩】编号：99005	 by三国有单
--[源码改动]为达成乱影效果 修改room.cpp下的askForCard() 
wai005 = sgs.General(extension,"wai005", "wai",4,false) 
mengxian=sgs.CreateTriggerSkill{
	name = "mengxian",
	frequency = sgs.Skill_Wake,
	priority=3,
	events = {sgs.EventPhaseStart},
	can_trigger=function(self,player)
		return player:hasSkill(self:objectName()) and (player:getMark("mengxian")==0) and player:faceUp() and player:getPhase() == sgs.Player_Start
	end,
	on_trigger=function(self,event,player,data)--
		local room = player:getRoom()
		if player:getPile("aojiaofsujingjie"):length()>2 then

			touhou_logmessage("#MengxianWake",player,self:objectName(),nil,player:getPile("aojiaofsujingjie"):length())
			room:notifySkillInvoked(player, self:objectName())
			player:addMark("mengxian")
			if room:changeMaxHpForAwakenSkill(player) then
				player:drawCards(4)
				--room:acquireSkill(player,"luanying")
				room:handleAcquireDetachSkills(player,"luanying")
			end
			return false
		end
	end,
}

luanying = sgs.CreateTriggerSkill{
	name = "luanying",
	events = { sgs.CardUsed},
	can_trigger=function(self,player)
		return player
	end,
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()--创建房间
		local merry = room:findPlayerBySkillName(self:objectName())
		if not merry then return false end
		if event==sgs.CardUsed then	
			if merry:getPile("aojiaofsujingjie"):length() ==0 then return false end
			card_use = data:toCardUse()
			if not card_use.card:isKindOf("BasicCard") then return false end
			if not card_use.card:isRed() and not card_use.card:isBlack() then return false end
			if card_use.from:objectName() == merry:objectName() then return false end
			
			local jingjies = merry:getPile("aojiaofsujingjie")
			local ids=sgs.IntList()
			local disabled=sgs.IntList()
			for _,id in sgs.qlist(jingjies) do
				if sgs.Sanguosha:getCard(id):sameColorWith(card_use.card) then
					ids:append(id)
				else
					disabled:append(id)
				end
			end
			if ids:length()==0 then return false end
			--for ai
			ai_data=sgs.QVariant()
			ai_data:setValue(card_use.from)
			room:setTag("luanying_target",ai_data)
			_data=sgs.QVariant()
			_data:setValue(card_use)
			merry:setTag("luanying_use",_data)
			--
			if (not room:askForSkillInvoke(merry,self:objectName(),data)) then return false end	
			room:removeTag("luanying_target")
				
			room:fillAG(jingjies, merry, disabled)
			card_id = room:askForAG(merry, ids, true, "luanying")
			room:clearAG(merry)
			if card_id ==-1 then return false end
			jingjies:removeOne(card_id)
			local card = sgs.Sanguosha:getCard(card_id)	
			local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, "", nil, self:objectName(), "")
            room:throwCard(card, reason, nil)
			touhou_logmessage("#weiya",card_use.from,self:objectName(),nil,card_use.card:objectName())
			return true
		end
	end,
}
if not sgs.Sanguosha:getSkill("luanying") then
    local skillList=sgs.SkillList()
    skillList:append(luanying)
    sgs.Sanguosha:addSkills(skillList)
end

wai005:addSkill("aojiaofsujingjie")

wai005:addSkill(mengxian)
wai005:addRelateSkill("luanying")


--【连系海与山的月之公主——绵月丰姬】编号：99006 
wai006 = sgs.General(extension,"wai006", "wai",3,false) 
lianxiCard = sgs.CreateSkillCard {
	name = "lianxi",

	filter = function(self, targets, to_select, player)
		if #targets <= 1 then
			local targets2 = sgs.PlayerList()
			local card =sgs.Sanguosha:cloneCard("iron_chain", sgs.Card_NoSuit, 0)
			return (card:isAvailable(player) and not player:isProhibited(to_select, card) and card:targetFilter(targets2, to_select, player)) 
		else
			return false
		end
	end,
	feasible = function(self, targets, player)
		return #targets <=2 
	end,
	on_validate = function(self, use)
		if use.to:length() <= 2  then
			local card =sgs.Sanguosha:cloneCard("iron_chain", sgs.Card_NoSuit, 0)
			card:setSkillName("lianxi")
			return card
		end
		return use.card
	end,
}
lianxivs = sgs.CreateViewAsSkill{ 
	name = "lianxi" ,
	n = 0 ,

	view_as = function(self, cards)
		local card =lianxiCard:clone()
		return card
	end ,
	enabled_at_play = function(self, player)
		return false
	end ,
	enabled_at_response = function(self, player, pattern)
		return (pattern == "@@lianxi") 
	end ,
}
lianxi = sgs.CreateTriggerSkill{
	name = "lianxi",
	events = {sgs.CardUsed,sgs.CardResponded},
	view_as_skill=lianxivs,
	
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		local card
		if event ==sgs.CardUsed then
			card = data:toCardUse().card
		else
			card = data:toCardResponse().m_card
		end
		if not card or not card:isKindOf("Slash") then return false end
		room:askForUseCard(player,"@@lianxi","@lianxi")	
	end
}	

yueshi=sgs.CreateTriggerSkill{
	name = "yueshi",
	frequency = sgs.Skill_Wake,
	priority=3,
	events = {sgs.EventPhaseStart},
	can_trigger=function(self,player)
		return player:hasSkill(self:objectName()) and (player:getMark("yueshi")==0) and player:faceUp() and player:getPhase() == sgs.Player_Start
	end,
	on_trigger=function(self,event,player,data)--
		local room = player:getRoom()
		if ( player:isChained()) then
			touhou_logmessage("#YueshiWake",player,"yueshi")
			room:notifySkillInvoked(player, self:objectName())
			player:addMark("yueshi")
			if room:changeMaxHpForAwakenSkill(player,1) then
				room:handleAcquireDetachSkills(player,"ruizhi")
            end
			return false
		end
	end,
}

wai006:addSkill(lianxi)
wai006:addSkill(yueshi)
 

--【受神灵凭依的月之公主——绵月依姬】 编号：99007  
wai007 = sgs.General(extension,"wai007", "wai",4,false) 
skill_comeback = function(room,player) --归还技能
	local back --被归还者
	local back_skillname
	for _,p in sgs.qlist(room:getOtherPlayers(player,true)) do	
		if p:getMark("pingyi")>0 then
			key =player:objectName().."pingyi"..p:objectName()
			back_skillname =room:getTag(key):toString()
			if back_skillname and back_skillname ~="" then
				back =p
				--清除原有tag
				local tag = sgs.QVariant("")		
				room:setTag(key,tag)
				break
			end
		end
	end

	if back then
		
		room:setPlayerMark(player, "pingyi_steal", 0)
		if back:isAlive() then
		room:setPlayerMark(back, "pingyi", back:getMark("pingyi")-1)--可被复数
		end
		room:handleAcquireDetachSkills(player, "-"..back_skillname)
		if back:isAlive() then
		room:handleAcquireDetachSkills(back,back_skillname)
		end
	end
end

pingyi=sgs.CreateTriggerSkill{
	name = "pingyi",
	priority= -1,
	events = {sgs.Damaged},

	on_trigger=function(self,event,player,data)--
		local room = player:getRoom()	
		if event == sgs.Damaged then
			local damage = data:toDamage()
			if damage.from and damage.from:isAlive() and damage.from:objectName() ~=player:objectName() then	
				if player:isNude() then return false end
				if damage.from:hasSkill("chuanghuan") then return false end
				skill_names="cancel"
				for _,skill in  sgs.qlist(damage.from:getVisibleSkillList()) do 
					if skill:isLordSkill() or skill:getFrequency() == sgs.Skill_Limited or skill:getFrequency() == sgs.Skill_Wake 
						or skill:getFrequency() == sgs.Skill_Eternal then			
					else
						if  not skill:isAttachedLordSkill() and not player:hasSkill(skill:objectName()) then  
							skill_names =skill_names.."+"..skill:objectName()
						end
						--(skill:getLocation() == sgs.Skill_Right) and
					end
				end
				if skill_names=="cancel" then 
				else
					_data=sgs.QVariant()
					_data:setValue(damage.from)
					player:setTag("pingyi_target",_data)
					skill_name = room:askForChoice(player, "pingyi", skill_names)
					
					if skill_name =="cancel" then return false end
					if  room:askForCard(player, ".|.|.|.", "@pingyi:"..damage.from:objectName()..":"..skill_name, data, sgs.Card_MethodDiscard, nil, true, self:objectName()) then 
				
					--if room:askForDiscard(player,"pingyi",1,1,true,true,"@pingyi") then
					--先应该归还旧技能
						if player:getMark("pingyi_steal") >0 then
							skill_comeback (room,player) --自定义函数
						end
					--再获得新技能
					
						tagname= player:objectName().."pingyi"..damage.from:objectName()
						local tag = sgs.QVariant(skill_name)		
						room:setTag(tagname,tag)
						
						room:setPlayerMark(player, "pingyi_steal", 1)
						room:setPlayerMark(damage.from, "pingyi", damage.from:getMark("pingyi")+1)--可以被复数凭依
						room:handleAcquireDetachSkills(damage.from, "-"..skill_name)
						room:handleAcquireDetachSkills(player, skill_name)
					
					end
				end
			end
		end
	end,
}
pingyi_handle =sgs.CreateTriggerSkill{-- 技能"凭依"的清除处理
	name = "#pingyi_handle",
	priority=3,
	events = {sgs.EventLoseSkill,sgs.Death},
	can_trigger=function(self,player)
		return player:getMark("pingyi")>0 or  player:getMark("pingyi_steal") >0 
	end,
	
	on_trigger=function(self,event,player,data)--
		local room = player:getRoom()
		if event ==sgs.EventLoseSkill and data:toString() == "pingyi" then
			if player:getMark("pingyi_steal") >0  and (not player:hasSkill("pingyi")) then --同将下该如何处理呢。。。
				skill_comeback (room,player) 
			end
		end
		if event==sgs.Death then
			local death = data:toDeath()
			if death.who:objectName() == player:objectName() and player:getMark("pingyi_steal") >0 then
				skill_comeback (room,player) 
			end
			if  death.who:objectName() == player:objectName() and player:getMark("pingyi") >0 then
				
				for _,p in sgs.qlist(room:getOtherPlayers(player)) do
					if p:getMark("pingyi_steal") >0 then
						key =p:objectName().."pingyi"..player:objectName()
						back_skillname =room:getTag(key):toString()
						if back_skillname and back_skillname ~="" then
								skill_comeback (room,p)
						end
					end
				end
			end
		end
	end
}

wai007:addSkill(pingyi)
wai007:addSkill(pingyi_handle)
extension:insertRelatedSkills("pingyi", "#pingyi_handle")
 
	
--【闪耀的日之光——桑妮•米尔克】 编号：99008 
wai008 = sgs.General(extension,"wai008", "wai",3,false) 
zhesheCard = sgs.CreateSkillCard{
        name = "zheshe" ,
		will_throw = true,
		
        filter = function(self, targets, to_select)
                return #targets == 0
        end,
		
        on_effect = function(self, effect)
                local room = effect.to:getRoom()
                room:setPlayerMark(effect.to, "zheshetransfer", 1)

                local damage = effect.from:getTag("zhesheDamage"):toDamage()
                damage.to = effect.to
                damage.transfer = true
                room:damage(damage) --胆守未处理
        end ,
}
zhesheVS = sgs.CreateOneCardViewAsSkill{
        name = "zheshe" ,
        filter_pattern = "^EquipCard!" ,
        response_pattern = "@@zheshe" ,
        view_as = function(self, card)
                local scard=zhesheCard:clone()
				scard:addSubcard(card)
				scard:setSkillName(self:objectName())
				return scard
        end ,
}
zheshe = sgs.CreateTriggerSkill{
        name = "zheshe" ,
        events = {sgs.DamageInflicted} ,
        view_as_skill = zhesheVS ,
        on_trigger = function(self, event, player, data)
                if data:toDamage().transfer then return false end
                player:setTag("zhesheDamage", data)
                return player:getRoom():askForUseCard(player, "@@zheshe", "@zheshe", -1, sgs.Card_MethodDiscard)
        end ,
}
zheshedraw = sgs.CreateTriggerSkill{
        name = "#zheshedraw" ,
        events = {sgs.DamageComplete} ,
        can_trigger = function(self, target)
                return target
        end ,
        on_trigger = function(self, event, player, data)
                local damage = data:toDamage()
                if damage.to:isAlive() and (damage.to:getMark("zheshetransfer") > 0) and damage.transfer then
                        damage.to:getRoom():setPlayerMark(damage.to, "zheshetransfer", 0)
                        damage.to:drawCards(damage.to:getLostHp())
                end
                return false
        end ,
}

tanchi = sgs.CreateTriggerSkill{
        name = "tanchi" ,
        frequency = sgs.Skill_Compulsory ,
        events = {sgs.PreHpRecover} ,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
					touhou_logmessage("#TriggerSkill", player,self:objectName())
					room:notifySkillInvoked(player, self:objectName())
				local recover = data:toRecover()
                recover.recover = recover.recover + 1
                data:setValue(recover)
        end ,
}

wai008:addSkill(zheshe)
wai008:addSkill(zheshedraw)
wai008:addSkill(tanchi)
extension:insertRelatedSkills("zheshe", "#zheshedraw")

		
--【寂静的月之光——露娜•琪尔德】 编号：99009 
wai009 = sgs.General(extension,"wai009", "wai",4,false) --99009
zhuonongCard = sgs.CreateSkillCard{
        name = "zhuonong" ,
        filter = function(self, targets, to_select)
                return #targets == 0
        end ,
        on_effect = function(self, effect)
                local room = effect.to:getRoom()
                local _data = sgs.QVariant()
                _data:setValue(effect.to)
				effect.from:setTag("zhuonong_target",_data)
				local choice = room:askForChoice(effect.from, self:objectName(), "rd+dr", _data)
                local recover = sgs.RecoverStruct()
                recover.who = effect.from
                local damage = sgs.DamageStruct(self:objectName(), effect.from, effect.to,1, sgs.DamageStruct_Fire)
                if (choice == "rd") and effect.to:isWounded() then
                        room:recover(effect.to, recover)
                end
                room:damage(damage) -- 胆守
                if (choice == "dr") and effect.to:isWounded() then
                        room:recover(effect.to, recover)
                end
				
        end ,
}
zhuonong = sgs.CreateZeroCardViewAsSkill{
        name = "zhuonong" ,
        view_as = function(self)
                return zhuonongCard:clone()
        end ,
        enabled_at_play = function(self, player)
                return not player:hasUsed("#zhuonong")
        end ,
}
jijing=sgs.CreateTriggerSkill{
	name="jijing",
	events={sgs.Damaged,sgs.EventPhaseChanging},
	frequency = sgs.Skill_Compulsory ,
	can_trigger=function(self,player)
		return player
	end,
	
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local luna =room:findPlayerBySkillName(self:objectName())
		if not luna then return false end
		if event==sgs.Damaged then
			local damage = data:toDamage()
			if luna:getPhase()==sgs.Player_NotActive then return false end
			if not damage.to or damage.to:isDead() then return false end
			if damage.to:objectName()== luna:objectName() then return false end
			room:setFixedDistance(luna,damage.to,1)
			damage.to:gainMark("@jijing",1)
		end
		if event == sgs.EventPhaseChanging then
			local change = data:toPhaseChange()
			if change.to == sgs.Player_NotActive then
				for _,p in sgs.qlist(room:getOtherPlayers(luna))do
					if p and p:getMark("@jijing")>0 then
						room:setFixedDistance(luna,p,-1)
						p:loseAllMarks("@jijing")
					end
				end
			end
		end
	end
}

wai009:addSkill(zhuonong)
wai009:addSkill(jijing)

 
 
--【照射的星之光——斯塔•莎菲雅】 编号：99010  by三国有单
wai010 = sgs.General(extension,"wai010", "wai",4,false) --99010
ganying=sgs.CreateTriggerSkill{-- 技能"感应"
	name = "ganying",
	events = {sgs.GameStart,sgs.EventLoseSkill,sgs.EventAcquireSkill},
	
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		if ( event ==sgs.GameStart) or (event == sgs.EventAcquireSkill and data:toString() == "ganying") then 
			if player:hasSkill(self:objectName()) then
				owner_count =0
				for _,p in sgs.qlist(room:getAlivePlayers()) do --考虑同将
					if p:getMark("ganying_owner")> owner_count then
						owner_count = p:getMark("ganying_owner")
					end	
				end
				room:setPlayerMark(player, "ganying_owner", player:getMark("ganying_owner")+owner_count+1)
			end  
			if player:getMark("ganying_owner")>0 then    --第一次建立距离标记
				for _,target in sgs.qlist(room:getOtherPlayers(player)) do	
					markname="ganying_offensive"..tostring(player:getMark("ganying_owner"))
					markname1="ganying_defensive"..tostring(player:getMark("ganying_owner"))
					room:setPlayerMark(target, markname, player:distanceTo(target))
					room:setPlayerMark(target, markname1, target:distanceTo(player))
				end
			end
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				if (not p:hasSkill("#ganying_mark")) then
					room:acquireSkill(p, "#ganying_mark")
				end
			end
		end
		if (event == sgs.EventLoseSkill and data:toString() == "ganying") or event == sgs.Death then
			--清除标记
			markname="ganying_offensive"..tostring(player:getMark("ganying_owner"))
			markname1="ganying_defensive"..tostring(player:getMark("ganying_owner"))
			for _,target in sgs.qlist(room:getOtherPlayers(player)) do	
				room:setPlayerMark(target, markname, 0)
				room:setPlayerMark(target, markname1, 0)
			end
			--移除附加技能
			local skill_onwer=0
			for _,liege in sgs.qlist(room:getOtherPlayers(player)) do
				if liege:hasSkill("ganying") then
					skill_onwer=skill_onwer+1
				end
			end
			if skill_onwer==0 then
				for _,liege in sgs.qlist(room:getAlivePlayers()) do
					if (liege:hasSkill("#ganying_mark")) then
						room:detachSkillToPlayer(liege, "#ganying_mark",true)
					end
				end
			end
		end
	end,
}

ganying_mark=sgs.CreateTriggerSkill{-- 技能"感应"处理标记变化 --作为attach技能给所有目标。。。
	name = "#ganying_mark",
	events = {sgs.HpChanged,sgs.Death,sgs.CardsMoveOneTime,sgs.MarkChanged,sgs.EventPhaseChanging},
	--move马匹进入装备区 MarkChanged 天仪魔里沙玩装备 得到彼岸标记改变距离... HpChanged公孙瓒一类 还有别的情况吗(=_=)枚举真恶心
	--8人局的最远位邓艾屯田能连续三次触发 
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		if event==sgs.CardsMoveOneTime then--人偶操作真心麻烦。。。
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				if p:hasFlag("renou_move") then
					return false
				end
				if p:hasFlag("fengyin_move") then
					return false
				end
				if p:hasFlag("jiejie_move") then
					return false
				end
			end
		end
		for _,p in sgs.qlist(room:getAlivePlayers()) do
			if  p:getMark("ganying_owner")>0 then
				markname="ganying_offensive"..tostring(p:getMark("ganying_owner"))
				markname1="ganying_defensive"..tostring(p:getMark("ganying_owner"))
					local all=sgs.SPlayerList()
					local distance_change_one_time =false
					for _,target in sgs.qlist(room:getOtherPlayers(p)) do
						if target:getMark(markname) ~= p:distanceTo(target) or target:getMark(markname1) ~= target:distanceTo(p) then
							if target:getHandcardNum()>0  and  p:canDiscard(target, "h") then
								all:append(target)
							end
							distance_change_one_time=true
							room:setPlayerMark(target, markname, p:distanceTo(target))
							room:setPlayerMark(target, markname1, target:distanceTo(p))
						end
					end
					if distance_change_one_time then
						--即便常识使得【感应】失效，也持续记录距离。
						if not p:hasSkill("ganying") then return false end
						if not all:isEmpty() then
							ganying_func(room,p,all,true)	
						else
							ganying_func(room,p,all,false)
						end
					end
			end
		end	
	end,
}
ganying_func = function(room,player,listt,bool) --感应效果
	choice=""
	local list_ganying
	if bool then
		list_ganying=listt
		list_ganying:append(player)
	else
		list_ganying=sgs.SPlayerList()
		list_ganying:append(player)
	end
		
	target = room:askForPlayerChosen(player, listt, "ganying","@ganying",true,true)
	if target then
		if target:objectName()==player:objectName()	then	
			player:drawCards(1)
		else
			
			local card = room:askForCardChosen(player, target, "h", "ganying",false,sgs.Card_MethodDiscard)
			room:throwCard(card,target,player)
			--[[if target:getHandcardNum()>0 and player:canDiscard(target, "h")  then 
				local card1 = room:askForCardChosen(player, target, "h", "ganying",false,sgs.Card_MethodDiscard)
				room:throwCard(card1,target,player)
			end]]
		end
	end
end

if not sgs.Sanguosha:getSkill("#ganying_mark") then
        local skillList=sgs.SkillList()
        skillList:append(ganying_mark)
        sgs.Sanguosha:addSkills(skillList)
end
wai010:addSkill(ganying)

 
--【独臂有角的仙人——茨木　華扇】 编号：99011
wai011 = sgs.General(extension,"wai011", "wai",4,false) 
--估计是东方杀体系下最能拉慢节奏的珍贵人物了。。。。
--保核作用一流  顺风局无解? 逆风局硬生生拉成顺风局
xingshan = sgs.CreateTriggerSkill{
    name = "xingshan" ,
    events = {sgs.CardUsed,sgs.CardsMoveOneTime, sgs.SlashEffected, sgs.CardEffected, sgs.CardFinished} ,
    
	can_trigger = function(self, player)
        return player and player:isAlive()
    end,
    on_trigger = function(self, event, player, data)
        local room = player:getRoom()
		if event == sgs.CardUsed then
            local use = data:toCardUse() 			   
            if use.card and (use.card:isKindOf("Slash") or use.card:isKindOf("Duel")) then
				local source = room:findPlayerBySkillName(self:objectName())
				if not source then return false end 
                if source:hasFlag("xingshan_used") then return false end
				targets =use.to
				prompt="cancel:"..use.from:objectName()..":"..use.card:objectName()
				if(not room:askForSkillInvoke(source, self:objectName(),sgs.QVariant(prompt))) then return false end			
					source:setFlags("xingshan_used")
					use.from:drawCards(1)
					--room:setCardFlag(use.card, "xingshan")
					return true
				end
		end
		if event ==sgs.CardsMoveOneTime then
			local source = room:findPlayerBySkillName(self:objectName())
			if not source then return false end 
            if source:hasFlag("xingshan_used") then return false end
			local move = data:toMoveOneTime()				
			reason = move.reason	
			if not move.from then return false end
			if move.from:objectName() ~= player:objectName() then return false end
			if move.from:getPhase() ==sgs.Player_Discard then return false end
			local target
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				if p:objectName() == move.reason.m_playerId then
					target=p
					break
				end
			end
			if not target then return false end
			if target:objectName() ~=move.from:objectName()then return false end
			if bit32.band(reason.m_reason, sgs.CardMoveReason_S_MASK_BASIC_REASON) == sgs.CardMoveReason_S_REASON_DISCARD then						 
				local t =false
				local tmp_card
				for _,id in sgs.qlist(move.card_ids) do							
					local card = sgs.Sanguosha:getCard(id)
					if move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceHand then--默认从手牌中弃置
						if card:isKindOf("Slash") or card:isKindOf("Duel") then 								
							tmp_card=card
							t =true
							break
						end
					end
				end
				if not tmp_card then return false end
				prompt="discard:"..move.from:objectName()..":"..tmp_card:objectName()
				
				if t and room:askForSkillInvoke(source, self:objectName(),sgs.QVariant(prompt)) then 
					source:setFlags("xingshan_used")
					--move.from:drawCards(1)
					--local target
					for _,p in sgs.qlist(room:getAlivePlayers())do
						if p:objectName()==move.from:objectName() then
							p:drawCards(1)
							break
						end
					end
				end
			end
		end
           
    end ,
}
xingshan_clear = sgs.CreateTriggerSkill{
	name = "#xingshan_clear",
	events = {sgs.EventPhaseChanging},
	can_trigger=function(self,player)
		return true
	end,
	on_trigger = function(self,event,player,data)
		local change = data:toPhaseChange()
		if change.to ~= sgs.Player_NotActive then return false end
		local room = player:getRoom()
		for _,p in sgs.qlist(room:getAlivePlayers()) do
			if p:hasFlag("xingshan_used") then
				p:setFlags("-xingshan_used")
			end	
		end
	end
}
yushouCard = sgs.CreateSkillCard{
        name = "yushou" ,
        target_fixed = true ,
        about_to_use = function(self, room, cardUse)
                local discardpile = room:getDiscardPile()
                for _, id in sgs.qlist(discardpile) do
                        if sgs.Sanguosha:getCard(id):isKindOf("DefensiveHorse") 
						or sgs.Sanguosha:getCard(id):isKindOf("OffensiveHorse")   then
                                self:cardOnUse(room, cardUse)
                                return
                        end
                end
        end ,
        on_use = function(self, room, source)
                local able = sgs.IntList()
                local discardpile = room:getDiscardPile()
                for _, id in sgs.qlist(discardpile) do
                        tmp_card=sgs.Sanguosha:getCard(id)
						if (tmp_card:isKindOf("DefensiveHorse") or tmp_card:isKindOf("OffensiveHorse"))
						and not tmp_card:objectName():startsWith("renou") then
                                able:append(id)
                        end
                end
                if able:isEmpty() then return end
                room:fillAG(able, source)
                local equipid = room:askForAG(source, able, false, self:objectName())
                room:clearAG(source)
				source:obtainCard(sgs.Sanguosha:getCard(equipid),true)
        end ,
}
yushou = sgs.CreateZeroCardViewAsSkill{
        name = "yushou" ,
        view_as = function(self)
                return yushouCard:clone()
        end ,
        enabled_at_play = function(self, player)
                return not player:hasUsed("#yushou") and not player:getDefensiveHorse() and not player:getOffensiveHorse()
        end ,
}
wai011:addSkill(xingshan)
wai011:addSkill(xingshan_clear)
wai011:addSkill(yushou)
extension:insertRelatedSkills("xingshan", "#xingshan_clear")
 --【识文解意的爱书人——本居小铃】 编号：99012
wai012 = sgs.General(extension,"wai012", "wai",3,false) 
zulin = sgs.CreateTriggerSkill{
    name = "zulin" ,
    events={sgs.CardFinished},
	
    on_trigger = function(self, event, player, data)
		local use = data:toCardUse()
		local room = player:getRoom()
		local use = data:toCardUse()
		if event ==sgs.CardFinished then
			if use.card and use.card:isNDTrick() then
				if use.card:isVirtualCard() or use.card:subcardsLength() ~= 1 then return false end
				if sgs.Sanguosha:getEngineCard(use.card:getEffectiveId())
                    and sgs.Sanguosha:getEngineCard(use.card:getEffectiveId()):objectName() == use.card:objectName() then
					
					local target = room:askForPlayerChosen(player,room:getOtherPlayers(player),self:objectName(),"@zulin-obtain:"..use.card:objectName(),true,true)
					if target then
						target:obtainCard(use.card)
					end
				end
			end
		end	
    end ,
}
shimo = sgs.CreateTriggerSkill{
        name = "shimo" ,
        events = {sgs.TargetConfirming, sgs.CardEffected} ,
        --can_trigger = function(self, target)
        --    return target and target:isAlive()
        --end,
        on_trigger = function(self, event, player, data)
            local room = player:getRoom()                
			if event == sgs.TargetConfirming then
                local use = data:toCardUse()
                if player:getPhase() ~=sgs.Player_NotActive then return false end
				if player:hasFlag("shimo") then return false end
				if use.card and use.card:isNDTrick() then
                    local card           
                    suit =use.card:getSuit()
					if suit ==sgs.Card_NoSuit then return false end
					if suit == sgs.Card_Spade then
						card= room:askForCard(player, ".S", "@shimo-discard1")
					end
					if suit == sgs.Card_Heart then
						card= room:askForCard(player, ".H", "@shimo-discard2")
					end
					if suit == sgs.Card_Club then
						card= room:askForCard(player, ".C", "@shimo-discard3")
					end
					if suit == sgs.Card_Diamond then
						card= room:askForCard(player, ".D", "@shimo-discard4")
					end
					if card then
						player:setFlags("shimo")
					end
				end
                elseif event == sgs.CardEffected then
                        local effect = data:toCardEffect()
                        if effect.card and effect.card:isNDTrick() and effect.to:hasFlag("shimo") then
							touhou_logmessage("#DanlaoAvoid",effect.to,effect.card:objectName(),nil,self:objectName())
							return true
                        end
                end
                return false
        end ,
}
shimo_clear= sgs.CreateTriggerSkill{
	name = "#shimo_clear",
	events = {sgs.EventPhaseChanging},
	can_trigger = function(self, target)
        return target
    end,	
	on_trigger=function(self,event,player,data)
		local room=player:getRoom()
		if event==sgs.EventPhaseChanging then
			local change = data:toPhaseChange()
			if change.to == sgs.Player_NotActive then
				for _,p in sgs.qlist(room:getAlivePlayers())do
					if p:hasFlag("shimo") then
						p:setFlags("-shimo")
					end
				end
			end
		end
	end,
}
wai012:addSkill(zulin)
wai012:addSkill(shimo)
wai012:addSkill(shimo_clear)
extension:insertRelatedSkills("shimo", "#shimo_clear")
