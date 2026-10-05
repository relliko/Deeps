--[[
* Deeps - damage meters for Ashita v4, as a Lua addon.
* Originally a plugin by kjLotus, updated by Relliko; ported from the Deeps plugin (v1.06).
*
* Reads action packets to add up everyone's damage, and draws it as bars. Left click a bar for
* where the damage came from, right click to go back, shift+drag to move it. The - in its title
* bar minimizes it to an icon in the tray at the bottom right (tray.lua); click that to bring it
* back. /dps for commands.
--]]

addon.name    = 'deeps';
addon.author  = 'Relliko, kjLotus';
addon.version = '2.2.1';
addon.desc    = 'Damage meters for Ashita v4.';
addon.link    = 'https://github.com/relliko/Deeps';

require('common');
local chat     = require('chat');
local settings = require('settings');
local damage   = require('damage');
local render   = require('render');
local tray     = require('tray');

local defaults = T{
    x         = 300,
    y         = 300,
    tvmode    = false,     -- 50% bigger, bold bars
    jobcolors = true,      -- bars in the party member's job color
    partyonly = true,      -- only party and alliance members
    sc        = 'player',  -- skillchain damage: 'player' (the closer's), 'bar' (its own bar) or 'off'
    maxbars   = 15,
    minimized = false,     -- shown as an icon at the bottom right (tray.lua) instead
};

-- The minimized meter's icon: three bars, longest first, in a teal like the bars'.
local ICON = {
    tip = 'Deeps: click to open',
    panel = 0xE0000000, edge = 0xC8808080, hot_panel = 0xF0202020, hot_edge = 0xFF8CBE28,
    glyph = function (dl, x, y, w, h)
        local x0, bh = x + w * 0.22, h * 0.14;
        for i, len in ipairs({ 0.56, 0.4, 0.26 }) do
            local top = y + h * (0.16 + 0.26 * (i - 1));
            dl:AddRectFilled({ x0, top }, { x0 + w * len, top + bh }, 0xFF8CBE28);
        end
    end,
};

local deeps = {
    settings  = nil,
    reporting = false, -- a /dps report is still being sent
};

local SC_MODES = { 'player', 'bar', 'off' };
local REPORT_GAP = 1.1; -- seconds between report lines

local function msg(text)
    print(chat.header('Deeps'):append(chat.message(text)));
end

local function err(text)
    print(chat.header('Deeps'):append(chat.error(text)));
end

local function help_line(cmd, text)
    print(chat.header('Deeps'):append(chat.color2(2, cmd)):append(chat.message(text)));
end

local function save()
    settings.save();
end

local function reset()
    damage.reset();
    render.char, render.source, render.last = nil, nil, -1;
end

local function help()
    err('Invalid command.');
    help_line('/dps reset', ' - Reset damage counters.');
    help_line('/dps report [s/p/l] [#]', ' - Report damage data to say, party, or linkshell (top # bars, 4 by default). With no s/p/l it only shows you.');
    help_line('/dps jobcolors', ' - Toggle job-based color coding.');
    help_line('/dps partyonly', ' - Toggle displaying data from non-party members.');
    help_line('/dps tvmode', ' - Scales Deeps up to a size that works better on large displays.');
    help_line('/dps sc [player|bar|off]', ' - Skillchain damage: counted for the closer, shown as its own bar, or left out.');
    help_line('/dps min', ' - Minimize the meter to an icon at the bottom right (so does the - in its title bar).');
    help_line('/dps show', ' - Bring the meter back (so does clicking its icon).');
end

local function minimize(on)
    deeps.settings.minimized = on;
    render.hide(on);
    save();
end

--[[
* /dps report [s|p|l] [n]: the title and the top n bars, one line every 1.1 s. Without s/p/l the
* lines are only printed for you; nothing is sent.
--]]
local function report(args)
    local mode, n = nil, 4;
    for i = 3, #args do
        local a = args[i]:lower();
        if (a:match('^%d+$')) then
            n = tonumber(a);
        elseif (a == 's' or a == 'p' or a == 'l') then
            mode = a;
        else
            err('Use /dps report [s/p/l] [#].');
            return;
        end
    end
    local lines = render.lines();
    while (#lines > n + 1) do
        table.remove(lines);
    end
    if (mode == nil) then
        for _, l in ipairs(lines) do
            msg(l);
        end
        return;
    end
    if (deeps.reporting) then
        err('Still sending the last report.');
        return;
    end
    deeps.reporting = true;
    for i, l in ipairs(lines) do
        ashita.tasks.once(REPORT_GAP * (i - 1), function ()
            AshitaCore:GetChatManager():QueueCommand(1, ('/%s %s'):fmt(mode, l));
            if (i == #lines) then
                deeps.reporting = false;
            end
        end);
    end
end

local function toggle(key, on, off)
    local s = deeps.settings;
    s[key] = not s[key];
    save();
    msg(s[key] and on or off);
end

ashita.events.register('load', 'deeps_load', function ()
    deeps.settings = settings.load(defaults);
    render.init(deeps.settings, addon.path .. 'bar.tga');
    tray.init('deeps');
end);

ashita.events.register('unload', 'deeps_unload', function ()
    tray.hide('deeps');
    save();
    render.release();
end);

settings.register('settings', 'deeps_settings_update', function (s)
    if (s ~= nil) then
        deeps.settings = s;
        render.s = s;
        if (render.bg ~= nil) then
            render.rebuild();
        end
    end
end);

ashita.events.register('command', 'deeps_command', function (e)
    local args = e.command:args();
    if (#args == 0) then
        return;
    end
    local cmd = args[1]:lower();
    if (cmd ~= '/dps' and cmd ~= '/deeps') then
        return;
    end
    e.blocked = true;
    local what = (args[2] or ''):lower();
    local s = deeps.settings;
    if (what == 'reset') then
        reset();
    elseif (what == 'report') then
        report(args);
    elseif (what == 'min') then
        minimize(true);
    elseif (what == 'show') then
        minimize(false);
    elseif (what == 'debug') then
        damage.debug = not damage.debug;
        msg(damage.debug and 'Debug Enabled' or 'Debug Disabled');
    elseif (what == 'jobcolors') then
        toggle('jobcolors', 'Job Colors Enabled', 'Job Colors Disabled');
        render.last = -1;
    elseif (what == 'partyonly') then
        toggle('partyonly', 'Party Only Enabled', 'Party Only Disabled');
        render.last = -1;
    elseif (what == 'tvmode') then
        toggle('tvmode', 'TV Mode Enabled', 'TV Mode Disabled');
        render.rebuild();
    elseif (what == 'sc') then
        local mode = (args[3] or ''):lower();
        if (mode == '') then
            for i, m in ipairs(SC_MODES) do
                if (m == s.sc) then
                    mode = SC_MODES[i % #SC_MODES + 1];
                end
            end
            if (mode == '') then
                mode = 'player';
            end
        elseif (mode ~= 'player' and mode ~= 'bar' and mode ~= 'off') then
            err('Use /dps sc player, bar or off.');
            return;
        end
        s.sc = mode;
        save();
        if (render.char == 'sc' and mode ~= 'bar') then
            render.char, render.source = nil, nil;
        end
        render.last = -1;
        msg(({ player = 'Skillchain damage counts for the closer.', bar = 'Skillchain damage has its own bar.',
            off = 'Skillchain damage is left out.' })[mode]);
    else
        help();
    end
end);

-- Read only: never blocks or changes a packet.
ashita.events.register('packet_in', 'deeps_packet_in', function (e)
    if (e.id == 0x028) then
        damage.on_action(e.data, e.data_raw);
    end
end);

ashita.events.register('d3d_present', 'deeps_present', function ()
    if (deeps.settings.minimized) then
        render.hide(true);
        if (tray.icon('deeps', ICON)) then
            minimize(false);
        end
        return;
    end
    tray.hide('deeps');
    render.hide(false);
    render.update();
    render.draw_min();
    if (render.minimize) then
        render.minimize = false;
        minimize(true);
    end
    if (render.moved) then
        render.moved = false;
        save();
    end
end);

ashita.events.register('mouse', 'deeps_mouse', function (e)
    if (render.mouse(e.message, e.x, e.y)) then
        e.blocked = true;
    end
end);
