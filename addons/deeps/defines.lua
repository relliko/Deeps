--[[
* Deeps - defines
* Message ids, action types, bar colors and window sizes (from the plugin's Defines.h).
--]]

local function argb(a, r, g, b)
    return bit.bor(bit.lshift(a, 24), bit.lshift(r, 16), bit.lshift(g, 8), b) % 0x100000000;
end

local defines = { argb = argb };

-- Window and bar sizes, in pixels before TV mode's scale.
defines.TITLE_FONT_HEIGHT      = 10;
defines.BAR_FONT_HEIGHT        = 8;
defines.TITLEBAR_HEIGHT        = 17;
defines.BAR_WIDTH              = 250;
defines.BAR_HEIGHT             = 13;
defines.BAR_HORIZONTAL_PADDING = 4;
defines.BAR_BACKGROUND_HEIGHT  = 16;
defines.BETWEEN_BAR_PADDING    = 3;
defines.WINDOW_WIDTH           = 258;

-- Action packet categories.
defines.ACTION = {
    MELEE            = 0x01,
    RA_FINISH        = 0x02,
    WS_FINISH        = 0x03,
    CAST_FINISH      = 0x04,
    ITEM_FINISH      = 0x05,
    JA               = 0x06,
    WS_START         = 0x07,
    CAST_START       = 0x08,
    ITEM_START       = 0x09,
    NPC_TP_FINISH    = 0x0B,
    RA_START         = 0x0C,
    AVATAR_BP_FINISH = 0x0D,
    JA_DNC           = 0x0E,
    JA_RUN           = 0x0F,
};

-- The categories whose results are counted.
defines.PARSED = {
    [0x01] = true, [0x02] = true, [0x03] = true, [0x04] = true, [0x06] = true,
    [0x0B] = true, [0x0D] = true, [0x0E] = true, [0x0F] = true,
};

local function set(list)
    local s = { };
    for _, v in ipairs(list) do
        s[v] = true;
    end
    return s;
end

-- Result messages. 110 is a job ability's damage (Jump, Chi Blast, Quick Draw, Weapon Bash, ...).
defines.HIT   = set({ 1, 2, 77, 110, 132, 157, 161, 163, 185, 187, 197, 227, 264, 281, 317, 352, 413, 522, 576, 577 });
defines.CRIT  = set({ 67, 252, 265, 274, 353, 379 });
defines.MISS  = set({ 15, 85, 158, 188, 245, 284, 324, 354 });
defines.EVADE = set({ 14, 30, 31, 32, 33, 189, 248, 282, 283, 323, 355 });
defines.PARRY = set({ 69, 70 });

-- A weapon skill category result with these messages is a job ability sent that way (Jump,
-- High Jump, Eagle Eye Shot, Weapon Bash, ...): its id is an ability id, not a weapon skill's.
defines.JA_AS_WS = set({ 110, 158 });

-- Additional effect damage.
defines.ADD_EFFECT_DMG = set({ 163, 229 });

-- Skillchain messages (in the closing weapon skill's additional effect), 287 + the chain.
defines.SKILLCHAIN = {
    [288] = 'Light',         [289] = 'Darkness',     [290] = 'Gravitation', [291] = 'Fragmentation',
    [292] = 'Distortion',    [293] = 'Fusion',       [294] = 'Compression', [295] = 'Liquefaction',
    [296] = 'Induration',    [297] = 'Reverberation', [298] = 'Transfixion', [299] = 'Scission',
    [300] = 'Detonation',    [301] = 'Impaction',    [302] = 'Skillchain',
};

-- Damage a defender deals back to its attacker, from the spikes part of the attacker's action.
defines.SPIKES = {
    [33]  = 'Counter',
    [44]  = 'Spikes',
    [132] = 'Spikes',
    [536] = 'Retaliation',
};

-- Bar colors by main job (job 0: /anon or unknown).
defines.JOB_COLORS = {
    [0]  = argb(0xFF, 0x19, 0x19, 0x70), -- NON: Midnight blue
    [1]  = argb(0xFF, 0xFF, 0x00, 0x00), -- WAR: Red
    [2]  = argb(0xFF, 0xFF, 0x8C, 0x00), -- MNK: Dark orange
    [3]  = argb(0xFF, 0xFF, 0xFF, 0xFF), -- WHM: White
    [4]  = argb(0xFF, 0x4B, 0x00, 0x82), -- BLM: Indigo
    [5]  = argb(0xFF, 0xFF, 0x69, 0xB4), -- RDM: Pink
    [6]  = argb(0xFF, 0x22, 0x8B, 0x22), -- THF: Forest green
    [7]  = argb(0xFF, 0xD3, 0xD3, 0xD3), -- PLD: Light grey
    [8]  = argb(0xFF, 0x44, 0x44, 0x44), -- DRK: Dark grey
    [9]  = argb(0xFF, 0x8B, 0x45, 0x13), -- BST: Saddle brown
    [10] = argb(0xFF, 0xFF, 0xFF, 0x00), -- BRD: Yellow
    [11] = argb(0xFF, 0xAD, 0xFF, 0x2F), -- RNG: Green yellow
    [12] = argb(0xFF, 0x80, 0x00, 0x00), -- SAM: Maroon
    [13] = argb(0xFF, 0x70, 0x80, 0x90), -- NIN: Slate grey
    [14] = argb(0xFF, 0x93, 0x70, 0xDB), -- DRG: Medium purple
    [15] = argb(0xFF, 0x00, 0xFF, 0xFF), -- SMN: Cyan
    [16] = argb(0xFF, 0x41, 0x69, 0xE1), -- BLU: Royal blue
    [17] = argb(0xFF, 0xFF, 0xD8, 0xB1), -- COR: Apricot
    [18] = argb(0xFF, 0xFF, 0xFA, 0xC8), -- PUP: Beige
    [19] = argb(0xFF, 0xE6, 0x19, 0x4B), -- DNC: Red
    [20] = argb(0xFF, 0xCD, 0x85, 0x3F), -- SCH: Peru
    [21] = argb(0xFF, 0x80, 0x80, 0x00), -- GEO: Olive
    [22] = argb(0xFF, 0x00, 0xFF, 0x00), -- RUN: placeholder
};

-- Bar colors for players outside the party, pets, and when job colors are off.
defines.RANDOM_COLORS = {
    argb(255, 12, 0, 155),   argb(255, 140, 0, 0),    argb(255, 255, 177, 32), argb(255, 143, 143, 143),
    argb(255, 68, 68, 68),   argb(255, 255, 0, 0),    argb(255, 0, 164, 49),   argb(255, 198, 198, 0),
    argb(255, 116, 0, 145),  argb(255, 165, 153, 10), argb(255, 184, 128, 10), argb(255, 224, 0, 230),
    argb(255, 234, 100, 0),  argb(255, 119, 0, 0),    argb(255, 130, 17, 255), argb(255, 79, 196, 0),
    argb(255, 0, 16, 217),   argb(255, 136, 68, 0),   argb(255, 244, 98, 0),   argb(255, 15, 190, 220),
    argb(255, 0, 123, 145),
};

-- The Skillchains bar (/dps sc bar).
defines.SKILLCHAIN_COLOR = argb(0xFF, 0xDA, 0xA5, 0x20);

return defines;
