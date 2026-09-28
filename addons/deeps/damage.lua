--[[
* Deeps - damage
* Reads action packets (0x028) and adds up damage by player and by source. Read only: it never
* changes or blocks a packet.
*
* Layout (bits from the start of the packet, least significant bit first):
*   40 actor id (32), 72 target count (6), 82 category (4), 86 action id (32), 118 recast (32),
*   150 targets. A target is its id (32) and result count (4), then each result:
*     +0 resolution (3), +3 kind (2), +5 animation (12), +17 info (5), +22 distortion (2),
*     +24 knockback (3), +27 param (17), +44 message (10), +54 modifier (31),
*     +85 additional effect flag (1) [+ kind (6), info (4), param (17), message (10)],
*     then the spikes flag (1) [+ kind (6), info (4), param (14), message (10)].
--]]

require('common');
local defines = require('defines');

local A = defines.ACTION;

local damage = {
    entities = { },    -- [server id] = { name, color, id, ownerid, sources = { [key] = source } }
    debug    = false,
    log      = print,  -- debug output
    seen     = { },    -- packets already read (the same packet can arrive twice)
    order    = { },
    index    = { },    -- [server id] = entity index, checked before use
};

local SEEN_MAX = 200;

--[[
* The entity index of a server id, or 0 when it isn't nearby.
--]]
function damage.index_of(id)
    local ent = AshitaCore:GetMemoryManager():GetEntity();
    local i = damage.index[id];
    if (i ~= nil and ent:GetServerId(i) == id) then
        return i;
    end
    -- Mobs, pets and NPCs carry their index in the low bits of the id.
    if (id >= 0x1000000) then
        i = bit.band(id, 0xFFF);
        if (i < 0x900 and ent:GetServerId(i) == id) then
            damage.index[id] = i;
            return i;
        end
    end
    for n = 0, 0x8FF do
        if (ent:GetServerId(n) == id) then
            damage.index[id] = n;
            return n;
        end
    end
    damage.index[id] = nil;
    return 0;
end

local function random_color()
    return defines.RANDOM_COLORS[math.random(1, #defines.RANDOM_COLORS)];
end

local function new_entity(id, name, ownerid)
    local e = { name = name or '(Unknown)', color = random_color(), id = id, ownerid = ownerid, sources = { } };
    damage.entities[id] = e;
    if (damage.debug) then
        local n = 0;
        for _ in pairs(damage.entities) do
            n = n + 1;
        end
        damage.log(('Total entities: %d'):fmt(n));
    end
    return e;
end

local function name_at(index)
    local n = AshitaCore:GetMemoryManager():GetEntity():GetName(index);
    return (n ~= nil and n ~= '') and n or nil;
end

--[[
* Notes the pet of an acting player: the pet's damage will count as theirs.
--]]
local function note_pet(owner, index)
    local ent = AshitaCore:GetMemoryManager():GetEntity();
    local pi = ent:GetPetTargetIndex(index);
    if (pi == nil or pi == 0) then
        return;
    end
    local pid = ent:GetServerId(pi);
    if (pid == nil or pid == 0) then
        return;
    end
    local pet = damage.entities[pid];
    if (pet == nil) then
        new_entity(pid, name_at(pi), owner.id);
    elseif (pet.ownerid ~= nil) then
        pet.ownerid = owner.id; -- the same pet id can come back as someone else's pet
    end
end

--[[
* An NPC that isn't known yet: the pet of a party member (who then gets its damage), or nil.
--]]
local function adopt_pet(id, index)
    local mm = AshitaCore:GetMemoryManager();
    local party, ent = mm:GetParty(), mm:GetEntity();
    for i = 0, 17 do
        local mi = party:GetMemberTargetIndex(i);
        local mid = party:GetMemberServerId(i);
        if (party:GetMemberIsActive(i) ~= 0 and mi ~= 0 and mid ~= 0 and ent:GetPetTargetIndex(mi) == index) then
            if (damage.entities[mid] == nil) then
                new_entity(mid, name_at(mi) or party:GetMemberName(i));
            end
            return new_entity(id, name_at(index), mid);
        end
    end
    return nil;
end

--[[
* The source an action goes into, made on first use.
--]]
local function source(owner, key, make)
    local s = owner.sources[key];
    if (s == nil) then
        s = make();
        s.damage = { };
        owner.sources[key] = s;
    end
    return s;
end

local function ability_name(id)
    local r = AshitaCore:GetResourceManager():GetAbilityById(id);
    return (r ~= nil and r.Name[1]) or ('Ability %d'):fmt(id);
end

local function spell_name(id)
    local r = AshitaCore:GetResourceManager():GetSpellById(id);
    return (r ~= nil and r.Name[1]) or ('Spell %d'):fmt(id);
end

--[[
* The source for one result of an action. category/id from the packet, message and animation
* from the result.
--]]
local function source_for(owner, is_pet, category, id, message, animation)
    if (is_pet) then
        return source(owner, 'pet', function () return { name = 'Pet', kind = 'pet' }; end);
    end
    if (category == A.MELEE and animation == 4) then
        category = A.RA_FINISH; -- Daken: a shuriken thrown during the attack round
    end
    if (category == A.MELEE) then
        return source(owner, 'atk', function () return { name = 'Attack' }; end);
    elseif (category == A.RA_FINISH) then
        return source(owner, 'ra', function () return { name = 'Ranged Attack' }; end);
    elseif (category == A.WS_FINISH and defines.JA_AS_WS[message]) then
        return source(owner, 'ja:' .. id, function () return { name = ability_name(id + 512) }; end);
    elseif (category == A.WS_FINISH or category == A.NPC_TP_FINISH) then
        return source(owner, 'ws:' .. id, function () return { name = ability_name(id) }; end);
    elseif (category == A.CAST_FINISH) then
        return source(owner, 'ma:' .. id, function () return { name = spell_name(id), kind = 'magic' }; end);
    elseif (category == A.JA or category == A.JA_DNC or category == A.JA_RUN) then
        return source(owner, 'ja:' .. id, function () return { name = ability_name(id + 512) }; end);
    end
    return source(owner, ('%d:%d'):fmt(category, id), function () return { name = ('Action %d'):fmt(id) }; end);
end

--[[
* Adds one result to a source's damage under `label` (Hit, Crit, Miss, ...).
--]]
local function add(src, label, value)
    local d = src.damage[label];
    if (d == nil) then
        d = { total = 0, count = 0, min = value, max = value };
        src.damage[label] = d;
    end
    d.total = d.total + value;
    d.count = d.count + 1;
    d.min = math.min(d.min, value);
    d.max = math.max(d.max, value);
end

--[[
* The label for a result message, and whether its param is damage; nil for messages not counted.
--]]
local function classify(message)
    if (defines.HIT[message]) then
        return 'Hit', true;
    elseif (defines.CRIT[message]) then
        return 'Crit', true;
    elseif (defines.MISS[message]) then
        return 'Miss', false;
    elseif (defines.EVADE[message]) then
        return 'Evade', false;
    elseif (defines.PARRY[message]) then
        return 'Parry', false;
    end
    return nil;
end

--[[
* True the second time the same packet is seen (the last SEEN_MAX are kept).
--]]
local function repeated(data)
    if (damage.seen[data]) then
        return true;
    end
    damage.seen[data] = true;
    damage.order[#damage.order + 1] = data;
    if (#damage.order > SEEN_MAX) then
        damage.seen[table.remove(damage.order, 1)] = nil;
    end
    return false;
end

--[[
* Walks every result of the packet: fn(target id, result) with the result's fields.
--]]
local function each_result(raw, fn)
    local function u(pos, len)
        return ashita.bits.unpack_be(raw, 0, pos, len);
    end
    local pos = 150;
    for _ = 1, u(72, 6) do
        local tid, count = u(pos, 32), u(pos + 32, 4);
        pos = pos + 36;
        for _ = 1, count do
            local r = {
                animation = u(pos + 5, 12),
                param     = u(pos + 27, 17),
                message   = u(pos + 44, 10),
            };
            pos = pos + 85;
            if (u(pos, 1) == 1) then
                r.add_param, r.add_message = u(pos + 11, 17), u(pos + 28, 10);
                pos = pos + 37;
            end
            pos = pos + 1;
            if (u(pos, 1) == 1) then
                r.spike_param, r.spike_message = u(pos + 11, 14), u(pos + 25, 10);
                pos = pos + 34;
            end
            pos = pos + 1;
            fn(tid, r);
        end
    end
end

--[[
* Who gets credit for damage `id` dealt: its entity, or its owner's if it's a pet (and whether
* it was a pet). nil when it isn't anyone we count.
--]]
local function credited(id, index)
    local e = damage.entities[id];
    if (e == nil) then
        if (id >= 0x1000000) then
            e = adopt_pet(id, index);
            if (e == nil) then
                return nil;
            end
        else
            e = new_entity(id, name_at(index));
        end
    end
    if (e.ownerid == nil) then
        note_pet(e, index);
        return e, false;
    end
    -- A pet: check its owner (the id can move to another player's pet after a resummon).
    local mm = AshitaCore:GetMemoryManager():GetEntity();
    local oi = mm:GetTrustOwnerTargetIndex(index);
    if (oi ~= nil and oi ~= 0) then
        local oid = mm:GetServerId(oi);
        if (oid ~= nil and oid ~= 0) then
            e.ownerid = oid;
        end
    end
    local owner = damage.entities[e.ownerid];
    if (owner == nil) then
        return nil;
    end
    return owner, true;
end

--[[
* A mob's action: the damage its targets dealt back to it (spikes, counters, retaliation).
--]]
local function spikes_from(raw)
    each_result(raw, function (tid, r)
        local name = r.spike_message and defines.SPIKES[r.spike_message];
        if (name == nil) then
            return;
        end
        local index = damage.index_of(tid);
        if (index == 0) then
            return;
        end
        local owner, is_pet = credited(tid, index);
        if (owner == nil) then
            return;
        end
        local src = is_pet and source(owner, 'pet', function () return { name = 'Pet', kind = 'pet' }; end)
            or source(owner, 'spk:' .. name, function () return { name = name, kind = 'spikes' }; end);
        add(src, 'Hit', r.spike_param);
    end);
end

--[[
* An incoming action packet: `data` is the packet as a string (to spot repeats), `raw` what
* ashita.bits reads (e.data_raw in game).
--]]
function damage.on_action(data, raw)
    if (repeated(data)) then
        return;
    end
    local function u(pos, len)
        return ashita.bits.unpack_be(raw, 0, pos, len);
    end
    local actor, category, id = u(40, 32), u(82, 4), u(86, 32);
    if (actor == 0 or category == 0) then
        return;
    end
    local index = damage.index_of(actor);
    if (index == 0) then
        return;
    end
    if (damage.debug) then
        damage.log(('Action Type: %d Action ID: %d'):fmt(category, id));
    end

    -- Someone we count, or a pet of theirs (whose damage is its owner's).
    local owner, is_pet = credited(actor, index);
    if (owner == nil) then
        if (actor >= 0x1000000) then
            spikes_from(raw); -- a mob attacking: its targets' spikes and counters
        end
        return;
    end
    if (not defines.PARSED[category] or id == 0) then
        return;
    end

    each_result(raw, function (_, r)
        if (damage.debug) then
            damage.log(('Animation: %d Param: %d Message: %d'):fmt(r.animation, r.param, r.message));
        end
        local label, dealt = classify(r.message);
        if (label ~= nil) then
            add(source_for(owner, is_pet, category, id, r.message, r.animation), label, dealt and r.param or 0);
        end
        if (r.add_message ~= nil and category ~= A.JA) then
            if (defines.ADD_EFFECT_DMG[r.add_message]) then
                add(source(owner, 'add', function () return { name = 'Additional Effect', kind = 'add' }; end), 'Hit', r.add_param);
            elseif (defines.SKILLCHAIN[r.add_message]) then
                add(source(owner, 'sc', function () return { name = 'Skillchain', kind = 'sc' }; end), defines.SKILLCHAIN[r.add_message], r.add_param);
            end
        end
    end);
end

--[[
* Totals.
--]]
function damage.source_total(src)
    local t = 0;
    for _, d in pairs(src.damage) do
        t = t + d.total;
    end
    return t;
end

function damage.source_count(src)
    local n = 0;
    for _, d in pairs(src.damage) do
        n = n + d.count;
    end
    return n;
end

-- A player's total; skillchains count only when /dps sc is 'player'.
function damage.total(e, sc)
    local t = 0;
    for _, src in pairs(e.sources) do
        if (src.kind ~= 'sc' or sc == 'player') then
            t = t + damage.source_total(src);
        end
    end
    return t;
end

-- Share of swings that didn't miss, as a percentage: weapon swings, weapon skills and job
-- abilities only (not spells, pets, skillchains, additional effects, spikes or counters).
function damage.hitrate(e)
    local count, missed = 0, 0;
    for _, src in pairs(e.sources) do
        if (src.kind == nil) then
            count = count + damage.source_count(src);
            missed = missed + (src.damage.Miss ~= nil and src.damage.Miss.count or 0);
        end
    end
    return count == 0 and 0 or 100 * (count - missed) / count;
end

function damage.reset()
    damage.entities, damage.index = { }, { };
end

return damage;
