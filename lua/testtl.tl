local sgs = require("sgs")

local tl = {
	testFromTl = function() : string
		sgs.information(nil, "tl", "hello world from tl")
		return "hello world"
	end
}

tl.xx = 2

return tl

