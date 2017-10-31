# Cosmos Quest PvE Instance Solver

## Summary
I took over the developement out of interest and educational purposes. Well, I also needed a good calculator and David's approach was a good place to start.
My goal is to provide a Calculator that is usable even on difficult problems in respect to memory usage and calculation time.

Game forum and disscusuion thread [here](http://www.kongregate.com/forums/910715-cosmos-quest/topics/933801-version-2-7-3-who-is-ready-for-some-controversial-changes).

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
**Controversional Changes Everybody!**
* **Hero Files do no longer exist!**
* Hero input is now done like you would in a lineup (f.e. `jackolantern:53`)
* Enter a hero, press enter, continue until you have all heroes entered, then press enter twice or type `done`
* As this can get tiring, I reccomend everyone to either use macrofiles or use Latas' UI (Links further up)
* Per default, the program now takes the `default.cqinput` file as input. If you want to input data manually you will have to remove that file from the folder. If you want to use a differntly named file you need to start the program via command line and pass the name as an argument.

Why did I do that? Hero files are messy, they can't be read easily, deprecate whenever new heroes come out and really only have been a relic from when i took up development of this tool.

Marco files on the other hand are straight forward and can be edited by anyone just by looking at them. Completely switching usage to them also cleans up my input handling and can potentially be verified.

They also don't get overwritten everytime you input other heroes. You can just have multiple files and switch between them. Running a different lineup is as easy as changing a few characters in a text editor and then starting the program again.

This should also settle debates about default answers to promts.

In case you haven't noticed by the way I talk about this. **I won't go back on that change!** So don't even bother asking.

What else?
* Halloween Heroes are added.
* After a calculation concludes, a battle replay string is printed. This string can be copied and viewed ingame just like a tournament replay. Please add this string to any report of a faulty solution as it helps me to compare ingame results with my own.
* Lots and Lots of code changes under the hood. From upholding of conventions to modern practices. Mostly explicit type casts to fix comiler warnings though.  

## Usage

For most people, only downloading the exe and running it will be enough. For those who are not on Windows they will need to download all files and compile for themselves.

### Compiling
Personally I get it to compile by running:
`g++ -std=c++11 -O3 -o CosmosQuest main.cpp cosmosClasses.cpp inputProcessing.cpp cosmosDefines.cpp battleLogic.cpp base64.cpp` from the command line.

**Makefile**: For those who know how to use them, I added a Makefile that BugsyLansky provided.

### Macro Files
Macro files are the future!

What are they?
* Macro files are nothing else than your input in text file form.
* The n-th line answers the n-th query the program asks.
* When the last line of the marco file is used, the program switches back to manual input.
* A `//` makes the programm ignore everything that comes after it.

If this is too much for you please refer to Latas' UI that will automatically generate macro files and run the calc for you.

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
Eventually I will release a low-RAM version that should remove most of these problems. 

**The programm outputs: "ERRORERRORERROR"**: If this happens you should leave a comment in the forums because this is not normal behaviour. 
