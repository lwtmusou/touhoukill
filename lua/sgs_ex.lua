local sgs_ex = {}

sgs_ex.TableType = {
    -- masks
    FirstTypeMask = 0xf00,
    SecondTypeMask = 0x0f0,
    ThirdTypeMask = 0x00f,

    Package = 0x000,
    CardPackage = 0x001,
    GeneralPackage = 0x002,

    CardFace = 0x100,

    BasicCard = 0x110,

    TrickCard = 0x120,
    NonDelayedTrick = 0x121,
    DelayedTrick = 0x122,

    EquipCard = 0x130,
    Weapon = 0x131,
    Armor = 0x132,
    DefensiveHorse = 0x133,
    OffensiveHorse = 0x134,
    Treasure = 0x135,

    SkillCard = 0x140,

    Skill = 0x200,
    ViewAsSkill = 0x210,
    FilterSkill = 0x220,
    ProhibitSkill = 0x230,
    DistanceSkill = 0x240,
    MaxCardsSkill = 0x250,
    AttackRangeSkill = 0x260,
    TreatAsEquippingSkill = 0x270,

    Trigger = 0x300,
    Rule = 0x310, -- ??
    SkillTrigger = 0x320,
    EquipSkillTrigger = 0x321,
    GlobalRecord = 0x330,
    FakeMoveRecord = 0x331,

    CardDescriptor = 0x400,

    GeneralDescriptor = 0x500,
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

    for _, i in ipairs(toBeValidated) do
        if not validateFunc(i) then
            return fail, 2, (functionName .. ": No. " .. tostring(_) .. " of " .. nameOfToBeValidated .. " is " .. invalidOutput)
        end
    end

    return true
end

sgs_ex.CardFace = function(desc, ...)
    local args = {...}
    local funcName = "sgs_ex.CardFace"
    if args[1] and type(args[1]) == "string" then
        funcName = args[1]
        table.remove(args, 1)
    end

    if type(desc) ~= "table" then
        return fail, funcName .. ": desc is not table"
    end

    -- Never ignore error check on desc.type since it contains the most important information of the card - The Card type
    if desc.type == null then
        return fail, funcName .. ": desc does not contain a valid Type"
    elseif desc.type & sgs_ex.TableType.FirstTypeMask ~= sgs_ex.TableType.CardFace then
        return fail, funcName .. ": desc does not contain a valid Card Type"
    end

    if maybeIgnoreErrorCheck(funcName, desc, table.unpack(args)) then
        return desc
    end

    -- Card Face common
    -- As a Card Face, the following are mandatory
    -- name -> string
    -- type (which is specified by desc.type using SecondTypeMask / ThirdTypeMask)
    -- subTypeName -> string
    -- kind -> table (sequence) of strings
    -- (if different than default) properties, including
    --  - targetFixed = function(player, card) -> boolean
    --  - hasPreAction = function() -> boolean
    --  - canDamage = function() -> boolean
    --  - canRecover = function() -> boolean
    --  - hasEffectValue = function() -> boolean
    --  - defaultHandlingMethod = function() -> QSanguosha_HandlingMethod
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

    -- the re-crafted return value
    local r = {}

    -- name -> string
    if desc.name == null then
        return fail, funcName .. ": desc does not contain name"
    elseif type(desc.name) ~= "string" then
        return fail, funcName .. ": desc.name is not string"
    else
        r.name = desc.name
    end

    -- type (which is specified by desc.type using SecondTypeMask / ThirdTypeMask)
    -- pre-checked before maybeIgnoreErrorCheck
    r.type = desc.type

    -- subTypeName -> string
    if desc.subTypeName == null then
        return fail, funcName .. ": desc does not contain subTypeName"
    elseif type(desc.subTypeName) ~= "string" then
        return fail, funcName .. ": desc.subTypeName is not string"
    else
        r.subTypeName = desc.subTypeName
    end

    -- kind -> table (sequence) of strings
    if desc.kind == null then
        return fail, funcName .. ": desc does not contain a vaild kind"
    else
        r.kind = desc.kind
        table.insert(r.kind, "CardFace")
    end

    --  - targetFixed = function(player, card) -> boolean
    if desc.targetFixed ~= null then
        if (type(desc.targetFixed) ~= "boolean") and (type(desc.targetFixed) ~= "function") then
            warn(funcName .. ": desc.targetFixed is not boolean or function and is ignored")
        else
            r.targetFixed = desc.targetFixed
        end
    end

    --  - hasPreAction = function() -> boolean
    if desc.hasPreAction ~= null then
        if (type(desc.hasPreAction) ~= "boolean") and (type(desc.hasPreAction) ~= "function") then
            warn(funcName .. ": desc.hasPreAction is not boolean or function and is ignored")
        else
            r.hasPreAction = desc.hasPreAction
        end
    end

    --  - canDamage = function() -> boolean
    if desc.canDamage ~= null then
        if (type(desc.canDamage) ~= "boolean") and (type(desc.canDamage) ~= "function") then
            warn(funcName .. ": desc.canDamage is not boolean or function and is ignored")
        else
            r.canDamage = desc.canDamage
        end
    end

    --  - canRecover = function() -> boolean
    if desc.canRecover ~= null then
        if (type(desc.canRecover) ~= "boolean") and (type(desc.canRecover) ~= "function") then
            warn(funcName .. ": desc.canRecover is not boolean or function and is ignored")
        else
            r.canRecover = desc.canRecover
        end
    end

    --  - hasEffectValue = function() -> boolean
    if desc.hasEffectValue ~= null then
        if (type(desc.hasEffectValue) ~= "boolean") and (type(desc.hasEffectValue) ~= "function") then
            warn(funcName .. ": desc.hasEffectValue is not boolean or function and is ignored")
        else
            r.hasEffectValue = desc.hasEffectValue
        end
    end

    --  - defaultHandlingMethod = function() -> QSanguosha_HandlingMethod
    if desc.defaultHandlingMethod ~= null then
        if (type(desc.defaultHandlingMethod) ~= "number") and (type(desc.defaultHandlingMethod) ~= "function") then -- todo: distinguishing number and integer
            warn(funcName .. ": desc.defaultHandlingMethod is not number or function and is ignored")
        else
            r.defaultHandlingMethod = desc.defaultHandlingMethod
        end
    end

    --  - targetsFeasible - function(playerList, player, card) -> boolean
    if desc.targetsFeasible ~= null then
        if type(desc.targetsFeasible) ~= "function" then
            warn(funcName .. ": desc.targetsFeasible is not function and is ignored")
        else
            r.targetsFeasible = desc.targetsFeasible
        end
    end

    --  - targetFilter - function(playerList, player, player, card) -> integer
    if desc.targetFilter ~= null then
        if type(desc.targetFilter) ~= "function" then
            warn(funcName .. ": desc.targetFilter is not function and is ignored")
        else
            r.targetFilter = desc.targetFilter
        end
    end

    --  - isAvailable - function(player, card) -> boolean
    if desc.isAvailable ~= null then
        if type(desc.isAvailable) ~= "function" then
            warn(funcName .. ": desc.isAvailable is not function and is ignored")
        else
            r.isAvailable = desc.isAvailable
        end
    end

    --  - validate - function(cardUse) -> card
    if desc.validate ~= null then
        if type(desc.validate) ~= "function" then
            warn(funcName .. ": desc.validate is not function and is ignored")
        else
            r.validate = desc.validate
        end
    end

    --  - validateInResponse - function(player, card) -> card
    if desc.validateInResponse ~= null then
        if type(desc.validateInResponse) ~= "function" then
            warn(funcName .. ": desc.validateInResponse is not function and is ignored")
        else
            r.validateInResponse = desc.validateInResponse
        end
    end

    --  - doPreAction - function(room, cardUse)
    if desc.doPreAction ~= null then
        if type(desc.doPreAction) ~= "function" then
            warn(funcName .. ": desc.doPreAction is not function and is ignored")
        else
            r.doPreAction = desc.doPreAction
        end
    end

    --  - onUse - function(room, cardUse)
    if desc.onUse ~= null then
        if type(desc.onUse) ~= "function" then
            warn(funcName .. ": desc.onUse is not function and is ignored")
        else
            r.onUse = desc.onUse
        end
    end

    --  - use - function(room, cardUse)
    if desc.use ~= null then
        if type(desc.use) ~= "function" then
            warn(funcName .. ": desc.use is not function and is ignored")
        else
            r.use = desc.use
        end
    end

    --  - onEffect(cardEffect)
    if desc.onEffect ~= null then
        if type(desc.onEffect) ~= "function" then
            warn(funcName .. ": desc.onEffect is not function and is ignored")
        else
            r.onEffect = desc.onEffect
        end
    end

    --  - isCancelable(cardEffect) -> boolean
    if desc.isCancelable ~= null then
        if type(desc.isCancelable) ~= "function" then
            warn(funcName .. ": desc.isCancelable is not function and is ignored")
        else
            r.isCancelable = desc.isCancelable
        end
    end

    --  - onNullified(player, card)
    if desc.onNullified ~= null then
        if type(desc.onNullified) ~= "function" then
            warn(funcName .. ": desc.onNullified is not function and is ignored")
        else
            r.onNullified = desc.onNullified
        end
    end

    return r
end

sgs_ex.BasicCard = function(desc, ...)
    -- BasicCard is no more than Card, except for its card type
    if type(desc) ~= "table" then
        return fail, "sgs_ex.BasicCard: desc is not table"
    end

    if desc.type == null then
        desc.type = sgs_ex.TableType.BasicCard
    end

    if desc.kind == null then
        desc.kind = {"BasicCard"}
    else
        table.insert(desc.kind, "BasicCard")
    end

    if maybeIgnoreErrorCheck(funcName, desc, ...) then
        return desc
    end

    return sgs_ex.CardFace(desc, "sgs_ex.BasicCard", ...)
end

sgs_ex.TrickCard = function(desc, ...)
    -- TrickCard has 2 subtypes: NonDelayedTrick and DelayedTrick
    -- For NonDelayedTrick, it is almost same as BasicCard
    -- For DelayedTrick, there should be a JudgeStruct attached to it
    -- Implement NonDelayedTrick and DelayedTrick separately and use this function as a backend
    local args = {...}
    local funcName = "sgs_ex.TrickCard"
    if args[1] and type(args[1]) == "string" then
        funcName = args[1]
        table.remove(args, 1)
    end

    if type(desc) ~= "table" then
        return fail, funcName .. ": desc is not table"
    end

    if desc.type == null then
        return fail, funcName .. ": desc does not contain a valid Type"
    end

    if desc.kind == null then
        return fail, funcName .. ": desc does not contain a valid kind"
    else
        table.insert(desc.kind, "TrickCard")
    end

    if maybeIgnoreErrorCheck(funcName, desc, table.unpack(args)) then
        return desc
    end

    return sgs_ex.CardFace(desc, funcName, table.unpack(args))
end

sgs_ex.NonDelayedTrick = function(desc, ...)
    -- NonDelayedTrick is no more than TrickCard, except for its card type
    if type(desc) ~= "table" then
        return fail, "sgs_ex.NonDelayedTrick: desc is not table"
    end

    if desc.type == null then
        desc.type = sgs_ex.TableType.NonDelayedTrick
    end

    if desc.kind == null then
        desc.kind = {"NonDelayedTrick"}
    else
        table.insert(desc.kind, "NonDelayedTrick")
    end

    if maybeIgnoreErrorCheck(funcName, desc, ...) then
        return desc
    end

    return sgs_ex.TrickCard(desc, "sgs_ex.NonDelayedTrick", ...)
end

sgs_ex.DelayedTrick = function(desc, ...)
    -- DelayedTrick has an extra member called judge and extra member function called takeEffect
    -- they can't be null

    if type(desc) ~= "table" then
        return fail, "sgs_ex.NonDelayedTrick: desc is not table"
    end

    if desc.type == null then
        desc.type = sgs_ex.TableType.NonDelayedTrick
    end

    if desc.kind == null then
        desc.kind = {"DelayedTrick"}
    else
        table.insert(desc.kind, "DelayedTrick")
    end

    if maybeIgnoreErrorCheck(funcName, desc, ...) then
        return desc
    end

    local r2 = {}

    if desc.judge ~= null then
        r2.judge = desc.judge
    else
        return fail, "sgs_ex.DelayedTrick: desc.judge is null"
    end

    if desc.takeEffect == null then
        return fail, "sgs_ex.DelayedTrick: desc.takeEffect is null"
    elseif type(desc.takeEffect) ~= "function" then
        return fail, "sgs_ex.DelayedTrick: desc.takeEffect is not function"
    else
        r2.takeEffect = desc.takeEffect
    end

    local r, e = sgs_ex.TrickCard(desc, "sgs_ex.DelayedTrick", ...)

    if not r then
        return r, e
    end

    r.judge = r2.judge
    r.takeEffect = r2.takeEffect

    return r
end

sgs_ex.EquipCard = function(desc, ...)
    local args = {...}
    local funcName = "sgs_ex.EquipCard"
    if args[1] and type(args[1]) == "string" then
        funcName = args[1]
        table.remove(args, 1)
    end

    if type(desc) ~= "table" then
        return fail, funcName .. ": desc is not table"
    end

    if desc.type == null then
        return fail, funcName .. ": desc does not contain a valid Type"
    end

    if desc.kind == null then
        return fail, funcName .. ": desc does not contain a valid kind"
    else
        table.insert(desc.kind, "EquipCard")
    end

    if maybeIgnoreErrorCheck(funcName, desc, table.unpack(args)) then
        return desc
    end

    -- EquipCard has 5 subtypes which are corresponding to the 5 types of Equip card in Sanguosha.
    -- The process of using EquipCard is totally different than using BasicCard and TrickCard. (Deal with it in C++)
    -- Implement 5 subtypes separately and use this function as a backend
    -- EquipCard has 2 optional function:
    --  - onInstall
    --  - onUninstall (for silverlion record, although this event seems able to record in a Trigger)

    local r2 = {}

    --  - onInstall(player)
    if desc.onInstall ~= null then
        if type(desc.onInstall) ~= "function" then
            warn(funcName .. ": desc.onInstall is not function and is ignored")
        else
            r2.onInstall = desc.onInstall
        end
    end

    --  - onUninstall(player)
    if desc.onUninstall ~= null then
        if type(desc.onUninstall) ~= "function" then
            warn(funcName .. ": desc.onUninstall is not function and is ignored")
        else
            r2.onUninstall = desc.onUninstall
        end
    end

    local r, e = sgs_ex.CardFace(desc, funcName, table.unpack(args))

    if not r then
        return r, e
    end

    r.onInstall = r2.onInstall
    r.onUninstall = r2.onUninstall

    return r
end

sgs_ex.Weapon = function(desc, ...)
    -- In addition to EquipCard, Weapons have an additional property which is 'range'
    -- 'range' is mandatory in definations of Weapon so check it in this function
    if type(desc) ~= "table" then
        return fail, "sgs_ex.Weapon: desc is not table"
    end

    if desc.type == null then
        desc.type = sgs_ex.TableType.Weapon
    end

    if desc.kind == null then
        desc.kind = {"Weapon"}
    else
        table.insert(desc.kind, "Weapon")
    end

    if maybeIgnoreErrorCheck(funcName, desc, ...) then
        return desc
    end

    local r2 = {}

    if desc.range == null then
        return fail, funcName .. ": desc does not contain range"
    elseif type(desc.range) ~= "number" then
        return fail, funcName .. ": desc.range is not number"
    else
        r2.range = desc.range
    end

    local r, e = sgs_ex.EquipCard(desc, "sgs_ex.Weapon", ...)

    if not r then
        return r, e
    end

    r.range = r2.range

    return r
end

sgs_ex.Armor = function(desc, ...)
    -- Armor is no more than EquipCard, except for its card type
    if type(desc) ~= "table" then
        return fail, "sgs_ex.Armor: desc is not table"
    end

    if desc.type == null then
        desc.type = sgs_ex.TableType.Armor
    end

    if desc.kind == null then
        desc.kind = {"Armor"}
    else
        table.insert(desc.kind, "Armor")
    end

    if maybeIgnoreErrorCheck(funcName, desc, ...) then
        return desc
    end

    return sgs_ex.EquipCard(desc, "sgs_ex.Armor", ...)
end

sgs_ex.DefensiveHorse = function(desc, ...)
    -- DefensiveHorse is no more than EquipCard, except for its card type
    if type(desc) ~= "table" then
        return fail, "sgs_ex.DefensiveHorse: desc is not table"
    end

    if desc.type == null then
        desc.type = sgs_ex.TableType.DefensiveHorse
    end

    if desc.kind == null then
        desc.kind = {"DefensiveHorse"}
    else
        table.insert(desc.kind, "DefensiveHorse")
    end

    if maybeIgnoreErrorCheck(funcName, desc, ...) then
        return desc
    end

    return sgs_ex.EquipCard(desc, "sgs_ex.DefensiveHorse", ...)
end

sgs_ex.OffensiveHorse = function(desc, ...)
    -- OffensiveHorse is no more than EquipCard, except for its card type
    if type(desc) ~= "table" then
        return fail, "sgs_ex.OffensiveHorse: desc is not table"
    end

    if desc.type == null then
        desc.type = sgs_ex.TableType.OffensiveHorse
    end

    if desc.kind == null then
        desc.kind = {"OffensiveHorse"}
    else
        table.insert(desc.kind, "OffensiveHorse")
    end

    if maybeIgnoreErrorCheck(funcName, desc, ...) then
        return desc
    end

    return sgs_ex.EquipCard(desc, "sgs_ex.OffensiveHorse", ...)
end

sgs_ex.Treasure = function(desc, ...)
    -- Treasure is no more than EquipCard, except for its card type
    if type(desc) ~= "table" then
        return fail, "sgs_ex.Treasure: desc is not table"
    end

    if desc.type == null then
        desc.type = sgs_ex.TableType.Treasure
    end

    if desc.kind == null then
        desc.kind = {"Treasure"}
    else
        table.insert(desc.kind, "Treasure")
    end

    if maybeIgnoreErrorCheck(funcName, desc, ...) then
        return desc
    end

    return sgs_ex.EquipCard(desc, "sgs_ex.Treasure", ...)
end

sgs_ex.SkillCard = function(desc, ...)
    -- SkillCard adds a property names throwWhenUsing
    if type(desc) ~= "table" then
        return fail, "sgs_ex.SkillCard: desc is not table"
    end

    if desc.type == null then
        desc.type = sgs_ex.TableType.SkillCard
    end

    if desc.kind == null then
        desc.kind = {"SkillCard"}
    else
        table.insert(desc.kind, "SkillCard")
    end

    if maybeIgnoreErrorCheck(funcName, desc, ...) then
        return desc
    end

    local r2 = {}

    --  - throwWhenUsing = function() -> boolean
    if desc.throwWhenUsing ~= null then
        if (type(desc.throwWhenUsing) ~= "boolean") and (type(desc.throwWhenUsing) ~= "function") then
            warn(funcName .. "sgs_ex.SkillCard: desc.throwWhenUsing is not boolean or function and is ignored")
        else
            r2.throwWhenUsing = desc.throwWhenUsing
        end
    end

    local r, e = sgs_ex.CardFace(desc, "sgs_ex.SkillCard", ...)

    if not r then
        return r, e
    end

    r.throwWhenUsing = r2.throwWhenUsing

    return r
end

-- Enough error check is necessary
-- Lua is a weak-type scripting language after all, but we should make it more robust
-- return fail plus an error message for error
sgs_ex.Package = function(desc, ...)
    if type(desc) ~= "table" then
        return fail, "sgs_ex.Package: desc is not table"
    end

    if maybeIgnoreErrorCheck("sgs_ex.Package", desc, ...) then
        if desc.type == null then
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
    if desc.cards ~= null then
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
    elseif desc.generals ~= null then
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

    isValid, num, msg = typeValidate("sgs_ex.Package", desc.triggers, "desc.triggers", function(c)
        return (type(c) == "table") and ((c.type & sgs_ex.TableType.FirstTypeMask) == sgs_ex.TableType.Trigger)
    end, "not Trigger")

    if not isValid then
        if num == 0 then
            -- do nothing
        elseif num == 1 then
            warn(msg)
        else
            return fail, msg
        end
    else
        r.triggers = desc.triggers
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
