
-- Initialization script for QSanguosha
-- Assuming standard module (w/o coroutine) and module "sgs" imported and is accessable globally

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
-- TODO: handle dofile and require to disable LUAC loading

-- dofile with qrc support
local originalDofile = dofile
dofile = function(filename)
    if not filename then
        error("QSanguosha does not support cin.")
    end

    if string.sub(filename, 1, 4) == "qrc:" then
        local resourceContent = sgs.qrc_contents(filename)
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
        return fail, "QSanguosha does not support cin."
    end

    if string.sub(filename, 1, 4) == "qrc:" then
        -- TODO: load contents from QRC
        local resourceContent = sgs.qrc_contents(filename)
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
    if sgs.qrc_contains(fileName) then
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
JSON = require("JSON")

-- Descriptors here
-- Should it be k-v pair or sequence?
-- Add it to sgs_ex table
sgs_ex.Packages = {}
sgs_ex.CardFaces = {}
sgs_ex.Skills = {}

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
            if extension.cardFaces then
                for _, c in ipairs(extension.cardFaces) do
                    sgs_ex.CardFaces[c.name] = c
                end
            end
            if extension.skills then
                for _, s in ipairs(extension.skills) do
                    sgs_ex.Skills[s.name] = s
                end
            end
            sgs_ex.Packages[extension.name] = extension
            return true
        end
    end

    return false
end

local extensionDefinition = function(jsonFile)
    local f, e = io.open(jsonFile)
    if not f then
        error("Extension definition file " .. jsonFile .. " is missing or failed to load, reason: " .. e)
    end

    local s, d = pcall(JSON.decode, JSON, f:read("a"))
    f:close()
    if not s then
        error("JSON decode of extension definition file " .. jsonFile .. " failed, reason: " .. d)
    end

    -- the extension file may be a folder of Lua files or a single Lua file
    -- How to calculate checksum of multiple files? Maybe one checksum per file?
    return d
end

local verifyExtensionChecksum = function(name, checksums, isBuiltin)
    -- INTENSIONALLY no suffix slash here for supporting any terms of file here.
    local extensionPath = "lua/extension/" .. name
    if isBuiltin then
        extensionPath = "lua/builtinExtension/" .. name
    end

    for key, value in pairs(checksums) do
        -- ".lua"
        -- ".tl"
        -- "/init.lua"
        -- "/init.tl"
        -- "/xxxxx.tl"
        -- etc.
        -- INTENSIONALLY no slash is added here
        local extensionFile = extensionPath .. key
        local ok = sgs.BuiltinExtension_VerifyChecksum(extensionFile, value.sha256, sgs.QCryptographicHash_Sha256)
        if not ok then
            return false
        end
        ok = sgs.BuiltinExtension_VerifyChecksum(extensionFile, value.keccak256, sgs.QCryptographicHash_Keccak_256)
        if not ok then
            return false
        end
    end

    return true
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

    local def = extensionDefinition("lua/builtinExtension/extensions.json")

    for name, value in pairs(def) do
        if not verifyExtensionChecksum(name, value, true) then
            warn("Checksum of " .. name .. " mismatches")
            checksumFailed = true
        end

        if not loadExtension(name, true) then
            warn("Builtin extension " .. name .. " has failed to load")
            loadFailed = true
        end
    end

    if checksumFailed or loadFailed then
        -- sgs.BuiltinExtension_disableConnectToServer()
    end
end

local loadInstalledExtensions = function()
    -- TODO: allow to load Lua file with checksum check, discard it if checksum mismatches
    -- It is possible to provide an extension shop, which downloads and validates extension by using checksum

    -- load extension names from config file?

    local def = extensionDefinition("lua/extension/extensions.json")

    -- this def contains an array of extension names
    for _, name in ipairs(def) do
        local subdef = extensionDefinition("lua/extension/" .. name .. ".json")
        if subdef.test then
            warn("Extension " .. name .. "is during testing, checksum check bypassed.")
        elseif not verifyExtensionChecksum(name, subdef, false) then
            warn("Checksum of " .. name .. " mismatches thus not loaded.")
        else
            if not loadExtension(name, false) then
                warn("Extension " .. name .. " has failed to load")
            end
        end
    end
end

loadBuiltinExtensions()
loadInstalledExtensions()
