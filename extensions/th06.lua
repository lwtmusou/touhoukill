module("extensions.th06", package.seeall)
extension = sgs.Package("th06")
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
--技能代码   红魔乡
------------------------------------------------------------------------------------------------------------
--【红之恶魔——蕾米莉亚•斯卡雷特】 编号：06001 
hmx001 = sgs.General(extension,"hmx001$","hmx",3,false)
hasKexueGenerals = function(player)
    for _, p in sgs.qlist(player:getAliveSiblings()) do
		if p:getHp() < 1 and p:hasSkill("skltkexue") then return true end 
    end
    return false
end
skltkexuepeachcard=sgs.CreateSkillCard{
name = "skltkexuepeach",
target_fixed=true,
will_throw=false,
on_use=function(self,room,source,targets)
        local room=source:getRoom()
		local who=room:getCurrentDyingPlayer()
        if who and who:hasSkill("skltkexue") then
                room:notifySkillInvoked(who, self:objectName())
                room:loseHp(source)
                source:drawCards(2)                
				local peach=sgs.Sanguosha:cloneCard("peach", sgs.Card_SuitToBeDecided, 0)
                peach:setSkillName("_skltkexue")
				
				room:notifySkillInvoked(who, "skltkexue")
				room:useCard(sgs.CardUseStruct(peach, source, who))				
        end
end
}
skltkexuepeach=sgs.CreateViewAsSkill{
name="skltkexuepeach",
n=0,

view_as=function(self,cards)    
   local card=skltkexuepeachcard:clone()
   return card
end,
enabled_at_play=function(self,player)
        return false
end,
enabled_at_response = function(self, player, pattern)
	if  player:getHp()>1 and (string.find(pattern, "peach") ) then
		return hasKexueGenerals(player)  and player:getMark("Global_PreventPeach") == 0
	end
	return false
end
}
skltkexue=sgs.CreateTriggerSkill{
name="skltkexue",
events={sgs.GameStart,sgs.EventAcquireSkill,sgs.Death,sgs.EventLoseSkill},
can_trigger=function(self,  player)
	return player
end,
on_trigger=function(self,event,player,data)	
		local room=player:getRoom()
        if event==sgs.GameStart or (event == sgs.EventAcquireSkill and data:toString() == "skltkexue")then
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
                if not p:hasSkill("skltkexuepeach") then
                    room:attachSkillToPlayer(p, "skltkexuepeach",true)													
                end
            end

		elseif event==sgs.Death or (event == sgs.EventLoseSkill and data:toString() == "skltkexue") then
			local room = player:getRoom()
			local onwers=sgs.SPlayerList()
			local others=sgs.SPlayerList()
			if event==sgs.Death then
				local death = data:toDeath()
				if not death.who:hasSkill("skltkexue")  then return false end
			end
			for _,liege in sgs.qlist(room:getAlivePlayers()) do
				if liege:hasSkill("skltkexue") then
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
				if (liege:hasSkill("skltkexuepeach")) then
					room:detachSkillFromPlayer(liege, "skltkexuepeach",true)
				end
			end
        end
end
}

skltmingyun = sgs.CreateTriggerSkill{
        name = "skltmingyun" ,
        events = {sgs.StartJudge} ,
        can_trigger = function(self, target)
                return target
        end ,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                prejudge=data:toJudge()
				local sklt = room:findPlayerBySkillName(self:objectName())
                if sklt then
						--for ai
                        sklt:setTag("mingyun_judge", data)
						--
						prompt="judge:"..prejudge.who:objectName()..":"..prejudge.reason
						if sklt:askForSkillInvoke(self:objectName(), sgs.QVariant(prompt)) then
                                sklt:drawCards(2)
								--local c = room:askForCard(sklt, ".|.|.|hand", "@skltmingyun", data, sgs.Card_MethodResponse, nil, true)
                                local cards=room:askForExchange(sklt, self:objectName(), 1, false, "@skltmingyun")
								id = cards:getSubcards():first()
								
								local judge = data:toJudge()
								card_ids = sgs.IntList()
								card_ids:append(id)	
								local move=sgs.CardsMoveStruct()
								move.card_ids=card_ids
								move.from=sklt
								move.to_place=sgs.Player_DrawPile
								room:moveCardsAtomic(move, false)
								room:getThread():delay()
								--直接打出最初的判定牌 本身没问题  主要是和红颜等锁定技冲突。。。
								--需要filtercard
								--[[judge.card = c
                                data:setValue(judge)
                                room:moveCardTo(c, judge.who, sgs.Player_PlaceJudge,
                                                sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_JUDGE, sklt:objectName(), judge.who:objectName(), nil, judge.reason), true)
                                
								
								judge:updateResult()
								room:setTag("SkipGameRule", sgs.QVariant(true))]]
								
						end
					
                end
                return false
        end ,
}

function targetsTable2QList(thetable)
        local theqlist = sgs.PlayerList()
        for _, p in ipairs(thetable) do
                theqlist:append(p)
        end
        return theqlist
end
skltxueyicard = sgs.CreateSkillCard{
        name = "skltxueyi" ,
        filter = function(self, targets, to_select, player)
                local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
                return slash:targetFilter(targetsTable2QList(targets), to_select, player)
        end ,
		on_validate= function(self, cardUse)
                cardUse.m_isOwnerUse = false
                local liubei = cardUse.from
                local targets = sgs.SPlayerList()
                for _, t in sgs.qlist(cardUse.to) do
                        targets:append(t)
                end

                local room = liubei:getRoom()
				
				touhou_logmessage("#ChoosePlayerWithSkill",liubei,"skltxueyi",targets,nil,1)
                local slash = nil
                local lieges = room:getLieges("hmx", liubei) 
                for _, target in sgs.qlist(targets) do
                        target:setFlags("skltxueyitarget")
                end
                for _, liege in sgs.qlist(lieges) do
                        local _data=sgs.QVariant()
						_data:setValue(liubei)
						slash = room:askForCard(liege, "slash", "@skltxueyi-slash:" .. liubei:objectName(),
                                        _data, sgs.Card_MethodResponse, liubei, false, nil, true)
                        if slash then
                                for _, target in sgs.qlist(targets) do
                                        target:setFlags("-skltxueyitarget")
                                end
                                return slash
                        end
                end
                for _, target in sgs.qlist(targets) do
                        target:setFlags("-skltxueyitarget")
                end
                room:setPlayerFlag(liubei, "Global_skltxueyiFailed")
                return nil
        end ,
}
hasHmxGenerals = function(player)
        for _, p in sgs.qlist(player:getAliveSiblings()) do
                if p:getKingdom() == "hmx" then return true end --测试时的魏势力
        end
        return false
end
skltxueyiVS =sgs.CreateZeroCardViewAsSkill{ --回合外使用杀 
	name = "skltxueyi$",
		
    view_as = function(self)
        local card=skltxueyicard:clone()
		card:setSkillName(self:objectName())
		return card
    end ,  
	
	enabled_at_play = function(self, player)
        return false
    end,
        
	enabled_at_response = function(self, player, pattern)        
			return hasHmxGenerals(player) and(pattern == "slash")
                       and (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE)
                        and (not player:hasFlag("Global_skltxueyiFailed"))
                       and (player:getPhase() == sgs.Player_NotActive)
    end,
}

skltxueyi = sgs.CreateTriggerSkill{
        name = "skltxueyi$" ,
        events = {sgs.CardAsked} ,
        view_as_skill = skltxueyiVS,
        can_trigger = function(self, target)
                return target and target:hasLordSkill("skltxueyi")
        end ,
        on_trigger = function(self, event, player, data)
                if player:getPhase() ~= sgs.Player_NotActive then return false end
                local pattern = data:toStringList()[1]
                local prompt = data:toStringList()[2]
                local can = ((pattern == "slash") or (pattern == "jink"))
                --检测prompt是为了避免类似刘备，伪帝互相激将的问题 pattern starts with @
				if (not can) or string.find(prompt, "@skltxueyi") then return false end
                local room = player:getRoom()
                local lieges =room:getLieges("hmx", player) 
                if (lieges:isEmpty()) then return false end
                player:setTag("skltxueyi_pattern",data)
				local prompt1="pattern:"..pattern
				if not player:askForSkillInvoke(self:objectName(), sgs.QVariant(prompt1)) then return false end
                local ai_data=sgs.QVariant()
				ai_data:setValue(player)
				for _, liege in sgs.qlist(lieges) do
                        local card = room:askForCard(liege, pattern, "@skltxueyi-" .. pattern .. ":" .. player:objectName(),
                                        ai_data, sgs.Card_MethodResponse, player, false, nil, true)
                        if card then
                                room:provide(card)
                                return true
                        end
                end
                return false
        end ,
}

hmx001:addSkill(skltkexue)
hmx001:addSkill(skltmingyun)
hmx001:addSkill(skltxueyi)

if not sgs.Sanguosha:getSkill("skltkexuepeach") then
        local skillList = sgs.SkillList()
        skillList:append(skltkexuepeach)
        sgs.Sanguosha:addSkills(skillList)
end


 --【恶魔之妹——芙兰朵露•斯卡雷特】 编号：06002
hmx002 = sgs.General(extension,"hmx002" ,"hmx",3,false) --06002
pohuai = sgs.CreateTriggerSkill{
        name = "pohuai",
        frequency = sgs.Skill_Compulsory,
        events = sgs.EventPhaseStart,
        on_trigger = function(self,event, player,data)
                if player:getPhase() ~= sgs.Player_Start then return false end
                local room = player:getRoom()
				touhou_logmessage("#TriggerSkill",player,"pohuai")
				room:notifySkillInvoked(player, self:objectName())
                        
				local judge = sgs.JudgeStruct()
                judge.who = player
                judge.pattern = "Slash"
                judge.good = true
                judge.negative = false
                judge.play_animation = true
                judge.reason = self:objectName()
                room:judge(judge)
                if judge:isBad() then return false end
				local all=sgs.SPlayerList()
                for _,p in sgs.qlist(room:getAlivePlayers()) do
                     if player:distanceTo(p) <= 1 then
                           all:append(p)
                     end
                end
                if all:isEmpty() then return false end
                room:getThread():delay()
				room:sortByActionOrder(all)
				for _,p in sgs.qlist(all) do
					local damage = sgs.DamageStruct()
					damage.from = player
					damage.to = p
					damage.reason=self:objectName()
					room:damage(damage)
				end
                return false
        end
}

yuxue = sgs.CreateTriggerSkill{
        name = "yuxue" ,
        events = {sgs.PreCardUsed, sgs.Damaged, sgs.ConfirmDamage,sgs.CardFinished} ,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if event == sgs.Damaged then
                        local damage =data:toDamage()
						room:setPlayerMark(player, "yuxue", damage.damage)
						room:askForUseCard(player, "slash", "@yuxue", -1, sgs.Card_MethodUse, false) 	
						room:setPlayerMark(player, "yuxue", 0)
                elseif event == sgs.PreCardUsed then
                        local use = data:toCardUse()
                        if (player:getMark("yuxue") > 0) and use.card and use.card:isKindOf("Slash") then
                                room:setPlayerMark(player, "yuxue", 0)
                                room:setCardFlag(use.card, "yuxueinvoked")
								touhou_logmessage("#ChoosePlayerWithSkill",player,"yuxue",use.to,nil,1)
								room:notifySkillInvoked(player, self:objectName())
                        end
                elseif event == sgs.ConfirmDamage then
                        local damage = data:toDamage()
                        --使用杀的过程中，芙兰死亡，浴血杀还是两血？？ 还是要像酒一样永久性的
						if (damage.chain or damage.transfer or not damage.by_user) then return false end
						if not damage.from or not damage.to then return false end
						if damage.from:objectName() ==damage.to:objectName() then return false end
			
						if damage.card and damage.card:isKindOf("Slash") and damage.card:hasFlag("yuxueinvoked") then
                                damage.damage = damage.damage + 1
								touhou_logmessage("#yuxue_damage",damage.from,"yuxue",damage.to)
								data:setValue(damage)
                        end
                end
                return false
        end ,
}
yuxuedis = sgs.CreateTargetModSkill{
        name = "#yuxue-dis" ,
        pattern = "Slash" ,
        distance_limit_func = function(self, player, card)
                if player:getMark("yuxue") > 0 then return 1000 end
                return 0
        end ,
}

shengyan = sgs.CreateTriggerSkill{
        name = "shengyan",
        events = sgs.Damage,
		frequency = sgs.Skill_Frequent,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                local damage = data:toDamage()
                for i=1,damage.damage do
                        if room:askForSkillInvoke(player, self:objectName(), data) then
                                player:drawCards(1)
                        else
                                break
                        end
                end
                return false
        end
}

hmx002:addSkill(pohuai)
hmx002:addSkill(yuxue)
hmx002:addSkill(yuxuedis)
extension:insertRelatedSkills("yuxue", "#yuxue-dis")
hmx002:addSkill(shengyan)


--【完美潇洒的从者——十六夜咲夜】 编号：06003 --by三国有单
hmx003 = sgs.General(extension, "hmx003", "hmx", 3,false)
suoding_card = sgs.CreateSkillCard{
	name = "suoding",
	target_fixed = false,
	will_throw = true,
	filter = function(self, targets, to_select, player)
		return #targets <3  and to_select:getHandcardNum()>0
	end,	
	on_use = function(self, room, source, targets)
		local count=0
		for var=1, #targets,1 do
			local target=targets[var]
			local card_id = room:askForCardChosen(source, target, "h", self:objectName())
			target:addToPile("suoding_cards", card_id,false)
			count=count+1
		end
		while count<3 do
			local listt=room:getAlivePlayers()
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				if p:getHandcardNum()==0 then
					listt:removeOne(p)
				end
			end
			local num_str=tostring(3-count)
			local extra_target = room:askForPlayerChosen(source, listt, self:objectName(),"@@suoding:"..num_str,true,true)
			if extra_target then
				local card_id = room:askForCardChosen(source, extra_target, "h", self:objectName());
				extra_target:addToPile("suoding_cards", card_id,false)
				count=count+1
			else
				break
			end
		end
		if source:getMark("suoding_reason")==0 then
			room:setPlayerMark(source, "suoding_reason", 1)
		end
	end,
}
suodingVS= sgs.CreateViewAsSkill{
	name = "suoding",
	n = 0,	
	enabled_at_play = function(self,player)
		return not player:hasUsed("#suoding")
	end,	
	view_as = function(self, cards)
			local card=suoding_card:clone()
			card:setSkillName(self:objectName())
			return card
	end
	
}
suoding = sgs.CreateTriggerSkill{
	name = "suoding",
	view_as_skill=suodingVS,
	events = {sgs.EventPhaseEnd,sgs.Death},
	can_trigger=function(self,player)
		return player  
	end,
	on_trigger=function(self,event,player,data)
		local room=player:getRoom()
		if (event ==sgs.EventPhaseEnd and player:getPhase() == sgs.Player_Finish) then  
			local sakuya
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				if  p:getMark("suoding_reason")>0 then
					sakuya=p
					break
				end
			end
			if not sakuya then return false end
			for _,liege in sgs.qlist(room:getAlivePlayers()) do 
				if (not liege:getPile("suoding_cards"):isEmpty()) then
					local source=room:findPlayerBySkillName("suoding") 
					if source then
						room:notifySkillInvoked(source, self:objectName())
					else
						room:notifySkillInvoked(player, self:objectName())
					end
					local suodings = liege:getPile("suoding_cards")
					local move=sgs.CardsMoveStruct()
					move.card_ids=suodings
					move.to_place=sgs.Player_PlaceHand
					move.to=liege
					room:moveCardsAtomic(move, false)
					
					touhou_logmessage("#suoding_Trigger",player,self:objectName(),liege,suodings:length())
				end
			end
			room:setPlayerMark(sakuya, "suoding_reason", 0)
		end
			
		if event ==sgs.Death then
			local death = data:toDeath()
			if death.who:getMark("suoding_reason") ==0 then return false end
			for _,liege in sgs.qlist(room:getOtherPlayers(player)) do
				if (not liege:getPile("suoding_cards"):isEmpty()) then
					if liege:objectName()==death.who:objectName() then
						local suodings = liege:getPile("suoding_cards")
						local move=sgs.CardsMoveStruct()
						move.card_ids=suodings
						move.to_place=sgs.Player_DiscardPile
						move.to=nil
						room:moveCardsAtomic(move, true)
					else
						room:notifySkillInvoked(death.who, self:objectName())
						room:broadcastSkillInvoke(self:objectName())
						
						local suodings = liege:getPile("suoding_cards")
						local move=sgs.CardsMoveStruct()
						move.card_ids=suodings
						move.to_place=sgs.Player_PlaceHand
						move.to=liege
						room:moveCardsAtomic(move, false)
						touhou_logmessage("#suoding_Trigger",player,self:objectName(),liege,suodings:length())
					end
				end
			end
			room:setPlayerMark(death.who, "suoding_reason", 0)
		end
	end,
}
	
huisu = sgs.CreateTriggerSkill{
	name = "huisu",
	events = {sgs.PreHpLost,sgs.PostHpReduced,sgs.Damaged},

	on_trigger=function(self,event,player,data)
		local x=0
		local room=player:getRoom()	
		if event==sgs.PreHpLost then
			room:setPlayerFlag(player,"huisu_losthp")
		end
		if event==sgs.Damaged then
			local damage = data:toDamage()
			x=damage.damage
		end
		if event ==sgs.PostHpReduced then
			x= data:toInt() --记录体力流失时体力
			if player:getHp()<1 and player:hasFlag("huisu_losthp") then
				room:setPlayerFlag(player,"-huisu_losthp")
				room:enterDying(player, nil)
			end
			room:setPlayerFlag(player,"-huisu_losthp")
		end
		if x<=0 then return false end
		if not player:isAlive() then return false end
		if   room:askForSkillInvoke(player, "huisu") then 					
			for var=1, x ,1 do		
					local judge =sgs.JudgeStruct()
					judge.pattern = ".|red|2~9"  
					judge.good=true
					judge.reason="huisu"
					judge.who=player
					room:judge(judge)
					
					if judge:isGood() then
						local recov = sgs.RecoverStruct()
						recov .recover =1
						recov.who = player
						room:recover(player,recov) --直接回血，
					end
			end	
		end			

	end,
}
hmx003:addSkill(suoding)
hmx003:addSkill(huisu)

	
--	【不动的大图书馆——帕秋莉•诺蕾姬】 编号：06004
hmx004 = sgs.General(extension, "hmx004", "hmx", 3,false)
bolan=sgs.CreateTriggerSkill{
	name="bolan",
	events={sgs.CardFinished,sgs.EventPhaseStart,sgs.BeforeCardsMove,sgs.CardsMoveOneTime,sgs.CardUsed},--
	can_trigger=function(self,player)
		return player   
	end,	
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()	
		local current = room:getCurrent()		
		if event == sgs.CardUsed then
            local use = data:toCardUse()
            if use.from:objectName() ~=player:objectName() then return false end
			if (use.card and use.card:willThrow() ) and use.card:isNDTrick() and (not use.card:isKindOf("Nullification"))then
				if (use.card:isVirtualCard() and use.card:subcardsLength() ~= 1) then
                    return false
				end
				if sgs.Sanguosha:getEngineCard(use.card:getEffectiveId())
                    and sgs.Sanguosha:getEngineCard(use.card:getEffectiveId()):objectName() == use.card:objectName() then
					room:setCardFlag(use.card:getEffectiveId(), "realNDTrick")	
				end
			end
		end	
		if event == sgs.CardsMoveOneTime  then
			local move = data:toMoveOneTime()
			local card = sgs.Sanguosha:getCard(move.card_ids:first())
			if not player:hasSkill(self:objectName()) then return false end
			if (card:hasFlag("realNDTrick")) and move.card_ids:length() == 1 
				and move.to_place == sgs.Player_DiscardPile  
				and move.reason.m_reason == sgs.CardMoveReason_S_REASON_USE 		
				and player:objectName() ~= move.from:objectName()  then 
				local prompt="obtain:"..card:objectName()
				if (not room:askForSkillInvoke(player,self:objectName(),sgs.QVariant(prompt))) then return false end
				player:addToPile("yao_mark", card)	
				room:setCardFlag(card, "-realNDTrick")		
			end
		end
		if event == sgs.EventPhaseStart and(current:getPhase() == sgs.Player_Finish) then
			local patchouli = room:findPlayerBySkillName(self:objectName())
			if patchouli then
				if (not patchouli:getPile("yao_mark"):isEmpty()) then 
					local qiyaos = patchouli:getPile("yao_mark")
				
					local move=sgs.CardsMoveStruct()
					move.card_ids=qiyaos
					move.to_place=sgs.Player_PlaceHand
					move.to=patchouli
					room:moveCardsAtomic(move, true)

					touhou_logmessage("#bolan_Invoke",patchouli,self:objectName(),nil,move.card_ids:length())
					if (patchouli:getHandcardNum()>3) then
						local x =patchouli:getHandcardNum()-3
						room:askForDiscard(patchouli,"qiyao_got",x,x,false,false,"bolan_discard:"..tostring(x))
					end
				end
			end
		end
	end
}

qiyaoVS = sgs.CreateViewAsSkill{
	name = "qiyao" ,
	n = 1 ,
	view_filter = function(self, selected, to_select)
		if sgs.Self:getPile("yao_mark"):isEmpty() then
			return to_select:isNDTrick() and not to_select:isEquipped()
		else
			return not to_select:isEquipped()
		end
		--local pattern ="TrickCard+^DelayedTrick"
		--return sgs.Sanguosha:matchExpPattern(pattern, sgs.Self, to_select)
	end,
	view_as = function(self, cards)
		if  #cards == 1 then
			local card = sgs.Sanguosha:cloneCard("peach", cards[1]:getSuit(), cards[1]:getNumber())     
            card:addSubcard(cards[1])
			card:setSkillName(self:objectName())
			return card
		end
	end ,
	enabled_at_play = function(self, player)
		return false
	end ,
	enabled_at_response = function(self, player, pattern)
		return   string.find(pattern, "peach") 
				and player:getPhase()== sgs.Player_NotActive 
				and player:getMark("Global_PreventPeach") == 0
				and (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE)
	end,
}
qiyao = sgs.CreateTriggerSkill{
	name = "qiyao", 
	view_as_skill = qiyaoVS,
	events = {sgs.PreCardUsed},  
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		local use = data:toCardUse()
		if use.card and use.card:getSkillName()==self:objectName() then
			if sgs.Sanguosha:getEngineCard(use.card:getEffectiveId()):isNDTrick() then
				return false
			end
			local pile=player:getPile("yao_mark")
			if pile:length() == 0 then return false end
			room:fillAG(pile,player)
			local id=room:askForAG(player,pile,false,self:objectName())
            local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, "", nil, self:objectName(), "")
            room:throwCard(sgs.Sanguosha:getCard(id), reason, nil)
            room:clearAG(player)
		end
	end
}
hmx004:addSkill(bolan)
hmx004:addSkill(qiyao)


 --【华人小姑娘——红美铃】 编号：06005
hmx005 = sgs.General(extension,"hmx005", "hmx",4,false) 
douhun = sgs.CreateTriggerSkill{
        name = "douhun",
        events = {sgs.SlashProceed},
        frequency = sgs.Skill_Compulsory,
        priority = 1000,--优先于强命
        can_trigger = function(self, target)
               return target and target:isAlive()
        end,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                local effect = data:toSlashEffect()
                if not effect.from:hasSkill(self:objectName()) and not effect.to:hasSkill(self:objectName()) then return false end
				local douhun_type
				local from
				local to
				if effect.from:hasSkill(self:objectName()) then
					from=effect.from
					douhun_type=1
					to=effect.to
				else
					from=effect.to
					douhun_type=2
					to=effect.from
				end
				
				touhou_logmessage("#douhun_invoke"..tostring(douhun_type),from,"douhun",to)
				room:notifySkillInvoked(from, self:objectName())
				local first, second = effect.to, effect.from
                while true do
                        if first:isDead() then break end
                        if not room:askForCard(first, "slash", "douhun-slash:"..from:objectName(), data, sgs.Card_MethodResponse, second) then
                                break
                        end
                        first, second = second, first
                end
                effect.to = first
				--该设置flag避免使用杀对目标角色造成伤害时的问题？
				--目前是各技能检测damage.from==damage.to
                room:slashResult(effect, nil)
                return true
        end
}

hmlzhanyi = sgs.CreateOneCardViewAsSkill{
        name = "hmlzhanyi" ,
        filter_pattern = ".|.|.|hand" ,
        view_as = function(self, card)
                local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_SuitToBeDecided, -1)
                slash:addSubcard(card)
                slash:setSkillName(self:objectName())
                return slash
        end ,
		
        enabled_at_play = function(self, player)             
			return false
        end ,
        enabled_at_response = function(self, player, pattern)  
			return pattern == "slash" and (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE)
        end ,
}
hmlzhanyi_draw=sgs.CreateTriggerSkill{
	name="#hmlzhanyi_draw",
	events = {sgs.CardResponded},

	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local card_star = data:toCardResponse().m_card
		if (card_star:getSkillName()=="hmlzhanyi") and card_star:isRed() then
			player:drawCards(1)
		end
	end
}

hmx005:addSkill(douhun)
hmx005:addSkill(hmlzhanyi)
hmx005:addSkill(hmlzhanyi_draw)
extension:insertRelatedSkills("hmlzhanyi", "#hmlzhanyi_draw")


--【湖上的冰精——琪露诺】 编号：06006 	
hmx006 = sgs.General(extension,"hmx006", "hmx",3,false) --06006
dongjie = sgs.CreateTriggerSkill{
        name = "dongjie",
        frequency = sgs.Skill_NotFrequent,
        events ={sgs.DamageCaused},

        on_trigger = function(self,event,player,data)
            local room = player:getRoom()      
            if (event == sgs.DamageCaused) then
				local damage = data:toDamage()
				if (damage.chain or damage.transfer or not damage.by_user) then return false end
				if not damage.from or not damage.to then return false end
				if damage.from:objectName() ==damage.to:objectName() then return false end
			
				if  damage.card and damage.card:isKindOf("Slash") then
					--for ai
					local _data=sgs.QVariant()
					_data:setValue(damage.to)
					local a_data=sgs.QVariant()
					a_data:setValue(damage)
					player:setTag("dongjie_damage",a_data)
					if room:askForSkillInvoke(player,"dongjie",_data)then		
						touhou_logmessage("#Yishi",player,"dongjie",damage.to)
						damage.to:turnOver()                     
						return true
					end
                end
			end
        end
}

bingpo = sgs.CreateTriggerSkill{
        name = "bingpo",
        frequency = sgs.Skill_Compulsory,
        events = {sgs.DamageInflicted},
		priority=-1,
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local damage = data:toDamage()
                if damage.nature ~= sgs.DamageStruct_Fire then
                    if damage.damage>1 or player:getHp()==1 then
						touhou_logmessage("#bingpolog",player,"bingpo",nil,tostring(damage.damage))
						room:notifySkillInvoked(player, self:objectName())
						return true
                    end
                end
        end
}

bendan = sgs.CreateFilterSkill{
        name = "bendan" ,

        view_filter = function(self, to_select)
                return true
        end,
        view_as = function(self, card)
                local id = card:getEffectiveId()
                local new_card = sgs.Sanguosha:getWrappedCard(id)
                new_card:setSkillName("bendan")
                new_card:setNumber(9)
                new_card:setModified(true)
                return new_card
        end
}

hmx006:addSkill(dongjie)
hmx006:addSkill(bingpo)
hmx006:addSkill(bendan)

	
--【宵暗的妖怪——露米娅】 编号：06007  --by三国有单
hmx007 = sgs.General(extension, "hmx007", "hmx", 3,false)	
zhenye = sgs.CreateTriggerSkill{
		name = "zhenye", 
		events = {sgs.EventPhaseStart},  
		view_as_skill=zhenyevs,
		on_trigger = function(self, event, player, data)
			local room = player:getRoom()--创建房间
			if(player:getPhase() == sgs.Player_Finish) then
				--filter_pattern = ".|black|.|hand" ,
				--room:askForUseCard(player, "@@zhenye", "@zhenye")			
				local target = room:askForPlayerChosen(player, room:getOtherPlayers(player), self:objectName(), "@zhenye-select", true, true)
				if target then
					player:turnOver()
					target:turnOver()
				end
			end
		end
}

anyu = sgs.CreateTriggerSkill{--技能"暗域"
		name = "anyu", 
		frequency = sgs.Skill_Compulsory,
		events = {sgs.TargetConfirming, sgs.SlashEffected, sgs.CardEffected},  --PhaseChange每阶段变化
		on_trigger = function(self, event, player, data)
			local room = player:getRoom()

			if event == sgs.TargetConfirming then
				local use = data:toCardUse()
				if  (use.card:isKindOf("Slash") or use.card:isKindOf("Duel")) then
					if  player:faceUp() then return false end
					_data=sgs.QVariant()
					_data:setValue(use)
					room:setTag("anyu_use",_data)
					if room:askForSkillInvoke(player,self:objectName(),sgs.QVariant("turnover:"..use.card:objectName()..":"..use.from:objectName())) then
						player:turnOver()
					else
						room:setCardFlag(use.card,"anyu_nullify")
					end
				end
			end
			if event == sgs.SlashEffected then
				local effect = data:toSlashEffect()
                if effect.slash and effect.slash:hasFlag("anyu_nullify") then
                    effect.slash:clearFlags()
                    
					touhou_logmessage("#DanlaoAvoid",effect.to,effect.slash:objectName(),nil,self:objectName())	
					return true
                end
			end
			if event == sgs.CardEffected then
				local effect = data:toCardEffect()
                if effect.card and effect.card:isKindOf("Duel") and effect.card:hasFlag("anyu_nullify") then
                    effect.card:clearFlags()
									
					touhou_logmessage("#DanlaoAvoid",effect.to,effect.card:objectName(),nil,self:objectName())				
                    return true
                end
			end
		end
}

hmx007:addSkill(zhenye)
hmx007:addSkill(anyu)

--【图书馆中的使魔——小恶魔】编号：06008
hmx008 = sgs.General(extension,"hmx008", "hmx",3,false) --06008
qiyue = sgs.CreateTriggerSkill{
        name = "qiyue",
        events = sgs.EventPhaseStart,
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local source = room:findPlayerBySkillName(self:objectName())
                local current = room:getCurrent()
				if not source then return false end
                if player:getPhase() == sgs.Player_Start and current:objectName() ~= source:objectName() then
						local prompt="target:"..current:objectName()
						if room:askForSkillInvoke(source,self:objectName(),sgs.QVariant(prompt)) then
                                source:drawCards(1)
                                local choice = room:askForChoice(source,self:objectName(),"hp_moxue+maxhp_moxue",data)
                                if choice == "hp_moxue" then
                                        room:loseHp(source,1)
                                else
                                        room:loseMaxHp(source,1)
                                end
                                current:skip(sgs.Player_Judge)
                                current:skip(sgs.Player_Draw)
                        end
                end
        end,
        can_trigger = function(self,target)
                return target
        end
}

moxue = sgs.CreateTriggerSkill{
        name = "moxue",
        frequency = sgs.Skill_Compulsory,
        events = {sgs.MaxHpChanged},
        on_trigger = function (self,event,player,data)
			if event==sgs.MaxHpChanged then	
				local room=player:getRoom()
				if player:getMaxHp()==1 then
						touhou_logmessage("#TriggerSkill",player,"moxue")
						room:notifySkillInvoked(player, self:objectName())
                        local handcardsnum = player:getHandcardNum()
                        player:drawCards(handcardsnum)
                end
			end
        end,
}

hmx008:addSkill(qiyue)
hmx008:addSkill(moxue)

--【雾之湖畔的妖精——大妖精】 编号：06009
hmx009 = sgs.General(extension,"hmx009", "hmx",3,false) --06009
--askforsuit直接改成点fillag？？？ 但是可以挑选完全不存在的花色？
juxiancard=sgs.CreateSkillCard{
name="juxian"
}
juxian=sgs.CreateTriggerSkill{
name="juxian",
events=sgs.Dying,
on_trigger=function(self,event,player,data)
        local room=player:getRoom()
        local who=room:getCurrentDyingPlayer()
        if player:objectName()==who:objectName() and player:faceUp() then
                if player:askForSkillInvoke(self:objectName(),data) then
                        room:notifySkillInvoked(player, self:objectName())
                        room:broadcastSkillInvoke(self:objectName())
                        player:turnOver()
                        local list=room:getNCards(3)
                        room:fillAG(list)
                        local e = {}
                        for _,c in sgs.qlist(list) do
                                table.insert(e, sgs.Sanguosha:getCard(c):toString())
                        end
                        local mes=sgs.LogMessage()
                        mes.type="$TurnOver"
                        mes.from=player
                        mes.card_str=table.concat(e, "+")
                        room:sendLog(mes)
                        --for ai
                                local ai_data = sgs.QVariant()
                                ai_data:setValue(list)
                                player:setTag("juxian_cards", ai_data)
                        --end
                        local suit = room:askForSuit(player, self:objectName())
                        --for ai
                                player:removeTag("juxian_cards")
                        --end
                        
						touhou_logmessage("#ChooseSuit",player,sgs.Card_Suit2String(suit))
                        local get=sgs.IntList()
                        local enter=juxiancard:clone()
                        for _,id in sgs.qlist(list) do
                                if sgs.Sanguosha:getCard(id):getSuit()~=suit then
                                        get:append(id)
                                else
                                        enter:addSubcard(id)
                                end
                        end
                        if not get:isEmpty() then
                                local move=sgs.CardsMoveStruct()
                                move.card_ids=get
                                move.reason.m_reason = sgs.CardMoveReason_S_REASON_DRAW
                                move.to=player
                                move.to_place=sgs.Player_PlaceHand
                                room:moveCardsAtomic(move, true)
                        end
                        if enter:getSubcards():length()>0 then
                                local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_NATURAL_ENTER, player:objectName(), self:objectName(), "")
                                room:throwCard(enter,reason,nil)
                                local recover=sgs.RecoverStruct()
                                recover.recover=enter:getSubcards():length()
                                room:recover(player,recover)
                        end
                        room:clearAG()
                end
        end
end
}

banyuecard=sgs.CreateSkillCard{
name="banyue",
target_fixed=false,
will_throw=true,
filter=function(self,targets,to_select)
        return #targets<3
end,
on_use=function(self,room,source,targets)
        room:loseHp(source)
        for _,p in pairs(targets) do
                if p:isAlive() then
                        p:drawCards(1)
                end
        end
end
}
banyue=sgs.CreateViewAsSkill{
name="banyue",
n=0,
view_as=function(self,cards)
        return banyuecard:clone()
end,
enabled_at_play=function(self,player)
        return not player:hasUsed("#banyue")
end
}

hmx009:addSkill(juxian)
hmx009:addSkill(banyue)
