# Cosmos Quest PvE Instance Solver

## Summary
I took over the developement out of interest and educational purposes. Well, I also needed a good calculator and David's approach was a good place to start.
My goal is to provide a Calculator that is usable even on difficult problems in respect to Memory usage and calculation time.

Game forum and disscusuion thread [here](http://www.kongregate.com/forums/910715-cosmos-quest/topics/915430-version-2-7-0-c-calc-now-supports-the-banhammer-overlord).

### Features
* C++ based Calcultor for solving PvE instances
* Heroes are implemented, levelable and abilities are fully functional
* Hero Levels will be saved between executions
* Completely controllable via Command Line
* All quests predefined and accessable
* Don't know how to input heroes, monster or just generally confused how to use this calculator? Type `help` when asked for input to get... well, help
* Macro your input by using external files
* Specify multiple quests and let it work on them uninterupted
* Precompiled exe included (Built on Windows 8.1 (64Bit), no guarantee for other versions)

### What's New?
**The Great In- and Output Overhaul + 2.7.1 Heroes**
* Maybe most importantly: **The Program has gotten significantly faster again.** Mostly due to a new flag when compiling... 
* Input via script is no longer possible. Instead it's now possible to specify a default macro file.
* There is also the option to hide queries if they are answered by a macro file
* There are now "true comments" in the macro files. Anything behind a double slash(`//`) is ignored.
* There is no option to specify maximum allowed monsters anymore. Instead it defaults to 6. However,
* Quests have a new input format which is: `quest##-#`. For example: `quest50-3` solves quest 50 with 4 monsters, `quest1-1` solves quest 1 with 6 monsters 
* Output for sizes 4 and lower has been reduced. What's more the program now asks about continuing only after it completed size 4 as the RAM usage only really picks up after that.
* General order of input has changed!! Make sure to compare your macro files with the new default one, if you're using them.
* And Obviously the new heroes are implemented

And finally:
* The calc can now calculate multiple instances one after another without stopping.
* To achieve that behaviour enter lineups separated by a space
* Example: `quest10-3 quest11-3 a11,e14 quest12-2`
* This can be used to go through multiple quests at once without having to input everything a gazillion times
* Note: You can't change maximum or minimum followers inbetween either. A restart is required for that.

## Usage

For most people, only downloading the exe and running it will be enough. For those who are not on Windows they will need to download all files and compile for themselves.

### Compiling
Personally I get it to compile by running:
`g++ -std=c++11 -O2 -o CosmosQuest main.cpp cosmosClasses.cpp inputProcessing.cpp cosmosDefines.cpp battleLogic.cpp` from the command line.

**Makefile**: For those who know how to use them, I added a Makefile that BugsyLansky provided.

### Input via command line
If you use the exe then the programm will take you through the steps you need to take to start calculating. Access the quests by typing `questX-Y` (X being the quest number and Y the difficulty (1-3)) when asked for a lineup. All other questions can be answered by typing `help` at any point.

### Control Variables (Input via Code)
Only a few control variable remain:
* `firstDominace` This controls at which army length the calc should start removing suboptimal solutions. Setting this higher *might* improve the solution provided but no guarantees. Also treat this with extreme caution as it can cause your PC run out of RAM.
* `macroFileName` Path to your default macro file
* `useDefaultMacroFile` Whether you want to always use the specified macro file or not
* `showMacroFileInput` If enabled will hide any input promts that are answered by a macro file 

**If you want to use this method you have to compile the program yourself!**

### Using a Macro file to automate your input
Always inputting the same data can get quite tiring. So I added a way to read your input from a file that you can specify. 

In short: When running from the command line you can say: 

`CosmosQuest.exe configFile` instead of just `CosmosQuest.exe` and the program will read everything from the file you specified and treat it as if you input it yourself. 

Alternatively you can specify a default macro file in the code and make the progam always use that. You have to compile yourself in that case

This can be used in many different ways but most importantly it's an easy way to switch between hero setups, and level heroes individually. 
It should also be a great help for people who are helping others in the chat, by enabling them to have files for everyone that they are helping without destroying their own data.

An example file (`default.cqinput`) is included and should be self-explanatory.

## Bugs, Warnings and other problems

### Regarding non-optimal solutions
I' feel like i need to put this up here because as many of you noticed, there are some situation where this calculator does in fact not give the optimal solution. 
The reason for this is that old optimization code back from when this program was designed for only normal monsters is still in there and has bugs related to hero abilities. However this code is **absolutely vital** to keep the runtime of the program in managable regions. 

On the user end, changing what heroes are enabled, what level they have, etc. can all affect the outcome of the calculation. Even small changes that I make in the code can be the reason why on one day it might suddenly give a worse or better solution than before. 
This is undesirable but will require heavy improvements on other ends to fix. Currently I'm working on finding a way to determine whether any one Monster or Hero is useless for solving a certain instance, that is, there is a 0% chance of that Monster being used in the solution. 
This is not as easy as it sounds though because Hero abilities can seriously mess with the value of a Monster.

### Regarding RAM usage
The RAM usage as well as computation time heavily scales with available Heroes and Monsters. 
So I reccomend disabling as many Heroes as possible and setting an appropriate lower limit on Monster Cost. 
If you calculate a DQ20 you probably wont need those Lv.1 Commons or anything cheaper than t5. 
Having only 10 Heroes is totally fine but if you enable all of them, your machine will probably run out of RAM.

### Bugs that I'm aware of
* It is theoretically possible that healing might revive dead monsters in the backline. 
But only if the AOE Damage source dies and the heal is strong enough. Should basically never happen (Famous last words)

### Potential Errors:
**bad_alloc**: You get this error when the program tries to use more RAM than your computer has available. 
There is no fix available for this currently, but I work hard to try and reduce the general RAM usage. 
Eventually I will release a low-RAM version that should remove most of these problems. 

**The programm outputs: "ERRORERRORERROR"**: If this happens you should leave a comment cause this is not normal behaviour. 
