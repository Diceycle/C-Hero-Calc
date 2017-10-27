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

// Define global variables used to track the best result
int32_t followerUpperBound;
Army best;

IOManager iomanager;

// Simulates fights with all armies against the target. Armies will contain Army objects with the results written in.
void simulateMultipleFights(vector<Army> & armies, Army target) {
    bool newFound = false;
    size_t i = 0;
    size_t armyAmount = armies.size();
    
    for (i = 0; i < armyAmount; i++) {
        simulateFight(armies[i], target);
        if (!armies[i].lastFightData.rightWon) {  // left (our side) wins:
            if (armies[i].followerCost < followerUpperBound) {
                if (!newFound) {
                    iomanager.suspendTimedOutputs(DETAILED_OUTPUT);
                }
                newFound = true;
                followerUpperBound = armies[i].followerCost;
                best = armies[i];
                iomanager.outputMessage(best.toString(), DETAILED_OUTPUT, 2);
            }
        }
    }
    if (newFound) {
        iomanager.resumeTimedOutputs(DETAILED_OUTPUT);
    }
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
void getQuickSolutions(Instance & instance) {
    Army tempArmy = Army();
    vector<int8_t> greedy {};
    vector<int8_t> greedyHeroes {};
    vector<int8_t> greedyTemp {};
    bool invalid = false;
    
    iomanager.outputMessage("Trying to find solutions greedily...", DETAILED_OUTPUT);
    
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
            invalid = greedy.size() < instance.maxCombatants;
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
void solveInstance(Instance & instance, size_t firstDominance) {
    Army tempArmy = Army();
    int startTime;
    
    size_t i, j, sj, si;

    // Get first Upper limit on followers
    if (instance.maxCombatants > ARMY_MAX_BRUTEFORCEABLE_SIZE) {
        getQuickSolutions(instance);
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
    size_t pureMonsterArmiesSize, heroMonsterArmiesSize;
    for (size_t armySize = 1; armySize <= instance.maxCombatants; armySize++) {
    
        pureMonsterArmiesSize = pureMonsterArmies.size();
        heroMonsterArmiesSize = heroMonsterArmies.size();
        // Output Debug Information
        iomanager.outputMessage("Starting loop for armies of size " + to_string(armySize), BASIC_OUTPUT);
        
        // Run Fights for non-Hero setups
        iomanager.timedOutput("Simulating " + to_string(pureMonsterArmiesSize) + " non-hero Fights... ", DETAILED_OUTPUT, 1, true);
        simulateMultipleFights(pureMonsterArmies, instance.target);
        
        // Run fights for setups with heroes
        iomanager.timedOutput("Simulating " + to_string(heroMonsterArmiesSize) + " hero Fights... ", DETAILED_OUTPUT, 1);
        simulateMultipleFights(heroMonsterArmies, instance.target);
        
        // If we have a valid solution with 0 followers there is no need to continue
        if (best.monsterAmount > 0 && best.followerCost == 0) { break; }
        
        if (armySize < instance.maxCombatants) { 
            // Sort the results by follower cost for some optimization
            iomanager.timedOutput("Sorting Lists... ", DETAILED_OUTPUT, 1);
            sort(pureMonsterArmies.begin(), pureMonsterArmies.end(), hasFewerFollowers);
            sort(heroMonsterArmies.begin(), heroMonsterArmies.end(), hasFewerFollowers);
                
            if (armySize == firstDominance && iomanager.outputLevel == BASIC_OUTPUT) {
                iomanager.outputLevel = DETAILED_OUTPUT; // Switch output level after pure brutefore is exhausted
            }
            if (armySize == firstDominance) {
                iomanager.outputMessage("", DETAILED_OUTPUT);
                if (best.monsterAmount > 0) {
                    iomanager.outputMessage("Best Solution so far:", DETAILED_OUTPUT);
                    iomanager.outputMessage(best.toString(), DETAILED_OUTPUT, 1);
                } else {
                    iomanager.outputMessage("Could not find a solution yet!", DETAILED_OUTPUT);
                }
                if (!iomanager.askYesNoQuestion("Continue calculation?", "  Continuing will most likely result in a cheaper solution but could consume a lot of RAM.\n", DETAILED_OUTPUT, POSITIVE_ANSWER)) {return;}
                startTime = time(NULL);
                iomanager.outputMessage("\nPreparing to work on loop for armies of size " + to_string(armySize+1), BASIC_OUTPUT);
                iomanager.outputMessage("Currently considering " + to_string(pureMonsterArmies.size()) + " normal and " + to_string(heroMonsterArmies.size()) + " hero armies.", BASIC_OUTPUT);
            }
                
            if (firstDominance <= armySize) {
                // Calculate which results are strictly better than others (dominance)
                iomanager.timedOutput("Calculating Dominance for non-heroes... ", DETAILED_OUTPUT, 1, firstDominance == armySize);
                
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
                
                iomanager.timedOutput("Calculating Dominance for heroes... ", DETAILED_OUTPUT, 1);
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
            iomanager.timedOutput("Expanding Lineups by one... ", DETAILED_OUTPUT, 1);
            vector<Army> nextPureArmies;
            vector<Army> nextHeroArmies;
            expand(nextPureArmies, nextHeroArmies, pureMonsterArmies, heroMonsterArmies, armySize);

            iomanager.timedOutput("Moving Data... ", DETAILED_OUTPUT, 1);
            pureMonsterArmies = move(nextPureArmies);
            heroMonsterArmies = move(nextHeroArmies);
        }
        iomanager.finishTimedOutput(DETAILED_OUTPUT);
    }
    instance.calculationTime = time(NULL) - startTime;
}

void outputSolution(Instance instance) {
    cout << endl << "Solution for " << instance.target.toString() << ":" << endl;
            
    // Announce the result
    if (best.monsterAmount > 0) {
        cout << "  " << best.toString() << endl;
        best.lastFightData.valid = false;
        simulateFight(best, instance.target); // Sanity check on the solution
        if (best.lastFightData.rightWon) {
            cout << "  This does not beat the lineup!!!" << endl;
            cout << "FATAL ERROR!!! Please comment this output in the Forums!" << endl;
        }
    } else {
        cout << endl << "Could not find a solution that beats this lineup." << endl;
    }
    cout << "  " << totalFightsSimulated << " Fights simulated." << endl;
    cout << "  Total Calculation Time: " << instance.calculationTime << endl << endl;
}

int main(int argc, char** argv) {
    
    // Declare Variables
    vector<int> heroLevels;
    int32_t minimumMonsterCost;
    int32_t userFollowerUpperBound;
    vector<Instance> instances;
    bool userWantsContinue;
 
    // Define User Input Data
    size_t firstDominance = ARMY_MAX_BRUTEFORCEABLE_SIZE;   // Set this to control at which army length dominance should first be calculated. Treat with extreme caution. Not using dominance at all WILL use more RAM than you have
    string macroFileName = "default.cqinput";               // Path to default macro file

    // Flow Control Variables
    bool useDefaultMacroFile = false;   // Set this to true to always use the specified macro file
    bool showMacroFileInput = true;     // Set this to true to see what the macrofile inputs
    bool individual = false;            // Set this to true if you want to simulate individual fights (lineups will be promted when you run the program)
    bool serverMode = false;            // Set this to true to get a completely silent program that only outputs the final solution as json
    
    iomanager = IOManager();
    iomanager.outputLevel = CMD_OUTPUT;
    // Check if the user provided a filename to be used as a macro file
    if (argc >= 2) {
        if (argc >= 3 && (string) argv[2] == "-server") {
            serverMode = true;
            showMacroFileInput = false;
            iomanager.outputLevel = SERVER_OUTPUT;
        }
        iomanager.initMacroFile(argv[1], showMacroFileInput);
    }
    else if (useDefaultMacroFile) {
        iomanager.initMacroFile(macroFileName, showMacroFileInput);
    }
    
    // -------------------------------------------- Program Start --------------------------------------------
    
    iomanager.outputMessage(welcomeMessage, CMD_OUTPUT);
    iomanager.outputMessage(helpMessage, CMD_OUTPUT);
    
    // Initialize global Data
    initMonsterData();
    
    if (individual && !serverMode) { // custom input mode
        iomanager.outputMessage("Simulating individual Figths", CMD_OUTPUT);
        while (true) {
            Army left = iomanager.takeInstanceInput("Enter friendly lineup: ")[0].target;
            Army right = iomanager.takeInstanceInput("Enter hostile lineup: ")[0].target;
            simulateFight(left, right, true);
            iomanager.outputMessage(to_string(left.lastFightData.rightWon) + " " + to_string(left.followerCost) + " " + to_string(right.followerCost), CMD_OUTPUT);
            
            if (!iomanager.askYesNoQuestion("Simulate another Fight?", "", CMD_OUTPUT, NEGATIVE_ANSWER)) {
                break;
            }
        }
        return 0;
    }
    
    // Collect the Data via Command Line
    availableHeroes = iomanager.takeHerolevelInput();
    minimumMonsterCost = stoi(iomanager.getResistantInput("Set a lower follower limit on monsters used: ", minimumMonsterCostHelp, integer));
    userFollowerUpperBound = stoi(iomanager.getResistantInput("Set an upper follower limit that you want to use: ", maxFollowerHelp, integer));
    
    // Fill monster arrays with relevant monsters
    filterMonsterData(minimumMonsterCost);
    
    do {
        instances = iomanager.takeInstanceInput("Enter Enemy Lineup(s): ");
        iomanager.outputMessage("Calculating with " + to_string(availableMonsters.size()) + " available Monsters and " + to_string(availableHeroes.size()) + " enabled Heroes.", CMD_OUTPUT);
        
        if (iomanager.outputLevel == CMD_OUTPUT) {
            if (instances.size() > 1) {
                iomanager.outputLevel = SOLUTION_OUTPUT;
            } else {
                iomanager.outputLevel = BASIC_OUTPUT;
            }
        }
        
        for (size_t i = 0; i < instances.size(); i++) {
            // Reset solution Data
            best = Army();
            totalFightsSimulated = 0;
            
            if (userFollowerUpperBound < 0) {
                followerUpperBound = numeric_limits<int>::max();
            } else {
                followerUpperBound = userFollowerUpperBound;
            }
            
            solveInstance(instances[i], firstDominance);
            outputSolution(instances[i]);
        }
        userWantsContinue = iomanager.askYesNoQuestion("Do you want to calculate more lineups?", "", CMD_OUTPUT, NEGATIVE_ANSWER);
    } while (userWantsContinue);
    
    iomanager.outputMessage("", CMD_OUTPUT);
    iomanager.haltExecution();
    return EXIT_SUCCESS;
}