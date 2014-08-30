module("extensions.protagonist", package.seeall)
extension = sgs.Package("protagonist")
--整体mod下的源码改动，非针对具体技能
--[源码改动]为了根据主公选将 更改bgm和背景 修改roomScene：onGameStart()
--[源码改动]Audio.cpp  Audio::playBGM()检测bgm文件存在
--[源码改动]武将界面浏览处增加主公登场台词和胜利台词一栏。GeneralOverview::on_tableWidget_itemSelectionChanged
--[源码改动]TablePile::showJudgeResult 解决了UI一个改判卡牌的问题
--[源码改动]能够在设置里 设置选将时神将的可选数量，（东方包神将作为一个单独势力的前提下），改动了engine.cpp  banRandomGods()
--[源码改动]room:damage()下的青冈tag去除 防止伤害类相关
--皮肤dashboard里也要追加 avatarNameFont kingdomMaskArea
--[源码改动]UI dashboard里追加横条的势力彩条。 GenericCardContainerUI.cpp GenericCardContainerUI.h
--  _m_dashboardKingdomColorMaskIcon
-- SkinBank.cpp SkinBank.h  QSanRoomSkin::S_SKIN_KEY_KINGDOM_COLOR_MASK 
--[源码改动] 个人头像rightframe部分位置调整 dashboard.cpp  _createRight()  _m_skillDock->setPos
--log框 和chat框 roomscene.cpp  setBottom()
--room:_cardmove()源码修正。。。to fix card block
--[源码改动] 附加技能断线重连后的问题。 lua的视为技没办法实现 attached_lord_skill = true; 
--room.cpp  attachSkillToPlayer   reconnect
--[源码改动]雌雄剑效果改动

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
--技能代码   主人公
------------------------------------------------------------------------------------------------------------
	
----【乐园的美妙巫女——博丽灵梦】 编号：01001		
zhu001 = sgs.General(extension, "zhu001$","zhu",3,false) 
lingqi_card=sgs.CreateSkillCard{
name="lingqi",
target_fixed=false,
will_throw=true,
filter = function(self, targets, to_select, player)
	local lingqi_targets = player:property("lingqi_targets"):toString():split("+")
    return #targets <10 and table.contains(lingqi_targets, to_select:objectName())
end,
on_use = function(self, room, source, targets)
	for var=1, #targets, 1 do
		s=targets[var]
		local judge = sgs.JudgeStruct()
		judge.who = s
		judge.pattern = ".|red"
		judge.good = true
		judge.reason="lingqi"
		room:judge(judge)
		if judge:isGood() then
			s:setFlags("lingqinullify")
		end
	end
end
}										
lingqivs= sgs.CreateViewAsSkill{
	name = "lingqi",
	n = 1,

	view_filter = function(self, selected, to_select)
		return sgs.Self:canDiscard(sgs.Self,to_select:getId()) --sgs.Self:isJilei(to_select)
	end,
	
	view_as = function(self, cards)
		if #cards==1 then
			local card=lingqi_card:clone()
			card:addSubcard(cards[1])
			return card
		end
	end,
	
	enabled_at_play = function(self,player)
		return false
	end,
	
	enabled_at_response = function(self, player, pattern)
        return pattern == "@@lingqi"        
    end
}
lingqi = sgs.CreateTriggerSkill{
        name = "lingqi" ,
        events = {sgs.CardUsed, sgs.SlashEffected, sgs.CardEffected},
        view_as_skill=lingqivs,
		can_trigger = function(self, player)
             return player and player:isAlive()
        end ,
        on_trigger = function(self, event, player, data)
            local room = player:getRoom()
			if event == sgs.CardUsed then
                  local use = data:toCardUse() 			   
                  if use.card and (use.card:isKindOf("Slash") or use.card:isNDTrick()) then
						local lingmeng = room:findPlayerBySkillName(self:objectName())
						if not lingmeng then return false end 
						if lingmeng:isNude() then return false end
                        --for ai--
						lingmeng:setTag("lingqicarduse", data)
                        ----------
						local lingqitargets = {}
						for _, p in sgs.qlist(use.to) do
							table.insert(lingqitargets, p:objectName()) 
						end
						if #lingqitargets == 0 then return end
						room:setPlayerProperty(lingmeng, "lingqi_targets", sgs.QVariant(table.concat(lingqitargets, "+")))
						prompt="@lingqi:"..use.from:objectName()..":"..use.card:objectName()
						ask_card =room:askForUseCard(lingmeng, "@@lingqi",prompt )
						room:setPlayerProperty(lingmeng, "lingqi_targets", sgs.QVariant())
						if ask_card then
							for _,p in sgs.qlist(use.to) do
								if p:hasFlag("lingqinullify") then
									p:setFlags("-lingqinullify")
									room:setCardFlag(use.card, "lingqi"..p:objectName())
									--card设置flag时加入人名，可以有效解决其他一些技能的插入问题。
								end
							end
						end	
                        --for ai--
						--lingmeng:removeTag("lingqicarduse")
                        ----------
                   end
			elseif event == sgs.CardEffected then
				local effect = data:toCardEffect()
				if effect.card and effect.card:isNDTrick()  and effect.card:hasFlag("lingqi"..player:objectName())  then
						touhou_logmessage("#DanlaoAvoid",effect.to,effect.card:objectName(),nil,self:objectName())
						room:setEmotion(effect.to, "skill_nullify")
						return true
				end
			elseif event == sgs.SlashEffected then
				local effect = data:toSlashEffect()
				if effect.slash and effect.slash:hasFlag("lingqi"..player:objectName())  then
						touhou_logmessage("#DanlaoAvoid",effect.to,effect.slash:objectName(),nil,self:objectName())
						room:setEmotion(effect.to, "skill_nullify")
                        return true
                end
			end
        end ,
}

hongbai = sgs.CreateFilterSkill{
        name = "hongbai" ,
        view_filter = function(self, to_select)
                local room = sgs.Sanguosha:currentRoom()
                return (room:getCardPlace(to_select:getEffectiveId()) == sgs.Player_PlaceJudge) and (to_select:isBlack())
        end ,
        view_as = function(self, card)
                local wrap = sgs.Sanguosha:getWrappedCard(card:getEffectiveId())
                wrap:setSkillName(self:objectName())
                wrap:setSuit(sgs.Card_Heart)
                wrap:setModified(true)
                return wrap
        end ,
}

boli = sgs.CreateTriggerSkill{
        name = "boli$" ,
        events = {sgs.AskForRetrial} ,
        can_trigger = function(self, player)
                return player and player:isAlive() and player:hasLordSkill(self:objectName())
        end ,
        on_trigger = function(self, event, player, data)    
				local judge = data:toJudge()
                if  (judge.card:getSuit() ~= sgs.Card_Heart) then 
                        local room = player:getRoom()
                        player:setTag("boli_judge", data)
						prompt= "judge:"..judge.who:objectName()..":"..judge.reason
						if(not room:askForSkillInvoke(player, "boli",sgs.QVariant(prompt))) then return false end
						for _,p in sgs.qlist(room:getOtherPlayers(player)) do
                                room:broadcastSkillInvoke(self:objectName())
								local prompts={"@boli-retrial"}
								table.insert(prompts,judge.who:objectName())
								table.insert(prompts,player:objectName())
								table.insert(prompts,judge.reason)				
								
                                local heartcard = room:askForCard(p, ".H", table.concat(prompts,":"), data, sgs.Card_MethodResponse, judge.who, true, self:objectName())
								if (heartcard) then
                                        room:retrial(heartcard, p, judge, self:objectName(), false)
										return true
                                end
						end
                end
                return false
        end ,
}

zhu001:addSkill(lingqi)
zhu001:addSkill(hongbai)
zhu001:addSkill(boli)


	
--	【普通的魔法使——雾雨魔理沙】 编号：01002
zhu002 = sgs.General(extension, "zhu002$","zhu",4,false)
mofaCard = sgs.CreateSkillCard{
        name = "mofa",
        target_fixed = true,
        on_use = function(self, room, source, targets)
			touhou_logmessage("#mofa_notice",source,self:objectName())
			room:setPlayerFlag(source, "mofa_invoked")
        end
}
mofaVS = sgs.CreateOneCardViewAsSkill{
        name = "mofa",
        filter_pattern = ".|spade!",
        view_as = function(self, card)
                local acard = mofaCard:clone()
                acard:addSubcard(card)
                return acard
        end,
        enabled_at_play = function(self, player)
                return not player:hasFlag("mofa_invoked")
        end
}
mofa = sgs.CreateTriggerSkill{
        name = "mofa",
        events = {sgs.CardUsed, sgs.ConfirmDamage},--sgs.DamageCaused
        view_as_skill = mofaVS,
        can_trigger=function(self,  player)--断肠后效果持续
			return player
		end,
		on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if event == sgs.CardUsed then
                        if not player:hasFlag("mofa_invoked") then return false end
                        local use = data:toCardUse()
                        if use.card then
                                room:setCardFlag(use.card, "mofa_card")--认卡不认人 使用后直接死亡也无妨
                        end
                end
				if event == sgs.ConfirmDamage then --sgs.DamageCaused
                        local damage = data:toDamage()
                        if damage.card and damage.card:hasFlag("mofa_card") then
								if damage.from then
									touhou_logmessage("#TriggerSkill",damage.from,self:objectName())
									touhou_logmessage("#mofa_damage",damage.from,damage.damage+1,damage.to,damage.damage)
								end
								damage.damage = damage.damage + 1
                                data:setValue(damage)
                        end
                end
                return false
        end
}

heibai = sgs.CreateFilterSkill{
        name = "heibai",
        view_filter = function(self, to_select)
                local room = sgs.Sanguosha:currentRoom()
                return (room:getCardPlace(to_select:getEffectiveId()) == sgs.Player_PlaceHand) and (to_select:isRed() and not to_select:isKindOf("Slash"))
        end ,
        view_as = function(self, card)
                local wrap = sgs.Sanguosha:getWrappedCard(card:getEffectiveId())
                wrap:setSkillName(self:objectName())
                wrap:setSuit(sgs.Card_Spade)
                wrap:setModified(true)
                return wrap
        end ,
}

wuyuCard = sgs.CreateSkillCard{
    name = "wuyuvs",
    handling_method = sgs.Card_MethodNone ,
	will_throw = false,
	mute = true,
    filter = function(self, targets, to_select)
        return #targets==0 and to_select:hasLordSkill("wuyu")
			and to_select:objectName() ~= sgs.Self:objectName() and not to_select:hasFlag("wuyuInvoked")
    end,
    on_use = function(self, room, source, targets)
        local zhu002 = targets[1]
        if zhu002:hasLordSkill("wuyu") then
             room:setPlayerFlag(zhu002, "wuyuInvoked")
             zhu002:obtainCard(self)
             local subcards = self:getSubcards()
			 for _,card_id in sgs.qlist(subcards) do
                  room:setCardFlag(card_id, "visible")
             end
             room:setEmotion(zhu002, "good")
             local zhu002s = sgs.SPlayerList()
             local players = room:getOtherPlayers(source)
             for _,p in sgs.qlist(players) do
                 if p:hasLordSkill("wuyu") and not p:hasFlag("wuyuInvoked") then
                        zhu002s:append(p)
                 end
			 end
             if zhu002s:length() == 0 then
                room:setPlayerFlag(source, "Forbidwuyu")
             end
        end
    end
}
wuyuvs = sgs.CreateViewAsSkill{
     name = "wuyuvs",
	 n = 1,
     view_filter = function(self, selected, to_select)
         return to_select:getSuit() == sgs.Card_Spade and not to_select:isEquipped()
     end,
     view_as = function(self, cards)
          if #cards == 1 then
              local card = wuyuCard:clone()
                   card:addSubcard(cards[1])
                   return card
              end
         end,
     enabled_at_play = function(self, player)
         return not player:hasFlag("Forbidwuyu")
     end
}
wuyu = sgs.CreateTriggerSkill{
    name = "wuyu$",
    frequency = sgs.Skill_NotFrequent,
    events = {sgs.GameStart, sgs.EventPhaseChanging,sgs.EventAcquireSkill,sgs.EventLoseSkill},
    can_trigger=function(self,  player)
		return player
	end,
	on_trigger = function(self, event, player, data)
        local room = player:getRoom()
        if event == sgs.GameStart or (event == sgs.EventAcquireSkill and data:toString() == "wuyu") then
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
                if not p:hasSkill("wuyuvs") then
                    room:attachSkillToPlayer(p, "wuyuvs",true)													
                end
            end
		elseif (event == sgs.EventLoseSkill and data:toString() == "wuyu") then
			local lords=sgs.SPlayerList()
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				if p:hasLordSkill(self:objectName()) then
					lords:append(p)
				end
			end
			if lords:length()>1 then return false end
			local others=sgs.SPlayerList()
			if lords:length()==0 then
				others= room:getAlivePlayers()
			else
				others:append(lords:first())
			end	
			for _,p in sgs.qlist(others) do
                if  p:hasSkill("wuyuvs") then
                    room:detachSkillFromPlayer(p, "wuyuvs",true)													
                end
            end
        elseif event == sgs.EventPhaseChanging then
				local phase_change = data:toPhaseChange()
                if phase_change.from == sgs.Player_Play then
                    if player:hasFlag("Forbidwuyu") then
                        room:setPlayerFlag(player, "-Forbidwuyu")
                    end
                    local players = room:getOtherPlayers(player)
                    for _,p in sgs.qlist(players) do
                        if p:hasFlag("wuyuInvoked") then
                            room:setPlayerFlag(p, "-wuyuInvoked")
                        end
                    end
				end
        end
		return false
    end,
}
if not sgs.Sanguosha:getSkill("wuyuvs") then
        local skillList=sgs.SkillList()
        skillList:append(wuyuvs)
        sgs.Sanguosha:addSkills(skillList)
end

zhu002:addSkill(mofa)
zhu002:addSkill(heibai)
zhu002:addSkill(wuyu)	


--【十万巫女——博丽灵梦】 编号:01003 --by三国有单
zhu003 = sgs.General(extension, "zhu003","zhu",4,false) 	
saiqian_card = sgs.CreateSkillCard{ 
	name = "saiqianvs",
	target_fixed = false,
	will_throw = false,
	
	filter = function(self, targets, to_select, player)
		return #targets == 0 and (not to_select:hasFlag("saiqianInvoked") ) and to_select:hasSkill("saiqian")  and to_select:objectName() ~=player:objectName()
	end,
	
	on_use = function(self, room, source, targets)
		local target = targets[1]
		target:obtainCard(self,false)
		--for ai
		local _data=sgs.QVariant()
		_data:setValue(source)
		target:setTag("saiqian_source",_data)
		--
		room:setPlayerFlag(target, "saiqianInvoked")
		local saiqianInvoker =0 
		for _,liege in sgs.qlist(room:getOtherPlayers(source)) do
			if (not liege:hasFlag("saiqianInvoked")) and liege:hasSkill("saiqian")then
				saiqianInvoker=saiqianInvoker+1
			end
		end
		if saiqianInvoker==0 then
			room:setPlayerFlag(source, "ForbidSaiqian")
		end
		
		local saiqian_str ={"losehp_saiqian","discard_saiqian","cancel_saiqian"}
		
		local recov = sgs.RecoverStruct()
		recov.card = nil
		recov.who = target 
		recov.reason="saiqian"
		
		while #saiqian_str>0  do
			local choice = room:askForChoice(target,"saiqian",table.concat(saiqian_str,"+"))
			if choice == "cancel_saiqian" then
				return false
			end
			if choice == "losehp_saiqian" then 
				touhou_logmessage("#saiqian_lose",target,"saiqian")
				room:loseHp(target)
				room:recover(source, recov)
			end 
			if choice == "discard_saiqian" then 
				local _data=sgs.QVariant()
				_data:setValue(source)
				local heartcard = room:askForCard(target, ".H", "@saiqian-discard:"..source:objectName(), _data, sgs.Card_MethodDiscard, nil, true, "saiqian")
                if heartcard then 
					room:recover(source, recov)
                end
				--table.remove(saiqian_str,2) 序号		
			end
			table.removeOne(saiqian_str, choice)
		end
	end,
}
saiqianvs = sgs.CreateViewAsSkill{
	name = "saiqianvs",
	n = 3,
	enabled_at_play = function(self,player)
		return not player:hasFlag("ForbidSaiqian") 
	end,
	view_filter = function(self, selected, to_select)
		return not to_select:isEquipped()
	end,
	
	view_as = function(self, cards)
		if(#cards == 0) then return nil end
		local qcard = saiqian_card:clone()		
		for var = 1, #cards, 1 do     
			qcard:addSubcard(cards[var])              
		end
		qcard:setSkillName(self:objectName())
		return qcard
	end
}
saiqian = sgs.CreateTriggerSkill{
	name = "saiqian",
	events = {sgs.GameStart , sgs.EventPhaseChanging,sgs.EventAcquireSkill ,sgs.Death,sgs.EventLoseSkill}, 
	can_trigger=function(self,  player)
		return player
	end,
	
	on_trigger = function(self, event, player, data)	
		if ((event == sgs.GameStart ) or (event == sgs.EventAcquireSkill and data:toString() == "saiqian")) then		
			if player then
				local room = player:getRoom()
				local lords=sgs.SPlayerList()
				for _,p in sgs.qlist(room:getAlivePlayers()) do
					if p:hasSkill(self:objectName()) then
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
					if not p:hasSkill("saiqianvs") then
						room:attachSkillToPlayer(p, "saiqianvs",true)													
					end
				end
			end
		end		
		if  event==sgs.Death or (event == sgs.EventLoseSkill and data:toString() == "saiqian") then
			local room = player:getRoom()
			local onwers=sgs.SPlayerList()
			local others=sgs.SPlayerList()
			if event==sgs.Death then
				local death = data:toDeath()
				if not death.who:hasSkill("saiqian")  then return false end
			end
			
			for _,liege in sgs.qlist(room:getAlivePlayers()) do
				if liege:hasSkill("saiqian") then
					onwer:append(liege)
				end
			end
			if onwers:length()>1 then return false end
			if onwers:length()==0 then
				others= room:getAlivePlayers()
			else
				others:append(onwers:first())
			end	
			for _,liege in sgs.qlist(others) do
				if (liege:hasSkill("saiqianvs")) then
					room:detachSkillFromPlayer(liege, "saiqianvs",true)
				end
			end
		end
		if (event == sgs.EventPhaseChanging) then
            local room = player:getRoom()
			local phase_change = data:toPhaseChange()
            if phase_change.from == sgs.Player_Play then
                local players = room:getAlivePlayers()
                for _,p in sgs.qlist(players) do
                    if p:hasFlag("ForbidSaiqian") then
						room:setPlayerFlag(p, "-ForbidSaiqian")
					end
					if p:hasFlag("saiqianInvoked") then
                       room:setPlayerFlag(p, "-saiqianInvoked")
                    end
                end
            end
		end
	end
}	

if not sgs.Sanguosha:getSkill("saiqianvs") then
    local skillList=sgs.SkillList()
    skillList:append(saiqianvs)
    sgs.Sanguosha:addSkills(skillList)
end	

zhu003:addSkill(saiqian)

--【大盗——雾雨魔理沙】 编号：01004 --by三国有单
zhu004 = sgs.General(extension, "zhu004","zhu",3,false)
jiezou_card = sgs.CreateSkillCard{ 
	name = "jiezou",
	target_fixed = false,
	will_throw = true,
	filter = function(self, targets, to_select, player)
		return #targets == 0 and ( to_select:objectName() ~= player:objectName()) and (not to_select:isAllNude())
	end,
	
	on_use = function(self, room, source, targets)
		local target = targets[1]
		local id = room:askForCardChosen(source, target, "hej", self:objectName())
		room:obtainCard(source,id,room:getCardPlace(id) ~= sgs.Player_PlaceHand)
		if not room:askForCard(source,".|spade","@jiezou_spadecard",sgs.QVariant(),sgs.Card_MethodDiscard,nil,false,self:objectName(),false) then  --"..S"
			room:loseHp(source)
			room:setPlayerFlag(source, "Global_PlayPhaseTerminated")
			touhou_logmessage("#jiezou_skip",source,"jiezou_skip",nil,self:objectName())
		end
	end,
}
jiezou= sgs.CreateZeroCardViewAsSkill{
	name = "jiezou",
	enabled_at_play = function()
		return true
	end,	
	view_as = function(self, cards)
		local qcard = jiezou_card:clone()
		qcard:setSkillName(self:objectName())
		return qcard
	end
}

shoucangcard=sgs.CreateSkillCard{
name="shoucang",
target_fixed=true,
will_throw=false,
handling_method = sgs.Card_MethodNone ,
on_use = function(self, room, source, targets)
	local cards= self:getSubcards() 
	for _,id in sgs.qlist(cards)do
		room:showCard(source, id)
	end
	touhou_logmessage("#shoucang_max",source,self:objectName(),nil,cards:length())
	room:setPlayerMark(source, "shoucang", cards:length())
end
}
shoucangvs = sgs.CreateViewAsSkill{
name = "shoucang",
n =4,
view_filter = function(self, selected, to_select)
    if #selected==0 then
		return not to_select:isEquipped()
	end
	if #selected>0 then
		suits={}
		for _,c in pairs(selected) do
			if c:getSuit()==sgs.Card_Spade then
				table.insert(suits,sgs.Card_Spade)
			end
			if c:getSuit()==sgs.Card_Heart then
				table.insert(suits,sgs.Card_Heart)
			end
			if c:getSuit()==sgs.Card_Club then
				table.insert(suits,sgs.Card_Club)
			end
			if c:getSuit()==sgs.Card_Diamond then
				table.insert(suits,sgs.Card_Diamond)
			end
		end
		return not to_select:isEquipped() and not table.contains(suits,to_select:getSuit())
	end
end,
view_as = function(self, cards)
        if #cards>0 then
                local card=shoucangcard:clone()
                for i=1,#cards,1 do
                        card:addSubcard(cards[i])
                end
				
                return card
        end
end,
enabled_at_play=function(self, player)
        return false
end,
enabled_at_response = function(self, player, pattern)
        return pattern=="@@shoucang"
end
}
 
shoucang= sgs.CreateTriggerSkill{
	name = "shoucang",
	events = {sgs.EventPhaseStart,sgs.EventPhaseChanging},
	view_as_skill=shoucangvs,	
	on_trigger=function(self,event,player,data)
		local room=player:getRoom()
		if event == sgs.EventPhaseStart and (player:getPhase() == sgs.Player_Discard) then 			
			if player:getHandcardNum() ==0 then return false end			   
			room:askForUseCard(player,"@@shoucang","@shoucang")
		end		
		if event==sgs.EventPhaseChanging then
			local change = data:toPhaseChange()
			if change.to == sgs.Player_NotActive then
				room:setPlayerMark(player, "shoucang", 0)
			end
		end
	end,
}
shoucang_maxcard = sgs.CreateMaxCardsSkill{
    name = "#shoucang",
    extra_func = function (self,target)
          return target:getMark("shoucang")
    end
}

zhu004:addSkill(jiezou)
zhu004:addSkill(shoucang)
zhu004:addSkill(shoucang_maxcard)
extension:insertRelatedSkills("shoucang", "#shoucang") 

--【超•恋之魔女——超魔理沙】 编号： 01005  by三国有单
zhu005 = sgs.General(extension, "zhu005","zhu",4,false)  
baoyi = sgs.CreateTriggerSkill{
	name = "baoyi", 
	events = {sgs.EventPhaseStart,sgs.EventPhaseEnd},  
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if(player:getPhase() ~= sgs.Player_Start)  then return false end
		if (event ==sgs.EventPhaseStart) then
			--怕【死欲】等中止结算 使得mark不能清除干净的话，保险起见可以自己先清理
			room:setPlayerMark(player, "spadecount", 0)
			room:setPlayerMark(player, "baoyi", 0)
			if player:getCards("ej"):length() >0 then 
				if(not room:askForSkillInvoke(player, self:objectName(),data)) then return false end
				local dummy = sgs.Sanguosha:cloneCard("jink", sgs.Card_NoSuit, 0)
				for _,card in sgs.qlist(player:getCards("ej")) do
					if card:getSuit()== sgs.Card_Spade  then player:addMark("spadecount") end
					dummy:addSubcard(card)
					player:addMark("baoyi")
				end
				room:throwCard(dummy, sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_DISCARD, player:objectName()), player)

				num =player:getMark("baoyi")
				for var=1, num ,1 do
					local targets = sgs.SPlayerList() 
					local slash=sgs.Sanguosha:cloneCard("slash",sgs.Card_NoSuit,0)
					for _,p in sgs.qlist(room:getOtherPlayers(player)) do
						if player:canSlash(p, slash,false) then 
							targets:append(p)
						end
					end
					if targets:isEmpty() then return false end
					local num_str=tostring(num-var+1)
					local target = room:askForPlayerChosen(player, targets, self:objectName(),"@@baoyi:"..num_str,true,true)
					if target then
						local carduse=sgs.CardUseStruct()
						slash:setSkillName(self:objectName())
						carduse.card=slash
						carduse.from=player
						carduse.to:append(target)
						room:useCard(carduse)
					else
						break
					end
				end
			end				
		end	
		if (event ==sgs.EventPhaseEnd) then
			if player:getMark("spadecount")>0 then
				room:setPlayerMark(player, "spadecount", 0)
				if(not room:askForSkillInvoke(player, "baoyi_draw",sgs.QVariant("drawcard" ))) then return false end
				player:drawCards(2)
			end	
			room:setPlayerMark(player, "baoyi", 0)
		end
	end
}
zhu005:addSkill(baoyi)	


--【春巫女——博丽灵梦】 编号：01006 
zhu006 = sgs.General(extension, "zhu006", "zhu", 4, false)
--change考虑改为phase_start 虽然实质一样。。。
aojiaofsuzhize=sgs.CreatePhaseChangeSkill{
name="aojiaofsuzhize",
frequency=sgs.Skill_Compulsory,
on_phasechange = function(self, player)
        if player:getPhase() == sgs.Player_Draw then--phaseChange.to??
                local room = player:getRoom()
                local targets=room:getOtherPlayers(player)
                for _,p in sgs.qlist(room:getOtherPlayers(player)) do
                        if p:isKongcheng() then
                                targets:removeOne(p)
                        end
                end
				local s
                if  targets:length()>0 then
					s=room:askForPlayerChosen(player,targets,self:objectName(),"@@aojiaofsuzhize",true,true)
				end  
                if s then
                     room:fillAG(s:handCards(),player)
                     local id=room:askForAG(player,s:handCards(),false,self:objectName())
                     room:clearAG(player)
                     if id>-1 then
                         room:obtainCard(player,id,false)
                     end
                     return true
                else
                     touhou_logmessage("#TriggerSkill",player,self:objectName())
					 room:notifySkillInvoked(player, self:objectName())
					 room:loseHp(player)
                     player:skip(sgs.Player_Play)
                end
        end
end
}

aojiaofsuchunxi=sgs.CreateTriggerSkill{
name="aojiaofsuchunxi",
events=sgs.CardsMoveOneTime,
on_trigger = function(self, event, player, data)
        local move=data:toMoveOneTime()
        if move.to and move.to:objectName()==player:objectName() and move.to_place==sgs.Player_PlaceHand then
                local room=player:getRoom()
                if room:getTag("FirstRound"):toBool() == true then return false end
                
				local targets=sgs.SPlayerList()
                for _,p in sgs.qlist(room:getOtherPlayers(player)) do
                        if not p:isKongcheng() then
                                targets:append(p)
                        end
                end
                if targets:isEmpty() then return false end
                for _,id in sgs.qlist(move.card_ids) do
                        if sgs.Sanguosha:getCard(id):getSuit()==sgs.Card_Heart then
                                local s=room:askForPlayerChosen(player,targets,self:objectName(),"@@aojiaofsuchunxi",true,true)
                                if not s then break end
                                room:showCard(player,id)
                                room:getThread():delay()
                                local id=room:askForCardChosen(player, s, "h", self:objectName())
                                room:obtainCard(player, id,false)
                                if s:isKongcheng() then
                                        targets:removeOne(s)
                                end
								if targets:isEmpty() then return false end
                        end
                end
        end
end
}

zhu006:addSkill(aojiaofsuzhize)
zhu006:addSkill(aojiaofsuchunxi)	


--【五欲的巫女——博丽灵梦】 编号：01007 
zhu007 = sgs.General(extension, "zhu007", "zhu", 4, false) 
bllmwuyucost = function(room, bllm, prompt)
        local tag = sgs.QVariant(prompt)		
		bllm:setTag("wuyu_prompt",tag)
		if room:askForCard(bllm, "..H", "@bllm-discard:" .. prompt) then
				touhou_logmessage("#InvokeSkill",bllm,prompt)				
				room:notifySkillInvoked(bllm, prompt)
				return true
        else
                if bllm:getMark("@yu") > 0 then
                        if bllm:askForSkillInvoke("bllm_useyu", sgs.QVariant("useyu:" .. prompt)) then
								bllm:loseMark("@yu")
								touhou_logmessage("#InvokeSkill",bllm,prompt)
								room:notifySkillInvoked(bllm, prompt)
                                return true
                        end
                end
        end
        return false
end
bllmcaiyu = sgs.CreateDrawCardsSkill{
        name = "#bllmcaiyu" ,
        is_initial = false ,
        draw_num_func = function(self, bllm, n)
                local room = bllm:getRoom()
                if not bllmwuyucost(room, bllm, "bllmcaiyu") then return n end
                return n + 1
        end ,
}
bllmseyuCard = sgs.CreateSkillCard{
        name = "bllmseyu" ,
        target_fixed = true ,
        on_use = function(self, room, player)
                room:setPlayerMark(player,"bllmseyu",player:getMark("bllmseyu")+1)
        end ,
        on_validate = function(self, cardUse)
                local bllm = cardUse.from
                local room = cardUse.from:getRoom()
                if not bllmwuyucost(room, bllm, "bllmseyu") then return nil end
                return cardUse.card
        end
}
bllmseyu = sgs.CreateTargetModSkill{
        name = "#bllmseyu" ,
        residue_func = function(self, player, card)
             return player:getMark("bllmseyu")
        end ,
}
bllmseyu_clear = sgs.CreateTriggerSkill{
	name = "#bllmseyu_clear",
	events = {sgs.EventPhaseChanging},
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		if player:getMark("bllmseyu")>0 then
			room:setPlayerMark(player, "bllmseyu", 0)
		end	
	end
}
bllmmingyu = sgs.CreateTriggerSkill{
        name = "#bllmmingyu" ,
        events = {sgs.EventPhaseChanging} ,
        on_trigger = function(self, event, bllm, data)
                local change = data:toPhaseChange()
                if change.to == sgs.Player_Judge then
                        local room = bllm:getRoom()
                        if not bllmwuyucost(room, bllm, "bllmmingyu") then return false end                       
						bllm:skip(sgs.Player_Judge)
                end
                return false
        end ,
}
bllmshiyudummy = sgs.CreateSkillCard{
        name = "bllmshiyudummy" ,
        target_fixed = true ,
        will_throw = false ,
        handling_method = sgs.Card_MethodNone ,
        on_use = function()
        end
}
bllmshiyuCard = sgs.CreateSkillCard{
        name = "bllmshiyu" ,
        target_fixed = true ,
        will_throw = false ,
        handling_method = sgs.Card_MethodUse ,
        on_validate = function(self, cardUse)
                local bllm = cardUse.from
                local room = cardUse.from:getRoom()
                if not bllmwuyucost(room, bllm, "bllmshiyu") then return nil end
                local dummy = room:askForCard(bllm, "@@bllmwuyu", "@bllmshiyu-basics", sgs.QVariant(), sgs.Card_MethodNone)
                if dummy then
                        local ana = sgs.Sanguosha:cloneCard("analeptic", sgs.Card_SuitToBeDecided, -1)
                        for _, id in sgs.qlist(dummy:getSubcards()) do
                                ana:addSubcard(id)
                        end
                        ana:setSkillName("_bllmwuyu")
                        return ana
                end
                return nil
        end ,
        on_validate_in_response = function(self, bllm)
                local room = bllm:getRoom()
                if not bllmwuyucost(room, bllm, "bllmshiyu") then return nil end
                local dummy = room:askForCard(bllm, "@@bllmwuyu", "@bllmshiyu-basics", sgs.QVariant(), sgs.Card_MethodNone)
                if dummy then
                        local ana = sgs.Sanguosha:cloneCard("analeptic", sgs.Card_SuitToBeDecided, -1)
                        for _, id in sgs.qlist(dummy:getSubcards()) do
                                ana:addSubcard(id)
                        end
                        ana:setSkillName("_bllmwuyu")
                        return ana
                end
                return nil
        end ,
}
--[源码改动]因睡欲 修改翼曹操【挟尊】
bllmshuiyu = sgs.CreatePhaseChangeSkill{
        name = "#bllmshuiyu" ,
        on_phasechange = function(self, bllm)
                if bllm:getPhase() == sgs.Player_Discard then--
                        local room = bllm:getRoom()
                        if not bllmwuyucost(room, bllm, "bllmshuiyu") then return false end
						room:setPlayerFlag(bllm, "bllmshuiyu")
                end
                return false
        end ,
}
bllmshuiyumc = sgs.CreateMaxCardsSkill{
        name = "#bllmshuiyu2" ,
       fixed_func = function(self, player)
				if ( player:hasFlag("bllmshuiyu")) then  
					return 4 
				end
                return -1
       end,
   
}
bllmwuyuCard = sgs.CreateSkillCard{
        name = "bllmwuyu" ,
        target_fixed = true ,
        about_to_use = function(self, room, cardUse)
                local bllm = cardUse.from
                local uselist = {}
                if sgs.Analeptic_IsAvailable(bllm) then
                        table.insert(uselist, "bllmshiyu")
                end
                --bllm:hasUsed("#bllmseyu") 技能修改后色欲不限制次数了
                table.insert(uselist, "bllmseyu")
                if #uselist == 0 then return end
                table.insert(uselist, "dismiss")
                local choice = room:askForChoice(bllm, self:objectName(), table.concat(uselist, "+"))
                if choice == "dismiss" then return false end
                if choice == "bllmshiyu" then
                        local shiyu = bllmshiyuCard:clone()
                        room:useCard(sgs.CardUseStruct(shiyu, bllm, sgs.SPlayerList()))
                elseif choice == "bllmseyu" then
                        local seyu = bllmseyuCard:clone()
                        room:useCard(sgs.CardUseStruct(seyu, bllm, nil))
                end
        end ,
}
bllmwuyuvs = sgs.CreateViewAsSkill{
        name = "bllmwuyu" ,
        n = 998 ,
        view_filter = function(self, selected, to_select)
                local pattern = sgs.Sanguosha:getCurrentCardUsePattern()
                if pattern == "@@bllmwuyu" then
                        return #selected < 2 and (not to_select:isEquipped()) 
                end
                return false
        end ,
        view_as = function(self, cards)
                local pattern = sgs.Sanguosha:getCurrentCardUsePattern()
                if pattern == "@@bllmwuyu" then
                        if #cards ~= 2 then return nil end
                        shiyu = bllmshiyudummy:clone()
                        for _, c in ipairs(cards) do
                                shiyu:addSubcard(c)
                        end
                        return shiyu
                elseif string.find(pattern, "analeptic") then
                        return bllmshiyuCard:clone()
                end
                return bllmwuyuCard:clone()
        end ,
        enabled_at_play = function(self, player)
				if player:getMark("@yu") ==0 and player:isKongcheng() then return false end
				if sgs.Analeptic_IsAvailable(player) then return true end
                return true
        end ,
        enabled_at_response = function(self, player, pattern)
                if string.find(pattern, "@@bllmwuyu") then return true end
                if string.find(pattern, "analeptic") then return true end
        end ,
}
bllmwuyu = sgs.CreatePhaseChangeSkill{
        name = "bllmwuyu" ,
        view_as_skill = bllmwuyuvs ,
        on_phasechange = function(self, player)
                if player:getPhase() == sgs.Player_Start then
                        local n = player:getMark("@yu")
                        local z = (player:getLostHp() + 1) - n
                        if z > 0 then
                                if player:askForSkillInvoke(self:objectName()) then
                                        player:gainMark("@yu", z)
                                end
                        end
                end
                return false
        end ,
}

zhu007:addSkill(bllmwuyu)
zhu007:addSkill(bllmcaiyu)
zhu007:addSkill(bllmseyu)
zhu007:addSkill(bllmseyu_clear)
zhu007:addSkill(bllmmingyu)
zhu007:addSkill(bllmshuiyu)
zhu007:addSkill(bllmshuiyumc)
extension:insertRelatedSkills("bllmwuyu", "#bllmcaiyu")
extension:insertRelatedSkills("bllmwuyu", "#bllmseyu")
extension:insertRelatedSkills("bllmwuyu", "#bllmseyu_clear")
extension:insertRelatedSkills("bllmwuyu", "#bllmmingyu")
extension:insertRelatedSkills("bllmwuyu", "#bllmshuiyu")
extension:insertRelatedSkills("bllmwuyu", "#bllmshuiyu2")


--	【强欲的魔法使——雾雨魔理沙】 编号：01008  --by三国有单
zhu008 = sgs.General(extension, "zhu008","zhu",3,false) --01008	
--[源码改动]void Room::drawCards()
qiangyu = sgs.CreateTriggerSkill{
	name = "qiangyu", 
	events = {sgs.CardsMoveOneTime},  
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if (event ==sgs.CardsMoveOneTime) and player:hasFlag("qiangyu") then
			local move = data:toMoveOneTime()
			if move.to and move.to:objectName()==player:objectName() and move.to_place==sgs.Player_PlaceHand then
				room:setPlayerFlag(player, "-qiangyu")
				if not room:askForCard(player,".|spade","qiangyu_spadecard",data,sgs.Card_MethodDiscard,nil,false,self:objectName(),false) then  --"..S"
					room:askForDiscard(player,self:objectName(),2,2,false,false)
				end
			end
		end
	end
}

tianyi_suit = function(room,player,equip,addOrLose) --修改天仪花色mark的函数
	if addOrLose then
		if equip:getSuit()==  sgs.Card_Spade then
			room:setPlayerMark(player, "tianyi_spade", player:getMark("tianyi_spade")+1)
		end
		if equip:getSuit()==  sgs.Card_Heart then
			room:setPlayerMark(player, "tianyi_heart", player:getMark("tianyi_heart")+1)
		end
		if equip:getSuit()==  sgs.Card_Club then
			room:setPlayerMark(player, "tianyi_club", player:getMark("tianyi_club")+1)
		end
		if equip:getSuit()==  sgs.Card_Diamond then
			room:setPlayerMark(player, "tianyi_diamond", player:getMark("tianyi_diamond")+1)
		end
	else
		if equip:getSuit()==  sgs.Card_Spade then
			room:setPlayerMark(player, "tianyi_spade", player:getMark("tianyi_spade")-1)
		end
		if equip:getSuit()==  sgs.Card_Heart then
			room:setPlayerMark(player, "tianyi_heart", player:getMark("tianyi_heart")-1)
		end
		if equip:getSuit()==  sgs.Card_Club then
			room:setPlayerMark(player, "tianyi_club", player:getMark("tianyi_club")-1)
		end
		if equip:getSuit()==  sgs.Card_Diamond then
			room:setPlayerMark(player, "tianyi_diamond", player:getMark("tianyi_diamond")-1)
		end
	end
end
tianyi_compare = function(player,card)--确认杀和天仪花色的函数
	local result =false
	if card:getSuit()==  sgs.Card_Spade and player:getMark("tianyi_spade")>0then
		result =true	
	end
	if card:getSuit()==  sgs.Card_Heart and player:getMark("tianyi_heart")>0 then
		result =true	
	end
	if card:getSuit()==  sgs.Card_Club and player:getMark("tianyi_club")>0then
		result =true	
	end
	if card:getSuit()==  sgs.Card_Diamond and player:getMark("tianyi_diamond")>0then
		result =true	
	end
	return result
end  
--设置天仪时马匹和防具的无效  --原武器攻击距离改变为1
--[源码改动]为了不发动武器技能   包括麒麟弓射马 被射马   改动standard-card下各个武器技能weapon skill  连弩，菊花刀还有问题？

tianyi_targetmod = sgs.CreateTargetModSkill{--天仪无视距离
    name = "#tianyi_targetmod",
    distance_limit_func = function(self, from, card)
		--只和天仪装备相关，可以失去魔开技能？？？
		if from:hasSkill(self:objectName())  and card:isKindOf("Slash")  and tianyi_compare(from,card)  then
            return 1000
        else
			return 0
        end
    end,
}
tianyi_horse = sgs.CreateDistanceSkill {--天仪取消马匹原效果
    name = "#tianyi_horse",
    correct_func = function (self,from,to)
        if from:hasSkill("#tianyi_horse")and from:getMark("@tianyi_OffensiveHorse")>0 then
			return 1
        end
        if to:hasSkill("#tianyi_horse") and to:getMark("@tianyi_DefensiveHorse") >0 then
           return -1
		end
    end
}
tianyi_attackrange = sgs.CreateAttackRangeSkill{
    name = "#tianyi_attackrange",
	fixed_func= function(self,from) 
        if from:hasSkill("#tianyi_attackrange") and from:getMark("@tianyi_Weapon") >0 then    
			return 1
        end
	end
}
tianyi_collateral = sgs.CreateProhibitSkill{--天仪不能被借刀
        name = "#tianyi_collateral" ,
        is_prohibited = function(self, from, to, card)
            if to:getMark("@tianyi_Weapon")>0 then
				return to:hasSkill(self:objectName()) and card:isKindOf("Collateral")
			end	
        end ,
}

touhou_tianyi = sgs.CreateTriggerSkill{--“天仪”装备技能本体  
	name = "#touhou_tianyi",
	events = {sgs.CardsMoveOneTime,sgs.BeforeCardsMove, sgs.SlashEffected},
	on_trigger = function(self, event, player, data)
		local room=player:getRoom()
		if event == sgs.BeforeCardsMove then
			local move = data:toMoveOneTime()
            if move.from and (move.from:objectName() == player:objectName()) and move.from_places:contains(sgs.Player_PlaceEquip) and (move.to_place ~= sgs.Player_PlaceEquip) then
				for _, id in sgs.qlist(move.card_ids) do	
					if room:getCardPlace(id) == sgs.Player_PlaceEquip then	
						local equip= sgs.Sanguosha:getCard(id)
						if equip:isKindOf("Weapon") and  player:getMark("@tianyi_Weapon") >0 then
							room:setCardFlag(equip, "tianyi_Weapon")
						end
						if equip:isKindOf("Armor") and  player:getMark("@tianyi_Armor") >0 then
							room:setCardFlag(equip, "tianyi_Armor")
						end
						if equip:isKindOf("DefensiveHorse") and  player:getMark("@tianyi_DefensiveHorse") >0 then
							room:setCardFlag(equip, "tianyi_DefensiveHorse")
						end
						if equip:isKindOf("OffensiveHorse") and  player:getMark("@tianyi_OffensiveHorse") >0 then
							room:setCardFlag(equip, "tianyi_OffensiveHorse")
						end 
					end
				end
			end
		end	
		if event == sgs.CardsMoveOneTime then  --天仪摸排和改变标记
			local move = data:toMoveOneTime()
            if move.from and (move.from:objectName() == player:objectName()) and move.from_places:contains(sgs.Player_PlaceEquip) and (move.to_place ~= sgs.Player_PlaceEquip) then
				tianyi_draw=false
				for _, id in sgs.qlist(move.card_ids) do	
						local card= sgs.Sanguosha:getCard(id)
						--if card:isKindOf("Weapon") and  player:getMark("@tianyi_Weapon") >0 and card:hasFlag("tianyi_Weapon") then
						if card:hasFlag("tianyi_Weapon") then
							room:setCardFlag(card, "-tianyi_Weapon")
							room:setPlayerMark(player, "@tianyi_Weapon", 0)
							tianyi_suit(room,player,card,false)
							tianyi_draw=true
						end
						if  card:hasFlag("tianyi_Armor") then
							room:setCardFlag(card, "-tianyi_Armor")
							room:setPlayerMark(player, "@tianyi_Armor", 0)
							room:setPlayerMark(player,"Armor_Nullified",0)
							tianyi_suit(room,player,card,false)
							tianyi_draw=true
						end
						if  card:hasFlag("tianyi_DefensiveHorse") then
							room:setCardFlag(card, "-tianyi_DefensiveHorse")
							room:setPlayerMark(player, "@tianyi_DefensiveHorse", 0)
							tianyi_suit(room,player,card,false)
							tianyi_draw=true
						end
						if  card:hasFlag("tianyi_OffensiveHorse") then
							room:setCardFlag(card, "-tianyi_OffensiveHorse")
							room:setPlayerMark(player, "@tianyi_OffensiveHorse", 0)
							tianyi_suit(room,player,card,false)
							tianyi_draw=true
						end 		 
				end
				if tianyi_draw then 
					touhou_logmessage("#tianyiEquip",player)
					room:notifySkillInvoked(player, self:objectName())
					player:drawCards(2) 
				end			
			end
		end
		if event == sgs.SlashEffected then --杀无效
			local effect = data:toSlashEffect()
			if tianyi_compare(player,effect.slash) then
				touhou_logmessage("#tianyiEquip1",effect.to,effect.slash:objectName())
				room:notifySkillInvoked(effect.to, self:objectName())
				return true
			end
		end
	end,
}
if not sgs.Sanguosha:getSkill("#touhou_tianyi") then
    local skillList=sgs.SkillList()
    skillList:append(touhou_tianyi)
    sgs.Sanguosha:addSkills(skillList)
end
--[[if not sgs.Sanguosha:getSkill("#tianyi_targetmod") then
    local skillList=sgs.SkillList()
    skillList:append(tianyi_targetmod)
    sgs.Sanguosha:addSkills(skillList)
end
if not sgs.Sanguosha:getSkill("#tianyi_horse") then
    local skillList=sgs.SkillList()
    skillList:append(tianyi_horse)
    sgs.Sanguosha:addSkills(skillList)
end
if not sgs.Sanguosha:getSkill("#tianyi_attackrange") then
    local skillList=sgs.SkillList()
    skillList:append(tianyi_attackrange)
    sgs.Sanguosha:addSkills(skillList)
end
if not sgs.Sanguosha:getSkill("#tianyi_collateral") then
    local skillList=sgs.SkillList()
    skillList:append(tianyi_collateral)
    sgs.Sanguosha:addSkills(skillList)
end]]

mokai = sgs.CreateTriggerSkill{--技能“魔开”   
	name = "mokai",
	events = {sgs.CardUsed},
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if event == sgs.CardUsed then --设置天仪
			local card = data:toCardUse().card
			if card:isKindOf("TrickCard") then
				if player:getEquips():length()>0 then
					local equips =sgs.IntList()	
					for _,equip in sgs.qlist(player:getEquips()) do
						if equip:isKindOf("Weapon") and  player:getMark("@tianyi_Weapon") ==0 then
							equips:append(equip:getId())
						end
						if equip:isKindOf("Armor") and  player:getMark("@tianyi_Armor") ==0 then
							equips:append(equip:getId())
						end
						if equip:isKindOf("DefensiveHorse") and  player:getMark("@tianyi_DefensiveHorse") ==0 then
							equips:append(equip:getId())
						end
						if equip:isKindOf("OffensiveHorse") and  player:getMark("@tianyi_OffensiveHorse") ==0 then
							equips:append(equip:getId())
						end 
					end
					--选择要设置的天仪 
					if equips:length() == 0 then return false end 
					room:fillAG(equips, player)
					local card_id = room:askForAG(player, equips, true, self:objectName())	--不想设置 则直接点确定
					room:clearAG(player)
					if not card_id or card_id ==-1 then
						return false 
					else			
						if not player:hasSkill("#touhou_tianyi") then
							room:handleAcquireDetachSkills(player, "#touhou_tianyi")
							--room:acquireSkill(player,"#tianyi_targetmod")
							--room:acquireSkill(player,"#tianyi_horse")
							--room:acquireSkill(player,"#tianyi_attackrange")
							--room:acquireSkill(player,"#tianyi_collateral")
							--room:handleAcquireDetachSkills(player, "#tianyi_targetmod")
							--room:handleAcquireDetachSkills(player, "#tianyi_horse")
							--room:handleAcquireDetachSkills(player, "#tianyi_attackrange")
							--room:handleAcquireDetachSkills(player, "#tianyi_collateral")
						end
						
						touhou_logmessage("#InvokeSkill",player,"mokai")
						local tianyi_card = sgs.Sanguosha:getCard(card_id)
						if tianyi_card:isKindOf("Weapon")  then  
							player:gainMark("@tianyi_Weapon", 1)
						end
						if tianyi_card:isKindOf("Armor")  then
							player:gainMark("@tianyi_Armor", 1)
							room:setPlayerMark(player,"Armor_Nullified",1)
						end
						if tianyi_card:isKindOf("DefensiveHorse") then
							player:gainMark("@tianyi_DefensiveHorse", 1)
						end
						if tianyi_card:isKindOf("OffensiveHorse") then
							player:gainMark("@tianyi_OffensiveHorse", 1)
						end
						tianyi_suit(room,player,tianyi_card,true) 
						
						touhou_logmessage("#tianyi_set",player,tianyi_card:objectName())
						room:notifySkillInvoked(player, self:objectName())
					end
				end
			end 	
		end
	end,
}

zhu008:addSkill(qiangyu)
zhu008:addSkill(mokai)
--直接在魔开里加入好像有问题。。。
zhu008:addSkill(tianyi_targetmod)
zhu008:addSkill(tianyi_horse)
zhu008:addSkill(tianyi_attackrange)
zhu008:addSkill(tianyi_collateral)											
