
-- Initialization script for QSanguosha
-- Assuming standard module (w/o coroutine) and module "sgs" imported

-- compatible with Lua 5.3
if _VERSION == "Lua 5.3" then
    fail = nil
end

-- first, kill some function that shouldn't be used
-- mainly process and IO related functions
-- note that I is acceptable while O isn't, with exception of writing tempoaray files.
os.exit = function()
    error("QSanguosha shouldn't exit due to call to os.exit for preventing malicious use.")
end
os.execute = function(n, ...)
    warn("QSanguosha won't support running an exectuable for preventing malicious use.")
    if not n then return fail end
    return fail, "signal", 9
end
os.remove = function(...)
    local failMsg = "QSanguosha won't support removing files for preventing malicious use."
    warn(failMsg)
    return fail, failMsg
end
os.rename = function(...)
    local failMsg = "QSanguosha won't support renaming files for preventing malicious use."
    warn(failMsg)
    return fail, failMsg
end
local originalOpen = io.open
io.open = function(filename, mode)
    local _mode = mode or "r"
    if (_mode == "r") or (_mode == "rb") then
        return originalOpen(filename, _mode)
    end
    local failMsg = "QSanguosha won't support creating/writing regular files for preventing malicious use. Use io.tmpfile if you really want to write to temporary file."
    warn(failMsg)
    return fail, failMsg
end
io.popen = function()
    error("QSanguosha won't support running an exectuable for preventing malicious use.")
end

-- To make a trusted runtime environment, only the above initialization seems not enough
-- e.g. dofile or require are not safe if it loads maliciously crafted bytecode.
-- we can't control how lua bytecode works since we are using Lua from distribution package managers if possible

-- dofile with qrc support
local originalDofile = dofile
dofile = function(filename)
    if not filename then
        error("QSanguosha does not support cin.")
    end

    if string.sub(filename, 1, 4) == "qrc:" then
        -- TODO: load contents from QRC
        local resourceContent = sgs.qrc:contents(filename)
        if filename == "qrc:/sgs_ex.lua" then
            print(type(resourceContent))
        end
        if resourceContent then
            local func, err = load(resourceContent, filename, "t")
            if func then
                return func()
            else
                error(err)
            end
        end
    end

    return originalDofile(filename)
end

-- loadfile with qrc support
local originalLoadfile = loadfile
loadfile = function(filename, ...)
    if not filename then
        return nil, "QSanguosha does not support cin."
    end

    if string.sub(filename, 1, 4) == "qrc:" then
        -- TODO: load contents from QRC
        local resourceContent = sgs.qrc:contents(filename)
        if resourceContent then
            return load(resourceContent, filename, ...)
        end
    end

    return originalLoadfile(filename, ...)
end

local QrcLoadFunction = function(name, fileName)
    print(name, fileName)
    return dofile(fileName)
end
local QrcSearchFunction = function(name)
    local name_ = string.gsub(name, "%.", "/")
    local fileName = "qrc:/" .. name_ .. ".lua"
    if sgs.qrc:contains(fileName) then
        print(name, fileName)
        return QrcLoadFunction, fileName
    end
    return nil
end
table.insert(package.searchers, 1, QrcSearchFunction)

-- how to implement require with qrc support?
-- we can't simply append 'qrc:/' to 'package.path'
-- just use following:
--   x = dofile("qrc://x.lua")
--   package.loaded.x = x

-- original Sanguosha.lua does following things
-- 1. load utilities
--    It should be done in this script
-- 2. load translation
--    It is currently replaced by Json
-- 3. load extensions
--    This is what we need to refactor
--    do not create new instance of skills / card faces / generals in Lua

-- in addition, we may do following things
-- a. check version of builtin extensions

-- 1. load utilities
dofile("qrc:/utilities.lua")

-- 3. load extensions
sgs_ex = require("sgs_ex")
tl = require("tl")
do 
    local report_all_errors
    do
        local function report_errors(category, errors)
            if not errors then
                return false
            end
            if #errors > 0 then
                local n = #errors
                print("========================================")
                print(n .. " " .. category .. (n ~= 1 and "s" or "") .. ":")
                for _, err in ipairs(errors) do
                    print(err.filename .. ":" .. err.y .. ":" .. err.x .. ": " .. (err.msg or ""))
                end
                return true
            end
            return false
        end
    
       report_all_errors = function(env, syntax_only)
          local any_syntax_err, any_type_err, any_warning
          for _, name in ipairs(env.loaded_order) do
             local result = env.loaded[name]
    
             local syntax_err = report_errors("syntax error", result.syntax_errors)
             if syntax_err then
                any_syntax_err = true
             elseif not syntax_only then
                any_warning = report_errors("warning", result.warnings) or any_warning
                any_type_err = report_errors("error", result.type_errors) or any_type_err
             end
          end
          local ok = not (any_syntax_err or any_type_err)
          return ok, any_syntax_err, any_type_err, any_warning
       end
    end
    
    local function type_check_and_load(module_name)
        local found_filename, fd, tried = tl.search_module(module_name, false)

        if not found_filename then 
            table.concat(tried, "\n\t")
        end

        fd:close()

        if not tl.package_loader_env then
            tl.package_loader_env = tl.init_env(found_filename:match("lua$"))
        end

        local result, err = tl.process(found_filename, tl.package_loader_env)
        if err then
            die(err)
        end
    
       local is_tl = found_filename:match("%.tl$")
       local _, syntax_err, type_err = report_all_errors(tl.package_loader_env, not is_tl)
    
       local chunk; 
       chunk, err = (loadstring or load)(tl.pretty_print_ast(result.ast), "@" .. found_filename)
       if chunk then
         return function()
            local ret = chunk()
            package.loaded[module_name] = ret
            return ret
         end
      else
         error("Internal Compiler Error: Teal generator produced invalid Lua. Please report a bug at https://github.com/teal-language/tl\n\n" .. err)
      end
    end

    -- append the loader into the environment.
    if package.searchers then
        table.insert(package.searchers, 2, type_check_and_load)
    else
        table.insert(package.loaders, 2, type_check_and_load)
    end
end


-- Check the type. 

dofile("lua/testtest.lua")

-- Descriptors here
-- Should it be k-v pair or sequence?

sgs.Packages = {}
sgs.CardFaces = {}
sgs.Skills = {}

-- TODO: load extensions

local loadExtension = function(name, isBuiltin)
    -- checksum check is done before this function is called
    -- each extension is a Lua table which has a key named 'type' and a value of 'sgs_ex.TableType.Package'
    -- should we require or dofile the table?
    local extensionPrefix = "extensions."
    if isBuiltin then
        extensionPrefix = "builtinExtension."
    end

    local extension = require(extensionPrefix .. name)
    if extension then
        if (type(extension) == "table") and ((extension.type & sgs_ex.TableType.FirstTypeMask) == sgs_ex.TableType.Package) then
            table.insert(sgs.Packages, extension)
            if extension.cardFaces then
                for _, c in iparis(extension.cardFaces) do
                    table.insert(sgs.CardFaces, c)
                end
            end
            if extension.skills then
                for _, s in ipairs(extension.skills) do
                    table.insert(sgs.Skills, s)
                end
            end
            table.insert(sgs.Packages, extension)
            return true
        end
    end

    return false
end

local loadBuiltinExtensions = function()
    -- TODO: builtin extensions might be accessed via Lua file with checksum check. Disable connection to server if checksum mismatches
    -- All packages may be loaded via Lua with checksum, with a few builtin CardFaces and Skills available in CPP
    -- A builtin extension updater is needed in CPP to provide the needed checksum algorithm
end

local loadInstalledExtensions = function()
    -- TODO: allow to load Lua file with checksum check, warn about it (instead of discard) if checksum mismatches
    -- It is possible to provide an extension shop, which downloads and validates extension by using checksum
end
