
-- Initialization script for QSanguosha
-- Assuming standard module and module "sgs" imported

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

-- disable coroutine since it use sjlj, which is not c++-exception-aware
coroutine = nil
package.loaded.coroutine = nil

-- dofile with qrc support
local originalDofile = dofile
dofile = function(filename)
    if not filename then
        error("QSanguosha does not support cin.")
    end

    if string.sub(filename, 1, 4) == "qrc:" then
        -- TODO: load contents from QRC
        local resourceContent = sgs.contentFromQrc(filename)
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
        local resourceContent = sgs.contentFromQrc(filename)
        if resourceContent then
            return load(resourceContent, filename, ...)
        end
    end

    return originalLoadfile(filename, ...)
end

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

-- 1. load utilities
dofile("qrc:/utilities.lua")

-- 3. load extensions
-- sgs_ex = require("qrc:/sgs_ex.lua")
sgs_ex = dofile("qrc:/sgs_ex.lua")
package.loaded["sgs_ex"] = sgs_ex

sgs.Packages = {}
sgs.CardFaces = {}
sgs.Skills = {}

-- TODO: load extensions
