module("extensions.th09", package.seeall)
extension = sgs.Package("th09")

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
--技能代码  花映塚+格斗作
------------------------------------------------------------------------------------------------------------
	--【乐园的最高裁判长——四季映姬•亚玛萨那度】 编号：09001 
 zhan001 = sgs.General(extension,"zhan001$","zhan",3,false) --09001
ymsndshenpan = sgs.CreateTriggerSkill{
        name = "ymsndshenpan" ,
        events = {sgs.EventPhaseStart, sgs.EventPhaseEnd} ,
        on_trigger = function(self, event, player, data)
                if player:getPhase() ~= sgs.Player_Draw then return false end
                local room = player:getRoom()
                if event == sgs.EventPhaseStart then
                        local target = room:askForPlayerChosen(player, room:getOtherPlayers(player), self:objectName(), "@ymsndshenpan-select", true, true)
                        if target then
                                room:damage(sgs.DamageStruct(self:objectName(), player, target, 1, sgs.DamageStruct_Thunder))
                                local _data = sgs.QVariant()
                                _data:setValue(target)
                                player:setTag("ymsndshenpan", _data)
                                return true
                        end
                elseif event == sgs.EventPhaseEnd then
                        local target = player:getTag("ymsndshenpan"):toPlayer()
                        if target then
                                player:removeTag("ymsndshenpan")
                                if target:getHandcardNum() > target:getHp() then
                                        if room:askForSkillInvoke(player, self:objectName(), sgs.QVariant("drawcard:" .. target:objectName())) then
                                                room:drawCards(player, 1)
                                        end
                                end
							
                        end
                end
                return false
        end
}

ymsndhuiwu = sgs.CreateTriggerSkill{ 
        name = "ymsndhuiwu" ,
        events = {sgs.TargetConfirming, sgs.CardEffected, sgs.SlashEffected} ,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if event == sgs.TargetConfirming then
                        local use = data:toCardUse()
                        if use.card and (use.card:isKindOf("Slash") or (use.card:isNDTrick())) then
                                if not use.from or use.from:isDead() or use.from:objectName()== player:objectName() then return false end
								--dead必须有 方天多杀触发葛笼而死
								--for AI
								local _data = sgs.QVariant()
								_data:setValue(player)
								use.from:setTag("ymsndhuiwu",_data)
								local a_data = sgs.QVariant()
								a_data:setValue(use)
								room:setTag("huiwu_use",a_data)
								--end
								prompt="target:"..player:objectName()..":"..use.card:objectName()
								if use.from:askForSkillInvoke(self:objectName(), sgs.QVariant(prompt)) then
                                        --由于恨意的插入的问题，因此cardflag需要记录人名
										--而不是player处记录一个同样的flag
										--room:setPlayerFlag(player, "ymsndhuiwu")
                                        room:setCardFlag(use.card, "ymsndhuiwu")
										room:setCardFlag(use.card, "ymsndhuiwu"..player:objectName())
										touhou_logmessage("#InvokeOthersSkill",use.from,self:objectName(),player)
										room:notifySkillInvoked(player, self:objectName())
										player:drawCards(1)
										
                                end
								--for AI
								--use.from:removeTag("ymsndhuiwu")
								--end
                        end
                elseif event == sgs.CardEffected then
                        local effect = data:toCardEffect()
                        if effect.card and effect.card:isNDTrick() and effect.card:hasFlag("ymsndhuiwu") 
								and effect.card:hasFlag("ymsndhuiwu"..effect.to:objectName()) then
								--恨意的万件插入会带来cardflag消失？？ playerflag的消失
								touhou_logmessage("#DanlaoAvoid",effect.to,effect.card:objectName(),nil,self:objectName())							
                                --room:setPlayerFlag(effect.to, "-ymsndhuiwu")
                                
								return true
                        end
                elseif event == sgs.SlashEffected then
                        local effect = data:toSlashEffect()
                        if effect.slash and effect.slash:hasFlag("ymsndhuiwu") and effect.slash:hasFlag("ymsndhuiwu"..effect.to:objectName()) then
                                touhou_logmessage("#DanlaoAvoid",effect.to,effect.slash:objectName(),nil,self:objectName())							
                                
                                --room:setPlayerFlag(effect.to, "-ymsndhuiwu")                
								return true
                        end
                end
                return false
        end ,
}

ymsndhuazhong = sgs.CreateTriggerSkill{
        name = "ymsndhuazhong$" ,
        events = {sgs.Damage, sgs.FinishJudge} ,
        can_trigger = function(self, player)
                return player and player:isAlive() and (player:getKingdom() == "zhan")
        end ,
        on_trigger = function(self, event, player, data)
                if (event == sgs.Damage) then
                        local room = player:getRoom()
						local count=data:toDamage().damage
                        
						for var=1, count, 1 do
							local ymsnds = sgs.SPlayerList()
							for _, p in sgs.qlist(room:getOtherPlayers(player)) do
                                if p:hasLordSkill(self:objectName()) then ymsnds:append(p) end
							end
							while not ymsnds:isEmpty() do
                                local target = room:askForPlayerChosen(player, ymsnds, self:objectName(), "@ymsndhuazhong-select", true, true)
                                if target then
                                        room:notifySkillInvoked(target, self:objectName())
										ymsnds:removeOne(target)
                                        local _data = sgs.QVariant()
                                        _data:setValue(target)
                                        player:setTag("ymsndhuazhong", _data)

                                        local judge = sgs.JudgeStruct()
                                        judge.pattern = ".|red"
                                        judge.who = player
                                        judge.reason = self:objectName()
                                        judge.good = true
                                        room:judge(judge) --胆守！！！可恨的朱然

                                        player:setTag("ymsndhuazhong", sgs.QVariant())
                                else
                                        break
                                end
							end
						
						end
                        
                else --event==
                        local judge = data:toJudge()
                        if (judge.reason == self:objectName()) and judge:isGood() then
                                judge.who:getTag("ymsndhuazhong"):toPlayer():obtainCard(judge.card)
                        end
                end
                return false
        end ,
}

zhan001:addSkill(ymsndshenpan)
zhan001:addSkill(ymsndhuiwu)
zhan001:addSkill(ymsndhuazhong)

 
--【三途河道的领路人——小野冢小町】 编号：09002	
zhan002 = sgs.General(extension,"zhan002", "zhan",4,false) --09002
mingtu = sgs.CreateTriggerSkill{
        name = "mingtu",
        frequency = sgs.Skill_Frequent,
		events = {sgs.EnterDying},--【具现】脱离濒死后，也要求能触发
        can_trigger = function(self,player)
			return player
		end,
		on_trigger = function(self,event,player,data)
			local room=player:getRoom()
			local dying=data:toDying()
			local who=dying.who
			
				--if event==sgs.QuitDying and who:isDead() then return false end
				local sources=room:findPlayersBySkillName(self:objectName())
				--有考虑同将模式
				if sources:length()==0 then return false end
				room:sortByActionOrder(sources)
				for _,s in sgs.qlist(sources) do
					if s:askForSkillInvoke(self:objectName(),data) then
						s:drawCards(1)
					end
				end
		end
}
silian = sgs.CreateTriggerSkill{
        name = "silian",
        frequency = sgs.Skill_Compulsory,
        events = {sgs.DamageCaused},
        on_trigger = function(self,event,player,data)
            local room=player:getRoom()
			
			if event==	sgs.DamageCaused then
				local damage = data:toDamage()
                if (damage.chain or damage.transfer or not damage.by_user) then return false end
				if not damage.from or not damage.to then return false end
				if damage.from:objectName() ==damage.to:objectName() then return false end
			
				if damage.card and damage.card:isKindOf("Slash") 
					and damage.to:getHp() ==1  then
						touhou_logmessage("#TriggerSkill",player,"silian",damage.to)
						room:notifySkillInvoked(player, self:objectName())
						damage.damage = damage.damage +2
                        data:setValue(damage)
                end
			end
        end

}

zhan002:addSkill(mingtu)
zhan002:addSkill(silian)

		
--【四季的鲜花之主——风见幽香】 编号：09003 by三国有单
--[源码改动] room.cpp下的askForCard()   askForNullification()  
zhan003 = sgs.General(extension,"zhan003","zhan",4,false)
weiya = sgs.CreateTriggerSkill{
	name = "weiya",
	frequency = sgs.Skill_Compulsory,
	events = {sgs.CardUsed},--sgs.TurnStart,sgs.Death,sgs.EventAcquireSkill,sgs.EventLoseSkill
	can_trigger = function(self, player)
		return player
	end, 
	
	on_trigger = function(self, event, player, data)
		local room = player:getRoom() 
        local current = room:getCurrent() 
		if  event ==sgs.CardUsed then  
			card_use = data:toCardUse()
			if not card_use.card:isKindOf("BasicCard") then return false end
			if not current:hasSkill(self:objectName()) then return false end
			if not current:isAlive() then return false end
			if card_use.from:objectName()~=current:objectName() then 
				player:setFlags("weiya_ask")
				weiya_pattern=card_use.card:objectName()
				if card_use.card:isKindOf("Slash") then
					weiya_pattern="slash"
				end
				--if weiya_pattern=="analeptic" then--为什么酒的pattern必须大写？？
				--	weiya_pattern="Analeptic"
				--end
				if not room:askForCard(player, weiya_pattern, "@weiya:"..card_use.card:objectName(), sgs.QVariant(),sgs.Card_MethodResponse) then
					player:setFlags("-weiya_ask")
					
					touhou_logmessage("#weiya",player,self:objectName(),nil,card_use.card:objectName())
					return true
				end
				player:setFlags("-weiya_ask")
			end
		end
	end,
}

zhan003:addSkill(weiya)	
	

--【小小的甜蜜毒药——梅蒂馨•梅兰克莉】 编号：09004
zhan004 = sgs.General(extension,"zhan004", "zhan",3,false) --09004
judu=sgs.CreateTriggerSkill{
name="judu",
events=sgs.Damage,
on_trigger=function(self,event,player,data)
        if data:toDamage().to:isAlive() then
                local _data = sgs.QVariant()
                _data:setValue(data:toDamage().to)
				if player:askForSkillInvoke(self:objectName(),_data) then
                        local room=player:getRoom()
                        room:broadcastSkillInvoke(self:objectName())
                        local judge=sgs.JudgeStruct()
                        judge.reason=self:objectName()
                        judge.who=player
                        judge.good=true
						judge.pattern = ".|black|2~9"
                        judge.negative = true
                        room:judge(judge)
                       
						if judge:isGood() then
                                local damage=sgs.DamageStruct()
                                damage.from=player
                                damage.to=data:toDamage().to
                                room:damage(damage)
                        end
					
                end
				
        end
end
}

henyi=sgs.CreateTriggerSkill{
name="henyi",
events=sgs.Damaged,
on_trigger=function(self,event,player,data)
        local damage = data:toDamage()
		if player:askForSkillInvoke(self:objectName(),data) then
                local room=player:getRoom()
                room:broadcastSkillInvoke(self:objectName())
                local card = sgs.Sanguosha:cloneCard("archery_attack", sgs.Card_NoSuit, 0)
                card:setSkillName("_henyi")
                local carduse = sgs.CardUseStruct()
                carduse.card = card
                carduse.from = player
                room:useCard(carduse)
        end
end
}

zhan004:addSkill(judu)
zhan004:addSkill(henyi)
 
		
--【捏造新闻记者——射命丸文】 编号：09005
zhan005 = sgs.General(extension,"zhan005", "zhan",4,false) --09005
smwwtoupai = sgs.CreatePhaseChangeSkill{
        name = "smwwtoupai" ,
        on_phasechange = function(self, player)
                if player:getPhase() == sgs.Player_Draw then
                        local room = player:getRoom()
                        local players = sgs.SPlayerList()
                        for _, p in sgs.qlist(room:getOtherPlayers(player)) do
                                if player:canDiscard(p, "h") then players:append(p) end
                        end
                        if players:isEmpty() then return false end
                        local target = room:askForPlayerChosen(player, players, self:objectName(), "@smwwtoupai-select", true, true)
                        if target then
                                for i = 1, 3, 1 do
                                        local ids = sgs.IntList()
                                        for _, c in sgs.qlist(target:getHandcards()) do
                                                if c:isRed() then ids:append(c:getEffectiveId()) end
                                        end
                                        local id = room:doGongxin(player, target, ids, "smwwtoupai")
                                        player:removeTag("smwwtoupai")
                                        if id == -1 then return true end
                                        local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_DISMANTLE, player:objectName(), "", "smwwtoupai", "")
                                        room:throwCard(sgs.Sanguosha:getCard(id), reason, target, player)
                                        if sgs.Sanguosha:getCard(id):isKindOf("BasicCard") then
                                                player:drawCards(1)
                                        end
                                end
                                return true
                        end
						
                end
                return false
        end ,
}

zhan005:addSkill(smwwtoupai)


--【不羁奔放的古豪——伊吹萃香】 编号：09006
zhan006 = sgs.General(extension,"zhan006$","zhan",3,false) --09006
zuiyue = sgs.CreateTriggerSkill{
        name = "zuiyue",
        events = sgs.CardUsed,
        on_trigger = function(self, event, player, data)
                if player:getPhase() ~= sgs.Player_Play then return false end
                local room = player:getRoom()
                local use = data:toCardUse()
				if not sgs.Analeptic_IsAvailable(player) then return false end
                if use.card:isKindOf("TrickCard") and room:askForSkillInvoke(player, self:objectName(), data) then
                        local ana_use = sgs.CardUseStruct()
                        ana_use.from = player
                        local card = sgs.Sanguosha:cloneCard("analeptic")
                        card:setSkillName(self:objectName())
                        ana_use.card = card
                        room:useCard(ana_use)
					
                end
                return false
        end
}

doujiu = sgs.CreateTriggerSkill{
name = "doujiu",
        events = {sgs.CardUsed, sgs.CardEffected},
        can_trigger = function(self, target)
                return target and target:isAlive()
        end,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if event == sgs.CardUsed then
                    if player:objectName() ~= room:getCurrent():objectName() then return false end
					local use = data:toCardUse()
					local current=room:getCurrent()
					if current:getPhase() ~=sgs.Player_Play then return false end
					if use.from and use.from:objectName() == room:getCurrent():objectName() then 
						
                        if not use.card:isKindOf("Peach") and not use.card:isKindOf("Analeptic") then return false end
                        if not use.to:contains(use.from) then return false end
						local dying
						for _,p in sgs.qlist(room:getAllPlayers()) do
							if p:hasFlag("Global_Dying") then
								dying=p
								break
							end
						end
						if dying and use.to:contains(dying) then return false end
						if player:isKongcheng() then return false end
						local source =room:findPlayerBySkillName("doujiu") 
						local _data = sgs.QVariant()
						
                        if source and source:hasSkill(self:objectName()) and player:objectName() ~= source:objectName() then 
							_data:setValue(use.from)
							source:setTag("doujiu_target", _data)
							if not room:askForSkillInvoke(source, self:objectName(), _data) then return false end
                                        source:drawCards(1)
                                        
                                        if source:pindian(player, self:objectName())  then
												if source:isWounded() then
													local recov = sgs.RecoverStruct()
													recov.who = player
													recov.recover = 1
													room:recover(source, recov)
												end
                                                local card = use.card
                                                room:setCardFlag(card, "doujiuNul")
                                                use.card = card
                                                data:setValue(use)
                                                --只有出牌阶段？？
												room:setPlayerFlag(player, "Global_PlayPhaseTerminated")
                                                
												return false
                                        end
                        end
					end
                elseif event == sgs.CardEffected then
                        local effect = data:toCardEffect()
                        if effect.card:hasFlag("doujiuNul") then return true end
                end
                return false
        end
}

--自己对别人用某种牌 可以参考on_validate
--[源码改动]client.cpp askForSinglePeach 保证转化为酒的技能也能响应【宴会】
--改动相应的翻译文件 sanguosha.ts  sanguosha.qm
--lua中濒死出酒的部分被屏蔽(lua目前只能对宴会使用真正的【酒】 转化不可)
--ai.cpp  Card *TrustAI::askForSinglePeach 对宴会出酒 实际ai没反应。。。
yanhuiCard = sgs.CreateSkillCard{
        name = "yanhuivs",
        filter = function(self, selected, to_select)
                if #selected ~= 0 then return false end
                --if sgs.Sanguosha:getCurrentCardUsePattern() == "peach" then
                --        return to_select:hasLordSkill("yanhui") and sgs.Self:property("currentdying"):toString() == to_select:objectName()
                --end
                return to_select:isWounded() and to_select:hasLordSkill("yanhui")
        end,
        on_validate = function(self, use)
                local room = use.from:getRoom()
				card=sgs.Sanguosha:getCard(self:getSubcards():first())

				card:setSkillName("yanhui")
				return card
        end
}
yanhuivs = sgs.CreateViewAsSkill{
        name = "yanhuivs",
        n = 1,
        view_filter = function(self, selected, to_select)
                --if sgs.Sanguosha:getCurrentCardUsePattern() == "peach" then
                --        return to_select:isKindOf("Analeptic")
                --end
                return to_select:isKindOf("Peach")
        end,
        view_as = function(self, cards)
                if #cards == 0 then return nil end
                local card = yanhuiCard:clone()
                card:addSubcard(cards[1])
				card:setSkillName("yanhui")
                return card
        end,
        enabled_at_play = function(self, player)
                if player:getKingdom() ~= "zhan" then return false end
                for _,p in sgs.qlist(player:getSiblings()) do
                        if p:hasLordSkill("yanhui") and p:isAlive() and p:isWounded() then
                                return true
                        end
                end
                return false
        end,
        --[[enabled_at_response = function(self, player, pattern)
                if player:getKingdom() ~= "zhan" then return false end
                if pattern == "peach" then
                        local victim = player:property("currentdying"):toString()
                        for _,p in sgs.qlist(player:getSiblings()) do
                                if p:hasLordSkill("yanhui") and p:objectName() == victim and p:isAlive() then
                                        return true
                                end
                        end
                end
                return false
        end]]
}
if not sgs.Sanguosha:getSkill("yanhuivs") then
        local skillList=sgs.SkillList()
        skillList:append(yanhuivs)
        sgs.Sanguosha:addSkills(skillList)
end
yanhui = sgs.CreateTriggerSkill{
        name = "yanhui$",
        events = {sgs.GameStart,sgs.EventAcquireSkill,sgs.EventLoseSkill},
        can_trigger = function(self, player) --target
            return player
			--player and player:isAlive() and player:hasLordSkill(self:objectName())
        end,
        on_trigger = function(self, event, player, data)
            
            if event == sgs.GameStart or (event == sgs.EventAcquireSkill and data:toString() == self:objectName()) then
                if not player then return false end
				local room = player:getRoom()
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
					if not p:hasSkill("yanhuivs") then
						room:attachSkillToPlayer(p, "yanhuivs",true)													
					end
				end    
            end
			if (event == sgs.EventLoseSkill and data:toString() == "yanhui") then
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
					if  p:hasSkill("yanhuivs") then
						room:detachSkillFromPlayer(p, "yanhuivs",true)													
					end
				end
			end
            return false
        end
}

zhan006:addSkill(zuiyue)
zhan006:addSkill(doujiu)
zhan006:addSkill(yanhui)
 	
	
--【非想非非想天的少女——比那名居天子】 编号：09007
zhan007 = sgs.General(extension,"zhan007$","zhan",4,false) --09007
feixiang = sgs.CreateTriggerSkill{
        name = "feixiang",
        events = {sgs.AskForRetrial},
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local judge = data:toJudge()
                        local targets = room:getAlivePlayers()
                        for _, p in sgs.qlist(targets) do
                                if p:isKongcheng() then
                                        targets:removeOne(p)
                                end
                        end
                        if targets:isEmpty() then 
							return false 
						end
						prompt="@feixiang-playerchosen:"..judge.who:objectName()..":"..judge.reason
						local _data=sgs.QVariant()
						_data:setValue(judge)
						--ai
						player:setTag("feixiang_judge",_data)
						--player:setTag("feixiang_id",sgs.QVariant(card_id))	
                        --
						local target = room:askForPlayerChosen(player,targets,self:objectName(),prompt,true,true)
                        player:removeTag("feixiang_judge")
						--player:removeTag("feixiang_id")
						if target then
							local card_id = room:askForCardChosen(player, target, "h", self:objectName())
							local card =sgs.Sanguosha:getCard(card_id)
							room:showCard(target,card_id)
							room:retrial(card,target,judge,self:objectName())
						end
        end

}

dizhen = sgs.CreateTriggerSkill{
        name = "dizhen",
        events = {sgs.FinishJudge},
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local judge = data:toJudge()
                local source=room:findPlayerBySkillName(self:objectName())
                if source then
                        if judge.card:isRed() and judge.who:getHp()>0 then
                                local _data = sgs.QVariant()
								_data:setValue(judge)
								source:setTag("dizhen_judge",_data)
								prompt="target:"..judge.who:objectName()..":"..judge.reason
								if room:askForSkillInvoke(source,self:objectName(),sgs.QVariant(prompt)) then
                                        local damage = sgs.DamageStruct()
                                        damage.from = source
                                        damage.to = judge.who
                                        damage.damage = 1
                                        room:damage(damage)
									
                                end
								
                        end
                end
				
        end,

        can_trigger = function(self, target)
                return target
        end
}

tianren = sgs.CreateTriggerSkill{
        name = "tianren$",
        events = sgs.TargetConfirming,
        can_trigger = function(self, target)
                if not target or target:isDead() then return false end
                local room = target:getRoom()
                for _,p in sgs.qlist(room:getOtherPlayers(target)) do
                        if p:hasLordSkill(self:objectName()) then
                                return true
                        end
                end
                return false
        end,
        on_trigger = function (self,event,player,data)
                local room = player:getRoom()
                local use = data:toCardUse()
                if player:getKingdom() ~= "zhan"  then return false end
                if use.card and (use.card:isKindOf("ExNihilo") or use.card:isKindOf("AmazingGrace")) then
                        
                                local lords = sgs.SPlayerList()
                                for _,p in sgs.qlist(room:getOtherPlayers(player)) do
                                        if p:hasLordSkill(self:objectName()) then
                                                lords:append(p)
                                        end
                                end
                                        while not lords:isEmpty() do
											local target = room:askForPlayerChosen(player, lords, self:objectName(), "@"..self:objectName(), true,true)
											
											if not target then return false end
											lords:removeOne(target)
											room:notifySkillInvoked(target, self:objectName())
			
											target:drawCards(1)
										end
                end
                return false
        end,

}

zhan007:addSkill(feixiang)
zhan007:addSkill(dizhen)
zhan007:addSkill(tianren)
	
	
--【美丽的绯之衣——永江衣玖】 编号：09008 
zhan008 = sgs.General(extension,"zhan008", "zhan",4,false) --09008
jingdian = sgs.CreateTriggerSkill{
        name = "jingdian",
        frequency = sgs.Skill_Compulsory,
        events = {sgs.DamageInflicted},--DamageForseen
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local damage = data:toDamage()
				
                if damage.nature == sgs.DamageStruct_Thunder then
						touhou_logmessage("#jingdian",player,"jingdian",nil,damage.damage)
						room:notifySkillInvoked(player, self:objectName())
						for i=1 ,damage.damage, 1 do
                                room:drawCards(player,3,self:objectName())
                        end
						
                        return true
                end
        end
}

leiyun = sgs.CreateViewAsSkill{
        name = "leiyun",
        n=1,

        view_filter = function(self,selected,to_select)
                if #selected<1 then
                return to_select:getSuit()== sgs.Card_Spade or to_select:getSuit()== sgs.Card_Heart and not to_select:isEquipped()
                end
        end,

        view_as = function(self,cards)
                if #cards<1 then return nil
                elseif #cards==1
                then
                        local card = cards[1]
                        local suit = card:getSuit()
                        local point = card:getNumber()
                        local id = card:getId()
                        local vs_card = sgs.Sanguosha:cloneCard("lightning",suit,point)
                        vs_card:addSubcard(id)
                        vs_card:setSkillName("leiyun")
                        return vs_card
                end
        end,
}

zhan008:addSkill(jingdian)
zhan008:addSkill(leiyun)
 
 
 --【当代的念写记者——姬海棠极】 编号：09009 
zhan009 = sgs.General(extension,"zhan009", "zhan",4,false) 
--操作应该整合为一步。。。
kuaizhao = sgs.CreateTriggerSkill{
        name = "kuaizhao",
        events = {sgs.DrawNCards,sgs.AfterDrawNCards},
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if event == sgs.DrawNCards then
                        local others = room:getOtherPlayers(player)
                        for _, p in sgs.qlist(room:getOtherPlayers(player)) do
                                if p:isKongcheng() or (not player:inMyAttackRange(p))then
                                        others:removeOne(p)
                                end
                        end
						if others:length()==0 then return false end
                        local target = room:askForPlayerChosen(player,others,self:objectName(),"@kuaizhao-select_one",true,true)
                        if target then
							local handcards = target:getHandcards()
							local num = 0
							for _, p in sgs.qlist(handcards) do
                                if p:isKindOf("BasicCard") then
                                        num = num + 1
                                end
							end
							room:showAllCards(target)
                                
							local count = data:toInt() - 1
							data:setValue(count)
							room:setPlayerMark(player,"kuaizhaoUsed",num)
						end
                elseif event == sgs.AfterDrawNCards then
                        if player:getMark("kuaizhaoUsed")>0 then
							room:drawCards(player,math.min(2,player:getMark("kuaizhaoUsed")))
						end
						room:setPlayerMark(player,"kuaizhaoUsed",0)
                end
        end
}
--[源码改动]因短焦 修改翼包 sp吕蒙 技能【偷袭】fixed_func
duanjiao = sgs.CreateAttackRangeSkill{
    name = "duanjiao",
    fixed_func = function(self,from)
        if from:hasSkill("duanjiao") then
            return 3
        end
    end
}

zhan009:addSkill(kuaizhao)
zhan009:addSkill(duanjiao)
 
--【表情丰富的扑克脸 ——秦心】 编号：09010 
zhan010 = sgs.General(extension,"zhan010$", "zhan",4,false)
shenqi = sgs.CreateTriggerSkill{
	name = "shenqi", 
	frequency = sgs.Skill_Compulsory, 
	--frequency = sgs.Skill_Eternal,
	events = {sgs.GameStart,sgs.EventPhaseStart},  
	can_trigger = function(self, target)
        return target
    end,
	on_trigger = function(self, event, player, data)
		if event==sgs.GameStart then
			room=player:getRoom()
			for _,p in sgs.qlist(room:getAllPlayers())do
				if  p:hasSkill(self:objectName()) then
					if p:getMark("@sad")> 0 
					or p:getMark("@happy")> 0 
					or p:getMark("@anger")> 0  then
						continue
					end
					local choice=room:askForChoice(p,self:objectName(),"@sad+@happy+@anger")
					p:gainMark(choice)
				end
			end
		end
		if event==sgs.EventPhaseStart then
			if player:getPhase() ~= sgs.Player_Start then return false end
			src=room:findPlayerBySkillName(self:objectName())
			if not src then return false end			
			room=player:getRoom()
			marks={"@sad","@happy","@anger"}
				local tmp
				for _,m in pairs(marks) do
					if src:getMark(m)> 0 then
						tmp=m
						break
					end
				end
				
				local choice=room:askForChoice(src,self:objectName(),"@sad+@happy+@anger")
				src:loseMark(tmp)
				src:gainMark(choice)
		end
	end
}
nengmian = sgs.CreateTriggerSkill{
	name = "nengmian", 
	events = {sgs.Damaged,sgs.CardsMoveOneTime},  
	can_trigger = function(self, target)
        return target
    end,
	on_trigger = function(self, event, player, data)
		room=player:getRoom()
		source=room:findPlayerBySkillName(self:objectName())
		if not source then return false end
		if event==sgs.Damaged then	--怒
			local damage = data:toDamage()
			if source:getMark("@anger")==0 then return false end
			if not damage.from or damage.from:isDead() then return false end
			if not damage.to or damage.to:isDead() then return false end
			local prompt="damagetarget:"..damage.from:objectName()..":"..damage.to:objectName()
			
			if source:inMyAttackRange(damage.to)  and room:askForSkillInvoke(source,self:objectName(),sgs.QVariant(prompt)) then
				if not room:askForCard(source, ".H", "@nengmian-discard") then
                    room:loseHp(source)
                end
				local damage1 = sgs.DamageStruct()
                damage1.from = source
				damage1.to = damage.from
                damage1.damage = 1
                room:damage(damage1)
			end
		end
		if event==sgs.CardsMoveOneTime then --忧
			if source:getMark("@sad")>0 then 
				local move = data:toMoveOneTime()
				can_invoke=false
				if move.from and move.from:objectName() ~= source:objectName() then
					if move.from:getPhase() ~=sgs.Player_NotActive then return false end
					if player:objectName() ~=source:objectName() then return false end
			
					if move.to and move.to:objectName() == move.from:objectName() and move.to_place== sgs.Player_PlaceHand then return false end
					for _,id in sgs.qlist(move.card_ids) do
						if move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceHand  then
							can_invoke=true
							break
						end		
					end
				end
				if not can_invoke then return false end
				local prompt="givetarget:"..move.from:objectName()
			
				if room:askForSkillInvoke(source,self:objectName(),sgs.QVariant(prompt)) then
					source:drawCards(1)
					if source:isKongcheng() then return false end
					local cards=room:askForExchange(source, self:objectName(), 1, false, "@nengmian-give:"..move.from:objectName())
					local mo = sgs.CardsMoveStruct()
					mo.card_ids:append(cards:getSubcards():first())
					mo.to = move.from
					mo.to_place = sgs.Player_PlaceHand
					room:moveCardsAtomic(mo,false)
				end
			end
		end
		if event==sgs.CardsMoveOneTime then --喜
			if source:getMark("@happy")>0 and not room:getTag("FirstRound"):toBool()then 
				if player:objectName() ~=source:objectName() then return false end
				if source:getPhase() ==sgs.Player_Draw then return false end
				local move = data:toMoveOneTime()
				if move.to and move.to:objectName()==source:objectName() and move.to_place==sgs.Player_PlaceHand then
					local target = room:askForPlayerChosen(source,room:getOtherPlayers(source),self:objectName(),"@nengmian-draw",true,true)
					if target then
						target:drawCards(1)
					end
				end
			end
		end
	end
}


xiwang = sgs.CreateTriggerSkill{
        name = "xiwang$",
        events = {sgs.EventPhaseStart,sgs.EventPhaseChanging},
        can_trigger = function(self, target)
            return true
        end,
        on_trigger = function (self,event,player,data)
             local room = player:getRoom()
			 if event == sgs.EventPhaseStart  then 					   
				current=room:getCurrent()
				lord=room:getLord()
				if current:objectName()==lord:objectName() then return false end
				if current:getKingdom() ~="zhan" then return false end
				if current:objectName()==player:objectName() and player:getPhase() == sgs.Player_Discard then
					
					if player:getHandcardNum() <= player:getMaxCards() then return false end
					if(not room:askForSkillInvoke(player, self:objectName(),data)) then return false end
					room:setPlayerMark(player, "hope", 1)
					for _,p in sgs.qlist(room:getOtherPlayers(player)) do
                        if p:hasLordSkill(self:objectName()) then
                           p:drawCards(1)
                        end
                    end
				end	
			 end
			 if event==sgs.EventPhaseChanging then
				local change = data:toPhaseChange()
				if change.to == sgs.Player_NotActive then
					room:setPlayerMark(player, "hope", 0)
				end
			end
        end,

}
xiwang_maxcard = sgs.CreateMaxCardsSkill{
    name = "#xiwang",
    extra_func = function (self,target)
        if target:getMark("hope")>0 then
            return 1
        end
    end
}
zhan010:addSkill(shenqi)
zhan010:addSkill(nengmian)
zhan010:addSkill(xiwang)
zhan010:addSkill(xiwang_maxcard)
extension:insertRelatedSkills("xiwang", "#xiwang_maxcard") 


