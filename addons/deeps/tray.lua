--[[
* tray - minimized windows as small icons in one row at the bottom right of the screen, shared by
* every addon that carries this file.
*
* An icon is as tall as the points addon's compact bar (16 of the game's menu pixels, scaled to
* its window the same way, libs/scaling) and twice as wide.
*
* No addon knows about any other. Each says which of its icons are showing, by name, with Ashita's
* plugin events (AshitaCore:GetPluginManager():RaiseEvent), and lays out the row from the names it
* hears: sorted, from the right. The events only go between the addons in your game; nothing is
* sent to the server. An icon not heard of for a few seconds (its addon unloaded or stopped) leaves
* the row. A new addon only needs this file; the others don't change. tray.status() says what an
* addon hears (/ar debug).
*
* tray.init(addon) at load; tray.icon(id, opts) each frame an icon shows, true when it's clicked;
* tray.hide(id) each frame it doesn't and at unload (it tells the others once).
* tray.title_button(...) draws the - in a window's title bar that minimizes it.
--]]

local bit   = require('bit');
local imgui = require('imgui');

local tray = {
    mine   = { },    -- [id] = { shown, said } for this addon's icons
    others = { },    -- [id] = when another addon's icon was last heard of (os.clock)
    live   = false,  -- this addon has heard one of the events (for tray.status)
    scale  = nil,    -- the game's window over its menu resolution, read once
};

local PREFIX = 'tray1|';  -- event names: tray1|here|<id>, tray1|gone|<id>, tray1|who
local EVERY  = 1;         -- seconds between announcements while an icon shows
local FORGET = 3.5;       -- seconds unheard before another addon's icon leaves the row
local H      = 16;        -- menu pixels: the height of the points addon's compact bar
local FLAGS  = bit.bor(ImGuiWindowFlags_NoDecoration, ImGuiWindowFlags_NoMove, ImGuiWindowFlags_NoSavedSettings,
    ImGuiWindowFlags_NoFocusOnAppearing, ImGuiWindowFlags_NoNav, ImGuiWindowFlags_NoBackground);

local function raise(what)
    pcall(function ()
        AshitaCore:GetPluginManager():RaiseEvent(PREFIX .. what, { 0 });
    end);
end

-- News of an icon (the plugin_event event; Ashita may pass it as a table or an object).
function tray.heard(e)
    local ok, name = pcall(function () return e.name; end);
    if (not ok or type(name) ~= 'string' or name:sub(1, #PREFIX) ~= PREFIX) then
        return;
    end
    tray.live = true;
    local what, id = name:sub(#PREFIX + 1):match('^(%a+)|?(.*)$');
    if (what == 'who') then
        for _, m in pairs(tray.mine) do
            m.said = nil;
        end
    elseif (id == nil or id == '') then
        return;
    elseif (tray.mine[id] ~= nil) then
        return;
    elseif (what == 'here') then
        tray.others[id] = os.clock();
    elseif (what == 'gone') then
        tray.others[id] = nil;
    end
end

function tray.init(addon)
    tray.mine, tray.others, tray.live = { }, { }, false;
    ashita.events.register('plugin_event', 'tray_' .. addon, tray.heard);
    raise('who');
end

-- How much bigger the game draws its menus than their own resolution (libs/scaling, as the
-- points addon reads it); 1 when that can't be read.
function tray.menu_scale()
    if (tray.scale == nil) then
        local ok, scaling = pcall(require, 'scaling');
        local k = ok and type(scaling) == 'table' and type(scaling.scaled) == 'table' and scaling.scaled.h or nil;
        tray.scale = (type(k) == 'number' and k > 0 and k < 10) and k or 1;
    end
    return tray.scale;
end

-- The icons in the row, in order from the right: this addon's showing and the others' heard of lately.
function tray.row(now)
    local row = { };
    for id, m in pairs(tray.mine) do
        if (m.shown) then
            row[#row + 1] = id;
        end
    end
    for id, heard in pairs(tray.others) do
        if (now - heard <= FORGET) then
            row[#row + 1] = id;
        else
            tray.others[id] = nil;
        end
    end
    table.sort(row);
    return row;
end

-- An icon's place in the row, 0 at the right.
function tray.slot(id, now)
    for i, other in ipairs(tray.row(now)) do
        if (other == id) then
            return i - 1;
        end
    end
    return 0;
end

-- What the row is, for a debug command.
function tray.status()
    local row = tray.row(os.clock());
    return string.format('Icon row (right to left): %s. %s', #row > 0 and table.concat(row, ', ') or 'empty',
        tray.live and 'The addons hear each other.' or 'No addon heard yet: if icons overlap, the addons can\'t hear each other.');
end

--[[
* Draws an icon in its place; true when it's clicked. opts: tip (the tooltip), panel, edge,
* hot_panel, hot_edge (colours, 0xAABBGGRR), glyph(dl, x, y, w, h) to draw on it.
--]]
function tray.icon(id, opts)
    local now = os.clock();
    local m = tray.mine[id];
    if (m == nil) then
        m = { shown = false, said = nil };
        tray.mine[id] = m;
    end
    if (not m.shown or m.said == nil or now - m.said >= EVERY) then
        m.shown, m.said = true, now;
        raise('here|' .. id);
    end
    local slot = tray.slot(id, now);
    local k = tray.menu_scale();
    local h = math.floor(H * k + 0.5);
    local w = h * 2;
    local gap = math.max(1, math.floor(2 * k + 0.5));
    local screen = imgui.GetIO().DisplaySize;
    imgui.SetNextWindowPos({ screen.x - (slot + 1) * w - slot * gap, math.floor(screen.y - (H + 1) * k) }, ImGuiCond_Always);
    imgui.SetNextWindowSize({ w, h }, ImGuiCond_Always);
    imgui.PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    imgui.PushStyleVar(ImGuiStyleVar_WindowMinSize, { 1, 1 });
    imgui.PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    local hit = false;
    if (imgui.Begin(string.format('%s icon###tray_%s', id, id), { true }, FLAGS)) then
        local x, y = imgui.GetWindowPos();
        hit = imgui.InvisibleButton('##tray_' .. id, { w, h });
        local hot = imgui.IsItemHovered();
        local dl = imgui.GetWindowDrawList();
        dl:AddRectFilled({ x, y }, { x + w, y + h }, hot and opts.hot_panel or opts.panel);
        dl:AddRect({ x, y }, { x + w, y + h }, hot and opts.hot_edge or opts.edge, 0, 0, 1);
        opts.glyph(dl, x, y, w, h);
        if (hot) then
            imgui.SetTooltip(opts.tip);
        end
    end
    imgui.End();
    imgui.PopStyleVar(3);
    return hit;
end

-- An icon isn't showing (its window is back, or hidden): tell the others, once.
function tray.hide(id)
    local m = tray.mine[id];
    if (m ~= nil and m.shown) then
        m.shown = false;
        raise('gone|' .. id);
    end
end

--[[
* The - that minimizes a window, in its title bar where ImGui puts the close button (the window
* is begun without one): FramePadding.x in from the right edge, a font size square, drawn like
* ImGui's own. Call it right after Begin with the window's position and width; true when clicked.
* cols: text, hover and held (0xAABBGGRR).
--]]
function tray.title_button(id, wx, wy, ww, cols)
    local st = imgui.GetStyle();
    local fs = imgui.GetFontSize();
    local x, y = wx + ww - st.FramePadding.x - fs, wy + st.FramePadding.y;
    local back_x, back_y = imgui.GetCursorScreenPos();
    imgui.PushClipRect({ wx, wy }, { wx + ww, wy + imgui.GetFrameHeight() }, false);
    imgui.SetCursorScreenPos({ x, y });
    local hit = imgui.InvisibleButton(id, { fs, fs });
    local dl = imgui.GetWindowDrawList();
    if (imgui.IsItemHovered()) then
        dl:AddRectFilled({ x, y }, { x + fs, y + fs }, imgui.IsItemActive() and cols.held or cols.hover);
        imgui.SetTooltip('Minimize to an icon at the bottom right');
    end
    local e = fs * 0.5 * 0.7071 - 1;
    local mx, my = x + fs / 2 - 0.5, y + fs / 2 - 0.5;
    dl:AddLine({ mx - e, my }, { mx + e, my }, cols.text, 1);
    imgui.PopClipRect();
    imgui.SetCursorScreenPos({ back_x, back_y });
    return hit;
end

return tray;
