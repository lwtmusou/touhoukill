module("extensions.th13", package.seeall)
extension = sgs.Package("th13")
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
--技能代码   神灵庙
------------------------------------------------------------------------------------------------------------

--【圣德道士——丰聪耳神子】 编号：13001 
slm001 = sgs.General(extension,"slm001$","slm", 4,false)
shengge = sgs.CreateTriggerSkill{
        name = "shengge",
        events = sgs.EventPhaseStart,
        frequency = sgs.Skill_Wake,
        can_trigger = function(self, target)
                return target and target:isAlive() and target:hasSkill(self:objectName())
                          and target:getHandcardNum() == 0 and target:getPhase() == sgs.Player_Start  
						  and target:getMark(self:objectName()) == 0
        end,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
				--room:doLightbox("$ShenggeAnimate", 4000)
				room:addPlayerMark(player, self:objectName())
				touhou_logmessage("#ZhijiWake",player,self:objectName())
				room:notifySkillInvoked(player, self:objectName())
				if room:changeMaxHpForAwakenSkill(player) then
						player:drawCards(3)
                end
        end
}

qingtingCard = sgs.CreateSkillCard{
        name = "qingting",
        target_fixed = true,
        on_use = function(self, room, source, targets)
                for _,p in sgs.qlist(room:getOtherPlayers(source)) do
                        if p:isKongcheng() then continue end
                        local card
                        if source:getMark("shengge") == 0 then
                            --for ai
							_data=sgs.QVariant()
							_data:setValue(source)
							p:setTag("qingting_give",_data)
							--end
							--local prompt = string.format("qingtingGive:%s", source:objectName())
							card = room:askForExchange(p, self:objectName(), 1, false, "qingtingGive:"..source:objectName())--
							--for ai
							p:removeTag("qingting_give")	
							--end
                        else
                            card = sgs.Sanguosha:getCard(room:askForCardChosen(source, p, "h", "qingting")) 
                        end
                        source:obtainCard(card, false)
						room:setPlayerMark(p,"@qingting",1)
                end
                for _,p in sgs.qlist(room:getOtherPlayers(source)) do
                        if p:getMark("@qingting")==0 then continue end
						room:setPlayerMark(p,"@qingting",0)
						if source:isKongcheng() then continue end  --由于讨还等导致空手牌 不用归还了 但不能消除flag 要让for循环走完
						
						--for ai
						_data=sgs.QVariant()
						_data:setValue(p)
						source:setTag("qingting_return",_data)
						--end
						local card = room:askForExchange(source, self:objectName(), 1, false, "qingtingReturn:"..p:objectName())
                        --for ai
						source:removeTag("qingting_return")
						--end
						p:obtainCard(card, false)	
                end
				
        end
}

qingting = sgs.CreateViewAsSkill{
        name = "qingting",
        n = 0,
        view_as = function(self, cards)
                return qingtingCard:clone()
        end,
        enabled_at_play = function(self, player)
                return not player:hasUsed("#qingting")
        end,
}

chiling = sgs.CreateTriggerSkill{
        name = "chiling$",
        events = {sgs.GameStart, sgs.CardsMoveOneTime,sgs.EventAcquireSkill,sgs.EventLoseSkill},
        can_trigger = function(self, target)
                return target
				--target and target:isAlive() and target:hasLordSkill(self:objectName())
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
						if not p:hasSkill("chilingGiven") then
							room:attachSkillToPlayer(p, "chilingGiven",true)
						end
					end         
				end
                if (event == sgs.EventLoseSkill and data:toString() == "chiling") then
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
						if  p:hasSkill("chilingGiven") then
							room:detachSkillFromPlayer(p, "chilingGiven",true)													
							
						end
					end
				end
				if event == sgs.CardsMoveOneTime then
                        local move = data:toMoveOneTime()
                        if move.from and move.from:hasLordSkill(self:objectName())  and
							move.from:objectName() == player:objectName() and move.from_places:contains(sgs.Player_PlaceHand)
                                    and move.to_place ==sgs.Player_PlaceHand and move.to and move.to:objectName() ~= player:objectName() 
									and move.to:getKingdom() == "slm" then
									 
                                local target
                                for _,p in sgs.qlist(room:getOtherPlayers(player)) do
                                        if p:objectName() == move.to:objectName() then
                                                target = p
                                                break
                                        end
                                end
								if not target then return false end
                                local h_ids = sgs.IntList()
                                for _,id in sgs.qlist(move.card_ids) do
                                        if move.from_places:at(move.card_ids:indexOf(id)) == sgs.Player_PlaceHand then
                                                h_ids:append(id)
                                        end
                                end
								
                                if h_ids:isEmpty()  then return false end
								--for ai and viewas skill
								local slash_ids = sgs.IntList()
                                for _,id in sgs.qlist(h_ids) do
                                        --room:showCard(target, id)
                                        room:getThread():delay(300)
                                        if sgs.Sanguosha:getCard(id):isKindOf("Slash") then 
											slash_ids:append(id) 
										end	
								end
								local _data=sgs.QVariant()
								if slash_ids:length()>0 then
									_data:setValue(1)
								else
									_data:setValue(0)
								end
								player:setTag("chiling_showslash",_data)
								local a_data=sgs.QVariant()
								a_data:setValue(target)
								player:setTag("chiling_givener",a_data)
								if not room:askForSkillInvoke(player, self:objectName(), sgs.QVariant("showcard:"..target:objectName())) then return false end
                                for _,id in sgs.qlist(h_ids) do
                                     room:showCard(target, id)
								end
								 
								room:setPlayerMark(target,"chiling-slash",slash_ids:first())
								local to_use = not slash_ids:isEmpty()
								while to_use do
                                        room:setPlayerMark(target,"chiling-slash",slash_ids:first())
										chiling_slash_list =sgs.SPlayerList()
										chiling_slash=sgs.Sanguosha:getCard(slash_ids:first())
										for _,p in sgs.qlist(room:getOtherPlayers(target)) do
											if target:canSlash(p,chiling_slash,true) then
												chiling_slash_list:append(p)
											end
										end
										if chiling_slash_list:isEmpty() then
											break
										else
											local card = room:askForUseCard(target, "@@chilingGiven", "@chiling-slash:"..player:objectName())
											if not card then break end
											slash_ids:removeOne(card:getEffectiveId())
											room:setPlayerMark(target,"chiling-slash",0)
											
										end
										to_use =not slash_ids:isEmpty()
                                end
                                
								room:setPlayerMark(target,"chiling-slash",0)
						end
                end
                return false
        end
}

chilingGiven = sgs.CreateViewAsSkill{
        name = "chilingGiven",
        n = 1,
        view_filter = function(self, selected, to_select)
                local id =sgs.Self:getMark("chiling-slash")
                return id==to_select:getEffectiveId()
        end,
        view_as = function(self, cards)
                if #cards == 0 then return nil end
                --local card=sgs.Sanguosha:cloneCard(cards[1]:objectName(), cards[1]:getSuit(), cards[1]:getNumber())
				--card:addSubcard(cards[1])
				--card:setSkillName("_"..self:objectName())
				return cards[1]--关系朱雀羽扇等可否二次转化的问题
        end,
        enabled_at_play = function(self, player)
                return false
        end,
        enabled_at_response = function(self, player, pattern)
            return pattern == "@@chilingGiven"
        end
}

if not sgs.Sanguosha:getSkill("chilingGiven") then
        local skillList=sgs.SkillList()
        skillList:append(chilingGiven)
        sgs.Sanguosha:addSkills(skillList)
end

chilingGiven_clearlimit=sgs.CreateTriggerSkill{
name="#chilingGiven_clearlimit",
events=sgs.CardUsed,
can_trigger = function(self, target)
    return target
end,
on_trigger=function(self,event,player,data)
        local card =data:toCardUse().card
		local from =data:toCardUse().from
        local room=player:getRoom()
		s=room:findPlayerBySkillName("chiling")
		
		if s and s:hasLordSkill("chiling") and from then
			if (card:getSkillName()=="chilingGiven") then                 
				room:addPlayerHistory(from, data:toCardUse().card:getClassName(), -1)	
			end
		end
			
end
}

slm001:addSkill(shengge)
slm001:addSkill(qingting)
slm001:addSkill(chiling)
slm001:addSkill(chilingGiven_clearlimit)
extension:insertRelatedSkills("chiling", "#chilingGiven_clearlimit")


 
--【狸猫怪十变化——二岩揣藏】 编号：13002 by三国有单
slm002 = sgs.General(extension, "slm002", "slm", 3, false) 
--如果从判断是否禁止时，cardLimitation()其实包含了xihua mark的禁止记录，
--但是为了clear方便，戏画的禁止记录还是需要独立。
xihua_choice =function(source) --提供出牌阶段使用戏画时的牌的种类选择
	choices={}
	--应该够用了  不用加
	--target:isCardLimited(slash, sgs.Card_MethodUse)
	if sgs.Slash_IsAvailable(source) and xihua_choice_limit(source,"slash")  then--杀
		table.insert(choices,"slash")
		table.insert(choices,"fire_slash")
		table.insert(choices,"thunder_slash")
	end
	if  sgs.Analeptic_IsAvailable(source) and xihua_choice_limit(source,"analeptic")  then--酒   
		table.insert(choices,"analeptic")
	end
	if source:isWounded() and xihua_choice_limit(source,"peach") then--桃
		table.insert(choices,"peach")
	end
	trick_strs={"amazing_grace","god_salvation","savage_assault","archery_attack","duel","ex_nihilo","snatch","dismantlement","collateral","iron_chain","fire_attack"}
	for _,trick_str in pairs (trick_strs) do
		if xihua_choice_limit(source,trick_str) then
			table.insert(choices,trick_str)
		end
	end
	table.insert(choices,"cancel")
	return table.concat(choices,"+")	
end
xihua_pattern_trans=function(pattern)
	local pattern1
	if pattern=="slash" or pattern=="fire_slash" or pattern=="thunder_slash" then
		pattern1="Slash"
	end
	if pattern=="jink"  then
		pattern1="Jink"
	end
	if pattern=="peach"  then
		pattern1="Peach"
	end
	if pattern=="analeptic"  then
		pattern1="Analeptic"
	end
	if pattern=="amazing_grace"  then
		pattern1="AmazingGrace"
	end
	if pattern=="god_salvation"  then
		pattern1="GodSalvation"
	end
	if pattern=="savage_assault"  then
		pattern1="SavageAssault"
	end
	if pattern=="archery_attack"  then
		pattern1="ArcheryAttack"
	end
	if pattern=="duel"  then
		pattern1="Duel"
	end
	if pattern=="ex_nihilo"  then
		pattern1="ExNihilo"
	end
	if pattern=="snatch"  then
		pattern1="Snatch"
	end
	if pattern=="dismantlement"  then
		pattern1="Dismantlement"
	end
	if pattern=="collateral"  then
		pattern1="Collateral"
	end
	if pattern=="iron_chain"  then
		pattern1="IronChain"
	end
	if pattern=="fire_attack"  then
		pattern1="FireAttack"
	end
	if pattern=="nullification"  then
		pattern1="Nullification"
	end
	return pattern1
end
xihua_choice_limit =function(player,pattern,method)--确定提供牌的种类时是否有使用限制
	markName="xihua_limit_"..pattern
	if not method then--默认为use
		method=sgs.Card_MethodUse
	end
	if player:getMark(markName) ==0 then   
		if not player:isCardLimited(sgs.Sanguosha:cloneCard(pattern), method, true) then
			return true
		else
			return false
		end
	else
		return false
	end
end
xihua_limit =function(room,player,pattern) --设置使用限制
	pattern1=xihua_pattern_trans(pattern)
	if pattern=="fire_slash" or pattern=="thunder_slash" then
		pattern="slash"
	end
	markName="xihua_limit_"..pattern
	room:setPlayerMark(player, markName,1)
	room:setPlayerCardLimitation(player, "use,response", pattern1,false)
end

function targetsTable2QList(thetable)
        local theqlist = sgs.PlayerList()
        for _, p in ipairs(thetable) do
                theqlist:append(p)
        end
        return theqlist
end
do_xihua=function(player,room,usecard)
	local target = room:askForPlayerChosen(player, room:getOtherPlayers(player), "xihua","@xihua_chosen:"..usecard:objectName(),false,true)
	local to_show = room:askForCardChosen(target, player, "h", "xihua")
	room:showCard(player, to_show)
	
	room:getThread():delay()
	local card = sgs.Sanguosha:getCard(to_show)
	t=false
	if card:isKindOf("TrickCard") and usecard:isKindOf("TrickCard") then
		t =true
	end
	if card:isKindOf("BasicCard") and usecard:isKindOf("BasicCard") then
		t =true
	end
	if card:getNumber()>10 or t then
		return true,to_show
	end
	touhou_logmessage("#Xihua_failed",player,"xihua",nil,usecard:objectName())
			
	return false,to_show
end
xihua_card = sgs.CreateSkillCard{
    name = "xihua",--"xihua_card"
	--target_fix没有起作用？？
    target_fixed = function(self)
		local pattern=self:getUserString()
		local usereason = sgs.Sanguosha:getCurrentCardUseReason()
		if (usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
			if pattern and  not pattern:startsWith("@") then
				local card = sgs.Sanguosha:cloneCard(pattern:split("+")[1])
				return card and card:targetFixed()
			end
		elseif 	(usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE) then
			return true
		end
		if pattern and  pattern=="@@xihua" then--一定是play阶段
			local str = player:property("xihua"):toString()
			local card =sgs.Sanguosha:cloneCard(str, sgs.Card_NoSuit, 0)
			return card and card:targetFixed()
		end
		return true
	end,
	
    will_throw = false,
    filter = function(self, targets, to_select, player)
		local pattern=self:getUserString()
		local usereason = sgs.Sanguosha:getCurrentCardUseReason()
		
		if (usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
			if pattern and  not pattern:startsWith("@") then
				local card = sgs.Sanguosha:cloneCard(pattern:split("+")[1])--:first()
				
				return card and card:targetFilter(targetsTable2QList(targets), to_select, player) and not sgs.Sanguosha:isProhibited(player, to_select, card)
			end
		elseif (usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE) then
			return false
		end
		
		if pattern and  pattern=="@@xihua" then--一定是play阶段
			local str = player:property("xihua"):toString()
			local card =sgs.Sanguosha:cloneCard(str, sgs.Card_NoSuit, 0)
			
			local t= card and card:targetFilter(targetsTable2QList(targets), to_select, player) and  not sgs.Sanguosha:isProhibited(player, to_select, card)
			if str=="ex_nihilo" or str=="peach" or str=="analeptic" then
				return t  and to_select:objectName()== player:objectName()
			end
			if card:isKindOf("AOE") or card:isKindOf("GlobalEffect") then
				return false
			end
			return t
		end
		return false 
	end,
	
	feasible = function(self, targets, player)
		local pattern=self:getUserString()
		local usereason = sgs.Sanguosha:getCurrentCardUseReason()
		
		if (usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
			if pattern and  not pattern:startsWith("@") then
				local card = sgs.Sanguosha:cloneCard(pattern:split("+")[1])
				return card  and card:targetsFeasible(targetsTable2QList(targets), player)
			end
		elseif (usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE) then
			return true
		end
		
		if pattern and   pattern=="@@xihua" then--一定是play阶段
			local str = player:property("xihua"):toString()
			local card =sgs.Sanguosha:cloneCard(str, sgs.Card_NoSuit, 0)
      
			return card and card:targetsFeasible(targetsTable2QList(targets), player)
		end
		return false
    end,
	
	on_validate = function(self, use)
		local room = use.from:getRoom()
		local pattern=self:getUserString()
		local str
		if  pattern=="@@xihua" then
			 str = use.from:property("xihua"):toString()
		else
			
			if not pattern:startsWith("@") then
				str= pattern:split("+")[1]
				if str == "slash" then
					str=room:askForChoice(use.from,"xihua","slash+thunder_slash+fire_slash")
				end
			end
		end
		if not str or str=="" then return nil end
		local card =sgs.Sanguosha:cloneCard(str, sgs.Card_NoSuit, 0)		
		
		local log = sgs.LogMessage()
		if use.to:isEmpty() then
			log.type =  "#GuhuoNoTarget" 
		else
			log.type = "#Guhuo"
		end
		log.from = use.from
		log.to = use.to
		log.arg = str
		log.arg2 = "xihua"

		room:sendLog(log);
		room:getThread():delay(300)
		result, id =do_xihua(use.from,room,card)
		if result then
			tmpcard= sgs.Sanguosha:getCard(id)
			use_card = sgs.Sanguosha:cloneCard(str, tmpcard:getSuit(), tmpcard:getNumber())
			use_card:setSkillName("xihua")
			use_card:addSubcard(id)
			use_card:deleteLater()
			return use_card
		else
			room:throwCard(id,use.from)
			xihua_limit(room,use.from,str)
			
			--借刀出杀     VS 主动出牌  nil 和 false 
			if pattern=="@@xihua" then
				return false	--use
			else
				return nil--response use
			end
		end
	end,
	
	on_validate_in_response = function(self, user)
		local room = user:getRoom()
		local pattern=self:getUserString()
		local str =pattern
		if (pattern == "peach+analeptic") then
			if not xihua_choice_limit(user,"peach",sgs.Card_MethodResponse) then
				str="analeptic"
			elseif not xihua_choice_limit(user,"analeptic",sgs.Card_MethodResponse)  then
				str="peach"
			else
				str=room:askForChoice(user,"xihua","peach+analeptic")
			end
			
		end
		if str == "slash" then
			str=room:askForChoice(user,"xihua","slash+thunder_slash+fire_slash")
		end
		if not str or str=="" then return nil end
		local card =sgs.Sanguosha:cloneCard(str, sgs.Card_NoSuit, 0)		
		
		local log = sgs.LogMessage()
		log.type =  "#GuhuoNoTarget" 
		log.from = user
		log.arg = str
		log.arg2 = "xihua"

		room:sendLog(log)
		room:getThread():delay(300)
		result, id =do_xihua(user,room,card)
		if result then
			tmpcard= sgs.Sanguosha:getCard(id)
			use_card = sgs.Sanguosha:cloneCard(str, tmpcard:getSuit(), tmpcard:getNumber())
			use_card:setSkillName("xihua")
			use_card:addSubcard(id)
			use_card:deleteLater()
			return use_card
		else
			room:throwCard(id,user)
			xihua_limit(room,user,str)
			--打出 
			return nil
		end
	end,
}
xihua = sgs.CreateViewAsSkill{
	name = "xihua",
	n = 0,
	
	view_as = function(self, cards)
		local pattern=sgs.Sanguosha:getCurrentCardUsePattern()
		local usereason = sgs.Sanguosha:getCurrentCardUseReason()
		if (usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE
            or usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
            card = xihua_card:clone()
            card:setUserString(pattern)
			return card
        end
		return xihua_choice_card:clone()
	end,
	
	enabled_at_play = function(self,player)
		return  player:getHandcardNum()>0
	end,
	
	enabled_at_response = function(self, player, pattern)
		local usereason = sgs.Sanguosha:getCurrentCardUseReason()
		if player:getHandcardNum()>0 then	
			return pattern =="@@xihua" 
				or   (xihua_choice_limit(player,"slash",sgs.Card_MethodResponse) and pattern=="slash")
				or (string.find(pattern,"peach") and (xihua_choice_limit(player,"peach",sgs.Card_MethodResponse)))
				or (string.find(pattern,"analeptic") and (xihua_choice_limit(player,"analeptic",sgs.Card_MethodResponse)) )
				or pattern=="nullification"
				or (xihua_choice_limit(player,"jink",sgs.Card_MethodResponse) and pattern=="jink") 
		end
	end,
	
	enabled_at_nullification=function(self,player) 
		pattern ="nullification"			
		if player:getHandcardNum()>0  and xihua_choice_limit(player,pattern,sgs.Card_MethodResponse) then
			return true
		end
	end,
}
xihua_choice_card = sgs.CreateSkillCard{ 
	name = "xihuachoice",
	target_fixed = true,
	will_throw = false,
	
	on_use = function(self, room, source, targets)
		ask_str =xihua_choice(source)
		local choice = room:askForChoice(source,"xihua",ask_str)
		--再次调用viewas
		if choice=="cancel" then return false end
		room:setPlayerProperty(source, "xihua", sgs.QVariant(choice))
					
		
		room:askForUseCard(source,"@@xihua","@xihua_target:"..choice) 
	end,

}

xihua_clear = sgs.CreateTriggerSkill{
	name = "#xihua_clear", 
	events = {sgs.EventPhaseChanging},  
	can_trigger=function(self,player)
		return player
	end,
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		local change = data:toPhaseChange()
		if change.to == sgs.Player_NotActive then
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				--if p:hasSkill("xihua")  then  --可能由于【凭依】而失去技能，但是已经作出的限制要直到回合后解除。
					local patterns={"slash","jink","analeptic","peach","amazing_grace","god_salvation","savage_assault","archery_attack","duel","ex_nihilo","snatch","dismantlement","collateral","iron_chain","fire_attack","nullification"}
					for _,pattern in pairs (patterns) do
						markName="xihua_limit_"..pattern
						if p:getMark(markName)>0 then--所以只认【戏画】限制的记录
							room:setPlayerMark(p, markName,0)
							local pattern1=xihua_pattern_trans(pattern)
							room:removePlayerCardLimitation(p, "use,response", pattern1.."$0")
						end
					end
					room:setPlayerFlag(p,"-xihua_slash")
				--end
			end
		end
	end
}

slm002:addSkill(xihua)
slm002:addSkill(xihua_clear)
extension:insertRelatedSkills("xihua", "#xihua_clear")



--【古代日本的尸解仙——物部布都】 编号：13003 by三国有单
slm003 = sgs.General(extension, "slm003", "slm", 3, false) --13003
shijie_save = function(room,source,card_ids,target) --尸解效果 自定义函数
	room:fillAG(card_ids, source)
	card_id = room:askForAG(source, card_ids, false, "shijie")
	room:clearAG(source)
	
	local owner
	for _,p in sgs.qlist(room:getAlivePlayers()) do
		for _,card in sgs.qlist(p:getCards("e")) do
			if card:getId() == card_id then 
				owner =p 
				break 
			end
		end
	end
	if owner then
		room:throwCard(card_id,owner, source)	
		local recover=sgs.RecoverStruct()
		recover.recover=1
		recover.who = source
		room:recover(target,recover)
	end
end
shijie_card=sgs.CreateSkillCard{
name="shijie",
target_fixed=true,
will_throw=false,

on_use=function(self,room,source,targets)
        
		local who=room:getCurrentDyingPlayer()
        if not who then return false end
		
		local judge =sgs.JudgeStruct()
		judge.pattern = ".|.|."  
		judge.good=true
		judge.reason="shijie"
		judge.who=who
		room:setPlayerMark(source,"shijie",1)		
		room:judge(judge)
		
		
		local spades = sgs.IntList()
		local hearts = sgs.IntList()
		local clubs = sgs.IntList()
		local diamonds = sgs.IntList()
		
		for _,p in sgs.qlist(room:getAlivePlayers()) do
			for _,card in sgs.qlist(p:getCards("e")) do
				if card:getSuit()==  sgs.Card_Heart  then  hearts:append(card:getId()) end
				if card:getSuit()==  sgs.Card_Spade  then  spades:append(card:getId()) end
				if card:getSuit()==  sgs.Card_Club  then  clubs:append(card:getId()) end
				if card:getSuit()==  sgs.Card_Diamond  then  diamonds:append(card:getId()) end
			end
		end
		if judge.card:getSuit()==  sgs.Card_Heart and hearts:length()>0 then
			shijie_save(room,source,hearts,who) --自定义函数
		end
		if judge.card:getSuit()==  sgs.Card_Spade and spades:length()>0 then
			shijie_save (room,source,spades,who) 
		end
		if judge.card:getSuit()==  sgs.Card_Club and clubs:length()>0 then
			shijie_save(room,source,clubs,who) 
		end
		if judge.card:getSuit()==  sgs.Card_Diamond and diamonds:length()>0 then
			shijie_save (room,source,diamonds,who) 
		end
end
}

shijie=sgs.CreateViewAsSkill{--"尸解"
name="shijie",
n=0,
view_as=function(self,cards)
        return shijie_card:clone()
end,
enabled_at_play=function(self,player)
        return false
end,
enabled_at_response = function(self, player, pattern)
   return  string.find(pattern, "peach") and player:getMark("shijie")==0  and not player:hasFlag("Global_PreventPeach")
end
}

shijie_clear = sgs.CreateTriggerSkill{--"尸解"clear flag
	name = "#shijie_clear", 
	events = {sgs.EventPhaseEnd},  --sgs.QuitDying不行?
	can_trigger=function(self,player)
		return player
	end,
	
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		for _,p in sgs.qlist(room:getAlivePlayers()) do		
			if p:hasSkill("shijie") then
				room:setPlayerMark(p, "shijie",0)
			end
		end
	end
}
fengshui = sgs.CreateTriggerSkill{
        name = "fengshui",
        events = {sgs.AskForRetrial},
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local judge = data:toJudge()
				if player:isKongcheng() then return false end
                prompt="@fengshui-retrial"
				if  room:askForCard(player, ".|.|.|hand", prompt, data, sgs.Card_MethodDiscard, nil, true, self:objectName()) then
					card_ids =room:getNCards(3)
					--for ai
					_data=sgs.QVariant()
					_data:setValue(judge)
					player:setTag("fengshui_judge",_data)
					--
					room:fillAG(card_ids, player)
					card_id = room:askForAG(player, card_ids, true, "fengshui")
					room:clearAG(player)
					player:removeTag("fengshui_judge")
					--
					if card_id==-1 then 	
						
						room:askForGuanxing(player,card_ids,true)
					else
						card_ids:removeOne(card_id)
						room:askForGuanxing(player,card_ids,1)
                    
						local card = sgs.Sanguosha:getCard(card_id)
						room:retrial(card,player,judge,self:objectName())
					end
					
					
				end
        end

}
slm003:addSkill(shijie)
slm003:addSkill(shijie_clear)
slm003:addSkill(fengshui)
 extension:insertRelatedSkills("shijie", "#shijie_clear")


 
--【神明后裔的亡灵——苏我屠自古】 编号：13004 
slm004 = sgs.General(extension, "slm004", "slm", 4,false)
leishiCard = sgs.CreateSkillCard{
        name = "leishi",
        target_fixed = false,
        will_throw = true,
        filter = function(self,targets,to_select,player)
                local slash = sgs.Sanguosha:cloneCard("thunder_slash", sgs.Card_NoSuit, 0)
				return #targets == 0 and not to_select:isKongcheng() and  player:canSlash(to_select,slash,false)
        end,
        on_effect = function(self,effect)
                local room = effect.from:getRoom()
                local slash = sgs.Sanguosha:cloneCard("thunder_slash", sgs.Card_NoSuit, 0)
                slash:setFlags("leishislash")
                slash:setSkillName("_" .. self:objectName())
                room:useCard(sgs.CardUseStruct(slash,effect.from,effect.to),false)
		end
}
leishiVS = sgs.CreateViewAsSkill{
        name = "leishi" ,
        n = 0,
        view_as = function(self,card)
                return leishiCard:clone()
        end,
        enabled_at_play = function (self,player)
                return not player:hasUsed("#leishi")
        end

}

leishi= sgs.CreateTriggerSkill{
        name = "leishi",
		view_as_skill = leishiVS ,
        events = sgs.SlashMissed,
        on_trigger = function(self,event,player,data)
            local effect = data:toSlashEffect()
            if effect.slash:hasFlag("leishislash") then
                local room = effect.from:getRoom()
				room:loseHp(effect.from , 1)
            end
        end
}

fenyuan = sgs.CreateTriggerSkill{
        name = "fenyuan",
        events = sgs.Dying,
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local victim = room:getCurrentDyingPlayer()
                local current=room:getCurrent()
				if not current:isAlive() then return false end
				if victim:objectName() == player:objectName() and current:objectName() ~= player:objectName() then

						prompt="invoke:"..current:objectName()
						if room:askForSkillInvoke(player,self:objectName(),sgs.QVariant(prompt)) then
								room:killPlayer(player)
                                local damage = sgs.DamageStruct()
                                damage.to =current                    
								damage.damage = 2
                                damage.nature = sgs.DamageStruct_Thunder
                                room:damage(damage)
							
                        end
						
                end
        end

}

slm004:addSkill(leishi)
slm004:addSkill(fenyuan)

 
--【穿壁之邪仙人——霍青娥】 编号：13005
slm005 = sgs.General(extension,"slm005", "slm", 3,false)
--to_select:isCardLimited暂时有问题。。。
xiefacard=sgs.CreateSkillCard{
name="xiefa",
will_throw=false,
filter = function(self, targets, to_select, player)
	local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
	
    slash:deleteLater()
    if #targets == 0 then
		--只好枚举了神雷米和狸猫
		--return not to_select:isCardLimited(slash,sgs.Card_MethodUse) 
		--手牌true false
		if to_select:objectName() == player:objectName() then
			return false
		end
		if  to_select:hasSkill("zhouye") and to_select:getMark("@ye")==0 then
			return false
		end
		if  to_select:hasSkill("xihua") and to_select:getMark("xihua_limit_slash")>0 then
			return false
		end
	end
    if #targets == 1 then
		--改用targetfilter反而比canslash麻烦？？
		if not targets[1]:canSlash(to_select,slash,true) then
			return false
		end
		--剑术等无视距离会通过canslash
		return targets[1]:inMyAttackRange(to_select) 
	end
	if #targets <2  then
		return  true
	end
end,
feasible= function(self, targets)
	return #targets==2 
end,
about_to_use = function(self, room, cardUse)
    --on_use里target顺序被sort，不能反映UI时选择顺序
	local from = cardUse.from
	local to1=cardUse.to:at(0)
	local to2=cardUse.to:at(1)
	--log
	touhou_logmessage("#ChoosePlayerWithSkill",from,"xiefa",cardUse.to,nil,1)
	to1:obtainCard(self,false)
        
	local slash=sgs.Sanguosha:cloneCard("slash",sgs.Card_NoSuit,0)
    slash:setSkillName("_xiefa")
    local use=sgs.CardUseStruct()
    use.from=to1
    use.to:append(to2)
    use.card=slash
    room:useCard(use)            
end ,
}
xiefavs=sgs.CreateViewAsSkill{
name="xiefa",
n=1,
view_filter=function(self,selected,to_select)
        return not to_select:isEquipped()
end,
view_as=function(self,cards)
        if #cards==1 then
                local card=xiefacard:clone()
                card:addSubcard(cards[1])
                return card
        end
end,
enabled_at_play=function(self,player)
        return not player:hasUsed("#xiefa")
end
}
xiefa=sgs.CreateTriggerSkill{
name="xiefa",
events={sgs.SlashMissed,sgs.Damaged},
view_as_skill=xiefavs,
on_trigger=function(self,event,player,data)
        local room=player:getRoom()

        local source=room:findPlayerBySkillName(self:objectName())

		if event==sgs.SlashMissed then
                if data:toSlashEffect().slash:getSkillName()==self:objectName() then
						room:loseHp(player)
                end
        elseif event==sgs.Damaged then
                damage=data:toDamage()
				card=data:toDamage().card
				if card and card:getSkillName()==self:objectName() then
                        if (damage.chain or damage.transfer ) then return false end
						if not damage.from or not damage.to then return false end
						if damage.from:objectName() ==damage.to:objectName() then return false end
			
						--or not damage.by_user 【无念】可摸牌
						if source then
                                source:drawCards(1)
                        end
                end
        end
end,
can_trigger=function(self, player)
        return player
end
}

chuanbi=sgs.CreateTriggerSkill{
name="chuanbi",
events=sgs.CardAsked,
on_trigger=function(self,event,player,data)
		if data:toStringList()[1]=="jink" then
                
				local room=player:getRoom()
                local current=room:getCurrent()
                if current and current:isAlive() and not current:getWeapon() then
                        if player:askForSkillInvoke(self:objectName(),data) then
                                room:broadcastSkillInvoke(self:objectName())
                                local jink = sgs.Sanguosha:cloneCard("jink", sgs.Card_NoSuit, 0)
                                jink:setSkillName("_chuanbi")
                                room:provide(jink)
                                
								return true
                        end
                end
        end
end
}

slm005:addSkill(xiefa)
slm005:addSkill(chuanbi)


--【忠诚的尸体——宫古芳香】 编号：13006
slm006 = sgs.General(extension,"slm006", "slm", 4,false) 
duzhuavs=sgs.CreateViewAsSkill{
name="duzhua",
n = 1,
view_filter = function(self, selected, to_select)
        return to_select:isRed() and not to_select:isEquipped()
end,
view_as = function(self, cards)
        if #cards == 1 then
				local slash = sgs.Sanguosha:cloneCard("slash", cards[1]:getSuit(), cards[1]:getNumber())
                slash:addSubcard(cards[1])
                slash:setSkillName(self:objectName())
                return slash
        end
end,
enabled_at_play = function(self, player)
        return not player:isKongcheng() and not player:hasFlag(self:objectName()) --and sgs.Slash_IsAvailable(player)
end
}

duzhua=sgs.CreateTriggerSkill{
name="duzhua",
events=sgs.PreCardUsed,
view_as_skill=duzhuavs,
on_trigger=function(self,event,player,data)
        if data:toCardUse().card:getSkillName()==self:objectName() then
                local room=player:getRoom()
                room:addPlayerHistory(player, data:toCardUse().card:getClassName(), -1)
                room:setPlayerFlag(player,self:objectName())
        end
end
}
duzhuaTargetMod = sgs.CreateTargetModSkill{
        name = "#duzhuaTargetMod",
        pattern="Slash", --为了ai pattern不能少??
		residue_func = function(self, player,card)
                if player:hasSkill(self:objectName())  and (card:getSkillName()=="duzhua")then
					return 1
                end
        end,
}
taotie=sgs.CreateTriggerSkill{
name="taotie",
events=sgs.CardResponded,
on_trigger=function(self,event,player,data)
        local room=player:getRoom()
        local source=room:findPlayerBySkillName(self:objectName())
        if not source then return false end
        if data:toCardResponse().m_card:isKindOf("Jink") and player:objectName()~=source:objectName() and source:isWounded() then
				if not data:toCardResponse().m_isUse then return false end
				if source:askForSkillInvoke(self:objectName(),data) then
                        room:broadcastSkillInvoke(self:objectName())
                        local judge=sgs.JudgeStruct()
                        judge.reason=self:objectName()
                        judge.who=source
                        judge.good=true
                        judge.pattern = ".|black"
                        judge.negative = false
                        room:judge(judge)
                        if judge:isGood() then
                                local recover=sgs.RecoverStruct()
                                room:recover(source,recover)
                        end
						
                end
				 
        end
end,
can_trigger=function(self,player)
        return player
end
}

slm006:addSkill(duzhua)
slm006:addSkill(duzhuaTargetMod)
extension:insertRelatedSkills("duzhua","#duzhuaTargetMod")
slm006:addSkill(taotie)

 
--【念经的山灵——幽谷响子】 编号：13007 
slm007 = sgs.General(extension, "slm007", "slm", 3,false) 
huisheng = sgs.CreateTriggerSkill{
        name = "huisheng",
        events = {sgs.TargetConfirming,sgs.CardFinished,sgs.PreCardUsed},
        priority = 1,
        view_as_skill = huishengVS,
		on_trigger = function(self,event,player,data)
                local use = data:toCardUse()
                local room = player:getRoom()
                --同将模式怎么办？？
				local source = room:findPlayerBySkillName("huisheng")
                if not source then return false end
				if event == sgs.CardFinished then
                        --if use.card:hasFlag("huishengflag") then
                       if use.from and use.from:objectName() ~= source:objectName() 
					   and (use.to:length() == 1 and use.to:contains(source)) 
					   and (use.card:isKindOf("BasicCard") or  use.card:isNDTrick())then
							if use.card:isKindOf("Jink") then return false end
							local card = sgs.Sanguosha:cloneCard(use.card:objectName(), sgs.Card_NoSuit, 0)
								
								--特殊例子  被神四季禁止杀 然后放aoe 阿燐【怨灵】火杀  不能【回声】
								if source:isCardLimited(card, sgs.Card_MethodUse) then
									return false
								end
								
								if  use.from:isAlive() and not source:isProhibited(use.from, card) then
									
									--检测alive防止被【反击】死 use.from没有
									--isProhibited并不能很好的检测借刀、、、
									if card:isKindOf("Collateral") and not source:getWeapon() then
										return false
									end
									_data=sgs.QVariant()
									_data:setValue(use.from)
									a_data=sgs.QVariant()
									a_data:setValue(use)
									room:setTag("huisheng_use",a_data)
									prompt="use:"..use.from:objectName()..":"..use.card:objectName()
									if room:askForSkillInvoke(source,self:objectName(),sgs.QVariant(prompt)) then
                                        
										card:setSkillName("_" .. self:objectName())
                                        local listt = sgs.SPlayerList()
										if card:isKindOf("AOE") or card:isKindOf("GlobalEffect") then
											use.from:setFlags("huisheng_aoe")
										end
										if card:isKindOf("Collateral") then
											listt=sgs.SPlayerList()
											listt:append(use.from)
											victims=sgs.SPlayerList()
											for _,p in sgs.qlist(room:getOtherPlayers(use.from))do
												if (use.from:canSlash(p)) then--不是实体牌，不用考虑空城最后一张手牌的借刀
													victims:append(p)
												end
											end
											if victims:length()==0 then return false end
											local victim = room:askForPlayerChosen(source,victims,self:objectName(),"@huisheng_collateral:"..use.from:objectName(),false,true)
                       
											if victim then
												listt:append(victim)
												room:useCard(sgs.CardUseStruct(card, source, listt))
											end
											
										else
											room:useCard(sgs.CardUseStruct(card, source, use.from))
										end
									end
								end
                        end
                end
				if event == sgs.PreCardUsed then--取消aoe多余目标 就是难看点。。。
					 if use.card:getSkillName()=="huisheng" then
						if use.card:isKindOf("AOE") or use.card:isKindOf("GlobalEffect") then
							local target 
							for _,p in sgs.qlist(room:getOtherPlayers(player)) do
								if p:hasFlag("huisheng_aoe") then
									p:setFlags("-huisheng_aoe")
								else
									if use.to:contains(p) then
										use.to:removeOne(p)
									end
								end
							end
							data:setValue(use)
						end
					 end
				end
        end,
        can_trigger = function(self,target)
                return target
		 end
}

songjing = sgs.CreateTriggerSkill{
        name = "songjing",
        events = sgs.CardUsed,
		frequency = sgs.Skill_Frequent,
        on_trigger = function(self,event,player,data)
                local use = data:toCardUse()
                local room = player:getRoom()
                local source = room:findPlayerBySkillName(self:objectName())
                if use.card:isKindOf("DelayedTrick") then
                        prompt="use:"..use.from:objectName()..":"..use.card:objectName()
						if room:askForSkillInvoke(source,self:objectName(),sgs.QVariant(prompt)) then
                                source:drawCards(2)
							
						end
						
                end
        end,
        can_trigger = function(self,player)
                return player
        end
}
slm007:addSkill(huisheng)
slm007:addSkill(songjing) 


--【不再彷徨的亡灵——西行寺幽幽子】 编号：13008
slm008 = sgs.General(extension, "slm008", "slm", 3, false) --13008
SuitValue = function(suit)
        if type(suit) == "string" then
                if suit == "club" then
                        return 1
                elseif suit == "diamond" then
                        return 2
                elseif suit == "heart" then
                        return 4
                elseif suit == "spade" then
                        return 8
                else
                        return 0
                end
        elseif type(suit) == "number" then
                if suit == sgs.Card_Club then
                        return 1
                elseif suit == sgs.Card_Diamond then
                        return 2
                elseif suit == sgs.Card_Heart then
                        return 4
                elseif suit == sgs.Card_Spade then
                        return 8
                else
                        return 0
                end
        end
        return 0
end
xxsyyzchuixue = sgs.CreateTriggerSkill{
        name = "xxsyyzchuixue",
        events = {sgs.CardsMoveOneTime, sgs.EventPhaseStart} ,
        can_trigger = function(self, target)
                return target and target:isAlive()
        end ,
        on_trigger = function(self, event, player, data)
                if (event == sgs.CardsMoveOneTime) then
                        local move = data:toMoveOneTime()
                        if move.from and (move.from:objectName() == player:objectName()) and (player:getPhase() == sgs.Player_Discard)
                                        and (bit32.band(move.reason.m_reason, sgs.CardMoveReason_S_MASK_BASIC_REASON) == sgs.CardMoveReason_S_REASON_DISCARD) then
                                local room = player:getRoom()
                                for _, id in sgs.qlist(move.card_ids) do
                                        local card = sgs.Sanguosha:getCard(id)
                                        local mark = player:getMark("xxsyyzchuixue")
                                        mark = bit32.bor(mark, SuitValue(card:getSuit()))
                                        room:setPlayerMark(player, "xxsyyzchuixue", mark)
                                end
                        end
                else
                        --if player:getPhase() == sgs.Player_NotActive then
                        --        player:getRoom():setPlayerMark(player, "xxsyyzchuixue", 0)
                        --end
                end
                return false
        end ,
}
xxsyyzchuixueget = sgs.CreateTriggerSkill{
        name = "#xxsyyzchuixue-get" ,
        events = {sgs.EventPhaseEnd} ,
        on_trigger = function(self, event, player, data)
                if player:getPhase() == sgs.Player_Discard then
                        local mark = player:getMark("xxsyyzchuixue")
                        if mark == 0 then return false end
                        local allsuits = {"club", "diamond", "heart", "spade"}
                        local suits = {}
                        for _, suit in ipairs(allsuits) do
                                if bit32.band(mark, SuitValue(suit)) == 0 then table.insert(suits, suit) end
                        end
                        local pattern = ""
                        if #suits == 0 then pattern = "" else pattern = ".|" .. table.concat(suits, ",") .. "|.|hand" end
                        
						local room = player:getRoom()
						room:setPlayerMark(player, "xxsyyzchuixue", 0)
                        local target = room:askForPlayerChosen(player, room:getOtherPlayers(player), "xxsyyzchuixue", "@xxsyyzchuixue-select", true, true)
                        if target then
                                --for ai
								--需要什么信息呢？
								--end
								if (#suits == 0) or not room:askForCard(target, pattern, "@xxsyyzchuixue-discard:"..player:objectName()) then
                                        room:loseHp(target, 1)
                                end
                        end
                end
                return false
        end ,
}

xxsyyzwushou = sgs.CreateTriggerSkill{
        name = "xxsyyzwushou" ,
        events = {sgs.TargetConfirming} ,
		frequency = sgs.Skill_Frequent,
        on_trigger = function(self, event, player, data)
                local use = data:toCardUse()
                local room= player:getRoom()
				if use.card and use.card:isKindOf("Slash") then
                        if player:askForSkillInvoke(self:objectName()) then
                                player:drawCards(3)
                                if (player:getHp() > 0) then
                                        player:getRoom():askForDiscard(player, self:objectName(), player:getHp(), player:getHp(), false, false,"wushou_discard:"..tostring(player:getHp()) )
                                end
							
                        end
						
                end
                return false
        end ,
}

slm008:addSkill(xxsyyzchuixue)
slm008:addSkill(xxsyyzchuixueget)
slm008:addSkill(xxsyyzwushou)

extension:insertRelatedSkills("xxsyyzchuixue", "#xxsyyzchuixue-get")
 
 
--【古代妖怪之一——封兽鵺】 编号：13009 by三国有单
slm009 = sgs.General(extension, "slm009", "slm", 3, false)
buming_card = sgs.CreateSkillCard{
	name = "buming",
	target_fixed = false,
	will_throw = true,
	
	filter = function(self, targets, to_select, player)
		if to_select:objectName() ==player:objectName() then
			return false
		end
		if #targets>0 then 
			return false
		end
		local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
		local duel = sgs.Sanguosha:cloneCard("duel", sgs.Card_NoSuit, 0)
		
		local cards= self:getSubcards()
		local rangefix=0
		if player:getWeapon() and player:getWeapon():getId() == cards:first() then
			if player:getAttackRange() >player:getAttackRange(false) then
				rangefix = rangefix +player:getAttackRange() - player:getAttackRange(false)
			end
		end
		if player:getOffensiveHorse() and player:getOffensiveHorse():getId() == cards:first() then
			rangefix=rangefix+1
		end	
		if (not cards:isEmpty()) then
			slash:addSubcard(cards:first())--添加subcard重要，关系rangefix
			local slash_targets =sgs.PlayerList()
			return 
				--杀相关
				(slash:targetFilter(slash_targets, to_select, player) and not(player:isCardLimited(slash, sgs.Card_MethodUse)))
				----(player:canSlash(to_select,slash,true,rangefix) and not(player:isCardLimited(slash, sgs.Card_MethodUse)))
				--决斗相关
				or (  player:distanceTo(to_select, rangefix) <= player:getAttackRange() and
				not player:isProhibited(to_select, duel)and not player:isCardLimited(duel, sgs.Card_MethodUse))
		end
		return false
	end,
	
	on_use = function(self, room, source, targets)
		local target = targets[1]
		choices={}
		local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
		local duel = sgs.Sanguosha:cloneCard("duel", sgs.Card_NoSuit, 0)
		if source:canSlash(target,slash,true) and not source:isCardLimited(slash, sgs.Card_MethodUse) then
			table.insert(choices, "slash_buming")
		end
		if not source:isProhibited(target, duel) and not source:isCardLimited(duel, sgs.Card_MethodUse)   then
			table.insert(choices, "duel_buming")
		end
		if #choices==0 then return false end --基本不会发生 targetfilter时就要尽量排除
		local choice
		if #choices==1 then
			choice=choices[1]
		else
			choice = room:askForChoice(target,self:objectName(),table.concat(choices, "+"))
		end
		local card_use =  sgs.CardUseStruct()	
		if choice == "slash_buming" then 
			
			card_use.card = slash 
		end 
		if choice == "duel_buming" then 
			
			card_use.card = duel
		end 
		card_use.card :setSkillName(self:objectName())
		card_use.to:append(target)
		card_use.from = source
		room:useCard(card_use,false)

	end,

}
buming = sgs.CreateViewAsSkill{
	name = "buming",
	n = 1,
	enabled_at_play = function(self,player)
		return not player:hasUsed("#buming")
	end,
	view_filter = function(self, selected, to_select)
		return not sgs.Self:isJilei(to_select)  
		--体会一下处理鸡肋下的状况，靠willthrow和isJilei()的区别
	end,
	
	view_as = function(self, cards)
		if(#cards ~= 1) then return nil end
		local qcard = buming_card:clone()
		qcard:addSubcard(cards[1])
		qcard:setSkillName(self:objectName())
		return qcard
	end
}
	
zhengti = sgs.CreateTriggerSkill{
	name = "zhengti", 
	frequency = sgs.Skill_Compulsory,
	events = {sgs.DamageInflicted,sgs.Damaged}, 
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()--创建房间
		local damage = data:toDamage()--创建伤害房间
		local to = data:toDamage().to
		local card = data:toDamage().card
			
		if event == sgs.DamageInflicted then
			local targets=sgs.SPlayerList()
			for _,liege in sgs.qlist(room:getOtherPlayers(player)) do
				if liege:getMark("@zhengti")>0 then 
					targets:append(liege)
				end
			end
			if targets:length()>0 then 
				target = room:askForPlayerChosen(player, targets, self:objectName(),"@zhengti-choose",false,true)
				target:loseMark("@zhengti",1)
				damage.to = target
				damage.transfer = true
				room:damage(damage)
					
				return true
			end
		end
		if event == sgs.Damaged then
			if damage.from and damage.from:objectName() ~= player:objectName() then
				room:notifySkillInvoked(player, self:objectName())	

				touhou_logmessage("#TriggerSkill",player,self:objectName())
				damage.from:gainMark("@zhengti", 1)
			end
		end
	end
} 
slm009:addSkill(buming)
slm009:addSkill(zhengti)


--【为难的遗忘之物——多多良小伞】 编号：12010
slm010 = sgs.General(extension, "slm010", "slm", 3, false) 
ddlxsqingyu = sgs.CreateMasochismSkill{
        name = "ddlxsqingyu" ,
		frequency = sgs.Skill_Frequent,
        on_damaged = function(self, player, damage)
                local room = player:getRoom()
                local targets = {}
                for _, p in sgs.qlist(room:getOtherPlayers(player)) do
                        if p:getHp() >= player:getHp() then table.insert(targets, p) end
                end
                if #targets == 0 then return end

                if not player:askForSkillInvoke(self:objectName()) then return end
                for _, p in ipairs(targets) do
                        if p:canDiscard(p, "he") then
                                --for ai
								_data=sgs.QVariant()
								_data:setValue(player)
								p:setTag("qingyu_source",_data)
								--
								if not room:askForDiscard(p, self:objectName(), 1, 1, true, true, "@ddlxsqingyu-discard:" .. player:objectName()) then
                                        player:drawCards(1)
                                end
								--ai
								p:removeTag("qingyu_source")
								--
                        else
                                player:drawCards(1)
                        end
                end
        end ,
}

ddlxsguoke = sgs.CreateTriggerSkill{
        name = "ddlxsguoke" ,
        frequency = sgs.Skill_Frequent ,
        events = {sgs.CardsMoveOneTime} ,
        on_trigger = function(self, event, player, data)
                local move = data:toMoveOneTime()
                if (move.from and (move.from:objectName() == player:objectName()) and move.from_places:contains(sgs.Player_PlaceDelayedTrick))
                                and ((not move.to) or (move.to:objectName() ~= player:objectName()) or (move.to_place ~= sgs.Player_PlaceDelayedTrick)) then
                        for _, place in sgs.qlist(move.from_places) do
                                if place == sgs.Player_PlaceDelayedTrick then
                                        if player:askForSkillInvoke(self:objectName()) then
                                                local room = player:getRoom()
                                                local choice = "draw"
                                                if player:isWounded() then choice = room:askForChoice(player, self:objectName(), "draw+recover") end
                                                if choice == "draw" then
                                                        player:drawCards(2)
                                                else
                                                        local recover = sgs.RecoverStruct()
                                                        recover.who = player
                                                        room:recover(player, recover)
                                                end
											
                                        end
										
                                end
                        end
                end
                return false
        end ,
}

slm010:addSkill(ddlxsqingyu)
slm010:addSkill(ddlxsguoke)


