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
    -- ViewHasSkill = 519, -- What does 'View Has' mean?

    Trigger = 768,
    CardDescriptior = 1024,
    GeneralDescriptor = 1280,
}

-- Enough error check is necessary
-- Lua is a weak-type scripting language after all, but we should make it more robust
-- return fail plus an error message for error
sgs_ex.Package = function(desc)
    if type(desc) ~= "table" then
        return fail, "sgs_ex.Package: desc is not table"
    end
    if (not desc.name) or (type(desc.name) ~= "string") then
        return fail, "sgs_ex.Package: type of item 'name' is incorrect"
    end

    local r = {}

    r.packageType = sgs_ex.TableType.Package
    if desc.cards and (type(desc.cards) == "table") then
        if desc.generals then
            warn("sgs_ex.Package: " .. desc.name .. " should contain either cards or generals, currently ignoring generals")
        end

        for _, c in ipairs(desc.cards) do
            if (type(c) ~= "table") or ((c.type & sgs_ex.TableType.FirstTypeMask) ~= sgs_ex.TableType.CardDescriptior) then
                return fail, ("sgs_ex.Package: No. " .. tostring(_) .. " of desc.cards is not CardDescriptor")
            end
        end

        r.packageType = sgs_ex.TableType.CardPackage
        r.cards = desc.cards
    elseif desc.generals and (type(desc.generals) == "table") then
        for _, g in ipairs(desc.generals) do -- use pairs?
            if (type(g) ~= "table") or ((g.type & sgs_ex.TableType.FirstTypeMask) ~= sgs_ex.TableType.GeneralDescriptor) then
                return fail, ("sgs_ex.Package: No. " .. tostring(_) .. " of desc.generals is not General")
            end
        end

        r.packageType = sgs_ex.TableType.GeneralPackage
        r.generals = desc.generals
    else
        return fail, ("sgs_ex.Package: " .. desc.name .. " should contain either cards or generals as table(array), currently none exists")
    end

    if desc.skills then
        if type(desc.skills) ~= "table" then
            warn("sgs_ex.Package: desc.skills is not table, ignoring.")
        else
            for _, s in ipairs(desc.skills) do -- use pairs?
                if (type(s) ~= "table") or ((s.type & sgs_ex.TableType.FirstTypeMask) ~= sgs_ex.TableType.Skill) then
                    return fail, ("sgs_ex.Package: No. " .. tostring(_) .. " of desc.skills is not Skill")
                end
            end

            r.skills = desc.skills
        end
    end

    if desc.cardFaces then
        -- Zhao Hulu Hua Piao
    end

    return r
end

return sgs_ex
