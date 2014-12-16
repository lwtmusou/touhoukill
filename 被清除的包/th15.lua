module("extensions.th15", package.seeall)
extension = sgs.Package("th15")
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
--技能代码   神卡
------------------------------------------------------------------------------------------------------------
sgs.LoadTranslationTable{
        ["th15"] = "test",
}

--【创幻神主——ZUN】 编号：00000  --by三国有单
 --shen000 = sgs.General(extension, "shen000", "touhougod", 0, true) 
 local function touhou_shuffle(atable)
	local count = #atable
	for i = 1, count do
		local j = math.random(1, count)
		atable[j], atable[i] = atable[i], atable[j]
	end
	return atable
end


--启动时机列表
--table 1 专门列出出牌阶段的viewas技
	local chuanghuan_attach_PlayViewAs={"mofa","jiezou","suoding","zhanyi","zhenye","banyue",
	"mocao","miyao","kuangzao","qizha","gesheng","yingguang","yazhi","bian","leiyun",
	"fengshen","xinshang","zaihuo","tianyan","fengrang","jiliao","maihuo","baigui","jiuchong",
	"jidu","LuaShouhuiVS","pudu","weizhi","qingting","leishi","xiefa","duzhua","buming","qiuwen","xiufu",
	"zhuonong","hongwu","shenqiang","shenfu","jiejie","yuzhu"}
	
--消除时机
	
 select_chuanghuan= function(player,skill_table)--选择可触发的技能 
	
	local room = player:getRoom()
	huanxiangs = player:getTag("huanxiangs"):toString()
		g_names = huanxiangs:split("+") --table
		--n=player:getMark("@huanxiangs")
		skill_names="cancel"
		for var=1,#g_names,1 do
			g_name=g_names[var]
			general = sgs.Sanguosha:getGeneral(g_name)
			if general then
				for _,skill in  sgs.qlist(general:getVisibleSkillList()) do
					if (skill:isLordSkill() or skill:getFrequency() == sgs.Skill_Limited or skill:getFrequency() == sgs.Skill_Wake  ) then			
					else
						if table.contains(skill_table,skill:objectName()) then
							skill_names =skill_names.."+"..skill:objectName()
						end
					end	
				end
			end
		end
		skill_name = room:askForChoice(player, "chuanghuan", skill_names)
		if skill_name ~="cancel" then
			--去除原有武将牌
			for var=1,#g_names,1 do
				g_name=g_names[var]
				general = sgs.Sanguosha:getGeneral(g_name)
				if general and general:hasSkill(skill_name) then
					table.remove(g_names,var)
					tag = sgs.QVariant(table.concat(g_names, "+")) 
					player:setTag("huanxiangs",tag)
					break
				end
			end
			room:setPlayerMark(player,"@huanxiangs",player:getMark("@huanxiangs")-1)
			local log=sgs.LogMessage()
			log.type = "#chuanghuan1"
			log.from = player
			log.arg = "chuanghuan"
			room:sendLog(log)
			room:notifySkillInvoked(player, "chuanghuan")
			room:handleAcquireDetachSkills(player,skill_name)
		end
 end
 find_use_to= function(list,player)
  local to=player
  for _,p in sgs.qlist(list) do
	if p:objectName()==player:objectName() then
		to=p
	end
  end
  if to then
	return true
	else
	return false
  end
 end
 find_suit=function(move)
	t=false
	for _,id in sgs.qlist(move.card_ids) do
		if sgs.Sanguosha:getCard(id):getSuit()==sgs.Card_Heart then
			t=true
			break
		end
	end	
	return t	
 end

 chuanghuan_attach=sgs.CreateTriggerSkill{
        name = "#chuanghuan",
		priority=3,--优先级应该是？？
        events = {sgs.EventPhaseStart,sgs.EventPhaseChanging,sgs.DrawNCards,sgs.AfterDrawNCards,sgs.PreHpRecover,sgs.HpRecover,sgs.PostHpReduced,sgs.StartJudge,sgs.AskForRetrial,sgs.FinishJudge,sgs.TurnedOver,sgs.Predamage, sgs.DamageForseen, sgs.DamageCaused,sgs.DamageInflicted, sgs.PreDamageDone, sgs.Damage,sgs.Damaged,sgs.Dying,sgs.AskForPeaches,sgs.Death,sgs.SlashProceed,sgs.SlashMissed,sgs.CardAsked,sgs.CardResponded,sgs.BeforeCardsMove,  sgs.CardsMoveOneTime,sgs.PreCardUsed, sgs.CardUsed,sgs.TargetConfirming,sgs.TargetConfirmed, sgs.CardEffected,sgs.PostCardEffected,sgs.CardFinished},
        
		can_trigger = function(self, player)
            return player 
        end,
		on_trigger = function(self, event, player, data)
            local skill_names={}
			local room = player:getRoom()
			--if not player:hasSkill("chuanghuan") then return false end
			local source = room:findPlayerBySkillName(self:objectName())
			if not source then return false end
			--sgs.GameStart,
    --sgs.TurnStart,
    if event==sgs.EventPhaseStart then
		if player:getPhase() == sgs.Player_Start then
			table.insert(skill_names,"xisan")
			--table.insert(skill_names,"anyu")
			--table.insert(skill_names,"tongju")
			if player:objectName()==source:objectName() then
				table.insert(skill_names,"pohuai")
				table.insert(skill_names,"jinghua")
				table.insert(skill_names,"fandu")
				table.insert(skill_names,"cuixiang")
				table.insert(skill_names,"weiya")
				table.insert(skill_names,"shishen")
				table.insert(skill_names,"chunyi")
				table.insert(skill_names,"youqu")
				
				--if source:getCards("e"):length()+source:getCards("j"):length() >0 then
					table.insert(skill_names,"baoyi")
				--end
			else
				table.insert(skill_names,"yongheng")
			end
		end
		--if player:getPhase() == sgs.Player_Judge then
		--	select_chuanghuan(source, chuanghuan_attach_EventPhaseStart2)
		--end
		if player:getPhase() == sgs.Player_Draw  then
			if player:objectName()==source:objectName() then
				table.insert(skill_names,"aojiaofsubisuo")
				table.insert(skill_names,"shenpan")
			end
		end
		if player:getPhase() == sgs.Player_Play then
			if player:objectName()==source:objectName() then
				table.insert(skill_names,"shishen")
			else
				table.insert(skill_names,"saiqian")
			end
		end
		if player:getPhase() == sgs.Player_Discard   then
			if player:objectName()==source:objectName() then
				table.insert(skill_names,"shoucang")
				table.insert(skill_names,"tansuo")
				table.insert(skill_names,"chuixue")	
			end
		end
		if player:getPhase()== sgs.Player_Finish then
			if player:objectName()==source:objectName() then
				table.insert(skill_names,"qianfu")
				table.insert(skill_names,"chunmian")
				table.insert(skill_names,"youqu")
				table.insert(skill_names,"jiushu")
				table.insert(skill_names,"hengxing")
			end
			table.insert(skill_names,"xijian")--隙间的条件还可以细化
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end	
	end
   -- sgs.EventPhaseProceeding,
    --sgs.EventPhaseEnd,
    if event==sgs.EventPhaseChanging then
			local change = data:toPhaseChange()
			local current=room:getCurrent()
			if current:objectName()~=player:objectName() then return false end
			if change.to == sgs.Player_Start then
				
					if current:objectName()==source:objectName() then
						table.insert(skill_names,"beisha")
					else
						table.insert(skill_names,"qiyue")
					end			
			end
			
            if change.to == sgs.Player_Draw then
				
					if current:objectName()==source:objectName() then
						table.insert(skill_names,"zhize")
						table.insert(skill_names,"toupai")
						
						table.insert(skill_names,"cuiji")
					end
				
			end
			if change.to == sgs.Player_Play then
				
					if current:objectName()==source:objectName() then
					else
						table.insert(skill_names,"quanjie")
					end
				
			end
			if change.to == sgs.Player_Discard then
				
					if current:objectName()==source:objectName() then
						table.insert(skill_names,"yongheng")
					end
					table.insert(skill_names,"jingdong")
			end
			if change.to == sgs.Player_Finish then
				
					if current:objectName()==source:objectName() then
						table.insert(skill_names,"zaozu")

					end
				
			end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
   -- sgs.EventPhaseSkipping,
	
    if event==sgs.DrawNCards then
		local current=room:getCurrent()
		if player:objectName()~=current:objectName() then return false end
		if current:objectName()==source:objectName() then
			table.insert(skill_names,"qingcang")
			table.insert(skill_names,"juhe")
			table.insert(skill_names,"chuangshi")
			table.insert(skill_names,"kuaizhao")
			table.insert(skill_names,"zhunbei")
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    if event == sgs.AfterDrawNCards then
		local current=room:getCurrent()
		if player:objectName()~=current:objectName() then return false end
		if current:objectName()==source:objectName() then
			table.insert(skill_names,"jingjie")
			table.insert(skill_names,"reimu_fengyin")
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    --sgs.DrawInitialCards ,
    --sgs.AfterDrawInitialCards ,

    if event==sgs.PreHpRecover then
		if player:objectName()==source:objectName() then
			table.insert(skill_names,"tanchi")
			table.insert(skill_names,"gaoao")
			
		end
		table.insert(skill_names,"jiexian")
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    --sgs.HpRecover,
    --if event==sgs.PreHpLost then
	--【不灭】
	--end
    --sgs.HpChanged,
    --sgs.MaxHpChanged,
	-------
	---无法定位只由创幻者触发。。。。可以考虑对这些情况单写
	-------
    --if event==sgs.PostHpReduced then
	--	select_chuanghuan(source,chuanghuan_attach_PostHpReduced)
	--end
    --sgs.EventLoseSkill,
    --sgs.EventAcquireSkill,

    if event==sgs.StartJudge then
		if player:objectName()==source:objectName() then
			table.insert(skill_names,"hongbai")
		end
		table.insert(skill_names,"mingyun")
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    if event==sgs.AskForRetrial then
		if player:objectName()==source:objectName() then
			table.insert(skill_names,"feixiang")
			table.insert(skill_names,"fengshui")

		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
   -- sgs.FinishRetrial,
    if event==sgs.FinishJudge then
		local judge = data:toJudge()
		if player:objectName()==source:objectName() then
			if judge.card:isRed() then
				table.insert(skill_names,"dizhen")
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
   -- sgs.PindianVerifying,
    --sgs.Pindian,

    --if event==sgs.TurnedOver then
	--	if player:hasSkill("chuanghuan") then
	--		select_chuanghuan(source,chuanghuan_attach_TurnedOver)
	--	end
	--end
    --sgs.ChainStateChanged,

    --sgs.ConfirmDamage, // confirm the damage's count and damage's nature
    if event==sgs.Predamage then --// trigger the certain skill -- jueqing
		damage = data:toDamage()
		if damage.from  then 
			if player:objectName()~=damage.from:objectName() then return false end
			if	damage.from:objectName() ==source:objectName() then
				
				table.insert(skill_names,"wunian")
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
	if event==sgs.DamageForseen then --// the first event in a damage -- kuangfeng dawu
		damage = data:toDamage()
		if damage.to then
			if damage.to:objectName() ~=player:objectName() then return false end
			if	damage.to:objectName() ==source:objectName() then
				table.insert(skill_names,"jingdian")
				table.insert(skill_names,"yewang")
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
	if event==sgs.DamageCaused then--// the moment for -- qianxi..
		damage = data:toDamage()
		if damage.from then
			if damage.from:objectName() ~=player:objectName() then return false end
			if  damage.from:objectName() ==source:objectName() then
				table.insert(skill_names,"lizhi")
				if damage.card then
					if damage.card:isKindOf("Slash") then
						table.insert(skill_names,"dongjie")
						table.insert(skill_names,"sidie")--死蝶条件是否还要细化？
						table.insert(skill_names,"silian")
						table.insert(skill_names,"ronghui")
					end
				end
				--if damage.to:getHandcardNum() + damage.to:getEquips():length()>0 then
					table.insert(skill_names,"shenyin")
				--end
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
	if event==sgs.DamageInflicted then --// the moment for -- tianxiang..
		damage = data:toDamage()
		if damage.to then
			if damage.to:objectName() ~=player:objectName() then return false end
			table.insert(skill_names,"jiexian")
			if  damage.to:objectName() ==source:objectName() then
				table.insert(skill_names,"zheshe")
				table.insert(skill_names,"zhengti")
				table.insert(skill_names,"bianhuan")
				table.insert(skill_names,"micai")
				if damage.nature ~= sgs.DamageStruct_Fire then
					table.insert(skill_names,"bingpo")
				end
				--有些条件还可以细化？
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    --sgs.PreDamageDone, // before reducing Hp
    --sgs.DamageDone, // it's time to do the damage
    if event==sgs.Damage then --// the moment for -- lieren..
		damage = data:toDamage()
		if damage.from then
			if damage.from:objectName() ~=player:objectName() then return false end
			if  damage.from:objectName() ==source:objectName() then
				table.insert(skill_names,"shengyan")
				
				table.insert(skill_names,"chuannan")--船难还要细化？
				if damage.to:objectName() ~=source:objectName() then
					table.insert(skill_names,"judu")
				end
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    if event==sgs.Damaged then-- // the moment for -- yiji..
		damage = data:toDamage()
		if damage.to then
			if damage.to:objectName() ~=player:objectName() then return false end
				if source:inMyAttackRange(damage.to) then
					table.insert(skill_names,"hina_jie")
					if damage.to:isAlive() then
						table.insert(skill_names,"bushu")
					end
				end
				if damage.nature == sgs.DamageStruct_Fire then
					table.insert(skill_names,"yaoban")
				end				
				if  damage.to:objectName() ==source:objectName() then
					table.insert(skill_names,"yuxue")
					table.insert(skill_names,"huisu")
					table.insert(skill_names,"jingjie")
					table.insert(skill_names,"shishen")
					table.insert(skill_names,"baochun")
					table.insert(skill_names,"wangyue")
					table.insert(skill_names,"jubao")
					table.insert(skill_names,"yuanling")
					table.insert(skill_names,"chuannan")
					table.insert(skill_names,"jingxia")
					table.insert(skill_names,"zhengti")
					table.insert(skill_names,"qingyu" )
					table.insert(skill_names,"fandu")
					table.insert(skill_names,"pingyi")--"凭依"还要细化
					table.insert(skill_names,"henyi")
					
				else
					if damage.to:isAlive() then
						table.insert(skill_names,"zhaoliao")
						if damage.from:objectName() ~=source:objectName() then
							if damage.card and (damage.card:isKindOf("Slash") or  damage.card:isKindOf("TrickCard")) then
								table.insert(skill_names,"xiangqi")
							end
						end
					end
				end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    --sgs.DamageComplete, // the moment for trigger iron chain

    if event==sgs.Dying then
		local who=room:getCurrentDyingPlayer()
		if who:hasSkill("chuanghuan") and player:objectName() ==who:objectName()  then
			--if who:getEquips():length()==0 then
				table.insert(skill_names,"jiaoxia")
			--end
			table.insert(skill_names,"juxian")
			table.insert(skill_names,"fenyuan")
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    --sgs.QuitDying,
    if event==sgs.AskForPeaches then
		local who= room:getCurrentDyingPlayer()
		local current=room:getCurrent() 
		if  player:objectName() ==current:objectName()  then
			if current:objectName()==source:objectName() then
				table.insert(skill_names,"songzang")
			end
		end
		if player:objectName()==source:objectName() then
			table.insert(skill_names,"sisheng")
			table.insert(skill_names,"shijie")
			if who:hasSkill("chuanghuan") then
				table.insert(skill_names,"LuaShouhuiVS")
			end
		end			
		if who:objectName()==source:objectName() then
			table.insert(skill_names,"changqing")
			if player:objectName()~=who:objectName() then
			table.insert(skill_names,"skltkexue")
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    --sgs.AskForPeachesDone,
    if event==sgs.Death then
		local death = data:toDeath()
		if death.who:objectName()==player:objectName() and death.who:hasSkill("chuanghuan")  then
			table.insert(skill_names,"chuancheng")
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    --sgs.BuryVictim,
    --sgs.BeforeGameOverJudge,
    --sgs.GameOverJudge,
    --sgs.GameFinished,

    --if event==sgs.SlashEffected then
		--【七曜】
	--end
    if event==sgs.SlashProceed then
		local effect = data:toSlashEffect()
		--sgs.SlashProceed只能由effect.from触发？
		if effect.from:objectName()==source:objectName() or effect.to:objectName()==source:objectName() then
				table.insert(skill_names,"douhun")
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    --sgs.SlashHit,
    if event==sgs.SlashMissed then
		local effect = data:toSlashEffect()
		if effect.from:objectName()==player:objectName()  then
			if player:hasSkill("chuanghuan") then
				table.insert(skill_names,"shenquan")
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end

   -- sgs.JinkEffect,

    if event==sgs.CardAsked then
		local pattern = data:toStringList()[1]
		if player:hasSkill("chuanghuan")then
			if pattern =="slash" then
				table.insert(skill_names,"zhanyi")
				table.insert(skill_names,"yuzhu")
				table.insert(skill_names,"tymhhuweivs")
			end
			if pattern =="jink" then
				table.insert(skill_names,"chuanbi")
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    if event==sgs.CardResponded then
		card_star = data:toCardResponse().m_card
		
		if player:hasSkill("chuanaghuan") then
			if (card_star:isKindOf("Slash")) then
				table.insert(skill_names,"shende")
				table.insert(skill_names,"lianxivs")
			end
			if (card_star:isKindOf("Jink")) then
				table.insert(skill_names,"yangchong")
			end
		else
			if (card_star:isKindOf("Jink")) then
				table.insert(skill_names,"taotie")
				
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
  --move类需要更加细化  
	if event==sgs.BeforeCardsMove then -- // sometimes we need to record cards before the move
		move = data:toMoveOneTime()
		reason = move.reason
		if move.from and move.from:objectName() ==player:objectName() then  
			if move.from_places:contains(sgs.Player_PlaceDelayedTrick) then
				if player:getPhase()== sgs.Player_Judge then
					table.insert(skill_names,"chuanran")
				end
			end
			
			if move.from:hasSkill("chuanghuan") then
				if bit32.band(reason.m_reason, sgs.CardMoveReason_S_MASK_BASIC_REASON) == sgs.CardMoveReason_S_REASON_DISCARD then
					local huwei_bool=false
					
					--table.insert(skill_names,"taohuan")
					for _,id in sgs.qlist(move.card_ids) do							
						local card = sgs.Sanguosha:getCard(id)
						if (card:isKindOf("Slash"))  then 								
							huwei_bool=true
							break
						end	
					end
					if huwei_bool then
						table.insert(skill_names,"tymhhuweivs")
					end
				end
				if move.to_place == sgs.Player_PlaceHand then
					table.insert(skill_names,"taohuan")
				end
			end
			
		end
		if move.to and move.to:objectName() ==player:objectName()then
			if move.to:hasSkill("chuanghuan")  then
				--目前只有【强欲】
				if move.to_place == sgs.Player_PlaceHand and move.from_places:contains (sgs.Player_DrawPile) and (not room:getTag("FirstRound"):toBool()) then
					table.insert(skill_names,"qiangyu" )
				end
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
	if event==sgs.CardsMoveOneTime then
		move = data:toMoveOneTime()
		local current=room:getCurrent()
		if move.from and move.from:objectName() ==player:objectName() then  
			if move.from:hasSkill("chuanghuan") then
				if move.is_last_handcard then
					table.insert(skill_names, "haidi")
				end
				--if move.from_places:contains(sgs.Player_PlaceHand) and current:objectName()~=source:objectName() then
					--table.insert(skill_names, "yongheng")
				--end
				if move.from_places:contains(sgs.Player_PlaceEquip)  then
					table.insert(skill_names, "yiwang") 
				end
				if move.from_places:contains(sgs.Player_PlaceDelayedTrick)  then
					table.insert(skill_names, "guoke") 
				end
			end
			if move.from and move.from:objectName() ~= source:objectName() then
				if current:objectName() == source:objectName() then
					if move.to_place == sgs.Player_DiscardPile then
						table.insert(skill_names, "souji")
					end
				end
			end
		end
		
		if move.to and move.to:objectName() ==player:objectName()then
			if move.to:hasSkill("chuanghuan") then	
				if current:objectName()~=source:objectName() then
					table.insert(skill_names, "gaoao")
				end
				if move.to_place == sgs.Player_PlaceHand then
					if find_suit(move) and (not room:getTag("FirstRound"):toBool()) then
						table.insert(skill_names,"chunxi")
						table.insert(skill_names,"xfd_xingyun")
					end
					--if current:objectName()~=source:objectName() then
					--	table.insert(skill_names, "yongheng")
					--end
				end
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
--carduse类也需要细化
    if event==sgs.PreCardUsed then-- // for AI to filter events only.
		local use = data:toCardUse()
		if use.from and use.from:objectName() ~= player:objectName() then
			if player:hasSkill("chuanghuan") then
				if use.card:isKindOf("BasicCard") or use.card:isKindOf("TrickCard") then
					if use.to:length()==1 then
						table.insert(skill_names, "xushi")
					end
				end
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
	--使用技能卡也叫cardused。。。。。
	if event==sgs.CardUsed then
		local use = data:toCardUse()
		if use.from and use.from:objectName() == player:objectName() then
			if use.from:hasSkill("chuanghuan") then	
				if use.card and use.card:isKindOf("Slash") then
					table.insert(skill_names,"tymhhuweivs" )
					table.insert(skill_names, "shende")
					table.insert(skill_names, "lianxivs")

					if use.from:getPhase() ==sgs.Player_Play then
						table.insert(skill_names,"shikong" )
					end
				end
				if use.card and use.card:isNDTrick() then
					if use.from:getPhase() ==sgs.Player_Play then
						table.insert(skill_names,"zuiyue")
					end
				end
			else
				if use.card and use.card:isNDTrick()  then
					 if not use.card:isKindOf("Nullification") then
						table.insert(skill_names,"bolan")
					 end
				end
				if use.card  and use.from:getPhase() ==sgs.Player_Play 
					and (use.card:isKindOf("Peach") or use.card:isKindOf("Analeptic")) then
					table.insert(skill_names,"doujiu")
				end
			end
			
		end
		if use.card and use.card:isKindOf("AmazingGrace") then
			table.insert(skill_names,"shouhuo")
		end
		if use.card and use.card:isKindOf("TrickCard") and not use.card:isNDTrick()  then
			table.insert(skill_names,"songjing")
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	
	end
    if event==sgs.TargetConfirming then
		local use = data:toCardUse()
		to=find_use_to(use.to,player)
		if to  then 
			if use.card and use.card:isKindOf("Slash")  then
				if use.to:first():objectName() == player:objectName() then
					table.insert(skill_names,"lingqi")
				end
				if source:inMyAttackRange(player) then
					table.insert(skill_names,"zhancao")
					if source:distanceTo(player) <2 and use.from:objectName()~=source:objectName() then
						table.insert(skill_names,"diaoping")
					end
				end
			end
			if use.card and use.card:isNDTrick() then
				if use.to:first():objectName() == player:objectName() then
					table.insert(skill_names,"lingqi")
				end
			end
			if player:hasSkill("chuanghuan") then
				if use.card and (use.card:isKindOf("Slash") or use.card:isNDTrick())then
					table.insert(skill_names,"huiwu")
				end
				if use.card and use.card:isKindOf("Slash") then
					table.insert(skill_names,"wangwu")
					table.insert(skill_names,"lxhuanshi")
					table.insert(skill_names,"gelong")
					table.insert(skill_names,"wushou")
					table.insert(skill_names,"longwei")
					if use.card:isBlack() then
						table.insert(skill_names,"zhengyi")
					end
				end
				if use.card and use.card:isKindOf("BasicCard") then
					if use.from:objectName()~=source:objectName() then
						table.insert(skill_names,"huisheng")
					end
				end
				if use.card and use.card:isNDTrick() then
					if use.from:objectName()~=source:objectName() then
						table.insert(skill_names,"wangwu")
						table.insert(skill_names,"weizhuang")
						table.insert(skill_names,"huisheng")
						table.insert(skill_names,"longwei")
					end
					if use.card:isBlack() then
						table.insert(skill_names,"zhengyi")
					end
				end
			end
				
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    if event==sgs.TargetConfirmed then
		local use = data:toCardUse()
		if use.from and use.from:objectName() == player:objectName() then
			if use.from:hasSkill("chuanghuan") then
				if use.card and use.card:isKindOf("Slash") then
					table.insert(skill_names,"guaili")
					if not use.card:isRed() then
						table.insert(skill_names,"louguan")
					end
					if not use.card:isBlack() then
						table.insert(skill_names,"bailou")
					end
					
				end
				if use.to:length()==1 and use.to:first():objectName() ~=source:objectName() then
					table.insert(skill_names,"ningshi")
				end
			end
		end
		to=find_use_to(use.to,player)
		if to then
			if player:hasSkill("chuanghuan") then
				if use.from:objectName()~=source:objectName() and (not use.from:inMyAttackRange(source)) then
					if use.card:isNDTrick() then
						table.insert(skill_names,"yunshang")
					end
				end
			end
		end	
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    --sgs.CardEffect, // for AI to filter events only
    if event==sgs.CardEffected then
		local effect = data:toCardEffect()
        if effect.to and effect.to:objectName() == player:objectName() then
			if effect.to:hasSkill("chuanghuan") then
				if effect.card:isKindOf("Analeptic") then
					table.insert(skill_names,"haoyin")
				end
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    if event==sgs.PostCardEffected then
		local effect = data:toCardEffect()
        if effect.to and effect.to:objectName() == player:objectName() then
			if effect.to:hasSkill("chuanghuan") then
				if effect.card:isNDTrick()then
					table.insert(skill_names,"ruizhi")
				end
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    if event==sgs.CardFinished then
		local use = data:toCardUse()
		if use.from and use.from:objectName()==player:objectName() then
			if use.from:hasSkill("chuanghuan") then
				if use.card:isNDTrick() and (not use.card:isKindOf("Nullification")) then
					table.insert(skill_names,"yaoshu")
				end
			end
		end
		if #skill_names>0 then
			select_chuanghuan(source,skill_names)
		end
	end
    --sgs.TrickCardCanceling,

    --sgs.PreMarkChange ,
    --sgs.MarkChanged ,

    --sgs.ChoiceMade,

    --sgs.StageChange, // For hulao pass only
    --sgs.FetchDrawPileCard, // For miniscenarios only
    --sgs.ActionedReset, // For 3v3 only
   -- sgs.Debut, // For 1v1 only

    --sgs.TurnBroken, // For the skill 'DanShou'. Do not use it to trigger events

    --sgs.NumOfEvents
			

end,--ontrigger的end

}
chuanghuan_attach_self=sgs.CreateTriggerSkill{
    name = "#chuanghuan_self",
	priority=3,--优先级应该是？？
	events = {sgs.EventPhaseStart,sgs.EventPhaseChanging,sgs.DrawNCards,sgs.AfterDrawNCards,sgs.PreHpRecover,sgs.HpRecover,sgs.PostHpReduced,sgs.StartJudge,sgs.AskForRetrial,sgs.FinishJudge,sgs.TurnedOver,sgs.Predamage, sgs.DamageForseen, sgs.DamageCaused,sgs.DamageInflicted, sgs.PreDamageDone, sgs.Damage,sgs.Damaged,sgs.Dying,sgs.AskForPeaches,sgs.Death,sgs.SlashProceed,sgs.SlashMissed,sgs.CardAsked,sgs.CardResponded,sgs.BeforeCardsMove,  sgs.CardsMoveOneTime,sgs.PreCardUsed, sgs.CardUsed,sgs.TargetConfirming,sgs.TargetConfirmed, sgs.CardEffected,sgs.PostCardEffected,sgs.CardFinished},
		
		
	on_trigger = function(self, event, player, data)     
		local room = player:getRoom() --player必须是创幻者本身
		local skill_names={}
		if event==sgs.PostHpReduced then
			if player:getHp()<1 then 
				table.insert(skill_names,"hpymsiyu")
				
			end
			table.insert(skill_names,"huisu")
			
			if #skill_names>0 then
				select_chuanghuan(player,skill_names)
			end
		end
		if event==sgs.TurnedOver then
			table.insert(skill_names,"jiyi")
			
			if #skill_names>0 then
				select_chuanghuan(player,skill_names)
			end
		end
			
	end,	
			
 }     
 acquire_generals= function(player, n) 
	local room =player:getRoom()
	local names
	huanxiangs = player:getTag("huanxiangs"):toString()--toStringList()
    if #huanxiangs==0 or huanxiangs[1]=="" then
		names={}
	else
		names = huanxiangs:split("+") --table
	end
	list = get_available_generals(player) --stringlist? table?
	--if list:isEmpty() then return false end
	
	
	n = math.min(n, #list)
	--nums =sgs.IntList()
	new_huanxiangs=touhou_shuffle(list)
	

	for var=1, n,1 do
		table.insert(names, new_huanxiangs[var])

		local log = sgs.LogMessage()
		general = sgs.Sanguosha:getGeneral(new_huanxiangs[var])
        log.type = "#chuanghuangain"
        log.from = player
        log.arg =general:objectName() 
        room:sendLog(log)
		
	end
	tag = sgs.QVariant(table.concat(names, "+")) 
	--table.contains(attackrange, to_select:objectName())

	player:setTag("huanxiangs",tag)
	room:setPlayerMark(player,"@huanxiangs",player:getMark("@huanxiangs")+n)
 end
 generals_contain= function(all,name)
	t=false
	names=all:split("+")
	if table.contains(names, name) then
		t=true
	end

	return t
 end
 generals_alive= function(room,name) 
 	t=false
	for _,p in sgs.qlist(room:getAlivePlayers()) do
		name1 = p:getGeneralName()
		if name==name1 then
			t =true
			break
		end
	end
	return t
 end
 get_available_generals= function(player)
	--没有qset？

	local room =player:getRoom()
	 local available = {}
	 --local count =0
	all =player:getTag("huanxiangs"):toString()--还有别的法子初始化么、、、
	--zhu
	zhu_num=8
	for var=1, zhu_num, 1 do
		--name ="zhu00"..tostring(var)
		name =""
		if var<10 then
			name ="zhu00"..tostring(var)
		else
			name ="zhu0"..tostring(var)
		end
		if generals_contain(all,name) or generals_alive(room,name)  then
			
		else
			table.insert(available, name)
			--count =count+1
			
		end 
	end
	--hmx
		hmx_num=9
	for var=1, hmx_num, 1 do
		--name ="hmx00"..tostring(var)
		name=""
		if var<10 then
			name ="hmx00"..tostring(var)
		else
			name ="hmx0"..tostring(var)
		end
		if generals_contain(all,name) or generals_alive(room,name) then
			--all:removeOne(name)
		else
			table.insert(available, name)
			--count =count+1
			--all:append(name)
		end 
	end
	--yym
	yym_num=13
	for var=1, yym_num, 1 do
		name=""
		if var<10 then
			name ="yym00"..tostring(var)
		else
			name ="yym0"..tostring(var)
		end
		if generals_contain(all,name) or generals_alive(room,name) then
			--all:removeOne(name)
		else
			table.insert(available, name)
			--count =count+1
			--all:append(name)
		end 
	end
	--yyc
	yyc_num=10
	for var=1, yyc_num, 1 do
		name=""
		if var<10 then
			name ="yyc00"..tostring(var)
		else
			name ="yyc0"..tostring(var)
		end
		if generals_contain(all,name) or generals_alive(room,name) then
			--all:removeOne(name)
		else
			table.insert(available, name)
			--count =count+1
			--all:append(name)
		end 
	end
	--zhan
	zhan_num=9
	for var=1, zhan_num, 1 do
		name=""
		if var<10 then
			name ="zhan00"..tostring(var)
		else
			name ="zhan0"..tostring(var)
		end
		if generals_contain(all,name) or generals_alive(room,name) then
			--all:removeOne(name)
		else
			table.insert(available, name)
			--count =count+1
			--all:append(name)
		end 
	end
	--fsl
	fsl_num=11
	for var=1, fsl_num, 1 do
		name=""
		if var<10 then
			name ="fsl00"..tostring(var)
		else
			name ="fsl0"..tostring(var)
		end
		if generals_contain(all,name) or generals_alive(room,name) then
			--all:removeOne(name)
		else
			table.insert(available, name)
			--count =count+1
			--all:append(name)
		end 
	end
	--dld
		dld_num=9
	for var=3, dld_num, 1 do
		name=""
		if var<10 then
			name ="dld00"..tostring(var)
		else
			name ="dld0"..tostring(var)
		end
		if generals_contain(all,name) or generals_alive(room,name) then
			--all:removeOne(name)
		else
			table.insert(available, name)
			--count =count+1
			--all:append(name)
		end 
	end
	--xlc
		xlc_num=8
	for var=1, xlc_num, 1 do
		name=""
		if var<10 then
			name ="xlc00"..tostring(var)
		else
			
			name ="xlc0"..tostring(var)
		end
		if generals_contain(all,name) or generals_alive(room,name) then
			--all:removeOne(name)
		else
			table.insert(available, name)
			--count =count+1
			--all:append(name)
		end 
	end
	--slm
	slm_num=10
	for var=1, slm_num, 1 do
		name=""
		if var<10 then
			name ="slm00"..tostring(var)
		else
			name ="slm0"..tostring(var)
		end
		if generals_contain(all,name) or generals_alive(room,name) then
			--all:removeOne(name)
		else
			table.insert(available, name)
			--count =count+1
			--all:append(name)
		end 
	end
	--wai
	wai_num=10
	for var=1, wai_num, 1 do
		name=""
		if var<10 then
			name ="wai00"..tostring(var)
		else
			name ="wai0"..tostring(var)
		end
		if generals_contain(all,name) or generals_alive(room,name) then
			--all:removeOne(name)
		else
			table.insert(available, name)
			--count =count+1
			--all:append(name)
		end 
	end
	--sp
		sp_num=6
	for var=1, sp_num, 1 do
		name=""
		if var<10 then
			name ="sp00"..tostring(var)
		else
			name ="sp0"..tostring(var)
		end
		if generals_contain(all,name) or generals_alive(room,name) then
			--all:removeOne(name)
		else
			table.insert(available, name)
			--count =count+1
			--all:append(name)
		end 
	end
	--touhougod
	touhougod_num=17
	for var=1, touhougod_num, 1 do
		name=""
		if var<10 then
			name ="shen00"..tostring(var)
		else
			name ="shen0"..tostring(var)
		end
		if generals_contain(all,name) or generals_alive(room,name) then
			--all:removeOne(name)
		else
			table.insert(available, name)
			--count =count+1
			--all:append(name)
		end 
	end

	--all还要根据mod来裁
	-- banned部分先不管
	return available --return count    - huanxiang_set - room_set).toList() --- banned
 end
 chuanghuan_card = sgs.CreateSkillCard{ 
	name = "chuanghuan",
	target_fixed = true,
	will_throw = false,
	
	on_use = function(self, room, source, targets)
		choices="chuanghuan_browse+chuanghuan_select+cancel"
		choice = room:askForChoice(source, "chuanghuan", choices)
		if choice =="chuanghuan_select" then
			skill_table=chuanghuan_attach_PlayViewAs
			select_chuanghuan(source,skill_table)
		end
		if choice =="chuanghuan_browse" then
			huanxiangs = source:getTag("huanxiangs"):toString()
			g_names = huanxiangs:split("+") --table
			generals={}
			for var=1,#g_names,1 do
				g_name=g_names[var]
				general = sgs.Sanguosha:getGeneral(g_name)
				if general then
				table.insert(generals,general:objectName()) 
				end
			end
			g_real_name = room:askForChoice(source, "chuanghuan", table.concat(generals, "+"))
		end
	end,
}
 
 chuanghuanVS = sgs.CreateViewAsSkill{
	name = "chuanghuan",
	n = 0,
	--目前只考虑在出牌阶段的viewas技
	enabled_at_play = function(self,player)
		return player:getMark("@huanxiangs")>0
	end,

	view_as = function(self, cards)
		local qcard = chuanghuan_card:clone()
		qcard:setSkillName(self:objectName())
		return qcard
	end
} 
 chuanghuan=sgs.CreateTriggerSkill{
	name = "chuanghuan",
	frequency = sgs.Skill_Eternal,
	events={sgs.GameStart,sgs.EventPhaseStart},
	view_as_skill=chuanghuanVS,
	priority=4,
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		player:getRoom():notifySkillInvoked(player, "chuanghuan");
        if event==sgs.GameStart then
		local log=sgs.LogMessage()
				log.type = "#TriggerSkill"
				log.from = player
				log.arg = "chuanghuan"
				room:sendLog(log)
				room:notifySkillInvoked(player, self:objectName())
		acquire_generals(player, 4)
		else
			if event==sgs.EventPhaseStart and (player:getPhase() == sgs.Player_Start) then
				local log=sgs.LogMessage()
				log.type = "#TriggerSkill"
				log.from = player
				log.arg = "chuanghuan"
				room:sendLog(log)
				room:notifySkillInvoked(player, self:objectName())
				acquire_generals(player, 2)
			end
		end
	end,
 }
zun_skill_table={"shenzhu","chuanghuan","#chuanghuan_detach","#chuanghuan_self","#chuanghuan"}
 chuanghuan_detach=sgs.CreateTriggerSkill{
	name = "#chuanghuan_detach",
	events={sgs.EventPhaseEnd,sgs.CardFinished,sgs.CardResponded},
	priority=-1,
	can_trigger = function(self, player)
        return player 
    end ,
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		current= room:getCurrent()
		local phase = current:getPhase()
		local source = room:findPlayerBySkillName(self:objectName()) 
        local skillList=sgs.SkillList()
        if event ==sgs.CardFinished then
			local use = data:toCardUse()
			local log=sgs.LogMessage()
				log.type = "#TriggerSkill"
				log.from = source
				log.arg = "chuanghuan"
				
			if use.card and use.card:getSkillName() =="zhanyi" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-zhanyi")
			end
			if use.card and use.card:getSkillName() =="yuzhu" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-yuzhu")
			end
			if use.card and use.card:getSkillName() =="changqingpeach" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-changqing")
			end
			if use.card and use.card:getSkillName() =="leiyun" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-leiyun")
			end
			if use.card and use.card:getSkillName() =="jidu" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-jidu")
			end
			if use.card and use.card:getSkillName() =="fengrang" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-fengrang")
			end
			if use.card and use.card:getSkillName() =="baigui" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-baigui")
			end
			if use.card and use.card:getSkillName() =="jiuchong" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-jiuchong")
			end
			if use.card and use.card:getSkillName() =="duzhua" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-duzhua")
			end
			if use.card and use.card:getSkillName() =="gesheng" then
				if source:objectName()~=player:objectName() then return false end
				if source:getMark("gesheng_used")>0 then
					room:setPlayerMark(source, "gesheng_used",0)
					room:sendLog(log)
					room:handleAcquireDetachSkills(source, "-gesheng")
				else
					source:addMark("gesheng_used")
				end
			end
		end
		if event==sgs.CardResponded then
			card_star = data:toCardResponse().m_card
			if card_star and card_star:getSkillName() =="zhanyi" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-zhanyi")
			end
			if card_star and card_star:getSkillName() =="yuzhu" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-yuzhu")
			end
		end
		
		if event==sgs.EventPhaseEnd and phase == sgs.Player_Finish then
			for _,skill in  sgs.qlist(source:getVisibleSkillList()) do --skilllist
				if not table.contains(zun_skill_table,skill:objectName()) then
					skillList:append(skill)
				end
			end
			for _,skill in sgs.qlist(skillList) do
				local log=sgs.LogMessage()
				log.type = "#TriggerSkill"
				log.from = source
				log.arg = "chuanghuan"
				room:sendLog(log)
				room:handleAcquireDetachSkills(source,"-"..skill:objectName())
			end
		end
		
	end,
 } 
 shenzhu=sgs.CreateTriggerSkill{
	name="shenzhu",
	frequency = sgs.Skill_Eternal,
	events={sgs.GameStart},
	
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		if event==sgs.GameStart then
			room =player:getRoom()
			n=room:getAlivePlayers():length()
			local value = math.ceil(n/2)
			local log=sgs.LogMessage()
				log.type = "#TriggerSkill"
				log.from = player
				log.arg = self:objectName()
				room:sendLog(log)
				room:notifySkillInvoked(player, self:objectName())
				room:setPlayerProperty(player,"maxhp",sgs.QVariant(value))
				room:setPlayerProperty(player,"hp",sgs.QVariant(value))
		end
	end,
}

                                
 --shen000:addSkill(chuanghuan)
 --shen000:addSkill(chuanghuan_attach)--触发获得技能 种类很杂
 --shen000:addSkill(chuanghuan_attach_self)--触发获得技能 触发者只为自己时
 --shen000:addSkill(chuanghuan_detach)--回合结束后清除一些获得的技能
 --shen000:addSkill(shenzhu)
 -- extension:insertRelatedSkills("chuanghuan", "#chuanghuan")
  --extension:insertRelatedSkills("chuanghuan", "#chuanghuan_self")
  --extension:insertRelatedSkills("chuanghuan", "#chuanghuan_detach") 



--【大结界的守护者——博丽灵梦】编号：00011 by三国有单
--要重新修改。。。
--[源码改动]Player:distanceTo()
--shen011 = sgs.General(extension, "shen011", "touhougod", 4, false) 
--[[jiejie_seat_distance = function(room,from,to,other)
	right = from:getSeat() - to:getSeat()
	if right <0 then
		right = to:getSeat()-from:getSeat()
	end
	left = room:getAlivePlayers():length() - right
	if other then
		if left >right then
			return right
		else
			return left
		end
	else
		if left >right then
			return left
		else
			return right
		end
	end
end
]]
--[[jiejie_distance_handle = function(room)--管理距离函数
	
	for _,to in sgs.qlist(room:getAlivePlayers()) do	
		for _,from in sgs.qlist(room:getOtherPlayers(to)) do
			if to:getMark("jiejie"..from:objectName())>0 and from:getMark("jiejie"..to:objectName())>0 then
				continue
			end
			--留一个针对彼岸和高顺陷阵的特殊对应 直接continue跳出循环
			if from:getMark("@bian")>0 and to:hasSkill("bian") then continue end
			if to:getMark("@bian")>0 and from:hasSkill("bian") then continue end
			
			local find_jiejie=false
			local find_jiejie1=false
			clockcount=0
			anti_clockcount=0
			--计算从from出发顺时针计算的情况
			if from:getSeat()-to:getSeat() >0 then
				dis =from:getSeat()-to:getSeat()
				if from:getPile("jiejie_left"):length()>0 or to:getPile("jiejie_right"):length()>0 then
					find_jiejie =true
				end
				for _,p in sgs.list(room:getOtherPlayers(from)) do
					if p:getSeat()-to:getSeat()> 0 and p:getSeat()-to:getSeat()<dis then
						clockcount=clockcount+1
						if p:getPile("jiejie_left"):length()>0 or p:getPile("jiejie_right"):length()>0 then
							find_jiejie =true
						end
					end
				end
				if from:getPile("jiejie_right"):length()>0 or to:getPile("jiejie_left"):length()>0 then
					find_jiejie1 =true
				end
				for _,p in sgs.list(room:getOtherPlayers(from)) do
					if p:getSeat()-to:getSeat()< 0 or  p:getSeat()-from:getSeat()>0 then
						anti_clockcount=anti_clockcount+1
						if p:getPile("jiejie_left"):length()>0 or p:getPile("jiejie_right"):length()>0 then
							find_jiejie1 =true
						end
					end
				end
			end
			--计算从from出发逆时针计算的情况
			if from:getSeat()-to:getSeat() <0 then
				dis =from:getSeat()-to:getSeat()
				if from:getPile("jiejie_right"):length()>0 or to:getPile("jiejie_left"):length()>0 then
					find_jiejie1 =true	
				end
				for _,p in sgs.list(room:getOtherPlayers(from)) do
					if p:getSeat()-to:getSeat()< 0 and p:getSeat()-to:getSeat()>dis then
						anti_clockcount=anti_clockcount+1
						if p:getPile("jiejie_left"):length()>0 or p:getPile("jiejie_right"):length()>0 then
							find_jiejie1 =true
						end
					end
				end
				if from:getPile("jiejie_left"):length()>0 or to:getPile("jiejie_right"):length()>0 then
					find_jiejie =true
				end
				for _,p in sgs.list(room:getOtherPlayers(from)) do
					if p:getSeat()-to:getSeat()> 0 or  p:getSeat()-from:getSeat()<0 then
						clockcount=clockcount+1
						if p:getPile("jiejie_left"):length()>0 or p:getPile("jiejie_right"):length()>0 then
							find_jiejie =true
						end
					end
				end
			end
			if find_jiejie and find_jiejie1  then
				--有没有可以查询两者本身就是distancefix状态的？
				room:setFixedDistance(from,to,900)
                room:setFixedDistance(to,from,900)
			else
				room:setFixedDistance(from,to,-1)
				room:setFixedDistance(to,from,-1)
				if find_jiejie then --from到to顺时针存在结界
					--保留其他影响距离因子
					differ = from:distanceTo(to) - jiejie_seat_distance(room,from,to,true)
					differ1 = to:distanceTo(from) - jiejie_seat_distance(room,from,to,true)
					
					basic_distance =anti_clockcount+1
					
					room:setFixedDistance(from,to,basic_distance+differ)
					room:setFixedDistance(to,from,basic_distance+differ1)
				end
				if find_jiejie1 then
					differ = from:distanceTo(to) - jiejie_seat_distance(room,from,to,true)
					differ1 = to:distanceTo(from) - jiejie_seat_distance(room,from,to,true)
					
					basic_distance =clockcount+1
					
					room:setFixedDistance(from,to,basic_distance+differ)
					room:setFixedDistance(to,from,basic_distance+differ1)
				end
			end
			--避免重复计算 设立mark 表示计算过这一对玩家的距离 计算量开销很大的。。。		
			room:setPlayerMark(from,"jiejie"..to:objectName(),1)
			room:setPlayerMark(to,"jiejie"..from:objectName(),1)
		end
	end
	--消除mark
	for _,to in sgs.qlist(room:getAlivePlayers()) do	
		for _,from in sgs.qlist(room:getOtherPlayers(to)) do
			room:setPlayerMark(from,"jiejie"..to:objectName(),0)
			room:setPlayerMark(to,"jiejie"..from:objectName(),0)
		end
	end
end
]]
jiejie_card = sgs.CreateSkillCard{--结界技能卡 
	name = "jiejie",
	target_fixed = false,
	will_throw = true,
	
	filter = function(self, targets, to_select, player)
		return #targets == 0 
	end,
	
	on_use = function(self, room, source, targets)
		--派发给每个角色  结界存在时管理距离的技能。。。
		for _,liege in sgs.qlist(room:getAlivePlayers()) do
			if (not liege:hasSkill("jiejie_confirm")) then
				room:acquireSkill(liege,"jiejie_confirm")
			end
			--if (not liege:hasSkill("#jiejie_handle")) then
			--	room:acquireSkill(liege,"#jiejie_handle")
			--end
		end
		local target = targets[1]		
		card_ids =room:getNCards(2)
		card_id=card_ids:first()
		target:setFlags("jiejie_move")--for 感应
		target:addToPile("jiejie_left", card_id)
		target:setFlags("jiejie_move")
		card_ids:removeOne(card_id)
		card_id=card_ids:first()
		target:addToPile("jiejie_right", card_id)
		if target:getMark("@in_jiejie")== 0 then
			target:gainMark("@in_jiejie",1)
		end
		lefter,righter =jiejie_adjacent(target)
		if lefter:getMark("@in_jiejie")== 0 then
			if lefter:getPile("jiejie_left"):length()>0 then
				lefter:gainMark("@in_jiejie",1)
			else
				lefter1,righter1 =jiejie_adjacent(lefter)
				if lefter1:getPile("jiejie_right"):length()>0 then
					lefter:gainMark("@in_jiejie",1)
				end
			end
		end
		if righter:getMark("@in_jiejie")== 0 then
			if righter:getPile("jiejie_right"):length()>0 then
				righter:gainMark("@in_jiejie",1)
			else
				lefter1,righter1 =jiejie_adjacent(righter)
				if righter1:getPile("jiejie_left"):length()>0 then
					righter:gainMark("@in_jiejie",1)
				end
			end
		end
		--jiejie_distance_handle(room)
	end,
}
jiejie= sgs.CreateViewAsSkill{--技能"结界"
	name = "jiejie",
	n = 1,
	
	enabled_at_play = function(self,player)
		return true --not player:hasUsed("#jiejie")
	end,
	
	view_filter = function(self, selected, to_select)
		return not to_select:isEquipped()
	end,
	
	view_as = function(self, cards)
		if(#cards ~= 1) then return nil end
		local qcard = jiejie_card:clone()
		qcard:addSubcard(cards[1])
		qcard:setSkillName(self:objectName())
		return qcard
	end
}
--计算结界本身的距离不受人物的距离技能影响

get_jiejie_list= function(player)--提供可以指定的"结界player"列表
	local jiejie_owners_left={} --左结界
	local jiejie_owners_right={} --右结界
	local targets={}
	local lefter=player
	local righter=player

	if player:getPile("jiejie_left"):length()>0 then
		table.insert(jiejie_owners_left,player:objectName())
	end
	if player:getPile("jiejie_right"):length()>0 then
		table.insert(jiejie_owners_right,player:objectName())
	end
	for _,p in sgs.qlist(player:getAliveSiblings()) do	
		if p:getPile("jiejie_left"):length()>0  then 
			--从player的逆时针开始
			if player:getPile("jiejie_right"):length()==0 then
					dis1=0
					local block=false
					righter=player
					for var=1, player:getAliveSiblings():length(),1 do
						lefter,righter =find_adjacent(righter)
						if righter:objectName() ==p:objectName() then
							dis1=dis1+1
							break
						else
							if righter:getPile("jiejie_left"):length()>0 
							or righter:getPile("jiejie_right"):length()>0 then
								block=true
								break
							else
								dis1=dis1+1
							end
						end
					end
					if  not block and player:getAttackRange(true)>=dis1 then
						table.insert(jiejie_owners_left,p:objectName())
					end
				end
				--从player的顺时针开始
				if  not table.contains(jiejie_owners_left,p:objectName()) and p:getPile("jiejie_right"):length()==0 and player:getPile("jiejie_left"):length()==0  then
					dis2=0
					local block=false
					lefter=player
					for var=1, player:getAliveSiblings():length(),1 do
						lefter,righter =find_adjacent(lefter)
						if lefter:objectName() ==p:objectName() then
							dis2=dis2+2
							break
						else
							if lefter:getPile("jiejie_left"):length()>0 
							or lefter:getPile("jiejie_right"):length()>0 then
								block =true
								break
							else	
							dis2=dis2+1
							end
						end
					end
					if not block and player:getAttackRange(true)>=dis2 then
						table.insert(jiejie_owners_left,p:objectName())
					end
				end
		end		
		if p:getPile("jiejie_right"):length()>0  then 
			--从player的顺时针开始
			if player:getPile("jiejie_left"):length()==0 then
				dis2=0
				local block=false
				lefter=player
				for var=1, player:getAliveSiblings():length(),1 do
					lefter,righter =find_adjacent(lefter)
					if lefter:objectName() ==p:objectName() then
						dis2=dis2+1
						break
					else	
						if lefter:getPile("jiejie_left"):length()>0 
						or lefter:getPile("jiejie_right"):length()>0 then
							block=true
							break
						else
							dis2=dis2+1
						end
					end
				end
				if not block and player:getAttackRange(true)>=dis2 then
					table.insert(jiejie_owners_right,p:objectName())
				end
			end
			--从player的逆时针开始
			if  not table.contains(jiejie_owners_right,p:objectName()) and p:getPile("jiejie_left"):length()==0 and player:getPile("jiejie_right"):length()==0 then
				dis1=0
				local block=false
				righter=player
				for var=1, player:getAliveSiblings():length(),1 do
					lefter,righter =find_adjacent(righter)
					if righter:objectName() ==p:objectName() then
						dis1=dis1+2
						break
					else
						if righter:getPile("jiejie_left"):length()>0 
						or righter:getPile("jiejie_right"):length()>0 then
							block=true
							break
						else
							dis1=dis1+1
						end
					end
				end
				if not block and player:getAttackRange(true)>=dis1 then
					table.insert(jiejie_owners_right,p:objectName())
				end
			end
		end						
	end
	return jiejie_owners_left,jiejie_owners_right
end
jiejie_confirm_card = sgs.CreateSkillCard{ 
	name = "jiejie_confirm",
	target_fixed = false,
	will_throw = true,
	
	filter = function(self, targets, to_select, player)
		--[[if #targets<1 then
			if player:objectName()==to_select:objectName() then
				return player:getPile("jiejie_right"):length()>0
				or player:getPile("jiejie_left"):length()>0
			end
			local seat1,seat2=adjust_jiejie_block(player)
			if seat1==0 and seat2==0 then return false end
			t1,t2= parse_jiejie_block(player,to_select,seat1,seat2)
			return t1 or t2
		end]]
		if #targets<1 then
			local jiejie_owners --左结界
			local jiejie_owners1 --右结界
			jiejie_owners,jiejie_owners1 =get_jiejie_list(player)
			if #jiejie_owners==0 and #jiejie_owners1 ==0 then return false end
			return table.contains(jiejie_owners,to_select:objectName()) 
			or table.contains(jiejie_owners1,to_select:objectName())
		end
	end,
	
	on_use = function(self, room, source, targets)		
		local jiejie_owners --左结界
		local jiejie_owners1 --右结界
		jiejie_owners,jiejie_owners1 =get_jiejie_list(source)
		local choices={}
		
		
		
		if table.contains(jiejie_owners,targets[1]:objectName()) then
		
				table.insert(choices, "jiejie_left")
			end
		if table.contains(jiejie_owners1,targets[1]:objectName()) then
				table.insert(choices, "jiejie_right")
		end

		local choice = room:askForChoice(source,self:objectName(),table.concat(choices,"+"))
		local reason = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, "", nil, self:objectName(), "")
        
		local move=sgs.CardsMoveStruct()
		move.to_place=sgs.Player_DiscardPile
		move.to=nil
		move.reason=sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, "", nil, self:objectName(), "")
		
		
		if choice == "jiejie_left" then
			move.card_ids:append(targets[1]:getPile("jiejie_left"):first())
	
			--room:throwCard(sgs.Sanguosha:getCard(targets[1]:getPile("jiejie_left"):first()), reason, nil)
		else
			move.card_ids:append(targets[1]:getPile("jiejie_right"):first())
	
			--room:throwCard(sgs.Sanguosha:getCard(targets[1]:getPile("jiejie_right"):first()), reason, nil)
		end
		room:moveCardsAtomic(move, true)
		adjust_jiejie_mark(room)
	end,
}
jiejie_confirm=sgs.CreateViewAsSkill{--用杀指定"结界"
	name = "jiejie_confirm",
	n = 1,
	view_filter = function(self, selected, to_select)
		return to_select:isKindOf("Slash")
	end ,
	enabled_at_play = function(self,player)
		return not player:isKongcheng()
	end,
	
	view_as = function(self, cards)
		if #cards==1 then
			local qcard = jiejie_confirm_card:clone()
			qcard:setSkillName(self:objectName())
			qcard:addSubcard(cards[1])
			return qcard
		end
	end
}

adjust_jiejie_mark=function(room)
	for _,p in sgs.list(room:getAlivePlayers()) do
		if p:getMark("@in_jiejie")> 0 then
			lefter,righter =find_adjacent(p)
			if p:getPile("jiejie_left"):length()==0 and lefter:getPile("jiejie_right"):length()==0 then
				p:loseMark("@in_jiejie")
			end
			if p:getPile("jiejie_right"):length()==0 and righter:getPile("jiejie_left"):length()==0 then
				p:loseMark("@in_jiejie")
			end
		end
	end
end
--[[adjust_jiejie_block=function(player)--把结界也当作一个点，记录全局的点
	local nodes={}
	local seat1,seat2
	local targets=player:getAliveSiblings()
	targets:append(player)
	local alivecount=targets:length()
	for var=1,alivecount*2,1 do
		table.insert(nodes,0)
	end
	for _,p in sgs.qlist(targets) do
		local left=p:getPile("jiejie_left"):length()
		local right=p:getPile("jiejie_left"):length()
		local seat=p:getSeat()
		if right>0 then
			if seat==alivecount then
				nodes[1]=nodes[1]+right
			else
				nodes[seat*2+1]=nodes[seat*2+1]+right
			end
		end
		if left>0 then
			nodes[seat*2-1]=nodes[seat*2-1]+left
		end
	end
	--增减结界
	--nodes[jiejie_node]=nodes[jiejie_node]+fix
	--给每个player计入中断点
	for var=player:getSeat()*2, alivecount*2,1 do
		if nodes[var]>0 then
			seat1=var
			break
		end
	end
	for var=player:getSeat()*2, 1,-1 do
		if nodes[var]>0 then
			seat2=var
			break
		end
	end
	if seat1 and seat2 then
		return seat1,seat2
	else
		if not seat1 and not seat2 then return 0,0 end
		if seat1 then
			for var=alivecount*2, seat1,-1 do
				if nodes[var]>0 then
					seat2=var
					break
				end
			end
		else
			for var=1, seat2,1 do
				if nodes[var]>0 then
					seat1=var
					break
				end
			end
		end
		return seat1,seat2
	end	
end
parse_jiejie_block=function(player,to_select,seat1,seat2)
	local left_seat=false
	local right_seat=false
	local small
	local large
	if player:getSeat()>to_select:getSeat()
		large=player:getSeat()
		small=to_select:getSeat()
	else
		large=to_select:getSeat()
		small=player:getSeat()
	end
	dis1=
	dis2=
	if to_select:getPile("jiejie_left"):length()==0 and  to_select:getPile("jiejie_right"):length()==0 then
		return false,false
	end
	if to_select:getPile("jiejie_left"):length()>0 then
		local seat=to_select:getSeat()*2-1
		if seat==seat1 then
			player:getAttackRange(true)
			
			left_seat=true
		end
	end
	if to_select:getPile("jiejie_right"):length()>0 then
		local seat=to_select:getSeat()*2+1
		if seat>(player:getAliveSiblings():length()+1)*2 then
			seat=1
		end
		if seat==seat1 or seat==seat2 then
			right_seat=true
		end
	end
	return left_seat,right_seat
end]]
--[[jiejie_handle=sgs.CreateTriggerSkill{-- 管理距离
	name = "#jiejie_handle",
	events = {sgs.HpChanged,sgs.Death,sgs.CardsMoveOneTime,sgs.MarkChanged},
	--move马匹进入装备区 MarkChanged 天仪魔里沙玩装备 得到彼岸标记改变距离... HpChanged公孙瓒一类 还有别的情况吗(=_=)枚举真恶心 
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		--move的情况还要细化  去掉不必要的 因为自定义函数计算量太大。。。
		--if event==sgs.BeforeCardsMove then		
		--	move=data:toMoveOneTime()
			
		--end
		if event ==sgs.Death or event == sgs.CardsMoveOneTime then
			if player:hasSkill("jiejie") then
				--move的情况 目前只考虑装马
				if event == sgs.CardsMoveOneTime then
					move=data:toMoveOneTime()
					if move.from_places:contains(sgs.Player_PlaceSpecial) or  move.to_place ==sgs.Player_PlaceEquip then
						jiejie_distance_handle(room)
						for _,p in sgs.list(room:getAlivePlayers()) do
							if p:getMark("@in_jiejie")> 0 then
								lefter,righter =find_adjacent(p)
								if p:getPile("jiejie_left"):length()==0 and lefter:getPile("jiejie_right"):length()==0 then
									room:setPlayerMark(p, "@in_jiejie", 0)
								end
								if p:getPile("jiejie_right"):length()==0 and righter:getPile("jiejie_left"):length()==0 then
									room:setPlayerMark(p, "@in_jiejie", 0)
								end
							end
						end
					else
						return false
					end
				end
			end
		else
			if event ==sgs.HpChanged then 
				if not player:hasSkill("fengsu") then return false end
				jiejie_distance_handle(room)
			else
				jiejie_distance_handle(room)
			end
		end
	end
}
]]
jiejie_adjacent = function(target) 
	local room=target:getRoom()
	local lefter
	local righter
	for _,p in sgs.qlist(room:getOtherPlayers(target)) do  --
		if target:isAdjacentTo(p) then
			if target:getSeat()-p:getSeat()==1 then
				lefter =p
			end
			if p:getSeat()-target:getSeat()==1 then
				righter=p
			end
			if target:getSeat()-p:getSeat()== room:getOtherPlayers(target):length()then
				righter=p
			end
			if p:getSeat()-target:getSeat()== room:getOtherPlayers(target):length()  then --room:getOtherPlayers(target):length()
				lefter=p
			end
		end
	end
	return lefter,righter
end 
find_adjacent = function(target) 
	local lefter
	local righter
	for _,p in sgs.qlist(target:getAliveSiblings()) do  --room:getOtherPlayers(target)
		if target:isAdjacentTo(p) then
			if target:getSeat()-p:getSeat()==1 then
				lefter =p
			end
			if p:getSeat()-target:getSeat()==1 then
				righter=p
			end
			if target:getSeat()-p:getSeat()== target:getAliveSiblings():length()then
				righter=p
			end
			if p:getSeat()-target:getSeat()== target:getAliveSiblings():length()  then --room:getOtherPlayers(target):length()
				lefter=p
			end
		end
	end
	return lefter,righter
end 
reimu_fengyin = sgs.CreateTriggerSkill{--技能"封印"
		name = "reimu_fengyin", 
		events = {sgs.AfterDrawNCards}, 
		on_trigger = function(self, event, player, data)
			local room = player:getRoom()
			listt =room:getOtherPlayers(player)
			local heartcard 
			local count =0
			for _,p in sgs.qlist(room:getOtherPlayers(player)) do	
				if p:getMark("@in_jiejie")== 0 then
					listt:removeOne(p)
				else
					count=count+1
				end
			end
			if count>0 then
				heartcard = room:askForCard(player, ".|heart", "@fengyin-discard", sgs.QVariant(), sgs.Card_MethodDiscard, nil, true, self:objectName())
			end
			if heartcard then
				target = room:askForPlayerChosen(player, listt , self:objectName(),"fengyin_chosenplayer",false,true)
				local damage=sgs.DamageStruct()
				damage.card = nil
				damage.damage = 1
				damage.from = player
				damage.to = target
				room:damage(damage)
				target:turnOver()
				--弃置结界牌
				local jiejie_cards
				local jiejie_cards1
				throw_jiejie_cards = sgs.IntList()
				local lefter
				local righter
				
				if target:getPile("jiejie_left"):length()>0 then
					jiejie_cards = target:getPile("jiejie_left")
				else
					lefter,righter =find_adjacent(target)--自定义函数
					jiejie_cards = lefter:getPile("jiejie_right")
				end
				if target:getPile("jiejie_right"):length()>0 then
					jiejie_cards1 = target:getPile("jiejie_right")
				else
					lefter,righter =find_adjacent(target)--自定义函数
					jiejie_cards1 = righter:getPile("jiejie_left")
				end
				throw_jiejie_cards:append(jiejie_cards:first())
				
				local move=sgs.CardsMoveStruct()
				move.card_ids=throw_jiejie_cards
				move.to_place=sgs.Player_DiscardPile
				move.to=nil
				move.reason=sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, "", nil, self:objectName(), "")
				player:setFlags("fengyin_move")
				room:moveCardsAtomic(move, true)
				player:setFlags("-fengyin_move")
				throw_jiejie_cards1 = sgs.IntList()
				throw_jiejie_cards1:append(jiejie_cards1:first())
				move.card_ids=throw_jiejie_cards1
				move.reason=sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_REMOVE_FROM_PILE, "", nil, self:objectName(), "")
				room:moveCardsAtomic(move, true)
				adjust_jiejie_mark(room)
			end
		end
}
if not sgs.Sanguosha:getSkill("jiejie_confirm") then
        local skillList=sgs.SkillList()
        skillList:append(jiejie_confirm)
        sgs.Sanguosha:addSkills(skillList)
end
--[[if not sgs.Sanguosha:getSkill("#jiejie_handle") then
        local skillList=sgs.SkillList()
        skillList:append(jiejie_handle)
        sgs.Sanguosha:addSkills(skillList)
end
]]
--shen011:addSkill(jiejie)
--shen011:addSkill(reimu_fengyin)
 



shen018 = sgs.General(extension, "shen018", "touhougod", 4, false)
god_bian = sgs.CreateTargetModSkill{
        name = "god_bian",
        --pattern="Slash,TrickCard+^DelayedTrick", --^DelayedTrick
        pattern="Slash,TrickCard",
		distance_limit_func = function(self,player,card)
                if player:hasSkill(self:objectName()) then 
                        return 1000
                else
                        return 0
                end
        end,
}

yuming = sgs.CreateTriggerSkill{
    name = "yuming" ,
    events = {sgs.ConfirmDamage} ,
    
	can_trigger = function(self, player)
        return player and player:isAlive()
    end,
    on_trigger = function(self, event, player, data)
         local damage = data:toDamage()
         damage.damage = math.max(damage.to:getHp()/2,1)
		 touhou_logmessage("#yuming_damage",damage.from,"yuming",damage.to,tostring(damage.damage)) 
	     data:setValue(damage)
    end ,
}
huanming = sgs.CreateTriggerSkill{
    name = "huanming" ,
    events = {sgs.DamageCaused} ,
    frequency = sgs.Skill_Limited,
	limit_mark = "@huanming",
    on_trigger = function(self, event, player, data)
		damage=data:toDamage()
		room=player:getRoom()
		if player:getMark("@huanming") > 0 then
			_data=sgs.QVariant()
			_data:setValue(damage.to)
			if room:askForSkillInvoke(player,self:objectName(),_data)then
				room:removePlayerMark(player, "@huanming")
				hp=player:getHp()
				hp1=damage.to:getHp()
				if player:getMaxHp()<hp1 then
					hp1=player:getMaxHp()
				end
				if damage.to:getMaxHp()<hp then
					hp=damage.to:getMaxHp()
				end
				room:setPlayerProperty(player,"hp",sgs.QVariant(hp1))
				room:setPlayerProperty(damage.to,"hp",sgs.QVariant(hp))
				return true
			end
		end
    end ,
} 
shen018:addSkill(god_bian)
shen018:addSkill(yuming)
shen018:addSkill(huanming)

shen019 = sgs.General(extension, "shen019", "touhougod", 3, false)
shen020 = sgs.General(extension, "shen020", "touhougod", 3, false)
shen021 = sgs.General(extension, "shen021", "touhougod", 4, false)
wunan = sgs.CreateTriggerSkill{
    name = "wunan" ,
    events = {sgs.ConfirmDamage} ,
    
	can_trigger = function(self, player)
        return player and player:isAlive()
    end,
    on_trigger = function(self, event, player, data)
         
    end ,
}
 shen021:addSkill(wunan)
 shen022 = sgs.General(extension, "shen022", "touhougod", 3, false)




--  【知识与历史的半兽——上白泽慧音】  编号：08005 by三国有单
--yyc005 = sgs.General(extension,"yyc005", "yyc",3,false) --08005
timing_xushi = function(room,source,target,xushi_card) --使用时机判断函数
	local result =true
	if source:isProhibited(target, xushi_card) then--都要加禁止使用的判断。。。
		result =false
		return result
	end
	if xushi_card:isKindOf("TrickCard") then
		if source:getPhase()  ~= sgs.Player_Play then  
			result =false
			return result
		end         
	end
	if xushi_card:isKindOf("Peach") then
		if not target:isWounded() then   
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("Analeptic") then
		if target:getHp()>0 or source:getPhase()  ~= sgs.Player_Play then   
			result =false
			return result
		end
	end
	return result
end
count_xushi = function(room,source,target,xushi_card) --使用次数判断函数
	local result =true
	if xushi_card:isKindOf("Slash") then
		if not sgs.Slash_IsAvailable(source) then   
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("Analeptic") then
		if not sgs.Analeptic_IsAvailable(source) then   
			result =false
			return result
		end
	end
	return result
end
distance_xushi = function(room,source,target,xushi_card) --使用距离判断函数
	local result=true
	if xushi_card:isKindOf("Slash") then
		if not source:inMyAttackRange(target) then   
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("Snatch") or xushi_card:isKindOf("SupplyShortage")  then
		if not source:hasSkill("qicai") then
			if source:distanceTo(target) >1 then   
				result =false
				return result
			end
		end
	end
	return result
end
target_xushi = function(room,source,target,xushi_card) --使用目标判断函数
	local result=true
	if xushi_card:isKindOf("Analeptic") or xushi_card:isKindOf("ExNihilo") or xushi_card:isKindOf("Lightning") then
		if source:objectName() ~= target:objectName()  then   
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("Collateral") then
		local ecard =target:getEquip(0) --确认武器
		if ecard  then
		else
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("FireAttack") then
		if target:isKongcheng()  then
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("Dismantlement") or xushi_card:isKindOf("Snatch") then
		if target:isNude() and target:getCards("j"):length() == 0 then
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("Dismantlement") or xushi_card:isKindOf("Snatch") or xushi_card:isKindOf("Duel") 
	  or xushi_card:isKindOf("Slash") or xushi_card:isKindOf("Collateral") or xushi_card:isKindOf("ArcheryAttack")
	or xushi_card:isKindOf("SavageAssault") or xushi_card:isKindOf("SupplyShortage") or xushi_card:isKindOf("Indulgence") then
		if source:objectName() == target:objectName() then
			result =false
			return result
		end
	end
	if xushi_card:isKindOf("TrickCard") and not xushi_card:isNDTrick() then
		for _,card in sgs.qlist(target:getCards("j")) do
            if card:objectName() == xushi_card:objectName() then
                result =false
				return result
            end
        end
	end
	return result
end

--[[xushi = sgs.CreateTriggerSkill{
	name = "xushi",
	events = {sgs.PreCardUsed},
	priority=-1, --注意优先度
	can_trigger = function(self, player)
		return true
	end,
	
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		if event == sgs.PreCardUsed then --还是改成targetconfriming?
			local use = data:toCardUse()
			local skillowner = room:findPlayerBySkillName(self:objectName())
			if not skillowner then return false end
			if use.from:objectName() ~= skillowner:objectName() and  use.to:length()==1  then
				if use.card:isKindOf("TrickCard") or use.card:isKindOf("BasicCard") and (not use.card:isKindOf("Nullification"))  then
					if use.card:hasFlag("xushi") then return false end
					if skillowner:getHandcardNum() ==0 then return false end
					ai_data =sgs.QVariant()				
					local ask_card = room:askForCard(skillowner, "^EquipCard","@xushi",ai_data, sgs.Card_MethodDiscard, nil, true, self:objectName())
					if ask_card then 
						local log=sgs.LogMessage()
						--use.to=sgs.SPlayerList()
						--data:setValue(use)
						
						log.type = "$CancelTargetNoUser"
						log.to = use.to
						log.arg = use.card:objectName()
						room:sendLog(log)
						
						
						log.type = "#xushi_effect"
						log.from = player
						
						log.arg = self:objectName()
						room:notifySkillInvoked(skillowner, self:objectName())
						--room:setCardFlag(use.card, "xushi_nullification")
						--if use.card:getSubcards():length()>0 then
						--	room:throwCard(use.card,use.from, use.from)
						--end
						--targets =sgs.PlayerList()
						if timing_xushi(room,use.from,use.to:first(),ask_card) and
							count_xushi(room,use.from,use.to:first(),ask_card) and
							--ask_card:targetFilter(targets, use.to, use.from)
							distance_xushi(room,use.from,use.to:first(),ask_card) and
							target_xushi(room,use.from,use.to:first(),ask_card) and
							not (use.from:isCardLimited(ask_card, sgs.Card_MethodUse)) then 
						
							--use.card=ask_card
							--data:setValue(use)
							room:sendLog(log)
							
							local carduse=sgs.CardUseStruct()
							ask_card:setFlags("xushi")
							--虚史会掩盖一些原有的skillName 奇迹 导致奇迹的mark加不上去？？ carduse时机在后。
							
							carduse.card=ask_card
							carduse.from=use.from
							carduse.to:append(use.to:first())
							room:useCard(carduse,true)
							
							--return true
						else
							log.type = "#xushi_effect1"
							room:sendLog(log)
						end
						
						--return true
					end
				end
			end
		end
	end
}
]]


xushi = sgs.CreateTriggerSkill{
	name = "xushi",
	events = {sgs.PreCardUsed},
	priority=-1, --注意优先度
	can_trigger = function(self, player)
		return player
	end,
	
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		if event == sgs.PreCardUsed then --还是改成targetconfriming?
			local use = data:toCardUse()
			local skillowner = room:findPlayerBySkillName(self:objectName())
			if not skillowner then return false end
			if use.from:objectName() ~= skillowner:objectName() and  use.to:length()==1  then
				if use.card:isKindOf("TrickCard") or use.card:isKindOf("BasicCard") and (not use.card:isKindOf("Nullification"))  then
					if use.card:hasFlag("xushi") then return false end
					if skillowner:getHandcardNum() ==0 then return false end
					ai_data =sgs.QVariant()				
					local ask_card = room:askForCard(skillowner, "^EquipCard","@xushi",ai_data, sgs.Card_MethodDiscard, nil, true, self:objectName())
					if ask_card then 
						local log=sgs.LogMessage()
						log.type = "#xushi_effect"
						log.from = player
						log.arg = self:objectName()
						room:notifySkillInvoked(skillowner, self:objectName())
						room:setCardFlag(use.card, "xushi_nullification")
						if use.card:getSubcards():length()>0 then
							room:throwCard(use.card,use.from, use.from)
						end
						--targets =sgs.PlayerList()
						if timing_xushi(room,use.from,use.to:first(),ask_card) and
							count_xushi(room,use.from,use.to:first(),ask_card) and
							--ask_card:targetFilter(targets, use.to, use.from)
							distance_xushi(room,use.from,use.to:first(),ask_card) and
							target_xushi(room,use.from,use.to:first(),ask_card) and
							not (use.from:isCardLimited(ask_card, sgs.Card_MethodUse)) then 
						
							--use.card=ask_card
							--data:setValue(use)
							room:sendLog(log)
							
							local carduse=sgs.CardUseStruct()
							ask_card:setFlags("xushi")
							--虚史会掩盖一些原有的skillName 奇迹 导致奇迹的mark加不上去？？ carduse时机在后。
							
							carduse.card=ask_card
							carduse.from=use.from
							carduse.to:append(use.to:first())
							room:useCard(carduse,true)
							
							return true
						else
							log.type = "#xushi_effect1"
							room:sendLog(log)
						end
						
						return true
					end
				end
			end
		end
		--同xushi_eff
		--[[if event == sgs.CardUsed then --sgs.CardEffected
			--local effect = data:toCardEffect()
			local use = data:toCardUse()
			if use.card:hasFlag("xushi_nullification") then
				return true
			end
		end
		if event == sgs.BeforeCardsMove then
			local move = data:toMoveOneTime() 
			if move.to_place==sgs.Player_PlaceDelayedTrick  then
				for _,id in sgs.qlist(move.card_ids) do							
					if sgs.Sanguosha:getCard(id):hasFlag("xushi_nullification") then
						move.to_place=sgs.Player_DiscardPile
						move.to=nil
						data:setValue(move)
						return true
					end
				end	
			end
		end]]
	end
}
xushi_effect = sgs.CreateTriggerSkill{
	name = "#xushi",
	events = {sgs.CardUsed,sgs.BeforeCardsMove},
	priority=3,
	can_trigger = function(self, player)
		return player
	end,
	
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		if event == sgs.CardUsed then 
			local use = data:toCardUse()
			if use.card:hasFlag("xushi_nullification") then
				return true
			end
		end
		if event == sgs.BeforeCardsMove then
			local move = data:toMoveOneTime() 
			if move.to_place==sgs.Player_PlaceDelayedTrick  then
				for _,id in sgs.qlist(move.card_ids) do							
					if sgs.Sanguosha:getCard(id):hasFlag("xushi_nullification") then
						move.to_place=sgs.Player_DiscardPile
						move.to=nil
						data:setValue(move)
						return true
					end
				end	
			end
		end
	end
}

--yyc005:addSkill(xushi)
--yyc005:addSkill(xushi_effect)
--extension:insertRelatedSkills("xushi", "#xushi")

 

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


 
--【梦幻的打击乐手——堀川雷鼓】 编号：14002
--hzc002 = sgs.General(extension,"hzc002", "hzc",4,false) 
--[[jiepai=sgs.CreateTriggerSkill{
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
						room:throwCard(to_throw, target, source)
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
]]
--hzc002:addSkill(jiepai)
--hzc002:addSkill(leiting)
--hzc002:addSkill(leiting_mod)
--extension:insertRelatedSkills("leiting", "#leiting_mod")


--【逆袭的天邪鬼——鬼人正邪】 编号：14003 
--hzc003 = sgs.General(extension,"hzc003", "hzc",3,false)	
--[[dianfu_card = sgs.CreateSkillCard{
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
			local reason1 = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_SWAP, player:objectName(),damage.from:objectName(),"feitouPile","")
                
			local move1 = sgs.CardsMoveStruct(player:handCards(), damage.from, sgs.Player_PlaceHand, reason1)
            local reason2 = sgs.CardMoveReason(sgs.CardMoveReason_S_REASON_SWAP, damage.from:objectName(),player:objectName(),"feitouPile","")
                
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
]]
--hzc003:addSkill(dianfu)
--hzc003:addSkill(nixi)
--hzc003:addSkill(nixi_clear)
--hzc003:addSkill(nizhuan)
--extension:insertRelatedSkills("nixi", "#nixi_clear")
--[[nizhuan = sgs.CreateTriggerSkill{
        name = "nizhuan",
        frequency = sgs.Skill_NotFrequent,
        events = {sgs.TargetConfirmed,sgs.SlashEffected},
		can_trigger = function(self, player)
			return player
		end, 
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if event == sgs.TargetConfirmed then
                    local use = data:toCardUse()
                    local room = player:getRoom()
						--local source=room:findPlayerBySkillName("nizhuan")
						if not player:hasSkill("nizhuan") then return false end
                        if use.card:isKindOf("Slash")  and  use.to:length()==1 then
                            local target= use.to:first()
							if not use.from or use.from:isDead() or use.from:getHp()<= target:getHp() then
								return false
							end
							if  player:askForSkillInvoke(self:objectName(),data) then
								room:setCardFlag(use.card, "nizhuan"..target:objectName());
								--需要检测使用杀的合法性?
								if  player:canDiscard(target, "h") 
								and  target:canSlash(use.from,nil,false) then
									local id = room:askForCardChosen(player, target, "h", self:objectName(), false, sgs.Card_MethodDiscard)
									room:throwCard(id, target, player)
									local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
									slash:setSkillName("_" .. self:objectName())
									room:useCard(sgs.CardUseStruct(slash, target, use.from),false)
								end
							end
                        end
                elseif event == sgs.SlashEffected then
                        local effect = data:toSlashEffect()
                        if effect.slash and effect.slash:hasFlag("nizhuan"..effect.to:objectName()) then
								touhou_logmessage("#DanlaoAvoid",effect.to,effect.slash:objectName(),nil,self:objectName())
								room:setEmotion(effect.to, "skill_nullify");
								return true
                        end
				end
        end
}
guizha = sgs.CreateTriggerSkill{
        name = "guizha",
        events = sgs.AskForPeaches,
		frequency = sgs.Skill_Compulsory,
		can_trigger = function(self, player)
			return player
		end, 
        on_trigger = function(self,event,player,data)
                local room = player:getRoom()
                local victim = room:getCurrentDyingPlayer()
				if not victim:hasSkill("guizha") then return false end
				if victim:objectName()==player:objectName() then return false end
                -- if player:objectName() ==victim:objectName() then return false end  
				local handcards = player:getHandcards()
				local peach=false
				for _, p in sgs.qlist(handcards) do
                    if p:isKindOf("Peach") then
                       peach=true
					   break
                    end
				end
				room:showAllCards(player)
				while( peach and victim:getHp()<1) do
					local supply_card=room:askForCard(player,"Peach|.|.|hand!","@chuannan:"..victim:objectName(),data,sgs.Card_MethodUse,victim,false,self:objectName(),false)
					room:useCard(sgs.CardUseStruct(supply_card, player, victim),false)
					peach=false
					for _, p in sgs.qlist(player:getHandcards()) do
						if p:isKindOf("Peach") then
							peach=true
							break
						end
					end
					room:showAllCards(player)
				end
				if victim:getHp()>0 then
					room:setPlayerFlag(victim, "-Global_Dying");
					return true --否则仍旧触发送葬
				end
        end
}
]]
--hzc003:addSkill(nizhuan)
--hzc003:addSkill(guizha)


--【古旧的琴的付丧神——九十九八桥】 编号：14004 
--hzc004 = sgs.General(extension,"hzc004", "hzc",3,false)	
--[[zhenhun=sgs.CreateTriggerSkill{
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
]]
--hzc004:addSkill(zhenhun)
--hzc004:addSkill(zhenhun_clear)
--hzc004:addSkill(touhou_liuli)
--extension:insertRelatedSkills("zhenhun", "#zhenhun_clear")

--【古旧琵琶的付丧神——九十九弁弁】 编号：14005
--hzc005 = sgs.General(extension,"hzc005", "hzc",3,false)	
--[[youyuan=sgs.CreateTriggerSkill{
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
]]
--hzc005:addSkill(youyuan)
--hzc005:addSkill(touhou_liuli)

--【竹林的人狼——今泉影狼】 编号：14006 
--hzc006 = sgs.General(extension,"hzc006", "hzc",4,false)	
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

--[[langying=sgs.CreateTriggerSkill{
name="langying",
events=sgs.CardAsked,
on_trigger=function(self,event,player,data)
		if data:toStringList()[1]=="jink" then
            if player:getEquips():length()== 0 then return false end    
			local room=player:getRoom()
                
             if player:askForSkillInvoke(self:objectName(),data) then
                room:broadcastSkillInvoke(self:objectName())
				
				local equips =sgs.IntList()	
				for _,equip in sgs.qlist(player:getEquips()) do
					equips:append(equip:getId())
				end
	
				local move =sgs.CardsMoveStruct()
				move.card_ids=equips
				move.from_place =sgs.Player_PlaceEquip
				--move.reason.m_reason = self:objectName()
				move.to_place =sgs.Player_PlaceHand
				move.from =player
				move.to =player
		
				room:moveCardsAtomic(move, true)
                local jink = sgs.Sanguosha:cloneCard("jink", sgs.Card_NoSuit, 0)
                jink:setSkillName("_langying")
                room:provide(jink)
                                
				return true
            end
        end
end
}
]]
--[[yuanfei_card = sgs.CreateSkillCard{
	name = "yuanfei",
	target_fixed = false,
	will_throw = true,
	filter = function(self, targets, to_select, player)
		return #targets==0  and not sgs.Self:inMyAttackRange(to_select)
			and to_select:objectName() ~= sgs.Self:objectName()
	end,	
	on_use = function(self, room, source, targets)
		local target=targets[1]
		--room:setPlayerCardLimitation(target, "use,response", pattern, false)
		--".|%1|.|hand$0" color
		--room:setPlayerCardLimitation(player, "use", "Slash", true)
		--room->removePlayerCardLimitation(player, "use,response,discard", jilei_type + "|.|.|hand$1");
        --room:setPlayerCardLimitation(target, "use,response", ".", true)
		room:setPlayerCardLimitation(target, "use,response", "BasicCard|.|.|.", true)
		room:setPlayerCardLimitation(target, "use,response", "EquipCard|.|.|.", true)
		room:setPlayerCardLimitation(target, "use,response", "TrickCard|.|.|.", true)
		--room:setPlayerCardLimitation(target, "use,response", ".|.|.|hand$1", true)
		room:setPlayerFlag(target,"yuanfei")
        touhou_logmessage("#yuanfei",target,self:objectName())
		--room:removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0")            
	end,
}
yuanfei= sgs.CreateViewAsSkill{
	name = "yuanfei",
	n = 0,	
	enabled_at_play = function(self,player)
		return not player:hasUsed("#yuanfei")
	end,	
	view_as = function(self, cards)
			local card=yuanfei_card:clone()
			card:setSkillName(self:objectName())
			return card
	end
	
}
yuanfei_clear= sgs.CreateTriggerSkill{
	name = "#yuanfei_clear",
	events = {sgs.EventPhaseChanging},
	can_trigger=function(self,player)--他人回合 以及凭依后也要能执行
		return true
	end,	
	on_trigger=function(self,event,player,data)
		local change = data:toPhaseChange()
		if change.to == sgs.Player_NotActive then
			local room=sgs.Sanguosha:currentRoom()
			for _,p in sgs.qlist(room:getAlivePlayers()) do
				if p:hasFlag("yuanfei") then
					p:setFlags("-yuanfei")
					room:removePlayerCardLimitation(p, "use,response", "BasicCard|.|.|.$1")
					room:removePlayerCardLimitation(p, "use,response", "EquipCard|.|.|.$1")
					room:removePlayerCardLimitation(p, "use,response", "TrickCard|.|.|.$1")
					--room:removePlayerCardLimitation(p, "use,response", ".|.|.|hand$1")
					--room:removePlayerCardLimitation(p, "use,response", ".$1")
				end
			end
		end
	end,
}
]]
--hzc006:addSkill(langying)
--hzc006:addSkill(yuanfei)
--hzc006:addSkill(yuanfei_clear)
--extension:insertRelatedSkills("yuanfei", "#yuanfei_clear")
--hzc006:addSkill(renlang)
--hzc006:addSkill(renlang_mod)
--hzc006:addSkill(touhou_bushi)
--hzc006:addSkill(manyue)
--extension:insertRelatedSkills("renlang", "#renlang_mod")


 --【辘轳首的怪奇——赤蛮奇】 编号：14007
--hzc007 = sgs.General(extension,"hzc007", "hzc",4,false)
--[源码修改]Room::askForUseCard
feitouFakeMoveCard = sgs.CreateSkillCard{
        name = "feitouCard",
        target_fixed = true,
        will_throw = false,
        on_validate = function(self, use)
                local room = use.from:getRoom()
                local card = room:askForUseCard(use.from, "@@feitou", "@feitou-twoCards",-1, sgs.Card_MethodUse, true)
        end
}

feitouVS = sgs.CreateViewAsSkill{
        name = "feitou",
        n = 1,
        view_filter = function(self, selected, to_select)
                local pattern = sgs.Sanguosha:getCurrentCardUsePattern()
                if pattern == "@@feitou" or  sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_PLAY then
                        local s_ids = sgs.Self:getPile("feitou")
						return s_ids:contains(to_select:getEffectiveId())
                end
                return false
        end,
        view_as = function(self, cards)
                local pattern = sgs.Sanguosha:getCurrentCardUsePattern()
                if pattern == "@@feitou" then
                        if #cards ~=1 then return nil end
						
						local slash = sgs.Sanguosha:cloneCard("slash",cards[1]:getSuit(),cards[1]:getNumber())
                        slash:setSkillName("feitou")
                        slash:addSubcard(cards[1])
                        return slash
                else
                        if sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_PLAY  then
							return feitouFakeMoveCard:clone()
						else
							if #cards ~=1 then return nil end
							if string.find(pattern, "slash") then
								local card = sgs.Sanguosha:cloneCard("slash",cards[1]:getSuit(),cards[1]:getNumber())
								card:addSubcard(cards[1])
								card:setSkillName(self:objectName())
								return card
							end
						end
                end
        end,
        enabled_at_play = function(self, player)
				return player:getPile("feitou"):length() > 0
        end,
        enabled_at_response = function(self, player, pattern)
				if pattern == "@@feitou" then return true end
                if player:getPile("feitou"):length() < 1 then return false end
                return string.find(pattern, "slash")  and (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE)
        end
}

feitou=sgs.CreateTriggerSkill{
name="feitou",
events={sgs.EventPhaseStart,sgs.PreCardUsed},
view_as_skill = feitouVS,
on_trigger = function(self, event, player, data)
        local room=player:getRoom()
        if event==sgs.EventPhaseStart  and player:getPhase()== sgs.Player_Finish  then
			if player:isKongcheng() then return false end
			local cards=room:askForExchange(player, self:objectName(), 1, true, "addfeitou")
            if cards then
				id = cards:getSubcards():first()
				player:addToPile("feitou",id)
			end
		end
        if event==sgs.PreCardUsed then
			local card=data:toCardUse().card
			if card:isKindOf("Slash") and card:getSkillName()~=self:objectName() then
                room:setPlayerFlag(player,self:objectName())
			end
			if not player:hasFlag(self:objectName()) and card:getSkillName()==self:objectName() then                 
				room:addPlayerHistory(player, data:toCardUse().card:getClassName(), -1)	
			end
		end
end
}

feitoumod = sgs.CreateTargetModSkill{
        name = "#feitoumod" ,
        pattern = "Slash" ,
        distance_limit_func = function(self, player, card)
                if card:getSkillName()=="feitou"  then return 1000 end
                return 0
        end,
		residue_func = function(self, player,card)
                if player:hasSkill("feitou")  and (card:getSkillName()=="feitou")then
					return 1000
                end
        end,
}
--近似解决 response use时 已成为飞头的“属性杀” 回到 手牌中 直接点击使用 还会是原属性杀
--用filter当作闪保证不能被直接用出去
feitouFilter = sgs.CreateFilterSkill{
        name = "#feitoufilter",
        view_filter = function(self, to_select)
                local room = sgs.Sanguosha:currentRoom()
				local source=room:findPlayerBySkillName("feitou")
				local s_ids = source:getPile("feitou")
				return s_ids:contains(to_select:getEffectiveId()) and to_select:isKindOf("Slash")
                --local place = room:getCardPlace(to_select:getEffectiveId())
                --if place == sgs.Player_PlaceHand then
                --        return to_select:isNDTrick() 
                --end
                --return false
        end,

        view_as = function (self,card)
                local id = card:getId()
                local suit = card:getSuit()
                local point = card:getNumber()
                local slash = sgs.Sanguosha:cloneCard("jink",suit,point)
                --slash:setSkillName("feitou")
                local vsc = sgs.Sanguosha:getWrappedCard(id)
                vsc:takeOver(slash)
                return vsc
        end
}
--[[feitou=sgs.CreateTriggerSkill{
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
]]
--hzc007:addSkill(feitou)
--hzc007:addSkill(feitoumod)
--hzc007:addSkill(feitouFilter)

--extension:insertRelatedSkills("feitou", "#feitoumod")
--extension:insertRelatedSkills("feitou", "#feitoufilter")


--【栖息于淡水的人鱼——若鹭姬】编号：14008 
--hzc008 = sgs.General(extension,"hzc008", "hzc",3,false)	
--[[jingtaovs= sgs.CreateZeroCardViewAsSkill {
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
]]
--[[shizhu= sgs.CreateTriggerSkill{
	name = "shizhu",
	events = {sgs.EventPhaseStart,sgs.EventPhaseChanging},
	can_trigger=function(self,player)--他人回合 以及凭依后也要能执行
		return player
	end,	
	on_trigger=function(self,event,player,data)
		
		local room=player:getRoom()
		if event ==sgs.EventPhaseStart then
				if player:getPhase()==sgs.Player_Finish then
				
				local source=room:findPlayerBySkillName(self:objectName())
			
				local ids=room:getTag("shizhuPeach"):toIntList()
				
				if not source  then return false end  --or not source:isWounded()
				if not ids or ids:length()==0 then return false end
				local current =room:getCurrent() 
				if current:objectName() ==source:objectName() then return false end
				--if not source:inMyAttackRange(current) then return false end
			
				if room:askForSkillInvoke(source,self:objectName(),data) then
					room:fillAG(ids,source)
					local id=room:askForAG(source,ids,false,self:objectName())
					room:clearAG(source)
					if (id>-1) then
						local peach=sgs.Sanguosha:getCard(id)		
						room:showAllCards(source)
						room:getThread():delay(1000)
						room:clearAG()
						local no_peach=true
						for _,c in sgs.qlist(source:getCards("h")) do
							if c:isKindOf("Peach") then
								no_peach=false
								break
							end
						end
						if no_peach then
							source:obtainCard(peach,true)
						end
					end
				end
			end
		end
		if event == sgs.EventPhaseChanging then
			local change = data:toPhaseChange()
			if change.to == sgs.Player_NotActive then
				room:removeTag("shizhuPeach")
			end
		end
	end,
}
shizhuCount= sgs.CreateTriggerSkill{
	name = "#shizhu" ,
    events = {sgs.CardsMoveOneTime},
    can_trigger=function(self,palyer)
		return true
	end,
	on_trigger=function(self,event,player,data)		
		if event == sgs.CardsMoveOneTime  then
				local room=sgs.Sanguosha:currentRoom()
				local move = data:toMoveOneTime()				
				if  move.to_place == sgs.Player_DiscardPile then						 
						local ids =sgs.IntList()
						local ids1=room:getTag("shizhuPeach"):toIntList()
						for _,id in sgs.qlist(move.card_ids) do							
							local card = sgs.Sanguosha:getCard(id)
							if (card:isKindOf("Peach")) and not ids1:contains(id)  
							and room:getCardPlace(id)==sgs.Player_DiscardPile then
							--有同时机的【搜集】存在，需要检测位置								
								ids1:append(id)
							end
						end
						local _data=sgs.QVariant()
						_data:setValue(ids1)
						room:setTag("shizhuPeach",_data)
				end 
		end
	end
}
suhui = sgs.CreateDistanceSkill{
        name = "suhui",
        correct_func = function (self,from,to)
                if from:hasSkill("suhui") then
					for _,equip in sgs.qlist(to:getEquips()) do
						if equip:isBlack() then
							return -2
						end
					end
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
					new_value= damage.damage-1
					if new_value<1 then
						new_value=0
					end
					damage.damage=new_value
					data:setValue(damage) --重新对伤害赋值
					touhou_logmessage("#shuixing",player,"shuixing",nil,1)
					room:notifySkillInvoked(player, self:objectName())
					if damage.damage==0 then
						return true
					end
                end
        end
}


aige = sgs.CreateTriggerSkill{
        name = "aige" ,
        events = {sgs.CardsMoveOneTime} ,
		 can_trigger = function(self, target)
            return target 
        end,
        on_trigger = function(self, event, player, data)
            local room=player:getRoom()
			local move = data:toMoveOneTime()
            local source=room:findPlayerBySkillName(self:objectName())  
			if (not source or not move.from or move.from:objectName() == source:objectName()  or player:objectName() ~= source:objectName() ) then
				return false
			end
			local target
			if move.from_places:contains(sgs.Player_PlaceDelayedTrick) then
				for _,p in sgs.qlist(room:getOtherPlayers(source)) do
					if (move.from:objectName() == p:objectName()) then
						target=p
						break
					end
				end
			end
			
			if (target and source:canDiscard(target, "h") )then
				local _data=sgs.QVariant()
				_data:setValue(target)
				if source:askForSkillInvoke(self:objectName(),_data) then
					local id = room:askForCardChosen(source, target, "h", self:objectName())
					room:throwCard(id, target, player)
				end
			end

            return false
        end ,
}
]]

--hzc008:addSkill(shizhu)
--hzc008:addSkill(shizhuCount)
--extension:insertRelatedSkills("shizhu", "#shizhu")
--hzc008:addSkill(aige)
--hzc008:addSkill(suhui)
--hzc008:addSkill(shuixing)
--hzc008:addSkill(jingtao)
--hzc008:addSkill(shuixing)
--hzc008:addSkill(shizhu)
--hzc008:addSkill(shizhu_clear)
--extension:insertRelatedSkills("shizhu", "#shizhu_clear")





 
 