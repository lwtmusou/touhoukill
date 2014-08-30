module("extensions.thndj", package.seeall)
extension = sgs.Package("thndj")
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
--技能代码   年代记 
------------------------------------------------------------------------------------------------------------
--【炎杀姬——藤原妹红】 ndj001
ndj001 = sgs.General(extension,"ndj001$","zhu",4,false)
--[源码改动]死斗弃置时对于武器牌的检测  修改Player:canDiscard() 
--麻痹，为什么player.cpp，player.h增加参数弃置动作的reason编译后能通过
--在lua中用这个参数却说arguments错误。。。我擦！  （应该是还要修改sanguosha.i）
--代替方案:给player一个mark取代reason，我不加新参数了还不行么
rexue_count = sgs.CreateTriggerSkill{
        name = "#rexue_count" ,
        events = {sgs.Death, sgs.EventPhaseStart} ,
        can_trigger = function(self, player)
                return player
        end ,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if event == sgs.Death then
                        local death = data:toDeath()
                        if (death.who:objectName() ~= player:objectName()) then return false end
                        
						local current = room:getCurrent()
						if not current:hasSkill("rexue") then return false end
                        if current then
							room:setTag("rexue_count", sgs.QVariant(false))
                        end
                elseif player:getPhase() == sgs.Player_Start  and player:hasSkill("rexue") then--记录清零
                        room:setTag("rexue_count", sgs.QVariant(true))
                end
                return false
        end ,
}
rexue = sgs.CreateTriggerSkill{
        name = "rexue",
		frequency = sgs.Skill_Compulsory,
        events = sgs.EventPhaseChanging,
        on_trigger = function(self,event,player,data)
            local room = player:getRoom()
			local change = data:toPhaseChange()
            if (change.to ~= sgs.Player_NotActive) then return false end 
			if room:getTag("rexue_count"):toBool() then
				if player:getHp()==1  then
					touhou_logmessage("#TriggerSkill",player,"rexue")
					room:notifySkillInvoked(player, "rexue")
					local recover = sgs.RecoverStruct()
                    recover.recover = 1
                    room:recover(player,recover)
					--room:setPlayerMark(player,"touhou-extra",1)
					player:gainAnExtraTurn()
				end
			end	
        end
}
sidou = sgs.CreateTriggerSkill{
        name = "sidou",
        events = sgs.EventPhaseStart,
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local current = room:getCurrent()
                if current and current:objectName() == player:objectName() and player:getPhase() == sgs.Player_Start then
                        local targets = room:getAlivePlayers()
                        local xx = sgs.SPlayerList()
                        room:setPlayerMark(player, "sidou", 1)
						for _,p in sgs.qlist(targets) do
                            if player:canDiscard(p,"ej") then
                                xx:append(p)
							end
                        end
                        if xx:length() > 0 then
                                local target = room:askForPlayerChosen(player,xx,self:objectName(),"@sidou_target",true,true)
                                if target then
                                    local card_id = room:askForCardChosen(player,target,"je",self:objectName(),false,sgs.Card_MethodDiscard)
                                    room:setPlayerMark(player, "sidou", 0)
									room:throwCard(card_id,target,player)
									player:drawCards(1)
									room:loseHp(player)
								end
                        end
						room:setPlayerMark(player, "sidou", 0)
                end
        end
}

tymh_wuyu = sgs.CreateTriggerSkill{
    name = "tymh_wuyu$",
	frequency = sgs.Skill_Compulsory,
    events = {sgs.Death},
	--priority=3,--优先度不能影响断肠？？ 断肠
    on_trigger = function(self, event, player, data)
       local room = player:getRoom()
       if event == sgs.Death then
				local death = data:toDeath()
				local source = room:findPlayerBySkillName(self:objectName())
				
				if source:getRole() ~= "lord" then return false end
				if death.who:objectName() == source:objectName() then return false end
				if death.damage then
					death.damage.from=source
				else 
					local die = sgs.DamageStruct()
                    die.from = source
					death.damage=die
				end
				data:setValue(death)
				touhou_logmessage("#TriggerSkill",source,"tymh_wuyu")	
		end
    end,

}

ndj001:addSkill(rexue)
ndj001:addSkill(rexue_count)
ndj001:addSkill(sidou)
ndj001:addSkill(tymh_wuyu)
extension:insertRelatedSkills("rexue", "#rexue_count")

--【贤月公主——蓬莱山辉夜】ndj002
ndj002 = sgs.General(extension,"ndj002","zhu",3,false)
huanyue=sgs.CreateTriggerSkill{
	name="huanyue",
	events={sgs.EventPhaseStart},
	on_trigger = function(self, event, player, data)
        local room = player:getRoom()
		if ( event ==sgs.EventPhaseStart and player:getPhase() == sgs.Player_Start) then
			for _,p in sgs.qlist(room:getOtherPlayers(player)) do
				if p:getMark("@huanyue")>0 then
					local log = sgs.LogMessage()
					log.from=player
					log.arg="huanyue"
					log.type = "#TriggerSkill"
					room:sendLog(log)
					room:notifySkillInvoked(player, "huanyue")
					p:loseAllMarks("@huanyue")
					
				end
				
			end
			if player:getMark("huanyue_reason")>0 then
				room:setPlayerMark(player, "huanyue_reason", 0)
			end
		end
		if ( event ==sgs.EventPhaseStart and player:getPhase() == sgs.Player_Play) then
			local target = room:askForPlayerChosen(player,room:getOtherPlayers(player),self:objectName(),"@huanyuechosen",true,true)
			if target then
				target:gainMark("@huanyue")
				player:addMark("huanyue_reason")
			end
		end
	end,
}
huanyue_damage=sgs.CreateTriggerSkill{
	name="#huanyue_damage",
	events={sgs.DamageInflicted},
	can_trigger=function(self,player)
		return player
	end,
	on_trigger = function(self, event, player, data)
        local room = player:getRoom()
		if event ==sgs.DamageInflicted then
			local damage = data:toDamage()
			if damage.to:getMark("@huanyue")==0 then return false end
			if not damage.card or not damage.card:isNDTrick()then return false end
			local s = room:findPlayerBySkillName(self:objectName())
			if s then
				ask_card = room:askForCard(s, ".|black|.|hand","@huanyue-discard:"..damage.to:objectName(),data, sgs.Card_MethodDiscard, nil, true, self:objectName())
				if ask_card then
					touhou_logmessage("#huanyue_log",damage.from,damage.damage,damage.to,damage.damage + 1)
					damage.damage = damage.damage + 1
					data:setValue(damage)
				end
			end
		end
	end,
}

sizhai=sgs.CreateTriggerSkill{
	name="sizhai",
	events={sgs.GameStart,sgs.EventPhaseEnd},
	frequency = sgs.Skill_Frequent,
	can_trigger=function(self,player)
		return player
	end,
	on_trigger = function(self, event, player, data)
        
		if event==sgs.GameStart then
			if player and player:hasSkill("sizhai") then
			local room = player:getRoom()
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				if not p:hasSkill("#sizhai_count") then
				room:acquireSkill(p,"#sizhai_count")
				end
			end
			end
		end
		if event==sgs.EventPhaseEnd then
			local room = player:getRoom()
			local current=room:getCurrent() 
			if current:getPhase() == sgs.Player_Finish then
				if current:getMark("sizhai")==0 then
					local s = room:findPlayerBySkillName(self:objectName())
					if s and room:askForSkillInvoke(s,self:objectName(),sgs.QVariant("draw:"..current:objectName())) then
						s:drawCards(1)
					end
				end
				room:setPlayerMark(current, "sizhai",0)
			end
		end
	end,	
}

sizhai_count=sgs.CreateTriggerSkill{
	name="#sizhai_count",
	events = {sgs.CardUsed, sgs.CardResponded},
	on_trigger = function(self, event, player, data)
            local room = player:getRoom()
			local current=room:getCurrent()
			if player:objectName() ~=current:objectName() then return false end
			if event == sgs.CardUsed then
				local card = data:toCardUse().card
				if card and card:isKindOf("BasicCard") then
					player:addMark("sizhai")
				end
			end
			if event == sgs.CardResponded then
				card_star = data:toCardResponse().m_card
				if card_star and card_star:isKindOf("BasicCard") then
					player:addMark("sizhai")
				end
			end
	end,
}
if not sgs.Sanguosha:getSkill("#sizhai_count") then
    local skillList=sgs.SkillList()
    skillList:append(sizhai_count)
    sgs.Sanguosha:addSkills(skillList)
end
ndj002:addSkill(huanyue)
ndj002:addSkill(huanyue_damage)
ndj002:addSkill(sizhai)
extension:insertRelatedSkills("huanyue", "#huanyue_damage")

--【修行的庭师——魂魄妖梦】 ndj004
ndj004 = sgs.General(extension,"ndj004","yym",3,false)

youmingCard = sgs.CreateSkillCard{
        name = "youming",
        target_fixed=true,
		will_throw=true,
		on_use = function(self, room, source, targets)
			room:setPlayerProperty(source, "maxhp", sgs.QVariant(source:getMaxHp()+1))
			touhou_logmessage("#GainMaxHp",source,1)
			touhou_logmessage("#GetHp",source,source:getHp(),nil,source:getMaxHp())
		end
       
}

youming = sgs.CreateViewAsSkill{
        name = "youming",
        n = 1,
		view_filter = function(self, selected, to_select)
			return sgs.Self:canDiscard(sgs.Self,to_select:getId()) --true
		end,
        view_as = function (self,cards)
                if #cards==1 then 
					local card=youmingCard:clone()
					card:addSubcard(cards[1])
					return card 
				end
        end,

        enabled_at_play = function(self, player)
                return  not player:isNude() and player:getMaxHp()<4
        end
}

fanji=sgs.CreateTriggerSkill{
	name="fanji",
	events={sgs.Damaged},
	can_trigger=function(self,player)
		return player
	end,
	on_trigger = function(self, event, player, data)
        local room = player:getRoom()
		local damage = data:toDamage()
		local source=room:findPlayerBySkillName(self:objectName())
		if not source then return false end
		if not damage.from or damage.from:isDead() then return false end
		if not damage.to or damage.to:isDead() then return false end	
		if damage.from:objectName()==damage.to:objectName() then return false end
		if damage.from:objectName()==source:objectName() then return false end
		--for ai
		local _data=sgs.QVariant()
		_data:setValue(damage)
		source:setTag("fanji_damage",_data)
		local prompt="target:"..damage.from:objectName()..":"..damage.to:objectName()
		if damage.to:objectName()==source:objectName()  then
			if room:askForSkillInvoke(source,self:objectName(),sgs.QVariant(prompt)) then
				local damage1 = sgs.DamageStruct()
                damage1.from = source
                damage1.to = damage.from
                damage1.damage = 1
                room:damage(damage1)
			end
		else
			if source:inMyAttackRange(damage.to) and source:getMaxHp()>1 and room:askForSkillInvoke(source,self:objectName(),sgs.QVariant(prompt)) then
				room:loseMaxHp(source,1)
				local damage1 = sgs.DamageStruct()
                damage1.from = source
                damage1.to = damage.from
                damage1.damage = 1
                room:damage(damage1)
			end
		end
	end,
}

ndj004:addSkill(youming)
ndj004:addSkill(fanji)

--【境界探访者——玛艾露贝莉】  ndj010	
ndj010 = sgs.General(extension,"ndj010","wai",1,false)
--[[touhou_meng_lordskill = sgs.CreateTriggerSkill{--没有梅莉时，添加给主公
	name = "#touhou_meng_lordskill",
	events = {sgs.GameStart},
	--frequency = sgs.Skill_Wake,
	--can_trigger = function(self, player)
	--	return true
	--end,
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		local lord = room:getLord()
		--if player:objectName() ~=lord:objectName() then return false end
		if lord then
			--:hasLordSkill(self:objectName()) 
			t=true
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				if p:hasSkill("mengwei") then
					t=false
				end
			end
			if t then
				m_drawPile=room:getDrawPile()
				for _,id in sgs.qlist(m_drawPile) do
					local card = sgs.Sanguosha:getCard(id)
					if card:objectName()=="meng_weapon" then
						lord:addToPile("meng_list",card)
						room:setTag("meng_equip0", sgs.QVariant(id))
					end
					if card:objectName()=="meng_armor" then
						lord:addToPile("meng_list",card)
						room:setTag("meng_equip1", sgs.QVariant(id))
					end
					if card:objectName()=="meng_dhorse" then
						lord:addToPile("meng_list",card)
						room:setTag("meng_equip2", sgs.QVariant(id))
					end
					if card:objectName()=="meng_ohorse" then
						lord:addToPile("meng_list",card)
						room:setTag("meng_equip3", sgs.QVariant(id))
					end
				end
			end
		end
	end
}

mengwei_start=sgs.CreateTriggerSkill{
	name="#mengwei_start",
	events={sgs.GameStart,sgs.DrawInitialCards,sgs.AfterDrawInitialCards},

	on_trigger=function(self,event,player,data)
		
		if event ==sgs.GameStart then
			local room=player:getRoom()
			m_drawPile=room:getDrawPile() 
			for _,id in sgs.qlist(m_drawPile) do
				local card = sgs.Sanguosha:getCard(id)
				if card:objectName()=="meng_weapon" then
					player:addToPile("meng_list",card)
					room:setTag("meng_equip0", sgs.QVariant(id))
				end
				if card:objectName()=="meng_armor" then
					player:addToPile("meng_list",card)
					room:setTag("meng_equip1", sgs.QVariant(id))
				end
				if card:objectName()=="meng_dhorse" then
					player:addToPile("meng_list",card)
					room:setTag("meng_equip2", sgs.QVariant(id))
				end
				if card:objectName()=="meng_ohorse" then
					player:addToPile("meng_list",card)
					room:setTag("meng_equip3", sgs.QVariant(id))
				end
			end
		end
		if (event == sgs.DrawInitialCards) then
			local room = player:getRoom()
			local log = sgs.LogMessage()
            log.type = "#TriggerSkill"
            log.from = player
            log.arg = "mengwei"
            room:sendLog(log)
            room:notifySkillInvoked(player, "mengwei")

            --data = data:toInt() + 6
			player:drawCards(3) --是否该写到摸排技里？避免双将问题，如强欲
		end
		if event== sgs.AfterDrawInitialCards then
            local room = player:getRoom()
			exchange_card = room:askForExchange(player, "mengwei", 3);
            --三张？的摆放。
			choices={"meng_equip0","meng_equip1","meng_equip2","meng_equip3"}
			for _,id in sgs.qlist(exchange_card:getSubcards()) do
				local choice = room:askForChoice(player, "mengwei", table.concat(choices, "+"))
				local g
				for s=1, #choices ,1 do
					g=choices[s]
					if g==choice then
						table.remove(choices,s)
					end
				end
				for n=0, 3, 1 do 
					key="meng_equip"..tostring(n)
					if choice==key then
						
						meng_id=room:getTag(key):toInt()
						--room:obtainCard(player,meng_id)
						
						local exchangeMove = sgs.CardsMoveList()
						--新"?"进入装备区           
						local move1 = sgs.CardsMoveStruct(meng_id, player, sgs.Player_PlaceEquip,
                                sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_PUT, player:objectName()))
						exchangeMove:append(move1)
						room:moveCardsAtomic(exchangeMove, true)
						--记录实际对应牌
						
						
						meng_key="meng"..tostring(n)
						room:setTag(meng_key, sgs.QVariant(id))
						player:addToPile(meng_key, id, false)
						--"meng_list"
						player:gainMark(key)
					end
				end
			end
			    --delete exchange_card;
		end
		return false
	end
}		
mengwei_handle=sgs.CreateTriggerSkill{--管理除【裂隙】外所有"?"移动
	name="#mengwei_handle",
	priority=3,
	events={sgs.BeforeCardsMove,sgs.CardsMoveOneTime},
	on_trigger=function(self,event,player,data)
		if event==sgs.BeforeCardsMove then
			
			local move = data:toMoveOneTime()
			meng_equips=sgs.IntList()
			local room =player:getRoom()
			local source=room:findPlayerBySkillName(self:objectName())
			if player:objectName() ~=source:objectName() then return false end
			if source:hasFlag("meng_move") then return false end
			t0=false
			t1=false
			t2=false
			t3=false
			if move.from and move.from_places:contains(sgs.Player_PlaceEquip) and move.from:objectName() ==player:objectName()then
				for _,id in sgs.qlist(move.card_ids) do																		
					if room:getCardPlace(id) == sgs.Player_PlaceEquip then
						local card = sgs.Sanguosha:getCard(id)
						if card:objectName()=="meng_weapon" then
							meng_equips:append(id)
							t0=true
						end
						if card:objectName()=="meng_armor" then
							meng_equips:append(id)
							t1=true
						end
						if card:objectName()=="meng_dhorse" then
							meng_equips:append(id)
							t2=true
						end
						if card:objectName()=="meng_ohorse" then
							meng_equips:append(id)
							t3=true
						end
					end
				end
				for _,id in sgs.qlist(meng_equips) do
					source:setFlags("meng_move")
					source:addToPile("meng_list",id)
					move.card_ids:removeOne(id)		
				end
				--目前对于要将人偶挪入装备区的move 还没测过
				if t0 then
					meng_id=room:getTag("meng0"):toInt()
					--move.card_ids:append(renou_id)
					room:setTag("meng0", sgs.QVariant())
					source:loseMark("meng_equip0")
					local mo = sgs.CardsMoveStruct()
                    mo.card_ids:append(meng_id)
					if move.to_place ==sgs.Player_PlaceEquip then
						mo.to = nil
						mo.to_place = sgs.Player_DiscardPile
					else
						mo.to = move.to
						mo.to_place = move.to_place
					end
                    room:moveCardsAtomic(mo,true)
				end
				if t1 then
					meng_id=room:getTag("meng1"):toInt()
					--move.card_ids:append(renou_id)
					room:setTag("meng1", sgs.QVariant())
					source:loseMark("meng_equip1")
					local mo = sgs.CardsMoveStruct()
                    mo.card_ids:append(meng_id)
                    if move.to_place ==sgs.Player_PlaceEquip then
						mo.to = nil
						mo.to_place = sgs.Player_DiscardPile
					else
						mo.to = move.to
						mo.to_place = move.to_place
					end
                    room:moveCardsAtomic(mo,true)
				end
				if t2 then
					meng_id=room:getTag("meng2"):toInt()
					--move.card_ids:append(renou_id)
					room:setTag("meng2", sgs.QVariant())
					source:loseMark("meng_equip2")
					local mo = sgs.CardsMoveStruct()
                    mo.card_ids:append(meng_id)
                    if move.to_place ==sgs.Player_PlaceEquip then
						mo.to = nil
						mo.to_place = sgs.Player_DiscardPile
					else
						mo.to = move.to
						mo.to_place = move.to_place
					end
                    room:moveCardsAtomic(mo,true)
				end
				if t3 then
					meng_id=room:getTag("meng3"):toInt()
					--move.card_ids:append(renou_id)
					room:setTag("meng3", sgs.QVariant())
					source:loseMark("meng_equip3")
					local mo = sgs.CardsMoveStruct()
                    mo.card_ids:append(meng_id)
                    if move.to_place ==sgs.Player_PlaceEquip then
						mo.to = nil
						mo.to_place = sgs.Player_DiscardPile
					else
						mo.to = move.to
						mo.to_place = move.to_place
					end
                    room:moveCardsAtomic(mo,true)
				end
				
				data:setValue(move)
				source:setFlags("-meng_move")
			end
		end
	end
}
mengwei=sgs.CreateTriggerSkill{
	name="mengwei",
	events={sgs.EventPhaseStart,sgs.DamageInflicted},
	--priority=2,
	frequency = sgs.Skill_Compulsory,
	
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		if event==sgs.EventPhaseStart and player:getPhase()==sgs.Player_Finish then
			if player:getCards("e"):length()<4 then
				player:drawCards(1)
				exchange_card = room:askForExchange(player, "mengwei", 1);
				
				choices={}
				if not player:getWeapon() then
					table.insert(choices,"meng_equip0")
				end
				if not player:getArmor() then
					table.insert(choices,"meng_equip1")
				end
				if not player:getDefensiveHorse() then
					table.insert(choices,"meng_equip2")
				end
				if not player:getOffensiveHorse() then
					table.insert(choices,"meng_equip3")
				end
				local choice = room:askForChoice(player, "mengwei", table.concat(choices, "+"))
				meng_id=room:getTag(choice):toInt()
				meng_card=sgs.Sanguosha:getCard(meng_id)
				n=0
				if meng_card:objectName()=="meng_weapon" then
					n=0
				end
				if meng_card:objectName()=="meng_armor" then
					n=1
				end
				if meng_card:objectName()=="meng_dhorse" then
					n=2
				end
				if meng_card:objectName()=="meng_ohorse" then
					n=3
				end
				local exchangeMove = sgs.CardsMoveList()
						--新"?"进入装备区           
				local move1 = sgs.CardsMoveStruct(meng_id, player, sgs.Player_PlaceEquip,
                                sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_PUT, player:objectName()))
				exchangeMove:append(move1)
				room:moveCardsAtomic(exchangeMove, true)
				--记录实际对应牌
				id=exchange_card:getSubcards():first()
				
				meng_key="meng"..tostring(n)
				room:setTag(meng_key, sgs.QVariant(id))
				player:addToPile(meng_key, id, false)
				--"meng_list"
				player:gainMark(choice)
			end
		end
		if event==sgs.DamageInflicted then
				
			choices={}
				
			for n=0,3,1 do
					if player:getEquip(n) then
						e=player:getEquip(n)
						if e:objectName()=="meng_weapon" or
							e:objectName()=="meng_armor" or
							e:objectName()=="meng_dhorse" or
							e:objectName()=="meng_ohorse" then
							table.insert(choices,"meng_equip"..tostring(n))
						end
					end
			end
			if #choices>0 then
				local log = sgs.LogMessage()
				log.arg="mengwei"
				log.from=player
				log.type = "#TriggerSkill"
				room:sendLog(log)
				room:notifySkillInvoked(player, self:objectName())
				
				local choice = room:askForChoice(player, "mengwei", table.concat(choices, "+"))
				meng_id=room:getTag(choice):toInt()
				meng_card=sgs.Sanguosha:getCard(meng_id)
				t=0
				if meng_card:objectName()=="meng_weapon" then
					t=0
				end
				if meng_card:objectName()=="meng_armor" then
					t=1
				end
				if meng_card:objectName()=="meng_dhorse" then
					t=2
				end
				if meng_card:objectName()=="meng_ohorse" then
					t=3
				end
				
				player:setFlags("meng_move")
				player:addToPile("meng_list", meng_id, false)
				player:setFlags("-meng_move")
				
				meng_key="meng"..tostring(t)
				id =room:getTag(meng_key):toInt()
				room:obtainCard(player,id,true)
				room:setTag(meng_key, sgs.QVariant())
				
				player:loseMark(choice)
				return true
			end
				
		end	

		
	end
}
mengwei_dis = sgs.CreateDistanceSkill{
        name = "#mengwei_dis",
        correct_func = function (self,from,to)

				
				if from:hasSkill(self:objectName()) and from:getMark("meng_equip3")>0 then
                    return 1
                end
                if to:hasSkill(self:objectName()) and to:getMark("meng_equip2")>0then
                    return -1
                end
        end
}
mengwei_collateral = sgs.CreateProhibitSkill{--天仪不能被借刀
        name = "#mengwei_collateral" ,
        is_prohibited = function(self, from, to, card)
            if to:getMark("meng_equip0")>0 then
				return to:hasSkill(self:objectName()) and card:isKindOf("Collateral")
			end	
        end,
}
mengwei_transfer=sgs.CreateTriggerSkill{
	name="#mengwei_transfer",
	priority=3,
	events={sgs.Death,sgs.EventLoseSkill},
	can_trigger = function(self, player)
		return player:getPile("meng_list"):length()>0 or player:getMark("meng_equip0")>0 or player:getMark("meng_equip1")>0 or player:getMark("meng_equip2")>0 or player:getMark("meng_equip3")>0
	end,
	on_trigger=function(self,event,player,data)
		if event==Death then
			local room = player:getRoom()
            local death = data:toDeath()
			if death.who:objectName() == player:objectName() then
				local lord = room:getLord()
				if death.who:objectName() ~= lord :objectName() then
					for n=0,3,1 do
						key="meng_equip"..tostring(n)
						id=room:getTag(key):toInt()
						death.who:setFlags("meng_move")
						room:obtainCard(death.who,id)
						lord:addToPile("meng_list",id)
						death.who:setFlags("-meng_move")
					end
					ids=sgs.IntList()
					for n=0,3,1 do
						meng_key="meng"..tostring(n)
						piles=death.who:getPile(renou_key)
						for _,id in sgs.qlist(piles) do
							ids:append(id)
						end
					end
					--真?牌需要送到弃牌堆
					local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, "", nil, self:objectName(), "")
					local move = sgs.CardsMoveStruct(ids, nil, sgs.Player_DiscardPile, reason)
					local exchangeMove= sgs.CardsMoveList()						
					exchangeMove:append(move)	
					room:moveCardsAtomic(exchangeMove, true)
				end
			end
		end
		if event ==sgs.EventLoseSkill and data:toString() == "mengwei" then
			for n=0,3,1 do
				key="meng_equip"..tostring(n)
				id=room:getTag(key):toInt()
				
				player:setFlags("meng_move")
				--room:obtainCard(player,id)
				player:addToPile("renou_list",id)
				player:setFlags("-meng_move")
			end
				ids=sgs.IntList()
				for n=0,3,1 do
					meng_key="meng"..tostring(n)
					piles=death.who:getPile(meng_key)
					for _,id in sgs.qlist(piles) do
						ids:append(id)
					end
				end
				--真?牌需要送到弃牌堆
				local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, "", nil, self:objectName(), "")
				local move = sgs.CardsMoveStruct(ids, nil, sgs.Player_DiscardPile, reason)
				local exchangeMove= sgs.CardsMoveList()						
				exchangeMove:append(move)	
				room:moveCardsAtomic(exchangeMove, true)
				
				for n=0,3,1 do
					key="meng_equip"..tostring(n)
					if player:getMark(key)>0 then
						player:loseAllMarks(key)
					end
				end
		end
	end
}
liexicard = sgs.CreateSkillCard{
        name = "liexi",
        target_fixed = false,
        will_throw = false,
		filter = function(self, targets, to_select,player)
                return (#targets == 0) and to_select:objectName() ~=player:objectName()
        end ,
		on_use = function(self, room, source, targets)
			target=targets[1]
			
			cards=self:getSubcards()
			for _,id in sgs.qlist(cards) do --处理两张？牌
				--local card = sgs.Sanguosha:getCard(id)
				room:throwCard(id,source,source)
			end	
			target:turnOver()
		end
}

liexi = sgs.CreateViewAsSkill{
        name = "liexi",
        n = 2,
        view_filter = function(self,selected,to_select)
            return to_select:objectName()=="meng_weapon" or to_select:objectName()=="meng_armor" or to_select:objectName() =="meng_dhorse" or to_select:objectName()=="meng_ohorse"
        end,
        view_as = function(self,cards)
            if #cards==2 then
				local card=liexicard:clone()
				card:addSubcard(cards[1])
				card:addSubcard(cards[2])
				card:setSkillName(self:objectName())
				return card
			end
        end,

    enabled_at_play = function(self, target)
            return not target:hasUsed("#liexi")
    end,
}
ndj010:addSkill(mengwei_start)
ndj010:addSkill(mengwei_handle)
ndj010:addSkill(mengwei_dis)
ndj010:addSkill(mengwei_collateral)
ndj010:addSkill(mengwei)
ndj010:addSkill(mengwei_transfer)
ndj010:addSkill(liexi)
extension:insertRelatedSkills("mengwei", "#mengwei_collateral")
extension:insertRelatedSkills("mengwei", "#mengwei_dis")
extension:insertRelatedSkills("mengwei", "#mengwei_handle")
extension:insertRelatedSkills("mengwei", "#mengwei_start")
extension:insertRelatedSkills("mengwei", "#mengwei_transfer")
if not sgs.Sanguosha:getSkill("#touhou_meng_lordskill") then
        local skillList = sgs.SkillList()
        skillList:append(touhou_meng_lordskill)
        sgs.Sanguosha:addSkills(skillList)
end
]]

--【科学少女——宇佐见莲子】 ndj011
ndj011 = sgs.General(extension,"ndj011","wai",4,false)
liangzi_chain=function(player)
	local room= player:getRoom()
	local log = sgs.LogMessage()
	log.from=player
	log.arg="liangzi"
	log.type = "#TriggerSkill"
	room:sendLog(log)
	room:notifySkillInvoked(player, "liangzi")
	if player:isChained() then
		player:setChained(false)
		sgs.Sanguosha:playSystemAudioEffect("chained")
		room:broadcastProperty(player, "chained")
		room:setEmotion(player, "chain")
	else
		player:setChained(true)
		sgs.Sanguosha:playSystemAudioEffect("chained")
		room:broadcastProperty(player, "chained")
		room:setEmotion(player, "chain")
	end
	
end
liangzi=sgs.CreateTriggerSkill{
	name="liangzi",
	frequency = sgs.Skill_Compulsory,
	events = {sgs.CardUsed, sgs.CardResponded},
	on_trigger = function(self, event, player, data)
            local room = player:getRoom()
	
			if event == sgs.CardUsed then
				local card = data:toCardUse().card
				if card and card:isKindOf("BasicCard") then
					liangzi_chain(player)
				end
			end
			if event == sgs.CardResponded then
				card_star = data:toCardResponse().m_card
				if card_star and card_star:isKindOf("BasicCard") then
					liangzi_chain(player)
				end
			end
	end,
}

kexue = sgs.CreateTargetModSkill{
        name = "kexue",
        pattern="Slash",
        distance_limit_func = function(self,player,card)
                if player:hasSkill(self:objectName()) and player:isChained() then 
                        return 1000
                else
                        return 0
                end
        end,
        extra_target_func = function(self,player,card)
                if player:hasSkill(self:objectName()) and card:isKindOf("Slash") and player:isChained() then 
                        return 1000
                else
                        return 0
                end
        end
}
ndj011:addSkill(liangzi)
ndj011:addSkill(kexue)

		
		
