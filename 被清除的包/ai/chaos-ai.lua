sgs.ai_skill_use["@@jisu"] = function(self, prompt)
	local card_str = sgs.ai_skill_use["@@shensu-card1"](self, "@shensu1")
	return string.gsub(card_str, "ShensuCard", "JisuCard")
end

sgs.ai_card_intention.JisuCard = sgs.ai_card_intention.Slash

sgs.ai_slash_prohibit.shuiyong = function(self, to, card, from)
	if from:hasSkill("jueqing") then return false end
	if from:hasFlag("nosjiefanUsed") then return false end
	if from:hasSkill("nosqianxi") and from:distanceTo(to) == 1 then return false end
	return card:isKindOf("FireSlash") and to:hasSkill("shuiyong")
end


--simple Han Ba ai by Fsu0413
sgs.ai_skill_playerchosen.fentian = function(self, targets)
	local targetstable = {}
	for _, p in sgs.qlist(targets) do
		if self:isEnemy(p) then
			table.insert(targetstable, p)
		end
	end
	self:sort(targetstable)
	return targetstable[1]
end

local xintan_skill = {}
xintan_skill.name = "xintan"
table.insert(sgs.ai_skills, xintan_skill)
xintan_skill.getTurnUseCard = function(self)
	if (self.player:getPile("burn"):length() > math.max(self.room:getAlivePlayers():length() / 2, 2)) and not self.player:hasUsed("XintanCard") then
		return sgs.Card_Parse("@XintanCard=.")
	end
	return nil
end

sgs.ai_skill_use_func.XintanCard = function(card, use, self)
	self:sort(self.enemies, "hp")
	use.card = sgs.Card_Parse("@XintanCard=.")
	if use.to then use.to:append(self.enemies[1]) end
end
