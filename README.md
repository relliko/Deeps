# Deeps for Ashita v4
Credit to kjLotus for the original version of Deeps that was the basis for this update.

Special thanks to Thorny for help with refactoring and general advice and tips on Ashita development!

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
- Static colors for job bars
- Damage from outside of party or alliance is now filtered out.
- Overall hit rating displayed alongside damage done


## Known issues
- Sometimes clicking the GUI can cause client crashes, recommended to avoid doing it during important encounters.


## TODO
- Add hit % to the report function
- Need to assign colors to some newer jobs
- Toggle to display damage from people outside of party/alliances
- Include pet damage
