# Cosmos Quest PvE Instance Solver

## Summary
I took over the developement out of interest and educational purposes. Well, I also needed a good calculator and David's approach was a good place to start.
My goal is to provide a Calculator that is usable even on difficult problems in respect to memory usage and calculation time.

Game forum and disscusuion thread [here](https://www.kongregate.com/forums/910715-cosmos-quest/topics/948525-version-2-7-6-new-quests-and-heroes-added-to-c-calc).

Other Repos that might be interesting:
* [Latas' UI for automatic generation of macro files](https://github.com/Wiedmolol/CQMacroCreator)
* [Malizia's version of the calc](https://github.com/Maliziacat/C-Hero-Calc)

### Features
* C++ based Calcultor for solving PvE instances
* Heroes are implemented, levelable and abilities are fully functional
* Hero Levels will be saved between executions
* Completely controllable via Command Line
* All quests predefined and accessable
* Don't know how to input heroes, monster or just generally confused how to use this calculator? Type `help` when asked for input to get... well, help
* Macro your input by using external files
* Specify multiple quests and let it work on them uninterupted
* Outputs a battle string that can be used to view the battle ingame
* Precompiled exe included (Built on Windows 8.1 (64Bit), no guarantee for other operating systems)

### What's New?



## Usage

For most people, just downloading the exe and running it will be enough. For those who are not on Windows they will need to download all files and compile for themselves.

### Compiling
Personally I get it to compile by running:
`g++ -std=c++11 -O3 -o CosmosQuest main.cpp cosmosClasses.cpp inputProcessing.cpp cosmosDefines.cpp battleLogic.cpp base64.cpp` from the command line.

**Makefile**: Base Makefile provided by BugsyLansky.

### Macro Files

What are they?
Macro files are the future!
* Macro files are nothing else than your input in text file form.
* Usually n-th line answers the n-th query the program asks.
* When the last line of the marco file is used, the program switches back to manual input.
* A `//` makes the programm ignore everything that comes after it.

If this is too much for you please refer to Latas' UI that will automatically generate macro files and run the calc for you.
If you want to learn, here is a [Imgur Album](https://imgur.com/a/CXy4A) explaning how to make your own macro files.

How to use them?
1. Start the program normally. It will take the `default.cqinput`-file as input
2. Start the program via command line like: `CosmosQuest.exe configFile` instead of just `CosmosQuest.exe` and the program will read everything from the file you specified. 
3. Compile yourself and add your own default macro file name. This will stop you having to start the program via command line.

### Input via command line
Input via command line is now mostly unavailable. Compiling yourself or or removing `defalut.cqinput` from the folder will still give you access to it though.

### Control Variables
* `firstDominace` This controls at which army length the calc should start removing suboptimal solutions. Setting this higher _might_ improve the solution. But treat this with extreme caution as it can cause your PC run out of RAM rather quickly.
* `macroFileName` Path to your default macro file
* `useDefaultMacroFile` Whether you want to always use the specified macro file or not
* `showMacroFileInput` If enabled will hide any input promts that are answered by a macro file 
* `showBattleReplayString` If disabled will hide the replay strings after every solution

**If you want to use change any of those values you have to compile the program yourself!**

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
Having only 15 or 20 Heroes is totally fine but if you enable all of them, your machine will probably run out of RAM.

### Bugs that I'm aware of
* It is theoretically possible that healing might revive dead monsters in the backline. 
But only if the AOE Damage source dies and the heal is strong enough. Should basically never happen (Famous last words)

### Potential Errors:
**bad_alloc**: You get this error when the program tries to use more RAM than your computer has available. 
There is no fix available for this currently, but I work hard to try and reduce the general RAM usage.

**The programm outputs: "FATAL ERROR"**: If this happens you should leave a comment in the forums because this is not normal behaviour. 
