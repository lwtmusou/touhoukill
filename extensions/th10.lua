module("extensions.th10", package.seeall)
extension = sgs.Package("th10")

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
--技能代码   风神录
------------------------------------------------------------------------------------------------------------
---【山坡与湖水的化身——八坂神奈子】 编号：10001 	
fsl001 = sgs.General(extension,"fsl001$","fsl",4,false) --10001
shendeDummyCard = sgs.CreateSkillCard{
        name = "shendeDummyCard",
        will_throw = false,
        target_fixed = true
}

shendeFakeMoveCard = sgs.CreateSkillCard{
        name = "shendeCard",
        target_fixed = true,
        will_throw = false,
        on_validate = function(self, use)
                local room = use.from:getRoom()
                local card_ids = use.from:getPile("ShenDePile") --
                local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_PREVIEWGIVE, use.from:objectName(),"ShenDePile","ShenDePile")
                local move = sgs.CardsMoveStruct(card_ids, use.from, use.from, sgs.Player_PlaceSpecial, sgs.Player_PlaceHand, reason)
                local moves = sgs.CardsMoveList()
                moves:append(move)
                local players = sgs.SPlayerList()
                players:append(use.from)
                room:notifyMoveCards(true, moves, true, players)
                room:notifyMoveCards(false, moves, true, players)
                --room:moveCardsAtomic(moves, true)
                local card = room:askForUseCard(use.from, "@@shende!", "@shende-twoCards")
                move = sgs.CardsMoveStruct(card_ids, use.from, nil, sgs.Player_PlaceHand, sgs.Player_PlaceTable, reason)
                moves = sgs.CardsMoveList()
                moves:append(move)
                room:notifyMoveCards(true, moves, true, players)
                room:notifyMoveCards(false, moves, true, players)
                --use.from:addToPile("ShenDePile", card_ids)
                if card then
                        local peach = sgs.Sanguosha:cloneCard("peach")
                        for _,id in sgs.qlist(card:getSubcards()) do
                                peach:addSubcard(id)
                        end
                        peach:setSkillName("shende")
                        return peach
                end
                return nil
        end
}

shendeViewAsSkill = sgs.CreateViewAsSkill{
        name = "shende",
        n = 2,
        view_filter = function(self, selected, to_select)
                local pattern = sgs.Sanguosha:getCurrentCardUsePattern()
                if pattern == "@@shende!" then
                        local s_ids = sgs.Self:getTag("shende_piles"):toIntList()
                        return s_ids:contains(to_select:getEffectiveId())
                end
                return false
        end,
        view_as = function(self, cards)
                local pattern = sgs.Sanguosha:getCurrentCardUsePattern()
                if pattern == "@@shende!" then
                        if #cards < 2 then return nil end
                        local card = shendeDummyCard:clone()
                        card:addSubcard(cards[1])
                        card:addSubcard(cards[2])
                        return card
                else
                        local tag = sgs.QVariant()
                        tag:setValue(sgs.Self:getPile("ShenDePile"))
                        sgs.Self:setTag("shende_piles", tag)
                        return shendeFakeMoveCard:clone()
                end
        end,
        enabled_at_play = function(self, player)
                if player:getPile("ShenDePile"):length() < 2 then return false end
                local peach = sgs.Sanguosha:cloneCard("peach")
                return peach:isAvailable(player)
        end,
        enabled_at_response = function(self, player, pattern)
                if player:getMark("Global_PreventPeach") > 0 then return false end
				if pattern == "@@shende!" then return true end
                if player:getPile("ShenDePile"):length() < 2 then return false end
                return string.find(pattern, "peach") 
        end
}

shende = sgs.CreateTriggerSkill{
        name = "shende",
        events = {sgs.CardUsed, sgs.CardResponded},
        view_as_skill = shendeViewAsSkill,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                local card
                if event == sgs.CardUsed then
                        card = data:toCardUse().card
                else
                        card = data:toCardResponse().m_card
                end
                if card and card:isKindOf("Slash") and room:askForSkillInvoke(player, self:objectName(), data) then
                        player:drawCards(1)
                        if player:isKongcheng() then return false end
                        local cards=room:askForExchange(player, self:objectName(), 1, false, "shende-exchange")
                        id = cards:getSubcards():first()
						player:addToPile("ShenDePile", id)
                end
                return false
        end
}

qiankun = sgs.CreateMaxCardsSkill{
        name = "qiankun",
        extra_func = function (self,target)
                if target:hasSkill("qiankun") then
                        return 2
                end
        end

}

gongfengCard = sgs.CreateSkillCard{
                name = "gongfengvs",
                target_fixed = false,
                will_throw = false,
                filter = function(self, targets, to_select)
                                if #targets == 0 then
                                                if to_select:hasLordSkill("gongfeng") then
                                                                if to_select:objectName() ~= sgs.Self:objectName() then
                                                                                return not to_select:hasFlag("gongfengInvoked")
                                                                end
                                                end
                                end
                                return false
                end,
                on_use = function(self, room, source, targets)
                                local fsl001 = targets[1]
                                if fsl001:hasLordSkill("gongfeng") then
                                                room:setPlayerFlag(fsl001, "gongfengInvoked")
                                                fsl001:obtainCard(self)
                                                local subcards = self:getSubcards()
                                                for _,card_id in sgs.qlist(subcards) do
                                                                room:setCardFlag(card_id, "visible")
                                                end
                                                room:setEmotion(fsl001, "good")
                                                local fsl001s = sgs.SPlayerList()
                                                local players = room:getOtherPlayers(source)
                                                for _,p in sgs.qlist(players) do
                                                                if p:hasLordSkill("gongfeng") then
                                                                                if not p:hasFlag("gongfengInvoked") then
                                                                                                fsl001s:append(p)
                                                                                end
                                                                end
                                                end
                                                if fsl001s:length() == 0 then
                                                                room:setPlayerFlag(source, "Forbidgongfeng")
                                                end
                                end
                end
}
gongfengvs = sgs.CreateViewAsSkill{
                name = "gongfengvs",
                n = 1,
                view_filter = function(self, selected, to_select)
                                return to_select:objectName() == "slash" or to_select:objectName() == "fire_slash" or to_select:objectName() == "thunder_slash"
                end,
                view_as = function(self, cards)
                                if #cards == 1 then
                                                local card = gongfengCard:clone()
                                                card:addSubcard(cards[1])
                                                return card
                                end
                end,
                enabled_at_play = function(self, player)
                    if player:getKingdom() == "fsl" then
                        return not player:hasFlag("Forbidgongfeng")
                    end
                    return false
                end
}
gongfeng = sgs.CreateTriggerSkill{
        name = "gongfeng$",
        frequency = sgs.Skill_NotFrequent,
        events = {sgs.GameStart, sgs.EventPhaseChanging,sgs.EventAcquireSkill,sgs.EventLoseSkill},
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
					if not p:hasSkill("gongfengvs") then
						room:attachSkillToPlayer(p, "gongfengvs",true)													
					end
				end                                   
			end
			if (event == sgs.EventLoseSkill and data:toString() == "gongfeng") then
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
					if  p:hasSkill("gongfengvs") then
						room:detachSkillFromPlayer(p, "gongfengvs",true)													
					end
				end
			end
			if event == sgs.EventPhaseChanging then
                local phase_change = data:toPhaseChange()
                if phase_change.from == sgs.Player_Play then
                    if player:hasFlag("Forbidgongfeng") then
                        room:setPlayerFlag(player, "-Forbidgongfeng")
                    end
                    local players = room:getOtherPlayers(player)
                    for _,p in sgs.qlist(players) do
                        if p:hasFlag("gongfengInvoked") then
							room:setPlayerFlag(p, "-gongfengInvoked")
                        end
                    end
                end
            end
            return false
		end,
    can_trigger = function(self, target)
        return target
    end
}

if not sgs.Sanguosha:getSkill("gongfengvs") then
        local skillList=sgs.SkillList()
        skillList:append(gongfengvs)
        sgs.Sanguosha:addSkills(skillList)
end

fsl001:addSkill(shende)
fsl001:addSkill(qiankun)
fsl001:addSkill(gongfeng)

		
--【土著神的顶点——泄矢诹访子】编号：10002	
fsl002 = sgs.General(extension,"fsl002", "fsl",3,false) 
bushu = sgs.CreateTriggerSkill{
        name = "bushu",
        frequency = sgs.Skill_NotFrequent,
        events = {sgs.Damaged,sgs.Pindian},
        on_trigger = function(self, event, player, data)
            if event==sgs.Damaged then
				local damage = data:toDamage()
                local source = damage.from
                local victim = damage.to
				if not source then return false end
				if not victim then return false end
				if not victim:isDead() and source:isAlive() then		
                    local room = player:getRoom()
                    local suwako = room:findPlayerBySkillName(self:objectName())
                        if suwako and not suwako:isKongcheng() then
                           if source and source:objectName() ~= suwako:objectName() and (not source:isKongcheng()) and suwako:inMyAttackRange(victim) then
                                --for ai
								local _data = sgs.QVariant()
								_data:setValue(damage)
								suwako:setTag("bushu_damage",_data)
								prompt="damage:"..damage.from:objectName()..":"..damage.to:objectName()
								if suwako:askForSkillInvoke(self:objectName(), sgs.QVariant(prompt)) then
									local success = suwako:pindian(source, self:objectName(), nil)
                                    if success then
                                        local recover = sgs.RecoverStruct()
                                        recover.recover = 1
                                        room:recover(victim,recover)
                                    end                   
                                end				
                            end
                        end
                end
			end
			if event==sgs.Pindian then
				local pindian =data:toPindian()
				if pindian.reason=="bushu" then
					local suwako=pindian.from
					if pindian.from_number > pindian.to_number then
					else
						if suwako:isAlive() then --防止死后卡牌在手上
							suwako:obtainCard(pindian.to_card)
						end
					end
				else
					return false
				end
			end
		end,
		
        can_trigger = function(self, target)
              return true
        end,
}

chuanchengDummyCard = sgs.CreateSkillCard{
                name = "chuanchengDummyCard"
}
chuancheng = sgs.CreateTriggerSkill{
        name = "chuancheng",
        frequency = sgs.Skill_NotFrequent,
        events = {sgs.Death},
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                local death = data:toDeath()

				if death.who:objectName() == player:objectName() then
      
                        local others = room:getOtherPlayers(player)
                        local target = room:askForPlayerChosen(player,others,self:objectName(),"@chuancheng",true,true)
                        if target then
                            local cards = player:getCards("hej")
							if cards:length() > 0 then
								local allcard = chuanchengDummyCard:clone()
									for _,card in sgs.qlist(cards) do
                                        allcard:addSubcard(card)
									end	 
									room:obtainCard(target, allcard,sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_RECYCLE, target:objectName()), false)

							end
							room:handleAcquireDetachSkills(target,"qiankun")
                            room:handleAcquireDetachSkills(target,"chuancheng")
                         end
	
                end
                return false
        end,
        can_trigger = function(self, target)
                if target then
                        return target:hasSkill(self:objectName())
                end
                return false
        end
}

fsl002:addSkill(bushu)
fsl002:addSkill(qiankun)
fsl002:addSkill(chuancheng)


		
--【被祭拜的风之人——东风谷早苗】 编号：10003 by三国有单	
fsl003 = sgs.General(extension,"fsl003", "fsl",3,false) --10003
zhunbei=sgs.CreateTriggerSkill{
	name="zhunbei",
	events={sgs.DrawNCards,sgs.EventPhaseChanging},
	on_trigger = function(self, event, player, data)
		local room=player:getRoom()
		if event==sgs.DrawNCards then
			if player:askForSkillInvoke(self:objectName(),data) then 
					data:setValue(0)
					player:setFlags("zhunbei")
					
					player:skip(sgs.Player_Draw)
			end	
		end
		if event ==sgs.EventPhaseChanging then
			local change = data:toPhaseChange()
			if change.to == sgs.Player_NotActive then
				if player:hasFlag("zhunbei") then
					player:setFlags("-zhunbei")
					touhou_logmessage("#TriggerSkill",player,"zhunbei")
					room:notifySkillInvoked(player, self:objectName())
					player:drawCards(3)
					
				end
				
			end
		end	
	end
}
--还应该考虑cardlimitation？？ 特别是response？？ 双将才会有这种问题？？
--虽然已经禁止了 viewas实际是不能返回确定并使用出去的 但是前面的点击其实也可以去掉？？（比如被鸡肋基本牌，被借刀还能先选择火杀 雷杀。。。）
--好吧 我偷懒了 

--[源码改动]Room::askForUseSlashTo() 被动使用杀时 如借刀
--被动使用杀时由于中间过了一次技能卡，会导致不论是否返回杀都会导致 Room::askForUseSlashTo() 返回true   


--方案2  一个技能卡askforusecard 另一个接受并targetfilter  由validate 和 validate_in_response 来解决？
card_pattern={}
qijipeachcard=sgs.CreateSkillCard{
	name="qijipeachcard",
	target_fixed=true,
	on_use=function(self, room, source, targets)
		local choice=room:askForChoice(source,"qiji","peach+analeptic")
		room:askForUseCard(source, choice,"@qijisave")
	end,
}

qijislashcard=sgs.CreateSkillCard{--被动使用杀时
	name="qijislashcard",
	target_fixed=true,
	on_use=function(self, room, source, targets)
		local choice=room:askForChoice(source,"qiji","slash+thunder_slash+fire_slash")
		room:setPlayerMark(source, "qiji_response_slash",1)
		local card =room:askForUseCard(source, choice,"@response_slash")
		--取消，并没有真正杀出去时也要注意消除标记		
		if not card then
			room:setPlayerFlag(source, "Global_qijislashFailed")
			room:setPlayerMark(source, "qiji_response_slash",0)
		end
	end,
}
qiji = sgs.CreateViewAsSkill{ --技能“奇迹”viewas
	name = "qiji" ,
	n = 1 ,
	
	view_filter = function(self, selected, to_select)
        return not to_select:isEquipped()
	end,
	--目前此视为技全部用列举。。。
	view_as = function(self, cards)
		if #card_pattern == 0 and sgs.Self:getMark("qijimark")<=0 then  return dfgzm_qijicard:clone() end
		local usereason = sgs.Sanguosha:getCurrentCardUseReason()
		if card_pattern[1]=="peach+analeptic" then
			return qijipeachcard:clone()
		end
		--借刀出杀处理复杂一些
		if card_pattern[1]=="slash" and usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE  
		and sgs.Self:getMark("qiji_response_slash") ==0 then
			return qijislashcard:clone()
		end
		
		if(#cards ~= 1) then return false end
		local card,cardname
		local id=sgs.Self:getMark("qijimark")-1
		if id~=-1 then
			cardname=sgs.Sanguosha:getCard(id):objectName()
		end
		if cardname then
			card=sgs.Sanguosha:cloneCard(cardname, cards[1]:getSuit(), cards[1]:getNumber())
		else
			card=sgs.Sanguosha:cloneCard(card_pattern[1], cards[1]:getSuit(), cards[1]:getNumber())
		end
		card:addSubcard(cards[1])
		card:setSkillName(self:objectName())
		return card
	end,
	
	enabled_at_play = function(self,player)
		if #card_pattern ~= 0 then
			table.remove(card_pattern)
		end
		if player:getMark("qijimark")~=0 then
			player:setMark("qijimark",0)
		end
		return  player:getHandcardNum()==1  and player:getMark("@qiji")==0
	end,
	
	enabled_at_response = function(self, player, pattern)
		if player:getHandcardNum()~=1 or player:getMark("@qiji")>0 then return false end
		if  pattern=="slash" or pattern=="jink" or pattern=="peach" or pattern=="analeptic" or pattern=="peach+analeptic" or pattern=="nullification" then
			table.remove(card_pattern)
			table.insert(card_pattern, pattern)
		end
		if pattern=="fire_slash" or pattern=="thunder_slash" then
			table.remove(card_pattern)
			table.insert(card_pattern, pattern)
		end
		return (#card_pattern ~= 0 or pattern == "@@qiji")
		--其实可以将pattern == "@@qiji" 直接变成具体的各个choice？
		--就是这么做会导致主动出杀（response_use）和被借刀出杀(response_use分辨起来困难？)
	end,
	
	enabled_at_nullification=function(self,player)
		if player:getHandcardNum()==1  and player:getMark("@qiji")==0 then
			return true
		end
	end,
}
dfgzm_qijicard = sgs.CreateSkillCard{--奇迹选牌（出牌阶段） 
	name = "dfgzm_qiji",
	target_fixed = true,
	will_throw = false,	
	
	on_use = function(self, room, source, targets)
		all_card={"cancel"}
		--isCardLimited实际只是对choice做筛选 
		--本来在viewas的时候也用不出去
		if sgs.Slash_IsAvailable(source) then--杀 	
			if not source:isCardLimited(sgs.Sanguosha:cloneCard("slash"), sgs.Card_MethodUse, true) then
			table.insert(all_card,"slash")
			table.insert(all_card,"fire_slash")
			table.insert(all_card,"thunder_slash")
			end
		end
		if  sgs.Analeptic_IsAvailable(source) 
		and not source:isCardLimited(sgs.Sanguosha:cloneCard("analeptic"), sgs.Card_MethodUse, true) then--酒 
			table.insert(all_card,"analeptic")
		end	
		if source:isWounded() 
		and not source:isCardLimited(sgs.Sanguosha:cloneCard("peach"), sgs.Card_MethodUse, true) then--桃
			table.insert(all_card,"peach")
		end
		--非延时锦囊
		tricks={"amazing_grace","god_salvation","savage_assault","archery_attack",
		"duel","ex_nihilo","snatch","dismantlement","collateral","iron_chain","fire_attack"}
		for _,pattern in pairs(tricks) do
			if not source:isCardLimited(sgs.Sanguosha:cloneCard(pattern), sgs.Card_MethodUse, true) then
				table.insert(all_card,pattern)
			end
		end
		--[[table.insert(all_card,"amazing_grace")
		table.insert(all_card,"god_salvation")
		table.insert(all_card,"savage_assault")
		table.insert(all_card,"archery_attack")
		table.insert(all_card,"duel")
		table.insert(all_card,"ex_nihilo")
		table.insert(all_card,"snatch")
		table.insert(all_card,"dismantlement")
		table.insert(all_card,"collateral")
		table.insert(all_card,"iron_chain")
		table.insert(all_card,"fire_attack")]]
			
		--再次调用viewas
		local choice = room:askForChoice(source,self:objectName(),table.concat(all_card,"+"))
		if choice=="cancel" then return false end
		local card
		for id=0,159,1 do
			 card=sgs.Sanguosha:getCard(id)
			if card:objectName()==choice then
				room:setPlayerMark(source,"qijimark",id+1)
				break
			end
		end
		room:askForUseCard(source,"@@qiji","@qiji_target:"..card:objectName()) 

	end,
}
dfgzm_qiji = sgs.CreateTriggerSkill{
	name = "#dfgzm_qiji",
	events = {sgs.PreCardUsed,sgs.CardResponded,sgs.CardAsked,sgs.AskForPeaches,sgs.TrickCardCanceling},--sgs.CardAsked,sgs.AskForPeaches,sgs.PreCardUsed
	view_as_skill = qiji_trick_vs,
	
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		if event==sgs.PreCardUsed then
			local use = data:toCardUse()
			if use.card:getSkillName()=="qiji" then
				player:gainMark("@qiji",1)
				room:setPlayerMark(player,"qijimark",0)
				room:setPlayerMark(player,"qiji_response_slash",0)
			end
		end
		if event==sgs.CardResponded then
			local card_star = data:toCardResponse().m_card
			if card_star:getSkillName()=="qiji" then
				player:gainMark("@qiji",1)
				room:setPlayerMark(player,"qijimark",0)
			end
		end
	end
}
dfgzm_qiji_clear = sgs.CreateTriggerSkill{
	name = "#dfgzm_qiji_clear",
	events = {sgs.EventPhaseChanging},
	can_trigger=function(self,player)
		return player
	end,
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		for _,p in sgs.qlist(room:getAlivePlayers()) do
			if p:getMark("@qiji")>0 then
				room:setPlayerMark(p, "@qiji", 0)
			end	
		end
	end
}
fsl003:addSkill(zhunbei)
fsl003:addSkill(dfgzm_qiji)
fsl003:addSkill(dfgzm_qiji_clear)
fsl003:addSkill(qiji)
extension:insertRelatedSkills("qiji", "#dfgzm_qiji")
extension:insertRelatedSkills("qiji", "#dfgzm_qiji_clear")


 
--【最接近里的天狗——射命丸文】 编号：10004
fsl004 = sgs.General(extension,"fsl004", "fsl",3,false)
fengshenCard = sgs.CreateSkillCard{
        name = "fengshenCard",
        filter = function(self, selected, to_select)
                if to_select:objectName() == sgs.Self:objectName() then
                        return false
                elseif #selected == 0 then
                        return sgs.Self:inMyAttackRange(to_select)
                elseif #selected == 1 then
                        return sgs.Self:distanceTo(selected[1]) <= 1 and sgs.Self:distanceTo(to_select) <= 1
                else
                        return sgs.Self:distanceTo(to_select) <= 1 
                end
        end,
        on_effect = function(self, effect)
                local room = effect.from:getRoom()
                if not room:askForCard(effect.to, "Slash", "@fengshen-discard:"..effect.from:objectName()) then
                        local damage = sgs.DamageStruct()
                        damage.from = effect.from
                        damage.to = effect.to
                        room:damage(damage)
                end		
        end
}
fengshen = sgs.CreateOneCardViewAsSkill{
        name = "fengshen",
        filter_pattern = ".|red|.|hand!",--"Jink",
        view_as = function(self, card)
                local acard = fengshenCard:clone()
                acard:addSubcard(card)
                return acard
        end,
        enabled_at_play = function(self, player)
                return not player:hasUsed("#fengshenCard")
        end
}

fengsu = sgs.CreateDistanceSkill{
        name = "fengsu",
        correct_func = function (self,from,to)
                if from:hasSkill("fengsu") then
                        return -(from:getLostHp())
                end
                if to:hasSkill("fengsu") then
                        return to:getLostHp()
                end
        end
}

fsl004:addSkill(fengshen)
fsl004:addSkill(fengsu)

		
--【超妖怪弹头——河城荷取】 编号：10005 
fsl005 = sgs.General(extension,"fsl005", "fsl",3,false) --10005
xinshangCard = sgs.CreateSkillCard{
        name = "xinshang",
        filter = function(self,selected,to_select)
                return #selected == 0 and to_select:objectName() ~= sgs.Self:objectName()
        end,
        on_effect = function(self,effect)
                local room = effect.from:getRoom()
                effect.to:drawCards(1)            --".|spade"--黑桃牌。。。汗
                local sdata =sgs.QVariant()
				sdata:setValue(effect.from)
				local card = room:askForCard(effect.to,".S","@xinshang-spadecard:"..effect.from:objectName(),sdata,sgs.Card_MethodNone,nil,false,self:objectName(),false)
                if card then
                        room:obtainCard(effect.from,card)
						room:setPlayerMark(effect.from,"xinshang_effect",1)
                else
                        --if effect.to:isNude() 
						if not effect.from:canDiscard(effect.to, "he")  then return false end
						room:throwCard(room:askForCardChosen(effect.from, effect.to, "he", self:objectName(),false,sgs.Card_MethodDiscard),effect.to,effect.from)
                        if effect.from:canDiscard(effect.to, "he") then
						room:throwCard(room:askForCardChosen(effect.from, effect.to, "he", self:objectName(),false,sgs.Card_MethodDiscard),effect.to,effect.from)
						end
                end
				
        end
}

xinshangVS = sgs.CreateViewAsSkill{
        name = "xinshang",
        n = 0,
        view_as = function (self,cards)
                return xinshangCard:clone()
        end,

        enabled_at_play = function(self, player)
                return not player:hasUsed("#xinshang")
        end
}
xinshang= sgs.CreateTriggerSkill{
	name = "xinshang",
	view_as_skill=xinshangVS,
	events = {sgs.EventPhaseChanging},	
	
	on_trigger=function(self,event,player,data)
		local change = data:toPhaseChange()
		if change.to == sgs.Player_NotActive then
			local room=player:getRoom()
			if player:getMark("xinshang_effect")>0 then
				room:setPlayerMark(player,"xinshang_effect",0)
			end
		end
	end,
}
xinshang_effect = sgs.CreateTargetModSkill{
        name = "#xinshang_effect",
        pattern="BasicCard,TrickCard",--Slash
        distance_limit_func = function(self,player,card)
                if player:hasSkill(self:objectName()) and player:getMark("xinshang_effect")>0 then
                        return 100
                else
                        return 0
                end
        end,
		
		residue_func = function(self, player,card)
                if player:hasSkill(self:objectName()) and player:getMark("xinshang_effect") >0 then
                        return 1000
				else
					return 0
                end
        end,

}
micai = sgs.CreateTriggerSkill{
        name = "micai",
        events = sgs.DamageInflicted,
        frequency = sgs.Skill_Compulsory,
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local damage = data:toDamage()
                if damage.to:objectName() == player:objectName() then
                        local num = player:getHandcardNum()

						if num == 0 then
								touhou_logmessage("#micai01",player,"micai",nil,damage.damage-num)
								room:notifySkillInvoked(player, self:objectName())
								
								return true
                        elseif damage.damage > num then
								touhou_logmessage("#micai01",player,"micai",nil,damage.damage-num)
								damage.damage = num
								room:notifySkillInvoked(player, self:objectName())
						end
						
                        data:setValue(damage)
                end
        end
}

fsl005:addSkill(xinshang)
fsl005:addSkill(xinshang_effect)
fsl005:addSkill(micai)
extension:insertRelatedSkills("xinshang", "#xinshang_effect")
	
	
--【秘神流人偶——键山雏】 编号：10006
fsl006 = sgs.General(extension,"fsl006", "fsl",3,false) 
zaihuo_card = sgs.CreateSkillCard{
	name = "zaihuo",
	target_fixed = false,
	will_throw = false,
	
	filter = function(self, targets, to_select, player)
		return #targets == 0 and to_select:objectName() ~= player:objectName()
	end,
	
	on_use = function(self, room, source, targets)
		local target = targets[1]
		local cards= self:getSubcards() 
		local suit1 = sgs.Sanguosha:getCard(cards:first()):getSuit()
		room:showCard(source,cards:first())
		room:obtainCard(target,cards:first(),true)
		
        room:getThread():delay()
		if target:isKongcheng() then return false end
		local card_id=room:askForCardChosen(source,target,"h",self:objectName())
		room:showCard(target,card_id)
		room:getThread():delay()
		
		local suit2 =sgs.Sanguosha:getCard(card_id):getSuit()
		
		if suit2 ~=suit1 then
			local dam = sgs.DamageStruct()
			dam.damage = 1
			dam.from = nil
			dam.to = target
			room:damage(dam)
		end
	end,
}
zaihuo = sgs.CreateViewAsSkill{
        name = "zaihuo",
        n = 1,
		view_filter = function(self, selected, to_select)
			return (not to_select:isEquipped()) 
		end,
		
        view_as = function (self,cards)
            if #cards==1 then    
				local card =zaihuo_card:clone()
				card:addSubcard(cards[1])
				card:setSkillName(self:objectName())
				return card
			end
        end,

        enabled_at_play = function(self, player)
			return not player:hasUsed("#zaihuo")
        end
}

hina_jie=sgs.CreateTriggerSkill{
	name="hina_jie",
	events={sgs.Damaged},
	frequency = sgs.Skill_Frequent,
	can_trigger=function(self,player)
		return player
	end,
	
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local damage = data:toDamage()
		hina=room:findPlayerBySkillName(self:objectName())
		if not damage.to or damage.to:isDead() then return false end
		
		if hina then
			if hina:getPhase() ~= sgs.Player_NotActive then return false end
			if not hina:inMyAttackRange(damage.to) then return false end
			local prompt="target:"..damage.to:objectName()	
			if room:askForSkillInvoke(hina,self:objectName(),sgs.QVariant(prompt)) then  
				hina:drawCards(1)
			
			end
		end	
	end
}

fsl006:addSkill(zaihuo)
fsl006:addSkill(hina_jie)
 
 
 --【现代的女高中生——东风谷早苗】 编号：10007 by三国有单
fsl007 = sgs.General(extension,"fsl007", "fsl",3,false) --10007
--[源码改动]方案1 常识早苗参照新于吉 缠怨 修改player.cpp的函数player:hasSkill（） 
--同时修改room:handleAcquireDetachSkills()  保证 永久技不会被断肠 不会被缠怨

--[源码改动]定义了永久技sgs.Skill_Eternal   
-- sanguosha.i skill.h  不需要枚举永久技
--UI相关  qsanbutton.cpp  void QSanSkillButton::setSkill

--方案2 用倾城mark也ok 但是倾城mark需要绑定具体技能 记录麻烦
--界限突破的关羽可以tag来记录loseskill 但是常识是多个人  对于tag本身还要做序号记录  真心不如直接改一个标记好使

--常识处理步骤 
--0处理禁止系等。。。
--1 全体+mark 2 处理武将牌上的牌
--3处理 “？”“人偶”“天仪”等特殊装备
--4 处理持续展示tag  
--5处理标记
--回合结束时步骤
--0全体去mark
--1归还禁止系。。。



changshi = sgs.CreateTriggerSkill{
	name = "changshi", 
	frequency = sgs.Skill_Eternal,
	priority=5,
	events = {sgs.EventPhaseStart,sgs.EventLoseSkill,sgs.Death,sgs.EventPhaseChanging}, --sgs.TurnStart, 
	can_trigger=function(self,player)
		return player and player:hasSkill(self:objectName())  
	end,
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if event ==sgs.EventPhaseStart and player:getPhase() == sgs.Player_Start then
			touhou_logmessage("#changshi01",player,"changshi")
			room:notifySkillInvoked(player, self:objectName())
			--处理禁止系 只好枚举
			for _,p in sgs.qlist(room:getOtherPlayers(player)) do
				--带来禁止变化系的mark
				local limit_marks={"@shi","@ye"}
				if p:hasSkill("zhouye") then
					if p:getMark("@ye")==0 then
						room:removePlayerCardLimitation(p, "use", "Slash$0")
					end
				end
				if p:hasSkill("yexing") then
					if p:getMark("@shi")==0 then
						room:removePlayerCardLimitation(p, "use", "TrickCard+^DelayedTrick$0")
					end
				end
				if p:hasSkill("aoyi") then
					room:removePlayerCardLimitation(p, "use", "TrickCard+^DelayedTrick$0")
				end
			end
			--加常识mark（封印技能）
			for _,p in sgs.qlist(room:getOtherPlayers(player)) do
					room:setPlayerMark(p,"changshi",1)
					room:filterCards(p, p:getCards("he"), true)
			end
			--倾城mark版
			--[[yongjiu_skills={"changshi","huanmeng","xuying","banling","chuanghuan","shenzhu","hezou","tymh_tianxiang"}
			for _,p in sgs.qlist(room:getOtherPlayers(player)) do
				local lose_skills = p:getTag("ChangshiSkills"):toString():split("+")
				local skills = p:getVisibleSkillList()
				for _, skill in sgs.qlist(skills) do
					if skill:getLocation() == sgs.Skill_Right 
					and not skill:inherits("SPConvertSkill") and not skill:isAttachedLordSkill() 
					and not table.contains(lose_skills,skill:objectName())then
					--not (Set(lose_skills))[skill:objectName()] 
						if not table.contains(yongjiu_skills,skill:objectName()) then
							room:addPlayerMark(p, "Qingcheng"..skill:objectName())
							table.insert(lose_skills, skill:objectName())
						
							--for _, sk in sgs.qlist(sgs.Sanguosha:getRelatedSkills(skill:objectName())) do
							--	room:addPlayerMark(p, "Qingcheng"..sk:objectName())
							--end
						end
					end
				end
				p:setTag("ChangshiSkills", sgs.QVariant(table.concat(lose_skills, "+")))	
			end]]
			
			--处理武将牌上的牌
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				local  idlist=sgs.IntList()
				for _,pile in pairs(p:getPileNames()) do
					if  pile=="meng_list" then 
						--而meng0到meng3 renou0到人偶3可以丢弃
					else
						local cards = p:getPile(pile)
						for _,card in sgs.qlist(cards) do
							idlist:append(card)
						end
					end
				end
				if  idlist:length()>0 then
					local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, p:objectName(), nil, self:objectName(), "")
					local move = sgs.CardsMoveStruct(idlist, p, sgs.Player_DiscardPile,
                                        reason)
					local exchangeMove= sgs.CardsMoveList()						
					exchangeMove:append(move)
					room:moveCardsAtomic(exchangeMove, true)
					--room:throwCard(sgs.Sanguosha:getCard(id), reason, nil)
					--throwcard可以附带log
				end
			end
			--pile 处理完后追加处理距离
			
			--追加处理人偶 ?
			--[[for _,p in sgs.qlist(room:getAlivePlayers()) do
				local throw=sgs.IntList()
				for i=0, 3,1 do
					if p:getEquip(i) then
						equip = p:getEquip(i)
						if equip:objectName():startsWith("renou") then
							throw:append(equip:getId())
						end
						--处理“？”
					end
				end
				if throw:length()>0 then
					local move=sgs.CardsMoveStruct()
					move.card_ids=throw
					move.to_place=sgs.Player_DiscardPile
					move.to=nil
					room:moveCardsAtomic(move, true)
				end
			end]]
			--处理持续展示的tag记录
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				p:setTag("enternal_show",sgs.QVariant()) 
				p:setTag("e_ids",sgs.QVariant())
			end
			--失去标记
			local marks={"@an", "@bian", "@clock", "@kinki", "@qiannian", "@shi","@ye","@yu","@zhengti",
			"@huanyue","@kuangqi","@yingguang","@eyun","@in_jiejie",
			"@tianyi_Weapon","@tianyi_Armor","@tianyi_DefensiveHorse","@tianyi_OffensiveHorse","tianyi_spade","tianyi_heart","tianyi_club","tianyi_diamond",}
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				for _,m in pairs (marks) do
					if p:getMark(m)>0 then
						p:loseAllMarks(m)
					
						--天仪丢弃处理
						if m=="@tianyi_Weapon" then
							id=p:getEquip(0):getEffectiveId()
							room:throwCard(id, p,p)
						end
						if m=="@tianyi_Armor" then
							id=p:getEquip(1):getEffectiveId()
							room:throwCard(id, p,p)
							room:setPlayerMark(p,"Armor_Nullified",0)
						end
						if m=="@tianyi_DefensiveHorse" then
							id=p:getEquip(2):getEffectiveId()
							room:throwCard(id, p,p)
						end
						if m=="@tianyi_OffensiveHorse" then
							id=p:getEquip(3):getEffectiveId()
							room:throwCard(id, p,p)
						end
					end
				end
			end
			
		end
		--归还技能 
		if event==sgs.Death or (event==sgs.EventPhaseChanging)  then
			
			if event==sgs.Death then
				local death = data:toDeath()
				if death.who:objectName() ~= player:objectName() then return false end
			end
			if event==sgs.EventPhaseChanging then
				local change = data:toPhaseChange()
				if change.to ~= sgs.Player_NotActive then return end
				
			end
			for _,p in sgs.qlist(room:getOtherPlayers(player)) do
				room:setPlayerMark(p,"changshi",0)
				--不要忘记filter
				room:filterCards(p, p:getCards("he"), true)
			end
			--倾城mark
			--[[for _, p in sgs.qlist(room:getOtherPlayers(player)) do
				-- for skills
				local lose_skills = p:getTag("ChangshiSkills"):toString():split("+")
				for _, skill_name in ipairs(lose_skills) do
					room:removePlayerMark(p, "Qingcheng"..skill_name)
					--for _, sk in sgs.qlist(sgs.Sanguosha:getRelatedSkills(skill_name)) do
					--	room:removePlayerMark(p, "Qingcheng"..sk:objectName())
					--end
				end
				p:setTag("ChangshiSkills", sgs.QVariant())
			end]]
			--对于永恒 需要在这里trigger phasechange？
			
			--处理禁止系 只好枚举
			for _,p in sgs.qlist(room:getOtherPlayers(player)) do
				if p:hasSkill("zhouye") then
					room:setPlayerCardLimitation(p, "use", "Slash", false)   
				end
				if p:hasSkill("yexing") then
					room:setPlayerCardLimitation(p, "use", "TrickCard+^DelayedTrick", false)
				end
				if p:hasSkill("aoyi") then
					room:setPlayerCardLimitation(p, "use", "TrickCard+^DelayedTrick", false)
				end
			end
		end
	end
}

jinian = sgs.CreateTriggerSkill{
	name = "jinian", 
	events = {sgs.CardsMoveOneTime,sgs.BeforeCardsMove},  
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()--创建房间
		--必须提前做记录  最后一张无中之后的move可能导致无法检测是否空城或者最后的手牌
		if event ==sgs.BeforeCardsMove then
			local move = data:toMoveOneTime()
			--local tag = sgs.QVariant()
			--player:setTag("jinian", tag)
			if move.from and move.from_places:contains(sgs.Player_PlaceHand) 
				then

				if not move.from:hasSkill("jinian") then return false end
				if not player:hasSkill("jinian") then return false end
				if move.from:hasFlag("jinian_used") then return false end
				local h_ids = sgs.IntList()
				for _,id in sgs.qlist(move.card_ids) do
                         if move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceHand then
                             h_ids:append(id)
                          end
                end
				if  move.from:getHandcardNum() <= h_ids:length()  then
					local tag = sgs.QVariant()
					tag:setValue(h_ids)
					player:setTag("jinian", tag)
				end
			end
		end
		if event ==sgs.CardsMoveOneTime then
			local move = data:toMoveOneTime()
			if not player:hasSkill("jinian") then return false end
			
			if player:hasFlag("jinian_used")  then return false end	
			if move.from  
				and move.origin_to_place == sgs.Player_DiscardPile   and move.from:objectName() == player:objectName() then
					--move.to_place==sgs.Player_DiscardPile
					--if not move.from:isKongcheng()  then return false end
					local mo = sgs.CardsMoveStruct()
					ids =  player:getTag("jinian"):toIntList() 
					local h_ids = sgs.IntList()
					for _,id in sgs.qlist(move.card_ids) do
						if ids:contains(id)then
							h_ids:append(id)
						end
						--if move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceHand 
						--or move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceTable
						--then
                             
						--	 h_ids:append(id)
                        -- end
					end
					if h_ids:length()==0 then return false end
					if  room:askForSkillInvoke(player,self:objectName(),data) then 
					
						player:setFlags("jinian_used")
                    
					
						mo.card_ids = h_ids
						mo.to = player
						mo.to_place = sgs.Player_PlaceHand
						room:moveCardsAtomic(mo,true)

						
					end
					local tag = sgs.QVariant()
					player:setTag("jinian", tag)
				end
		end
	end
}
jinian_clear= sgs.CreateTriggerSkill{
	name = "#jinian_clear",
	events = {sgs.EventPhaseChanging},
	can_trigger=function(self,player)
		return player
	end,	
	on_trigger=function(self,event,player,data)
		local change = data:toPhaseChange()
		if change.to == sgs.Player_NotActive then
			local room=player:getRoom()
			local source=room:findPlayerBySkillName(self:objectName())
			if not source then return false end
			if source:hasFlag("jinian_used") then
				source:setFlags("-jinian_used")
			end
		end
	end,
}

fsl007:addSkill(changshi)
 fsl007:addSkill(jinian)
 fsl007:addSkill(jinian_clear)
 extension:insertRelatedSkills("jinian", "#jinian_clear")
 

--【山上的千里眼——犬走椛】 编号：10008 by三国有单	
fsl008 = sgs.General(extension,"fsl008", "fsl",4,false) 
tianyan_card = sgs.CreateSkillCard{
	name = "tianyan",
	target_fixed = true,
	will_throw = false,
	
	on_use = function(self, room, source, targets)
		local target = targets[1]
		local x = room:alivePlayerCount()
		x=math.min(x,4)

		source:drawCards(x)
		local cards=room:askForExchange(source, self:objectName(), x, false, "tianyan_exchange:"..tostring(x))
        local card_ids = sgs.IntList()
		for _,card in sgs.qlist(cards:getSubcards()) do
			card_ids:append(card)
		end		
		--需要先移动再观星
		local move=sgs.CardsMoveStruct()
		move.card_ids=card_ids
		move.from=source
		move.to_place=sgs.Player_DrawPile
		room:moveCardsAtomic(move, false)
		--1225版观星函数变了
		--room:askForGuanxing(source,room:getNCards(x),true) --true为不能控底
		room:askForGuanxing(source,room:getNCards(x),1)
	end,
}
tianyan = sgs.CreateViewAsSkill{
	name = "tianyan",
	n = 0,
	
	enabled_at_play = function(self,player)
		return not player:hasUsed("#tianyan")
	end,

	view_as = function(self, cards)
		local qcard = tianyan_card:clone()
		qcard:setSkillName(self:objectName())
		return qcard
	end
} 

fsl008:addSkill(tianyan)

 
--【丰收与成熟的象征——秋穰子】 编号：10009	
fsl009 = sgs.General(extension,"fsl009", "fsl",4,false) --10009
fengrangCard = sgs.CreateSkillCard{
        name = "fengrang",
        target_fixed = true,
        on_validate = function(self, use)
                local card = sgs.Sanguosha:cloneCard("amazing_grace")
                card:setSkillName("fengrang")
                return card
        end
}
fengrang = sgs.CreateZeroCardViewAsSkill{
        name = "fengrang",
        view_as = function(self)
                return fengrangCard:clone()
        end,
        enabled_at_play = function(self, player)
                local card = sgs.Sanguosha:cloneCard("amazing_grace")
                return card:isAvailable(player) and not player:hasUsed("#fengrang")
        end
}

shouhuo = sgs.CreateTriggerSkill{
        name = "shouhuo",
        events = {sgs.CardUsed, sgs.CardEffected},
        frequency = sgs.Skill_Compulsory,
        priority = 0,
        can_trigger = function(self, target)
                return target and target:isAlive()
        end,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                local thread = room:getThread()
                if event == sgs.CardUsed then
                        local use = data:toCardUse()
                        if not use.card or not use.card:isKindOf("AmazingGrace") then return false end
                        
                        room:setCardFlag(use.card, "shouhuoAG")
                        
                        --preAction
                        local alive = room:getAlivePlayers()
                        local card_count = alive:length()
                        for _,p in sgs.qlist(alive) do
                                if p:hasSkill(self:objectName()) then

										touhou_logmessage("#TriggerSkill",p,"shouhuo")
										room:notifySkillInvoked(p, self:objectName())
										card_count = card_count + 1
                                end
                        end
                        local card_ids = room:getNCards(card_count)
                        room:fillAG(card_ids)
                        local tag = sgs.QVariant()
                        tag:setValue(card_ids)
                        room:setTag("AmazingGrace", tag)
                        
                        --trigger TargetConfirming and TargetConfirmed
                        local targets = use.to
                        if use.from and not use.to:isEmpty() then
                                for _,p in sgs.qlist(use.to) do
                                        if targets:contains(p) then
                                                thread:trigger(sgs.TargetConfirming, room, p, data)
                                                local new_use = data:toCardUse()
                                                targets = new_use.to
                                        end
                                end
                        end
                        use = data:toCardUse()
                        
                        --warning: the action of catching and throwing trigger events is ignored here
                        
                        if use.from and not use.to:isEmpty() then
                                for _,p in sgs.qlist(room:getAlivePlayers()) do
                                        thread:trigger(sgs.TargetConfirmed, room, p, data)
                                end
                        end
                        
                        use.card:use(room, use.from, use.to)

                        room:setTag("SkipGameRule", sgs.QVariant(true))
                        
                elseif event == sgs.CardEffected then
                        local effect = data:toCardEffect()
                        if not effect.card or not effect.card:isKindOf("AmazingGrace") or not effect.card:hasFlag("shouhuoAG")
                                or not player:hasSkill(self:objectName()) then return false end
                        
						touhou_logmessage("#TriggerSkill",player,"shouhuo")
						room:notifySkillInvoked(player, self:objectName())
						for i=1,2 do
                                local aglist = room:getTag("AmazingGrace"):toIntList()
                                local card_id = room:askForAG(effect.to, aglist, false, self:objectName())
                                room:takeAG(effect.to, card_id)
                                aglist:removeOne(card_id)
                                local tag = sgs.QVariant()
                                tag:setValue(aglist)
                                room:setTag("AmazingGrace", tag)
                        end
                        room:setTag("SkipGameRule", sgs.QVariant(true))
                end
                return false
        end
}

fsl009:addSkill(fengrang)
fsl009:addSkill(shouhuo)

		
--【寂寞与终焉的象征——秋静叶】 编号：10010   by三国有单	
fsl010 = sgs.General(extension,"fsl010", "fsl",4,false) 
jiliao_card = sgs.CreateSkillCard{
	name = "jiliao",
	target_fixed = false,
	will_throw = true,
	
	filter = function(self, targets, to_select, player)
		return #targets == 0
	end,
	
	on_use = function(self, room, source, targets)
		local target = targets[1]
		local equips =sgs.IntList()	
		for _,equip in sgs.qlist(target:getEquips()) do
			equips:append(equip:getId())
		end
	
		local move =sgs.CardsMoveStruct()
		move.card_ids=equips
		--move.from_place =sgs.Player_PlaceEquip
		--move.reason.m_reason = ?
		move.to_place =sgs.Player_PlaceHand
		--move.from =target
		move.to =target
		
		--room:moveCards(move,  false)
		room:moveCardsAtomic(move, true)
		
		if target:getHandcardNum() > target:getMaxHp() and source:canDiscard(target, "h") then
			if (not room:askForSkillInvoke(source,"jiliao",sgs.QVariant("throwcard:" .. target:objectName()))) then return false end
			local to_throw = room:askForCardChosen(source, target, "h", self:objectName(),false,sgs.Card_MethodDiscard)
			room:throwCard(to_throw,target, source) 
		end
	end,
}
jiliao = sgs.CreateViewAsSkill{
	name = "jiliao",
	n = 0,
	
	enabled_at_play = function(self,player)
		return not player:hasUsed("#jiliao")
	end,
	
	view_as = function(self, cards)
		local qcard = jiliao_card:clone()
		qcard:setSkillName(self:objectName())
		return qcard
	end
}	
		
zhongyan = sgs.CreateTriggerSkill{
	name = "zhongyan", 
	frequency = sgs.Skill_Limited,
	limit_mark = "@zhongyan",
	events = {sgs.DamageInflicted},  
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		
			local damage = data:toDamage()				
			if damage.from ==nil or damage.from:isDead() or damage.from: objectName() == player: objectName() then return false end --排除自己是伤害来源
			if player:getMark("@zhongyan") > 0 then
				--for ai
				local a_data = sgs.QVariant()
				a_data:setValue(damage)
				room:setTag("zhongyan_damage",a_data)
				n=math.max(1,damage.from:getLostHp())
				local prompt="target:"..damage.from:objectName()..":"..tostring(n)
				if (not room:askForSkillInvoke(player,self:objectName(),sgs.QVariant(prompt))) then 
					return false 
				end
				room:removePlayerMark(player, "@zhongyan")
				local x = damage.from:getMaxHp()- damage.from:getHp()
				room:loseHp(damage.from,math.max(1,x))
				return true
			end
	end
}

fsl010:addSkill(jiliao)		
fsl010:addSkill(zhongyan)

 local function xinyang_shuffle(atable)
	local count = #atable
	for i = 1, count do
		local j = math.random(1, count)
		atable[j], atable[i] = atable[i], atable[j]
	end
	return atable
end 	

--【私欲的巫女——东风谷早苗】 编号：10011
fsl011 = sgs.General(extension,"fsl011", "fsl",3,false)
dfgzm_siyu=sgs.CreateTriggerSkill{
	name="dfgzm_siyu",
	--priority=3,
	frequency = sgs.Skill_Compulsory,
	
	events={sgs.BeforeCardsMove,sgs.Damaged,sgs.EventPhaseStart},
	on_trigger=function(self,event,player,data)
		if event==sgs.BeforeCardsMove then
			
			local move = data:toMoveOneTime()
			
			local room =player:getRoom()
			local source=room:findPlayerBySkillName(self:objectName())
			if player:objectName() ~=source:objectName() then return false end
				if move.from and move.from:objectName() ==source:objectName() then
					if (move.to and move.to:objectName() ~=source:objectName()) or (move.to==nil)then
						
						local ids=move.card_ids
						for _,id in sgs.qlist(ids) do
							if room:getCardPlace(id) == sgs.Player_PlaceSpecial then
								ids:removeOne(id)
							end
						end
						if ids:length()>0 then
							local log=sgs.LogMessage()
							log.from = player
							log.arg = self:objectName()
							log.type = "#TriggerSkill"
							room:sendLog(log)
							room:notifySkillInvoked(source, self:objectName())
						end
						for _,id in sgs.qlist(ids) do
							
							source:addToPile("yuwang_list",id)
							
						end
						ids=sgs.IntList()
						--move.card_ids:removeOne(id)
						move.card_ids=ids
						data:setValue(move)
						--return true
					end
				end
		end
		if event==sgs.Damaged or (event==sgs.EventPhaseStart and player:getPhase() == sgs.Player_Finish)then
			local room=player:getRoom()
			local num=1
			local ids=player:getPile("yuwang_list")
			if ids:length()==0 then return false end
			if event==sgs.Damaged then
				num=data:toDamage().damage
			end
			atable={}
			for _,id in sgs.qlist(ids) do
				table.insert(atable,id)
			end
			xinyang_shuffle(atable)
			if num> #atable then
				num=#atable
			end
			local mo = sgs.CardsMoveStruct()
			mo.to = player
			mo.to_place = sgs.Player_PlaceHand
			for var=1, num, 1 do
				mo.card_ids:append(atable[var])
			end
			
			local log=sgs.LogMessage()
			log.from = player
			log.arg = self:objectName()
			log.type = "#TriggerSkill"
			room:sendLog(log)
			room:notifySkillInvoked(player, self:objectName())
			
			room:moveCardsAtomic(mo,true)
		end
	end,
}
xinyang = sgs.CreateTriggerSkill{
	name = "xinyang", 
	
	events = {sgs.CardsMoveOneTime,sgs.BeforeCardsMove}, 
	
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()--创建房间
		if event ==sgs.CardsMoveOneTime then
			local move = data:toMoveOneTime()
			if player:hasFlag("xinyang_used")  then return false end
			--if move.from and move.from:objectName() == player:objectName() and move.from_places:contains(sgs.Player_PlaceHand) and (move.is_last_handcard or (move.from and move.from:hasFlag("yuwang_last")))then
			if (move.from and move.from:objectName() == player:objectName() and move.from_places:contains(sgs.Player_PlaceHand))
                                and ((not move.to) or (move.to:objectName() ~= player:objectName()) or (move.to_place ~= sgs.Player_PlaceHand))
                                and move.is_last_handcard
                                then
								
				ids=player:getPile("yuwang_list")
				
				if ids:length()>0 then
					atable={}
					for _,id in sgs.qlist(ids) do
						table.insert(atable,id)
					end
					
					xinyang_shuffle(atable)
					if not room:askForSkillInvoke(player,self:objectName(),data) then return false end
					local mo = sgs.CardsMoveStruct()
					mo.card_ids:append(atable[1])
					mo.to = player
					mo.to_place = sgs.Player_PlaceHand
					room:moveCardsAtomic(mo,true)
					if not room:askForSkillInvoke(player,self:objectName(),data) then 
						player:setFlags("xinyang_used")
						return false 
					end
					local mo1 = sgs.CardsMoveStruct()
					mo1.card_ids:append(atable[2])
					mo1.to = player
					mo1.to_place = sgs.Player_PlaceHand
					room:moveCardsAtomic(mo1,true)
					player:setFlags("xinyang_used")
				end
			end
					
		end
	end
}

xinyang_clear= sgs.CreateTriggerSkill{
	name = "#xinyang_clear",
	events = {sgs.EventPhaseEnd},
	can_trigger=function(self,player)
		return player
	end,	
	on_trigger=function(self,event,player,data)
		local room=player:getRoom()
		if player:hasFlag("xinyang_used") then
			player:setFlags("-xinyang_used")
		end
	end,
}

qishu = sgs.CreateTargetModSkill{
        name = "qishu",
        pattern="Slash,TrickCard+^DelayedTrick",
        distance_limit_func = function(self,player,card)
                if player:hasSkill(self:objectName()) and player:getHandcardNum()==1 then --player:isLastHandcard(card)
                        return 1000
                else
                        return 0
                end
        end,
        extra_target_func = function(self,player,card)
                if player:hasSkill(self:objectName()) and (card:isKindOf("Slash") or card:isNDTrick()) and player:getHandcardNum()==1 then--player:isLastHandcard(card) 
                        return 1000
                else
                        return 0
                end
        end
}

fsl011:addSkill(dfgzm_siyu)
fsl011:addSkill(qishu)
--fsl011:addSkill(xinyang_clear)
--fsl011:addSkill(xinyang)
--extension:insertRelatedSkills("xinyang", "#xinyang_clear")
	

