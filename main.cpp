#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <limits>

#include "inputProcessing.h"
#include "cosmosData.h"
#include "battleLogic.h"

using namespace std;

IOManager iomanager;

// Simulates fights with all armies against the target. The Fightresults are written to the corresponding structs in armies.
// If a solution is found, armies that are more expensive than that solution are ignored
void simulateMultipleFights(vector<Army> & armies, Instance & instance) {
    bool newFound = false;
    size_t armyAmount = armies.size();
    
    if (!instance.hasWorldBoss) {
        for (size_t i = 0; i < armyAmount; i++) {
            if (armies[i].followerCost < instance.followerUpperBound) { // Ignore if a cheaper solution exists
                simulateFight(armies[i], instance.target);
                if (!armies[i].lastFightData.rightWon) {  // left (our side) wins:
                    if (!newFound) {
                        iomanager.suspendTimedOutputs(DETAILED_OUTPUT);
                    }
                    newFound = true;
                    instance.followerUpperBound = armies[i].followerCost;
                    instance.bestSolution = armies[i];
                    iomanager.outputMessage(instance.bestSolution.toString(), DETAILED_OUTPUT, 2);
                }
            }
        }
        if (newFound) {
            iomanager.resumeTimedOutputs(DETAILED_OUTPUT);
        }
    } else {
        for (size_t i = 0; i < armyAmount; i++) {
            simulateFight(armies[i], instance.target);
            if (instance.lowestBossHealth == -1 || armies[i].lastFightData.frontHealth < instance.lowestBossHealth) {
                instance.bestSolution = armies[i];
                instance.lowestBossHealth = armies[i].lastFightData.frontHealth;
            }
        }
    }
}

// Take the data from oldArmies and write all armies into newArmies with an additional monster at the end.
// Armies that are dominated are ignored.
// TODO: Dont expand with heroes if army is too expensive
void expand(vector<Army> & newPureArmies, vector<Army> & newHeroArmies, 
            const vector<Army> & oldPureArmies, const vector<Army> & oldHeroArmies, 
            const size_t currentArmySize, const Instance & instance) {

    int remainingFollowers;
    size_t availableMonstersSize = availableMonsters.size();
    size_t availableHeroesSize = availableHeroes.size();
    size_t oldPureArmiesSize = oldPureArmies.size();
    size_t oldHeroArmiesSize = oldHeroArmies.size();
    size_t i, m;
    
    for (i = 0; i < oldPureArmiesSize; i++) {
        remainingFollowers = instance.followerUpperBound - oldPureArmies[i].followerCost;
        if (!oldPureArmies[i].lastFightData.dominated && remainingFollowers >= 0) {
            for (m = 0; m < availableMonstersSize && monsterReference[availableMonsters[m]].cost < remainingFollowers; m++) {
                newPureArmies.push_back(oldPureArmies[i]);
                newPureArmies.back().add(availableMonsters[m]);
                newPureArmies.back().lastFightData.valid = !instance.hasAoe && !instance.hasAsymmetricAoe;
            }
            for (m = 0; m < availableHeroesSize; m++) {
                newHeroArmies.push_back(oldPureArmies[i]);
                newHeroArmies.back().add(availableHeroes[m]);
                newHeroArmies.back().lastFightData.valid = !monsterReference[availableHeroes[m]].skill.violatesFightResults && !instance.hasAoe && !instance.hasAsymmetricAoe;
            }
        }
    }
    
    vector<bool> usedHeroes; usedHeroes.resize(monsterReference.size(), false);
    HeroSkill currentSkill;
    bool invalidSkill;
    bool friendsInfluence;
    bool rainbowInfluence;
    for (i = 0; i < oldHeroArmiesSize; i++) {
        remainingFollowers = instance.followerUpperBound - oldHeroArmies[i].followerCost;
        if (!oldHeroArmies[i].lastFightData.dominated && remainingFollowers >= 0) {
            friendsInfluence = false;
            rainbowInfluence = false;
            invalidSkill = false;
            for (m = 0; m < currentArmySize; m++) {
                currentSkill = monsterReference[oldHeroArmies[i].monsters[m]].skill;
                invalidSkill |= currentSkill.hasAoe || currentSkill.hasAsymmetricAoe;
                friendsInfluence |= currentSkill.skillType == FRIENDS;
                rainbowInfluence |= currentSkill.skillType == RAINBOW && currentArmySize > m + 4; // Hardcoded number of elements required to activate rainbow
                usedHeroes[oldHeroArmies[i].monsters[m]] = true;
            }
            for (m = 0; m < availableMonstersSize && monsterReference[availableMonsters[m]].cost < remainingFollowers; m++) {
                newHeroArmies.push_back(oldHeroArmies[i]);
                newHeroArmies.back().add(availableMonsters[m]);
                newHeroArmies.back().lastFightData.valid = !friendsInfluence && !rainbowInfluence && !invalidSkill && !instance.hasAoe && !instance.hasAsymmetricAoe;
            }
            for (m = 0; m < availableHeroesSize; m++) {
                if (!usedHeroes[availableHeroes[m]]) {
                    newHeroArmies.push_back(oldHeroArmies[i]);
                    newHeroArmies.back().add(availableHeroes[m]);
                    newHeroArmies.back().lastFightData.valid = !monsterReference[availableHeroes[m]].skill.violatesFightResults && !rainbowInfluence && !invalidSkill && !instance.hasAoe && !instance.hasAsymmetricAoe;
                }
                usedHeroes[availableHeroes[m]] = false;
            }
        }
    }
}

// Takes the armies sorts them and compares them with each other. Armies that are strictly worse than other armies or have no chance of winning get dominated
void calculateDominance(Instance & instance, bool optimizable,
                        vector<Army> & pureMonsterArmies, vector<Army> & heroMonsterArmies,
                        size_t armySize, size_t firstDominance) {
    size_t i, j, si, sj;
    size_t pureMonsterArmiesSize = pureMonsterArmies.size();
    size_t heroMonsterArmiesSize = heroMonsterArmies.size();
    
    int leftFollowerCost;
    FightResult * currentFightResult;
    
    // First Check dominance for non-Hero setups
    iomanager.timedOutput("Calculating Dominance for non-heroes... ", DETAILED_OUTPUT, 1, firstDominance == armySize);
    
    // Preselection based on the information that no monster can beat 2 monsters alone if optimizable is true
    if (armySize == (instance.maxCombatants - 1) && optimizable) { // Must be optimizable and the last expansion
        for (i = 0; i < pureMonsterArmiesSize; i++) {
            pureMonsterArmies[i].lastFightData.dominated = pureMonsterArmies[i].lastFightData.monstersLost < (int) (instance.targetSize - 2);
        }
    }
    
    sort(pureMonsterArmies.begin(), pureMonsterArmies.end(), hasFewerFollowers);
    for (i = 0; i < pureMonsterArmiesSize; i++) {
        leftFollowerCost = pureMonsterArmies[i].followerCost;
        currentFightResult = &pureMonsterArmies[i].lastFightData;
        if (currentFightResult->dominated || leftFollowerCost > instance.followerUpperBound) {
            break; // All dominated results are in the back
        }

        // Another pureResults got farther with a less costly lineup
        for (j = i+1; j < pureMonsterArmiesSize; j++) {
            if (leftFollowerCost < pureMonsterArmies[j].followerCost) {
                break; 
            } else if (*currentFightResult <= pureMonsterArmies[j].lastFightData) { // currentFightResult has more followers implicitly 
                currentFightResult->dominated = true;
                break;
            }
        }
    }
    // Domination for setups with heroes
    iomanager.timedOutput("Calculating Dominance for heroes... ", DETAILED_OUTPUT, 1);
    // Preselection based on the information that no monster can beat 2 monsters alone if optimizable is true
    // Like the rest of dominance this is unreliable because an aoe hero could easily affect earlier rounds
    if (armySize == (instance.maxCombatants - 1) && optimizable) { // Must be optimizable and the last expansion
        for (i = 0; i < heroMonsterArmiesSize; i++) {
            currentFightResult = &heroMonsterArmies[i].lastFightData;
            
            currentFightResult->dominated = currentFightResult->rightAoeDamage == 0 && // make sure there is no interference to the optimized calculation
                                            currentFightResult->monstersLost < (int) (instance.targetSize - 2); // Army left at least 2 enemies alive
        }
    }
    
    sort(heroMonsterArmies.begin(), heroMonsterArmies.end(), hasFewerFollowers);
    
    vector<bool> leftMonsterSet; leftMonsterSet.resize(monsterReference.size());
    size_t leftMonsterSetSize = leftMonsterSet.size();
    bool usedHeroSubset;
    for (i = 0; i < leftMonsterSetSize; i++) { // prepare monsterlist
        leftMonsterSet[i] = monsterReference[i].rarity != NO_HERO; // Normal Monsters are true by default
    }
    
    for (i = 0; i < heroMonsterArmiesSize; i++) {
        leftFollowerCost = heroMonsterArmies[i].followerCost;
        currentFightResult = &heroMonsterArmies[i].lastFightData;
        if (currentFightResult->dominated || leftFollowerCost > instance.followerUpperBound) {
            break; // All dominated results are in the back
        }
        
        for (si = 0; si < armySize; si++) {
            leftMonsterSet[heroMonsterArmies[i].monsters[si]] = true; // Add lefts monsters to set
        }

        // Proper dominance check
        if (!currentFightResult->dominated) {
            // if i costs more followers and got less far than j, then i is dominated
            for (j = i+1; j < heroMonsterArmiesSize; j++) {
                if (leftFollowerCost < heroMonsterArmies[j].followerCost) {
                    break;
                } else if (*currentFightResult <= heroMonsterArmies[j].lastFightData) { // i has more followers implicitly
                    usedHeroSubset = true; // If j doesn't use a strict subset of the heroes i used, it cannot dominate i
                    for (sj = 0; sj < armySize; sj++) { // for every hero in j there must be the same hero in i
                        if (!leftMonsterSet[heroMonsterArmies[j].monsters[sj]]) {
                            usedHeroSubset = false;
                            break;
                        }
                    }
                    if (usedHeroSubset) {
                        currentFightResult->dominated = true;
                        break;
                    }                           
                }
            }
        }
        // Clean up monster set for next iteration
        for (si = 0; si < armySize; si++) { 
            leftMonsterSet[heroMonsterArmies[i].monsters[si]] = monsterReference[heroMonsterArmies[i].monsters[si]].rarity == NO_HERO; // Remove only heroes from the set
        }
    }
}

// Use a greedy method to get a first upper bound on follower cost for the solution
// Greedy approach for 4 or less monsters is obsolete, as bruteforce is still fast enough
void getQuickSolutions(Instance & instance) {
    Army tempArmy;
    vector<int8_t> greedy;
    vector<int8_t> greedyHeroes;
    vector<int8_t> greedyTemp;
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
            invalid = greedy.size() < instance.maxCombatants; // if true it didnt find a monster that drew position i
        }
        
        if (!invalid) {
            if (instance.followerUpperBound > tempArmy.followerCost) {
                instance.bestSolution = tempArmy;
                instance.followerUpperBound = tempArmy.followerCost;
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
            if (instance.followerUpperBound > tempArmy.followerCost) {
                instance.bestSolution = tempArmy;
                instance.followerUpperBound = tempArmy.followerCost;
            }
        }
    }
}

// Main method for solving an instance.
void solveInstance(Instance & instance, size_t firstDominance) {
    Army tempArmy;
    time_t startTime;
    size_t i;

    // Get first Upper limit on followers with  agreedy algorithm
    if (instance.maxCombatants > ARMY_MAX_BRUTEFORCEABLE_SIZE) {
        getQuickSolutions(instance);
    }
    
    // Fill two vectors with armies each containing exactly one unique available hero or monster
    vector<Army> pureMonsterArmies;
    vector<Army> heroMonsterArmies;
    for (i = 0; i < availableMonsters.size(); i++) {
        if (monsterReference[availableMonsters[i]].cost <= instance.followerUpperBound) {
            pureMonsterArmies.push_back(Army( {availableMonsters[i]} ));
        }
    }
    for (i = 0; i < availableHeroes.size(); i++) { // Ignore checking for Hero Cost
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
    for (size_t armySize = 1; armySize <= instance.maxCombatants; armySize++) {
        // Output Debug Information
        iomanager.outputMessage("Starting loop for armies of size " + to_string(armySize), BASIC_OUTPUT);
        
        // Run Fights for non-Hero setups
        iomanager.timedOutput("Simulating " + to_string(pureMonsterArmies.size()) + " non-hero Fights... ", DETAILED_OUTPUT, 1, true);
        simulateMultipleFights(pureMonsterArmies, instance);
        
        // Run fights for setups with heroes
        iomanager.timedOutput("Simulating " + to_string(heroMonsterArmies.size()) + " hero Fights... ", DETAILED_OUTPUT, 1);
        simulateMultipleFights(heroMonsterArmies, instance);
        
        // If we have a valid solution with 0 followers there is no need to continue
        if (!instance.hasWorldBoss && instance.bestSolution.monsterAmount > 0 && instance.bestSolution.followerCost == 0) { break; }
        
        // Start Expansion routine if there is still room
        if (armySize < instance.maxCombatants) { 
            // Manage output format 
            if (armySize == firstDominance && iomanager.outputLevel == BASIC_OUTPUT) {
                iomanager.outputLevel = DETAILED_OUTPUT; // Switch output level after pure brutefore is exhausted
            }
            if (armySize == firstDominance) {
                iomanager.outputMessage("", DETAILED_OUTPUT);
                if (!instance.bestSolution.isEmpty()) {
                    iomanager.outputMessage("Best Solution so far:", DETAILED_OUTPUT);
                    iomanager.outputMessage(instance.bestSolution.toString(), DETAILED_OUTPUT, 1);
                    if (instance.hasWorldBoss) {
                        iomanager.outputMessage("Damage Done: " + to_string(WORLDBOSS_HEALTH - instance.lowestBossHealth), DETAILED_OUTPUT, 1);
                    }
                } else {
                    iomanager.outputMessage("Could not find a solution yet!", DETAILED_OUTPUT);
                }
                if (!iomanager.askYesNoQuestion("Continue calculation?", DETAILED_OUTPUT, POSITIVE_ANSWER)) {return;}
                startTime = time(NULL);
                iomanager.outputMessage("\nPreparing to work on loop for armies of size " + to_string(armySize+1), BASIC_OUTPUT);
                iomanager.outputMessage("Currently considering " + to_string(pureMonsterArmies.size()) + " normal and " + to_string(heroMonsterArmies.size()) + " hero armies.", BASIC_OUTPUT);
            }
                
            // Calculate which results are strictly better than others (dominance)
            if (firstDominance <= armySize) {
                calculateDominance(instance, optimizable, pureMonsterArmies, heroMonsterArmies, armySize, firstDominance);
            }
                
            // now we expand to add the next monster to all non-dominated armies
            iomanager.timedOutput("Expanding Lineups by one... ", DETAILED_OUTPUT, 1);
            vector<Army> nextPureArmies;
            vector<Army> nextHeroArmies;
            expand(nextPureArmies, nextHeroArmies, pureMonsterArmies, heroMonsterArmies, armySize, instance);

            iomanager.timedOutput("Moving Data... ", DETAILED_OUTPUT, 1);
            pureMonsterArmies = move(nextPureArmies);
            heroMonsterArmies = move(nextHeroArmies);
        }
        iomanager.finishTimedOutput(DETAILED_OUTPUT);
    }
    instance.calculationTime = time(NULL) - startTime;
}

void outputSolution(Instance instance, bool replayStrings) {
    instance.bestSolution.lastFightData.valid = false;
    simulateFight(instance.bestSolution, instance.target); // Sanity check on the solution
    bool sane;
    sane = !instance.hasWorldBoss && (!instance.bestSolution.lastFightData.rightWon || instance.bestSolution.isEmpty());
    sane |= instance.hasWorldBoss && instance.bestSolution.lastFightData.frontHealth == instance.lowestBossHealth;
    
    if (iomanager.outputLevel == SERVER_OUTPUT) {
        iomanager.outputMessage(instance.toJSON(sane), SERVER_OUTPUT);
    } else {
        iomanager.outputMessage(instance.toString(sane, replayStrings), CMD_OUTPUT);
    }
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
    bool useDefaultMacroFile = true;    // Set this to true to always use the specified macro file
    bool showMacroFileInput = true;     // Set this to true to see what the macrofile inputs
    bool individual = false;            // Set this to true if you want to simulate individual fights (lineups will be promted when you run the program)
    bool showReplayStrings = true;      // Set this to true to see battle replay strings that can be used ingame
    
    iomanager.outputLevel = CMD_OUTPUT;
    // Check if the user provided a filename to be used as a macro file
    try {
        if (argc >= 2) {
            if (argc >= 3 && (string) argv[2] == "-server") {
                showMacroFileInput = false;
                iomanager.outputLevel = SERVER_OUTPUT;
            }
            iomanager.initMacroFile(argv[1], showMacroFileInput);
        }
        else if (useDefaultMacroFile) {
            iomanager.initMacroFile(macroFileName, showMacroFileInput);
        }
    } catch (InputException e) {
        if (e == MACROFILE_MISSING) {
            if (iomanager.outputLevel == SERVER_OUTPUT) {
                iomanager.outputMessage(iomanager.getJSONError(e), SERVER_OUTPUT);
                return EXIT_FAILURE;
            } else {
                iomanager.outputMessage("Macrofile not Found. Switching no manual Input...", CMD_OUTPUT);
            }
        } else {
            throw e;
        }
    }
    
    // Initialize global Data
    initGameData();
    
    // -------------------------------------------- Program Start --------------------------------------------    
    
    iomanager.outputMessage(welcomeMessage, CMD_OUTPUT);
    iomanager.outputMessage(helpMessage, CMD_OUTPUT);
    
    if (individual) {
        iomanager.outputMessage("Simulating individual Figths", CMD_OUTPUT);
        while (true) {
            Army left = iomanager.takeInstanceInput("Enter friendly lineup: ")[0].target;
            Army right = iomanager.takeInstanceInput("Enter hostile lineup: ")[0].target;
            simulateFight(left, right, true);
            iomanager.outputMessage(to_string(left.lastFightData.rightWon) + " " + to_string(left.followerCost) + " " + to_string(right.followerCost), CMD_OUTPUT);
            
            if (!iomanager.askYesNoQuestion("Simulate another Fight?", CMD_OUTPUT, NEGATIVE_ANSWER)) {
                break;
            }
        }
        return EXIT_SUCCESS;
    }
    
    // Collect the Data via Command Line
    try {
        availableHeroes = iomanager.takeHerolevelInput();
        minimumMonsterCost = stoi(iomanager.getResistantInput("Set a lower follower limit on monsters used: ", integer));
        userFollowerUpperBound = stoi(iomanager.getResistantInput("Set an upper follower limit that you want to use: ", integer));
    } catch (InputException e) {
        if (e == MACROFILE_USED_UP) {
            if (iomanager.outputLevel == SERVER_OUTPUT) {
                iomanager.outputMessage(iomanager.getJSONError(e), SERVER_OUTPUT);
                return EXIT_FAILURE;
            }
        } else {
            throw e;
        }
    }
    
    // Fill monster arrays with relevant monsters
    filterMonsterData(minimumMonsterCost);
    
    do {
        try {
            instances = iomanager.takeInstanceInput("Enter Enemy Lineup(s): ");
        } catch (InputException e) {
            if (e == MACROFILE_USED_UP) {
                if (iomanager.outputLevel == SERVER_OUTPUT) {
                    iomanager.outputMessage(iomanager.getJSONError(e), SERVER_OUTPUT);
                    return EXIT_FAILURE;
                }
            } else {
                throw e;
            }
        }
        iomanager.outputMessage("\nCalculating with " + to_string(availableMonsters.size()) + " available Monsters and " + to_string(availableHeroes.size()) + " enabled Heroes.", CMD_OUTPUT);
        
        if (iomanager.outputLevel == CMD_OUTPUT) { // TODO: Reset Output Level if userwantscontinue
            if (instances.size() > 1) {
                iomanager.outputLevel = SOLUTION_OUTPUT;
            } else {
                iomanager.outputLevel = BASIC_OUTPUT;
            }
        }
        
        for (size_t i = 0; i < instances.size(); i++) {
            totalFightsSimulated = &(instances[i].totalFightsSimulated);
            
            if (userFollowerUpperBound < 0) {
                instances[i].followerUpperBound = numeric_limits<int>::max();
            } else {
                instances[i].followerUpperBound = userFollowerUpperBound;
            }
            
            solveInstance(instances[i], firstDominance);
            outputSolution(instances[i], showReplayStrings);
        }
        userWantsContinue = iomanager.askYesNoQuestion("Do you want to calculate more lineups?", CMD_OUTPUT, NEGATIVE_ANSWER);
    } while (userWantsContinue);
    
    iomanager.outputMessage("", CMD_OUTPUT);
    iomanager.haltExecution();
    return EXIT_SUCCESS;
}