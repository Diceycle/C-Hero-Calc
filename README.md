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
* Precompiled exe included (Built on Windows 8.1 (64Bit), no guarantee for other versions)

### What's New?
**New Heroes have been added**
* All heroes should be fully functional now

## Usage

For most people, only downloading the exe and running it will be enough. For those who are not on Windows they will need to download all files and compile for themselves.

### Compiling
Personally I get it to compile by running:
`g++ -std=c++11 -o CosmosQuest main.cpp cosmosClasses.cpp inputProcessing.cpp cosmosDefines.cpp battleLogic.cpp` from the command line.

**Makefile**: For those who know how to use them, I added a Makefile that BugsyLansky provided.

### Input via command line
If you use the exe then the programm will take you through the steps you need to take to start calculating. Access the quests by typing `questX` (X being the quest number) when asked for a lineup. All other questions can be answered by typing `help` at any point.

### Control Variables (Input via Code)
If you know your way around source code you can input your data directly into the file and avoid having to input it over and over again. Here are the values that can be adjusted, all located in the `main`-method in the `main.cpp` file.
* `firstDominace` This controls at which army length the calc should start removing suboptimal solutions. Setting this higher *might* improve the solution provided but no guarantees. Also treat this with extreme caution as it can cause your PC run out of RAM.
* `maxMonstersAllowed` controls how many monsters are allowed in a solution. Set this to 4 or 5 when you do quests X-2 and X-3.
* `minimumMonsterCost`: controls how expensive a monster has to be to be considered by the calc
* `follower UpperBound` lets the calc know when the soluitons get too expensive for you. It will not consider any solutions more expensive than this number.
* `stringLineup` defines against which lineup of monsters you want to fight against. Lineups are defined a bit above. You can make your own lineup by following the patterns. Just remember to give it a unique name. All quests are available as `quests[1]`, `quests[2]` and so on (Indices **start at 1** in this case)
* `yourHeroLevels`: Input your hero levels according to the names you see on the side. 

Flow Control
* `ignoreConsole`: If true, this skips the question asking which input mode should be used.
* `manualInput`: If true, all data is going to be asked via command line, otherwise the variables above are going to be used.
* `individual` lets you run individual battles if you set it to `true`. It will ask for the two lineups via command line.
* `debugOutput`: If false will remove most of the output regarding what the calculator is doing at the moment. 

**If you want to use this method you have to compile the program yourself!**

### Using a "Config File" to macro your input
Always inputting the same data can get quite tiring. So I added a way to read your input from a file that you can specify. 

In short: When running from the command line (You have to do this!) you can say: 

`CosmosQuest.exe configFile` instead of just `CosmosQuest.exe` and the program will read everything from the file you specified and treat it as if you input it yourself. 

This can be used in many different ways but most importantly it's an easy way to switch between hero setups, and level heroes individually. 
It should also be a great help for people who are helping others in the chat, by enabling them to have files for everyone that they are helping without destroying their own data.

An example file (`default.cqinput`) is included and should be self-explanatory. Only thing to note is that there are no actual comments, the program just ignores everything after a space.

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
