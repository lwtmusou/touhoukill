module("extensions.thsp", package.seeall)
extension = sgs.Package("thsp")
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
--【sp古明地觉】 编号：sp001	  by hmqgg
sp001 = sgs.General(extension,"sp001$","dld",3,false,false) --SP001


kongpiao = sgs.CreateTriggerSkill{
        name = "kongpiao",
        events = {sgs.EventPhaseChanging},
        frequency = sgs.Skill_Compulsory,
        on_trigger = function(self, event, player, data)
				local room = player:getRoom()
                local satori = room:findPlayerBySkillName(self:objectName())
                if not satori then return false end
				local change = data:toPhaseChange()
                if change.to == sgs.Player_Play then
					if satori:getPhase() == sgs.Player_NotActive then
                        local num = satori:getHandcardNum()
						local x = 0
						local current = room:getCurrent()
						x = math.max(x,current:getHandcardNum())
						
						if num < 5 and( x > num) then

										touhou_logmessage("#TriggerSkill",satori,self:objectName())
										room:notifySkillInvoked(player, self:objectName())
										room:broadcastSkillInvoke(self:objectName())
										satori:drawCards(1)
                        end
					end
				end
				
        end,
        can_trigger = function(self, target)
                return target
        end,
}

shouhuiCard = sgs.CreateSkillCard{
        name = "shouhui",
        target_fixed = true,
        will_throw = true,
        on_use = function(self, room, source, targets)
                if source:isAlive() then
                        local count = self:subcardsLength()
                        room:drawCards(source, count)
                end
        end
}
shouhuivs = sgs.CreateViewAsSkill{
        name = "shouhui",
        n = 999,
        view_filter = function(self, selected, to_select)
                return to_select:isKindOf("EquipCard") and not sgs.Self:isJilei(to_select)  
        end,
        view_as = function(self, cards)
                if #cards > 0 then
                        local shouhuicard = shouhuiCard:clone()
                        for _,card in pairs(cards) do
                                shouhuicard:addSubcard(card)
                        end
                        return shouhuicard
                end
        end,
        enabled_at_play = function(self, player)
                return true
        end,
        enabled_at_response = function(self, player, pattern)
                return pattern == "@@LuaShouhui"
        end,
}
shouhui = sgs.CreateTriggerSkill{
        name = "shouhui",
        frequency = sgs.Skill_NotFrequent,
        events = {sgs.Dying},
		view_as_skill=shouhuivs,
        on_trigger = function(self, event, player, data)
                local dying = data:toDying()
                local dead = dying.who
                local room = player:getRoom()
                if player:objectName() ~= dead:objectName() then return end
                while room:askForUseCard(player, "@@LuaShouhui","@LuaShouhuiD") do end
        end,
        can_trigger = function(self, target)
                if target then
                        if target:isAlive() and target:hasSkill(self:objectName()) then
                                return target:getCardCount(true) > 0
                        end
                end
                return false
        end,
}
woyuCard = sgs.CreateSkillCard{
        name = "woyu",
        target_fixed = false,
        will_throw = true,
        filter = function(self, targets, to_select)
                if #targets == 0 then
                        return to_select:objectName() ~= sgs.Self:objectName()
                end
                return false
        end,
        on_use = function(self, room, source, targets)
                room:removePlayerMark(source, "@Woyu")
                local target = targets[1]
                local role = target:getRole()
				touhou_logmessage("#WoyuAnnounce",source,role,room:getAllPlayers(),target:getGeneralName(),1)
                if role == "rebel" then
                        source:drawCards(3)
                elseif role == "loyalist" then
                        source:throwAllHandCardsAndEquips()
                end
        end,
}
woyuVS = sgs.CreateViewAsSkill{
        name = "woyu",
        n = 0,
        view_as = function(self, cards)
                return woyuCard:clone()
        end,
        enabled_at_play = function(self, player)
                if player:isLord() then
                        if player:hasLordSkill(self:objectName(),false) then
                                return player:getMark("@Woyu") >= 1
                        end
                end
                return false
        end
}
woyu = sgs.CreateTriggerSkill{
        name = "woyu$",
        frequency = sgs.Skill_Limited,
		view_as_skill = woyuVS,
		limit_mark = "@Woyu",
        events = {sgs.GameStart},
        on_trigger = function(self, event, player, data)
        end
}
sp001:addSkill(kongpiao)
sp001:addSkill(shouhui)
sp001:addSkill(woyu)

--【sp西行寺幽幽子】 编号：sp002
sp002 = sgs.General(extension,"sp002","yym",4,false) 
beisha = sgs.CreatePhaseChangeSkill{
        name = "beisha",
        on_phasechange = function(self, player)
                if player:getPhase() == sgs.Player_Start then
                        local slashlist = sgs.SPlayerList()
                        local losehplist = sgs.SPlayerList()
						local listt = sgs.SPlayerList()
                        local num = player:getHandcardNum()
                        local room = player:getRoom()
                        if player:isKongcheng() then return false end
						for _, p in sgs.qlist(room:getAllPlayers()) do
                                if (p:getHandcardNum() <= num) and player:canSlash(p,nil,false) then
                                        
										local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
										if  player:canSlash(p,slash,false) then
											 slashlist:append(p)
											 listt:append(p)
										end
                                end
                                if (p:getHandcardNum() <= (num / 2)) then
                                        losehplist:append(p)
										if not listt:contains(p) then
											listt:append(p)
										end
                                end
                        end
                        if (slashlist:isEmpty() and losehplist:isEmpty()) then return false end
						local target = room:askForPlayerChosen(player, listt, self:objectName(), "@beisha", true, true)
						if not target then return false end
						local choices = {}
                        if  slashlist:contains(target) then 
							table.insert(choices, "useslash") 
						end
                        if  losehplist:contains(target) then table.insert(choices, "losehp") end
                        
						local choice
                        if #choices == 1 then
                            choice = choices[1]
                        else
                            choice = room:askForChoice(player, self:objectName(), table.concat(choices, "+"))
                                
                        end
                        if choice == "useslash" then
                               
                                        slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
                                        slash:setSkillName("_" .. self:objectName())
                                        room:useCard(sgs.CardUseStruct(slash, player, target))
                        else
                                
								room:loseHp(target) 
                        end
						
                end
                return false
        end ,
}

sp002:addSkill(beisha)

--【sp雾雨魔理沙】 编号：sp003 BY orochiwhf
 sp003 = sgs.General(extension, "sp003","zhu",4,false) 
xisan = sgs.CreateTriggerSkill{
        name = "xisan",
        frequency = sgs.Skill_NotFrequent,
        events = {sgs.EventPhaseStart},
        can_trigger = function(self,player)
                return player
        end,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                
                if player:getPhase() == sgs.Player_Start then
                        local selfplayer=room:findPlayerBySkillName(self:objectName())
						if not selfplayer then return false end
						local hd_n=selfplayer:getHandcardNum()
						if room:askForSkillInvoke(selfplayer,self:objectName(),data) then 
							local choise = room:askForChoice(selfplayer,self:objectName(), "a+b")
							if choise == "a" then
                                selfplayer:drawCards(3-hd_n)
							else
                                room:askForDiscard(selfplayer,self:objectName(),hd_n,1,true,false, "@xisan-discard")
                                local newhd_n=selfplayer:getHandcardNum()
                                selfplayer:drawCards(2-newhd_n)
							end
							
						end	
                end
                return false 
        end,

}
sp003:addSkill(xisan)
 
 --【sp射命丸文】 编号：sp004
sp004 = sgs.General(extension,"sp004","zhan",3,false) 
jubao = sgs.CreateMasochismSkill{
        name = "jubao" ,
        on_damaged = function(self, player, damage)
                local room = player:getRoom()
                local targets = sgs.SPlayerList()
                for _, p in sgs.qlist(room:getAllPlayers()) do
                        if player:canDiscard(p, "h") then targets:append(p) end
                end
                if (targets:isEmpty()) then return end
                local victim = room:askForPlayerChosen(player, targets, self:objectName(), "@jubao-select", true, true)
                if victim then
                        local id = room:askForCardChosen(player, victim, "h", self:objectName(), false, sgs.Card_MethodDiscard)
                        room:throwCard(id, victim, player)
                        local dummy = sgs.Sanguosha:cloneCard("Slash", sgs.Card_NoSuit, 0)
                        count=0
						for _, c in sgs.qlist(victim:getEquips()) do
                                if player:canDiscard(victim, c:getEffectiveId()) and (c:isRed() == sgs.Sanguosha:getCard(id):isRed()) then 
									dummy:addSubcard(c) 
									count=count+1
								end
                        end
						if count>0 then
							room:throwCard(dummy, victim, player)
						end
					
                end
				
        end ,
}

haidi = sgs.CreateTriggerSkill{
        name = "haidi",
		frequency = sgs.Skill_Frequent,
        events = {sgs.CardsMoveOneTime} ,
        frequency = sgs.Skill_Frequent ,
        on_trigger = function(self, event, player, data)
                local move = data:toMoveOneTime()
                local room=player:getRoom()
				if (move.from and move.from:objectName() == player:objectName() and move.from_places:contains(sgs.Player_PlaceHand))
                                and ((not move.to) or (move.to:objectName() ~= player:objectName()) or (move.to_place ~= sgs.Player_PlaceHand))
                                and move.is_last_handcard
                                and player:isWounded() then
                        if (player:askForSkillInvoke(self:objectName())) then
                                local recover = sgs.RecoverStruct()
                                recover.who = player
                                player:getRoom():recover(player, recover)
							
                        end
						
                end
                return false
        end ,
}

sp004:addSkill(jubao)
sp004:addSkill(haidi)
 
-- 【第二届线外萌萌王——藤原妹红】编号：SP005
sp005 = sgs.General(extension,"sp005$","yyc",3,false) 
--[源码改动]修改源码Room:askForCard room:askForUseCard  askForNullification askForSinglePeach
--修改severplayer::hasNullification
shanjiFakeMoveCard = sgs.CreateSkillCard{
        name = "shanjiCard",
        target_fixed = true,
        will_throw = false,
        on_validate = function(self, use)
            local room = use.from:getRoom()
			room:askForUseCard(use.from, "@@shanji", "@shanji",-1, sgs.Card_MethodUse, true)	
		end
}
shanjiVS = sgs.CreateViewAsSkill{
        name = "shanji",
        n = 1,
        view_filter = function(self, selected, to_select)
                local pattern = sgs.Sanguosha:getCurrentCardUsePattern()
                if pattern == "@@shanji" then
                    
					if to_select:isAvailable(sgs.Self) then
						if to_select:isKindOf("Slash") then
							used = sgs.Self:getSlashCount()
							if sgs.Self:hasWeapon("Crossbow") then
								return true
							else
								num=1+sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, sgs.Self, to_select)
								if used<num then
									return true
								end
							end
							--return sgs.Slash_IsAvailable(sgs.Self) 不好使					
						elseif to_select:isKindOf("Analeptic") then
							return sgs.Analeptic_IsAvailable(sgs.Self)
						
						elseif to_select:isKindOf("Peach") then
							return sgs.Self:isWounded()
						else
							return (to_select:isKindOf("TrickCard") and not to_select:isKindOf("Nullification")) or (to_select:isKindOf("EquipCard"))
						end
					end
					return false
				end
                return true
        end,
        view_as = function(self, cards)
                local pattern = sgs.Sanguosha:getCurrentCardUsePattern()
                if pattern == "@@shanji" then
                        if #cards < 1 then return nil end            
						local card=cards[1]
						card:setSkillName("shanji")
                        return card
                else
						local tag = sgs.QVariant()
                        tag:setValue(sgs.Self:getPile("piao"))
                        sgs.Self:setTag("piao", tag)
                        return shanjiFakeMoveCard:clone()
                end
        end,
		
        enabled_at_play = function(self, player)
                if player:getPile("piao"):length() ==0 then return false end
				return true
				
        end,
		
        enabled_at_response = function(self, player, pattern)
                if player:getPile("piao"):length() ==0 then return false end
				if pattern == "@@shanji" then return true end
				local pattern_exmple=sgs.Sanguosha:getCurrentCardUsePattern()
				
        end,	
}
shanji=sgs.CreateTriggerSkill{
	name="shanji",
	view_as_skill = shanjiVS,
	events={sgs.DrawInitialCards,sgs.AfterDrawInitialCards},

	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		if (event == sgs.DrawInitialCards) then
			local log = sgs.LogMessage()
            log.type = "#TriggerSkill"
            log.from = player
            log.arg = "shanji"
            room:sendLog(log)
            room:notifySkillInvoked(player, "shanji")
			data:setValue(data:toInt() + 6)
		else
			room:broadcastSkillInvoke("shanji");
            exchange_card = room:askForExchange(player, "shanji", 6);
            player:addToPile("piao", exchange_card:getSubcards(), false)
		end
		return false
	end
}

yazhi_card = sgs.CreateSkillCard{
	name = "yazhi",
	target_fixed = true,
	will_throw = false,
	
	on_use = function(self, room, source, targets)
		cards= self:getSubcards() 
		source:addToPile("piao", cards:first(),false)
		
	end,
	
}
yazhi=sgs.CreateViewAsSkill{
	name = "yazhi",
	n = 1,
	view_filter = function(self, selected, to_select)
		return (not to_select:isEquipped()) 
	end,
	
	enabled_at_play = function(self,player)
		return not player:hasUsed("#yazhi")
	end,
	
	view_as = function(self, cards)
		if #cards==1 then
			local card=yazhi_card:clone()
			card:addSubcard(cards[1])
			card:setSkillName(self:objectName())
			return card
		end
	end
}

--[源码改动] 修改gamerule.cpp下getWinner
tymh_tianxiang=sgs.CreateTriggerSkill{
	name="tymh_tianxiang$",
	frequency = sgs.Skill_Eternal,
	events={sgs.GameStart},
	on_trigger=function(self,event,player,data)
		if player:hasLordSkill(self:objectName()) and player:isLord() then
			local room = player:getRoom()
			for _,p in sgs.qlist(room:getAllPlayers()) do
				room:setPlayerMark(p,"tianxiang",1)
			end
		end
	end
}

sp005:addSkill(shanji)
sp005:addSkill(yazhi)
sp005:addSkill(tymh_tianxiang)

--【sp蕾米莉亚•斯卡雷特】 编号：sp006
sp006 = sgs.General(extension,"sp006", "hmx",3,false)
qingcang = sgs.CreateTriggerSkill{
        name = "qingcang",
        events = {sgs.DrawNCards,sgs.AfterDrawNCards},
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if event == sgs.DrawNCards then
                        if player:getCards("j"):isEmpty() then
                            if room:askForSkillInvoke(player, "qingcang", data) then
                                local count = data:toInt() + 4
                                data:setValue(count)
                                room:setPlayerFlag(player,"qingcangUsed")
                            end
                        end
                elseif event == sgs.AfterDrawNCards and player:hasFlag("qingcangUsed") and not player:isKongcheng() then
                        local card = room:askForCard(player,".","@qingcang-card",sgs.QVariant(),sgs.Card_MethodNone,nil,false,self:objectName(),false)
                        local supplyshortage = sgs.Sanguosha:cloneCard("supply_shortage",card:getSuit(),card:getNumber())
                        local vs_card = sgs.Sanguosha:getWrappedCard(card:getId())
                        vs_card:setSkillName("_qingcang")
                        vs_card:takeOver(supplyshortage)
                        room:broadcastUpdateCard(room:getAlivePlayers(),vs_card:getId(),vs_card)
                        local move = sgs.CardsMoveStruct()
                        move.card_ids:append(vs_card:getId())
                        move.to = player
                        move.to_place = sgs.Player_PlaceDelayedTrick
                        room:moveCardsAtomic(move,true)
						
                end                
        end
}


changqing=sgs.CreateTriggerSkill{
name="changqing",
frequency = sgs.Skill_Compulsory,
events=sgs.Dying,
on_trigger=function(self,event,player,data)
        local room=player:getRoom()
        local who=room:getCurrentDyingPlayer()
        if who:objectName()~=player:objectName() then return false end
        if room:getAlivePlayers():length()<5 then return false end
        room:notifySkillInvoked(player, self:objectName())
		touhou_logmessage("#TriggerSkill",player,self:objectName())
															
		room:recover(player,sgs.RecoverStruct())
        
		
end
}

sp006:addSkill(qingcang)
sp006:addSkill(changqing)


