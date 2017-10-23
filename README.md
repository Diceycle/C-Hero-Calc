# Cosmos Quest PvE Instance Solver

## Summary
I took over the developement out of interest and educational purposes. Well, I also needed a good calculator and David's approach was a good place to start.
My goal is to provide a Calculator that is usable even on difficult problems in respect to memory usage and calculation time.

Game forum and disscusuion thread [here](http://www.kongregate.com/forums/910715-cosmos-quest/topics/928830-version-2-7-2-new-c-calc-grab-em-while-theyre-ho-never-mind-theyre-cold-already).

### Features
* C++ based Calcultor for solving PvE instances
* Heroes are implemented, levelable and abilities are fully functional
* Hero Levels will be saved between executions
* Completely controllable via Command Line
* All quests predefined and accessable
* Don't know how to input heroes, monster or just generally confused how to use this calculator? Type `help` when asked for input to get... well, help
* Macro your input by using external files
* Specify multiple quests and let it work on them uninterupted
* Precompiled exe included (Built on Windows 8.1 (64Bit), no guarantee for other operating systems)

### What's New?
**Code Quality Overhaul, Season Reward Heroes**
* Season Reward Heroes are now functional (There are some interactions that I'm not sure about but at least in DQs they shouldn't happen)
* battleLogic Code reworked to use more functions. This slows the program down marginally but makes the code MUCH more maintainable
* Program now stops automatically if it encounters a solution with 0 Followers
* **Rounding Errors have been fixed. So, to my knowledge there should be no wrong solutions anymore.** If you find one please leave a comment in the forum thread
* The provided exe is now compilef with `-Ofast`. I'm told this can have detremental effects in some cases. If you notice _significant_ slowdown in the new version leave a comment as well

* Various Small to medium code optimizations / compiler instructions. It adds up and probably saves a few seconds here and there
* Improve Code Quality in regards to namespaces, convetions and documentation

## Usage

For most people, only downloading the exe and running it will be enough. For those who are not on Windows they will need to download all files and compile for themselves.

### Compiling
Personally I get it to compile by running:
`g++ -std=c++11 -Ofast -o CosmosQuest main.cpp cosmosClasses.cpp inputProcessing.cpp cosmosDefines.cpp battleLogic.cpp` from the command line.

**Makefile**: For those who know how to use them, I added a Makefile that BugsyLansky provided.

### Input via command line
If you use the exe then the programm will take you through the steps you need to take to start calculating. 
Access the quests by typing `questX-Y` (X being the quest number and Y the difficulty (1-3)) when asked for a lineup. 
All other questions can be answered by typing `help` at any point.

### Control Variables (Input via Code)
Only a few control variable remain:
* `firstDominace` This controls at which army length the calc should start removing suboptimal solutions. Setting this higher _might_ improve the solution. But treat this with extreme caution as it can cause your PC run out of RAM rather quickly.
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
The reason for this is that old optimization code back from when this program was designed for only normal monsters is still in there and has bugs related to hero abilities. 
However this code is **absolutely vital** to keep the runtime of the program in managable regions. 

On the user end, changing what heroes are enabled, what level they have, etc. can all affect the outcome of the calculation. 
Even small changes that I make in the code can be the reason why on one day it might suddenly give a worse or better solution than before. 
This is undesirable but will require heavy improvements on other ends or a completely new approach to fix. 

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

**The programm outputs: "ERRORERRORERROR"**: If this happens you should leave a comment in the forums because this is not normal behaviour. 
