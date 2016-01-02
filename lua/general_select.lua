
initialize = function()
	sgs.Sanguosha:currentRoom():getPlayers():first():speak("generalselector initialize")
end,

selectFirst = function(player, candidates) -- string
	player:speak("selectfirst")
	return nil
end,

selectSecond = function(player, candidates) -- string
	player:speak("selectSecond")
	return nil
end,

select1v1 = function(candidates) -- string
	player:speak("select1v1")
	return nil
end,

select3v3 = function(player, candidates) -- string
	player:speak("select3v3")
	return nil
end,

arrange3v3 = function(player) -- stringlist
	player:speak("arrange3v3")
	return {}
end,

arrange1v1 = function(player) -- stringlist
	player:speak("arrange1v1")
	return {}
end,

get1v1ArrangeValue = function(name) -- int
	player:speak("get1v1ArrangeValue")
	return 0
end,