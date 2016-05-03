

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
		if #captures == 3 then
			local key = captures[1] .. ":" .. captures[2] .. ":" .. role
			first_general_table[key] = tonumber(captures[3])
		end
		str = roleFile:read()
	end
	roleFile:close()
end

local loadGeneralSeatTable = function()
	local seatFile = io.open("etc/seat.txt")
	local str = seatFile:read()
	while str do
		local captures = {string.match(str, "([%u%l%d_]+)%s+([%u%l%d_]+)%s+([%u%l%d_]+)%s+([%u%l%d_]+)%s+([%u%l%d_]+)%s+")}
		if #captures == 5 then
			local key1 = captures[1] .. ":" .. captures[2] .. ":loyalist"
			general_seat_table[key1] = captures[3]
			local key2 = captures[1] .. ":" .. captures[2] .. ":rebel"
			general_seat_table[key2] = captures[4]
			local key3 = captures[1] .. ":" .. captures[2] .. ":renegade"
			general_seat_table[key3] = captures[5]
		end
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
		if #captures == 3 then
			local key = captures[1] .. "+" .. captures[2]
			second_general_table[key] = tonumber(captures[3])
		end
		str = secondGenFile:read()
	end
	secondGenFile:close()
end

local load3v3Table = function()
	local secondGenFile = io.open("etc/3v3-priority.txt")
	local str = secondGenFile:read()
	while str do
		local captures = {string.match(str, "([%u%l%d_]+)%s+(%d+)")}
		if #captures == 2 then
			priority_3v3_table[captures[1]] = tonumber(captures[2])
		end
		str = secondGenFile:read()
	end
	secondGenFile:close()
end

local load1v1Table = function()
	local secondGenFile = io.open("etc/1v1-priority.txt")
	local str = secondGenFile:read()
	while str do
		local captures = {string.match(str, "([%u%l%d_]+)%s+(%d+)%s*(%*?)")}
		if #captures == 3 then
			priority_1v1_table[captures[1]] = tonumber(captures[2])
			if string.len(captures[3]) ~= 0 then
				table.insert(sacrifice, captures[1])
			end
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

local selectHighest = function(tabl, candidates, default_value)
	local maxx = -1
	local max_general
	for _, candidate in ipairs(candidates) do
		local value = tabl[candidate] or default_value
		if (value > maxx) then
			maxx = value
			max_general = candidate
		end
	end

	assert(max_general)
	return max_general
end

initialize = function()
	loadFirstGeneralTable()
	loadSecondGeneralTable()
	load1v1Table()
	load3v3Table()
end

--version 1: read table from etc folder
selectFirst = function(player, candidates) -- string
	local values = {}
	local role = player:getRole()
	local lord = player:getRoom():getLord()

	local seat_place
	if player:getSeat() - 1 <= 2 then
		seat_place = "1"
	elseif player:getRoom():alivePlayerCount() - player:getSeat() <= 2 then
		seat_place = "3"
	else
		seat_place = "2"
	end

	for _, candidate in ipairs(candidates) do
		local value = 5.0
		local general = sgs.Sanguosha:getGeneral(candidate)
		if role == "loyalist" and (general:getKingdom() == lord:getKingdom() or general:getKingdom() == "zhu" or general:getKingdom() == "touhougod") then
			value = value * 1.04
		end
		local key = "_:" .. candidate .. ":" .. role
		value = value * math.pow(1.1, first_general_table[key] or 0.0)
		local key2 = lord:getGeneralName() .. ":" .. candidate .. ":" .. role
		value = value * math.pow(1.1, first_general_table[key2] or 0.0)

		--considering seat
		local seat_key = "_:" .. candidate .. ":" .. role
		local seat_str = general_seat_table[seat_key] or "_"
		if (seat_str ~= "_") then
			local seat_strs = string.split(seat_str, ":")
			if (table.contains(seat_strs, seat_place .. "0")) then
				value = value * 1.2
			elseif (table.contains(seat_strs, seat_place .. "1")) then
				value = value * 0.8
			end
		end
		local seat_key2 = lord:getGeneralName() .. ":" .. candidate .. ":" .. role
		local seat_str2 = general_seat_table[seat_key2] or "_"
		if (seat_str2 ~= "_") then
			local seat_strs2 = string.split(seat_str2, ":")
			if (table.contains(seat_strs2, seat_place .. "0")) then
				value = value * 1.2
			elseif (table.contains(seat_strs2, seat_place .. "1")) then
				value = value * 0.8
			end
		end

		values[candidate] = value
	end

	local _candidates = {}
	for _, c in ipairs(candidates) do
		table.insert(_candidates, c)
	end
	local choice_list = {}
	while (#_candidates > 0 and #choice_list < 6) do
		local maxx = -1
		local choice
		for _, candidate in ipairs(_candidates) do
			local value = values[candidate] and values[candidate] or 5.0
			if (value > maxx) then
				maxx = value
				choice = candidate
			end
			table.insert(choice_list, choice)
			table.removeOne(_candidates, choice)
		end
	end
	local max_general
	math.randomseed(os.clock())
	local rnd = math.random(0, 99)
	local total = #choice_list
	local prob = {70, 85, 92, 95, 97, 99}
	for i = 1, 6, 1 do
		if (rnd <= prob[i] or total <= i) then
			max_general = string.split(choice_list[i], ":")[1]
			break
		end
	end

	assert(max_general)
	return max_general
end

--todo: lwtmusou  to bulid a table
--return weight of relation
weightRelation =  function(relation, role)
	if relation == "anti" then
		if role == "loyalist" then
			return 0.8
		elseif  role == "rebel" then
			return 1.1
		end
	end
	return 1.0
end
--todo: lwtmusou  to bulid a new method
--decide relation between nodes of skill property
relationOfSkillProperty =  function(node1, node2, player1, player2, propertyType)
	for _, str1 in pairs(node1) do
		for _, str2 in pairs(node2) do
			if propertyType == "effect" then
				if str1 == "Anti" .. str2 or str2 == "Anti" .. str1 then
					return "anti"
				end
			end
			if propertyType == "trigger" then --like"NotActive"  "Current"
				if str1 ==  str2  then
					return "related"
				end
			end
			if propertyType == "target" then -- "SkillOwner"  "AlivePlayers"  "OtherAlivePlayers"
				if str1 ==  str2 and str1 == "SkillOwner" then
					if player1:objectName() == player2:objectName() then
						return "related"
					end
				elseif str1 == "AlivePlayers" or str2 == "AlivePlayers" then
					return "related"
				elseif str1 == "OtherAlivePlayers" or str2 == "OtherAlivePlayers" then
					if player1:objectName() ~= player2:objectName() then
						return "related"
					end
				end
			end
		end
	end
	return "unrelated"
end

weightSkillProperty =  function(player, lord, general)
	local value = 1.0
	--notice that skillsList in player or lord is empty at now
	local lordGeneral = sgs.Sanguosha:getGeneral(lord:getGeneralName())

	for _, skill in sgs.qlist(general:getVisibleSkillList()) do
		local s_name = skill:objectName()
		local pro = sgs.ai_skill_property[s_name]
		if not pro or  type(pro) ~= "table" then continue end

		for _, l_skill in sgs.qlist(lordGeneral:getVisibleSkillList()) do
			local ls_name = l_skill:objectName()
			local l_pro = sgs.ai_skill_property[ls_name]
			if not l_pro or type(l_pro) ~= "table" then continue end

			for i, effect1 in pairs(pro["effect"]) do
				for k, effect2 in pairs(l_pro["effect"]) do
					local relation  =  relationOfSkillProperty(effect1, effect2, player, lord, "effect")
					local weight =  weightRelation(relation, player:getRole())
					if weight ~= 1.0 then-- if find specail relationship, check whether two effect nodes in same condition(target, trigger)
						local target1 = pro["target"][i] or pro["target"][1]
						local target2 = l_pro["target"][k] or l_pro["target"][1]
						local relation1  =  relationOfSkillProperty(target1, target2, player, lord, "target")
						if relation1   ~= "related" then continue end

						local trigger1 = pro["trigger"][i] or pro["trigger"][1]
						local trigger2 = l_pro["trigger"][k] or l_pro["trigger"][1]
						local relation2  =  relationOfSkillProperty(trigger1, trigger2, player, lord, "trigger")
						if relation2   ~= "related" then continue end

						value = value *  weight
					end
				end
			end
		end
	end
	return value
end
--version 2: test version parsing skill property
--[[selectFirst = function(player, candidates) -- string
	local values = {}
	local role = player:getRole()
	local lord = player:getRoom():getLord()


	for _, candidate in ipairs(candidates) do
		local value = 5.0
		local general = sgs.Sanguosha:getGeneral(candidate)
		if role == "loyalist" and (general:getKingdom() == lord:getKingdom() or general:getKingdom() == "zhu" or general:getKingdom() == "touhougod") then
			value = value * 1.04
		end

		value = value *  weightSkillProperty(player, lord, general)

		values[candidate] = value
	end


	local _candidates = {}
	for _, c in ipairs(candidates) do
		table.insert(_candidates, c)
	end
	local choice_list = {}
	while (#_candidates > 0 and #choice_list < 6) do
		local maxx = -1
		local choice
		for _, candidate in ipairs(_candidates) do
			local value = values[candidate] and values[candidate] or 5.0
			if (value > maxx) then
				maxx = value
				choice = candidate
			end
			table.insert(choice_list, choice)
			table.removeOne(_candidates, choice)
		end
	end
	local max_general
	math.randomseed(os.clock())
	local rnd = math.random(0, 99)
	local total = #choice_list
	local prob = {70, 85, 92, 95, 97, 99}
	for i = 1, 6, 1 do
		if (rnd <= prob[i] or total <= i) then
			max_general = string.split(choice_list[i], ":")[1]
			break
		end
	end

	assert(max_general)
	return max_general
end
]]


selectSecond = function(player, candidates) -- string
	local first = player:getGeneralName()
	local maxx = -1000
	local max_general
	for _, candidate in ipairs(candidates) do
		local key = first .. "+" .. candidate
		local value = second_general_table[key] or 0
		if (value == 0) then
			key = candidate .. "+" .. first
			value = second_general_table[key] or 3
		end

		if value > maxx then
			maxx = value
			max_general = candidate
		end
	end

	assert(max_general)
	return max_general
end

select1v1 = function(candidates) -- string
	return selectHighest(priority_1v1_table, candidates, 5)
end

select3v3 = function(player, candidates) -- string
	return selectHighest(priority_3v3_table, candidates, 5)
end

arrange3v3 = function(player) -- stringlist
	local arranged = player:getSelected()
	table.shuffle(arranged)
	local arranged4 = {table.unpack(arranged, 1, 4)}
	local compareByMaxHp = function(a, b)
		local g1 = sgs.Sanguosha:getGeneral(a)
		local g2 = sgs.Sanguosha:getGeneral(b)
		return g1:getMaxHp() < g2:getMaxHp()
	end
	table.sort(arranged4, compareByMaxHp)
	arranged4[1], arranged4[2] = arranged4[2], arranged4[1]
	return arranged4
end

arrange1v1 = function(player) -- stringlist
	local arranged = player:getSelected()
	local compareFunction = function(a, b)
		return get1v1ArrangeValue(a) > get1v1ArrangeValue(b)
	end
	table.sort(arranged, compareFunction)
	local result = {}
	local i
	for i = 1, 3, 1 do
		if (get1v1ArrangeValue(arranged[i]) > 1000) then
			table.insert(result, arranged[i])
			break
		end
	end
	if #result > 0 then
		local strong = (i == 1) and 2 or 1
		local weak = (i == 3) and 2 or 3
		table.insert(result, arranged[weak])
		table.insert(result, arranged[strong])
	else
		table.insert(result, arranged[2])
		table.insert(result, arranged[3])
		table.insert(result, arranged[1])
	end
	return result
end
