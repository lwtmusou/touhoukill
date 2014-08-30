module("extensions.th11", package.seeall)
extension = sgs.Package("th11")

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
--技能代码   地灵殿
------------------------------------------------------------------------------------------------------------
--【连怨灵也恐惧的少女——古明地觉】 编号：11001		
dld001 = sgs.General(extension,"dld001$","dld",3,false)
xiangqi = sgs.CreateTriggerSkill{
        name = "xiangqi",
        events = {sgs.Damaged},
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local damage = data:toDamage()
                local source = room:findPlayerBySkillName(self:objectName())
                if not source then return false end
				if not damage.from then return false end
				if damage.from:objectName()==source:objectName() then return false end
				if damage.to:objectName() ~= damage.from:objectName() and damage.to:isAlive() then                 
                    if damage.card and not damage.from:isKongcheng() then
						prompt= "show:"..damage.from:objectName()..":"..damage.to:objectName()..":"..damage.card:objectName()
						local ai_data=sgs.QVariant()
						ai_data:setValue(damage.from)
						source:setTag("xiangqi_from",ai_data)
						ai_data:setValue(damage.to)
						source:setTag("xiangqi_to",ai_data)
						ai_data:setValue(damage.card)
						source:setTag("xiangqi_card",ai_data)
						if not source:askForSkillInvoke("xiangqi",sgs.QVariant(prompt)) then return false end
						local id = room:askForCardChosen(source,damage.from,"h",self:objectName())
                        room:showCard(damage.from, id)
						local showcard= sgs.Sanguosha:getCard(id)                				
						local same=false
						
						if showcard:isKindOf("BasicCard") and damage.card:isKindOf("BasicCard") then
							same=true
						end
						if showcard:isKindOf("TrickCard") and damage.card:isKindOf("TrickCard") then
							same=true
						end
						if same and damage.to:objectName() ~=source:objectName() then
							room:throwCard(id, damage.from,source)
							
							local dam = sgs.DamageStruct()
							dam.damage = 1
							dam.from = source
							dam.to = damage.to
							room:damage(dam)
						else
							room:obtainCard(damage.to, showcard) 
						end
						
                     end
                end
        end,

        can_trigger = function(self,target)
                return target
        end
}

--[源码改动]     修改room.cpp的函数askforcardchosen
duxin=sgs.CreateTriggerSkill{
	name="duxin",
	frequency = sgs.Skill_Compulsory,
	events={sgs.GameStart},	
	on_trigger=function(self,event,player,data)
	end
}
huzhu = sgs.CreateTriggerSkill{
        name="huzhu$",
        events=sgs.TargetConfirming,
        on_trigger=function(self,event,player,data)
                local use=data:toCardUse()
                local room=player:getRoom()
				if not player:hasLordSkill(self:objectName()) then return false end
				if use.card and use.card:isKindOf("Slash") then
                    local targets =room:getLieges("dld", player)         
					for _,p in sgs.qlist(targets) do
						if use.from and p:objectName() then
							targets:removeOne(use.from)
							continue
						end
						if  not use.from:canSlash(p,use.card,false) or use.to:contains(p) then
							targets:removeOne(p)
						end
					end
					if targets:isEmpty() then return false end
					if player:askForSkillInvoke(self:objectName(),data) then
                        for _,p in sgs.qlist(targets) do
                            local ai_data=sgs.QVariant()
							ai_data:setValue(player)
							room:setTag("huzhu_target",ai_data)
                            prompt= "slashtarget:"..use.from:objectName()..":"..player:objectName()..":"..use.card:objectName()
							if p:askForSkillInvoke("huzhu_change",sgs.QVariant(prompt)) then
								room:removeTag("huzhu_target")
								use.to:append(p)
                                use.to:removeOne(player)
                                data:setValue(use)
																		
								touhou_logmessage("$CancelTarget",use.from,use.card:objectName(),player)
								touhou_logmessage("#huzhu_change",use.from,use.card:objectName(),p)
								--必须重新触发TargetConfirming 
								room:getThread():trigger(sgs.TargetConfirming, room, p, data)
                                break
                            end
                        end
                    end
                end
        end
}
dld001:addSkill(xiangqi)
dld001:addSkill(duxin)
dld001:addSkill(huzhu)

		
--【紧闭着的恋之瞳——古明地恋】 编号：11002
dld002 = sgs.General(extension,"dld002", "dld",3,false) --11002
maihuoCard = sgs.CreateSkillCard{
        name = "maihuo" ,
        will_throw = false ,
        handling_method = sgs.Card_MethodNone ,
        on_use = function(self, room, source, targets)
                room:moveCardTo(self, nil, sgs.Player_DrawPile)
                local card_to_show = room:getNCards(2, false)
                local move = sgs.CardsMoveStruct(card_to_show, nil, sgs.Player_PlaceTable,
                                         sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_TURNOVER, targets[1]:objectName()))
                room:moveCardsAtomic(move, true)
                room:getThread():delay()
                room:getThread():delay()

                local dummy = sgs.Sanguosha:cloneCard("Slash", sgs.Card_NoSuit, 0)
                for _, id in sgs.qlist(card_to_show) do
                        dummy:addSubcard(id)
                end
                
                local bothred = true
                for _, id in sgs.qlist(card_to_show) do
                        if not sgs.Sanguosha:getCard(id):isRed() then
                                bothred = false
                                break
                        end
                end
				room:obtainCard(targets[1], dummy)
                if bothred then
                        local choice = "draw"
                        if source:isWounded() then choice = room:askForChoice(source, self:objectName(), "draw+recover") end
                        if choice == "draw" then
                                source:drawCards(2)
                        elseif choice == "recover" then
                                local recover = sgs.RecoverStruct()
                                recover.who = source
                                room:recover(source, recover)
                        else
                                assert(nil)
                        end
                end
                return false
        end ,
}
maihuo = sgs.CreateOneCardViewAsSkill{
        name = "maihuo" ,
        filter_pattern = ".|.|.|hand" ,
        view_as = function(self, card)
                local c = maihuoCard:clone()
                c:addSubcard(card)
                return c
        end ,
        enabled_at_play = function(self, target)
                return not target:hasUsed("#maihuo")
        end ,
}

wunian = sgs.CreateProhibitSkill{
        name = "wunian" ,
        is_prohibited = function(self, from, to, card)
			if from:objectName() ~=to:objectName()  then
				return   to:hasSkill(self:objectName()) and to:isWounded() and card:isKindOf("TrickCard")
			end
        end ,
}
wuniantr = sgs.CreateTriggerSkill{
        name = "#wuniantr" ,
        events = {sgs.DamageCaused},--sgs.Predamage,
        on_trigger = function(self, event, player, data)
                local room=player:getRoom()
				local damage = data:toDamage()
                damage.from = nil
                damage.by_user=false--重要
				touhou_logmessage("#TriggerSkill",player,"wunian")
				room:notifySkillInvoked(player, self:objectName())
				data:setValue(damage)
        end ,
}

dld002:addSkill(maihuo)
dld002:addSkill(wunian)
dld002:addSkill(wuniantr)
extension:insertRelatedSkills("wunian", "#wuniantr")

		
--【炽热焦躁的神之火——灵乌路空】 编号：11003
dld003 = sgs.General(extension,"dld003", "dld",4,false) 
ldlkyaobanCard = sgs.CreateSkillCard{
        name = "ldlkyaoban" ,
        filter = function(self, targets, to_select, player)
                local yaobanTargets = player:property("ldlkyaoban"):toString():split("+")
                return #targets == 0 and table.contains(yaobanTargets, to_select:objectName())
        end ,
        on_effect = function(self, effect)
                effect.from:getRoom():damage(sgs.DamageStruct("ldlkyaoban", effect.from, effect.to))
        end ,
}
ldlkyaobanVS = sgs.CreateOneCardViewAsSkill{
        name = "ldlkyaoban" ,
        filter_pattern = ".|.|.|hand!",
        response_pattern = "@@ldlkyaoban" ,
        view_as = function(self, card)
                local yaoban = ldlkyaobanCard:clone()
                yaoban:addSubcard(card)
                return yaoban
        end ,
}
ldlkyaoban = sgs.CreateMasochismSkill{
        name = "ldlkyaoban" ,
        view_as_skill = ldlkyaobanVS ,
        can_trigger = function(self, player)
                return player
        end ,
        on_damaged = function(self, player, damage)
                local room = player:getRoom()
                local ldlk = room:findPlayerBySkillName(self:objectName())
                if ldlk and (not ldlk:isKongcheng()) and (damage.nature == sgs.DamageStruct_Fire) then
                        local yaobanTargets = {}
                        for _, p in sgs.qlist(room:getOtherPlayers(player)) do
								table.insert(yaobanTargets, p:objectName()) 
                        end
                        if #yaobanTargets == 0 then return end
                        --ai
						local _data = sgs.QVariant()
						_data:setValue(damage)
						ldlk:setTag("yaoban_damage",_data)
						--
						room:setPlayerProperty(ldlk, "ldlkyaoban", sgs.QVariant(table.concat(yaobanTargets, "+")))
                        room:askForUseCard(ldlk, "@@ldlkyaoban", "@ldlkyaoban:"..damage.to:objectName()) --胆守
                        room:setPlayerProperty(ldlk, "ldlkyaoban", sgs.QVariant())
						ldlk:removeTag("yaoban_damage")
                end
        end ,
}

ldlkhere = sgs.CreateTriggerSkill{
        name = "ldlkhere",
		frequency = sgs.Skill_Compulsory,
        events = {sgs.TargetConfirming},
        can_trigger = function(self, player)
            return player
        end,
		on_trigger = function(self, event, player, data)
                local use=data:toCardUse()
				local room=player:getRoom()
				local source=room:findPlayerBySkillName(self:objectName())
				if not source then return false end
				if ((use.from and use.from:objectName() == source:objectName()) or use.to:contains(source)) then
					if use.card:isKindOf("Slash") and (not  use.card:isKindOf("FireSlash")) then
						--获得全部子卡
						
						local new_slash = sgs.Sanguosha:cloneCard("fire_slash", use.card:getSuit(), use.card:getNumber())
						for _,id in sgs.qlist(use.card:getSubcards()) do
                                new_slash:addSubcard(id)
                        end
						--保持原杀的skillname
                        new_slash:setSkillName(use.card:getSkillName())
						use.card=new_slash
						data:setValue(use)
						touhou_logmessage("#TriggerSkill",source,"ldlkhere")
						--room:useCard(sgs.CardUseStruct(new_slash, use.from, use.to),false)
					end
				end
        end,
}

dld003:addSkill(ldlkyaoban)
dld003:addSkill(ldlkhere)

		
--【地狱的车祸——火焰猫燐】 编号：11004
dld004 = sgs.General(extension,"dld004", "dld",4,false) --11004
--[源码改动]gamerule.cpp  火属性插入 对于铁锁结算 多记录一个传导的触发者
yuanling = sgs.CreateTriggerSkill{
        name = "yuanling",
        events = sgs.Damaged,
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local damage = data:toDamage()
                local source = room:findPlayerBySkillName("yuanling")
				if damage.to:objectName() == source:objectName() then
                    --可以用damage.by_user？？ 
					if damage.from and damage.from:isAlive() 
					and damage.from:objectName()~=source:objectName() then
						local slash = sgs.Sanguosha:cloneCard("fire_slash", sgs.Card_NoSuit, 0)
						if source:isCardLimited(slash, sgs.Card_MethodUse) then
							return false
						end
						
						_data=sgs.QVariant()
						_data:setValue(damage.from)
						source:setTag("yuanling",_data)
						prompt="target:"..damage.from:objectName()
						if room:askForSkillInvoke(source,self:objectName(),sgs.QVariant(prompt)) then
                                slash:setSkillName("_" .. self:objectName())
                                room:useCard(sgs.CardUseStruct(slash, source, damage.from))
						end
					end	
                end
        end
}

songzang = sgs.CreateTriggerSkill{
        name = "songzang",
        events = sgs.AskForPeaches,
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local victim = room:getCurrentDyingPlayer()
                 if player:objectName() ==victim:objectName() then return false end  
                 
				 if room:askForCard(player,".|spade","@songzang:"..victim:objectName(),data,self:objectName()) then
                        local die = sgs.DamageStruct()
                        die.from = player
                        room:killPlayer(victim,die)				
                end
        end
}

dld004:addSkill(yuanling)
dld004:addSkill(songzang)
	
	
--【小型的百鬼夜行——伊吹萃香】 编号：11005 
dld005 = sgs.General(extension,"dld005", "dld",3, false)
do_cuiji=function(player)
    local room = player:getRoom()
	local choice=room:askForChoice(player, "yccxcuiji", "red+black+cancel")
    if choice=="cancel" then return false end
	local isred = ( choice== "red")
	touhou_logmessage("#cuiji_choice", player,"yccxcuiji",nil,choice)
    local acquired = 0
    while (acquired < 1) do
         local id = room:drawCard()
         local move = sgs.CardsMoveStruct(id, nil, sgs.Player_PlaceTable, sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_TURNOVER, player:objectName()))
         move.reason.m_skillName = "yccxcuiji"
         room:moveCardsAtomic(move, true)
         room:getThread():delay()
         local card = sgs.Sanguosha:getCard(id)
         if card:isRed() == isred then
            acquired = acquired + 1
            local move2 = sgs.CardsMoveStruct(id, player, sgs.Player_PlaceHand, sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_GOTBACK, player:objectName()))
            room:moveCardsAtomic(move2, false)
         else
             local move3 = sgs.CardsMoveStruct(id, nil, sgs.Player_DiscardPile, sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_NATURAL_ENTER, ""))
             room:moveCardsAtomic(move3, true)
         end
     end
	  return true
end
yccxcuiji=sgs.CreateTriggerSkill{
	name="yccxcuiji",
	events={sgs.DrawNCards},
	on_trigger = function(self, event, player, data)
		local room=player:getRoom()
		if event==sgs.DrawNCards then
			if do_cuiji(player) then
				data:setValue(data:toInt()-1)
				if do_cuiji(player) then
					data:setValue(data:toInt()-1)
				end
			end
		end
	end
}


yccxbaigui = sgs.CreateOneCardViewAsSkill{
        name = "yccxbaigui" ,
        filter_pattern = ".|spade|.|hand" ,
        view_as = function(self, card)
                local sa = sgs.Sanguosha:cloneCard("savage_assault", sgs.Card_SuitToBeDecided, -1)
                sa:addSubcard(card)
                sa:setSkillName(self:objectName())
                return sa
        end ,
}

yccxjiuchong = sgs.CreateOneCardViewAsSkill{
        name = "yccxjiuchong" ,
        filter_pattern = ".|heart|.|hand" ,
        view_as = function(self, card)
                local ana = sgs.Sanguosha:cloneCard("analeptic", sgs.Card_SuitToBeDecided, -1)
                ana:addSubcard(card)
                ana:setSkillName(self:objectName())
                return ana
        end ,
        enabled_at_play = function(self, player)
                return sgs.Analeptic_IsAvailable(player)
        end ,
        enabled_at_response = function(self, player, pattern)
                return string.find(pattern, "analeptic") and (sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE)
        end ,
}

dld005:addSkill(yccxcuiji)
dld005:addSkill(yccxbaigui)
dld005:addSkill(yccxjiuchong)
	
	
--【人所谈论的怪力乱神——星熊勇仪】 编号：11006
dld006 = sgs.General(extension,"dld006" ,"dld",4,false) --11006
guaili = sgs.CreateTriggerSkill{
        name = "guaili",
        frequency = sgs.Skill_NotFrequent,
        events = {sgs.TargetConfirmed, sgs.SlashProceed,sgs.CardUsed},--,sgs.CardResponded
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if event == sgs.TargetConfirmed then
                        local use = data:toCardUse()
                        local card = use.card
                        local source = use.from
                        local room = player:getRoom()
                        if card:isKindOf("Slash") and card:isRed() and source:objectName() == player:objectName() then
                                local targets = use.to
                                for _,target in sgs.qlist(targets) do
										--for ai
                                        _data=sgs.QVariant()
										_data:setValue(target)
										player:setTag("guaili_target",_data)
										if player:askForSkillInvoke(self:objectName(),sgs.QVariant("cannotjink:"..target:objectName())) then
											room:setPlayerFlag(target, "guailiTarget")
                                        end
										player:removeTag("guaili_target")
                                end
                        end
                elseif event == sgs.SlashProceed then
                        local effect = data:toSlashEffect()
                        local dest = effect.to
                        if dest:hasFlag("guailiTarget") then
                                room:setPlayerFlag(dest, "-guailiTarget")
                                room:slashResult(effect, nil)
                                return true
                        end
                elseif event == sgs.CardUsed then
                        local use = data:toCardUse()
                        if use.from:objectName() == player:objectName() and use.card:isKindOf("Slash") and not use.card:isRed() then
  
								ask_card = room:askForCard(player, ".|.|.|hand" ,"@guaili",data, sgs.Card_MethodDiscard, nil, true, self:objectName())
	                            if ask_card then 
										local id = use.card:getId()
                                        local name = use.card:objectName()
                                        local point = use.card:getNumber()
                                        local slash = sgs.Sanguosha:cloneCard(name,sgs.Card_NoSuitRed,point)
                                        slash:addSubcard(id)
                                        slash:setSkillName("guaili")
                                        use.card = slash
										--use.card:setSuit(sgs.Card_Heart)--其实这样写更简洁？
										--use.card:setSkillName("guaili")
										touhou_logmessage("#guaili1",use.from,"guaili")
										data:setValue(use)
										
                                end
                        end
                end
                        --[[local response = data:toCardResponse()
                        if response.m_who:objectName() ~= player:objectName() and response.m_card:isKindOf("Slash") and not response.m_card:isRed() then
                                ask_card = room:askForCard(player, ".|.|.|hand" ,"@guaili",data, sgs.Card_MethodDiscard, nil, true, self:objectName())
	                            if ask_card then 
                                        local id = response.card:getId()
                                        local name = response.card:objectName()
                                        local point = response.card:getNumber()
                                        local slash = sgs.Sanguosha:cloneCard(name,sgs.Card_NoSuitRed,point)
                                        slash:addSubcard(id)
                                        slash:setSkillName("guaili")
                                        response.m_card = slash
										touhou_logmessage("#guaili1",player,"guaili")
                                        data:setValue(response)
                                end								
                        end]]

                
        end
}

haoyin = sgs.CreateTriggerSkill{
        name = "haoyin",
        frequency = sgs.Skill_Compulsory,
        events = sgs.CardEffected,
        priority = 1,
        on_trigger = function(self,event,player,data)
                local effect = data:toCardEffect()
                local room = player:getRoom()
                if effect.card:isKindOf("Analeptic") and effect.from:objectName() == player:objectName() then
                        room:setEmotion(effect.to, "analeptic")
                        if (effect.to:hasFlag("Global_Dying") and sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_PLAY) then
                               
								touhou_logmessage("#haoyin1",effect.from,"haoyin")
								room:notifySkillInvoked(effect.from, self:objectName())
								local recover = sgs.RecoverStruct()
                                recover.card = effect.card
                                recover.recover = 2
                                recover.who = effect.from
                                
								room:recover(effect.to, recover)
                                
								return true
                        else
								touhou_logmessage("#haoyin2",effect.to,"haoyin")
								room:notifySkillInvoked(effect.to, self:objectName())
								room:addPlayerMark(effect.to, "drank",2)
								return true
                        end
                end
        end
}
haoyin_draw = sgs.CreateTriggerSkill{
	name = "#haoyin_draw",
	frequency = sgs.Skill_Compulsory,
	events = {sgs.CardUsed},
    can_trigger = function(self, player)
        return player
    end ,
	on_trigger = function(self, event, player, data)
		local room=player:getRoom()
		local source=room:findPlayerBySkillName(self:objectName())
		if not source then return false end
		
		local use = data:toCardUse()
        if  not use.card:isKindOf("Analeptic") then return false end

		touhou_logmessage("#TriggerSkill",source,"haoyin")
		room:notifySkillInvoked(source, "haoyin")
		source:drawCards(1)               
	end
}
dld006:addSkill(guaili)
dld006:addSkill(haoyin)
dld006:addSkill(haoyin_draw)
extension:insertRelatedSkills("haoyin", "#haoyin_draw")

	
--【地壳下的嫉妒心——水桥帕露西】 编号：11007
dld007 = sgs.General(extension,"dld007", "dld",3,false) 
jiduVS = sgs.CreateOneCardViewAsSkill{
        name = "jidu" ,
        filter_pattern = ".|.|.|hand" ,
        view_as = function(self, card)
				local duel = sgs.Sanguosha:cloneCard("Duel", sgs.Card_SuitToBeDecided, -1)
                duel:addSubcard(card)
                duel:setSkillName(self:objectName())
                return duel
        end ,
}
jidu=sgs.CreateTriggerSkill{
	name="jidu",
	events={sgs.Damaged},
	view_as_skill=jiduVS,
	
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		if event ==sgs.Damaged then
			local damage = data:toDamage()
			if damage.card and damage.card:isKindOf("Duel")  then
				if player:askForSkillInvoke(self:objectName(), data) then
					player:drawCards(1)
				end
			end
		end
	end
}
jiduprevent = sgs.CreateProhibitSkill{
        name = "#jiduprevent" ,
        is_prohibited = function(self, from, to, card, players)
                return card:getSkillName() == "jidu" and from:getHp() > to:getHp()
        end ,
}

gelong=sgs.CreateTriggerSkill{
name="gelong",
events=sgs.TargetConfirming,
frequency=sgs.Skill_Compulsory,
on_trigger=function(self,event,player,data)
        local use=data:toCardUse()
		local room=player:getRoom()
		local source = room:findPlayerBySkillName(self:objectName())
        if use.card and use.card:isKindOf("Slash") and use.to:contains(player) then
                if use.from and use.from:isAlive() then
                        local room=player:getRoom()
						
						touhou_logmessage("#TriggerSkill",player,self:objectName())
                        room:notifySkillInvoked(player, self:objectName())
                        room:broadcastSkillInvoke(self:objectName())
                        local supply=sgs.Sanguosha:cloneCard("supply_shortage",sgs.Card_NoSuit,0)
                        local choice
                        local flag=true
                        if player:isProhibited(use.from, supply) then
                                flag=false
                        end
                        if use.from:containsTrick("supply_shortage")  then
							flag=false
						end
						
                        if flag then
                                choice=room:askForChoice(use.from,self:objectName(),"gelong1+gelong2")
                        else
                                choice="gelong1"
                        end
                        if choice=="gelong1" then
                                room:loseHp(use.from)
                                if use.from:isAlive() then
                                        use.from:drawCards(1)
                                end
                        else
                                local first=sgs.Sanguosha:getCard(room:drawCard())
                                
								local supplyshortage = sgs.Sanguosha:cloneCard("supply_shortage",first:getSuit(),first:getNumber())
								local vs_card = sgs.Sanguosha:getWrappedCard(first:getId())
								vs_card:setSkillName("_gelong")
								vs_card:takeOver(supplyshortage)
								room:broadcastUpdateCard(room:getAlivePlayers(),vs_card:getId(),vs_card)
								local move = sgs.CardsMoveStruct()
								move.card_ids:append(vs_card:getId())
								move.to = use.from
								move.to_place = sgs.Player_PlaceDelayedTrick
								room:moveCardsAtomic(move,true)
								local mes=sgs.LogMessage()
								mes.type="$PasteCard"
								mes.from=use.from
								mes.to:append(use.from)
								mes.arg=self:objectName()
								mes.card_str=vs_card:toString() 
								room:sendLog(mes)
                        end
                end
        end
end
}

dld007:addSkill(jidu)
dld007:addSkill(jiduprevent)
dld007:addSkill(gelong)
extension:insertRelatedSkills("jidu", "#jiduprevent")
	
	
--【黑暗洞窟中明亮的网——黑谷山女】 编号：11008
dld008 = sgs.General(extension,"dld008" ,"dld",4,false)
chuanrancard=sgs.CreateSkillCard{
name="chuanran",
target_fixed=false,
will_throw=true,
handling_method = sgs.Card_MethodNone ,
filter = function(self, targets, to_select, player)
    local chuanranTargets = player:property("chuanran"):toString():split("+")
	return #targets ==0 and table.contains(chuanranTargets, to_select:objectName())
end,
on_use = function(self, room, source, targets)
	for _,p in pairs(targets) do
		p:addMark("chuanran_target")
	end
end
}
chuanranvs = sgs.CreateViewAsSkill{
name = "chuanran",
n =1,
view_filter = function(self, selected, to_select)
    return to_select:isBlack() and not sgs.Self:isJilei(to_select)
end,
view_as = function(self, cards)
    if #cards==1 then
		local card=chuanrancard:clone()
		card:addSubcard(cards[1])
		return card
	end
end,
enabled_at_play=function(self, player)
        return false
end,
enabled_at_response = function(self, player, pattern)
        return pattern=="@@chuanran"
end
}
chuanran = sgs.CreateTriggerSkill{
        name = "chuanran",
        events = {sgs.BeforeCardsMove,sgs.FinishJudge},
        view_as_skill=chuanranvs,
		on_trigger = function(self,event,player,data)
			
			if event== sgs.BeforeCardsMove then
				local phase = player:getPhase()
                local room = player:getRoom()
                local move = data:toMoveOneTime()
                local source = room:findPlayerBySkillName(self:objectName())
                if not source then return false end
                if phase == sgs.Player_Judge then
                        if move.from_places:contains(sgs.Player_PlaceDelayedTrick) then
                                for _, p in sgs.qlist(move.card_ids) do
                                        if room:getCardPlace(p) ~= sgs.Player_PlaceDelayedTrick or not sgs.Sanguosha:getCard(p):isKindOf("DelayedTrick") then
                                                move.card_ids:removeOne(p)
										else
											sgs.Sanguosha:getCard(p):setFlags("chuanranFlag")
                                        end
                                end
                               
                        end
                        if  move.to_place == sgs.Player_DiscardPile then
                                for _,p in sgs.qlist(move.card_ids) do
                                        local card = sgs.Sanguosha:getCard(p)
                                        if not card:hasFlag("chuanranFlag") then return false end
                                        if room:askForSkillInvoke(source,self:objectName(),data) then
                                                card:setFlags("-chuanranFlag")
												local others = room:getOtherPlayers(room:getCurrent())
                                                local all=room:getOtherPlayers(room:getCurrent())
												local chuanranTargets = {}
												for _, p in sgs.qlist(all) do
                                                    for _, c in sgs.qlist(p:getCards("j")) do
                                                        if c:objectName() == card:objectName()then
                                                            others:removeOne(p)

														end
                                                     end
                                                end
												
												if (others:length()>0)  then
                                                    for _,p in sgs.qlist(others)do
														table.insert(chuanranTargets, p:objectName())
													end
													room:setPlayerProperty(source, "chuanran", sgs.QVariant(table.concat(chuanranTargets, "+")))
														local tag = sgs.QVariant(card:objectName())
														source:setTag("chuanran_cardname",tag)
														
														room:askForUseCard(source, "@@chuanran", "@chuanran:"..card:objectName())
														local target
														for _,p in sgs.qlist(room:getOtherPlayers(room:getCurrent())) do
															if p:getMark("chuanran_target")>0 then
																room:setPlayerMark(p, "chuanran_target", 0)
																target=p
																break
															end
														end
														room:setPlayerProperty(source, "chuanran", sgs.QVariant())
														if target then
															move.to = target
															move.to_place = sgs.Player_PlaceDelayedTrick
															data:setValue(move)
														else
															move.to = source
															move.to_place = sgs.Player_PlaceHand
															data:setValue(move)
														end
                                                else    
														move.to = source
                                                        move.to_place = sgs.Player_PlaceHand
                                                        data:setValue(move)
                                                end
											
                                        end
										
                                end
                        end
                end
			end
        end,

        can_trigger = function(self,player)
                return player
        end

}

dld008:addSkill(chuanran)


--【可怕的水井妖怪——琪斯美】 编号：11009 by三国有单
dld009 = sgs.General(extension,"dld009", "dld",3,false) --11009
--技能调整为技能卡？？
diaoping = sgs.CreateTriggerSkill{--技能"钓瓶"
		name = "diaoping",
		events = {sgs.TargetConfirming,sgs.SlashEffected,sgs.CardFinished},  
		can_trigger = function(self, player)
			return player
		end,
		on_trigger = function(self, event, player, data)
			local room = player:getRoom()
			
			if event ==sgs.TargetConfirming then
				skillowner = room:findPlayerBySkillName(self:objectName())
				if not skillowner then return false end
				local use=data:toCardUse()
				if use.from and use.from:objectName() == skillowner:objectName() then return false end
				local targets=sgs.SPlayerList()
				if use.card and use.card:isKindOf("Slash")  then 					
					for _,p in  sgs.qlist(use.to) do 
						if skillowner:inMyAttackRange(p) and not p:hasFlag("diaoping_Used") then  --p:distanceTo(player) <2
							targets:append(p)
						end
					end
				end
				if targets:length()>0 and skillowner:getHandcardNum() >0 and use.from:getHandcardNum() >0 then
					local good_result=false
					target = room:askForPlayerChosen(skillowner, targets, self:objectName(),"diaoping_chosenplayer",true,true)
						
					if target then
						room:setPlayerFlag(target, "diaoping_Used")
					
						while  use.from and use.from:isAlive() and good_result==false and skillowner:getHandcardNum() >0 and use.from:getHandcardNum() >0  do
							if not room:askForSkillInvoke(skillowner, "diaoping",data) then return false end
							
							if (skillowner:pindian(use.from, "diaoping", nil)) then
								use.from:turnOver()
								good_result=true
								room:setCardFlag(use.card, "diaoping_nullification") 
								room:setPlayerFlag(target, "diaoping_nullification")
								
								break
							end
						end
					end
				end	
			end
			if  event ==sgs.SlashEffected then
				local effect = data:toSlashEffect()
				local card = data:toSlashEffect().slash
			    if  card:hasFlag("diaoping_nullification") and effect.to:hasFlag("diaoping_nullification") then
					local log = sgs.LogMessage()
					log.type = "#DanlaoAvoid";
					log.from = effect.to
					log.arg = effect.slash:objectName()
					log.arg2 = self:objectName()
					room:sendLog(log)
					return true
				end
			end
			if  event ==sgs.CardFinished then
				local use = data:toCardUse()
			    if  use.card:hasFlag("diaoping_nullification") then
					room:setCardFlag(use.card, "-diaoping_nullification")
					for _,p in sgs.qlist(use.to) do
						room:setPlayerFlag(p, "-diaoping_Used")
						room:setPlayerFlag(p, "-diaoping_nullification")
					end
					return false
				end
			end
		end,
}

tongju=sgs.CreateProhibitSkill{--技能"桶居"
    name="tongju",
	is_prohibited=function(self,from,to,card)
        return to:hasSkill(self:objectName()) and ((card:isKindOf("SavageAssault") or card:isKindOf("IronChain")) or  card:isKindOf("ArcheryAttack"))
    end,  
}	

dld009:addSkill(diaoping)
dld009:addSkill(tongju)


