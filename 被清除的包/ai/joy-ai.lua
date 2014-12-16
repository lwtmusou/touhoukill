--根据V1的基础上
function SmartAI:useCardShit(card, use)
	if (card:getSuit() == sgs.Card_Heart or card:getSuit() == sgs.Card_Club) and self.player:isChained() and
		#(self:getChainedFriends()) > #(self:getChainedEnemies()) then return end
	if self.player:getHp()>3 and (self.player:hasSkills("shenfen+kuangbao") or self:getDamagedEffects()) then use.card = card return end
	if self.player:hasSkill("kuanggu") and card:getSuitString() ~= "spade" then use.card = card return end
	if card:getSuit() == sgs.Card_Heart and (self.player:hasArmorEffect("GaleShell") or self.player:hasArmorEffect("Vine")) then return end
	if not self.player:isWounded() then
		if self:hasSkills(sgs.need_kongcheng) and self.player:getHandcardNum() == 1 then use.card = card return end
		for _, askill in sgs.qlist(self.player:getVisibleSkillList()) do
			if sgs[askill:objectName() .. "_suit_value"] then
				if (sgs[askill:objectName() .. "_suit_value"][card:getSuitString()] or 0) > 0 then return end
			end
		end
		local peach = self:getCard("Peach")
		if peach then
			self:sort(self.friends, "hp")
			if not self:isWeak(self.friends[1]) then
				use.card = card
				return
			end
		end
	end
end

sgs.ai_use_value.Shit = -10
sgs.ai_keep_value.Shit = 6

sgs.ai_skill_invoke.grab_peach = function(self, data)
	local struct = data:toCardUse()
	return self:isEnemy(struct.from) and (struct.to:isEmpty() or self:isEnemy(struct.to:first()))
end

function SmartAI:useCardGaleShell(card, use)
	use.broken = true
	for _, enemy in ipairs(self.enemies) do
		if self.player:distanceTo(enemy) <=1 and not self:hasSkills("jijiu|wusheng|longhun", enemy) then
			use.card = card
			if use.to then
				use.to:append(enemy)
			end
			return
		end
	end
	for _, friend in ipairs(self.friends) do
		if self.player:distanceTo(friend) <= 1 and self:hasSkills("jijiu|longhun", friend) then
			use.card = card
			if use.to then
				use.to:append(friend)
			end
			return
		end
	end
end

function sgs.ai_armor_value.GaleShell()
	return -10
end

sgs.ai_card_intention.GaleShell = 80
sgs.ai_use_priority.GaleShell = 0.9
sgs.dynamic_value.control_card.GaleShell = true

sgs.weapon_range.YxSword = 3

sgs.ai_skill_playerchosen.YxSword = function(self, targets)
	local damage = self.room:getTag("YxSwordData"):toDamage()
	local dmg = damage.damage
	local who = damage.to
	if not who then return end
	if (who:hasArmorEffect("Vine") or who:hasArmorEffect("GaleShell")) and damage.nature == sgs.DamageStruct_Fire then dmg = dmg + 1 end
	if who:hasArmorEffect("SilverLion") then dmg = 1 end
	local invoke = "no"
	for _, askill in sgs.qlist(who:getVisibleSkillList()) do
		local name = askill:objectName()
		if string.find(name, "ganglie") or string.find(name, "fankui") or string.find(name, "enyuan") then
			invoke = "yes"
			break
		end
	end
	if who and ((who:hasSkill("duanchang") and who:getHp() - dmg < 1) or invoke == "yes") then
		if sgs.evaluatePlayerRole(who) == "rebel" then
			for _, player in sgs.qlist(targets) do
				if self:isFriend(player) then
					return player
				end
			end
		elseif sgs.evaluatePlayerRole(who) == "loyalist" then
			if self:isEnemy(who) then return self.room:getLord() end
		end
		return self.enemies[1]
	end
	return nil
end
sgs.ai_playerchosen_intention.YxSword = -10

