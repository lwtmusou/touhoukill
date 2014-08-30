module("extensions.touhougod",package.seeall)
extension = sgs.Package("touhougod")

--直接借用了东方包的一些辅助函数
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
--[源码改动]询问转换势力 直接写入了源码gamerule里
--同时改动 Room::askForKingdom 加入选势力时的国籍信息 
--client.Cpp  client.h void Client::askForKingdom(const Json::Value &arg) 会禁止神势力进入askforkingdomchosen列表
--ai.cpp  ai选势力也不允许原三国和东方交叉??  QString TrustAI::askForKingdom()

--[[touhou_askforkingdom = sgs.CreateTriggerSkill{
	name = "#touhou_askforkingdom",
	events ={sgs.GameStart},
	priority = 5,
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if room:getMode() == "mini" then return false end
		if room:getMode() == "custom_scenario" then return false end
		--if player:getState() == "robot" then
		--	local str = room:askForChoice(player,self:objectName(),"touhou_cancel+wei+shu+wu+qun+touhou_kingdom_baka+touhou_kingdom_men")
		--	if str == "touhou_cancel" then
		--		return
		--	else
		--		room:setPlayerProperty(player, "kingdom", sgs.QVariant(str))
				--touhou_logmessage("#touhou_playerkingdom",player,str)
		--	end
		--else
			local kname = room:askForKingdom(player)
			room:setPlayerProperty(player, "kingdom", sgs.QVariant(kname))
			touhou_logmessage("#ChooseKingdom", player, kname)
		--end
	end
}]]
--[[touhou_extraTurnClear = sgs.CreateTriggerSkill{ --清除额外回合
        name = "#touhou_extraTurnClear",
        events = {sgs.EventPhaseChanging},

		on_trigger = function(self, event, player, data)
                local room = player:getRoom()
				if event == sgs.EventPhaseChanging then
                    local change = data:toPhaseChange()
                    if change.to == sgs.Player_NotActive then
						if player:getMark("touhou-extra")>0 then
							room:setPlayerMark(player,"touhou-extra",0)
						end
					end
                end
        end
}]]
function touhou_getAllGenerals()
	touhouGeneral={}
	--zhu_num=8
	--touhougod_num=17--依次类推
	nums={8,9,13,10,10,11,9,8,10,8,12,17,6,5}
	--sp和年代记包含在内 黑历史不包含
	package_names={"zhu","hmx","yym","yyc","zhan","fsl","dld","xlc","slm","hzc","wai","shen","sp","ndj"}
	for i=1, #nums , 1 do
		package_name=package_names[i]
		if package_name=="ndj" then
			table.insert(touhouGeneral, "ndj001")
			table.insert(touhouGeneral, "ndj002")
			table.insert(touhouGeneral, "ndj004")
			table.insert(touhouGeneral, "ndj010")
			table.insert(touhouGeneral, "ndj011")
		else
			local x=1
			if package_name=="shen" then
				x=0
			end
			for var=x, nums[i], 1 do
				name=package_name
				if var<10 then
					name =name.."00"..tostring(var)
				else
					name =name.."0"..tostring(var)
				end
				table.insert(touhouGeneral, name)
			end
		end
	end
	
	return touhouGeneral
end
--[[function touhou_kingdom()
	touhouGeneral=touhou_getAllGenerals()
	for _, touhougeneralname in ipairs(touhouGeneral) do
		local atouhougeneral = sgs.Sanguosha:getGeneral(touhougeneralname)
		if atouhougeneral and (atouhougeneral:getKingdom()=="zhu" or atouhougeneral:getKingdom()=="touhougod") then
			atouhougeneral:addSkill("#touhou_askforkingdom")
		end
	end
end]]
function touhou_tianyi()--天仪装备技能
	touhouGeneral=touhou_getAllGenerals()
	for _, touhougeneralname in ipairs(touhouGeneral) do
		if touhougeneralname ~="zhu008" then
			local atouhougeneral = sgs.Sanguosha:getGeneral(touhougeneralname)
			if atouhougeneral then
				atouhougeneral:addSkill("#tianyi_targetmod")
				atouhougeneral:addSkill("#tianyi_horse")
				atouhougeneral:addSkill("#tianyi_attackrange")
				atouhougeneral:addSkill("#tianyi_collateral")
			end
		end
	end
end

--[[function touhou_extraTurn()--清除额外回合
	touhouGeneral=touhou_getAllGenerals()
	for _, touhougeneralname in ipairs(touhouGeneral) do
		local atouhougeneral = sgs.Sanguosha:getGeneral(touhougeneralname)
		if atouhougeneral then
			atouhougeneral:addSkill("#touhou_extraTurnClear")
		end
	end
end]]
--[[if not sgs.Sanguosha:getSkill("#touhou_askforkingdom") then
    local skillList=sgs.SkillList()
    skillList:append(touhou_askforkingdom)
    sgs.Sanguosha:addSkills(skillList)
end]]
--[[if not sgs.Sanguosha:getSkill("#touhou_extraTurnClear") then
    local skillList=sgs.SkillList()
    skillList:append(touhou_extraTurnClear)
    sgs.Sanguosha:addSkills(skillList)
end]]
-----------------------------------------------------------------------------------------------------------
--技能代码   神卡
------------------------------------------------------------------------------------------------------------
--【创幻神主——ZUN】 编号：00000  --by三国有单
 shen000 = sgs.General(extension, "shen000", "touhougod", 0, true) 
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
	local chuanghuan_attach_PlayViewAs={"mofa","jiezou","suoding","hmlzhanyi","zhenye","banyue",
	"shrxmocao","miyao","kuangzao","qizha","gesheng","yingguang","yazhi","bian","leiyun",
	"fengshen","xinshang","zaihuo","tianyan","fengrang","jiliao","maihuo","yccxbaigui","yccxjiuchong",
	"jidu","LuaShouhuiVS","pudu","weizhi","qingting","leishi","xiefa","duzhua","buming","qiuwen","sjlzzxiufu",
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
				table.insert(skill_names,"ymsndshenpan")
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
				table.insert(skill_names,"xxsyyzchuixue")	
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
						table.insert(skill_names,"aojiaofsuzhize")
						table.insert(skill_names,"smwwtoupai")
						
						table.insert(skill_names,"yccxcuiji")
					end
				
			end
			if change.to == sgs.Player_Play then
				
					if current:objectName()==source:objectName() then
					else
						table.insert(skill_names,"ymsndquanjie")
					end
				
			end
			if change.to == sgs.Player_Discard then
				
					if current:objectName()==source:objectName() then
						table.insert(skill_names,"yongheng")
					end
					table.insert(skill_names,"aojiaofsujingdong")
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
			table.insert(skill_names,"aojiaofsujingjie")
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
		table.insert(skill_names,"skltmingyun")
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
					table.insert(skill_names,"ldlkyaoban")
				end				
				if  damage.to:objectName() ==source:objectName() then
					table.insert(skill_names,"yuxue")
					table.insert(skill_names,"huisu")
					table.insert(skill_names,"aojiaofsujingjie")
					table.insert(skill_names,"shishen")
					table.insert(skill_names,"baochun")
					table.insert(skill_names,"wangyue")
					table.insert(skill_names,"jubao")
					table.insert(skill_names,"yuanling")
					table.insert(skill_names,"chuannan")
					table.insert(skill_names,"ddlxsjingxia")
					table.insert(skill_names,"zhengti")
					table.insert(skill_names,"ddlxsqingyu" )
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
			table.insert(skill_names,"aojiaofsusisheng")
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
				table.insert(skill_names,"hmlzhanyi")
				table.insert(skill_names,"yuzhu")
				table.insert(skill_names,"tymh_huweivs")
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
						table.insert(skill_names,"tymh_huweivs")
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
					table.insert(skill_names, "ddlxsyiwang") 
				end
				if move.from_places:contains(sgs.Player_PlaceDelayedTrick)  then
					table.insert(skill_names, "ddlxsguoke") 
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
						table.insert(skill_names,"aojiaofsuchunxi")
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
						table.insert(skill_names, "sbzhy_xushi")
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
					table.insert(skill_names,"tymh_huweivs" )
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
					table.insert(skill_names,"shrxzhancao")
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
					table.insert(skill_names,"ymsndhuiwu")
				end
				if use.card and use.card:isKindOf("Slash") then
					table.insert(skill_names,"wangwu")
					table.insert(skill_names,"easthuanshi")
					table.insert(skill_names,"gelong")
					table.insert(skill_names,"xxsyyzwushou")
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
				
			if use.card and use.card:getSkillName() =="hmlzhanyi" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-hmlzhanyi")
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
			if use.card and use.card:getSkillName() =="yccxbaigui" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-yccxbaigui")
			end
			if use.card and use.card:getSkillName() =="yccxjiuchong" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-yccxjiuchong")
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
			if card_star and card_star:getSkillName() =="hmlzhanyi" then
				room:sendLog(log)
				room:handleAcquireDetachSkills(source, "-hmlzhanyi")
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

                                
 shen000:addSkill(chuanghuan)
 shen000:addSkill(chuanghuan_attach)--触发获得技能 种类很杂
 shen000:addSkill(chuanghuan_attach_self)--触发获得技能 触发者只为自己时
 shen000:addSkill(chuanghuan_detach)--回合结束后清除一些获得的技能
 shen000:addSkill(shenzhu)
  extension:insertRelatedSkills("chuanghuan", "#chuanghuan")
  extension:insertRelatedSkills("chuanghuan", "#chuanghuan_self")
  extension:insertRelatedSkills("chuanghuan", "#chuanghuan_detach") 


--【幻想之界——八云紫】 编号：00001 
shen001 = sgs.General(extension, "shen001", "touhougod", 4, false) 
--[源码改动]sturct.h  sanguosha.i 新增recover.reason 用于判定仇恨
jiexian = sgs.CreateTriggerSkill{
        name = "jiexian",
        events = {sgs.DamageInflicted, sgs.PreHpRecover},
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                local source = room:findPlayerBySkillName(self:objectName())
                if source  and not source:isNude() then
                        if event == sgs.DamageInflicted then
                                if room:askForCard(source, "..H", "@jiexiandamage:"..player:objectName(),data,self:objectName()) then
                                        local damage=data:toDamage()
										touhou_logmessage("#jiexiandamage",player,self:objectName(),nil,damage.damage)
										if player:isWounded() then
                                                local recover = sgs.RecoverStruct()
                                                recover.who = source
												recover.reason = self:objectName()
                                                room:recover(player, recover)
                                        end
										
                                        return true
                                end
                        elseif event == sgs.PreHpRecover then
                                --ai
								local _data = sgs.QVariant()
								_data:setValue(player)
								if room:askForCard(source, "..S", "@jiexianrecover:"..player:objectName(),_data,self:objectName()) then
                                        touhou_logmessage("#jiexianrecover",player,self:objectName(),nil,data:toRecover().recover)
										local damage = sgs.DamageStruct()
                                        damage.to = player
                                        damage.from = nil
                                        damage.reason=self:objectName()
										room:damage(damage)
										
                                        return true
                                end
                        end
                end
                return false
        end,
        can_trigger = function(self, player)
                return player
        end
}

shen001:addSkill(jiexian)


--【永远的红之幼月——蕾米莉亚•斯卡雷特】 编号：00002  by三国有单
shen002 = sgs.General(extension, "shen002", "touhougod", 3, false) 
zhouye = sgs.CreateTriggerSkill{
		name = "zhouye", 
		frequency = sgs.Skill_Compulsory,
		events = { sgs.GameStart,   sgs.EventPhaseStart}, 
		on_trigger = function(self, event, player, data)
			local room = player:getRoom()
			
			if event == sgs.GameStart and player:hasSkill(self:objectName()) then 
				room:setPlayerCardLimitation(player, "use", "Slash",false)
			end
			
			if event==sgs.EventPhaseStart and (player:getPhase() == sgs.Player_Start)  then
				touhou_logmessage("#TriggerSkill",player,self:objectName())
				room:notifySkillInvoked(player, self:objectName())
				if player:getMark("@ye") >0 then
					player:loseAllMarks("@ye")
				end
				local idlist=room:getNCards(1)
				local cd_id =idlist:first() 
				room:fillAG(idlist,nil)				
				room:askForAG(player,idlist,true,self:objectName())				
				--room:getThread():delay()
				local card =sgs.Sanguosha:getCard(cd_id)				
				room:clearAG()
				
				if card:isBlack() then 
					player:gainMark("@ye", 1)
				end	
				room:throwCard(cd_id,player)
				--应该用类似萃集的“置入”？？				
			end
			
		end
	}
zhouye_change = sgs.CreateTriggerSkill{
        name = "#zhouye_change" ,
        events = {sgs.PreMarkChange} ,
		can_trigger = function(self, player)
            return player 
        end,
        on_trigger = function(self, event, player, data)

                local change = data:toMarkChange()
                if change.name ~= "@ye" then return false end
                local mark = player:getMark("@ye")
                local room = player:getRoom()
                
				if (mark == 0) and (mark + change.num > 0) then
                    room:removePlayerCardLimitation(player, "use", "Slash$0")
						
               elseif (mark > 0) and (mark + change.num == 0) then
                    room:setPlayerCardLimitation(player, "use", "Slash", false)   
               end
               return false
        end ,
}	
hongwu_card = sgs.CreateSkillCard{
	name = "hongwu",
	target_fixed = true,
	will_throw = true,
	
	filter = function(self, targets, to_select, player)
		return #targets == 0 and to_select:hasSkill(self:objectName())
	end,
	
	on_use = function(self, room, source, targets)
		source:gainMark("@ye", 1)
	end,
	
}
hongwu = sgs.CreateViewAsSkill{
	name = "hongwu",
	n = 2,
	
	enabled_at_play = function()
		return  sgs.Self:getMark("@ye") ==0       
	end,
	
	view_filter = function(self, selected, to_select)
		return  to_select:isRed() and not sgs.Self:isJilei(to_select)
	end,
	
	view_as = function(self, cards)
		if(#cards ~= 2) then return nil end
		local qcard = hongwu_card:clone()
		qcard:addSubcard(cards[1]) 
		qcard:addSubcard(cards[2])
		qcard:setSkillName(self:objectName())
		return qcard
	end
}
	
shenqiang_card = sgs.CreateSkillCard{
	name = "shenqiang",
	target_fixed = false,
	will_throw = true,
	
	filter = function(self, targets, to_select, player)
		return #targets == 0 and  to_select:objectName()~=sgs.Self:objectName()
	end,
	
	on_use = function(self, room, source, targets)
		
		local target = targets[1]
		local damage=sgs.DamageStruct()
		damage.card = nil
		damage.damage = 1
		damage.from = source
		damage.to = target

		room:damage(damage)
	end,
	
}
shenqiang = sgs.CreateViewAsSkill{
	name = "shenqiang",
	n = 1,
	
	enabled_at_play = function()
		return  sgs.Self:getMark("@ye") >0       
	end,
	
	view_filter = function(self, selected, to_select)
		return to_select:isKindOf("Weapon") or to_select:getSuit()==  sgs.Card_Heart
			and not sgs.Self:isJilei(to_select)
	end,
	
	view_as = function(self, cards)
		if(#cards ~= 1) then return nil end
		local qcard = shenqiang_card:clone()
		qcard:addSubcard(cards[1]) 
		qcard:setSkillName(self:objectName())
		return qcard
	end
}
	
yewang = sgs.CreateTriggerSkill{
		name = "yewang", 
		frequency = sgs.Skill_Compulsory,
		events = {sgs.DamageInflicted},  --sgs.DamageForseen 
		on_trigger = function(self, event, player, data)
			local room = player:getRoom()
			local damage = data:toDamage()

			if player:getMark("@ye") >0  then  
				damage.damage =damage.damage -1
				touhou_logmessage("#YewangTrigger",player,self:objectName(),nil,1)
				room:notifySkillInvoked(player, self:objectName())
				data:setValue(damage)
				if damage.damage==0 then
					return true
				end
			end
		end
	}
	
	shen002:addSkill(zhouye)
	shen002:addSkill(zhouye_change)
	shen002:addSkill(hongwu)
	shen002:addSkill(shenqiang)
	shen002:addSkill(yewang)
	--extension:insertRelatedSkills("zhouye", "#zhouye_change")
	
	
--【最强——琪露诺】 编号：00003 --by三国有单
shen003 = sgs.General(extension, "shen003", "touhougod", 4, false)
--方案1  纯lua写  为给每个人添加一个收到冰杀伤害后的铁锁传导技能

--方案2  比起方案1 修改源码中gamerule中铁锁的结算 
--但并没有定义 冰属性  实质仍旧是普通属性。。。。只是遇到奥义牌的伤害会触发铁锁
--对付藤甲仍旧是枚举。  对于其他技能“非属性伤害”这种描述依旧需要枚举

--方案3 彻底改源码方案
--[源码改动]定义新的属性 冰杀  ice slash  定义在joy包（确保一般不会进入游戏） 其实可以定义在一个新包的  不过我懒了。。。
-- struct.h damage struct追加ice
--Room::sendDamageLog()

--显然方案3才是完美的 目前代码实际采用方案3
--方案1 2被注释掉了
aoyi_handle=sgs.CreateTriggerSkill{
	name="#aoyi",
	frequency = sgs.Skill_Compulsory,
	events={sgs.GameStart,sgs.CardUsed,sgs.TargetConfirmed,sgs.EventAcquireSkill,sgs.EventLoseSkill},
	
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		if ((event == sgs.GameStart ) or (event == sgs.EventAcquireSkill and data:toString() == "aoyi")) then
			room:setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false)
			--for _,p in sgs.qlist(room:getAlivePlayers()) do
			--	room:acquireSkill(p,"#ice_damage")
			--end
		end
		if(event == sgs.EventLoseSkill and data:toString() == "aoyi")  then
			room:removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0")
		end
		if event==sgs.CardUsed or event==sgs.TargetConfirmed then
				local use = data:toCardUse()
				if not use.card:isKindOf("IceSlash") then return false end
			    --定义了冰杀之后，就不用靠技能名识别了
				--if not use.card:isKindOf("Slash") then return false end
			
				--if use.card:getSkillName() ~="aoyi" then return false end
				
				--[[if event==sgs.TargetConfirmed then 
					for _, p in sgs.qlist(use.to) do
                        t=false
						local ecard =p:getEquip(1) --确认藤甲
						if ecard and ecard:isKindOf("Vine") then t=true end
						if t then               
							p:addQinggangTag(use.card)
						end
                    end
					--return true
				end]]
				if event~=sgs.CardUsed then return false end
				--之后的处理为cardused的情况
				local choices = {}
				table.insert(choices, "aoyi1")
				all=sgs.SPlayerList()
				for _,p in sgs.qlist(room:getAlivePlayers()) do	
					if  player:canDiscard(p, "ej") then 
						all:append(p) 
					end
				end
				if all:length()>0 then
					table.insert(choices, "aoyi2")
				end
				local choice = room:askForChoice(player, "aoyi", table.concat(choices, "+"))
				if choice=="aoyi1" then
					touhou_logmessage("#InvokeSkill",player,"aoyi")
					room:notifySkillInvoked(player, self:objectName())
					player:drawCards(player:getLostHp())
				else
					
					local s=room:askForPlayerChosen(player,all,self:objectName(),"aoyi_chosenplayer",true,true)
					local to_throw = room:askForCardChosen(player, s, "ej", self:objectName(),false,sgs.Card_MethodDiscard)
					room:throwCard(to_throw,s,player)
				end
		end
		
	end
}

--[[ice_damage=sgs.CreateTriggerSkill{
	name="#ice_damage",
	frequency = sgs.Skill_Compulsory,
	events={sgs.DamageComplete},

	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		
		local damage = data:toDamage() 
		if  damage.from and damage.from:hasSkill("aoyi")  then
			if damage.card and damage.card:isKindOf("Slash")  then
				
				
				if damage.card:getSkillName() ~="aoyi" then return false end
				local target
				if player:isChained() then
					player:setChained(false)
					sgs.Sanguosha:playSystemAudioEffect("chained")
					 room:broadcastProperty(player, "chained")
					room:setEmotion(player, "chain")
					listt= room:getOtherPlayers(player)
					room:sortByActionOrder(listt)
					for _,p in sgs.qlist(listt) do
						if p:isChained() then
							target=p
							break
						end
					end
					if target then
								local damage1 = sgs.DamageStruct()
								damage1.from = damage.from
								damage1.to = target
								damage1.card =damage.card
								damage1.damage =damage.damage
								room:damage(damage1)
					end
				end
				
			end
		end
	end
}
]]
aoyi= sgs.CreateFilterSkill{
        name = "aoyi",
        view_filter = function(self, to_select)
                local room = sgs.Sanguosha:currentRoom()
                local place = room:getCardPlace(to_select:getEffectiveId())
                if place == sgs.Player_PlaceHand then
                        return to_select:isNDTrick() 
                end
                return false
        end,

        view_as = function (self,card)
                local id = card:getId()
                local suit = card:getSuit()
                local point = card:getNumber()
                local slash = sgs.Sanguosha:cloneCard("ice_slash",suit,point)
                slash:setSkillName("aoyi")
                local vsc = sgs.Sanguosha:getWrappedCard(id)
                vsc:takeOver(slash)
                return vsc
        end
}

aoyi_mod = sgs.CreateTargetModSkill{
        name = "#aoyi_mod",
        pattern="Slash",
        residue_func = function(self,player,card)     
				--if player:hasSkill("aoyi") and card:isKindOf("Slash") and  (card:getSkillName()=="aoyi") then
				
				if player:hasSkill("aoyi") and card:isKindOf("IceSlash") then
					return 1000		
                else
                    return 0
                end
        end,
}
aoyi_clearlimit=sgs.CreateTriggerSkill{
name="#aoyi_clearlimit",
events=sgs.PreCardUsed,
on_trigger=function(self,event,player,data)
        local card =data:toCardUse().card 
        local room=player:getRoom()
		--if  card:isKindOf("Slash") and (not (card:getSkillName()=="aoyi"))then                 
		if card:isKindOf("Slash") and not card:isKindOf("IceSlash") then                 
			player:setFlags("aoyi_slash")
		end
		--if (not player:hasFlag("aoyi_slash")) and (card:getSkillName()=="aoyi") then                 
        if (not player:hasFlag("aoyi_slash")) and  card:isKindOf("IceSlash") then                 
			room:addPlayerHistory(player, data:toCardUse().card:getClassName(), -1)	
		end
end
}
--[[if not sgs.Sanguosha:getSkill("#ice_damage") then
    local skillList=sgs.SkillList()
    skillList:append(ice_damage)
    sgs.Sanguosha:addSkills(skillList)
end]]
shen003:addSkill(aoyi)
shen003:addSkill(aoyi_handle)
shen003:addSkill(aoyi_mod)
shen003:addSkill(aoyi_clearlimit)
extension:insertRelatedSkills("aoyi", "#aoyi_handle")
extension:insertRelatedSkills("aoyi", "#aoyi_mod")
extension:insertRelatedSkills("aoyi", "#aoyi_clearlimit")


--【地底的太阳——灵乌路空】 编号：00004  by三国有单
shen004 = sgs.General(extension, "shen004", "touhougod", 4, false)   
--[源码改动] 杀的targetFeasbie()   onUse() UI相关
--当前情况下，【失控】必须指定的个数
shikong_modNum=function(player,slash)
	--local s= sgs.PlayerList()
	local num=0
	local rangefix = 0
	local ids = slash:getSubcards()
	if (player:getWeapon() and ids:contains(player:getWeapon():getId())) then
		if player:getAttackRange() >player:getAttackRange(false) then
			rangefix = rangefix +player:getAttackRange() - player:getAttackRange(false)
		end
	end
	if player:getOffensiveHorse() 
	and ids:contains(player:getOffensiveHorse():getId()) then
		rangefix=rangefix+1
	end

	for _,p  in sgs.qlist(player:getAliveSiblings())do
		if (player:inMyAttackRange(p)  and player:canSlash(p, slash, true, rangefix)) 
		or sgs.Slash_IsSpecificAssignee(p, player, slash) then
			num=num+1
		end
	end
	return math.max(1,num)
end
shikong = sgs.CreateTargetModSkill{ --还牵扯杀的ai。。。。
        name = "shikong",
        pattern="Slash",
        extra_target_func = function(self,player,card)
                if player:hasSkill(self:objectName()) and player:getPhase()==sgs.Player_Play then
                        return shikong_modNum(player,card)-1
                else
                        return 0
                end
        end,
}
--旧方案是写成了触发技 指定一人后，自动加入全部目标 


ronghui = sgs.CreateTriggerSkill{
	name = "ronghui", 
	frequency = sgs.Skill_Compulsory,
	events = {sgs.DamageCaused},  
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()	

		if event==sgs.DamageCaused then
			local damage = data:toDamage()
			local to = data:toDamage().to
			local card = data:toDamage().card
			if (damage.chain or damage.transfer or not damage.by_user) then return false end
			if not damage.from or not damage.to then return false end
			if damage.from:objectName() ==damage.to:objectName() then return false end
			
			if card:isKindOf("Slash") and (player:getPhase() == sgs.Player_Play) then  	
				local card_ids = sgs.IntList()
				for _,card in sgs.qlist(to:getCards("e")) do
					card_ids:append(card:getId())
				end
				if card_ids:length() == 0 then return false end
					touhou_logmessage("#TriggerSkill",player,self:objectName())
					room:notifySkillInvoked(player, self:objectName())
				local move=sgs.CardsMoveStruct()
				move.card_ids=card_ids
				move.from=to
				move.to_place=sgs.Player_DiscardPile		
				--move.reason.m_reason == sgs.CardMoveReason_S_REASON_DISCARD 						 	
				room:moveCardsAtomic(move, true)
				
			end
		end
	end
}
	
jubian = sgs.CreateTriggerSkill{
	name = "jubian",
	events = {sgs.Damage,sgs.CardFinished},
	frequency = sgs.Skill_Compulsory,
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if (player:getPhase() ~= sgs.Player_Play)  then return false end
		if event == sgs.Damage then
			local damage = data:toDamage()--创建伤害房间
			local to = data:toDamage().to
			local card = data:toDamage().card
			if card ==nil then return false end
			if (card:hasFlag("jubian_card"))  then 
				if not player:hasFlag("jubian_used") then
					touhou_logmessage("#TriggerSkill",player,self:objectName())
					room:notifySkillInvoked(player, self:objectName())
					
					local recov = sgs.RecoverStruct()
					recov.who = player
					recov.recover =1
					room:recover(player, recov)	
					room:setPlayerFlag(player, "jubian_used")
				end
			else
				room:setCardFlag(card, "jubian_card")
			end
		end
		if event == sgs.CardFinished then
			local card = data:toCardUse().card
			if card:hasFlag("jubian_card") and player:hasFlag("jubian_used") then
				player:setFlags("-jubian_used")
			end
		end	
	end,
}
	
hengxing = sgs.CreateTriggerSkill{
	name = "hengxing", 
	frequency = sgs.Skill_Compulsory,
	events = {sgs.EventPhaseStart}, 
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if(player:getPhase() == sgs.Player_Finish) then
			touhou_logmessage("#TriggerSkill",player,self:objectName())
			room:notifySkillInvoked(player, self:objectName())
			room:loseHp(player)
			player:drawCards(3)	
		end
	end
}	
shen004:addSkill(shikong)
shen004:addSkill(ronghui)	
shen004:addSkill(jubian)
shen004:addSkill(hengxing)	

 
 
--	【虚幻的萃聚之梦——伊吹萃香】 编号：00005   by三国有单  
shen005 = sgs.General(extension, "shen005", "touhougod", 0, false) 
--[源码改动]配合萃香的零体力概念  修改player.cpp下面相关函数getHp getMaxHp
--[源码改动]没有伤害 修改void Room::damage
--和其他角色双将时，体力可能会有问题
huanmeng = sgs.CreateTriggerSkill{--技能"幻梦"
	name = "huanmeng", 
	frequency = sgs.Skill_Eternal,
	priority=3,
		
	events = {sgs.GameStart,sgs.PreHpLost, sgs.TurnStart,sgs.EventPhaseChanging}, 
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if event==sgs.GameStart and player:isLord() then
			room:setPlayerProperty(player,"maxhp",sgs.QVariant(0))
			room:setPlayerProperty(player,"hp",sgs.QVariant(0))
		end
		if event ==sgs.PreHpLost then --没有掉体力的概念  应该优先于寒冰
			touhou_logmessage("#TriggerSkill",player,self:objectName())
			room:notifySkillInvoked(player, self:objectName())
			return true
		end 
		if  event==sgs.TurnStart  then
			if player:getHandcardNum()==0 then
				
            touhou_logmessage("#TriggerSkill",player,self:objectName())
			room:notifySkillInvoked(player, self:objectName())
				room:killPlayer(player)
			end
		end
		if event ==sgs.EventPhaseChanging then
			local change = data:toPhaseChange()
            if change.to == sgs.Player_Draw then
				touhou_logmessage("#TriggerSkill",player,self:objectName())
				room:notifySkillInvoked(player, self:objectName())
				player:skip(sgs.Player_Draw)
			end
			if change.to == sgs.Player_Discard then
                touhou_logmessage("#TriggerSkill",player,self:objectName())
				room:notifySkillInvoked(player, self:objectName())
				player:skip(sgs.Player_Discard)
            end
		end
	end
}	
cuixiang = sgs.CreateTriggerSkill{--技能"萃想"
	name = "cuixiang", 
	frequency = sgs.Skill_Compulsory,
		
	events = {sgs.EventPhaseStart}, 
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if player:getPhase() ~= sgs.Player_Start then return false end

			touhou_logmessage("#TriggerSkill",player,self:objectName())
			room:notifySkillInvoked(player, self:objectName())
		idlist = sgs.IntList()
		idlist1 = sgs.IntList()
		for _,p in sgs.qlist(room:getOtherPlayers(player)) do
			if p:getHandcardNum() >0 then
				local cards=room:askForExchange(p, self:objectName(), 1, false, "cuixiang-exchange:"..player:objectName()..":"..self:objectName())
                id = cards:getSubcards():first()
				idlist:append(id)
				room:throwCard(id,p,p)
			else
				cards = room:getNCards(1)
				idlist1:append(cards:first())
			end
		end
		--因为"纪念" 重新检查牌的位置
		ids=sgs.IntList()
		for _,id in sgs.qlist(idlist)do
			if room:getCardPlace(id)==sgs.Player_DiscardPile then
				ids:append(id)
			end
		end
		for _,id in sgs.qlist(idlist1)do
			ids:append(id)
		end
		local x =ids:length()
		if x> 2 then x =2 end
		gotlist=sgs.IntList()
		for var =1, x ,1 do
			room:fillAG(ids, nil)
			card_id = room:askForAG(player, ids, false, "cuixiang_chose")
			gotlist:append(card_id)
			room:obtainCard(player,card_id,true)
			ids:removeOne(card_id)
			room:clearAG()
		end
	end
}	
xuying = sgs.CreateTriggerSkill{--技能"虚影"
	name = "xuying", 
	frequency = sgs.Skill_Eternal,
	priority=3,	
	events = {sgs.SlashEffected ,sgs.SlashHit,sgs.SlashMissed}, 
	can_trigger=function(self,player)
		return player
	end,
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if event == sgs.SlashEffected then
			 local effect = data:toSlashEffect()
			if effect.to:objectName() == player:objectName() and player:hasSkill("xuying") then
				player:setFlags("xuying")
			end
		end
		if event == sgs.SlashHit then
			 local effect = data:toSlashEffect()
			if effect.to:hasFlag("xuying") then
				if effect.to:getHandcardNum()>0 then
					x =effect.to:getHandcardNum() / 2
					if x<1 then x=1 end
					touhou_logmessage("#TriggerSkill",effect.to,self:objectName())
					room:notifySkillInvoked(effect.to, self:objectName())
					room:askForDiscard(effect.to,"xuying",x,x,false,false,"xuying_discard:"..tostring(x))
					effect.to:setFlags("-xuying")
				end
			end
		end
		if event == sgs.SlashMissed then
			local effect = data:toSlashEffect()
			if effect.to:hasFlag("xuying") then
				touhou_logmessage("#TriggerSkill",effect.to,self:objectName())	
				room:notifySkillInvoked(effect.to, self:objectName())
				effect.to:drawCards(1)
				effect.to:setFlags("-xuying")
			end
		end
		if event ==sgs.CardFinished then --灵气等让杀无效化，之后也得消除flag? falg保持一回合？ 还得考虑连弩杀
			if player:hasFlag("xuying") then  player:setFlags("-xuying") end
		end
	end
}

shen005:addSkill(huanmeng)	
shen005:addSkill(cuixiang)	
shen005:addSkill(xuying)	


--【绯色月下——芙兰朵露•斯卡雷特】 编号：00006 --新版本by三国有单
shen006 = sgs.General(extension, "shen006", "touhougod", 3, false)
kuangyan = sgs.CreateTriggerSkill{
	name = "kuangyan",
	events = {sgs.Dying},
	
	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		local who=room:getCurrentDyingPlayer()
		local current=room:getCurrent()
		if not current:isAlive() then return false end
		if player:objectName()==current:objectName() then return false end
		if player:objectName()~=who:objectName() then return false end
		if player:askForSkillInvoke(self:objectName(),sgs.QVariant("recover:"..current:objectName())) then
			player:gainMark("@kinki")
			local recover=sgs.RecoverStruct()
            recover.recover= 1 - player:getHp()
            room:recover(player,recover)

			if current:isAlive() then
				local damage = sgs.DamageStruct()
                damage.from = player
                damage.to = current
                room:damage(damage)
			end
		end
	end
}

huimie_card=sgs.CreateSkillCard{
name="huimie",
target_fixed=false,
will_throw=true,
filter=function(self,targets,to_select)
        return #targets==0 and to_select:objectName()~=sgs.Self:objectName()
end,
on_use = function(self, room, source, targets)
     local target = targets[1]
	 source:gainMark("@kinki")
	 if target:isChained() then
		t=sgs.QVariant(false)
		room:setPlayerProperty(target, "chained", t)
	 else
		t=sgs.QVariant(true)
		room:setPlayerProperty(target, "chained", t)
	 end
	 local damage = sgs.DamageStruct(self:objectName(), source, target,1, sgs.DamageStruct_Fire)
	 room:damage(damage)
end
}

huimie = sgs.CreateViewAsSkill{
        name = "huimie",
        n = 0,
        view_as = function (self,cards)
                return huimie_card:clone()
        end,

        enabled_at_play = function(self, player)
                return not player:hasUsed("#huimie")
        end


}

jinguo = sgs.CreateTriggerSkill{
	name = "jinguo",
	frequency = sgs.Skill_Compulsory,
	events = {sgs.EventPhaseEnd},

	on_trigger = function(self,event,player,data)
		local room = player:getRoom()
		if event==sgs.EventPhaseEnd and player:getPhase() == sgs.Player_Play then
						touhou_logmessage("#TriggerSkill",player,self:objectName(),logto,logarg2,logtotype)
						room:notifySkillInvoked(player, self:objectName())
			local judge = sgs.JudgeStruct()
            judge.who = player
            judge.good=true
			judge.pattern = ".|spade"
            judge.negative = false
            judge.play_animation = true
            judge.reason = self:objectName()
            room:judge(judge)
			
			if not judge:isGood() then 
				x=player:getMark("@kinki")
				if x==0 then return false end
				y= math.ceil(x/2)
				if x>player:getHandcardNum() then
					room:loseHp(player,y)
				else
					if room:askForDiscard(player,self:objectName(),x,x,true,true,"@jinguo:"..tostring(x)..":"..tostring(y)) then
					else
						room:loseHp(player,y)
					end
				end
			end
		end
	end
}

shen006:addSkill(kuangyan)
shen006:addSkill(huimie)
shen006:addSkill(jinguo)

		
--【铭刻的月之钟——十六夜咲夜】 编号：00007 by三国有单
shen007 = sgs.General(extension, "shen007", "touhougod", 3, false) --00007
shicao = sgs.CreateTriggerSkill{--技能"时操"
	name = "shicao", 
	frequency = sgs.Skill_Compulsory,
	events = {sgs.EventPhaseStart},
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()--创建房间
		if(player:getPhase() == sgs.Player_Start) then
			local log=sgs.LogMessage()
            log.from=player
            log.arg=self:objectName()
            log.type="#TriggerSkill"
            room:sendLog(log)
			room:notifySkillInvoked(player, self:objectName())			
			
			if player:getMark("@clock") >0 then
				player:loseAllMarks("@clock")
			end
			if player:getMark("touhou-extra")==0 then
				player:gainMark("@clock", 1)
			end
		end
	end
}

--[源码改动]对于额外回合 没有明确mark 刘禅 神司马
--因此修改源码 gainAnExtraTurn() 和 gamerule 
--对于额外回合给予一个明确标记touhou-extra，并管理此标记的加减

--时停插入额外回合的时机 必须比【断罪】等回合后晚
--方案1 phasechange phasestart  phase==noactive
--方案2 phasechanging from noactive to not noactive
shiting = sgs.CreateTriggerSkill{--技能"时停" --额外回合直接开限定技 幻在时要注意
        name = "shiting",
        priority = 1,
		events={sgs.EventPhaseChanging},
        can_trigger = function(self, target)
             return  target
        end,
        on_trigger = function(self, event, player, data)
            local room = player:getRoom()
            local current = room:getCurrent()
			local change = data:toPhaseChange()
			if change.from ==sgs.Player_NotActive and change.to ~=sgs.Player_NotActive then
			--if change.to ==sgs.Player_NotActive then
				local skillowner = room:findPlayerBySkillName(self:objectName())
				if skillowner and skillowner:getMark("@clock")>0 and current:getMark("touhou-extra")==0 and current:isAlive() then --存活检测不可少
					prompt="extraturn:"..current:objectName()
					if (not room:askForSkillInvoke(skillowner,self:objectName(),sgs.QVariant(prompt))) then return false end
                    if (skillowner:isAlive()) then
						--room:setPlayerMark(skillowner,"touhou-extra",1)
						skillowner:loseAllMarks("@clock")
						touhou_logmessage("#touhouExtraTurn",skillowner,nil)
					
						skillowner:gainAnExtraTurn()
						--由于已经算作from notactive to XXphase
						--时停回合结束后，不能第二次触发from notactive to XXphase
						--手动触发（保证额外回合后 开限定技幻在还能直接联动）
						qdata=sgs.QVariant()
						local change1 = sgs.PhaseChangeStruct()
						change1.from=sgs.Player_NotActive
						change1.to=change.to
						qdata:setValue(change)
						room:getThread():trigger(sgs.EventPhaseChanging, room, skillowner, qdata)

						--room:setPlayerMark(skillowner,"touhou-extra",0)
					end
				end
			end
            return false
        end
}

--[[shiting = sgs.CreatePhaseChangeSkill{--技能"时停" 
        name = "shiting" ,
        priority = 1 ,
        can_trigger = function(self, target)
             return  target:getPhase() == sgs.Player_NotActive
        end,
        on_phasechange = function(self, player)
            local room = player:getRoom()
            local current = room:getCurrent()
			local skillowner = room:findPlayerBySkillName(self:objectName())
			--只对神16夜自己技能带来的这种额外回合做了判断。。。 别人的技能带来的额外回合该如何弄？
            if skillowner and skillowner:getMark("@clock")>0  and current:getMark("touhou-extra")==0 then
					if (not room:askForSkillInvoke(skillowner,self:objectName())) then return false end
                    if (skillowner:isAlive()) then
					--room:setPlayerMark(skillowner,"touhou-extra",1)
					skillowner:loseAllMarks("@clock")
					touhou_logmessage("#touhouExtraTurn",skillowner,nil,current:getNext())
								
					skillowner:gainAnExtraTurn()
					--room:setPlayerMark(skillowner,"touhou-extra",0)
                end
            end
            return false
        end
}]]

huanzai = sgs.CreateTriggerSkill{--技能"幻在"
	name = "huanzai", 
	events = {sgs.GameStart,sgs.EventPhaseStart},  
	frequency = sgs.Skill_Limited,
	limit_mark = "@huanzai",
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if event==sgs.GameStart then
		end
		if(event ==sgs.EventPhaseStart) and (player:getPhase() == sgs.Player_Finish) then
			if player:getMark("@clock") ==0 and player:getMark("@huanzai") > 0 then
				if (not room:askForSkillInvoke(player,self:objectName())) then return false end
				player:gainMark("@clock", 1)
				room:removePlayerMark(player, "@huanzai")
			end
		end
	end
}

shanghun = sgs.CreateTriggerSkill{--技能"伤魂"
	name = "shanghun", 
	frequency = sgs.Skill_Limited,
	limit_mark = "@shanghun",
	events = {sgs.GameStart,sgs.Damaged},
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()--创建房间
		if event==sgs.GameStart then
		end
		if event==sgs.Damaged then
			if player:getMark("@clock") ==0 and  player:getMark("@shanghun") > 0 then
				if (not room:askForSkillInvoke(player,self:objectName())) then return false end
					player:gainMark("@clock", 1)
				room:removePlayerMark(player, "@shanghun")
			end
		end
	end
}
shen007:addSkill(shicao)
shen007:addSkill(shiting)
shen007:addSkill(huanzai)	
shen007:addSkill(shanghun)

 
 
--【贯穿幽冥的双刃——魂魄妖梦】编号：00008
--[源码改动] player::getHP()  room::recover()
--iswounded() getlosthp()因为getHP()改动而关联，没有修改
--Room::enterDying()貌似不用动？
--死亡的奖惩问题没有碰到
shen008 = sgs.General(extension, "shen008", "touhougod", 3, false) --00008
banling = sgs.CreateTriggerSkill{
        name = "banling",
        events = {sgs.GameStart, sgs.PreHpLost, sgs.DamageInflicted, sgs.PreHpRecover, sgs.MarkChanged},
		frequency = sgs.Skill_Eternal,
		priority = -1, --优先级很重要
		on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                
                if event == sgs.GameStart then
					room:setPlayerMark(player, "@lingtili", player:getMaxHp())
					room:setPlayerMark(player, "@rentili", player:getMaxHp())
					local value = sgs.QVariant(player:getMaxHp())
					room:setPlayerProperty(player, "hp", value)
                elseif event == sgs.PreHpLost then
                        for i = 1, data:toInt(), 1 do
                                local x = player:getMark("@lingtili")
								local y = player:getMark("@rentili")
                                local choices = {}
								if y > 0 then
                                    table.insert(choices,"rentili")
                                end
                                if x > 0 then
                                    table.insert(choices,"lingtili")
									
                                end
								
								choice = room:askForChoice(player, "banling_minus", table.concat(choices,"+"))
                                if choice == "lingtili" then
										room:setPlayerMark(player, "@lingtili", x - 1)
										touhou_logmessage("#lingtilidamage",player,"banling",nil,1)
                                        
                                end
								if choice == "rentili" then
										room:setPlayerMark(player, "@rentili", y - 1)
										touhou_logmessage("#rentilidamage",player,"banling",nil,1)
                                        
                                end
                        end
						x=player:getMark("@lingtili")
						y=player:getMark("@rentili")
						z=math.min(x, y);
						room:setPlayerProperty(player, "hp", sgs.QVariant(z))
						local thread=room:getThread()
						--qdata=sgs.QVariant()
						--qdata:setValue(data:toInt())
						-- HpChanged PostHpReduced
						local log=sgs.LogMessage()
						log.type = "#LoseHp"
						log.from = player
						log.arg = tostring(data:toInt())
						room:sendLog(log)
						log.arg = tostring(player:getHp())
						log.arg2 = tostring(player:getMaxHp())
						log.type = "#GetHp"
						room:sendLog(log)
						--local l = sgs.JsonValueForLUA()
                        --l:setStringAt(0, player:objectName())
                        --l:setStringAt(1, tostring(-data:toInt()))
                        --l:setStringAt(2,  tostring(-1))
                                
						--room:doBroadcastNotify(sgs.CommandType.S_COMMAND_CHANGE_HP, l);
						
						thread:trigger(sgs.PostHpReduced, room, player, data)
						--if player:getHp()<1 then
						--	room:enterDying(player, nil)
						--end
						return true
                elseif event == sgs.DamageInflicted then
                        local damage = data:toDamage()
                        local count=0
						
						for i = 1, damage.damage, 1 do
                                local x = player:getMark("@lingtili")
								local y = player:getMark("@rentili")
                                local choices = {}
								if y > 0 then
                                    table.insert(choices,"rentili")
                                end
                                if x > 0 then
                                    table.insert(choices,"lingtili")
                                end
								
								choice = room:askForChoice(player, "banling_minus", table.concat(choices,"+"))
                                if choice == "lingtili" then
										room:setPlayerMark(player, "@lingtili", x - 1)
										touhou_logmessage("#lingtilidamage",player,"banling",nil,1)
                                end
								if choice == "rentili" then
										room:setPlayerMark(player, "@rentili", y - 1)
										touhou_logmessage("#rentilidamage",player,"banling",nil,1)
                                end
                        end
						--该是清算铁锁和触发伤害的时候了
						local thread=room:getThread()
						local qdata=sgs.QVariant() --注意要有local 否则插入伤害时会出问题
						qdata:setValue(damage)
						
						thread:trigger(sgs.PreDamageDone, room, damage.to, qdata)
							
							thread:trigger(sgs.DamageDone, room, damage.to, qdata)
							
							x=damage.to:getMark("@lingtili")
							y=damage.to:getMark("@rentili")
							z=math.min(x, y);
							room:setPlayerProperty(damage.to, "hp", sgs.QVariant(z))
							
							thread:trigger(sgs.Damage, room, damage.from, qdata)
                           
							thread:trigger(sgs.Damaged, room, damage.to, qdata)
						
						thread:trigger(sgs.DamageComplete, room, damage.to, qdata)
						
						--触发完一系列事件之后再return
						
						return true
                        
                elseif event == sgs.PreHpRecover then
                        for i = 1, data:toRecover().recover, 1 do
                                local x = player:getMark("@lingtili")
                                local choice = "rentili"
								local y=player:getMark("@rentili")--真正的人体力
                                if x<player:getMaxHp()  and y<player:getMaxHp() then
                                        choice = room:askForChoice(player, "banling_plus", "rentili+lingtili")
                                else
									if x==player:getMaxHp() then
										choice = "rentili"
									else
										choice = "lingtili"
									end
								end
								if choice == "rentili" then
                                        room:setPlayerMark(player, "@rentili", y + 1)
										touhou_logmessage("#rentilirecover",player,"banling",nil,1)
                                        continue 
                                end
                                if choice == "lingtili" then
                                        room:setPlayerMark(player, "@lingtili", x + 1)
										touhou_logmessage("#lingtilirecover",player,"banling",nil,1)
                                        continue 
                                end
                        end
				end		
               --[[ elseif event == sgs.MarkChanged then --markchanged之后进入濒死
					local change = data:toMarkChange()
					if (change.name ~= "@rentili" or change.name ~= "@lingtili" )then return false end
					local mark = player:getMark("@rentili")
					local mark1 = player:getMark("@lingtili")
					local room = player:getRoom()
                
					if (mark <= 0) or (mark1<= 0) then
                        room:enterDying(player, nil)
					end
					return false
                end]]
                return false
        end
}

renguicard = sgs.CreateSkillCard{
        name = "rengui",
        filter = function(self, targets, to_select)
                return #targets == 0
        end,
        on_use = function(self, room, source, targets)
                local x = source:getMaxHp() - source:getMark("@lingtili")
                if x > 2 then
                        x = 2
                end
                targets[1]:drawCards(x)
				
				local y = source:getMaxHp() - source:getMark("@rentili")
                if y > 0 then
                        if y > 2 then
                                y = 2
                        end
                         local all = sgs.SPlayerList()
                        for _, p in sgs.qlist(room:getAlivePlayers()) do
                                if not p:isNude() then
                                        all:append(p)
                                end
                        end
                        if not all:isEmpty() then
                                local s = room:askForPlayerChosen(source, all, self:objectName(), "@rengui-discard:" .. tostring(y), true, true)
                                if s then
                                        local dummy = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
                                        local card_ids = {}
                                        local original_places = {}
                                        local count = 0
                                        s:setFlags("rengui_InTempMoving")
                                        for i = 1, y, 1 do
                                                if not source:canDiscard(s, "he") then break end
                                                local id = room:askForCardChosen(source, s, "he", self:objectName(),false, sgs.Card_MethodDiscard)
                                                table.insert(card_ids, id)
                                                local place = room:getCardPlace(id)
                                                table.insert(original_places, place)
                                                dummy:addSubcard(id)
                                                s:addToPile("#rengui", id, false)--先行把牌拿走？？
                                                count = count + 1
                                        end
                                        for i = 1, count, 1 do
                                                local card = sgs.Sanguosha:getCard(card_ids[i])
                                                room:moveCardTo(card, s, original_places[i], false)
                                        end
                                        s:setFlags("-rengui_InTempMoving")
                                        if count > 0 then
                                                room:throwCard(dummy, s, source)
                                        end
                                end
                        end
                end
                return false
        end
}

renguivs = sgs.CreateZeroCardViewAsSkill{
        name = "rengui" ,
        response_pattern = "@@rengui" ,
        view_as = function()
                return renguicard:clone()
        end
}

rengui = sgs.CreatePhaseChangeSkill{
        name = "rengui",
        view_as_skill = renguivs,
        on_phasechange = function(self, player)
                --在改了源码的情况下 可以用player:getLostHp()
				if player:getPhase() == sgs.Player_Start and (player:getMark("@rentili") <player:getMaxHp() or   player:getMark("@lingtili") <player:getMaxHp()) then
                    local x=  player:getMaxHp()- player:getMark("@lingtili")  
					player:getRoom():askForUseCard(player, "@@rengui", "@rengui:"..tostring(x))
                end
        end
}

renguifakemove = sgs.CreateFakeMoveSkill("rengui")

shen008:addSkill(banling)
shen008:addSkill(rengui)
shen008:addSkill(renguifakemove)

extension:insertRelatedSkills("rengui", "#rengui-fake-move")

	
--【狂气的赤眼——铃仙•优昙华院•稻叶】编号：00009 by三国有单
shen009 = sgs.General(extension, "shen009", "touhougod", 4, false) --00009
ningshi = sgs.CreateTriggerSkill{
		name = "ningshi", 
		frequency = sgs.Skill_Compulsory,
		events = { sgs.TargetConfirmed}, 
		on_trigger = function(self, event, player, data)
			local room = player:getRoom()
			local use = data:toCardUse()
			if not player:hasSkill(self:objectName()) then return false end
			if player:getPhase()~=sgs.Player_Play then return false end
			if use.from:objectName() == player:objectName() and  use.to:length()==1 and use.to:first():objectName() ~= player:objectName() then
				if use.card:isKindOf("Slash") or use.card:isKindOf("TrickCard") then
					target = use.to:first()
					touhou_logmessage("#TriggerSkill",player,self:objectName(),logto,logarg2,logtotype)
					room:notifySkillInvoked(player, self:objectName())
				
					target:gainMark("@kuangqi")
					x=target:getMark("@kuangqi")
					if target:getCards("he"):length() >= x and room:askForDiscard(target,"ningshi",x,x,true,true,"@ningshi:"..tostring(x)) then
						--加入手牌限制 主要是默认全弃置 然后弃置张数不到x也返回true。。。
					else
						y=x/2
						if y<1 then y=1 end
						target:loseMark("@kuangqi",y)
						room:loseHp(target,y)
					end
				end
				
			end
		end
}

gaoao = sgs.CreateTriggerSkill{
		name = "gaoao", 
		frequency = sgs.Skill_Compulsory,
		events = { sgs.PreHpRecover, sgs.CardsMoveOneTime}, 
		on_trigger = function(self, event, player, data)
			local room = player:getRoom()
			if player:getPhase() ~= sgs.Player_NotActive then return false end 
			if room:getTag("FirstRound"):toBool() == true then return false end
			if event == sgs.CardsMoveOneTime then
				local move = data:toMoveOneTime() 
				if move.to and move.to:objectName() ==player:objectName() then
					touhou_logmessage("#TriggerSkill",player,self:objectName(),logto,logarg2,logtotype)
					room:notifySkillInvoked(player, self:objectName())
					local dummy = sgs.Sanguosha:cloneCard("Slash", sgs.Card_NoSuit, 0)
					for _,id in sgs.qlist(move.card_ids) do							
						dummy:addSubcard(sgs.Sanguosha:getCard(id)) 	
					end	
					room:throwCard(dummy,player, player)
					
				end
			end
		end
}
		
shen009:addSkill(ningshi)	
shen009:addSkill(gaoao)

	
	
--【现代人的现人神——东风谷早苗】编号：00010 by三国有单
shen010 = sgs.General(extension, "shen010", "touhougod", 4, false) --00010
shenshou_choice = function(room,source,target,x,y,z,c,notice) 
	local choices = {}
	if x>0 then
		table.insert(choices,"shenshou_slash")
	end
	if y>0 then
		table.insert(choices,"shenshou_obtain")
	end
	if z>0 then
		table.insert(choices,"shenshou_draw")
	end
	table.insert(choices,"cancel")
	--for ai
		source:setTag("shenshou_x",sgs.QVariant(x))
		source:setTag("shenshou_y",sgs.QVariant(y))
		source:setTag("shenshou_z",sgs.QVariant(z))
		_data=sgs.QVariant()
		_data:setValue(target)
		source:setTag("shenshou_target",_data)
	--
	local choice = room:askForChoice(source,"shenshou",table.concat(choices, "+"))
	--for ai
		source:removeTag("shenshou_x")
		source:removeTag("shenshou_y")
		source:removeTag("shenshou_z")
	--
	--需要改为提前判断杀和顺手的合法性？
	if  choice =="shenshou_slash" then  
		listt =room:getOtherPlayers(target)
		for _,p in sgs.list(room:getOtherPlayers(target)) do
			if (not target:inMyAttackRange(p))  or (not target:canSlash(p,nil,false)) then
				listt:removeOne(p)
			end
		end		
		slashtarget = room:askForPlayerChosen(source, listt, "shenshou","@shenshou-slash:"..target:objectName())--@dummy-slash2
		if slashtarget then
			local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
			slash:setSkillName("shenshou")--ai也用得着
			local carduse=sgs.CardUseStruct()
			carduse.card=slash
			carduse.from=target
			carduse.to:append(slashtarget)
			room:useCard(carduse)		
		end
		x=0
	end
	if  choice =="shenshou_obtain" then 
		listt1 =room:getOtherPlayers(target)
		for _,p in sgs.list(room:getOtherPlayers(target)) do
			if (not target:inMyAttackRange(p)) or p:getHandcardNum()==0 then
				listt1:removeOne(p)  --
			end
		end
		cardtarget = room:askForPlayerChosen(source, listt1, "shenshou","@shenshou-obtain:"..target:objectName())
		if cardtarget then
			local card1 = room:askForCardChosen(target, cardtarget, "h", "shenshou")
			room:obtainCard(target,card1,false)			
		end
		y=0
	end
	if choice =="shenshou_draw" then 
		source:drawCards(1)
		z=0
	end
	if choice =="cancel" then 
		c=1
	end
	source:removeTag("shenshou_target")
	if c>0 or (x==0 and y==0 and z==0) then 
		notice=1
	end
	return notice ,x,y,z,c
end	
shenshou_card = sgs.CreateSkillCard{
	name = "shenshou",
	target_fixed = false,
	will_throw = false,
	
	filter = function(self, targets, to_select, player)
		return #targets == 0 and to_select:objectName() ~= player:objectName()
	end,
	
	on_use = function(self, room, source, targets)
		local target = targets[1]		
		cards= self:getSubcards() 
		
		local card = sgs.Sanguosha:getCard(cards:first())

		local x=0  --x记录杀 y记录黑桃 z记录点数 c记录取消
		local y=0
		local z=0
		local c=0
		if  card:isKindOf("Slash") then  x=1 end
		if card:getSuit() == sgs.Card_Spade then  y =1 end
		if card:getNumber()  >4 and  card:getNumber()  <10 then z =1 end
		notice=0
		if source:hasSkill("shenshou")  and source:getHandcardNum() ==1 then  --针对神代不能很好触发的权宜之计。。。
			x=1
			y=1
			z=1
		end
		
		room:showCard(source, cards:first())
		--补一个log
		tempcard=sgs.Sanguosha:cloneCard("slash",sgs.Card_Spade,5)

		local mes=sgs.LogMessage()
        mes.type="$ShenshouTurnOver"
        mes.from=source
		mes.arg="shenshou"
        mes.card_str=tempcard:toString() 
        room:sendLog(mes)
		room:moveCardTo(self, target, sgs.Player_PlaceHand, true)
		while  notice<1 do
			notice,x,y,z,c =shenshou_choice(room,source,target,x,y,z,c,notice) --自定义函数
		end
		
	end,
}
shenshou= sgs.CreateViewAsSkill{
	name = "shenshou",
	n = 1,
	
	enabled_at_play = function(self,player)
		return not player:hasUsed("#shenshou")
	end,
	
	view_filter = function(self, selected, to_select)
		return not to_select:isEquipped()
	end,
	
	view_as = function(self, cards)
		if(#cards ~= 1) then return nil end
		local qcard = shenshou_card:clone()
		qcard:addSubcard(cards[1])
		qcard:setSkillName(self:objectName())
		return qcard
	end
}	

shen010:addSkill(shenshou)	
 
 
--【大结界的守护者——博丽灵梦】编号：00011 by三国有单
--要重新修改。。。
--[源码改动]Player:distanceTo()
shen011 = sgs.General(extension, "shen011", "touhougod", 4, false) 
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
shen011:addSkill(jiejie)
shen011:addSkill(reimu_fengyin)
 
--【幻想乡的裁判长——四季映姬•亚玛萨那度】编号：00012
shen012 = sgs.General(extension, "shen012", "touhougod", 4, false) 
ymsndquanjie = sgs.CreatePhaseChangeSkill{
        name = "ymsndquanjie" ,
        can_trigger = function(self, player)
                return player and player:isAlive()
        end ,
        on_phasechange = function(self, player)
                if player:getPhase() ~= sgs.Player_Play  then return false end
                local room = player:getRoom()
                local ymsnd = room:findPlayerBySkillName(self:objectName())
				if not ymsnd then return false end
                if player:objectName() == ymsnd:objectName() then return false end
                local _data = sgs.QVariant()
                _data:setValue(player)
				--mark为了死欲中断 清除
                room:setPlayerMark(player, self:objectName(),0)
				if ymsnd:askForSkillInvoke(self:objectName(), _data) then
                        if not room:askForCard(player, "%slash,%thunder_slash,%fire_slash", "@ymsndquanjie-discard") then  
								player:drawCards(1)
                                room:setPlayerMark(player, self:objectName(),1)
								room:setPlayerCardLimitation(player, "use", "Slash", true)
                        end
						
                end
                return false
        end ,
}

--确保了断罪（原技能者的回合内）被凭依（变成凭依者的回合外），被常识情况下，也能准确计数和准确发动
--sgs.EventPhaseChanging change.to=xx 
--早于 CreatePhaseChangeSkill getphase()=xx 
ymsndduanzuicount = sgs.CreateTriggerSkill{
        name = "#ymsndduanzui-count" ,
        events = {sgs.Death, sgs.EventPhaseStart} ,
        can_trigger = function(self, player)
                return player
        end ,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                if event == sgs.Death then
                        local death = data:toDeath()
                        if (death.who:objectName() ~= player:objectName()) then return false end
                        --由于凭依可能导致技能变换，所以对于回合人的回合内外判断要延后到回合结束时判断。
						--local current = room:getCurrent()
                        --if current and (current:isAlive() or (death.who:objectName() == current:objectName()))
                        --                and (current:getPhase() ~= sgs.Player_NotActive) and (not current:hasSkill("ymsndduanzui")) then
                                room:setTag("ymsndduanzui", sgs.QVariant(true))
                        --end
                elseif player:getPhase() == sgs.Player_NotActive then
                        room:setTag("ymsndduanzui", sgs.QVariant(false))
				end
				
                return false
        end,
}
ymsndduanzui = sgs.CreateTriggerSkill{
        name = "ymsndduanzui",
        events = {sgs.EventPhaseChanging} ,
        --frequency = sgs.Skill_Frequent,
        priority = 1 ,
        can_trigger = function(self, target)
                return target
        end ,
        on_trigger = function(self, event, player, data)
                local change = data:toPhaseChange()
                if (change.to ~= sgs.Player_NotActive) then return false end
                local room = player:getRoom()
                local ymsnd = room:findPlayerBySkillName(self:objectName())
                if (not ymsnd) or (not room:getTag("ymsndduanzui"):toBool()) then return false end
                local current = room:getCurrent()
				room:setTag("ymsndduanzui", sgs.QVariant(false))
				if current:objectName()== ymsnd:objectName() then return false end
                if (not ymsnd:askForSkillInvoke(self:objectName())) then return false end
                
				ymsnd:gainMark("@duanzui-extra")
				touhou_logmessage("#touhouExtraTurn",ymsnd,nil,current:getNext())
				ymsnd:gainAnExtraTurn()
                ymsnd:loseMark("@duanzui-extra")
				
				--local _data = sgs.QVariant()
				
				
                --_data:setValue(ymsnd)
                --room:setTag("ymsndduanzuiInvoke", _data)
                return false
        end ,
}
--[[ymsndduanzui_clear=sgs.CreateTriggerSkill{
	name="ymsndduanzui_clear",
	events={sgs.EventPhaseChanging},
	on_trigger = function(self, event, player, data)
		local room=player:getRoom()
		if event ==sgs.EventPhaseChanging then
			local change = data:toPhaseChange()
			if change.to == sgs.Player_NotActive then
				if player:getMark("@duanzui-extra")>0 then
					player:loseMark("@duanzui-extra")	
				end
				
			end
		end	
	end
}]]

--[[ymsndduanzuido = sgs.CreatePhaseChangeSkill{
        name = "#ymsndduanzui-do" ,
        priority = 1 ,
        can_trigger = function(self, target)
            return target and target:getPhase() == sgs.Player_NotActive
        end ,
        on_phasechange = function(self, player)
                local room = player:getRoom()
                local target = room:getTag("ymsndduanzuiInvoke"):toPlayer()
                if target then
                        room:setTag("ymsndduanzuiInvoke", sgs.QVariant())
                        if (target:isAlive()) then
							target:gainMark("@duanzui-extra")
							--room:setPlayerMark(target,"touhou-extra",1)
							target:gainAnExtraTurn()
                            target:loseMark("@duanzui-extra")
						end
                end
                return false
        end
}]]
ymsndduanzuishenpan = sgs.CreateTriggerSkill{ --需要技能库里面有"ymsndshenpan"技能，由于此技能与09001联动，所以！！！！切勿更改技能名！！！！
        name = "#ymsndduanzui-shenpan" ,
        events = {sgs.PreMarkChange} ,
         can_trigger = function(self, target)--被凭依后，也要能正确消除临时的技能
            return (target:getMark("@duanzui-extra")>0 or target:hasSkill("ymsndduanzui")) and target:getPhase() == sgs.Player_NotActive
        end ,
		on_trigger = function(self, event, player, data)
                local change = data:toMarkChange()
                if (change.name == "@duanzui-extra") then
                        if (change.num > 0) and (player:getMark("@duanzui-extra") == 0) and (not player:hasSkill("ymsndshenpan")) then
                                player:getRoom():handleAcquireDetachSkills(player, "ymsndshenpan")
                        elseif (change.num < 0) and (player:getMark("@duanzui-extra") + change.num <= 0) and (player:hasSkill("ymsndshenpan")) then
                                player:getRoom():handleAcquireDetachSkills(player, "-ymsndshenpan")
                        end
                end
                return false
        end,
}


shen012:addSkill(ymsndquanjie)
shen012:addSkill(ymsndduanzuicount)
shen012:addSkill(ymsndduanzui)
--shen012:addSkill(ymsndduanzui_clear)
--shen012:addSkill(ymsndduanzuido)
shen012:addSkill(ymsndduanzuishenpan)

extension:insertRelatedSkills("ymsndduanzui", "#ymsndduanzui-count")
--extension:insertRelatedSkills("ymsndduanzui", "#ymsndduanzui-clear")
--extension:insertRelatedSkills("ymsndduanzui", "#ymsndduanzui-do")
extension:insertRelatedSkills("ymsndduanzui", "#ymsndduanzui-shenpan")

		
--【龙神的化身——红美铃】编号：00013
shen013 = sgs.General(extension, "shen013", "touhougod", 4, false) --00013
hualong = sgs.CreateTriggerSkill{
        name = "hualong",
        frequency = sgs.Skill_Wake,
        events = sgs.EventPhaseStart,
        on_trigger = function(self,event,player,data)
                if player:getPhase() == sgs.Player_Start then
                        if player:getHp() == 1 and player:getMark("hualong") == 0 then
                                local room = player:getRoom()
								touhou_logmessage("#HunziWake",player,self:objectName())
								room:notifySkillInvoked(player, self:objectName())
								if room:changeMaxHpForAwakenSkill(player) then
								
									local recover = sgs.RecoverStruct()
									recover.recover = player:getLostHp()
									room:recover(player,recover)
									player:gainMark("hualong")
									
								end
                        end
                end
        end
}

meilingluanwu = sgs.CreateViewAsSkill{
        name = "meilingluanwu",
        n = 2,
        view_filter = function(self,selected,to_select)
                if sgs.Self:getMark("hualong") > 0 then
                        return #selected == 0
                else
                        return #selected < 2
                end                
        end,
        view_as = function(self,cards)
                local usereason = sgs.Sanguosha:getCurrentCardUseReason()
                if #cards == 0 then
                        return nil
                elseif #cards == 1 and sgs.Self:getMark("hualong") > 0 then
                        local card = cards[1]
                        local suit = card:getSuit()
                        local point = card:getNumber()
                        local id = card:getId()
                        if usereason == sgs.CardUseStruct_CARD_USE_REASON_PLAY then
                                local slash = sgs.Sanguosha:cloneCard("slash",suit,point)
                                slash:addSubcard(card)
                                slash:setSkillName(self:objectName())
                                return slash
                        elseif (usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE) or (usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
                                local pattern = sgs.Sanguosha:getCurrentCardUsePattern()
                                if pattern == "jink" then
                                        local jink = sgs.Sanguosha:cloneCard("jink", suit,point)
                                        jink:addSubcard(card)
                                        jink:setSkillName(self:objectName())
                                        return jink
                                elseif pattern == "slash" then
                                        local slash = sgs.Sanguosha:cloneCard("slash",suit,point)
                                        slash:addSubcard(card)
                                        slash:setSkillName(self:objectName())
                                        return slash
                                else
                                        return nil
                                end
                        end
                elseif #cards == 2 then
                        local card = sgs.CardList()
                        for p=1,2,1 do
                                card:append(cards[p])
                        end
                        if usereason == sgs.CardUseStruct_CARD_USE_REASON_PLAY then
                                local slash = sgs.Sanguosha:cloneCard("slash",sgs.Card_SuitToBeDecided, 0)
                                slash:addSubcards(card)
                                slash:setSkillName(self:objectName())
                                return slash
                        elseif (usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE) or (usereason == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE) then
                                local pattern = sgs.Sanguosha:getCurrentCardUsePattern()
                                if pattern == "jink" then
                                        local jink = sgs.Sanguosha:cloneCard("jink",sgs.Card_SuitToBeDecided, 0)
                                        jink:addSubcards(card)
                                        jink:setSkillName(self:objectName())
                                        return jink
                                elseif pattern == "slash" then
                                        local slash = sgs.Sanguosha:cloneCard("slash",sgs.Card_SuitToBeDecided, 0)
                                        slash:addSubcards(card)
                                        slash:setSkillName(self:objectName())
                                        return slash
                                else
                                        return nil
                                end
                        end
                end
        end,

    enabled_at_play = function(self, target)
                        return sgs.Slash_IsAvailable(target)
        end,
        enabled_at_response = function(self, target, pattern)
                        return (pattern == "slash") or (pattern == "jink")
        end
}

longwei = sgs.CreateTriggerSkill{
        name = "longwei",
        events = sgs.TargetConfirming,
        on_trigger = function(self,event,player,data)
                local use = data:toCardUse()
                local room = player:getRoom()
                if use.from:objectName() ~= player:objectName() and use.to:contains(player) and (use.card:isKindOf("Slash") or use.card:isNDTrick()) then
                        _data=sgs.QVariant()
						_data:setValue(use.from)
						if room:askForSkillInvoke(player,self:objectName(),_data) then
                                if player:getMark("hualong") > 0 then
                                        room:askForDiscard(use.from,self:objectName(),2,2,false,false,"@longwei-askfordiscard:"..player:objectName()..":"..tostring(2))
                                else
                                        room:askForDiscard(use.from,self:objectName(),1,1,false,false,"@longwei-askfordiscard:"..player:objectName()..":"..tostring(1))
                                end
							
                        end
						
                end
        end

}

shen013:addSkill(hualong)
shen013:addSkill(meilingluanwu)
shen013:addSkill(longwei)

		
--【月都大贤者——八意永琳】编号：00014 by三国有单
shen014 = sgs.General(extension, "shen014", "touhougod", 4, false) --00014
--[源码改动]洗牌次数的计数直接加入room.cpp的函数swapPile
qiannian_draw = sgs.CreateDrawCardsSkill{
    name = "#qiannian_draw",
    is_initial = false,
	
    draw_num_func = function(self, player, n)
		return n +player:getMark("@qiannian")
    end ,
}

qiannian_max = sgs.CreateMaxCardsSkill{
    name = "#qiannian_max",
    extra_func = function (self,target)
        if target:getMark("@qiannian")>0 then
           return target:getMark("@qiannian")
        end
    end

}
qiannian = sgs.CreateTriggerSkill{
	name = "qiannian",
	events = {sgs.GameStart,sgs.PreMarkChange},
	frequency = sgs.Skill_Compulsory,
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		local log=sgs.LogMessage()
		log.type = "#TriggerSkill"
		log.from = player
		log.arg = self:objectName()
				
		if event ==sgs.GameStart then
			player:gainMark("@qiannian",1)
		end
		if event ==sgs.PreMarkChange then
			local change = data:toMarkChange()
            if (change.name == "@qiannian") then
                if (change.num > 0)   then
					room:sendLog(log)
					room:notifySkillInvoked(player, self:objectName())
				end
            end
		end
	end
}

shen014:addSkill(qiannian)
shen014:addSkill(qiannian_max)
shen014:addSkill(qiannian_draw)
extension:insertRelatedSkills("qiannian", "#qiannian_max")
extension:insertRelatedSkills("qiannian", "#qiannian_draw")

 
--【大和神——八坂神奈子】编号：00015  by三国有单
shen015 = sgs.General(extension, "shen015", "touhougod", 4, false)
qinlueCard = sgs.CreateSkillCard{
    name = "qinlueCard",
    target_fixed = false,
    will_throw = false,
    filter = function(self, targets, to_select, player)
       local id = player:getMark("qinlue_use")-1
       local card =sgs.Sanguosha:getCard(id)	
	   local users=player:getAliveSiblings()
	   local user
	   for _, p in sgs.qlist(users) do
			--if p:getPhase()==sgs.Player_Play then 
			if p:hasFlag("Global_qinlueFailed") then
				user=p
				break
			end
	   end
	   if not user then return false end
	   if sgs.Sanguosha:isProhibited(user, to_select, card) then return false end
	   return card:targetFilter(targetsTable2QList(targets), to_select, user)
    end,
	
	feasible = function(self, targets, player)
        if #targets == 0 then return false end
        
		local id = player:getMark("qinlue_use")-1
		local card =sgs.Sanguosha:getCard(id)
        local users=player:getAliveSiblings()
	   local user
	   for _, p in sgs.qlist(users) do
			--if p:getPhase()==sgs.Player_Play then 
			if p:hasFlag("Global_qinlueFailed") then
				user=p
				break
			end
	   end
		if not user then return false end
		for _, p in ipairs(targets) do
            if sgs.Sanguosha:isProhibited(user, p, card) then return false end
        end
        return card:targetsFeasible(targetsTable2QList(targets), user)
    end,
	
	about_to_use = function(self, room, cardUse)
    --on_use里target顺序被sort，不能反映UI时选择顺序
		local from = cardUse.from
		local id = from:getMark("qinlue_use")-1
		local card =sgs.Sanguosha:getCard(id)
		if  card:isKindOf("Collateral") then 
			local users=room:getOtherPlayers(from)
			local user=room:getCurrent()
			--choicemade后 flag已经没了
			--[[for _, p in sgs.qlist(users) do
				--if p:getPhase()==sgs.Player_Play then 
				if p:hasFlag("Global_qinlueFailed") then
					user=p
					break
				end
			end]]
			if not user then return false end 
			
			touhou_logmessage("#ChoosePlayerWithSkill",from,"qinlue",cardUse.to,nil,1)
	
			--card:setSkillName("_chuangshi")--ai用得着
			local use=sgs.CardUseStruct()
			use.from=user
			use.to=cardUse.to
			use.card=card
			room:useCard(use)
		else
			self:cardOnUse(room, cardUse)--声明about to use 后需要引用onuse
		end
	end,
	--借刀的处理放在about_to_use里
    on_use = function(self, room, source, targets)
        local id = source:getMark("qinlue_use")-1
		local card =sgs.Sanguosha:getCard(id)
        if card:isKindOf("Collateral") then return false end
		local users=room:getOtherPlayers(source)
	   local user=room:getCurrent()
	   
	   --[[for _, p in sgs.qlist(users) do
			--if p:getPhase()==sgs.Player_Play then 
			if p:hasFlag("qinlue") then
				user=p
				break
			end
	   end]]
	   if not user then return false end
	   --room:setPlayerMark(user, "chuangshi_user", 0)
	   local carduse=sgs.CardUseStruct()
		--card:setSkillName("_chuangshi")--ai用得着
		carduse.card=card
		carduse.from=user
		for var=1, #targets,1 do
			carduse.to:append(targets[var])
		end
		room:sortByActionOrder(carduse.to)
		room:useCard(carduse,true)
    end
}
qinlueVS = sgs.CreateViewAsSkill{
    name = "qinlue",
    n = 0,
                view_filter = function(self, selected, to_select)
					return  true
                end,
                view_as = function(self, cards)          
                    local card = qinlueCard:clone()
                    return card
                end,
				
                enabled_at_play = function(self, player)
                    return false
                end,
				
				enabled_at_response = function(self, player, pattern)
					return pattern == "@@qinlue"
				end,
}
qinlue=sgs.CreateTriggerSkill{
	name="qinlue",
	events={sgs.EventPhaseChanging,sgs.DamageCaused,sgs.PreCardUsed},--sgs.EventPhaseStart,
	view_as_skill = qinlueVS,
	can_trigger = function(self, player)
       return player
    end ,
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		local source = room:findPlayerBySkillName(self:objectName())
        local current= room:getCurrent()
		if not source or not current then return false end
		if not source:isAlive() then return false end--被葛笼 亡舞？
		if source:objectName()==current:objectName() then return false end
		--if current:getPhase()~=sgs.Player_Play then return false end
		if event == sgs.EventPhaseChanging then
			local change = data:toPhaseChange()
			if change.to ~=sgs.Player_Play then
				return false
			end
			room:setPlayerMark(source, "qinlue", 1)
			--local prompt="target:"..current:objectName()
			local use_slash =room:askForUseSlashTo(source, current, "@qinlue-slash:"..current:objectName(),false, true, false) 
			room:setPlayerMark(source, "qinlue", 0)
		end
		if event == sgs.PreCardUsed then
            local use = data:toCardUse()
			if (source:getMark("qinlue") > 0) and use.card and use.card:isKindOf("Slash") then
				room:setPlayerMark(source, "qinlue", 0)
				room:setCardFlag(use.card, "qinlue")				
			end
		end
		if event==sgs.DamageCaused then
			local damage =data:toDamage() 
			if damage.card:hasFlag(self:objectName())
			and  damage.to:objectName()==current:objectName() 
			--and damage.to:getPhase()==sgs.Player_Play 
			then
				--获得手牌
				if damage.to:isKongcheng() then return false end
				local id1=-1
				local id2=-1
				room:fillAG(damage.to:handCards(),source)
				 id1=room:askForAG(source,damage.to:handCards(),true,self:objectName())
                room:clearAG(source)
                if id1>-1 then
                   room:obtainCard(player,id1,false)
                end
				--令其使用
				--canuse disable 的排除顺序重要
				local canuse=sgs.IntList()
				local disabled=sgs.IntList()
				if not damage.to:isKongcheng() then
					for _,id in sgs.qlist(damage.to:handCards())do
						local card=sgs.Sanguosha:getCard(id)
						if card:isKindOf("Jink") or card:isKindOf("Nullification") then
							disabled:append(id)
						elseif current:isCardLimited(card,sgs.Card_MethodUse) then
							disabled:append(id)
						elseif card:isKindOf("Peach") and  damage.to:isWounded() then
							canuse:append(id)
						elseif not card:cardIsAvailable(current) then
							disabled:append(id)
						elseif card:isKindOf("EquipCard") then
							canuse:append(id)
						elseif card:isKindOf("Analeptic") then
							canuse:append(id)
						end
						if not disabled:contains(id) then
							if card:isKindOf("Slash") then
								for _,p in sgs.qlist(room:getOtherPlayers(current)) do
									if damage.to:canSlash(p,card,true) then
										canuse:append(id)
										break
									end
								end
							elseif card:isKindOf("TrickCard") and card:objectName()~="ex_nihilo" then
								if card:isKindOf("GlobalEffect")  then
									canuse:append(id)
								else
									for _,p in sgs.qlist(damage.to:getAliveSiblings()) do
										if  not damage.to:isProhibited(p, card) then
											local targets={}
											if card:targetFilter(targetsTable2QList(targets), p, damage.to) then
												canuse:append(id)
												break
											end
										end 
									end
								end
								
								
							end
						end
						--只剩下杀和trick没做判断
						if not disabled:contains(id) and  not canuse:contains(id) then 
							disabled:append(id)
						end
					end
				end

				if canuse:length()>0 then
					room:fillAG(damage.to:handCards(), source, disabled)
					 id2 = room:askForAG(source, canuse, true, self:objectName())
					room:clearAG(source)
					if id2>-1 then
						room:setPlayerMark(source,"qinlue_use",id2+1)
						local tmpcard=sgs.Sanguosha:getCard(id2)
						local carduse=sgs.CardUseStruct()
						carduse.card=tmpcard
						carduse.from=damage.to
						local choice=tmpcard:objectName()
						local choices={"amazing_grace","ex_nihilo","peach","analeptic"}
						if table.contains(choices,choice) or tmpcard:isKindOf("EquipCard") 
						or tmpcard:isKindOf("AOE") or tmpcard:isKindOf("GlobalEffect") then
							room:useCard(carduse,true)
						else
							room:setPlayerFlag(damage.to, "Global_qinlueFailed")
							local c_card =room:askForUseCard(source,"@@qinlue", "@qinlue_victim:"..current:objectName()..":"..tmpcard:objectName())
						end
					end
				end
				--取消伤害
				--room:setPlayerFlag(damage.to, "Global_PlayPhaseTerminated")
				--touhou_logmessage("#jiezou_skip",damage.to,"jiezou_skip",nil,self:objectName())
				damage.to:skip(sgs.Player_Play)
				--if id1>-1 or id2>-1 then
					return true
				--end
				
			end
		end
	end
}


yuzhuVS = sgs.CreateOneCardViewAsSkill{
        name = "yuzhu" ,
        view_filter = function(self, card)
            if card:isKindOf("TrickCard") or card:isKindOf("BasicCard") then return false end
                if (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_PLAY) then
                        local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_SuitToBeDecided, -1)
                        slash:addSubcard(card)
                        return slash:cardIsAvailable(sgs.Self) and sgs.Slash_IsAvailable(sgs.Self, slash)
                end
                return true
        end,
        view_as = function(self, card)
                local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_SuitToBeDecided, -1)
                slash:addSubcard(card)
                slash:setSkillName(self:objectName())
                return slash
        end ,
        enabled_at_play = function(self, player)
                return sgs.Slash_IsAvailable(player)
        end ,
        enabled_at_response = function(self, player, pattern)
                return pattern == "slash" and (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE)
        end ,
}
yuzhu=sgs.CreateTriggerSkill{
	name="yuzhu",
	events={sgs.TargetConfirmed,sgs.SlashProceed},
	view_as_skill=yuzhuVS,
	
	on_trigger=function(self,event,player,data)
		local room = player:getRoom()
		if event==sgs.TargetConfirmed then
			local use = data:toCardUse()
			if use.card and use.from:objectName() == player:objectName()
			and use.card:isKindOf("Slash") and  use.card:getSkillName()==self:objectName() then
                for _, p in sgs.qlist(use.to) do
                    p:addQinggangTag(use.card)
                end

				--touhou_logmessage("#TriggerSkill",player,"yuzhu")
				--room:notifySkillInvoked(player, self:objectName())
				room:setEmotion(player, "weapon/qinggang_sword")			
            end
		end
		if event == sgs.SlashProceed then
             local effect = data:toSlashEffect()
             if effect.slash:getSkillName()==self:objectName() then
                room:slashResult(effect, nil)
				return true
			end
		end
	end
}
--[[yuzhu = sgs.CreateOneCardViewAsSkill{
        name = "yuzhu" ,
        view_filter = function(self, card)
                if card:isKindOf("TrickCard") then return false end
				if card:isKindOf("BasicCard") and (not card:isKindOf("Jink"))  then return false end
                if (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_PLAY) then
                        local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_SuitToBeDecided, -1)
                        slash:addSubcard(card)
                        return slash:cardIsAvailable(sgs.Self) and sgs.Slash_IsAvailable(sgs.Self, slash)
                end
                return true
        end ,
        view_as = function(self, card)
                local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_SuitToBeDecided, -1)
                slash:addSubcard(card)
                slash:setSkillName(self:objectName())
                return slash
        end ,
        enabled_at_play = function(self, player)
                return sgs.Slash_IsAvailable(player)
        end ,
        enabled_at_response = function(self, player, pattern)
                return pattern == "slash"
        end ,
}]]

shen015:addSkill(qinlue)
shen015:addSkill(yuzhu)


--【灭除八苦的尼公——圣白莲】编号：00016
shen016 = sgs.General(extension, "shen016", "touhougod", 3, false)
--界面刷新问题？
--追加log
function targetsTable2QList(thetable)
        local theqlist = sgs.PlayerList()
        for _, p in ipairs(thetable) do
                theqlist:append(p)
        end
        return theqlist
end
function cardCanRecast(card)
        return card:canRecast() and (not card:isKindOf("Weapon"))
end
sblchaorenpreventrecast = sgs.CreateSkillCard{
        name = "sblchaorenpreventrecast" ,
        will_throw = false ,
        handling_method = sgs.Card_MethodUse ,
        filter = function(self, targets, to_select, player)
                local card = sgs.Sanguosha:getCard(player:property("sblchaoren"):toInt())
                if sgs.Sanguosha:isProhibited(player, to_select, card) then return false end
                
				return card:targetFilter(targetsTable2QList(targets), to_select, player)
        end ,
        feasible = function(self, targets, player)
                if #targets == 0 then return false end
                local card = sgs.Sanguosha:getCard(player:property("sblchaoren"):toInt())
                for _, p in ipairs(targets) do
                        if sgs.Sanguosha:isProhibited(player, p, card) then return false end
                end
                return card:targetsFeasible(targetsTable2QList(targets), player)
        end ,
        on_validate = function(self, cardUse)
              card=   sgs.Sanguosha:getCard(cardUse.from:property("sblchaoren"):toInt())
			card:setSkillName("sblchaoren")
			return card
		end ,
}
sblchaorenvs = sgs.CreateZeroCardViewAsSkill{
        name = "sblchaoren" ,
        view_as = function(self)
                local card = sgs.Sanguosha:getCard(sgs.Self:property("sblchaoren"):toInt())
                if card then
                        if cardCanRecast(card) then --防止重铸
							return sblchaorenpreventrecast:clone() 
						else 
							card:setSkillName("sblchaoren")
							return card 
						end
                end
                return nil
        end ,
        enabled_at_play = function(self, player)
                local card = sgs.Sanguosha:getCard(player:property("sblchaoren"):toInt())
                if card and card:isAvailable(player) then
                        if cardCanRecast(card) and player:isCardLimited(card, sgs.Card_MethodUse) then return false end
                        return true
                end
                return false
        end ,
        enabled_at_response = function(self, player, pattern)
                local card = sgs.Sanguosha:getCard(player:property("sblchaoren"):toInt())
                if not card then return false end
                local realpattern = pattern:split("+")
                if player:hasFlag("Global_PreventPeach") then
                        table.removeOne(realpattern, "peach")
                end
                local handlingmethod = (sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_S_REASON_RESPONSE_USE) and sgs.Card_MethodUse or sgs.Card_MethodResponse
                --注意属性杀
				if table.contains(realpattern, "slash") then
					table.insert(realpattern,"fire_slash")
					table.insert(realpattern,"thunder_slash")
				end
				return table.contains(realpattern, card:objectName()) and not player:isCardLimited(card, handlingmethod)
        end ,
        enabled_at_nullification = function(self, player)
                local card = sgs.Sanguosha:getCard(player:property("sblchaoren"):toInt())
                if not card then return false end
                return card:isKindOf("Nullification") and not player:isCardLimited(card, sgs.Card_MethodUse)
        end ,
}
sblchaoren = sgs.CreateTriggerSkill{
        name = "sblchaoren" ,
        view_as_skill = sblchaorenvs ,
        events = {sgs.CardUsed, sgs.CardFinished, sgs.CardsMoveOneTime} ,
        can_trigger = function(self, player)
                return player
        end ,
        on_trigger = function(self, event, player, data)
                local room = player:getRoom()
                local sbl = room:findPlayerBySkillName(self:objectName())
                if (not sbl) or sbl:isDead() then return false end
                local drawpile = room:getDrawPile()
                local firstcard = -1
                if not drawpile:isEmpty() then firstcard = drawpile:first() end
                room:setPlayerProperty(sbl, "sblchaoren", sgs.QVariant(firstcard))
                if room:getTag("FirstRound"):toBool() then return false end
                if (event == sgs.CardUsed) or (event == sgs.CardFinished) then
                        local use = data:toCardUse()
                        if (use.card and use.card:isKindOf("AmazingGrace")) then
                                sbl:setFlags((event == sgs.CardUsed) and "agusing" or "-agusing")
                        end
                end
                local invoke = (event == sgs.CardUsed)
                if (event == sgs.CardsMoveOneTime) and (player and player:isAlive() and player:hasSkill(self:objectName())) then
                        local move = data:toMoveOneTime()
                        if (move.from_places:contains(sgs.Player_DrawPile) or (move.to_place == sgs.Player_DrawPile)) then
                                invoke = true
                        end
                end
                if invoke then
                        if (firstcard ~= -1) then
                                --由于最新源码更换了实现方式，所以这段代码在1102已经不可用，所以请编译最新代码配合这段LUA
                                local l = sgs.JsonValueForLUA()
                                l:setStringAt(0, "$xiangshudrawpile")
                                l:setStringAt(1, "")
                                l:setStringAt(2, "")
                                l:setStringAt(3, tostring(firstcard))
                                l:setStringAt(4, "")
                                l:setStringAt(5, "")
                                room:doNotify(sbl, sgs.CommandType.S_COMMAND_LOG_SKILL, l)

                                if not sbl:hasFlag("agusing") then
                                        local gongxinargs = sgs.JsonValueForLUA()
                                        gongxinargs:setStringAt(0, "")
                                        gongxinargs:setBoolAt(1, false)
                                        local jsondrawpile = sgs.JsonValueForLUA()
                                        jsondrawpile:setNumberAt(0, firstcard)
                                        gongxinargs:setArrayAt(2, jsondrawpile)
                                        room:doNotify(sbl, sgs.CommandType.S_COMMAND_SHOW_ALL_CARDS, gongxinargs)
                                end
                                --就是这样
                        end
                end
                return false
        end ,
}
shen016:addSkill(sblchaoren)

--【哈特曼的妖怪少女——古明地恋】编号：00017 by三国有单
shen017 = sgs.General(extension, "shen017", "touhougod", 3, false) 
biaoxiang=sgs.CreateTriggerSkill{-- 技能"表象"
	name = "biaoxiang",
	frequency = sgs.Skill_Wake,
	priority=3,
	events = {sgs.EventPhaseStart},
	can_trigger=function(self,player)
		return player:hasSkill(self:objectName()) and (player:getMark("biaoxiang")==0) and player:faceUp() and player:getPhase() == sgs.Player_Start
	end,
	on_trigger=function(self,event,player,data)--
		local room = player:getRoom()
		if ( player:getHandcardNum() < 2) then
	
			touhou_logmessage("#BiaoxiangWake",player,self:objectName())
			room:notifySkillInvoked(player, self:objectName())
			player:addMark("biaoxiang")
			x=player:getMaxHp()
			local y
			if x>3 then 
				y=3-x
			else
				y=4-x
			end

			if room:changeMaxHpForAwakenSkill(player, y) then
				room:handleAcquireDetachSkills(player,"ziwo")
			end
			
		end
	end,
}
ziwo_card = sgs.CreateSkillCard{
	name = "ziwo",
	target_fixed = true,
	will_throw = true,
	
	on_use = function(self, room, source, targets)	
		local recov = sgs.RecoverStruct()
		recov.card = self
		recov.recover = 1
		recov.who = source
		room:recover(source, recov)
	end,
	
}
ziwo = sgs.CreateViewAsSkill{--技能“自我”
	name = "ziwo",
	n = 2,
	
	enabled_at_play = function(self,player)
		return   player:isWounded()
	end,
	
	view_filter = function(self, selected, to_select)
		return (not to_select:isEquipped()) and not sgs.Self:isJilei(to_select)
	end,
	
	view_as = function(self, cards)
		if(#cards ~= 2) then return nil end
		local qcard = ziwo_card:clone()
		qcard:addSubcard(cards[1]) 
		qcard:addSubcard(cards[2])
		qcard:setSkillName(self:objectName())
		return qcard
	end
}

koishi_removeskill = function(room,player) --移除技能自定义函数
	for _,skill in  sgs.qlist(player:getVisibleSkillList()) do --skilllist
		--skill:getLocation() == sgs.Skill_Left or
		if  skill:getFrequency() == sgs.Skill_Wake  or skill:isAttachedLordSkill()  then			
			
		else
			local attach_skills = room:getTag("OtherAttachSkills"):toStringList()    
			skill_name=skill:objectName()
			local attach =false
			for _,str in pairs(attach_skills) do
				if str ==skill_name then
						attach =true
						break
					end
				end
			if attach then continue end
			room:handleAcquireDetachSkills(player, "-"..skill_name)
			--没测试关联的隐藏技能是否全部清除干净。。
			for _,relatedskill in sgs.qlist(sgs.Sanguosha:getRelatedSkills(skill_name)) do	
				if (not relatedskill:isVisible()) then
					room:detachSkillFromPlayer(damage.from, relatedskill:objectName(),true)
				end
			end
		end
	end
end
shifang = sgs.CreateTriggerSkill{--技能”释放“ 
	name = "shifang",
	events = {sgs.CardsMoveOneTime,sgs.BeforeCardsMove},
	frequency = sgs.Skill_Wake,
	priority=3,
	
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if (player:getMark("shifang")>0) then return false end
		if (not player:hasSkill("shifang")) then return false end
		move = data:toMoveOneTime()
		if event ==sgs.BeforeCardsMove then			
			  
			if move.from and move.from:objectName() ==player:objectName() and player:getCards("e"):length()==1 then
				for _,id in sgs.qlist(move.card_ids) do	--(move.from_place == sgs.Player_PlaceEquip)				
					if (room:getCardPlace(id) == sgs.Player_PlaceEquip) then
						room:setPlayerFlag(player, "shifangInvoked")
						break
					end
				end
			end
			
		else
			if move.from and (move.from_place == sgs.Player_Equip)and move.from:objectName() ==player:objectName() then
				if player:hasFlag("shifangInvoked") and player:getMark("shifang")==0 then

					touhou_logmessage("#ShifangWake",player,self:objectName())
					koishi_removeskill(room,player) 
					
					room:notifySkillInvoked(player, self:objectName())
					player:addMark("shifang")	
					x=player:getMaxHp()
					if room:changeMaxHpForAwakenSkill(player, 4-x) then
						room:handleAcquireDetachSkills(player,"benwo")
						room:setPlayerFlag(player, "-shifangInvoked")
					end
					return false
				end
			end
		end
	end
}
benwo = sgs.CreateTriggerSkill{--技能"本我"
	name = "benwo", 
	events = {sgs.DamageInflicted},  
	on_trigger = function(self, event, player, data)
		local room = player:getRoom()
		if player:isWounded() then
			local damage = data:toDamage()
			if damage.from and damage.from:isAlive() then
				_data=sgs.QVariant()
				_data:setValue(damage.from)
				player:setTag("benwo_target",_data)
				prompt="invoke:"..damage.from:objectName()..":"..tostring(x)
				x=player:getMaxHp()-player:getHp()
				if (not room:askForSkillInvoke(player,self:objectName(),_data)) then return false end
				
				player:drawCards(x)
				
				room:askForDiscard(damage.from,self:objectName(),x,x,false,true)
			end
		end
	end
}

yizhi=sgs.CreateTriggerSkill{-- 技能"抑制"--
	name = "yizhi",
	frequency = sgs.Skill_Wake,
	priority=3,
	events = {sgs.Dying},
	can_trigger=function(self,player)
		return player:hasSkill(self:objectName()) and (player:getMark("yizhi")==0)
	end,
	on_trigger=function(self,event,player,data)--
		local room = player:getRoom()
		local victim = room:getCurrentDyingPlayer()
        if victim:objectName() ~= player:objectName() then return false end
		x=1-player:getHp()
		local recov = sgs.RecoverStruct()
		recov.recover = x
		recov.who = player
		room:recover(player, recov)
		
			touhou_logmessage("#YizhiWake",player,self:objectName())
			room:notifySkillInvoked(player, self:objectName())
			x=player:getMaxHp()
			if room:changeMaxHpForAwakenSkill(player, 3-x) then
				koishi_removeskill(room,player) 
				player:addMark("yizhi")
				room:handleAcquireDetachSkills(player,"chaowo")
			end
			
			return false
		
	end,
}

chaowoCard = sgs.CreateSkillCard{
        name = "chaowo",
        target_fixed = false,
        will_throw = true,

        filter = function(self,selected,to_select)
                return #selected < 1 and to_select:getMaxHp() >= sgs.Self:getMaxHp() 
        end,

        on_use = function(self, room, source, targets)
			target=targets[1]
            target:drawCards(2)
			if target:getMaxHp() ==3 then
				source:drawCards(2)
			end
		end
}
chaowoVS = sgs.CreateViewAsSkill{
    name = "chaowo",
    n = 1,
	view_filter = function(self, selected, to_select)
		return (not to_select:isEquipped()) and not sgs.Self:isJilei(to_select)
	end,
    view_as = function (self,cards)
        if #cards==1 then   
				local card=chaowoCard:clone()
				card:addSubcard(cards[1])
				return card
		end
    end,

        enabled_at_play = function(self, player)
                return false
        end,
		enabled_at_response = function(self, player, pattern)        
			return pattern=="@@chaowo"
		end,

}
chaowo=sgs.CreateTriggerSkill{-- 技能"超我"--
	name = "chaowo",
	events = {sgs.EventPhaseStart},
	view_as_skill = chaowoVS,
	on_trigger=function(self,event,player,data)--
		if	player:getPhase() == sgs.Player_Finish then
			local room = player:getRoom()
			ai_data =sgs.QVariant()
			room:askForUseCard(player, "@@chaowo", "@chaowo")
		end
	end,
}

if not sgs.Sanguosha:getSkill("ziwo") then
        local skillList=sgs.SkillList()
        skillList:append(ziwo)
        sgs.Sanguosha:addSkills(skillList)
end
shen017:addSkill(biaoxiang)	
if not sgs.Sanguosha:getSkill("benwo") then
        local skillList=sgs.SkillList()
        skillList:append(benwo)
        sgs.Sanguosha:addSkills(skillList)
end
shen017:addSkill(shifang)	
if not sgs.Sanguosha:getSkill("chaowo") then
        local skillList=sgs.SkillList()
        skillList:append(chaowo)
        sgs.Sanguosha:addSkills(skillList)
end
shen017:addSkill(yizhi)	
shen017:addRelateSkill("ziwo")
shen017:addRelateSkill("benwo")
shen017:addRelateSkill("chaowo")

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

--touhou_kingdom()
touhou_tianyi()

--touhou_extraTurn()