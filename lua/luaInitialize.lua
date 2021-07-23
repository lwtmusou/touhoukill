
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
    return dofile(fileName)
end
local QrcSearchFunction = function(name)
    local name_ = string.gsub(name, "%.", "/")
    local fileName = "qrc:/" .. name_ .. ".lua"
    if sgs.qrc:contains(fileName) then
        return QrcLoadFunction, fileName
    end
    return nil
end
table.insert(package.searchers, 1, QrcSearchFunction)

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
-- b. check configuration of how we load extensions

-- 1. load utilities
dofile("qrc:/utilities.lua")

-- 3. load extensions
sgs_ex = require("sgs_ex")

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
    local extensionPrefix = "extension."
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
    -- TODO: builtin extensions might be accessed via Luac file with checksum check. Disable connection to server if checksum mismatches
    -- All packages may be loaded via Lua, with a few builtin CardFaces and Skills available in CPP
    -- A builtin extension updater is needed in CPP to provide the needed checksum algorithm, since checksum is calculated in CPP

    -- First, we should get names of all builtin extensions from CPP.
    -- There was 'config.lua' who did this thing by just listing names of extensions in a table, but it isn't now.
    -- We should find a place for putting the configurations somewhere.
    local checksumFailed = false
    local loadFailed = false

    local builtinExtensionNames = sgs.BuiltExtension_names()
    for _, name in ipairs(builtinExtensionNames) do
        if not sgs.BuiltinExtension_verifyChecksum(name) then
            warn("Checksum of " .. name .. " mismatches")
            checksumFailed = true
        end

        if not loadExtension(name, true) then
            warn("Builtin extension " .. name .. " has failed to load")
            loadFailed = true
        end
    end

    if checksumFailed or loadFailed then
        sgs.BuiltinExtension_disableConnectToServer()
    end
end

local loadInstalledExtensions = function()
    -- TODO: allow to load Lua file with checksum check, warn about it (instead of discard) if checksum mismatches
    -- It is possible to provide an extension shop, which downloads and validates extension by using checksum

    -- load extension names from config file?
end
