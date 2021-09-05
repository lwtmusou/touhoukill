local sgs_ex = {}

sgs_ex.TableType = {
    -- masks
    FirstTypeMask = 3840, -- 0xf00
    SecondTypeMask = 15, -- 0x00f
    ThirdTypeMask = 240, -- 0x0f0

    Package = 0,     -- 0x000
    CardPackage = 1,
    GeneralPackage = 2,

    CardFace = 256,  -- 0x100

    BasicCard = 257, -- 0x101

    TrickCard = 258, -- 0x102
    NonDelayedTrick = 274, -- 0x112
    DelayedTrick = 290, -- 0x122

    EquipCard = 259, -- 0x103
    Weapon = 275,
    Armor = 291,
    DefensiveHorse = 307,
    OffensiveHorse = 323,
    Treasure = 339,

    SkillCard = 260,

    Skill = 512,      -- 0x200
    ViewAsSkill = 513,
    FilterSkill = 514,
    ProhibitSkill = 515,
    DistanceSkill = 516,
    MaxCardsSkill = 517,
    AttackRangeSkill = 518,
    TreatAsEquippingSkill = 519,

    Trigger = 768,
    CardDescriptor = 1024,
    GeneralDescriptor = 1280,
}

-- if string "ignoreErrorCheck" is given as last parameter of all functions, ignores any error check
local maybeIgnoreErrorCheck = function(functionName, desc, ...)
    local ignoreErrorCheck = desc.ignoreErrorCheck
    if not ignoreErrorCheck then
        local vararg = {...}
        if vararg[#vararg] == "ignoreErrorCheck" then
            ignoreErrorCheck = true
        end
    end

    if ignoreErrorCheck then
        warn(functionName .. ": ignoring error check on table " .. desc.name)
        desc.ignoreErrorCheck = true
    end

    return ignoreErrorCheck
end

-- return 1 or 3 values, where:
-- only true if it is valid
-- fail plus 0 and an error message: toBeValidated is nil
-- fail plus 1 and an error message: toBeValidated is not table
-- fail plus 2 and an error message: at least one item in toBeValidated is invalid
local typeValidate = function(functionName, toBeValidated, nameOfToBeValidated, validateFunc, invalidOutput)
    if not toBeValidated then
        return fail, 0, (functionName .. ": " .. nameOfToBeValidated .. " is nil.")
    end

    if type(toBeValidated) ~= "table" then
        return fail, 1, (functionName .. ": " .. nameOfToBeValidated .. " is not table.")
    end

    for _, i in iparis(toBeValidated) do
        if not validateFunc(i) then
            return fail, 2, (functionName .. ": No. " .. tostring(_) .. " of " .. nameOfToBeValidated .. " is " .. invalidOutput)
        end
    end

    return true
end

-- Should this be the public interface?
sgs_ex.CardFace = function(desc, ...)
    local args = {...}
    local funcName = "sgs_ex.CardFace"
    if args[1] and type(args[1]) == "string" then
        funcName = args[1]
    end

    if type(desc) ~= "table" then
        return fail, funcName .. ": desc is not table"
    end

    -- Never ignore error check on desc.type since it contains the most important information of the card - The Card type
    if not desc.type then
        return fail, funcName .. ": desc does not contain a valid Type"
    elseif desc.type & sgs_ex.TableType.FirstTypeMask ~= sgs_ex.TableType.CardFace then
        return fail, funcName .. ": desc does not contain a valid Card Type"
    end

    if maybeIgnoreErrorCheck(funcName, desc, ...) then
        return desc
    end

    -- Card Face common
    -- As a Card Face, the following are mandatory
    -- name -> string
    -- type (which is specified by desc.type using SecondTypeMask / ThirdTypeMask)
    -- subTypeName -> string
    -- (if different than default) properties, including
    --  - target_fixed = function(player, card) -> boolean
    --  - throw_when_using = function() -> boolean
    --  - has_preact = function() -> boolean
    --  - can_damage = function() -> boolean
    --  - can_recover = function() -> boolean
    --  - has_effectvalue = function() -> boolean
    --  - default_method = function() -> Card_HandlingMethod
    -- these may be a fixed value or Lua Function, depanding on its usage. Function prototype is provided in case a function should be used.
    -- methods, including
    --  - targetsFeasible - function(playerList, player, card) -> boolean
    --  - targetFilter - function(playerList, player, player, card) -> integer
    --  - isAvailable - function(player, card) -> boolean
    --  - validate - function(cardUse) -> card
    --  - validateInResponse - function(player, card) -> card
    --  - doPreAction - function(room, cardUse)
    --  - onUse - function(room, cardUse)
    --  - use - function(room, cardUse)
    --  - onEffect(cardEffect)
    --  - isCancelable(cardEffect) -> boolean
    --  - onNullified(player, card)
    -- All of them are optional but this card does nothing if none is provided.

    -- TODO: implementations
end

sgs_ex.BasicCard = function(desc, ...)
    -- BasicCard is no more than Card, except for its card type
    if type(desc) ~= "table" then
        return fail, "sgs_ex.BasicCard: desc is not table"
    end

    if not desc.type then
        desc.type = sgs_ex.TableType.BasicCard
    end

    return sgs_ex.CardFace(desc, "sgs_ex.BasicCard", ...)
end

sgs_ex.TrickCard = function(desc, ...)
    local args = {...}
    local funcName = "sgs_ex.TrickCard"
    if args[1] and type(args[1]) == "string" then
        funcName = args[1]
    end
    -- TrickCard has 2 subtypes: NonDelayedTrick and DelayedTrick
    -- For NonDelayedTrick, it is almost same as BasicCard
    -- For DelayedTrick, there should be a JudgeStruct attached to it
    -- Implement NonDelayedTrick and DelayedTrick separately and use this function as a backend

    -- TODO: implementations
    return sgs_ex.CardFace(desc, funcName, ...)
end

sgs_ex.NonDelayedTrick = function(desc, ...)
    -- TODO: designation and implementations
    return sgs_ex.TrickCard(desc, "sgs_ex.NonDelayedTrick", ...)
end

sgs_ex.DelayedTrick = function(desc, ...)
    -- TODO: designation and implementations
    return sgs_ex.TrickCard(desc, "sgs_ex.DelayedTrick", ...)
end

sgs_ex.EquipCard = function(desc, ...)
    local args = {...}
    local funcName = "sgs_ex.EquipCard"
    if args[1] and type(args[1]) == "string" then
        funcName = args[1]
    end
    -- EquipCard has 5 subtypes which are corresponding to the 5 types of Equip card in Sanguosha.
    -- The process of using EquipCard is totally different than using BasicCard and TrickCard. (Deal with it in C++?)
    -- Implement 5 subtypes separately and use this function as a backend
    -- EquipCard has 2 optional function:
    --  - onInstall
    --  - onUninstall (for silverlion record, although this event seems able to record in a Trigger)

    -- TODO: implementations
end

sgs_ex.Weapon = function(desc, ...)
    -- In addition to EquipCard, Weapons have an additional property which is 'range'
    -- 'range' is mandatory in definations of Weapon so check it in this function

    -- TODO: implementations
    return sgs_ex.EquipCard(desc, "sgs_ex.Weapon", ...)
end

sgs_ex.Armor = function(desc, ...)

end

sgs_ex.DefensiveHorse = function(desc, ...)

end

sgs_ex.OffensiveHorse = function(desc, ...)

end

sgs_ex.Treasure = function(desc, ...)

end

-- Enough error check is necessary
-- Lua is a weak-type scripting language after all, but we should make it more robust
-- return fail plus an error message for error
sgs_ex.Package = function(desc, ...)
    if type(desc) ~= "table" then
        return fail, "sgs_ex.Package: desc is not table"
    end

    if maybeIgnoreErrorCheck("sgs_ex.Package", desc, ...) then
        if not desc.type then
            if desc.cards then
                desc.type = sgs_ex.TableType.CardPackage
            else
                desc.type = sgs_ex.TableType.GeneralPackage
            end
        end
        return desc
    end

    if (not desc.name) or (type(desc.name) ~= "string") then
        return fail, "sgs_ex.Package: type of item 'name' is incorrect"
    end

    local r = {}
    local isValid, num, msg

    r.type = sgs_ex.TableType.Package
    r.name = desc.name
    if desc.cards then
        isValid, num, msg = typeValidate("sgs_ex.Package", desc.cards, "desc.cards", function(c)
            return (type(c) == "table") and ((c.type & sgs_ex.TableType.FirstTypeMask) == sgs_ex.TableType.CardDescriptor)
        end, "not CardDescriptor")

        if not isValid then
            return fail, msg
        end

        if desc.generals then
            warn("sgs_ex.Package: " .. desc.name .. " should contain either cards or generals, currently both, ignoring generals")
        end

        r.type = sgs_ex.TableType.CardPackage
        r.cards = desc.cards
    elseif desc.generals then
        isValid, num, msg = typeValidate("sgs_ex.Package", desc.generals, "desc.generals", function(c)
            return (type(c) == "table") and ((c.type & sgs_ex.TableType.FirstTypeMask) == sgs_ex.TableType.GeneralDescriptor)
        end, "not GeneralDescriptor")

        if not isValid then
            return fail, msg
        end

        r.type = sgs_ex.TableType.GeneralPackage
        r.generals = desc.generals
    else
        return fail, ("sgs_ex.Package: " .. desc.name .. " should contain either cards or generals as table(array), currently none exists")
    end

    isValid, num, msg = typeValidate("sgs_ex.Package", desc.skills, "desc.skills", function(c)
        return (type(c) == "table") and ((c.type & sgs_ex.TableType.FirstTypeMask) == sgs_ex.TableType.Skill)
    end, "not Skill")

    if not isValid then
        if num == 0 then
            -- do nothing
        elseif num == 1 then
            warn(msg)
        else
            return fail, msg
        end
    else
        r.skills = desc.skills
    end

    isValid, num, msg = typeValidate("sgs_ex.Package", desc.cardFaces, "desc.cardFaces", function(c)
        return (type(c) == "table") and ((c.type & sgs_ex.TableType.FirstTypeMask) == sgs_ex.TableType.CardFace)
    end, "not CardFace")

    if not isValid then
        if num == 0 then
            -- do nothing
        elseif num == 1 then
            warn(msg)
        else
            return fail, msg
        end
    else
        r.cardFaces = desc.cardFaces
    end

    return r
end

return sgs_ex
