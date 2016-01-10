
local first_general_table = {}
local second_general_table = {}
local general_seat_table = {}
local priority_3v3_table = {}
local priority_1v1_table = {}
local sacrifice = {}

local loadFirstGeneralTableRole = function(role)
	local roleFile = io.open("etc/" .. role .. ".txt")
	local str = roleFile:read()
	while str do
		local captures = {string.match(str, "([%u%l%d_]+)%s+([%u%l%d_]+)%s+([%d%.]+)")}
		if #captures ~= 3 then continue end
		local key = captures[1] .. ":" .. captures[2] .. ":" .. role
		first_general_table[key] = tonumber(captures[3])
		str = roleFile:read()
	end
	roleFile:close()
end

local loadGeneralSeatTable = function()
	local seatFile = io.open("etc/seat.txt")
	local str = seatFile:read()
	while str do
		local captures = {string.match(str, "([%u%l%d_]+)%s+([%u%l%d_]+)%s+([%u%l%d_]+)%s+([%u%l%d_]+)%s+([%u%l%d_]+)%s+")}
		if #captures ~= 5 then continue end
		local key1 = captures[1] .. ":" .. captures[2] .. ":loyalist"
		general_seat_table[key1] = captures[3]
		local key2 = captures[1] .. ":" .. captures[2] .. ":rebel"
		general_seat_table[key2] = captures[4]
		local key3 = captures[1] .. ":" .. captures[2] .. ":renegade"
		general_seat_table[key3] = captures[5]
		str = seatFile:read()
	end
	seatFile:close()
end

local loadFirstGeneralTable = function()
	loadFirstGeneralTableRole("loyalist")
	loadFirstGeneralTableRole("rebel")
	loadFirstGeneralTableRole("renegade")
	loadGeneralSeatTable()
end

local loadSecondGeneralTable = function()
	local secondGenFile = io.open("etc/double-generals.txt")
	local str = secondGenFile:read()
	while str do
		local captures = {string.match(str, "([%u%l%d_]+)%s+([%u%l%d_]+)%s+(%d+)")}
		if #captures ~= 3 then continue end
		local key = captures[1] .. "+" .. captures[2]
		second_general_table[key] = tonumber(captures[3])
		str = secondGenFile:read()
	end
	secondGenFile:close()
end

local load3v3Table = function()
	local secondGenFile = io.open("etc/3v3-priority.txt")
	local str = secondGenFile:read()
	while str do
		local captures = {string.match(str, "([%u%l%d_]+)%s+(%d+)")}
		if #captures ~= 2 then continue end
		priority_3v3_table[captures[1]] = tonumber(captures[2])
		str = secondGenFile:read()
	end
	secondGenFile:close()
end

local load1v1Table = function()
	local secondGenFile = io.open("etc/1v1-priority.txt")
	local str = secondGenFile:read()
	while str do
		local captures = {string.match(str, "([%u%l%d_]+)%s+(%d+)%s*(%*?)")}
		if #captures ~= 3 then continue end
		priority_1v1_table[captures[1]] = tonumber(captures[2])
		if string.len(capture[3]) ~= 0 then
			table.insert(sacrifice, captures[1])
		end
		str = secondGenFile:read()
	end
	secondGenFile:close()
end

local selectHighest = function(tabl, candidates, default_value)
	local maxx = -1
	local max_general
	for _, candidate in ipairs(candidates) do
		local value = tabl[candidate]
		if not value then value = default_value end
		if value > maxx then
			maxx = value
			max_general = candidate
		end
	end
	
	assert(max_general)
	return max_general
end

local get1v1ArrangeValue = function(name)
	local value = priority_1v1_table[name]
	if not value then value = 5 end
	if table.contains(sacrifice, name) then
		value = value + 1000
	end
	return value
end

initialize = function()
	loadFirstGeneralTable()
	loadSecondGeneralTable()
	load1v1Table()
	load3v3Table()
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
