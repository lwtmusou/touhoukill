-- This is the start script of QSanguosha

package.path = package.path .. ";./lua/lib/?.lua"

dofile "lua/utilities.lua"

function load_translation(file)
	local t = dofile(file)
	if type(t) ~= "table" then
		error(("file %s is should return a table!"):format(file))
	end

	sgs.LoadTranslationTable(t)
end

function load_translations()
	local lang = sgs.GetConfig("Language", "zh_CN")
	local subdir = { "", "Audio", "Package" }
	for _, dir in ipairs(subdir) do
		local lang_dir = "lang/" .. lang .. "/" .. dir
		local lang_files = sgs.GetFileNames(lang_dir)
		for _, file in ipairs(lang_files) do
			load_translation(("%s/%s"):format(lang_dir, file))
		end
	end
end

local done_loading = sgs.Sanguosha:property("DoneLoading"):toBool()
if not done_loading then
	load_translations()
	done_loading = sgs.QVariant(true)
	sgs.Sanguosha:setProperty("DoneLoading", done_loading)
end
