# Cosmos Quest PvE Instance Solver

### More information
The old readme is available by clicking OLDREADME.md above. In this one, I'll only list changes, so you may want to read the old one after this one if you're not familiar with the program.

### Looking at forked versions and other resources
This is the final version I'm releasing. For help finding most updated forks, you can try [here](https://github.com/nls0/C-Hero-Calc/network).

Last discussion thread I made [here](http://www.kongregate.com/forums/910715-cosmos-quest/topics/1683470-calculator-v3-0-2-0g). You might be able to find a more updated thread for a newer fork at some point.

* [Alternative Input throgh Latas' Graphical User Interface](https://github.com/Wiedmolol/CQMacroCreator)
* [Battle Specification and Hero Skill Explanations](https://www.kongregate.com/forums/910715-cosmos-quest/topics/959359-detailed-explanations-of-the-hero-abilities) (somewhat out-of-date, imperfect)

### Files you may want to use (or just download everything)
CosmosQuest.exe  
default.cqconfig  
cqdata.txt

### Important changes/notes
The goal of the calculator is to provide accurate calculations, but the game code currently has several [disparities](https://www.kongregate.com/forums/910715-cosmos-quest/topics/1678312-battle-oddities) where one side is treated differently than the other. The calculator currently assumes both sides are treated the same and like the left side. It would be difficult to make two sets of rules for the calculator.

The game replays currently show different damage scores compared to what it internally calculates. For example, hitting a fire enemy with a water unit of 79 attack will result in 119 damage, but only 118.5 damage score. Other things to look at are ricochet damage and aoe damage. It is possible to fix this by looking at the game code, but I won't be doing it.

When using CQMacroCreator with send checked, your cqconfig file will not be used. It will still be used if you use the macrocreator to open up a console window. (at time of writing)

Multithreading has been added. Default number of threads is 6. You can change this in default.cqconfig.

The calculator now stops on first solution found by default. You have to change the default.cqconfig option STOP\_FIRST\_SOLUTION to false if you want to use the calculator to find the cheapest solution (e.g. when looking for how many more followers you need to complete a quest).

Gambler heroes (Dicemaster, Luxurius Maximus, Pokerface) are working. The way they work is by considering enemy lineup and order including empty space. For the enemy, calc assumes empty space is in very front. If you get a solution with fewer than 6 units against a lineup with an enemy gambler, you should also put any empty space in the very front. Theoretically it would be possible to look for solutions with alternate empty space placements, but the calc doesn't currently do it.

The calculator begins depth first searching at max size - 2. This significantly reduces runtime (when a solution exists) and memory usage. You should be able to use more units now, but you should still avoid using bad units, particularly very cheap monsters.

Dominance checks have been removed. This was necessary as this was resulting in a possibility of failing to find a solution where it existed. It was somewhat arbitrarily pruning possibilities providing a false speed-up.

Monsters, heroes, hero aliases, and quests can be added and changed by modifying the cqdata.txt file. Adding new heroes in the data file is only possible if they use an existing ability. For the data file, don't use any empty lines, place new entries at the bottom of the section (order is important). For quests, make sure there is a space followed by a + at the end. If the file is not present, hardcoded data will be used.

### Compiling
You can build it in one command with:  
g++ -Ofast -std=c++11 -o CosmosQuest main.cpp inputProcessing.cpp cosmosData.cpp battleLogic.cpp base64.cpp -s -static -static-libstdc++ -static-libgcc -pthread

The .exe included was built on Windows 10 (64Bit) using [MingW-W64-builds](http://mingw-w64.org/doku.php/download) x86\_64-8.1.0-posix-seh-rt_v6-rev0.

### Hero aliases
Open up cqdata.txt to see them. You can also add your own.