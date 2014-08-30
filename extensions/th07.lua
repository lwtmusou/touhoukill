module("extensions.th07", package.seeall)
extension = sgs.Package("th07")

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
--技能代码   妖妖梦
------------------------------------------------------------------------------------------------------------

--【华胥的亡灵——西行寺幽幽子】 编号：07001 revised by三国有单
yym001 = sgs.General(extension,"yym001$","yym",4,false) --07001
--[源码改动]EightDiagramSkill的invoke时没有加入data
--死蝶杀八卦时需要判断是杀还是万箭齐发以及杀的源头
--所以给源码中加入cardasked里的data
sidie = sgs.CreateTriggerSkill{
    name = "sidie",
    events = {sgs.DamageCaused},
	
    on_trigger = function(self,event,player,data)
        local room = player:getRoom()
		if event==sgs.DamageCaused then		 
			if player:getPhase() ~= sgs.Player_Play then return false end
			local damage = data:toDamage()
			if (damage.chain or damage.transfer or not damage.by_user) then return false end
			if not damage.from or not damage.to then return false end
			if damage.from:objectName() ==damage.to:objectName() then return false end
			if damage.card and damage.card:isKindOf("Slash") then
				local listt=room:getAlivePlayers()
				local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
                if damage.to:isCardLimited(slash, sgs.Card_MethodUse) then
					return false
				end
				for _,p in sgs.qlist(room:getAlivePlayers()) do
					if not damage.to:canSlash(p,slash,false) then
						listt:removeOne(p)
					end
				end
					if listt:length()>0 then
                        --for ai
						local _data=sgs.QVariant()
						_data:setValue(damage.to)
						player:setTag("sidie_target",_data)
						--end
						local target = room:askForPlayerChosen(player,listt,self:objectName(),"@sidie:"..damage.to:objectName(),true,true)
                        if target then
							touhou_logmessage("#Yishi",player,"sidie",damage.to)
							player:drawCards(2)
							slash:setSkillName("_" .. self:objectName())
							local ai_data=sgs.QVariant()
							ai_data:setValue(player)
							room:setTag("sidie_source",ai_data)
							room:useCard(sgs.CardUseStruct(slash, damage.to, target),false)
							
							return true
						end
						--for ai
						player:removeTag("sidie_target")
						--end
					end
            end
        end
	end
}

fanhun = sgs.CreateTriggerSkill{
        name = "fanhun$" ,
        events = {sgs.Damaged, sgs.FinishJudge} ,
        can_trigger = function(self, player)
                return player and player:isAlive() and (player:getKingdom() == "yym")
        end ,
        on_trigger = function(self, event, player, data)
                if (event == sgs.Damaged) then
                    local room = player:getRoom()
					local targets = sgs.SPlayerList()
					for _, p in sgs.qlist(room:getOtherPlayers(player)) do
                        if p:hasLordSkill(self:objectName()) then targets:append(p) end
					end
					while not targets:isEmpty() do
                        local target = room:askForPlayerChosen(player, targets, self:objectName(), "@@fanhun", true, true)
						if target then
                                        room:notifySkillInvoked(target, self:objectName())
										targets:removeOne(target)
                                        local _data = sgs.QVariant()
                                        _data:setValue(target)
                                        player:setTag("uuz_fanhun", _data)

                                        local judge = sgs.JudgeStruct()
                                        judge.pattern = ".|black"
                                        judge.who = player
                                        judge.reason = self:objectName()
                                        judge.good = true
                                        room:judge(judge) 

										player:setTag("uuz_fanhun", sgs.QVariant())
                        else
                            break
                        end
					end 
                else
                        local judge = data:toJudge()
                        if (judge.reason == self:objectName()) and judge:isGood() then
                                judge.who:getTag("uuz_fanhun"):toPlayer():obtainCard(judge.card)
                        end
                end
                return false
        end ,
}

yym001:addSkill(sidie)
yym001:addSkill(fanhun)

--【境界的妖怪——八云紫】 编号：07002 
yym002 = sgs.General(extension,"yym002", "yym",3,false)
aojiaofsujingjie=sgs.CreateTriggerSkill{
name="aojiaofsujingjie",
frequency = sgs.Skill_Frequent,
events={sgs.AfterDrawNCards,sgs.Damaged},
on_trigger = function(self, event, player, data)
        local room=player:getRoom()
		local num=1
        if event==sgs.AfterDrawNCards then
                num=1
        elseif event==sgs.Damaged then
               num= data:toDamage().damage	
		end		
		for i=1,num,1 do
            if not player:askForSkillInvoke(self:objectName(),data) then break end
            player:drawCards(1)
            if not player:isKongcheng() then
                  local cards=room:askForExchange(player, self:objectName(), 1, false, "Aojiaofsujingjie")
                  local id = cards:getSubcards():first()
                  player:addToPile("aojiaofsujingjie",id)
             end
       end
end
}

aojiaofsusisheng=sgs.CreateTriggerSkill{
name="aojiaofsusisheng",
events=sgs.Dying,
on_trigger=function(self,event,player,data)
        local room=player:getRoom()
        local who=room:getCurrentDyingPlayer()
		if who:isNude() then return false end
		local yukari=room:findPlayerBySkillName(self:objectName())
        local pile=yukari:getPile("aojiaofsujingjie")
		if yukari and pile:length()>0 then
			while yukari:canDiscard(who, "he") and yukari:getPile("aojiaofsujingjie"):length()>0 and who:getHp()<1 do
				if yukari:askForSkillInvoke(self:objectName(),data) then
					local ids=yukari:getPile("aojiaofsujingjie")
					room:fillAG(ids,yukari)
					local id=room:askForAG(yukari,ids,false,self:objectName())
					local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, "", nil, self:objectName(), "")
					room:clearAG(yukari)
					room:throwCard(sgs.Sanguosha:getCard(id), reason, nil)
					
					local id2=room:askForCardChosen(yukari, who, "he", self:objectName(),false,sgs.Card_MethodDiscard)
					if yukari:canDiscard(who,id2) then
						room:throwCard(id2, who,yukari)
					end
					local recover=sgs.RecoverStruct()
					recover.who=yukari
					room:recover(who,recover)
				else
					break
				end
			end
		end
end
}

aojiaofsujingdong=sgs.CreateTriggerSkill{
name="aojiaofsujingdong",
events=sgs.EventPhaseChanging,
on_trigger = function(self, event, player, data)
        local room=player:getRoom()
        local s=room:findPlayerBySkillName(self:objectName())
        if not s then return false end
        if data:toPhaseChange().to==sgs.Player_Discard then
                local pile=s:getPile("aojiaofsujingjie")
				--for ai
				local _data=sgs.QVariant()
				_data:setValue(player)
				s:setTag("jingdong_target",_data)				
				--end
				local prompt="target:"..player:objectName()
                if not pile:isEmpty() and room:askForSkillInvoke(s,self:objectName(), sgs.QVariant(prompt)) then
                        room:fillAG(pile,s)
                        local id=room:askForAG(s,pile,false,self:objectName())
                        local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, "", nil, self:objectName(), "")
                        room:throwCard(sgs.Sanguosha:getCard(id), reason, nil)
                        room:clearAG(s)
                        player:skip(sgs.Player_Discard)
                end
				--for ai
				s:removeTag("jingdong_target")
				--end
        end
end,
can_trigger=function(self,player)
        return player
end
}

yym002:addSkill(aojiaofsujingjie)
yym002:addSkill(aojiaofsusisheng)
yym002:addSkill(aojiaofsujingdong)


--【境界妖怪的式神——八云蓝】 编号：07003
yym003 = sgs.General(extension,"yym003", "yym",3,false) 
zhaoliaocard=sgs.CreateSkillCard{
name="zhaoliao",
target_fixed=true,
will_throw=false,
handling_method = sgs.Card_MethodNone ,
on_use = function(self, room, source, targets)
end
}
zhaoliaovs = sgs.CreateViewAsSkill{
name = "zhaoliao",
n =999,
view_filter = function(self, selected, to_select)
        return true
end,
view_as = function(self, cards)
        if #cards>0 then
                local card=zhaoliaocard:clone()
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
        return pattern=="@@zhaoliao"
end
}
zhaoliao=sgs.CreateTriggerSkill{
name="zhaoliao",
events=sgs.Damaged,
view_as_skill=zhaoliaovs,
on_trigger=function(self,event,player,data)
        local room=player:getRoom()
        local s=room:findPlayerBySkillName(self:objectName())
        if not s then return false end
        local a=data:toDamage().to
        if a:isAlive() and a:objectName()~=s:objectName() and not s:isNude() then
                local cards=room:askForCard(s, "@@zhaoliao", "@zhaoliao:"..a:objectName(), data, sgs.Card_MethodNone)
				if cards then
						touhou_logmessage("#ChoosePlayerWithSkill",s,"zhaoliao",player)
						--ai
						local _data = sgs.QVariant()
						_data:setValue(a)
						s:setTag("zhaoliao_target",_data)
						room:obtainCard(player,cards,false)
						
                        local choice
						local exnihilo=sgs.Sanguosha:cloneCard("ex_nihilo", sgs.Card_NoSuit, 0)
						
						if player:isKongcheng()  or player:isCardLimited(exnihilo, sgs.Card_MethodUse, true) then
                                choice="zhaoliao1"
                        else
                                choice=room:askForChoice(s,self:objectName(),"zhaoliao1+zhaoliao2")
                        end
                        if choice=="zhaoliao1" then
                                s:drawCards(1)
                        else
                                local card= room:askForExchange(player, self:objectName(), 1, false, "zhaoliaouse")
                                id = card:getSubcards():first()
                                exnihilo:addSubcard(id)
                                exnihilo:setSkillName("_zhaoliao")
                                local use=sgs.CardUseStruct()
                                use.card=exnihilo
                                use.from=player
                                room:useCard(use)
                        end				
                end
        end

end,
can_trigger=function(self,player)
        return player
end
}

jiaoxia=sgs.CreateTriggerSkill{
name="jiaoxia",
events=sgs.Dying,
on_trigger=function(self,event,player,data)
        local room=player:getRoom()
        local who=room:getCurrentDyingPlayer()
        if who:objectName()~=player:objectName() then return false end
        if player:getEquips():length()>0 then return false end
        if not player:askForSkillInvoke(self:objectName(),data) then return false end
        local judge=sgs.JudgeStruct()
        judge.reason=self:objectName()
        judge.who=player
        judge.good=false
        judge.pattern = ".|heart"
        --judge.negative = true
        room:judge(judge)
        if judge:isGood() then
                room:recover(player,sgs.RecoverStruct())
        end
		
end
}

yym003:addSkill(zhaoliao)
yym003:addSkill(jiaoxia)


--【半分虚幻的庭师——魂魄妖梦】 编号：07004
yym004 = sgs.General(extension,"yym004", "yym",4,false) --07004
--filter类做主技能，可以避免“获得技能时需要手动filterCard”这一情况
jianshu = sgs.CreateFilterSkill{
        name = "jianshu",
        view_filter = function(self, to_select)
                local room = sgs.Sanguosha:currentRoom()
                local place = room:getCardPlace(to_select:getEffectiveId())
                if place == sgs.Player_PlaceHand then
                        return to_select:isKindOf("Weapon")
                end
                return false
        end,
        view_as = function (self,card)
                local slash = sgs.Sanguosha:cloneCard("slash",card:getSuit(),card:getNumber())
                slash:setSkillName("jianshu")
                local vsc = sgs.Sanguosha:getWrappedCard(card:getId())
                vsc:takeOver(slash)
                return vsc
        end
}
jianshuTargetMod = sgs.CreateTargetModSkill{
        name = "#jianshuTargetMod",
        
		distance_limit_func = function(self, from, card)
                if from:hasSkill(self:objectName()) and card:isKindOf("Slash") and not(card:isKindOf("FireSlash") or card:isKindOf("ThunderSlash")) then
                        return 1000
                else
                        return 0
                end
        end,
        residue_func = function(self, player,card)
                if player:hasSkill(self:objectName()) and (card:isKindOf("FireSlash") or card:isKindOf("ThunderSlash"))then
                        return 1000
                end
        end,
}
jianshuWeapon = sgs.CreateTriggerSkill{
        name = "#jianshu",
        events = {sgs.CardsMoveOneTime,sgs.EventAcquireSkill},
        frequency = sgs.Skill_Compulsory,
        on_trigger = function(self,event,player,data)
            if event==sgs.CardsMoveOneTime then
				local move = data:toMoveOneTime()
                local room = player:getRoom()
				local moveweapon = sgs.CardsMoveStruct()
                local t_ids= sgs.IntList()
				if move.to and (move.to:objectName() == player:objectName()) and (move.to_place == sgs.Player_PlaceEquip) then
                        for _, id in sgs.qlist(move.card_ids) do
                                if (sgs.Sanguosha:getCard(id):isKindOf("Weapon")) then 
                                        t_ids:append(id)
										
                                end
                        end
						if t_ids:length()>0 then
							touhou_logmessage("#JianshuUninstall",player,"jianshu")
							for _,id in sgs.qlist(t_ids) do
								room:throwCard(id,player,player)
							end
						end
                end				
			else
				if data:toString() ~="jianshu" then return false end
						if player:hasSkill(self:objectName()) then
                                local room = player:getRoom()
                                local weapon = player:getCards("e")
								local weapon1=sgs.IntList()
                                for _,card in sgs.qlist(weapon) do
                                        if not card:isKindOf("Weapon") then
                                                weapon:removeOne(card)
										else
											weapon1:append(card:getId())
                                        end
                                end
								touhou_logmessage("#JianshuUninstall",player,"jianshu")
                                local move = sgs.CardsMoveStruct()
								
                                move.card_ids = weapon1
                                move.to_place = sgs.Player_DiscardPile
                                room:moveCardsAtomic(move,true)
                        end
			end
        end
}
jianshu_clearlimit=sgs.CreateTriggerSkill{--修正先出属性杀就出不了普通杀的问题
name="#jianshu_clearlimit",
events=sgs.PreCardUsed,--配合虚史 须用precarduse
on_trigger=function(self,event,player,data)
        local card =data:toCardUse().card 
        local room=player:getRoom()
		if  card:isKindOf("Slash") and (not(card:isKindOf("FireSlash") or card:isKindOf("ThunderSlash")))then                 
           player:setFlags("jianshu_slash")
		end
		if (not player:hasFlag("jianshu_slash")) and (card:isKindOf("FireSlash") or card:isKindOf("ThunderSlash"))then                 
            room:addPlayerHistory(player, data:toCardUse().card:getClassName(), -1)	
		end
end
}

louguan = sgs.CreateTriggerSkill{
        name = "louguan",
        frequency = sgs.Skill_Compulsory,
        events = sgs.TargetConfirmed,
        on_trigger = function(self,event,player,data)
                local use = data:toCardUse()
                local room = player:getRoom()
                if event == sgs.TargetConfirmed then
                        if use.card and use.from:objectName() == player:objectName()and use.card:isKindOf("Slash") and not use.card:isRed() then
                                for _, p in sgs.qlist(use.to) do
                                        p:addQinggangTag(use.card)
                                end

							touhou_logmessage("#TriggerSkill",player,"louguan")
							room:notifySkillInvoked(player, self:objectName())
							room:setEmotion(player, "weapon/qinggang_sword")
							
                        end
                end
        end,
}

bailou = sgs.CreateTriggerSkill{
        name = "bailou",
        events = sgs.TargetConfirmed,
        on_trigger = function(self,event,player,data)
                local use = data:toCardUse()
                local room = player:getRoom()
                if use.from:objectName() == player:objectName() and use.card and use.card:isKindOf("Slash") and not use.card:isBlack() then
                        for _, p in sgs.qlist(use.to) do
                                if p:isKongcheng() then return false end
                                --for AI and UI
                                        local _data = sgs.QVariant()
                                        _data:setValue(p)
                                --end
                                if room:askForSkillInvoke(player,self:objectName(), _data)then
                                    room:setEmotion(player, "weapon/ice_sword")
									room:throwCard(room:askForCardChosen(player,p,"h",self:objectName(),false, sgs.Card_MethodDiscard),p,player)
									
								end
                        end

                end
        end,
}

yym004:addSkill(jianshu)
yym004:addSkill(jianshuWeapon)
yym004:addSkill(jianshuTargetMod)
yym004:addSkill(jianshu_clearlimit)
extension:insertRelatedSkills("jianshu","#jianshu")
extension:insertRelatedSkills("jianshu","#jianshuTargetMod")
extension:insertRelatedSkills("jianshu","#jianshu_clearlimit")
yym004:addSkill(louguan)
yym004:addSkill(bailou)

--【骚灵乐团——普莉兹姆利巴三姐妹】  07005
yym005 = sgs.General(extension,"yym005", "yym",3,false) 
hezou=sgs.CreateTriggerSkill{--空技能
	name="hezou",
	frequency = sgs.Skill_Eternal,
	events={sgs.EventPhaseStart,sgs.Damaged},

	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
	
	end
}
yym005:addSkill(hezou)


--【七色的人偶使——爱丽丝•玛格特罗依德】 07006
yym006 = sgs.General(extension,"yym006", "yym",4,false) 
--[源码改动]moveCardsToEndOfDrawpile对付位于处理区的牌，不给力。。。
--还是修改了room:askforguanxing()  sanguosha.i来实现  通过增加一个参数 skillname
local function renou_shuffle(atable)
	local count = #atable
	local list=sgs.IntList()
	for i = 1, count do
		local j = math.random(1, count)
		atable[j], atable[i] = atable[i], atable[j]
	end
	for _,s in pairs(atable) do
		list:append(s)
	end
	return list
end
renou=sgs.CreateTriggerSkill{
	name="renou",
	events={sgs.EventPhaseStart},
	can_trigger=function(self,player)
		return player   
	end,	
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()	
		local current = room:getCurrent()
		if event == sgs.EventPhaseStart and(current:getPhase() == sgs.Player_Finish) then
			local source = room:findPlayerBySkillName(self:objectName())
			if source and source:getEquips():length() < 4  then
				if not  room:askForSkillInvoke(source,self:objectName(), data)then return false end
                room:notifySkillInvoked(source, self:objectName())
                room:broadcastSkillInvoke(self:objectName())
                local list=room:getNCards(5)
				local able = sgs.IntList()
				local disabled=sgs.IntList()
                for _,id in sgs.qlist(list) do
					local tmp_card=sgs.Sanguosha:getCard(id)
					if tmp_card:isKindOf("EquipCard") then
						local tmp_equip = tmp_card:getRealCard():toEquipCard()
						if source:getEquip(tmp_equip:location()) then
							disabled:append(id)
						else
							able:append(id)
						end
					else
						disabled:append(id)
					end
                end
				--翻开
				room:fillAG(list, nil, disabled)
                local cardInfo = {}
                for _,c in sgs.qlist(list) do
                    table.insert(cardInfo, sgs.Sanguosha:getCard(c):toString())
                end
                local mes=sgs.LogMessage()
                mes.type="$TurnOver"
                mes.from=source
                mes.card_str=table.concat(cardInfo, "+")
                room:sendLog(mes)
				--开始选择
				local equipid =-1
                if not able:isEmpty() then 
					equipid=room:askForAG(source, able, true, self:objectName())
                end
				if equipid ~=-1 then
					local move = sgs.CardsMoveStruct(equipid, source, sgs.Player_PlaceEquip,
                                sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_PUT, source:objectName()))	
				
					room:moveCardsAtomic(move, true)
					local mes2=sgs.LogMessage()
					mes2.type="$renou_put"
					mes2.from=source
					mes2.card_str=sgs.Sanguosha:getCard(equipid):toString()
					room:sendLog(mes2)
				end
				local putdown = {}
				local putdownInfo = {}
				for _,id in sgs.qlist(list) do
					if id  ~= equipid then
						table.insert(putdown,id)
						table.insert(putdownInfo,sgs.Sanguosha:getCard(id):toString())
					end
				end 
                local endList =renou_shuffle(putdown)
				--没有进入场上的牌  moveCardsToEndOfDrawpile不了？？
				--最后只好依靠观星。。。
				room:askForGuanxing(source,endList,-1,self:objectName()) --,1
				--touhou_logmessage("#mofa_damage",damage.from,damage.damage+1,damage.to,damage.damage)
				local mes1=sgs.LogMessage()
                mes1.type="$renou_movedown"
                mes1.from=source
				mes1.arg=tostring(endList:length())
                mes1.card_str=table.concat(putdownInfo, "+")
                room:sendLog(mes1)
				--最后关闭和弃置
				room:getThread():delay(1000)--没有delay ag可能一闪而过。。。
				room:clearAG()
			end
		end
	end
}
junshi = sgs.CreateViewAsSkill{
        name = "junshi",
        n = 1,
        view_filter = function(self,selected,to_select)
            return to_select:isKindOf("EquipCard")
		end,
		
        view_as = function(self,cards)
            if #cards ~=1 then return false end
			local usereason = sgs.Sanguosha:getCurrentCardUseReason()
			local pattern = sgs.Sanguosha:getCurrentCardUsePattern()
			if (pattern == "jink") then
				local card = sgs.Sanguosha:cloneCard("jink",cards[1]:getSuit(),cards[1]:getNumber())
				card:addSubcard(cards[1])
				card:setSkillName(self:objectName())
				return card
			else
				local card = sgs.Sanguosha:cloneCard("slash",cards[1]:getSuit(),cards[1]:getNumber())
				card:addSubcard(cards[1])
				card:setSkillName(self:objectName())
				return card
			end
        end,

		enabled_at_play = function(self, target)
            return sgs.Slash_IsAvailable(target)
        end,
        enabled_at_response = function(self, target, pattern)
		   return  (pattern == "slash") or (pattern == "jink")
		end
}

yym006:addSkill(renou)
yym006:addSkill(junshi)


--【凶兆的黑猫——橙】 编号：07007  --by三国有单
yym007 = sgs.General(extension,"yym007", "yym",3,false) 
shishen=sgs.CreateTriggerSkill{
	name="shishen",
	events={sgs.EventPhaseStart,sgs.Damaged},

	on_trigger=function(self,event,player,data)
		local room = player:getRoom()	
		if ( event ==sgs.EventPhaseStart and player:getPhase() == sgs.Player_Start) or (event == sgs.Damaged) then
			if player:getMark("@shi") >0 then
				if event== sgs.Damaged then
					--ai
					player:setFlags("shishen_choice")
				end
				local choice=room:askForChoice(player,self:objectName(),"shishen1+cancel")
				player:setFlags("-shishen_choice")
				if choice=="shishen1" then 
					player:loseMark("@shi")
				end
			end	
		end
		if event ==sgs.EventPhaseStart and (player:getPhase() == sgs.Player_Play) and player:getMark("@shi") ==0 then
			if player:getMark("@shi")>0 then return false end
			local choice=room:askForChoice(player,self:objectName(),"shishen2+cancel")
			if choice=="shishen2" then 
				player:gainMark("@shi", 1)
			end
		end
	end
}

yexing = sgs.CreateTriggerSkill{
    name = "yexing",
    events = {sgs.GameStart,sgs.PreMarkChange,sgs.CardEffected,sgs.SlashEffected,sgs.EventAcquireSkill,sgs.EventLoseSkill},
	frequency = sgs.Skill_Compulsory,
    on_trigger=function(self, event, player, data)
		local room = player:getRoom()
		if event == sgs.GameStart  then --获得技能时 zun 依姬
			--or (event == sgs.EventAcquireSkill and data:toString() == self:objectName()) 
			room:setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false)
		end
		--if (event == sgs.EventLoseSkill and data:toString() == self:objectName()) then 
		--	room:removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0")
		--end
        if event==sgs.PreMarkChange then
			local change = data:toMarkChange()
            if change.name ~= "@shi" then return false end
            local mark = player:getMark("@shi")
            local room = player:getRoom()   
			if (mark > 0) and (mark + change.num == 0) then  
                    room:setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false)
			elseif (mark == 0) and (mark + change.num > 0) then
                    room:removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0")
			end
            return false
		end
		if event==sgs.SlashEffected then
			--[[strings =player:getTag("Qinggang"):toStringList() --确认青冈
				qinggang=false
				for _,str in pairs(strings) do
					if str ==effect.slash:toString() then
						qinggang =true
						break
					end
				end]]
			
			local effect = data:toSlashEffect()
            if (effect.nature == sgs.DamageStruct_Normal  and  effect.slash:getSkillName()~="aoyi")  then
				 
				if player:getMark("@shi")==0 then
					touhou_logmessage("#DanlaoAvoid",effect.to,effect.slash:objectName(),nil,self:objectName())
					room:notifySkillInvoked(player, self:objectName())
					return true
				end
			end
		end
		if (event == sgs.CardEffected) then
           local  effect = data:toCardEffect()
            if (effect.card:isKindOf("SavageAssault") or effect.card:isKindOf("ArcheryAttack")) then
				if player:getMark("@shi")==0 then
					touhou_logmessage("#DanlaoAvoid",effect.to,effect.card:objectName(),nil,self:objectName())
					room:notifySkillInvoked(player, self:objectName())
					return true
				end
			end
		end
	end,
}
yexing_attackRange = sgs.CreateAttackRangeSkill{
    name = "#yexing_atteckRange",
	extra_func = function(self,from,include)
        if from:hasSkill("#yexing_atteckRange") and from:getMark("@shi")==0 then
            return 1 
        end
    end
}

yaoshu_trick_vs = sgs.CreateViewAsSkill{ 
	name = "yaoshu" ,
	n = 0 ,
	view_as = function(self, cards)
		local card,cardname
		local id=sgs.Self:getMark("yaoshumark")-1
		if id~=-1 then
			cardname=sgs.Sanguosha:getCard(id):objectName()
		end
		card=sgs.Sanguosha:cloneCard(cardname, sgs.Card_NoSuit, 0)
		
		card:setSkillName("yaoshu")
		return card
	end ,
	enabled_at_play = function(self, player)
		return false
	end ,
	enabled_at_response = function(self, player, pattern)
		return pattern == "@@yaoshu"
	end ,
}
yaoshu=sgs.CreateTriggerSkill{
	name="yaoshu",
	events={sgs.CardFinished},
	view_as_skill=yaoshu_trick_vs,
	
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local use = data:toCardUse()
		if event ==sgs.CardFinished then
			
			if use.card and use.card:isNDTrick() and (not use.card:isKindOf("Nullification")) then
				if use.card:getSkillName()=="yaoshu" then
					room:setPlayerMark(player,"yaoshumark",0)
					return false					
				end
				
				--使用的是手牌的锦囊 0牌的虚拟牌不可
				--
				if use.card:isVirtualCard() and use.card:getSubcards():length()==0 then return false end
				--决斗中途可能会导致失去标记 检测是否可以使用为好
				
				if player:isCardLimited(sgs.Sanguosha:cloneCard(use.card:objectName(), sgs.Card_NoSuit, 0), sgs.Card_MethodUse) then
					return false
				end
				--标准包和军争包的锦囊的id
				--id号或者字符串可以用tag来记录？？
				for id=0,159,1 do
					local card=sgs.Sanguosha:getCard(id)
					if card:objectName()==use.card:objectName() then
						
						room:setPlayerMark(player,"yaoshumark",id+1)
						break
					end
				end				
				if not room:askForUseCard(player,"@@yaoshu","@yaoshu:"..use.card:objectName()) then
				
					room:askForUseCard(player,"@@yaoshu","@yaoshu:"..use.card:objectName())
					room:setPlayerMark(player,"yaoshumark",0)
					return false
				end			
			end
		end		
	end
}
yym007:addSkill(shishen)
yym007:addSkill(yexing)
yym007:addSkill(yexing_attackRange)
yym007:addSkill(yaoshu) 
extension:insertRelatedSkills("yexing", "#yexing_attackRange")  
--关联skill导致凭依失败？？？

 
 --【冬季的遗忘之物——蕾迪•霍瓦特罗克】 编号：07008
yym008 = sgs.General(extension,"yym008", "yym",4,false) --07008
--[[jiyi = sgs.CreateTriggerSkill{
        name = "jiyi",
        events = {sgs.TurnedOver},
        on_trigger = function(self, event, player, data)
                local room=player:getRoom()
				if player:askForSkillInvoke(self:objectName(), data) then
                        local room = player:getRoom()
                        room:broadcastSkillInvoke(self:objectName())
                        local _player = sgs.SPlayerList()
                        _player:append(player)
                        local cards = room:getNCards(2, false)
                        local move = sgs.CardsMoveStruct(cards, nil, player, sgs.Player_PlaceTable, sgs.Player_PlaceHand,
                                        sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_PREVIEW, player:objectName(), self:objectName(), ""))
                        local moves = sgs.CardsMoveList()
                        moves:append(move)
                        room:notifyMoveCards(true, moves, false, _player)
                        room:notifyMoveCards(false, moves, false, _player)
                        local list = sgs.IntList()
                        for _, id in sgs.qlist(cards) do
                                list:append(id)
                        end
						--askforyiji实质是牌堆里摸排 
                        while (room:askForYiji(player, cards, self:objectName(), true, false, true, -1, room:getAlivePlayers())) do
                                local move_list = sgs.IntList()
                                for _, id in sgs.qlist(list) do
                                        if room:getCardPlace(id) ~= sgs.Player_DrawPile then
                                                move_list:append(id)
                                                cards:removeOne(id)
                                        end
                                end
                                local move1=sgs.CardsMoveStruct(move_list, player, nil, sgs.Player_PlaceHand, sgs.Player_PlaceTable,
                                                sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_PREVIEW, player:objectName(), self:objectName(), ""))
                                local list = sgs.IntList()
                                for _, id in sgs.qlist(cards) do
                                        list:append(id)
                                end
                                local moves2 = sgs.CardsMoveList()
                                moves2:append(move1)
                                room:notifyMoveCards(true, moves2, false, _player)
                                room:notifyMoveCards(false, moves2, false, _player)
                                if not player:isAlive() then return false end
                        end
                        if not cards:isEmpty() then
                                local move3 = sgs.CardsMoveStruct(cards, player, nil, sgs.Player_PlaceHand, sgs.Player_PlaceTable,
                                                sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_PREVIEW, player:objectName(), self:objectName(), ""))
                                local moves3 = sgs.CardsMoveList()
                                moves3:append(move3)
                                room:notifyMoveCards(true, moves3, false, _player)
                                room:notifyMoveCards(false, moves3, false, _player)
                                for _, id in sgs.qlist(cards) do
                                    room:obtainCard(player, id, false)
                                end

                        end
                end
				
                return false
        end
}]]
jiyi = sgs.CreateTriggerSkill{
        name = "jiyi",
        events = {sgs.TurnedOver},
        on_trigger = function(self, event, player, data)
                local room=player:getRoom()
				if player:askForSkillInvoke(self:objectName(), data) then
                        local room = player:getRoom()
                        room:broadcastSkillInvoke(self:objectName())
                        player:drawCards(2)
						room:askForRende(player, player:handCards(), self:objectName(), false, true, math.min(2, player:getHandcardNum()))
				end		
                return false
        end
}
chunmian=sgs.CreateTriggerSkill{
name="chunmian",
events=sgs.EventPhaseStart,
frequency=sgs.Skill_Compulsory,
on_trigger = function(self, event, player, data)
        if player:getPhase()==sgs.Player_Finish then
                local room=player:getRoom()		
				touhou_logmessage("#TriggerSkill",player,self:objectName())
				room:notifySkillInvoked(player, self:objectName())
                player:turnOver()
				
        end
end
}
yym008:addSkill(jiyi)
yym008:addSkill(chunmian)


--【带来春天的妖精——莉莉白】 编号：07009  
yym009 = sgs.General(extension,"yym009", "yym",3,false) --07009
baochun = sgs.CreateTriggerSkill{
        name = "baochun",
        --frequency = sgs.Skill_NotFrequent,
        events = {sgs.Damaged},

        on_trigger = function (self,event,player,data)
                local room = player:getRoom()
                if not player:isAlive() then return false end
                
                    local target = room:askForPlayerChosen(player, room:getAlivePlayers(),self:objectName(),"@"..self:objectName()..":"..tostring(player:getLostHp()), true,true)
                    if target then
						target:drawCards(player:getLostHp())
					end
					
        end
}

chunyi = sgs.CreateTriggerSkill{
        name = "chunyi",
        frequency = sgs.Skill_Compulsory,
        events = {sgs.EventPhaseStart},

        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                if event == sgs.EventPhaseStart then
                        if player:getPhase()==sgs.Player_Start then
                                if player:getMaxHp() < 6 then 
								room:setPlayerProperty(player, "maxhp", sgs.QVariant(player:getMaxHp()+1))
								touhou_logmessage("#TriggerSkill",player,self:objectName())
								touhou_logmessage("#GainMaxHp",player,1)
								touhou_logmessage("#GetHp",player,player:getHp(),nil,player:getMaxHp())
								room:notifySkillInvoked(player, self:objectName())
									
                                end
                        end
                end
        end
}

yym009:addSkill(baochun)
yym009:addSkill(chunyi)

--【人偶使的手足——上海人形】 编号：07010
yym010 = sgs.General(extension,"yym010", "yym",3,false) --07010
shrxzhancao = sgs.CreateTriggerSkill{
        name = "shrxzhancao" ,
        events = {sgs.TargetConfirming, sgs.SlashEffected} ,
        can_trigger = function(self, target)
                return target and target:isAlive()
        end ,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()                
				if event == sgs.TargetConfirming then
                        local use = data:toCardUse()
                        local shrx = room:findPlayerBySkillName(self:objectName())
                        if shrx and shrx:inMyAttackRange(player) and use.card and use.card:isKindOf("Slash") then
                                --for UI --for AI
                                        local _data = sgs.QVariant()
                                        _data:setValue(player)
                                        shrx:setTag("shrxzhancao_carduse", data)
										shrx:setTag("shrxzhancao_target", _data)
                                --end
								local prompt="target:"..use.from:objectName()..":"..player:objectName()
                                if room:askForSkillInvoke(shrx,self:objectName(), sgs.QVariant(prompt)) then
                                        --修改flag设置，因为有使用杀的插入
										room:setCardFlag(use.card, "shrxzhancao"..player:objectName())
                                        if not room:askForCard(shrx, ".Equip", "@shrxzhancao-discard") then
                                                room:loseHp(shrx)
                                        end
									
                                end
                                --for AI
                                        shrx:removeTag("shrxzhancao_carduse")
                                --end
							
                        end
                elseif event == sgs.SlashEffected then
                        local effect = data:toSlashEffect()
                        if effect.slash and effect.slash:hasFlag("shrxzhancao"..effect.to:objectName()) then
								touhou_logmessage("#DanlaoAvoid",effect.to,effect.slash:objectName(),nil,self:objectName())
								return true
                        end
                end
                return false
        end ,
}

shrxmocaoCard = sgs.CreateSkillCard{
        name = "shrxmocao" ,
        filter = function(self, targets, to_select,player)
                return (#targets == 0) and (not to_select:getEquips():isEmpty()) and to_select:objectName() ~= player:objectName()
        end ,
        on_effect = function(self, effect)
                local room = effect.to:getRoom()
                local card_id = room:askForCardChosen(effect.from, effect.to, "e", self:objectName())
                room:obtainCard(effect.from, card_id)
                if effect.to:isWounded() then
                        effect.to:drawCards(effect.to:getLostHp())
                end
				
        end ,
}
shrxmocao = sgs.CreateZeroCardViewAsSkill{
        name = "shrxmocao" ,
        view_as = function()
                return shrxmocaoCard:clone()
        end ,
        enabled_at_play = function(self, player)
                return not player:hasUsed("#shrxmocao")
        end ,
}

yym010:addSkill(shrxzhancao)
yym010:addSkill(shrxmocao)


--【千年前的妖怪——八云紫】 编号：07011 --by三国有单
yym011 = sgs.General(extension,"yym011", "yym",4,false) --07011
shenyin = sgs.CreateTriggerSkill{
		name = "shenyin", 
		events = {sgs.DamageCaused},  
		on_trigger = function(self, event, player, data)
			local room = player:getRoom()
			local damage = data:toDamage()
			local to = data:toDamage().to
			local card = data:toDamage().card
			
			if  not to:isNude() then  
				local _data = sgs.QVariant()
                _data:setValue(to)
				if (room:askForSkillInvoke(player,self:objectName(),_data)) then 
					local to_throw = room:askForCardChosen(player, to, "he", self:objectName())
					player:addToPile("yin_mark", to_throw)
					if not to:isNude() then 
						local to_throw1 = room:askForCardChosen(player, to, "he", self:objectName())
						player:addToPile("yin_mark", to_throw1)
					end
					return true
				end
			end
		end
}

xijian=sgs.CreateTriggerSkill{
	name="xijian",
	events={sgs.EventPhaseStart,sgs.Damaged},
	can_trigger = function(self, player)
        return player 
    end,
	
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local use = data:toCardUse()
		local current = room:getCurrent()
		local can_invoke=false
		if event == sgs.EventPhaseStart and(current:getPhase() == sgs.Player_Finish)  then
			if player:objectName()~=current:objectName() then return false end	
			can_invoke=true
		end	
		if event == sgs.Damaged  then
			local damage=data:toDamage()
			if (not damage.to) or (not damage.to:isAlive()) then return false end
			current=damage.to
			can_invoke=true
		end	
		local yukari = room:findPlayerBySkillName(self:objectName())	
		if not yukari  then return false end
		
		if not 	can_invoke then return false end
		local plist =	room:getAlivePlayers()--检测可指定的合法目标
		for _,liege in  sgs.list(room:getAlivePlayers()) do
			num	=0				
			for _,pile in pairs(liege:getPileNames()) do
				if pile=="suoding_cards" or pile=="jiejie_right" or pile=="jiejie_left" or pile=="renou" or pile=="meng_list" 
					or pile=="meng0" or pile=="meng1" or pile=="meng2" or pile=="meng3" 
					then --不能获取结界牌和锁定牌 暂存的“？”和人偶
					else
						num =num + liege:getPile(pile):length()
				end
			end
			if num==0 then plist:removeOne(liege) end
		end	
			--for ai
			local _data=sgs.QVariant()
			_data:setValue(current)
			yukari:setTag("xijian_target",_data)
			--end
			if plist:length()>0 then 
				local player_haspile = room:askForPlayerChosen(yukari, plist , "xijian","@xijian:"..current:objectName(),true,true)
				
				if player_haspile then
					local pilenames = player_haspile:getPileNames() 
					local idlist = sgs.IntList()	
						for _,pile in pairs(pilenames) do
						if pile=="suoding_cards" or pile=="jiejie_right" or pile=="jiejie_left" or pile=="renou" or pile=="meng_list" 
							or pile=="meng0" or pile=="meng1" or pile=="meng2" or pile=="meng3" 
							then --不能获取结界牌和锁定牌
						else
							local ids = player_haspile:getPile(pile)
							for _,id in sgs.qlist(ids) do
								idlist:append(id)
							end
						end	
					end
					
					room:fillAG(idlist, yukari)
					local card_id = room:askForAG(yukari, idlist, true, self:objectName())
					room:clearAG(yukari)
					local can_open=false
					local pile_name
					for _,p in pairs(pilenames) do
						local ids = player_haspile:getPile(p)
						if ids:contains(card_id) then
							pile_name=p
							break
						end
					end
					if pile_name and player_haspile:pileOpen(pile_name,yukari:objectName())then
						can_open=true
					end
					room:obtainCard(current,card_id,can_open)
				end
			end
			--for ai
			yukari:removeTag("xijian_target")
			--end
	end
}

yym011:addSkill(shenyin)
yym011:addSkill(xijian)

 --【千年前的舞姬——西行寺幽幽子】 编号：07012 
yym012 = sgs.General(extension,"yym012", "yym",3,false) --07012
--[源码改动]为了死灵牌可见 （没有进入自己手牌而加入pile）
--改动bool Room::notifyMoveCards(bool isLostPhase, QList<CardsMoveStruct> cards_moves, bool forceVisible, QList<ServerPlayer *> players)   
youqu=sgs.CreateTriggerSkill{
name="youqu",
events=sgs.EventPhaseStart,
frequency=sgs.Skill_Compulsory,
on_trigger = function(self, event, player, data)
        local room=player:getRoom()
        local phase=player:getPhase()
        
        if phase==sgs.Player_Start then
                local sl=player:getPile("siling")
                if sl:length()>=2 then
                        room:notifySkillInvoked(player, self:objectName())
                        room:broadcastSkillInvoke(self:objectName())

						touhou_logmessage("#TriggerSkill",player,self:objectName())
                        local move=sgs.CardsMoveStruct()
                        move.card_ids=sl
                        move.to_place=sgs.Player_PlaceHand
                        move.to=player
                        room:moveCardsAtomic(move, true)

						touhou_logmessage("#silinggain",player,self:objectName(),nil,sl:length())
                        
                        local damage=sgs.DamageStruct()
                        damage.from=player
                        damage.to=player
                        room:damage(damage)

                end
        elseif phase==sgs.Player_Finish then
                room:notifySkillInvoked(player, self:objectName())
                room:broadcastSkillInvoke(self:objectName())
                touhou_logmessage("#TriggerSkill",player,self:objectName())
                local choice=room:askForChoice(player,self:objectName(),"siling1+siling2+siling3")
                local list
                if choice=="siling1" then
						list=room:getNCards(1)
                elseif choice=="siling2" then
						list=room:getNCards(2)
                else
						list=room:getNCards(3)
                end
				--源码中的改动 move.reason默认为“siling”
				local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_UNKNOWN, "", nil, "siling", "") 
				player:addToPile("siling",list,false,reason)
        end
end
}

wangwu=sgs.CreateTriggerSkill{
name="wangwu",
events=sgs.TargetConfirming,
on_trigger = function(self, event, player, data)
        local use=data:toCardUse()
        if use.card and (use.card:isKindOf("Slash") or use.card:isNDTrick()) then
                if not use.card:isRed() and not use.card:isBlack() then return false end
				if use.to:contains(player) then
                        local list=player:getPile("siling")
                        if not list:isEmpty() and use.from:isAlive() then
                                local _data = sgs.QVariant()
								_data:setValue(use.from)
								player:setTag("wangwu_target",_data)
								local prompt="invoke:"..use.from:objectName()..":"..use.card:objectName()
								if player:askForSkillInvoke(self:objectName(),sgs.QVariant(prompt)) then
                                        --没有可发动的亡舞牌 也应该询问，保证技能威慑
										--如果跳过询问，别人就知道其没有这一类颜色牌了
										local same=sgs.IntList()
										local disabled=sgs.IntList()
                                        for _,id in sgs.qlist(list) do
                                                if sgs.Sanguosha:getCard(id):sameColorWith(use.card) then
                                                        same:append(id)
                                                else
													disabled:append(id)
												end
                                        end
                                        local room=player:getRoom()
                                        room:fillAG(list,player, disabled)
                                        --for ai
										local _data=sgs.QVariant()
										_data:setValue(use.card)
										player:setTag("wangwu_card",_data)
										--end
										if same:isEmpty() then
												--list
                                                room:askForAG(player, same, true, self:objectName())
                                                room:clearAG(player)
                                        else
                                                local id = room:askForAG(player, same, true, self:objectName())
                                                room:clearAG(player)--有五谷插入 必须player关闭
                                                if id>-1 then
                                                        local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, "", nil, self:objectName(), "")
                                                        room:throwCard(sgs.Sanguosha:getCard(id), reason, nil)
                                                        player:drawCards(1)
														local damage=sgs.DamageStruct()
                                                        damage.from=player
                                                        damage.to=use.from
                                                        room:damage(damage)
				
                                                end
                                        end
										--for ai
										player:removeTag("wangwu_card")
										--end
                                end
                        end
                end
        end
end
}

yym012:addSkill(youqu)
yym012:addSkill(wangwu)


--【死欲的半灵——魂魄妖梦】  编号：07013
yym013 = sgs.General(extension,"yym013", "yym",2,false) 
function touhou_siyu_clear(player)--死欲中断时，清除各种必须立刻清除的flag mark tag
	--处理四大类型
	--1 标记类mark
	--2禁止卡牌使用
	--3 flag
	--4因为终止结算 而没有进入弃牌堆的牌
	--还需要手动清除cardflag么？？？
	local room=player:getRoom()
	
	--清除死欲中止五谷的情况 死欲+亡舞
	room:clearAG()
	local dummy = sgs.Sanguosha:cloneCard("jink", sgs.Card_NoSuit, 0)
    local num=0
	local ag_list = room:getTag("AmazingGrace"):toIntList()
	for _, c in sgs.qlist(ag_list) do
        dummy:addSubcard(c)
		num=num+1
    end
    if num>0 then --否则log会比较奇怪。。。
		room:throwCard(dummy, sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_NATURAL_ENTER, "", "amazing_grace", ""), nil)
    end
	--清除因为cardfinish未触发，相应的牌没有进入弃牌堆
	for _,id in sgs.qlist(sgs.Sanguosha:getRandomCards())  do
         if (room:getCardPlace(id) == sgs.Player_PlaceTable)then  --getCardPlace(id) == Player::PlaceJudge
            room:moveCardTo(sgs.Sanguosha:getCard(id), nil, sgs.Player_DiscardPile, true)
         end
		 if (sgs.Sanguosha:getCard(id):hasFlag("using")) then
              room:setCardFlag(sgs.Sanguosha:getCard(id), "-using")
		end
	end
    
	
	--只考虑【劝诫】没考虑其他技能禁止使用杀
	local current=room:getCurrent()
	if current:getMark("ymsndquanjie")>0 then
		room:setPlayerMark(current, "ymsndquanjie",0)
		room:removePlayerCardLimitation(current, "use", "Slash$1")
	end
	--断罪中断怎么弄。。。
	
	--额外回合标记
	for _,p in sgs.qlist(room:getAlivePlayers()) do
		
		--flag之外，mark需要独立清除
		p:clearFlags()
		if p:getMark("@duanzui-extra")>0 then
			p:loseMark("@duanzui-extra")
		end
		--各种zun的used mark...
		--其实强行trigger了一个phasechange之后，许多changing触发的clear技能的mark大多数应该能消除
		local marks={"spadecount","baoyi","chuangshi","xinshang_effect","shituDamage","shituDeath","zheshetransfer",
		"touhou-extra","@qianxi_red","@qianxi_black","sizhai","@qingting"}
		
		for _,a in pairs (marks) do
			room:setPlayerMark(p, a, 0)
		end
		--通过trigger 已经触发过戏画limit_clear
		
		--神灵梦封印伤害插入 会不会影响什么呢？？
		
	end
end
hpymsiyu = sgs.CreateTriggerSkill{
        name = "hpymsiyu",
        events = {sgs.EventPhaseEnd, sgs.PostHpReduced} ,
        frequency = sgs.Skill_Compulsory ,
        on_trigger = function(self, event, player, data)
                local room=player:getRoom()
				if event == sgs.PostHpReduced then
                        if player:getHp() < 1 then
                                local room = player:getRoom()
                                if not player:faceUp() then player:turnOver() end
                                local dummy = sgs.Sanguosha:cloneCard("jink", sgs.Card_NoSuit, 0)
                                
								for _, c in sgs.qlist(player:getJudgingArea()) do
                                        dummy:addSubcard(c)
										
                                end
                                if dummy:getSubcards():length()>0 then --否则log会比较奇怪。。。
								room:throwCard(dummy, sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_DISCARD, player:objectName()), player)
                                end
								player:addMark("siyuinvoke")
								touhou_logmessage("#TriggerSkill",player,self:objectName())
								room:notifySkillInvoked(player, self:objectName())
								
								--手动触发一些回合内外产生变化的技能
								
								local current = room:getCurrent()
								--[[qdata=sgs.QVariant()
								local change = sgs.PhaseChangeStruct()
								change.from=current:getPhase()
								current:setPhase(sgs.Player_NotActive)--需要设置为回合外 否则会影响一些技能 如【永恒】
								change.to=sgs.Player_NotActive
								qdata:setValue(change)
								local thread=room:getThread()
								--额外回合标记
								thread:trigger(sgs.EventPhaseChanging, room, current, qdata)--是否应该每个人都触发一遍？
								]]
								touhou_logmessage("#touhouExtraTurn",player,self:objectName())
								current:changePhase(current:getPhase(), sgs.Player_NotActive)
								
								touhou_siyu_clear(player)
								
								player:gainAnExtraTurn()
                                room:throwEvent(sgs.TurnBroken)
                                return true
                        end
                elseif player:getPhase() == sgs.Player_Play then
                        if player:getMark("siyuinvoke") > 0 then
                                player:removeMark("siyuinvoke")
                                if player:getHp() < 1 then player:getRoom():enterDying(player, nil) end	
                        end
                end
                return false
        end ,
}

juhe = sgs.CreateTriggerSkill{
        name = "juhe",
        events = {sgs.DrawNCards,sgs.AfterDrawNCards},
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if event == sgs.DrawNCards then
                        if room:askForSkillInvoke(player, "juhe", data) then
								local count = data:toInt() + 3
                                data:setValue(count)
                                room:setPlayerFlag(player,"juheUsed")
								
						end
                elseif event == sgs.AfterDrawNCards and player:hasFlag("juheUsed") and player:getHp()>0 then
                        room:askForDiscard(player,self:objectName(),player:getHp(),player:getHp(),false,false,"juhe_discard:"..tostring(player:getHp()))
                end
        end
}

yym013:addSkill(hpymsiyu)
yym013:addSkill(juhe)



