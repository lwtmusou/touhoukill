module("extensions.th14", package.seeall)
extension = sgs.Package("th14")
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
--技能代码   辉针城
------------------------------------------------------------------------------------------------------------
--【小人的末裔——少名针妙丸】 编号：14001 
hzc001 = sgs.General(extension,"hzc001$", "hzc",3,false)
baochui = sgs.CreateTriggerSkill{
        name = "baochui",
        events = sgs.EventPhaseStart,
        frequency = sgs.Skill_Wake,
        can_trigger = function(self, target)
                return target and target:isAlive() and target:hasSkill(self:objectName())
                          and target:getEquips():length()>1 and target:getPhase() == sgs.Player_Start  
						  and target:getMark(self:objectName()) == 0
        end,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
				room:addPlayerMark(player, self:objectName())
				
				room:notifySkillInvoked(player, self:objectName())
				if room:changeMaxHpForAwakenSkill(player,1) then
					local recov = sgs.RecoverStruct()
					recov.card = nil
					recov.who = player
					room:recover(player, recov)		
					room:handleAcquireDetachSkills(player,"jinji")
				
				end
        end
}
--[[baochui=sgs.CreateTriggerSkill{
	name="baochui",
	events={sgs.TurnStart,sgs.EventPhaseStart},

	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		if event== sgs.TurnStart then
			local target =player:getTag("yaoqi_target"):toPlayer()
			if target then
				room:handleAcquireDetachSkills(target, "-yaoqi")
				player:setTag("yaoqi_target",sgs.QVariant())
			end
		end
		if event==sgs.EventPhaseStart  and player:getPhase()== sgs.Player_Start  then
			if not player:askForSkillInvoke(self:objectName(),data) then return false end
			player:drawCards(1)
			local target
			for _,p in sgs.qlist(room:getOtherPlayers(player)) do
				local choice=room:askForChoice(p,self:objectName(),"addyaoqi+cancel")
                if choice =="addyaoqi" then
					target=p
					break
				end
			end
			if not target then
				target = room:askForPlayerChosen(player,room:getOtherPlayers(player),self:objectName(),"@baochui",false,true)      
				
			end
			room:handleAcquireDetachSkills(target, "yaoqi")
			local _data = sgs.QVariant()
            _data:setValue(target)
			player:setTag("yaoqi_target",_data)
		end
	end
}]]
yaoqi=sgs.CreateTriggerSkill{
	name="yaoqi",
	frequency = sgs.Skill_Compulsory,
	events={sgs.DrawNCards,sgs.EventPhaseEnd},
	on_trigger = function(self, event, player, data)
		local room=player:getRoom()
		if event==sgs.DrawNCards then
			local num =data:toInt()
			data:setValue(num+1)
		end
		if event==sgs.EventPhaseEnd  and player:getPhase()== sgs.Player_Play  then
			local source=room:findPlayerBySkillName("baochui")
			if not source then return false end
			choices={}
			if player:getEquips():length()>0 then
				table.insert(choices,"equip")
			end
			if not player:isKongcheng() then
				table.insert(choices,"handcard")
			end
			if #choices == 0 then return false end
			local choice=room:askForChoice(source,self:objectName(),table.concat(choices,"+"))
			if choice == "equip" then
				local id = room:askForCardChosen(source, player, "e", self:objectName())
				room:obtainCard(source,id,room:getCardPlace(id) ~= sgs.Player_PlaceHand)
			end
			if choice =="handcard" then
				local dummy = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
                local card_ids = {}
                local original_places = {}
                local count = 0
                player:setFlags("yaoqi_InTempMoving")
                for i = 1, 2, 1 do
                    if  player:isKongcheng() then break end
                    local id = room:askForCardChosen(source, player, "he", self:objectName())
                    table.insert(card_ids, id)
                    local place = room:getCardPlace(id)
                    table.insert(original_places, place)
                    dummy:addSubcard(id)
                    player:addToPile("#yaoqi", dummy, false)--先行把牌拿走？？
                    count = count + 1
                end
                for i = 1, count, 1 do
                    local card = sgs.Sanguosha:getCard(card_ids[i])
                    room:moveCardTo(card, player, original_places[i], false)
                end
                player:setFlags("-yaoqi_InTempMoving")
                if count > 0 then
                    room:moveCardTo(dummy, source, sgs.Player_PlaceHand, false)
                end
			end
			--if room:getCardPlace(id)==sgs.Player_DiscardEquip then

		end
	end
}
yaoqi_distance = sgs.CreateDistanceSkill{
    name = "#yaoqi_distance",
    correct_func = function (self,from,to)
        if from:hasSkill("yaoqi") then
            return -1
		end
    end
}
yaoqi_mod = sgs.CreateTargetModSkill{
        name = "#yaoqi_mod",
        pattern="Slash",
        residue_func = function(self,player,card)
			if player:hasSkill("yaoqi") and card:isKindOf("Slash") then
				return 1		
            else
                return 0
            end
        end,
}

jinjivs= sgs.CreateViewAsSkill{
	name = "jinji" ,
	n = 1,
	
	view_filter = function(self, selected, to_select)
        return  to_select:isKindOf("EquipCard")
	end,
	view_as = function(self, cards)
		if  sgs.Self:getMark("jinjimark")<=0 then  return jinjicard:clone() end
		if(#cards ~= 1) then return false end
		local card,cardname
		local id=sgs.Self:getMark("jinjimark")-1
		if id~=-1 then
			cardname=sgs.Sanguosha:getCard(id):objectName()
		end
		if cardname then
			card=sgs.Sanguosha:cloneCard(cardname, cards[1]:getSuit(), cards[1]:getNumber())
		end
		card:addSubcard(cards[1])
		card:setSkillName(self:objectName())
		return card
	end,
	
	enabled_at_play = function(self,player)
		return  player:getMark("@jinji")==0
	end,
	
	enabled_at_response = function(self, player, pattern)
		return ( pattern == "@@jinji")
	end,
}
jinjicard = sgs.CreateSkillCard{--奇迹选牌（出牌阶段） 
	name = "jinji",
	target_fixed = true,
	will_throw = false,	
	
	on_use = function(self, room, source, targets)
		all_card={"cancel"}
		--非延时锦囊
		tricks={"amazing_grace","god_salvation","savage_assault","archery_attack",
		"duel","ex_nihilo","snatch","dismantlement","collateral","iron_chain","fire_attack"}
		for _,pattern in pairs(tricks) do
			if not source:isCardLimited(sgs.Sanguosha:cloneCard(pattern), sgs.Card_MethodUse, true) then
				table.insert(all_card,pattern)
			end
		end	
		--再次调用viewas
		local choice = room:askForChoice(source,self:objectName(),table.concat(all_card,"+"))
		if choice=="cancel" then return false end
		local card
		for id=0,159,1 do
			 card=sgs.Sanguosha:getCard(id)
			if card:objectName()==choice then
				room:setPlayerMark(source,"jinjimark",id+1)
				break
			end
		end
		room:askForUseCard(source,"@@jinji","@jinji_target:"..card:objectName()) 
	end,
}
jinji= sgs.CreateTriggerSkill{
	name = "jinji",
	events = {sgs.EventPhaseChanging,sgs.PreCardUsed,},
	view_as_skill = jinjivs,
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		if event==sgs.PreCardUsed then
			local use = data:toCardUse()
			if use.card:getSkillName()=="jinji" then
				player:gainMark("@jinji",1)
				room:setPlayerMark(player,"jinjimark",0)
			end
		end
		if event==sgs.EventPhaseChanging then
			if player:getMark("@jinji")>0 then
				room:setPlayerMark(player, "@jinji", 0)
			end
		end
	end
}
--[源码改动]修改翼包 sp吕蒙 技能【偷袭】fixed_func
yicun_fix = sgs.CreateAttackRangeSkill{
    name = "#yicun",
    fixed_func = function(self,from)
        if from:hasSkill("yicun") then
            return math.max(from:getEquips():length(),1)
        end
    end
}
yicun = sgs.CreateTriggerSkill{ 
	name = "yicun",
	events = {sgs.EventPhaseStart},
	on_trigger = function(self,event,player,data)
		if player:getPhase() ~=sgs.Player_Play then return false end
		local room=player:getRoom()
		local discardpile = room:getDiscardPile()
        local t=false
		for _, id in sgs.qlist(discardpile) do
            if sgs.Sanguosha:getCard(id):isKindOf("EquipCard") then
                t=true
				break
            end
        end
		if not t then return false end
		if not player:askForSkillInvoke("yicun",data) then return false end
						
		local n=player:getEquips():length()
		local willdraw=false
		if n>0 then
			willdraw=room:askForDiscard(player,self:objectName(),n,n,true,false,"yicun_discard")
        end
		if willdraw or n==0 then
			local able = sgs.IntList()
                for _, id in sgs.qlist(discardpile) do
                        tmp_card=sgs.Sanguosha:getCard(id)
						if tmp_card:isKindOf("EquipCard") and not tmp_card:objectName():startsWith("renou") then
                                able:append(id)
                        end
                end
                if able:isEmpty() then return end
                room:fillAG(able, player)
                local equipid = room:askForAG(player, able, false, self:objectName())
                room:clearAG(player)
				room:obtainCard(player,equipid,true)
		end				
	end
}

--[[moyi = sgs.CreateMaxCardsSkill{
    name = "moyi",
    extra_func = function (self,target)
        if target:hasSkill(self:objectName()) then
           return target:getEquips():length()
		else
			return 0
        end
    end
}]]

moyiCard = sgs.CreateSkillCard{
    name = "moyiCard",
    target_fixed = false,
    will_throw = false,
    filter = function(self, targets, to_select)
        if #targets == 0 then
            if to_select:hasLordSkill("moyi") then
                if to_select:objectName() ~= sgs.Self:objectName() then
                     return not to_select:hasFlag("moyiInvoked")
                end
             end
        end
         return false
    end,
                on_use = function(self, room, source, targets)
                                local fsl001 = targets[1]
                                if fsl001:hasLordSkill("moyi") then
                                                room:setPlayerFlag(fsl001, "moyiInvoked")
                                                fsl001:obtainCard(self)
                                                local subcards = self:getSubcards()
                                                for _,card_id in sgs.qlist(subcards) do
                                                    room:setCardFlag(card_id, "visible")
                                                end
                                                room:setEmotion(fsl001, "good")
                                                local fsl001s = sgs.SPlayerList()
                                                local players = room:getOtherPlayers(source)
                                                for _,p in sgs.qlist(players) do
                                                                if p:hasLordSkill("moyi") then
                                                                                if not p:hasFlag("moyiInvoked") then
                                                                                                fsl001s:append(p)
                                                                                end
                                                                end
                                                end
                                                if fsl001s:length() == 0 then
                                                                room:setPlayerFlag(source, "Forbidmoyi")
                                                end
                                end
                end
}
moyiVS = sgs.CreateViewAsSkill{
    name = "moyiVS",
    n = 1,
    view_filter = function(self, selected, to_select)
        return to_select:isKindOf("EquipCard")
    end,
    view_as = function(self, cards)
        if #cards == 1 then
            local card = moyiCard:clone()
            card:addSubcard(cards[1])
            return card
        end
    end,
    enabled_at_play = function(self, player)
        if player:getKingdom() == "hzc" then
            return not player:hasFlag("Forbidmoyi")
        end
        return false
    end
}
moyi=sgs.CreateTriggerSkill{
	name="moyi$",
	events = {sgs.GameStart, sgs.EventPhaseChanging,sgs.EventAcquireSkill,sgs.EventLoseSkill},
	on_trigger=function(self,event,player,data)
		if not player then return false end
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
				if not p:hasSkill("moyiVS") then
					room:attachSkillToPlayer(p, "moyiVS",true)													
				end
			end                                   
		end
		if (event == sgs.EventLoseSkill and data:toString() == "moyi") then
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
				if  p:hasSkill("moyiVS") then
					room:detachSkillFromPlayer(p, "moyiVS",true)													
				end
			end
		end
		if event == sgs.EventPhaseChanging then
            local phase_change = data:toPhaseChange()
            if phase_change.from == sgs.Player_Play then
                if player:hasFlag("Forbidmoyi") then
                        room:setPlayerFlag(player, "-Forbidmoyi")
                end
                local players = room:getOtherPlayers(player)
                for _,p in sgs.qlist(players) do
                    if p:hasFlag("moyiInvoked") then
						room:setPlayerFlag(p, "-moyiInvoked")
                    end
               end
            end
        end
	end,
	can_trigger = function(self, target)
        return true
    end,
}

if not sgs.Sanguosha:getSkill("yaoqi") then
    local skillList=sgs.SkillList()
    skillList:append(yaoqi)
    sgs.Sanguosha:addSkills(skillList)
end
if not sgs.Sanguosha:getSkill("#yaoqi_distance") then
    local skillList=sgs.SkillList()
    skillList:append(yaoqi_distance)
    sgs.Sanguosha:addSkills(skillList)
end
if not sgs.Sanguosha:getSkill("#yaoqi_mod") then
    local skillList=sgs.SkillList()
    skillList:append(yaoqi_mod)
    sgs.Sanguosha:addSkills(skillList)
end
if not sgs.Sanguosha:getSkill("moyiVS") then
        local skillList=sgs.SkillList()
        skillList:append(moyiVS)
        sgs.Sanguosha:addSkills(skillList)
end
if not sgs.Sanguosha:getSkill("jinji") then
        local skillList=sgs.SkillList()
        skillList:append(jinji)
        sgs.Sanguosha:addSkills(skillList)
end
--yaoqifakemove = sgs.CreateFakeMoveSkill("yaoqi")
hzc001:addSkill(baochui)
hzc001:addSkill(yicun_fix)
hzc001:addSkill(yicun)
hzc001:addSkill(moyi)
extension:insertRelatedSkills("yicun", "#yicun_fix")
extension:insertRelatedSkills("yaoqi", "#yaoqi_distance")
extension:insertRelatedSkills("yaoqi", "#yaoqi_mod")
--extension:insertRelatedSkills("yaoqi", "#yaoqi-fake-move")
sgs.LoadTranslationTable{["hzc001"] = "少名针妙丸",

  ["#hzc001"] = "小人的末裔",
  ["jinji"] = "进击",
  [":jinji"] = "你可以将一张装备当作任意非延时锦囊使用。每阶段限一次。",
 ["~jinji"] = "选择装备牌和目标后，确定",
 ["@jinji_target"]="是否发动“进击”，使用【%src】",
 
 ["baochui"] = "宝槌",
 --[":baochui"] = "回合开始阶段开始时，你可以摸一张牌并令所有其他角色依次作出选择：是否愿意获得技能“妖器”，直到你的下一个回合开始。（若无人响应，则由你指定一名角色）（妖器，锁定技，摸牌阶段你额外摸一张牌，每回合你可以额外使用一张杀，你和其他角色计算距离时始终-1。你的出牌阶段结束时，“宝槌”技能持有者须选择一项：获得你一张装备区的牌或者获得你两张手牌。",
 [":baochui"] = "觉醒技。回合开始阶段开始时，若你的装备区有不少于2张牌，你须增加一点体力上限，回复一点体力。然后获得技能【进击】。（进击：你可以将一张装备当作任意非延时锦囊使用。每阶段限一次。）",
 
 ["@baochui"] ="由于没有人愿意响应“宝槌”，你需要指定一名角色",
 ["baochui:addyaoqi"]="获得技能“妖器”",
 ["yaoqi"]="妖器",
 [":yaoqi"]="<font color=\"blue\"><b>锁定技，</b></font>摸牌阶段你额外摸一张牌，每回合你可以额外使用一张杀，你和其他角色计算距离时始终-1。你的出牌阶段结束时，“宝槌”技能持有者须选择一项：获得你一张装备区的牌或者获得你两张手牌。",
 ["yaoqi:equip"]="获得装备",
 ["yaoqi:handcard"]="获得手牌",
 --["moyi"] = "末裔",
 -- [":moyi"] = "<font color=\"blue\"><b>锁定技，</b></font>你的手牌上限+x，x为你的装备数量。",
 ["yicun"] = "一寸",
  [":yicun"] = "出牌阶段开始时，你可以弃置x张手牌（x为装备区卡牌数量），从弃牌堆中获取一张装备牌。你的攻击范围始终为y（y为你的装备区的卡牌数，且最小为1。）",
 
 
 ["moyi"] = "末裔",
 [":moyi"] ="<font color=\"orange\"><b>主公技，</b></font>其他辉势力角色的出牌阶段，该角色可以交给你一张装备牌。<font color=\"green\"><b>每阶段限一次</b></font>",
 ["moyiVS"] = "末裔送牌",
 [":moyiVS"] = "出牌阶段，你可以交给主公技“末裔”持有者一张装备牌。<font color=\"green\"><b>每阶段限一次</b></font>",
 }	

 
--【梦幻的打击乐手——堀川雷鼓】 编号：14002
hzc002 = sgs.General(extension,"hzc002", "hzc",4,false) 
jiepai=sgs.CreateTriggerSkill{
	name="jiepai",
	events={sgs.SlashMissed,sgs.SlashHit},

	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local effect = data:toSlashEffect()
		if event==sgs.SlashHit then
			player:drawCards(1)
		end
		if event==sgs.SlashMissed then
			if effect.to and effect.to:isAlive() then
				--local card=room:askForUseCard(effect.to, "slash", "@jiepai:"..player:objectName(), -1, sgs.Card_MethodUse, false) 			
				local card = room:askForUseSlashTo(effect.to, player, "@jiepai-slash:"..player:objectName(),false);
    
				if not card then
					if player:canDiscard(effect.to, "he") then 
						to_throw = room:askForCardChosen(player, effect.to, "he", self:objectName(), false, sgs.Card_MethodDiscard)
						room:throwCard(to_throw, effect.to, player)
					end
				end
			end
		end
	end
}

leiting_vs = sgs.CreateViewAsSkill{ 
	name = "leiting" ,
	n = 1 ,
	view_filter = function(self, selected, to_select)
		return (not to_select:isEquipped()) and to_select:isKindOf("Slash") and (not to_select:isKindOf("ThunderSlash"))
	end,
	
	view_as = function(self, cards)
		if #cards==1 then
			local card=sgs.Sanguosha:cloneCard("thunder_slash", cards[1]:getSuit(), cards[1]:getNumber())
			card:addSubcard(cards[1])
			card:setSkillName("leiting")  
			return card
		end
	end ,
	enabled_at_play = function(self, player)
		return sgs.Slash_IsAvailable(player)
	end ,
	enabled_at_response = function(self, player, pattern)
		return (pattern == "slash") and (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE)
	end ,
}
leiting=sgs.CreateTriggerSkill{
	name="leiting",
	events={sgs.CardFinished},
	view_as_skill=leiting_vs,
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local use=data:toCardUse()
		if use.card:isKindOf("ThunderSlash")  and  use.card:getSkillName()=="leiting" then
				--target_num =1--方天等追加的额外目标？？
				--target_num =target_num+sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, player, use.card)
				--x=use.to:length()-1
				room:loseHp(player,1)
		end
	end
}
leiting_mod = sgs.CreateTargetModSkill{
        name = "#leiting_mod",
        pattern="Slash",
        extra_target_func = function(self,player,card)
			if player:hasSkill(self:objectName()) and card:isKindOf("ThunderSlash") then
				local num=math.max(player:getLostHp(),1)
				return num		
            else
                return 0
            end
        end,
		distance_limit_func = function(self, from, card)
			if from:hasSkill(self:objectName())  and card:isKindOf("ThunderSlash")  then
				return 1000
			else
				return 0
			end
    end,
}

hzc002:addSkill(jiepai)
hzc002:addSkill(leiting)
hzc002:addSkill(leiting_mod)
extension:insertRelatedSkills("leiting", "#leiting_mod")
sgs.LoadTranslationTable{["hzc002"] = "堀川雷鼓",

  ["#hzc002"] = "梦幻的打击乐手",
  ["jiepai"] = "节拍",
  [":jiepai"] ="<font color=\"blue\"><b>锁定技，</b></font>每当你的【杀】被目标角色的【闪】抵消后，目标角色须对你使用一张【杀】，否则你弃置其一张牌。每当你的【杀】命中目标角色后，你摸一张牌。",
  ["@jiepai-slash"] = "你可以对<font color=\"#00FF00\"><b>%src </b></font>使用一张【杀】，否则会被<font color=\"#00FF00\"><b>%src </b></font>弃置一张牌",
  ["leiting"] = "雷霆",
  [":leiting"] = "你使用雷【杀】时，可以额外指定X名角色。（x为你已经损失的体力值，且最小为1）。你可以将非雷【杀】当作雷【杀】使用。若你以此法使用雷【杀】，在此雷【杀】结算完毕后，你须失去1点体力。",
 }

--【逆袭的天邪鬼——鬼人正邪】 编号：14003 
hzc003 = sgs.General(extension,"hzc003", "hzc",3,false)	
dianfu_card = sgs.CreateSkillCard{
	name = "dianfu",
	target_fixed = false,
	will_throw = true,
	
	filter = function(self, targets, to_select, player)
		return #targets <2  and to_select:objectName() ~= player:objectName()
	end,
	feasible= function(self, targets)
		return #targets==2 
	end,
	on_use = function(self, room, source, targets)
		room:swapSeat(targets[1],targets[2])
		touhou_logmessage("#DianfuSwap",targets[1],"dianfu",targets[2])
	end,
}
dianfu= sgs.CreateViewAsSkill{
	name = "dianfu",
	n = 0,
	
	enabled_at_play = function(self,player)
		return not player:hasUsed("#dianfu")
	end,
	
	view_as = function(self, cards)
			local card=dianfu_card:clone()
			card:setSkillName(self:objectName())
			return card
	end
	
}

nixi=sgs.CreateTriggerSkill{
	name="nixi",
	events={sgs.Damaged},

	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		if player:hasFlag("nixi_used") then return false end
		local damage =data:toDamage()
		if damage.from and damage.from:isAlive() and damage.from:objectName() ~= player:objectName() then
			if not player:askForSkillInvoke(self:objectName(),data) then return false end
			player:setFlags("nixi_used")
			player:setFlags("DimengTarget")
			damage.from:setFlags("DimengTarget")
			local  n1 = player:getHandcardNum()
			local n2 = damage.from:getHandcardNum()
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				if p:objectName() ~=player:objectName() and p:objectName() ~=damage.from:objectName() then
					local gongxinargs = sgs.JsonValueForLUA()
					gongxinargs:setStringAt(0, player:objectName())
					gongxinargs:setStringAt(1, damage.from:objectName())
					room:doNotify(p,sgs.CommandType.S_COMMAND_EXCHANGE_KNOWN_CARDS, 
                               gongxinargs)
				 
				end			   
			end
			local exchangeMove = sgs.CardsMoveList()
			local reason1 = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_SWAP, player:objectName(),damage.from:objectName(),"ShenDePile","")
                
			local move1 = sgs.CardsMoveStruct(player:handCards(), damage.from, sgs.Player_PlaceHand, reason1)
            local reason2 = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_SWAP, damage.from:objectName(),player:objectName(),"ShenDePile","")
                
			local move2 = sgs.CardsMoveStruct(damage.from:handCards(), player, sgs.Player_PlaceHand, reason2)
            
			--exchangeMove.push_back(move1);
			--exchangeMove.push_back(move2);
			exchangeMove:append(move1)
			exchangeMove:append(move2)
			
			touhou_logmessage("#Dimeng",player,tostring(n1),damage.from,tostring(n2))
			room:moveCards(exchangeMove, false)
			player:setFlags("-DimengTarget")
			damage.from:setFlags("-DimengTarget")
		end
		
	end
}
nixi_clear = sgs.CreateTriggerSkill{
	name = "#nixi_clear",
	events = {sgs.EventPhaseChanging},
	can_trigger=function(self,player)
		return true
	end,
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		local phase = data:toPhaseChange()
		if phase.to ~=sgs.Player_NotActive then return false end
		for _,p in sgs.qlist(room:getAlivePlayers()) do
			if p:hasFlag("nixi_used") then
				p:setFlags("-nixi_used")
			end	
		end
	end
}

nizhuan_card = sgs.CreateSkillCard{
	name = "nizhuan",
	target_fixed = true,
	will_throw = true,	
	
	on_use = function(self, room, source, targets)
		room:removePlayerMark(source, "@nizhuan")
		local choice = room:askForChoice(source,self:objectName(),"0+1+2+3+4+5")
        local n=tonumber(choice)
		local all =room:getAlivePlayers()
		room:sortByActionOrder(all)
		for _,p in sgs.qlist(all) do
			if p:getHp()<n  and p:isWounded() then
				local recover = sgs.RecoverStruct()
                recover.who = source
                room:recover(p, recover)
				continue
			end
			if p:getHp()>n  then
				room:loseHp(p,1)
				continue
			end
		end
	end,
}
nizhuanvs= sgs.CreateZeroCardViewAsSkill {
	name = "nizhuan",
	
	enabled_at_play = function(self,player)
		return  player:getMark("@nizhuan")>0
	end,
	view_as = function(self, cards)
		local qcard = nizhuan_card:clone()
		qcard:setSkillName(self:objectName())
		return qcard
	end
}
nizhuan = sgs.CreateTriggerSkill{
	name = "nizhuan", 
	frequency = sgs.Skill_Limited,
	view_as_skill = nizhuanvs,
	limit_mark = "@nizhuan",
	events = {sgs.GameStart},  
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if event ==sgs.GameStart then
			--player:gainMark("@nizhuan",1)
		end
	end
}

hzc003:addSkill(dianfu)
hzc003:addSkill(nixi)
hzc003:addSkill(nixi_clear)
hzc003:addSkill(nizhuan)
extension:insertRelatedSkills("nixi", "#nixi_clear")
sgs.LoadTranslationTable{["hzc003"] = "鬼人正邪",

  ["#hzc003"] = "逆袭的天邪鬼",
  ["dianfu"] = "颠覆",
  [":dianfu"] ="出牌阶段，你可以指定两名其他角色交换位置。<font color=\"green\"><b>每阶段限一次</b></font>",
 ["#DianfuSwap"]="由于%arg 的效果， %from 和 %to 交换了座位",
 ["nixi"] = "逆袭",
  [":nixi"] ="每当你受到一次伤害后，你可以和伤害来源交换手牌。<font color=\"green\"><b>每回合限一次</b></font>",
 ["nizhuan"] = "逆转",
  [":nizhuan"] ="<font color=\"red\"><b>限定技，</b></font>出牌阶段，你可以选择一个0到5之间的数值X。所有体力大于X的角色必须失去1点体力，所有体力小于X的角色必须回复一点体力。",
	["@nizhuan"] = "逆转",
}

--【古旧的琴的付丧神——九十九八桥】 编号：14004 
hzc004 = sgs.General(extension,"hzc004", "hzc",3,false)	
zhenhun=sgs.CreateTriggerSkill{
name="zhenhun",
events=sgs.Dying,
on_trigger=function(self,event,player,data)
    local room=player:getRoom()
    local who=room:getCurrentDyingPlayer()
	local source=room:findPlayerBySkillName(self:objectName())
	if not source  then return false end
	if source:hasFlag("zhenhun_used") then return false end
	if source:getPhase() ~= sgs.Player_NotActive then return false end
	local num=0
	for _,p in sgs.qlist(room:getAllPlayers()) do
		if p:getHandcardNum()>num then
			num=p:getHandcardNum()
		end
	end
	if num==0 then return false end
	local list=sgs.SPlayerList()
	for _,p in sgs.qlist(room:getAllPlayers()) do
		if p:getHandcardNum()==num  and source:canDiscard(p, "h") then
			list:append(p)
		end
	end
	if list:length()==0 then return false end
	local target = room:askForPlayerChosen(source,list,self:objectName(),"@zhenhun:"..who:objectName(),true,true)
    if target then                  				
		local id=room:askForCardChosen(source, target, "h", self:objectName(),false,sgs.Card_MethodDiscard)
		room:throwCard(id, target,source)
		local recover=sgs.RecoverStruct()
		recover.who=source
		room:recover(who,recover)
		source:setFlags("zhenhun_used")
	end
end
}
zhenhun_clear = sgs.CreateTriggerSkill{
	name = "#zhenhun_clear",
	events = {sgs.EventPhaseChanging},
	can_trigger=function(self,player)
		return true
	end,
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		local phase = data:toPhaseChange()
		if phase.to ~=sgs.Player_NotActive then return false end
		for _,p in sgs.qlist(room:getAlivePlayers()) do
			if p:hasFlag("zhenhun_used") then
				p:setFlags("-zhenhun_used")
			end	
		end
	end
}

touhou_liuli_card = sgs.CreateSkillCard{
	name = "touhou_liuli",
	target_fixed = false,
	will_throw = true,
	
	filter = function(self, targets, to_select, player)
		local x= math.max(1,player:getLostHp())
		return #targets <x and ((not to_select:isWounded()) or  to_select:getHandcardNum()>to_select:getMaxHp())
	end,
	
	on_use = function(self, room, source, targets)
		count=0
		for var=1, #targets,1 do
			target=targets[var]
			if target:getHandcardNum() >= 2 and room:askForDiscard(target,self:objectName(),2,2,true,false,"@touhou_liuli:"..tostring(2)) then
						--加入手牌限制 主要是默认全弃置 然后弃置张数不到x也返回true。。。
			else
				room:loseHp(target,1)
			end
		end
	end,
}
touhou_liuli= sgs.CreateViewAsSkill{
	name = "touhou_liuli",
	n = 0,
	
	enabled_at_play = function(self,player)
		return not player:hasUsed("#touhou_liuli")
	end,
	
	view_as = function(self, cards)
			local card=touhou_liuli_card:clone()
			card:setSkillName(self:objectName())
			return card
	end
	
}

hzc004:addSkill(zhenhun)
hzc004:addSkill(zhenhun_clear)
hzc004:addSkill(touhou_liuli)
extension:insertRelatedSkills("zhenhun", "#zhenhun_clear")
sgs.LoadTranslationTable{["hzc004"] = "九十九八桥",

  ["#hzc004"] = "古旧的琴的付丧神",
  ["zhenhun"] = "镇魂",
  [":zhenhun"] ="你的回合外，当一名角色进入濒死时，你可以弃置场上手牌数最多者的一张手牌，令该角色回复1点体力。<font color=\"green\"><b>每回合限一次</b></font>",
 ["@zhenhun"] = "选择一名手牌数最多的目标,你可以弃置其一张牌，令<font color=\"#00FF00\"><b>%src </b></font>回复一点体力。" ,
        
  ["touhou_liuli"] = "琉璃",
  [":touhou_liuli"] ="出牌阶段，你可以指定x名没有受伤或者手牌数大于其体力上限的角色，令其选择一项：弃置两张手牌或者失去一点体力。X为你已经损失的体力值，且最少为1。<font color=\"green\"><b>每阶段限一次</b></font>",
 ["@touhou_liuli"] ="你必须弃置 %src 张手牌，否则将失去1点体力",
  
 }
--【古旧琵琶的付丧神——九十九弁弁】 编号：14005
hzc005 = sgs.General(extension,"hzc005", "hzc",3,false)	
youyuan=sgs.CreateTriggerSkill{
	name="youyuan",
	events={sgs.EventPhaseStart},
	can_trigger=function(self,player)
		return true
	end,
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local current =room:getCurrent()
		local source=room:findPlayerBySkillName(self:objectName())
		
		if not source then return false end
		
		if current:objectName() == source:objectName() then return false end
		
		if event==sgs.EventPhaseStart  and current:getPhase()== sgs.Player_Start  then
			if current:getHp() > source:getHp() and not current:isKongcheng() then
				if not source:askForSkillInvoke(self:objectName(),data) then return false end
				local id=room:askForCardChosen(source, current, "h", self:objectName())
				room:obtainCard(source,id,false) 	
			end
		end
	end
}
hzc005:addSkill(youyuan)
hzc005:addSkill(touhou_liuli)
sgs.LoadTranslationTable{["hzc005"] = "九十九弁弁",

  ["#hzc005"] = "古旧琵琶的付丧神",
  ["youyuan"] = "幽怨",
  [":youyuan"] ="其他角色的回合开始阶段开始时，若其体力值大于你，你可以获得其一张手牌。",
 
 }

--【竹林的人狼——今泉影狼】 编号：14006 
hzc006 = sgs.General(extension,"hzc006", "hzc",4,false)	
--[源码改动]修改Player::canDiscard()
renlang=sgs.CreateTriggerSkill{
	name="renlang",
	frequency = sgs.Skill_Compulsory,
	events={sgs.EventPhaseStart},

	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		if event==sgs.EventPhaseStart  and player:getPhase()== sgs.Player_Start  then
			if player:getMark("manyue") == 0 then
				if player:getMark("@wolf")== 0then
					player:gainMark("@wolf")
				else
					player:loseMark("@wolf")
				end
			end
		end
	end
}
renlang_mod = sgs.CreateTargetModSkill{
        name = "#renlang_mod",
        pattern="Slash",
        distance_limit_func = function(self, from, card)
            if from:hasSkill(self:objectName()) and from:getMark("@wolf")>0 and card:isKindOf("Slash") then
                return 1000
            else
                return 0
            end
        end,
}
touhou_bushi = sgs.CreateTriggerSkill{
    name = "touhou_bushi",
	events = {sgs.Damage,sgs.SlashMissed},
	on_trigger = function(self, event, player, data)
        local room = player:getRoom()
        if event ==sgs.SlashMissed then
			local effect = data:toSlashEffect()
			if effect.from:getWeapon() then return false end
			if not effect.to:isAlive()  then return false end
			if not effect.from:canSlash(effect.to, nil, false) then return false end
			local card = room:askForUseSlashTo(effect.from, effect.to, "@bushi-slash:"..effect.to:objectName(),false,true) --false,true 参数可以限制方天类额外指定多个目标
		end
		if event == sgs.Damage then
			local damage = data:toDamage()
			if damage.card and damage.card:isKindOf("Slash") then
				if (damage.chain or damage.transfer or not damage.by_user) then return false end
				if not damage.from or not damage.to then return false end
				if damage.from:objectName() ==damage.to:objectName() then return false end
			
				if damage.to:isAlive() and not damage.to:isNude() and room:askForSkillInvoke(player, self:objectName(), data) then
					local id = room:askForCardChosen(player, damage.to, "he", self:objectName())
					room:obtainCard(player,id,room:getCardPlace(id) ~= sgs.Player_PlaceHand)
				end
			end
		end
        return false
    end
}
manyue = sgs.CreateTriggerSkill{
        name = "manyue",
        events = sgs.Dying,
        frequency = sgs.Skill_Wake,
        can_trigger = function(self, target)
            return target:hasSkill(self:objectName()) and target:getMark("manyue")==0
				and target:getMark("@wolf") ==0
        end,
        on_trigger = function(self, event, player, data)
            local room = player:getRoom()
			local victim = room:getCurrentDyingPlayer()
			if victim:objectName() ~= player:objectName() then return false end    
				
            room:addPlayerMark(player, self:objectName())

			touhou_logmessage("#ManyueWake",player,self:objectName())
			room:notifySkillInvoked(player, self:objectName())
			if room:changeMaxHpForAwakenSkill(player) then
				player:gainMark("@wolf")
				x=3-player:getHp()
				local recov = sgs.RecoverStruct()
				recov.recover = x
				recov.who = player
				room:recover(player, recov)
				player:drawCards(6)
            end
        end
}
hzc006:addSkill(renlang)
hzc006:addSkill(renlang_mod)
hzc006:addSkill(touhou_bushi)
hzc006:addSkill(manyue)
extension:insertRelatedSkills("renlang", "#renlang_mod")
sgs.LoadTranslationTable{["hzc006"] = "今泉影狼",

  ["#hzc006"] = "竹林的人狼",
  ["renlang"] = "人狼",
  [":renlang"] ="<font color=\"#808080\"><b>永久技，</b></font>回合开始阶段开始时，“满月”觉醒前，若你没有狼标记，你获得一枚“狼”标记。反之，失去一枚“狼”标记。当你拥有狼标记时，你使用杀没有距离限制，你的装备区里的防具不能被其他角色弃置。",
	["@wolf"]="狼",
	
	["touhou_bushi"] = "捕食",
  [":touhou_bushi"] ="你使用的【杀】被目标角色的【闪】抵消后，若你没有装备武器，你可以对该角色再使用一张【杀】。你使用【杀】对目标角色造成一次伤害后，你可以获得该角色一张牌。",
  ["@bushi-slash"] = "你可以对<font color=\"#00FF00\"><b>%src </b></font>追加使用一张【杀】",
  
  ["manyue"] = "满月",
  [":manyue"] ="<font color=\"purple\"><b>觉醒技，</b></font>你进入濒死时，若你没有“狼”标记，你可以获得一枚“狼”标记，扣减一点体力上限，将体力回复到3，摸6张牌，并永久持有“狼”标记。",
["#ManyueWake"]="%from 进入濒死状态且没有“狼”标记，触发“%arg”觉醒",
 }

 --【辘轳首的怪奇——赤蛮奇】 编号：14007
hzc007 = sgs.General(extension,"hzc007", "hzc",4,false)
feitou=sgs.CreateTriggerSkill{
name="feitou",
events={sgs.EventPhaseStart},
on_trigger = function(self, event, player, data)
        local room=player:getRoom()
        if event==sgs.EventPhaseStart  and player:getPhase()== sgs.Player_Discard  then
			if player:isKongcheng() then return false end
			if player:getHandcardNum() <= player:getMaxCards() then return false end
			local cards=room:askForExchange(player, self:objectName(), 1, true, "addfeitou")
            if cards then
				id = cards:getSubcards():first()
				player:addToPile("feitou",id)
			end
		end        
end
}
feitou_slash=sgs.CreateTriggerSkill{
name="#feitou_slash",
events={sgs.CardFinished},
can_trigger=function(self,player)
	return true
end,
on_trigger = function(self, event, player, data)
        local room=player:getRoom()
        if event==sgs.CardFinished then
			local source=room:findPlayerBySkillName("feitou")
			if not source then return false end
			--if player:objectName() ~=source:objectName() then return false end
			local feitous=source:getPile("feitou")
			if feitous:length()==0 then return false end
			local use = data:toCardUse()
			if use.card and use.card:isKindOf("Slash") then
				if use.to:length() ==1  then
					 local victim =use.to:first()
					 if victim:isDead() then return false end
					 if victim:objectName()==source:objectName() then return false end
					 local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
					if source:isCardLimited(slash, sgs.Card_MethodUse) then
						return false
					end
					if not source:canSlash(victim,slash,true) then
						return false
					end
					if not source:inMyAttackRange(victim) then return false end
					prompt="extra_slash:"..victim:objectName()
					if not source:askForSkillInvoke("feitou",sgs.QVariant(prompt)) then return false end
					room:fillAG(feitous,source)
					local id=room:askForAG(source,feitous,false,"feitou")
					local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, "", nil, self:objectName(), "")
					room:throwCard(sgs.Sanguosha:getCard(id), reason, nil)
					room:clearAG()
					slash:setSkillName("feitou")
					room:useCard(sgs.CardUseStruct(slash, source, victim),false)	
				end
			end
		end        
end
}
hzc007:addSkill(feitou)
hzc007:addSkill(feitou_slash)
extension:insertRelatedSkills("feitou", "#feitou_slash")
sgs.LoadTranslationTable{["hzc007"] = "赤蛮奇",

  ["#hzc007"] = "辘轳首的怪奇",
  ["feitou"] = "飞头",
  ["#feitou_slash"]= "飞头",
  ["feitou:extra_slash"] = "你是否对<font color=\"#00FF00\"><b>%src </b></font>发动“飞头”",
  ["addfeitou"] = "你可以将一张手牌作为“飞头”，置于武将牌上",
  [":feitou"] ="弃牌阶段开始时，若你的手牌数大于手牌上限，你可以将一张手牌置于你的武将牌上，称为“飞头”。每当一名角色使用【杀】，仅指定了一个目标（目标角色不能为你）且结算完毕之后，若该目标在你的攻击范围内，你可以将一张“飞头”置入弃牌堆，视为对该目标角色使用一张【杀】。",
 }
--【栖息于淡水的人鱼——若鹭姬】编号：14008 
hzc008 = sgs.General(extension,"hzc008", "hzc",3,false)	
jingtaovs= sgs.CreateZeroCardViewAsSkill {
	name = "jingtao",
	
	enabled_at_play = function(self,player)
		return  player:isWounded() and not player:hasFlag(self:objectName())
	end,
	view_as = function(self, cards)
		local card = sgs.Sanguosha:cloneCard("drowning",sgs.Card_NoSuit,0)
		card:setSkillName(self:objectName())
		return card
	end
}
jingtao=sgs.CreateTriggerSkill{
name="jingtao",
events=sgs.CardUsed,
view_as_skill=jingtaovs,
on_trigger=function(self,event,player,data)
        if data:toCardUse().card:getSkillName()==self:objectName() then
                local room=player:getRoom()
                room:setPlayerFlag(player,self:objectName())
        end
end
}

shuixing = sgs.CreateTriggerSkill{
    name = "shuixing",
    frequency = sgs.Skill_Compulsory,
    events = {sgs.DamageInflicted},
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local damage = data:toDamage()
				
                if damage.nature == sgs.DamageStruct_Fire then
						local x=player:getLostHp()
						if x>0 then 
							new_value= damage.damage-x
							if new_value<0 then
								new_value=0
							end
							damage.damage=new_value
							data:setValue(damage) --重新对伤害赋值
							touhou_logmessage("#shuixing",player,"shuixing",nil,x)
							room:notifySkillInvoked(player, self:objectName())
							if damage.damage==0 then
								return true
							end
						end
                end
        end
}

shizhu = sgs.CreateTriggerSkill{
    name = "shizhu",
    events = {sgs.CardsMoveOneTime},
        on_trigger = function(self,event,player,data)
            local room = player:getRoom()
            local move = data:toMoveOneTime()
			local reason = move.reason
			if move.from and move.from_places:contains(sgs.Player_PlaceEquip) 
				and bit32.band(reason.m_reason, sgs.CardMoveReason_S_MASK_BASIC_REASON) == sgs.CardMoveReason_S_REASON_DISCARD 				 
				then
				if move.from:objectName() == player:objectName() then return false end
				if player:hasFlag("shizhu_used") then return false end
				local e_ids = sgs.IntList()
				for _,id in sgs.qlist(move.card_ids) do
                         if move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceEquip then
                             e_ids:append(id)
                          end
                end
				if e_ids:length()>0 and  player:askForSkillInvoke(self:objectName(),data) then 
					room:fillAG(e_ids, player)
					card_id = room:askForAG(player, e_ids, true, self:objectName())
					room:clearAG()
					if card_id ~=-1 then 
						room:obtainCard(player,card_id,true)
						player:setFlags("shizhu_used")
					end
				end
			end   
        end
}
shizhu_clear = sgs.CreateTriggerSkill{
	name = "#shizhu_clear",
	events = {sgs.EventPhaseChanging},
	can_trigger=function(self,player)
		return true
	end,
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		local phase = data:toPhaseChange()
		if phase.to ~=sgs.Player_NotActive then return false end
		for _,p in sgs.qlist(room:getAlivePlayers()) do
			if p:hasFlag("shizhu_used") then
				p:setFlags("-shizhu_used")
			end	
		end
	end
}
hzc008:addSkill(jingtao)
hzc008:addSkill(shuixing)
hzc008:addSkill(shizhu)
hzc008:addSkill(shizhu_clear)
extension:insertRelatedSkills("shizhu", "#shizhu_clear")
sgs.LoadTranslationTable{["hzc008"] = "若鹭姬",

  ["#hzc008"] = "栖息于淡水的人鱼",
  ["jingtao"] = "惊涛",
  [":jingtao"] ="出牌阶段，若你已经受伤， 可以指定一个其他角色，视为对其使用【水淹七军】。<font color=\"green\"><b>每阶段限一次</b></font>",
  ["shuixing"] = "水性",
  [":shuixing"] ="<font color=\"blue\"><b>锁定技，</b></font>你受到的火焰伤害-x,x为你已损失的体力。",
  ["#shuixing"] = "%from的技能“%arg”被触发, %from 防止了 %arg2 点火焰属性伤害.",
  
  ["shizhu"] = "拾珠",
  [":shizhu"] ="其他角色装备区内的装备被弃置，且进入弃牌堆后，你可以获得之。<font color=\"green\"><b>每回合限一次</b></font>",
 }

 --【湖的冰精——琪露诺】编号：14009 
hzc009 = sgs.General(extension,"hzc009", "hzc",3,false)	
sgs.LoadTranslationTable{["hzc009"] = "琪露诺",

  ["#hzc009"] = "湖的冰精",
 }
sgs.LoadTranslationTable{
	
	["th14"] = "辉针城",
	["hzc"] = "辉",

}



