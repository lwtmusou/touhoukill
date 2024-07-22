
sgs.ai_skill_discard.cadan = function(self)
	if self.player:getCards("j"):length() >= 2 then
		return self:askForDiscard("dummy", 2, 2, false, true)
	end
	return {}
end
