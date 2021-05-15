local sgs_ex = {}

sgs_ex.TableType = {
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
    CardDescription = 1024,
}

sgs_ex.CardPackage = function(desc) {

}

return sgs_ex
