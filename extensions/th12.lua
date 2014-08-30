module("extensions.th12", package.seeall)
extension = sgs.Package("th12")
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
--技能代码   星莲船
------------------------------------------------------------------------------------------------------------
--【被封印的大魔法使——圣白莲】 编号：12001 
xlc001 = sgs.General(extension,"xlc001$","xlc",4,false) --12001
puduCard = sgs.CreateSkillCard{
        name = "pudu",
        target_fixed = false,
        will_throw = true,
        filter = function(self,targets,to_select)
                return (#targets == 0) and to_select:isWounded() and to_select:objectName()~=sgs.Self:objectName()
        end,
        on_effect = function(self,effect)
                local from = effect.from
                local to = effect.to
                local room = from:getRoom()
                local recover = sgs.RecoverStruct()
                room:recover(to,recover)
                room:loseHp(from,1)
        end
}
pudu = sgs.CreateViewAsSkill{
        name = "pudu",
        n = 0,

        view_as = function (self,cards)
                local vs_card = puduCard:clone()
                return vs_card
        end,

        enabled_at_play = function(self, target)
                return not target:hasUsed("#pudu") and hasWoundedGeneral(target)
        end ,
}

hasWoundedGeneral=function(player)
	for _,p in sgs.qlist(player:getAliveSiblings()) do
		if p:isWounded() then return true end
	end
	return false
end
jiushu = sgs.CreateTriggerSkill{
        name = "jiushu",
        frequency = sgs.Skill_Frequent,
        events = {sgs.EventPhaseStart},
        on_trigger = function (self,event,player,data)
                local room = player:getRoom()
                local phase = player:getPhase()

                if phase == sgs.Player_Finish
                then
                        if player:isWounded()
                        then
                                room:askForSkillInvoke(player,"jiushu",data)
                                player:drawCards(player:getLostHp())
								
                        end
						
                end
        end
}

fahua=sgs.CreateTriggerSkill{
	name="fahua$",
	events={sgs.TargetConfirming},
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local use=data:toCardUse()
		if use.card and use.from and  use.card:isKindOf("TrickCard") and use.from:objectName() ~= player:objectName() then
			if use.to:contains(player) and player:hasLordSkill(self:objectName()) then
					local targets=sgs.SPlayerList()
					for _,p in sgs.qlist(room:getOtherPlayers(use.from)) do
						--tmp_players=use.to
						--tmp_players:removeOne(player)
						if p:getKingdom()~="xlc" then
							continue
						end
						if  use.to:contains(p) or use.from:isProhibited(p, use.card) then
						--or not use.card:targetsFeasible(tmp_players, use.from) 
						--or not  use.card:targetFilter(tmp_players, to_select, use.from) then
						--feasible等无法无视距离。。。需要重新定义targetmod、、、
							continue
						end
						
						
						if use.card:isKindOf("DelayedTrick") and p:containsTrick(use.card:objectName()) then
							continue
						end
						if use.card:isKindOf("Collateral") and (not p:getEquip(0))then
							continue
						end
						if (use.card:isKindOf("FireAttack") or use.card:isKindOf("Snatch") or  use.card:isKindOf("Dismantlement"))and p:isKongcheng() then
							continue
						end
						targets:append(p)
					end
					if targets:length()>0 and room:askForSkillInvoke(player,self:objectName(),data) then
						--for ai
						local _data=sgs.QVariant()
						_data:setValue(player)
						room:setTag("fahua_target",_data)
						local a_data=sgs.QVariant()
						a_data:setValue(use)
						room:setTag("fahua_use",a_data)
						--end
						for _,p in sgs.qlist(targets) do
								
								--src dest arg1 arg2 
								prompt= "tricktarget:"..use.from:objectName()..":"..player:objectName()..":"..use.card:objectName()
								if p:askForSkillInvoke("fahua_change",sgs.QVariant(prompt)) then
									--room:removeTag("fahua_use")
								
                                    use.to:append(p)
                                    use.to:removeOne(player)
                                    data:setValue(use)
									
									touhou_logmessage("$CancelTarget",use.from,use.card:objectName(),player)
									touhou_logmessage("#fahua_change",use.from,use.card:objectName(),p)
									--处理延时锦囊的情况
									if use.card:isKindOf("DelayedTrick") then
										local move=sgs.CardsMoveStruct()
										move.card_ids:append(use.card:getId())
										move.to_place=sgs.Player_PlaceDelayedTrick
										move.to=p
										room:moveCardsAtomic(move, true)
									end
									--必须重新触发TargetConfirming 
									room:getThread():trigger(sgs.TargetConfirming, room, p, data);
                                    break
								end
								--room:removeTag("use")
							
						end
					end
			end
		end
	end
}

xlc001:addSkill(pudu)
xlc001:addSkill(jiushu)
xlc001:addSkill(fahua)


--【未确认幻想飞行少女——封兽鵺】 编号：12002
xlc002 = sgs.General(extension,"xlc002", "xlc",3,false) --12002
weizhicard=sgs.CreateSkillCard{
name="weizhi",
target_fixed=true,
will_throw=true,
on_use = function(self, room, source, targets)
        source:drawCards(self:subcardsLength()+1)
end
}
weizhi=sgs.CreateViewAsSkill{
name="weizhi",
n=999,
--filter_pattern = "^TrickCard!" ,
view_filter=function(self,selected,to_select)
        return not to_select:isKindOf("TrickCard") and not sgs.Self:isJilei(to_select)
end,
view_as=function(self,cards)
        if #cards==0 then return nil end
        local card=weizhicard:clone()
        for i=1,#cards,1 do
                card:addSubcard(cards[i])
        end
        return card
end,
enabled_at_play=function(self,player)
        return not player:hasUsed("#weizhi") and not player:isNude()
end
}

weizhuang=sgs.CreateTriggerSkill{
name = "weizhuang",
frequency = sgs.Skill_Compulsory,
events = {sgs.CardEffected, sgs.TargetConfirming},
on_trigger = function(self, event, player, data)
        local room = player:getRoom()
        
        if event == sgs.TargetConfirming then
                local use = data:toCardUse()
                if use.card and use.card:isNDTrick() and use.from:objectName() ~=player:objectName() then
                        room:broadcastSkillInvoke(self:objectName())
                        room:notifySkillInvoked(player, self:objectName())
                        
						touhou_logmessage("#TriggerSkill",player,"weizhuang")
                        local ai_data = sgs.QVariant()
                        ai_data:setValue(player)
						use.from:setTag("weizhuang_target",ai_data)
                        local prompt = "@weizhuang-discard:"..player:objectName()..":"..use.card:objectName()
                        if not room:askForCard(use.from, ".Basic", prompt, data, sgs.Card_MethodDiscard) then
                                room:setCardFlag(use.card,"weizhuang")
								room:setCardFlag(use.card,"weizhuang"..player:objectName())
								return true
                        end
						
                end
        elseif event==sgs.CardEffected then
                local effect=data:toCardEffect()
                if effect.card:hasFlag("weizhuang") and  effect.card:hasFlag("weizhuang"..effect.to:objectName())then
                        --room:setCardFlag(effect.card,"-weizhuang")
                        
						touhou_logmessage("#DanlaoAvoid",effect.to,effect.card:objectName(),nil,self:objectName())
						room:notifySkillInvoked(effect.to, self:objectName())
						room:setEmotion(effect.to, "armor/vine")
                        
						--room:setEmotion(effect.to, "skill/luanwu")
						--room:getThread():delay(3000)
						return true
                end
        end
end
}

xlc002:addSkill(weizhi)
xlc002:addSkill(weizhuang)
	

--【毘沙门天的弟子——寅丸星】 编号：12003 
xlc003 = sgs.General(extension,"xlc003", "xlc",4,false)
jinghua = sgs.CreateTriggerSkill{
        name = "jinghua",
        events = sgs.EventPhaseStart,
		can_trigger = function(self, target)
                return  target
        end,
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local current = room:getCurrent()
				local source = room:findPlayerBySkillName(self:objectName())
                if current  and current:getPhase() == sgs.Player_Start then
                        local targets = room:getAlivePlayers()
                        local xx = sgs.SPlayerList()
                        for _,p in sgs.qlist(targets) do
                                if p:getCards("j"):length() > 0 then
                                        xx:append(p)
                                end
                        end
                        if xx:length() > 0 then
                                
								local target = room:askForPlayerChosen(source,xx,self:objectName(),"@targetchoose",true,true)
                                
								if target then
                                        local card_id = room:askForCardChosen(source,target,"j",self:objectName())
                                        local card_ids = sgs.IntList()
                                        card_ids:append(card_id)
                                        room:moveCardsToEndOfDrawpile(card_ids)
									if current:objectName() ~= source:objectName() then
										room:loseHp(source,1)
									end
									
								end
								
                        end
                end
        end
}

zhengyiEffect = sgs.CreateTriggerSkill{
        name = "#zhengyi",
        frequency = sgs.Skill_Compulsory,
        --priority=-1,
		events = {sgs.TargetConfirming, sgs.SlashEffected, sgs.CardEffected} ,
        on_trigger = function(self,event,player,data)
                
                local room = player:getRoom()
				
				if event == sgs.TargetConfirming then
                        local use = data:toCardUse()
						if use.card:isBlack() and (use.card:isKindOf("Slash") or use.card:isNDTrick()) and use.to:contains(player) then
                                --for ai
								_data=sgs.QVariant()
								_data:setValue(use)
								room:setTag("zhengyi_use",_data)
								--end
                                if room:askForSkillInvoke(player,"zhengyi",sgs.QVariant("drawcard:"..use.from:objectName()..":"..use.card:objectName())) then
                                        player:drawCards(1)
                                else
                                        --room:setCardFlag(use.card,"zhengyi")
										room:setCardFlag(use.card,"zhengyi"..player:objectName())
                                end
                        end
                elseif event == sgs.SlashEffected then
                        local effect = data:toSlashEffect()
                        if effect.slash and effect.slash:hasFlag("zhengyi"..effect.to:objectName()) then
                               --and effect.slash:hasFlag("zhengyi") 
							   -- effect.slash:clearFlags()
                                --不能随意clear 此卡还没有结算完毕
								touhou_logmessage("#DanlaoAvoid",effect.to,effect.slash:objectName(),nil,"zhengyi")
								room:notifySkillInvoked(effect.to, "zhengyi")
								
								return true
                        end
                elseif event == sgs.CardEffected then
                        local effect = data:toCardEffect()
                        if effect.card and effect.card:isNDTrick() and  effect.card:hasFlag("zhengyi"..effect.to:objectName()) then
								touhou_logmessage("#DanlaoAvoid",effect.to,effect.card:objectName(),nil,"zhengyi")
								room:notifySkillInvoked(effect.to, "zhengyi")
                                return true
                        end
                end
        end

}

zhengyiArmor = sgs.CreateTriggerSkill{
        name = "#zhengyiArmor",
        events = {sgs.CardsMoveOneTime,sgs.EventAcquireSkill},--sgs.BeforeCardsMove,
        frequency = sgs.Skill_Compulsory,
        on_trigger = function(self,event,player,data)
                if event == sgs.CardsMoveOneTime then
                        local move = data:toMoveOneTime()
                        local room = player:getRoom()
                        local movearmor = sgs.CardsMoveStruct()
						local t_ids= sgs.IntList()
						if move.to and move.to:objectName() == player:objectName() and move.to_place == sgs.Player_PlaceEquip then
                                for _, id in sgs.qlist(move.card_ids) do
                                        if (sgs.Sanguosha:getCard(id):isKindOf("Armor")) then
                                                --movearmor.card_ids:append(id)
												t_ids:append(id)
                                        end
                                end
								if t_ids:length()>0 then
									touhou_logmessage("#ZhengyiUninstall",player,"zhengyi")
								
									--movearmor.to_place = sgs.Player_DiscardPile
									for _,id in sgs.qlist(t_ids) do
										room:throwCard(id,player,player)
									end
									--room:moveCardsAtomic(movearmor,true)
								end
								
                        end
                else
                    if data:toString() ~="zhengyi" then return false end
						if player:hasSkill(self:objectName()) then
                                local room = player:getRoom()
                                local armor = player:getCards("e")
								local armor1=sgs.IntList()
                                for _,card in sgs.qlist(armor) do
                                        if not card:isKindOf("Armor") then
                                                armor:removeOne(card)
										else
											armor1:append(card:getId())
                                        end
                                end
								touhou_logmessage("#ZhengyiUninstall",player,"zhengyi")
                                local move = sgs.CardsMoveStruct()
								
								
                                move.card_ids = armor1
                                move.to_place = sgs.Player_DiscardPile
                                room:moveCardsAtomic(move,true)
                        end
                end
        end
}

zhengyi = sgs.CreateFilterSkill{--必须做主技能 或者要filtercard??
        name = "zhengyi",
        view_filter = function(self, to_select)
                local room = sgs.Sanguosha:currentRoom()
                local place = room:getCardPlace(to_select:getEffectiveId())
                if place == sgs.Player_PlaceHand then
                        return to_select:isKindOf("Armor")
                end
                return false
        end,

        view_as = function (self,card)
                local id = card:getId()
                local suit = card:getSuit()
                local point = card:getNumber()
                local nul = sgs.Sanguosha:cloneCard("nullification",suit,point)
                nul:setSkillName("zhengyi")
                local vsc = sgs.Sanguosha:getWrappedCard(id)
                vsc:takeOver(nul)
                return vsc
        end
}

xlc003:addSkill(jinghua)
xlc003:addSkill(zhengyiEffect)
xlc003:addSkill(zhengyiArmor)
xlc003:addSkill(zhengyi)
extension:insertRelatedSkills("zhengyi", "#zhengyiArmor")
extension:insertRelatedSkills("zhengyi", "#zhengyi")


--【水难事故的念缚灵——村纱水蜜】 编号：12004 by三国有单
xlc004 = sgs.General(extension,"xlc004", "xlc",4,false) --12004
chuannan=sgs.CreateTriggerSkill{--技能"船难"
	name="chuannan",
	events={sgs.Damaged,sgs.Damage},
	on_trigger=function(self,event,player,data)
		local room=player:getRoom()
		local damage = data:toDamage()
		local target
		if event==sgs.Damaged then
			if damage.from ~=nil  and damage.from:isAlive() and player:objectName() ~=damage.from:objectName() then target = damage.from end	
		elseif event==sgs.Damage then
			if damage.to:isAlive() and player:objectName() ~=damage.to:objectName() then target =damage.to end
		end
		if not target  then return false end 
		local supply=sgs.Sanguosha:cloneCard("supply_shortage",sgs.Card_NoSuit,0)
		local canuse=true
		if room:isProhibited(player, target, supply) then
			--room 全局 和 client的差异？？
			--if target:isProhibited(player, supply) then
			canuse=false
        end
        for _,card in sgs.qlist(target:getCards("j")) do
            if card:isKindOf("SupplyShortage") then
                canuse=false
                break
            end
        end
		if canuse then 
				if event==sgs.Damaged then
					_data=sgs.QVariant()
					_data:setValue(target)
					if room:askForSkillInvoke(player,self:objectName(),_data) then
						player:drawCards(1)
						room:setPlayerMark(player,"chuannan_mustuse",1)
					end
				end
				if player:getHandcardNum() >0 then
					local supply_card
					if player:getMark("chuannan_mustuse")>0 then
						room:setPlayerMark(player,"chuannan_mustuse",0)
						supply_card=room:askForCard(player,".!","@chuannan:"..target:objectName(),data,sgs.Card_MethodNone)
					else
						supply_card=room:askForCard(player,".|.|.|hand","@chuannan:"..target:objectName(),data,sgs.Card_MethodNone,nil,false,self:objectName(),false)
					end
					if supply_card then
						local card=sgs.Sanguosha:cloneCard("supply_shortage",supply_card:getSuit(),supply_card:getNumber())
					
						card:addSubcard(supply_card:getId())
						card:setSkillName("_chuannan")
						local carduse=sgs.CardUseStruct()
						carduse.card=card
						carduse.from=player
						carduse.to:append(target)
						room:useCard(carduse)
					end
				end
		end	

	end,
}
xlc004:addSkill(chuannan)


--【守护与被守护的大轮——云居一轮】 编号：12005
xlc005 = sgs.General(extension,"xlc005", "xlc",4,false) --12005
lizhi = sgs.CreateTriggerSkill{
        name = "lizhi" ,
        events = {sgs.DamageCaused} ,
        on_trigger = function(self, event, player, data)
                local room= player:getRoom()
				--for AI
				damage=data:toDamage()
				_data=sgs.QVariant()
				_data:setValue(damage.to)
				a_data=sgs.QVariant()
				a_data:setValue(damage)
				player:setTag("lizhi_damage",a_data)
				if (player:askForSkillInvoke(self:objectName(), _data)) then
						player:drawCards(2)
						return true
                end
                return false
        end ,
}

yunshang = sgs.CreateTriggerSkill{
        name = "yunshang" ,
        events = {sgs.TargetConfirmed, sgs.CardEffected} ,
        frequency = sgs.Skill_Compulsory ,
        on_trigger = function(self, event, player, data)
               local room=player:getRoom()
			   if event == sgs.TargetConfirmed then
                        local use = data:toCardUse()
                        if use.card:isNDTrick() and use.to:contains(player) and (not use.from:inMyAttackRange(player)) then
                                local room = player:getRoom()
                                room:setPlayerFlag(player, "yunshang")
                                room:setCardFlag(use.card, "yunshang")
								room:notifySkillInvoked(player, self:objectName())
                        end
                elseif event == sgs.CardEffected then
                        local effect = data:toCardEffect()
                        if effect.card:isNDTrick() and effect.card:hasFlag("yunshang") and player:hasFlag("yunshang") then
                                player:getRoom():setPlayerFlag(player, "-yunshang")
                                touhou_logmessage("#DanlaoAvoid",effect.to,effect.card:objectName(),nil,self:objectName())
								
								
								return true
                        end
                end
                return false
        end ,
}

xlc005:addSkill(lizhi)
xlc005:addSkill(yunshang)
	
	
--【探宝的小小大将——娜兹玲】 编号：12006 
xlc006 = sgs.General(extension,"xlc006", "xlc",3,false) --12006
souji = sgs.CreateTriggerSkill{
        name = "souji",
        events = sgs.CardsMoveOneTime,
		frequency = sgs.Skill_Frequent,
        on_trigger = function(self,event,player,data)
            local move = data:toMoveOneTime()
            local room = player:getRoom()
            local current = room:getCurrent()
            if current and current:objectName() == player:objectName() then
                if move.from and move.from:objectName() ~= player:objectName() then
                    if move.to_place == sgs.Player_DiscardPile then
                        local obtain_ids=sgs.IntList()

						for _,id in sgs.qlist(move.card_ids) do
							if move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceHand 
							or move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceEquip
							or move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceTable
							then
								if not sgs.Sanguosha:getCard(id):objectName():startsWith("renou") then
									obtain_ids:append(id)
								end
							end
						end
						
						if obtain_ids:length()>0 and room:askForSkillInvoke(player,self:objectName(),data) then
                            local mo = sgs.CardsMoveStruct()
                            mo.card_ids =obtain_ids
                            mo.to = player
                            mo.to_place = sgs.Player_PlaceHand
                            room:moveCardsAtomic(mo,true)
							
						end
										
                    end
                end
            end
        end
}

tansuo = sgs.CreateTriggerSkill{
        name = "tansuo",
        frequency = sgs.Skill_Frequent,
		events = {sgs.CardsMoveOneTime, sgs.EventPhaseEnd},
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if player:getPhase() == sgs.Player_Discard then
                        if event == sgs.CardsMoveOneTime then
                                local move = data:toMoveOneTime()
                                local source = move.from
                                if source and source:objectName() == player:objectName() then
                                        if move.to_place == sgs.Player_DiscardPile then
                                                local count = player:getMark("tansuo")
                                                count = count + move.card_ids:length()
                                                player:setMark("tansuo", count)
                                        end
                                end
                        else
                                if player:getMark("tansuo") >= player:getHp() then
                                        if player:askForSkillInvoke(self:objectName()) then
                                                local move = sgs.CardsMoveStruct()
                                                move.to = player
                                                move.to_place = sgs.Player_PlaceHand
                                                --for i=1,2,1 do
                                                        local id = room:getDrawPile():last()
                                                        len =room:getDrawPile():length()
														local id1 = room:getDrawPile():at(len-2)
														move.card_ids:append(id)
														move.card_ids:append(id1)
                                                --end
												room:moveCardsAtomic(move,false)
											
                                        end
										
                                end
                                player:setMark("tansuo", 0)
                        end
                end
        end
}

xlc006:addSkill(souji)
xlc006:addSkill(tansuo)
	
	
--【愉快的遗忘之伞——多多良小伞】 编号：12007
--[源码改动]GameRule.cpp GameRule::onPhaseProceed 需要再次验证延时锦囊。。。有可能判定阶段中移除
xlc007 = sgs.General(extension,"xlc007", "xlc",3,false) --12007
ddlxsyiwang = sgs.CreateTriggerSkill{
        name = "ddlxsyiwang" ,
        events = {sgs.CardsMoveOneTime} ,
        on_trigger = function(self, event, player, data)
                local move = data:toMoveOneTime()
                if move.from and (move.from:objectName() == player:objectName()) and move.from_places:contains(sgs.Player_PlaceEquip) and (move.to_place ~= sgs.Player_PlaceEquip) then
                        local room = player:getRoom()
                        for _, place in sgs.qlist(move.from_places) do
                                if place == sgs.Player_PlaceEquip then
                                        local wounded = sgs.SPlayerList()
                                        for _, others in sgs.qlist(room:getAllPlayers()) do
                                                if others:isWounded() then wounded:append(others) end
                                        end
                                        if  wounded:isEmpty() then return false end
                                       
										local recovertarget = room:askForPlayerChosen(player, wounded, self:objectName(), "@ddlxsyiwang-recover",true,true)
                                                if recovertarget then 
													local recover2 = sgs.RecoverStruct()
													recover2.who = player
													room:recover(recovertarget, recover2)
													if recovertarget:objectName()~=player:objectName() then
														player:drawCards(1)
													end
													
												end
									
                                end
                        end
                end
                return false
        end ,
}

ddlxsjingxia = sgs.CreateMasochismSkill{
        name = "ddlxsjingxia" ,
        on_damaged = function(self, player, damage)
            for var=1,damage.damage,1 do
				local choices = {}
                if damage.from and player:canDiscard(damage.from, "he") then table.insert(choices, "discard") end
                local room = player:getRoom()
                local fieldcard = sgs.SPlayerList()
                for _, p in sgs.qlist(room:getAllPlayers()) do
                        if player:canDiscard(p, "ej") then fieldcard:append(p) end
                end
                if not fieldcard:isEmpty() then table.insert(choices, "discardfield") end
                if #choices == 0 then return end
                table.insert(choices, "dismiss")
                local _data = sgs.QVariant()
                _data:setValue(damage)
				--ai
				player:setTag("jingxia",_data)
                local choice = room:askForChoice(player, self:objectName(), table.concat(choices, "+"), _data)
                player:removeTag("jingxia")
				if choice == "dismiss" then return end

				touhou_logmessage("#InvokeSkill",player,self:objectName())
				room:notifySkillInvoked(player, self:objectName())
				if choice == "discard" then
						for i = 1, 2, 1 do
                                if not player:canDiscard(damage.from, "he") then return end
                                local card_id = room:askForCardChosen(player, damage.from, "he", self:objectName() .. "-discard", false, sgs.Card_MethodDiscard)
                                room:throwCard(card_id, damage.from, player)
                        end
                else
						local aidelay = sgs.GetConfig("AIDelay", 0)
                        sgs.SetConfig("AIDelay", 0)
                        ---------------AIDELAY == 0-------------------
                        fieldcard = sgs.SPlayerList()
                        for _, p in sgs.qlist(room:getAllPlayers()) do
                                if player:canDiscard(p, "ej") then fieldcard:append(p) end
                        end
						if fieldcard:length()==0 then return false end
                        local player1 = room:askForPlayerChosen(player, fieldcard, self:objectName(), "@ddlxsjingxia-discardfield")
                        local card1 = room:askForCardChosen(player, player1, "ej", self:objectName() .. "-discardfield", false, sgs.Card_MethodDiscard)
                        sgs.SetConfig("AIDelay", aidelay)
                        ----------------------------------------------
                        room:throwCard(card1, player1, player)
                        ---------------AIDELAY == 0-------------------
                        sgs.SetConfig("AIDelay", 0)
                        fieldcard = sgs.SPlayerList()
                        for _, p in sgs.qlist(room:getAllPlayers()) do
                                if player:canDiscard(p, "ej") then fieldcard:append(p) end
                        end
						if fieldcard:length()==0 then return false end
                        local player2 = room:askForPlayerChosen(player, fieldcard, self:objectName(), "@ddlxsjingxia-discardfield2", true)
                        if player2 then
                                local card2 = room:askForCardChosen(player, player2, "ej", self:objectName() .. "-discardfield", false, sgs.Card_MethodDiscard)
                                sgs.SetConfig("AIDelay", aidelay)
                                room:throwCard(card2, player2, player)
                        end
                        ----------------------------------------------
                        sgs.SetConfig("AIDelay", aidelay)
                end
				
			end
			
        end ,
}

xlc007:addSkill(ddlxsyiwang)
xlc007:addSkill(ddlxsjingxia)

--【脱离时代的顽固老爹——云山】编号：12008 by三国有单
xlc008 = sgs.General(extension,"xlc008", "xlc",4,true) --12008
bianhuan = sgs.CreateTriggerSkill{
	name = "bianhuan", 
	events = {sgs.DamageInflicted},  
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()--创建房间
		if (not room:askForSkillInvoke(player,self:objectName(),data)) then return false end
		room:loseMaxHp(player,1)
		return true
	end
}
	
nuhuoCard = sgs.CreateSkillCard{
        name = "nuhuo",
        target_fixed = false,
        will_throw = true,

        filter = function(self,selected,to_select)
                return #selected < 1 and sgs.Self:objectName()~=to_select:objectName() 
        end,

        on_use = function(self, room, source, targets)
			target=targets[1]
            local damage = sgs.DamageStruct()
            damage.from = target
            damage.to = source
			damage.reason="nuhuo"
            room:damage(damage)
			all=sgs.SPlayerList()
			local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
			for _,p in sgs.qlist(room:getOtherPlayers(source)) do
				if source:canSlash(p,slash,true) then
					all:append(p)
				end
			end
			if not all:isEmpty() then
				local victim = room:askForPlayerChosen(target,all,self:objectName(),"@nuhuo:"..source:objectName(),false)
				local carduse=sgs.CardUseStruct()
				carduse.card=slash
				carduse.card:setSkillName("_"..self:objectName())
				carduse.from=source
				carduse.to:append(victim)
				room:useCard(carduse,false)
			end
		end
}

nuhuo = sgs.CreateViewAsSkill{
        name = "nuhuo",
        n = 0,
        view_as = function (self,cards)
                return nuhuoCard:clone()
        end,

        enabled_at_play = function(self, player)
                return not player:hasUsed("#nuhuo")
        end


}

xlc008:addSkill(bianhuan)
xlc008:addSkill(nuhuo)



