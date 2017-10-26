#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <limits>

#include "inputProcessing.h"
#include "cosmosDefines.h"
#include "battleLogic.h"

using namespace std;

// Enum to control the amount of output generate
enum OutputLevel {
    NO_OUTPUT = 0,
    BASIC_OUTPUT = 1,
    DETAILED_OUTPUT = 2
};

// Define global variables used to track the best result
int32_t followerUpperBound;
Army best;

// Simulates fights with all armies against the target. Armies will contain Army objects with the results written in.
void simulateMultipleFights(vector<Army> & armies, Army target, int outputLevel) {
    bool newFound = false;
    size_t i = 0;
    size_t armyAmount = armies.size();
    
    for (i = 0; i < armyAmount; i++) {
        simulateFight(armies[i], target);
        if (!armies[i].lastFightData.rightWon) {  // left (our side) wins:
            if (armies[i].followerCost < followerUpperBound) {
                if (!newFound && outputLevel > BASIC_OUTPUT) {
                    cout << endl;
                }
                newFound = true;
                followerUpperBound = armies[i].followerCost;
                best = armies[i];
                debugOutput(time(NULL), "    " + best.toString(), outputLevel > BASIC_OUTPUT, false, true);
            }
        }
    }
    debugOutput(time(NULL), " ", newFound && outputLevel > BASIC_OUTPUT, false, false);
}

// Take the data from oldArmies and write all armies into newArmies with an additional monster at the end.
// Armies that are dominated are ignored.
void expand(vector<Army> & newPureArmies, vector<Army> & newHeroArmies, 
            vector<Army> & oldPureArmies, vector<Army> & oldHeroArmies, 
            size_t currentArmySize) {

    int remainingFollowers;
    size_t availableMonstersSize = availableMonsters.size();
    size_t availableHeroesSize = availableHeroes.size();
    vector<bool> usedHeroes; usedHeroes.resize(availableHeroesSize, false);
    size_t i, j, m;
    SkillType currentSkill;
    bool globalAbilityInfluence;
    
    for (i = 0; i < oldPureArmies.size(); i++) {
        if (!oldPureArmies[i].lastFightData.dominated) {
            remainingFollowers = followerUpperBound - oldPureArmies[i].followerCost;
            for (m = 0; m < availableMonstersSize && monsterReference[availableMonsters[m]].cost < remainingFollowers; m++) {
                newPureArmies.push_back(oldPureArmies[i]);
                newPureArmies.back().add(availableMonsters[m]);
                newPureArmies.back().lastFightData.valid = true;
            }
            for (m = 0; m < availableHeroesSize; m++) {
                currentSkill = monsterReference[availableHeroes[m]].skill.type;
                newHeroArmies.push_back(oldPureArmies[i]);
                newHeroArmies.back().add(availableHeroes[m]);
                newHeroArmies.back().lastFightData.valid = (currentSkill == P_AOE || currentSkill == FRIENDS || currentSkill == BERSERK || currentSkill == ADAPT); // These skills are self centered
            }
        }
    }
    
    for (i = 0; i < oldHeroArmies.size(); i++) {
        if (!oldHeroArmies[i].lastFightData.dominated) {
            globalAbilityInfluence = false;
            remainingFollowers = followerUpperBound - oldHeroArmies[i].followerCost;
            for (j = 0; j < currentArmySize; j++) {
                for (m = 0; m < availableHeroesSize; m++) {
                    if (oldHeroArmies[i].monsters[j] == availableHeroes[m]) {
                        currentSkill = monsterReference[oldHeroArmies[i].monsters[j]].skill.type;
                        globalAbilityInfluence |= (currentSkill == FRIENDS || currentSkill == RAINBOW);
                        usedHeroes[m] = true;
                        break;
                    }
                }
            }
            for (m = 0; m < availableMonstersSize && monsterReference[availableMonsters[m]].cost < remainingFollowers; m++) {
                newHeroArmies.push_back(oldHeroArmies[i]);
                newHeroArmies.back().add(availableMonsters[m]);
                newHeroArmies.back().lastFightData.valid = !globalAbilityInfluence;
            }
            for (m = 0; m < availableHeroesSize; m++) {
                if (!usedHeroes[m]) {
                    currentSkill = monsterReference[availableHeroes[m]].skill.type;
                    newHeroArmies.push_back(oldHeroArmies[i]);
                    newHeroArmies.back().add(availableHeroes[m]);
                    newHeroArmies.back().lastFightData.valid = (currentSkill == P_AOE || currentSkill == FRIENDS || currentSkill == BERSERK || currentSkill == ADAPT); // These skills are self centered
                }
                usedHeroes[m] = false;
            }
        }
    }
}

// Use a greedy method to get a first upper bound on follower cost for the solution
// Greedy approach for 4 or less monsters is obsolete, as bruteforce is still fast enough
void getQuickSolutions(Instance instance, int outputLevel) {
    Army tempArmy = Army();
    vector<int8_t> greedy {};
    vector<int8_t> greedyHeroes {};
    vector<int8_t> greedyTemp {};
    bool invalid = false;
    
    debugOutput(time(NULL), "Trying to find solutions greedily...", outputLevel > BASIC_OUTPUT, false, true);
    
    // Create Army that kills as many monsters as the army is big
    if (instance.targetSize <= instance.maxCombatants) {
        for (size_t i = 0; i < instance.maxCombatants; i++) {
            for (size_t m = 0; m < availableMonsters.size(); m++) {
                tempArmy = Army(greedy);
                tempArmy.add(availableMonsters[m]);
                simulateFight(tempArmy, instance.target);
                if (!tempArmy.lastFightData.rightWon || (tempArmy.lastFightData.monstersLost > (int) i && i+1 < instance.maxCombatants)) { // the last monster has to win the encounter
                    greedy.push_back(availableMonsters[m]);
                    break;
                }
            }
            invalid = greedy.size() < instance.maxCombatants && tempArmy.followerCost <= followerUpperBound;
        }
        if (!invalid) {
            best = tempArmy;
            if (followerUpperBound > tempArmy.followerCost) {
                followerUpperBound = tempArmy.followerCost;
            }
            
            // Try to replace monsters in the setup with heroes to save followers
            greedyHeroes = greedy;
            for (size_t m = 0; m < availableHeroes.size(); m++) {
                for (size_t i = 0; i < greedyHeroes.size(); i++) {
                    greedyTemp = greedyHeroes;
                    greedyTemp[i] = availableHeroes[m];
                    tempArmy = Army(greedyTemp);
                    simulateFight(tempArmy, instance.target);
                    if (!tempArmy.lastFightData.rightWon) { // Setup still needs to win
                        greedyHeroes = greedyTemp;
                        break;
                    }
                }
            }
            tempArmy = Army(greedyHeroes);
            best = tempArmy;
            if (followerUpperBound > tempArmy.followerCost) { // Take care not to override custom follower counts
                followerUpperBound = tempArmy.followerCost;
            }
        }
    }
}

// Main method for solving an instance. Returns time taken to calculate in seconds
int solveInstance(Instance instance, size_t firstDominance, int outputLevel) {
    Army tempArmy = Army();
    int startTime;
    int tempTime;
    
    size_t i, j, sj, si;

    // Get first Upper limit on followers
    if (instance.maxCombatants > ARMY_MAX_BRUTEFORCEABLE_SIZE) {
        getQuickSolutions(instance, outputLevel);
    }
    
    vector<Army> pureMonsterArmies {}; // initialize with all monsters
    vector<Army> heroMonsterArmies {}; // initialize with all heroes
    for (i = 0; i < availableMonsters.size(); i++) {
        if (monsterReference[availableMonsters[i]].cost <= followerUpperBound) {
            pureMonsterArmies.push_back(Army( {availableMonsters[i]} ));
        }
    }
    for (i = 0; i < availableHeroes.size(); i++) { // Ignore chacking for Hero Cost
        heroMonsterArmies.push_back(Army( {availableHeroes[i]} ));
    }
    
    // Check if a single monster can beat the last two monsters of the target. If not, solutions that can only beat n-2 monsters need not be expanded later
    bool optimizable = (instance.targetSize > ARMY_MAX_BRUTEFORCEABLE_SIZE && instance.targetSize > 3);
    if (optimizable) {
        tempArmy = Army({instance.target.monsters[instance.targetSize - 2], instance.target.monsters[instance.targetSize - 1]}); // Make an army from the last two monsters
    }
    
    if (optimizable) { // Check with normal Mobs
        for (i = 0; i < pureMonsterArmies.size(); i++) {
            simulateFight(pureMonsterArmies[i], tempArmy);
            if (!pureMonsterArmies[i].lastFightData.rightWon) { // Monster won the fight
                optimizable = false;
                break;
            }
        }
    }

    if (optimizable) { // Check with Heroes
        for (i = 0; i < heroMonsterArmies.size(); i++) {
            simulateFight(heroMonsterArmies[i], tempArmy);
            if (!heroMonsterArmies[i].lastFightData.rightWon) { // Hero won the fight
                optimizable = false;
                break;
            }
        }
    }

    // Run the Bruteforce Loop
    startTime = time(NULL);
    tempTime = startTime;
    size_t pureMonsterArmiesSize, heroMonsterArmiesSize;
    for (size_t armySize = 1; armySize <= instance.maxCombatants; armySize++) {
    
        pureMonsterArmiesSize = pureMonsterArmies.size();
        heroMonsterArmiesSize = heroMonsterArmies.size();
        // Output Debug Information
        debugOutput(tempTime, "Starting loop for armies of size " + to_string(armySize), outputLevel > NO_OUTPUT, false, true);
        
        // Run Fights for non-Hero setups
        debugOutput(tempTime, "  Simulating " + to_string(pureMonsterArmiesSize) + " non-hero Fights... ", outputLevel > BASIC_OUTPUT, false, false);
        tempTime = time(NULL);
        simulateMultipleFights(pureMonsterArmies, instance.target, outputLevel);
        
        // Run fights for setups with heroes
        debugOutput(tempTime, "  Simulating " + to_string(heroMonsterArmiesSize) + " hero Fights... ", outputLevel > BASIC_OUTPUT, true, false);
        tempTime = time(NULL);
        simulateMultipleFights(heroMonsterArmies, instance.target, outputLevel);
        
        // If we have a valid solution with 0 followers there is no need to continue
        if (best.monsterAmount > 0 && best.followerCost == 0) { break; }
        
        if (armySize < instance.maxCombatants) { 
            // Sort the results by follower cost for some optimization
            debugOutput(tempTime, "  Sorting List... ", outputLevel > BASIC_OUTPUT, true, false);
            tempTime = time(NULL);
            sort(pureMonsterArmies.begin(), pureMonsterArmies.end(), hasFewerFollowers);
            sort(heroMonsterArmies.begin(), heroMonsterArmies.end(), hasFewerFollowers);
                
            if (armySize == firstDominance && outputLevel > NO_OUTPUT) {
                outputLevel = DETAILED_OUTPUT;
                cout << endl;
                if (best.monsterAmount > 0) {
                    cout << "Best Solution so far:" << endl;
                    cout << "  " << best.toString() << endl;
                } else {
                    cout << "Could not find a solution yet!" << endl;
                }
                if (!askYesNoQuestion("Continue calculation?", "  Continuing will most likely result in a cheaper solution but could consume a lot of RAM.\n")) {return 0;}
                startTime = time(NULL);
                tempTime = startTime;
                debugOutput(tempTime, "\nPreparing to work on loop for armies of size " + to_string(armySize+1), outputLevel > BASIC_OUTPUT, false, true);
            }
                
            if (firstDominance <= armySize) {
                // Calculate which results are strictly better than others (dominance)
                debugOutput(tempTime, "  Calculating Dominance for non-heroes... ", outputLevel > BASIC_OUTPUT, outputLevel > BASIC_OUTPUT && firstDominance < armySize, false);
                tempTime = time(NULL);
                
                int leftFollowerCost;
                FightResult * currentFightResult;
                int8_t leftHeroList[ARMY_MAX_SIZE];
                size_t leftHeroListSize;
                int8_t rightMonster;
                int8_t leftMonster;
                // First Check dominance for non-Hero setups
                for (i = 0; i < pureMonsterArmiesSize; i++) {
                    leftFollowerCost = pureMonsterArmies[i].followerCost;
                    currentFightResult = &pureMonsterArmies[i].lastFightData;
                    // A result is obsolete if only one expansion is left but no single mob can beat the last two enemy mobs alone (optimizable)
                    if (armySize == (instance.maxCombatants - 1) && optimizable) {
                        // TODO: Investigate whether this is truly correct: What if the second-to-last mob is already damaged (not from aoe) i.e. it defeated the last mob of left?
                        if (currentFightResult->rightWon && currentFightResult->monstersLost < (int) (instance.targetSize - 2) && currentFightResult->rightAoeDamage == 0) {
                            currentFightResult->dominated = true;
                        }
                    }
                    // A result is dominated If:
                    if (!currentFightResult->dominated) { 
                        // Another pureResults got farther with a less costly lineup
                        for (j = i+1; j < pureMonsterArmiesSize; j++) {
                            if (leftFollowerCost < pureMonsterArmies[j].followerCost) {
                                break; 
                            } else if (*currentFightResult <= pureMonsterArmies[j].lastFightData) { // currentFightResult has more followers implicitly 
                                currentFightResult->dominated = true;
                                break;
                            }
                        }
                        // A lineup without heroes is better than a setup with heroes even if it got just as far
                        for (j = 0; j < heroMonsterArmiesSize; j++) {
                            if (leftFollowerCost > heroMonsterArmies[j].followerCost) {
                                break; 
                            } else if (heroMonsterArmies[j].lastFightData <= *currentFightResult) { // currentFightResult has less followers implicitly
                                heroMonsterArmies[j].lastFightData.dominated = true;
                            }                       
                        }
                    }
                }
                
                debugOutput(tempTime, "  Calculating Dominance for heroes... ", outputLevel > BASIC_OUTPUT, true, false);
                tempTime = time(NULL);
                // Domination for setups with heroes
                bool usedHeroSubset, leftUsedHero;
                for (i = 0; i < heroMonsterArmiesSize; i++) {
                    leftFollowerCost = heroMonsterArmies[i].followerCost;
                    currentFightResult = &heroMonsterArmies[i].lastFightData;
                    leftHeroListSize = 0;
                    for (si = 0; si < armySize; si++) {
                        leftMonster = heroMonsterArmies[i].monsters[si];
                        if (monsterReference[leftMonster].isHero) {
                            leftHeroList[leftHeroListSize] = leftMonster;
                            leftHeroListSize++;
                        }
                    }
                    
                    // A result is obsolete if only one expansion is left but no single mob can beat the last two enemy mobs alone (optimizable)
                    if (armySize == (instance.maxCombatants - 1) && optimizable && currentFightResult->rightAoeDamage == 0) {
                        // TODO: Investigate whether this is truly correct: What if the second-to-last mob is already damaged (not from aoe) i.e. it defeated the last mob of left?
                        if (currentFightResult->rightWon && currentFightResult->monstersLost < (int) (instance.targetSize - 2)){
                            currentFightResult->dominated = true;
                        }
                    }
                    
                    // A result is dominated If:
                    if (!currentFightResult->dominated) {
                        // if i costs more followers and got less far than j, then i is dominated
                        for (j = i+1; j < heroMonsterArmiesSize; j++) {
                            if (leftFollowerCost < heroMonsterArmies[j].followerCost) {
                                break;
                            } else if (*currentFightResult <= heroMonsterArmies[j].lastFightData) { // i has more followers implicitly
                                usedHeroSubset = true; // If j doesn't use a strict subset of the heroes i used, it cannot dominate i
                                for (sj = 0; sj < armySize; sj++) { // for every hero in j there must be the same hero in i
                                    leftUsedHero = false; 
                                    rightMonster = heroMonsterArmies[j].monsters[sj];
                                    if (monsterReference[rightMonster].isHero) { // mob is a hero
                                        for (si = 0; si < leftHeroListSize; si++) {
                                            if (leftHeroList[si] == rightMonster) {
                                                leftUsedHero = true;
                                                break;
                                            }
                                        }
                                        if (!leftUsedHero) {
                                            usedHeroSubset = false;
                                            break;
                                        }
                                    }
                                }
                                if (usedHeroSubset) {
                                    currentFightResult->dominated = true;
                                    break;
                                }                           
                            }
                        }
                    }
                }
            }
            // now we expand to add the next monster to all non-dominated armies
            debugOutput(tempTime, "  Expanding Lineups by one... ", outputLevel > BASIC_OUTPUT, true, false);
            tempTime = time(NULL);
            
            vector<Army> nextPureArmies;
            vector<Army> nextHeroArmies;
            expand(nextPureArmies, nextHeroArmies, pureMonsterArmies, heroMonsterArmies, armySize);

            debugOutput(tempTime, "  Moving Data... ", outputLevel > BASIC_OUTPUT, true, false);
            tempTime = time(NULL);
            
            pureMonsterArmies = move(nextPureArmies);
            heroMonsterArmies = move(nextHeroArmies);
        }
        debugOutput(tempTime, "", outputLevel > BASIC_OUTPUT, true, true);
    }
    return time(NULL) - startTime;
}

int main(int argc, char** argv) {
    
    // Declare Variables
    vector<int> heroLevels;
    int32_t minimumMonsterCost;
    int32_t userFollowerUpperBound;
    vector<Instance> instances;
    int outputLevel;
    bool userWantsContinue;
 
    // Define User Input Data
    size_t firstDominance = ARMY_MAX_BRUTEFORCEABLE_SIZE;   // Set this to control at which army length dominance should first be calculated. Treat with extreme caution. Not using dominance at all WILL use more RAM than you have
    string macroFileName = "default.cqinput";               // Path to default macro file

    // Flow Control Variables
    bool useDefaultMacroFile = false;    // Set this to true to always use the specified macro file
    bool showMacroFileInput = true;    // Set this to true to see what the macrofile inputs
    bool individual = false;            // Set this to true if you want to simulate individual fights (lineups will be promted when you run the program)
    
    // -------------------------------------------- Program Start --------------------------------------------
    
    cout << welcomeMessage << endl;
    cout << helpMessage << endl << endl;
    
    // Initialize global Data
    initMonsterData();
    
    if (individual) { // custom input mode
        cout << "Simulating individual Figths" << endl;
        while (true) {
            Army left = takeInstanceInput("Enter friendly lineup: ")[0].target;
            Army right = takeInstanceInput("Enter hostile lineup: ")[0].target;
            simulateFight(left, right, true);
            cout << left.lastFightData.rightWon << " " << left.followerCost << " " << right.followerCost << endl;
            
            if (!askYesNoQuestion("Simulate another Fight?", "")) {
                break;
            }
        }
        return 0;
    }
    
    // Check if the user provided a filename to be used as a macro file
    if (argc >= 2) {
        initMacroFile(argv[1], showMacroFileInput);
    }
    else if (useDefaultMacroFile) {
        initMacroFile(macroFileName, showMacroFileInput);
    }
    
    // Collect the Data via Command Line
    heroLevels = takeHerolevelInput();
    minimumMonsterCost = stoi(getResistantInput("Set a lower follower limit on monsters used: ", minimumMonsterCostHelp, integer));
    userFollowerUpperBound = stoi(getResistantInput("Set an upper follower limit that you want to use: ", maxFollowerHelp, integer));
    
    // Fill monster arrays with relevant monsters
    filterMonsterData(minimumMonsterCost);
    initializeUserHeroes(heroLevels);
    
    do {
        instances = takeInstanceInput("Enter Enemy Lineup(s): ");
        cout << "Calculating with " << availableMonsters.size() << " available Monsters and " << availableHeroes.size() << " enabled Heroes." << endl;
        
        for (size_t i = 0; i < instances.size(); i++) {
            // Reset solution Data
            best = Army();
            totalFightsSimulated = 0;
            
            if (userFollowerUpperBound < 0) {
                followerUpperBound = numeric_limits<int>::max();
            } else {
                followerUpperBound = userFollowerUpperBound;
            }
            
            outputLevel = BASIC_OUTPUT;
            if (instances.size() > 1) {
                outputLevel = NO_OUTPUT;
            }
            int totalTime = solveInstance(instances[i], firstDominance, outputLevel);
            
            cout << endl << "Solution for " << instances[i].target.toString() << ":" << endl;
            
            // Announce the result
            if (best.monsterAmount > 0) {
                cout << "  " << best.toString() << endl;
                best.lastFightData.valid = false;
                simulateFight(best, instances[i].target); // Sanity check on the solution
                if (best.lastFightData.rightWon) {
                    cout << "  This does not beat the lineup!!!";
                    for (int i = 1; i <= 10; i++) {
                        cout << "ERROR";
                    } cout << endl;
                    haltExecution();
                    return EXIT_FAILURE;
                }
            } else {
                cout << endl << "Could not find a solution that beats this lineup." << endl;
            }
            cout << "  " << totalFightsSimulated << " Fights simulated." << endl;
            cout << "  Total Calculation Time: " << totalTime << endl;
        }
        userWantsContinue = askYesNoQuestion("Do you want to calculate more lineups?", "");
    } while (userWantsContinue);
    cout << endl;
    haltExecution();
    return EXIT_SUCCESS;
}
