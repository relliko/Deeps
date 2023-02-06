# Deeps for Ashita v4
Credit to kjLotus for the original version of Deeps that was the basis for this update.

Special thanks to Thorny for help with refactoring and general advice and tips on Ashita development!

Also thanks to ShiyoKozuki for helping test and helping me figure out some packet stuff.

Forked from https://git.ashitaxi.com/Plugins/Deeps

## Installation
- Download and extract the ```plugins``` and ```resources``` directories from the [latest release zip](https://github.com/relliko/Deeps/releases/latest) directly into your base Ashita v4 directory.
- Type /load deeps in game

Note: The only necessarily required file is ```plugins/Deeps.dll```; if you don't like the bar texture you can leave out the resources folder and just have flat colored bars and the plugin will still work, although some jobs may have text that is difficult to read.


## Usage
You can type /dps or /deeps to show the available commands. 

Left clicking on a bar will show additional details about the damage dealt, right click to go back.

Shift clicking the background will allow you to reposition the window.


## New features
- Pet damage included in player's damage contribution
- Static colors for job bars
- Damage from outside of party or alliance can now be filtered out
- Overall hit rating displayed alongside damage done

## Known issues
- High jump displays as Avalanche and Jump displays as Gale axe

## Patch Notes

### v1.06
- Hopefully fixed pet damage ending up associated with the wrong owner after resummoning.
- Fixed colors being random when someone is /anon, now it is consistently blue.
- Spells no longer affect overall hit rate

### v1.05
- Fixed SMN blood pacts not being included in pet damage
- Added /dps tvmode to scale the size up by 50% so it's easier to look at on big screens.

### v1.04
- Fix for crash while clicking bars

### v1.03
- Stability fixes
- Improved visibility of DRK bars

### v1.02
- Pet damage now counts towards a player's total damage contribution. It should not affect the displayed overall hit rate
- Added a setting to toggle static job colors, typing /dps jobcolors will bring back randomized coloring for jobs
- Added a setting to display data from non-party members, toggle by typing /dps partyonly

## TODO (No guarantees)
- Config to exclude skillchain damage or display skillchains as their own category
- Log saving to disk
