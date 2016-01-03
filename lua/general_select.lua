
initialize = function()
	--sgs.Sanguosha:currentRoom():getPlayers():first():speak("generalselector initialize")
end

selectFirst = function(player, candidates) -- string
	player:speak("selectfirst")
	return candidates[math.random(#candidates)]
end

selectSecond = function(player, candidates) -- string
	player:speak("selectSecond")
	return candidates[math.random(#candidates)]
end

select1v1 = function(candidates) -- string
	player:speak("select1v1")
	return candidates[math.random(#candidates)]
end

select3v3 = function(player, candidates) -- string
	player:speak("select3v3")
	return candidates[math.random(#candidates)]
end

arrange3v3 = function(player) -- stringlist
	player:speak("arrange3v3")
	return {}
end

arrange1v1 = function(player) -- stringlist
	player:speak("arrange1v1")
	return {}
end
