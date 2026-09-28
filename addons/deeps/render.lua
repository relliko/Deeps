--[[
* Deeps - render
* The meter: a background font with a title, and one font per bar, drawn by Ashita like the
* plugin's. Three views: everyone's damage, one player's sources, and one source's results.
* Left click a bar to open it, right click to go back, shift+drag to move it.
--]]

require('common');
local ffi     = require('ffi');
local fonts   = require('fonts');
local defines = require('defines');
local damage  = require('damage');

pcall(ffi.cdef, [[
    int16_t GetKeyState(int32_t vkey);
]]);

local D = defines;

local render = {
    s       = nil,   -- settings: x, y, tvmode, jobcolors, partyonly, sc, maxbars
    bg      = nil,   -- the background font (title)
    bars    = { },   -- bar fonts, top to bottom
    rows    = { },   -- what each bar is: { id = entity id or 'sc', key = source key or closer id }
    char    = nil,   -- the opened bar: an entity id, or 'sc' for the Skillchains bar
    source  = nil,   -- the opened source: a source key (or a closer's id under Skillchains)
    drag    = false,
    last_x  = 0,
    last_y  = 0,
    last    = -1,    -- when the bars were last updated
    moved   = false, -- the position changed and should be saved
    texture = nil,   -- bar.tga
    now     = os.clock,
    shift   = function () return bit.band(ffi.C.GetKeyState(0x10), 0x8000) ~= 0; end,
};

local UPDATE_EVERY = 0.1;

local function scale()
    return render.s.tvmode and 1.5 or 1;
end

--[[
* Creates the background font; bars are made as needed.
--]]
function render.init(s, texture)
    render.s, render.texture = s, texture;
    local k = scale();
    render.bg = fonts.new({
        visible     = true,
        font_family = 'Arial',
        font_height = D.TITLE_FONT_HEIGHT * k,
        auto_resize = false,
        color       = 0xFFFFFFFF,
        bold        = false,
        text        = '',
        position_x  = s.x,
        position_y  = s.y,
        background  = {
            visible   = true,
            color     = D.argb(0xCC, 0x00, 0x00, 0x00),
            width     = D.WINDOW_WIDTH * k,
            height    = D.TITLEBAR_HEIGHT * k,
            can_focus = false,
        },
    });
end

function render.release()
    while (#render.bars > 0) do
        table.remove(render.bars):destroy();
    end
    if (render.bg ~= nil) then
        render.bg:destroy();
        render.bg = nil;
    end
end

--[[
* Makes or removes bars until there are n (RepairBars). Each bar hangs under the one above it.
--]]
function render.repair(n)
    local k = scale();
    while (#render.bars < n) do
        local bar = fonts.new({
            visible     = true,
            font_family = 'Arial',
            auto_resize = false,
            color       = 0xFFFFFFFF,
            can_focus   = false,
            background  = { visible = true, color = D.argb(0xFF, 0x00, 0x7C, 0x5C) },
        });
        if (render.s.tvmode) then
            bar.create_flags = FontCreateFlags.Bold;
        end
        bar.font_height = D.BAR_FONT_HEIGHT * k;
        local bg = bar.background;
        if (render.texture ~= nil) then
            bg.texture = render.texture;
        end
        bg.width, bg.height = D.BAR_WIDTH * k, D.BAR_HEIGHT * k;
        if (#render.bars == 0) then
            bar.parent = render.bg;
            bar.position_x = D.BAR_HORIZONTAL_PADDING * k;
            bar.position_y = D.TITLEBAR_HEIGHT * k - k;
        else
            bar.parent = render.bars[#render.bars];
            bar.anchor_parent = FrameAnchor.BottomLeft;
            bar.position_x = 0;
            bar.position_y = D.BETWEEN_BAR_PADDING * k;
        end
        render.bars[#render.bars + 1] = bar;
    end
    while (#render.bars > n) do
        local bar = table.remove(render.bars);
        bar.parent = nil;
        bar:destroy();
    end
end

--[[
* Remakes the window at the current size and position (after /dps tvmode, or new settings).
--]]
function render.rebuild()
    render.repair(0);
    render.bg.font_height = D.TITLE_FONT_HEIGHT * scale();
    render.bg.position_x, render.bg.position_y = render.s.x, render.s.y;
    render.last = -1;
end

--[[
* The bar color for an entity: its job's color when job colors are on and it's in the party
* (/anon shows as job 0), otherwise its own random color.
--]]
function render.color(id, own)
    if (not render.s.jobcolors) then
        return own;
    end
    local party = AshitaCore:GetMemoryManager():GetParty();
    for i = 0, 17 do
        if (party:GetMemberServerId(i) == id) then
            return D.JOB_COLORS[party:GetMemberMainJob(i)] or own;
        end
    end
    return own;
end

--[[
* Whether an entity is shown: always, or only party and alliance members with /dps partyonly.
--]]
function render.shown(id)
    if (not render.s.partyonly) then
        return true;
    end
    local party = AshitaCore:GetMemoryManager():GetParty();
    for i = 0, 17 do
        if (party:GetMemberServerId(i) == id) then
            return true;
        end
    end
    return false;
end

local function by_total(a, b)
    if (a.total ~= b.total) then
        return a.total > b.total;
    end
    return a.name < b.name;
end

local function percent(part, whole)
    return whole == 0 and 0 or 100 * part / whole;
end

--[[
* Shows `list` ({ name, total, color, row, text }) as bars under `title`, the widest being the
* largest.
--]]
local function show(title, list)
    render.bg.text = title;
    render.repair(#list);
    render.rows = { };
    local top = list[1] ~= nil and list[1].total or 0;
    local k = scale();
    for i, item in ipairs(list) do
        local bar = render.bars[i];
        local bg = bar.background;
        bg.width = D.BAR_WIDTH * k * (top == 0 and 1 or item.total / top);
        bg.color = item.color;
        bar.text = item.text;
        render.rows[i] = item.row;
    end
end

-- The closers' skillchain damage (shown players only), for the Skillchains bar.
local function skillchains()
    local list, sum = { }, 0;
    for id, e in pairs(damage.entities) do
        local sc = e.sources.sc;
        if (sc ~= nil and render.shown(id)) then
            local t = damage.source_total(sc);
            if (t > 0) then
                list[#list + 1] = { name = e.name, total = t, id = id, source = sc };
                sum = sum + t;
            end
        end
    end
    table.sort(list, by_total);
    return list, sum;
end

-- Everyone's damage.
local function players_view()
    local s = render.s;
    local list, sum = { }, 0;
    for id, e in pairs(damage.entities) do
        local t = damage.total(e, s.sc);
        if (t ~= 0 and render.shown(id)) then
            list[#list + 1] = { name = e.name, total = t, e = e };
            sum = sum + t;
        end
    end
    if (s.sc == 'bar') then
        local _, t = skillchains();
        if (t > 0) then
            list[#list + 1] = { name = 'Skillchain', total = t, sc = true };
            sum = sum + t;
        end
    end
    table.sort(list, by_total);
    while (#list > s.maxbars) do
        table.remove(list);
    end
    for i, item in ipairs(list) do
        if (item.sc) then
            item.color = D.SKILLCHAIN_COLOR;
            item.row = { id = 'sc' };
            item.text = (' %d. %-10.10s %6d (%03.1f%%)\n'):fmt(i, item.name, item.total, percent(item.total, sum));
        else
            item.color = render.color(item.e.id, item.e.color);
            item.row = { id = item.e.id };
            item.text = (' %d. %-10.10s %6d (%03.1f%%)  -  Hit: %03.1f%% \n'):fmt(i, item.name, item.total,
                percent(item.total, sum), damage.hitrate(item.e));
        end
    end
    show(' Deeps - Damage Done', list);
end

-- One player's sources (or the closers, for the Skillchains bar).
local function sources_view()
    local list, sum, title, color = { }, 0, nil, nil;
    if (render.char == 'sc') then
        local closers;
        closers, sum = skillchains();
        for _, c in ipairs(closers) do
            list[#list + 1] = { name = c.name, total = c.total, row = { id = 'sc', key = c.id } };
        end
        title, color = ' Skillchains - Sources\n', D.SKILLCHAIN_COLOR;
    else
        local e = damage.entities[render.char];
        if (e == nil) then
            render.char = nil;
            return players_view();
        end
        for key, src in pairs(e.sources) do
            local t = damage.source_total(src);
            if (t ~= 0 and (src.kind ~= 'sc' or render.s.sc == 'player')) then
                list[#list + 1] = { name = src.name, total = t, row = { id = e.id, key = key } };
                sum = sum + t;
            end
        end
        title, color = (' %s - Sources\n'):fmt(e.name), render.color(e.id, e.color);
    end
    table.sort(list, by_total);
    while (#list > 15) do
        table.remove(list);
    end
    for i, item in ipairs(list) do
        item.color = color;
        item.text = (' %d. %-10.10s %6d (%03.1f%%)\n'):fmt(i, item.name, item.total, percent(item.total, sum));
    end
    show(title, list);
end

-- One source's results: hits, crits, misses... (or a closer's skillchains by chain).
local function results_view()
    local src, who, what, color;
    if (render.char == 'sc') then
        local e = damage.entities[render.source];
        src = e ~= nil and e.sources.sc or nil;
        who, what, color = 'Skillchains', e ~= nil and e.name or '', D.SKILLCHAIN_COLOR;
    else
        local e = damage.entities[render.char];
        src = e ~= nil and e.sources[render.source] or nil;
        who, what = e ~= nil and e.name or '', src ~= nil and src.name or '';
        color = e ~= nil and render.color(e.id, e.color) or 0;
    end
    if (src == nil) then
        render.source = nil;
        return sources_view();
    end
    local list, count = { }, 0;
    for label, d in pairs(src.damage) do
        if (d.count ~= 0) then
            list[#list + 1] = { name = label, total = d.count, d = d, row = { } };
            count = count + d.count;
        end
    end
    table.sort(list, by_total);
    while (#list > 15) do
        table.remove(list);
    end
    for _, item in ipairs(list) do
        local d = item.d;
        item.color = color;
        item.text = (' %-5sCnt:%4d  Avg:%5d  Max:%5d (%3.1f%%)\n'):fmt(item.name, d.count, math.floor(d.total / d.count),
            d.max, percent(d.count, count));
    end
    show((' %s - %s\n'):fmt(who, what), list);
end

--[[
* Updates the bars (at most every 0.1 s), from d3d_present.
--]]
function render.update()
    if (render.bg == nil) then
        return;
    end
    local t = render.now();
    if (render.last >= 0 and t - render.last <= UPDATE_EVERY) then
        return;
    end
    local k = scale();
    render.bg.font_height = D.TITLE_FONT_HEIGHT * k;
    local bgb = render.bg.background;
    bgb.width = D.WINDOW_WIDTH * k;
    if (render.char == nil) then
        players_view();
    elseif (render.source == nil) then
        sources_view();
    else
        results_view();
    end
    bgb.height = #render.bars * D.BAR_BACKGROUND_HEIGHT * k + D.TITLEBAR_HEIGHT * k;
    render.last = t;
end

-- The lines on screen, title first (for /dps report).
function render.lines()
    local out = { render.bg.text };
    for _, bar in ipairs(render.bars) do
        out[#out + 1] = bar.text;
    end
    for i, l in ipairs(out) do
        out[i] = l:gsub('\n', ''):gsub('^%s+', ''):gsub('%s+$', '');
    end
    return out;
end

local function hit_window(x, y)
    return render.bg.obj:GetBackground():HitTest(x, y);
end

-- Whether (x, y) is on a bar's row (bars span the window's width).
local function hit_bar(bar, y)
    local bg = bar.obj:GetBackground();
    local top = bg:GetPositionY();
    return y >= top and y < top + bg:GetHeight();
end

--[[
* Mouse input (WM_* message, x, y). Returns true to keep the click from the game: clicks on
* the meter, and the end of a drag.
--]]
function render.mouse(msg, x, y)
    if (render.bg == nil) then
        return false;
    end
    if (render.drag) then
        render.bg.position_x = render.bg.position_x + (x - render.last_x);
        render.bg.position_y = render.bg.position_y + (y - render.last_y);
        render.last_x, render.last_y = x, y;
        if (msg == 514 or not render.shift()) then
            render.drag = false;
            render.s.x, render.s.y = render.bg.position_x, render.bg.position_y;
            render.moved = true;
            return true;
        end
    end

    if (msg == 513 and render.shift() and hit_window(x, y)) then
        render.drag, render.last_x, render.last_y = true, x, y;
    end
    if (not hit_window(x, y)) then
        return false;
    end

    -- Left button up on a bar opens it.
    if (msg == 514 and not render.drag) then
        for i, bar in ipairs(render.bars) do
            if (hit_bar(bar, y)) then
                local row = render.rows[i];
                if (row ~= nil and render.char == nil) then
                    render.char = row.id;
                elseif (row ~= nil and render.source == nil and row.key ~= nil) then
                    render.source = row.key;
                end
                render.last = -1;
                return true;
            end
        end
    end

    -- Right button up goes back.
    if (msg == 517) then
        if (render.source ~= nil) then
            render.source = nil;
        else
            render.char = nil;
        end
        render.last = -1;
        return true;
    end

    return msg == 513 or msg == 516;
end

return render;
