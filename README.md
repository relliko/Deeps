# Deeps for Ashita v4
A damage meter addon for Ashita v4: one bar per player with their share of the damage, click a bar to see where the damage came from.

## Credits
Credit to kjLotus for the original version of Deeps that was the basis for this update.

Special thanks to Thorny for help with refactoring and general advice and tips on Ashita development!

Also thanks to ShiyoKozuki for helping test and helping me figure out some packet stuff.

Forked from https://git.ashitaxi.com/Plugins/Deeps

## Installation
- Download the zip from the [latest release](https://github.com/relliko/Deeps/releases/latest) and extract it into your Ashita v4 directory (it contains `addons/deeps`; if playing on Horizon this is the `Game\` directory). Or copy the `addons/deeps` directory from this repository into the `addons` directory of your Ashita v4 install.
- Type `/addon load deeps` in game.

If you used the old Deeps plugin, `/unload deeps` and remove it from your scripts first: they share the `/dps` and `/deeps` commands.

## Usage
Type `/dps` or `/deeps` to show the available commands.

- Left click a bar to see more about the damage dealt, right click to go back.
- Shift+drag the background to move the window.
- The - at the right of the title bar minimizes the meter to an icon at the bottom right of the screen; click the icon, or type `/dps show`, to bring it back. `/dps min` minimizes it too.
- `/dps sc player|bar|off` chooses where skillchain damage goes: into the closer's total, onto its own Skillchain bar, or nowhere. `/dps sc` on its own cycles through them.

## Features
- Pet damage included in the owner's damage contribution
- Static colors for job bars
- Damage from outside of party or alliance can be filtered out
- Overall hit rating displayed alongside damage done
- Spikes, counters, retaliation, skillchains and job ability damage all count

## Known issues
- The way crit percentage is displayed doesn't account for misses, so your crit rate will look lower than it actually is.

## Legacy plugin (deprecated)
Deeps started as a C++ plugin (v1.x). It is no longer maintained: the addon above replaces it and fixes its known issues. Its source and last build are kept in [`legacy/`](legacy) for reference, and the plugin downloads stay on the [v1.06 release](https://github.com/relliko/Deeps/releases/tag/v1.06).

## Patch Notes

### v2.2.1
- The meter can no longer get lost off the screen: if its title bar is off-screen (dragged out, or saved at a bigger resolution), it's pulled back on and the new position is saved.

### v2.2
- The Lua addon is now the only maintained version of Deeps. The old plugin is deprecated and its code moved to `legacy/`.

### v2.1.1
- The meter can be minimized to an icon at the bottom right of the screen, with the - in its title bar or `/dps min`. Click the icon or type `/dps show` to bring it back. It stays minimized through a reload.
- The icon sits in one row with the minimized windows of other addons that use the same tray (allrecipes, droptables).

### v2.0
- Deeps is now a Lua addon. It looks and works like the plugin: same bars and texture, click to open a bar, right click to go back, shift+drag to move.
- Spikes, counters and retaliation count for the player (or pet owner) who dealt them.
- Skillchain damage can have its own bar (`/dps sc bar`), and switching `/dps sc` no longer loses any numbers.
- Job ability damage counts (Jump, High Jump, Chi Blast, Quick Draw, Weapon Bash, Eagle Eye Shot...), and Jump and High Jump are named correctly instead of Gale Axe and Avalanche Axe.
- Fixed results being misread after an additional effect or spikes, which could lose the rest of an attack round.
- Fixed an unrecognized message dropping the rest of a packet.
- Fixed a Daken shuriken turning the rest of the round's swings into ranged attacks.
- Fixed long action ids being cut to 10 bits.
- Additional effect procs no longer raise the hit rate.
- A pet's damage counts even before its owner does anything, and follows its new owner after a resummon.
- `/dps report [s/p/l] [#]` sends exactly # bars (4 by default); without s/p/l it only prints them for you.
- `/dps tvmode` no longer resets the numbers.
- Every setting is saved: position, TV mode, job colors, party only and skillchains.

### v1.06 (plugin)
- Added a configuration setting (`/dps sc`) to disable skillchains counting towards a player's damage contribution.
- Hopefully fixed pet damage ending up associated with the wrong owner after resummoning.
- Fixed colors being random when someone is /anon, now it is consistently blue.
- Spells no longer affect overall hit rate

### v1.05 (plugin)
- Fixed SMN blood pacts not being included in pet damage
- Added /dps tvmode to scale the size up by 50% so it's easier to look at on big screens.

### v1.04 (plugin)
- Fix for crash while clicking bars

### v1.03 (plugin)
- Stability fixes
- Improved visibility of DRK bars

### v1.02 (plugin)
- Pet damage now counts towards a player's total damage contribution. It should not affect the displayed overall hit rate
- Added a setting to toggle static job colors, typing /dps jobcolors will bring back randomized coloring for jobs
- Added a setting to display data from non-party members, toggle by typing /dps partyonly

## TODO (No guarantees)
- Log saving to disk
- Setting to reset on every kill
