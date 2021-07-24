-- utilities, i.e: convert QList<const Card> to Lua's native table
function sgs.QList2Table(qlist)
	local t = {}
	for i = 0, qlist:length() - 1 do
		table.insert(t, qlist:at(i))
	end

	return t
end

-- the iterator of QList object
local qlist_iterator = function(list, n)
	if n < list:length() - 1 then
		return n + 1, list:at(n + 1) -- the next element of list
	end
end

function sgs.qlist(list)
	return qlist_iterator, list, -1
end

-- more general iterator
function sgs.list(list)
	if type(list) == "table" then
		return ipairs(list)
	else
		return sgs.qlist(list)
	end
end

function sgs.reverse(list)
	local new = {}
	for i = #list, 1, -1 do
		table.insert(new, list[i])
	end
	return new
end

-- copied from "Well House Consultants"
-- used to split string into a table, similar with php' explode function
function string:split(delimiter)
	local result = {}
	local from = 1
	local delim_from, delim_to = string.find(self, delimiter, from)
	while delim_from do
		table.insert(result, string.sub(self, from, delim_from - 1))
		from  = delim_to + 1
		delim_from, delim_to = string.find(self, delimiter, from)
	end
	table.insert(result, string.sub(self, from))
	return result
end

function table:contains(element)
	if #self == 0 or type(self[1]) ~= type(element) then return false end
	for _, e in ipairs(self) do
		if e == element then return true end
	end
end

function table:removeOne(element)
	if #self == 0 or type(self[1]) ~= type(element) then return false end

	for i = 1, #self do
		if self[i] == element then
			table.remove(self, i)
			return true
		end
	end
	return false
end

function table:removeAll(element)
	if #self == 0 or type(self[1]) ~= type(element) then return 0 end
	local n = 0
	for i = 1, #self do
		if self[i] == element then
			table.remove(self, i)
			n = n + 1
		end
	end
	return n
end

function table:insertTable(list)
	for _, e in ipairs(list) do
		table.insert(self, e)
	end
end

function table:removeTable(list)
	for _, e in ipairs(list) do
		table.removeAll(self, e)
	end
end

function table.copyFrom(list)
	local l = {}
	for _, e in ipairs(list) do
		table.insert(l, e)
	end
	return l
end

function table:indexOf(value, from)
	from = from or 1
	for i = from, #self do
		if self[i] == value then return i end
	end
	return -1
end

function table:shuffle()
	for i = 1, #self, 1 do
		local r = math.random(0, #self - i + 1) + i
		self[i], self[r] = self[r], self[i]
	end
end

function table:keys()
	local new = {}
	for k in pairs(self) do
		table.insert(new, k)
	end

	return new
end

function string:matchOne(option)
	return self:match("^" .. option .. "%p") or self:match("%p" .. option .. "%p") or self:match("%p" .. option .. "$")
end

function string:startsWith(substr)
	local len = string.len(substr)
	if len == 0 or len > string.len(self) then return false end
	return string.sub(self, 1, len) == substr
end

function string:endsWith(substr)
	local len = string.len(substr)
	if len == 0 or len > string.len(self) then return false end
	return string.sub(self, -len, -1) == substr
end

function math:mod(num)
	return math.fmod(self, num)
end
