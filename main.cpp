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

// Simulates fights with all armies against the target. The FightResults are written to the corresponding structs in armies.
// If a solution is found, armies that are more expensive than that solution are ignored
void simulateMultipleFights(vector<Army> & armies, Instance & instance) {
    bool newFound = false;
    size_t armyAmount = armies.size();

    if (!instance.hasWorldBoss) {
        for (size_t i = 0; i < armyAmount; i++) {
            if (armies[i].followerCost < instance.followerUpperBound) { // Ignore if a cheaper solution exists
                if (simulateFight(armies[i], instance.target)) {  // left (our side) wins:
                    if (!newFound) {
                        interface.suspendTimedOutputs(DETAILED_OUTPUT);
                    }
                    newFound = true;
                    instance.followerUpperBound = armies[i].followerCost;
                    instance.bestSolution = armies[i];
                    interface.outputMessage(instance.bestSolution.toString(), DETAILED_OUTPUT, 2);
                }
            }
        }
        if (newFound) {
            interface.resumeTimedOutputs(DETAILED_OUTPUT);
        }
    } else {
        for (size_t i = 0; i < armyAmount; i++) {
            simulateFight(armies[i], instance.target);
            if ( //instance.lowestBossHealth == -1 ||
                armies[i].lastFightData.frontHealth < instance.lowestBossHealth) {
                instance.bestSolution = armies[i];
                instance.lowestBossHealth = armies[i].lastFightData.frontHealth;
            }
            else if (armies[i].lastFightData.frontHealth > 0) { // reached the limit
                instance.bestSolution = armies[i];
                instance.lowestBossHealth = numeric_limits<DamageType>::min();
            }
        }
    }
}

// Take the data from oldArmies and write all armies into newArmies with an additional monster at the end.
// Armies that are dominated are ignored.
void expand(vector<Army> & newPureArmies, vector<Army> & newHeroArmies,
            const vector<Army> & oldPureArmies, const vector<Army> & oldHeroArmies,
            const size_t currentArmySize, const Instance & instance) {

    FollowerCount remainingFollowers;
    size_t availableMonstersSize = availableMonsters.size();
    size_t availableHeroesSize = availableHeroes.size();
    size_t oldPureArmiesSize = oldPureArmies.size();
    size_t oldHeroArmiesSize = oldHeroArmies.size();
    size_t i, m;

    bool removeUseless = currentArmySize == (instance.maxCombatants-1) && !instance.hasWorldBoss  && !instance.hasGambler;
    bool instanceInvalid = instance.hasHeal || instance.hasAsymmetricAoe || instance.hasGambler;

    // enemy booze will invalidate FightResults
    bool boozeInfluence = instance.hasBeer && currentArmySize >= instance.targetSize;

    // Expansion for non-Hero Armies
    for (i = 0; i < oldPureArmiesSize; i++) {
        if (!oldPureArmies[i].lastFightData.dominated) {
            remainingFollowers = instance.followerUpperBound - oldPureArmies[i].followerCost;
            // Add Normal Monsters. Check for Cost
            for (m = 0; m < availableMonstersSize; m++) {
                if (monsterReference[availableMonsters[m]].cost <= remainingFollowers) {
                    if (!removeUseless || instance.monsterUsefulLast[availableMonsters[m]] || instance.targetSize == oldPureArmies[i].lastFightData.monstersLost) {
                        newPureArmies.push_back(oldPureArmies[i]);
                        newPureArmies.back().add(availableMonsters[m]);
                        newPureArmies.back().lastFightData.valid = !instanceInvalid && !boozeInfluence;
                    }
                }
            }
            // Add Hero. no check needed because it is the First Added
            for (m = 0; m < availableHeroesSize; m++) {
                if (!removeUseless || instance.monsterUsefulLast[availableHeroes[m]] || instance.targetSize == oldPureArmies[i].lastFightData.monstersLost) {
                    newHeroArmies.push_back(oldPureArmies[i]);
                    newHeroArmies.back().add(availableHeroes[m]);
                    newHeroArmies.back().lastFightData.valid = !instanceInvalid &&
                                                               !boozeInfluence &&
                                                               !monsterReference[availableHeroes[m]].skill.violatesFightResults;
                }
            }
        }
    }

    vector<bool> usedHeroes; usedHeroes.resize(monsterReference.size(), false);
    HeroSkill currentSkill;
    bool invalidSkill;
    bool friendsInfluence;
    bool rainbowInfluence;
    for (i = 0; i < oldHeroArmiesSize; i++) {
        if (!oldHeroArmies[i].lastFightData.dominated) {
            remainingFollowers = instance.followerUpperBound - oldHeroArmies[i].followerCost;
            friendsInfluence = false;
            rainbowInfluence = false;
            invalidSkill = false;
            // Check for influences that can invalidate fightresults and gather used heroes
            for (m = 0; m < currentArmySize; m++) {
                currentSkill = monsterReference[oldHeroArmies[i].monsters[m]].skill;
                invalidSkill |= currentSkill.hasHeal || currentSkill.hasAsymmetricAoe;
                friendsInfluence |= currentSkill.skillType == FRIENDS;
                rainbowInfluence |= currentSkill.skillType == RAINBOW && currentArmySize > m + 4; // Hardcoded number of elements required to activate rainbow
                boozeInfluence   |= currentSkill.skillType == BEER;
                usedHeroes[oldHeroArmies[i].monsters[m]] = true;
            }

            // Add Normal Monster. No checks needed except cost
            for (m = 0; m < availableMonstersSize && monsterReference[availableMonsters[m]].cost <= remainingFollowers; m++) {
                // In case of a draw this could cause problems if no more suitable units are available
                if (!removeUseless || instance.monsterUsefulLast[availableMonsters[m]] || instance.targetSize == oldHeroArmies[i].lastFightData.monstersLost) {
                    newHeroArmies.push_back(oldHeroArmies[i]);
                    newHeroArmies.back().add(availableMonsters[m]);
                    newHeroArmies.back().lastFightData.valid = !instanceInvalid &&
                                                               !friendsInfluence &&
                                                               !rainbowInfluence &&
                                                               !boozeInfluence &&
                                                               !invalidSkill;
                }
            }
            // Add Hero. Check if hero was used before.
            for (m = 0; m < availableHeroesSize; m++) {
                if (!usedHeroes[availableHeroes[m]]) {
                    if (!removeUseless || instance.monsterUsefulLast[availableHeroes[m]] || instance.targetSize == oldHeroArmies[i].lastFightData.monstersLost) {
                        newHeroArmies.push_back(oldHeroArmies[i]);
                        newHeroArmies.back().add(availableHeroes[m]);
                        newHeroArmies.back().lastFightData.valid = !instanceInvalid &&
                                                                   !monsterReference[availableHeroes[m]].skill.violatesFightResults &&
                                                                   !rainbowInfluence &&
                                                                   !boozeInfluence &&
                                                                   !(monsterReference[availableHeroes[m]].skill.skillType == DAMPEN && instance.hasAoe) &&
                                                                   !invalidSkill;
                    }
                }
                // Clean up for the next army
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

    FollowerCount leftFollowerCost;
    FightResult * currentFightResult;

    // First Check dominance for non-Hero setups
    interface.timedOutput("Calculating Dominance for non-heroes... ", DETAILED_OUTPUT, 1, firstDominance == armySize);

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
    interface.timedOutput("Calculating Dominance for heroes... ", DETAILED_OUTPUT, 1);
    // Preselection based on the information that no monster can beat 2 monsters alone if optimizable is true
    // Like the rest of dominance this is unreliable because an aoe hero could easily affect earlier rounds
    if (armySize == (instance.maxCombatants - 1) && optimizable) { // Must be optimizable and the last expansion
        for (i = 0; i < heroMonsterArmiesSize; i++) {
            currentFightResult = &heroMonsterArmies[i].lastFightData;

            currentFightResult->dominated = currentFightResult->rightAoeDamage == 0 && // make sure there is no interference to the optimized calculation
                                            currentFightResult->monstersLost < (int) (instance.targetSize - 2); // Army left at least 2 enemies alive (this is actually checking if 3 alive)
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
                        // even with a strict subset, order is important, so pruning a solution can result in failure to find any solution
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
    vector<MonsterIndex> greedy;
    vector<MonsterIndex> greedyHeroes;
    vector<MonsterIndex> greedyTemp;
    bool invalid = false;

    interface.outputMessage("Trying to find solutions greedily...", DETAILED_OUTPUT);

    // Create Army that kills as many monsters as the army is big
    if (instance.targetSize <= instance.maxCombatants) {
        for (size_t i = 0; i < instance.maxCombatants; i++) {
            for (size_t m = 0; m < availableMonsters.size(); m++) {
                tempArmy = Army(greedy);
                tempArmy.add(availableMonsters[m]);
                if (simulateFight(tempArmy, instance.target) || (tempArmy.lastFightData.monstersLost > (int) i && i+1 < instance.maxCombatants)) { // the last monster has to win the encounter
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
                    if (simulateFight(tempArmy, instance.target)) { // Setup still needs to win
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

    // Get first Upper limit on followers with a greedy algorithm
//    if (instance.maxCombatants > ARMY_MAX_BRUTEFORCEABLE_SIZE) {
//        getQuickSolutions(instance);
//    }

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
//    bool optimizable = (instance.targetSize > ARMY_MAX_BRUTEFORCEABLE_SIZE && instance.targetSize > 3);
//    if (optimizable) {
//        tempArmy = Army({instance.target.monsters[instance.targetSize - 2], instance.target.monsters[instance.targetSize - 1]}); // Make an army from the last two monsters
//    }
//    if (optimizable) { // Check with normal Mobs
//        for (i = 0; i < pureMonsterArmies.size(); i++) {
//            if (simulateFight(pureMonsterArmies[i], tempArmy)) { // Monster won the fight
//                optimizable = false;
//                break;
//            }
//        }
//    }
//    if (optimizable) { // Check with Heroes
//        for (i = 0; i < heroMonsterArmies.size(); i++) {
//            if (simulateFight(heroMonsterArmies[i], tempArmy)) { // Hero won the fight
//                optimizable = false;
//                break;
//            }
//        }
//    }

    // Run the Bruteforce Loop
    startTime = time(NULL);
    for (size_t armySize = 1; armySize <= instance.maxCombatants; armySize++) {
        // Output Debug Information
        interface.outputMessage("Starting loop for armies of size " + to_string(armySize), BASIC_OUTPUT);

        // Run Fights for non-Hero setups
        interface.timedOutput("Simulating " + to_string(pureMonsterArmies.size()) + " non-hero Fights... ", DETAILED_OUTPUT, 1, true);
        simulateMultipleFights(pureMonsterArmies, instance);

        // Run fights for setups with heroes
        interface.timedOutput("Simulating " + to_string(heroMonsterArmies.size()) + " hero Fights... ", DETAILED_OUTPUT, 1);
        simulateMultipleFights(heroMonsterArmies, instance);

        // If we have a valid solution with 0 followers there is no need to continue
        if (!instance.hasWorldBoss && instance.bestSolution.monsterAmount > 0 && instance.bestSolution.followerCost == 0) { break; }

        // Start Expansion routine if there is still room
        if (armySize < instance.maxCombatants) {
            // Manage output format
            if (armySize == firstDominance && config.outputLevel == BASIC_OUTPUT && config.autoAdjustOutputLevel) {
                config.outputLevel = DETAILED_OUTPUT; // Switch output level after pure bruteforce is exhausted
            }
            if (armySize == firstDominance) {
                if (!config.autoAdjustOutputLevel) {
                    interface.finishTimedOutput(DETAILED_OUTPUT);
                }
                interface.outputMessage("", DETAILED_OUTPUT);
                if (!instance.bestSolution.isEmpty()) {
                    interface.outputMessage("Best Solution so far:", DETAILED_OUTPUT);
                    interface.outputMessage(instance.bestSolution.toString(), DETAILED_OUTPUT, 1);
                    if (instance.hasWorldBoss) {
                        interface.outputMessage("Damage Done: " + to_string(WORLDBOSS_HEALTH - instance.lowestBossHealth), DETAILED_OUTPUT, 1);
                    }
                } else {
                    interface.outputMessage("Could not find a solution yet!", DETAILED_OUTPUT);
                }
                if (!iomanager.askYesNoQuestion("Continue calculation?", DETAILED_OUTPUT, TOKENS.YES)) {return;}
                startTime = time(NULL);
                interface.outputMessage("\nPreparing to work on loop for armies of size " + to_string(armySize+1), DETAILED_OUTPUT);
                interface.outputMessage("Currently considering " + to_string(pureMonsterArmies.size()) + " normal and " + to_string(heroMonsterArmies.size()) + " hero armies.", DETAILED_OUTPUT);
            }

            // Calculate which results are strictly better than others (dominance)
            // Reduces memory, but increases calculation time and can result in completely missing a correct solution
//            if (firstDominance <= armySize && availableMonsters.size() > 0) {
//                calculateDominance(instance, optimizable, pureMonsterArmies, heroMonsterArmies, armySize, firstDominance);
//            }

            if (armySize < instance.maxCombatants - 2) {
                // now we expand to add the next monster to all non-dominated armies
                interface.timedOutput("Expanding Lineups by one... ", DETAILED_OUTPUT, 1);
                vector<Army> nextPureArmies;
                vector<Army> nextHeroArmies;
                expand(nextPureArmies, nextHeroArmies, pureMonsterArmies, heroMonsterArmies, armySize, instance);

                interface.timedOutput("Moving Data... ", DETAILED_OUTPUT, 1);
                pureMonsterArmies = move(nextPureArmies);
                heroMonsterArmies = move(nextHeroArmies);
            }
            else {
                // for the second to last expansion, expand and fight each lineups individually (or in small packets) to keep memory usage low
                // some max length solutions will therefore be seen before other solutions of one lower size
                // TODO: refactor this to get rid of code repetition someday
                interface.finishTimedOutput(DETAILED_OUTPUT);
                interface.outputMessage("Starting loop for armies of size " + to_string(armySize + 1) + "+", BASIC_OUTPUT);
                interface.timedOutput("Simulating fights by expanding Lineups one by one ...", DETAILED_OUTPUT, 1, true);

                sort(pureMonsterArmies.begin(), pureMonsterArmies.end(), isMoreEfficient);
                sort(heroMonsterArmies.begin(), heroMonsterArmies.end(), isMoreEfficient);
                for (size_t i = 0, j = 0; i < pureMonsterArmies.size() || j < heroMonsterArmies.size(); ) {
                    vector<Army> tempArmies, pureBranchArmies, heroBranchArmies, pureBranchArmies2, heroBranchArmies2;
                    for (size_t k = 0; k < config.branchwiseExpansionLimit; ++k) {
                        if (i < pureMonsterArmies.size()) pureBranchArmies.push_back(pureMonsterArmies[i++]);
                        if (j < heroMonsterArmies.size()) heroBranchArmies.push_back(heroMonsterArmies[j++]);
                    }
                    expand(pureBranchArmies2, heroBranchArmies2, pureBranchArmies, heroBranchArmies, armySize, instance);
                    simulateMultipleFights(pureBranchArmies2, instance);
                    simulateMultipleFights(heroBranchArmies2, instance);
                    if (!instance.hasWorldBoss && instance.bestSolution.monsterAmount > 0 && instance.bestSolution.followerCost == 0) break;
                    expand(tempArmies, tempArmies, pureBranchArmies2, heroBranchArmies2, armySize + 1, instance);
                    simulateMultipleFights(tempArmies, instance);
                    if (!instance.hasWorldBoss && instance.bestSolution.monsterAmount > 0 && instance.bestSolution.followerCost == 0) break;
                }

                interface.finishTimedOutput(DETAILED_OUTPUT);
                break;
            }
        }
        interface.finishTimedOutput(DETAILED_OUTPUT);
    }
    instance.calculationTime = time(NULL) - startTime;
}

void outputSolution(Instance instance) {
    instance.bestSolution.lastFightData.valid = false;
    bool leftWins = simulateFight(instance.bestSolution, instance.target); // Sanity check on the solution

    bool sane;
    sane = !instance.hasWorldBoss && (leftWins || instance.bestSolution.isEmpty());
    sane |= instance.hasWorldBoss && instance.bestSolution.lastFightData.frontHealth == instance.lowestBossHealth;

    if (config.JSONOutput) {
        interface.outputMessage(makeJSONFromInstance(instance, sane), SOLUTION_OUTPUT);
    } else {
        interface.outputMessage(makeStringFromInstance(instance, sane, config.showReplayStrings), SOLUTION_OUTPUT);
    }
}

int main(int argc, char** argv) {
    // Declare Variables
    FollowerCount minimumMonsterCost;
    FollowerCount userFollowerUpperBound;
    vector<Instance> instances;
    bool userWantsContinue;

    if (argc >= 3 && (string) argv[2] == "-server") {
        config.showQueries = false;
        config.ignoreQuestions = true;
        config.JSONOutput = true;
        config.outputLevel = SOLUTION_OUTPUT;
        config.ignoreExecutionHalt = true;
        config.allowConfig = false;
    }

    interface.outputMessage(welcomeMessage + " v" + VERSION, NOTIFICATION_OUTPUT);
    interface.outputMessage(helpMessage + "\n", NOTIFICATION_OUTPUT);

    // Check if the user provided a filename to be used as an inputfile
    if (argc >= 2) {
        iomanager.loadInputFiles(argv[1]);
    } else {
        iomanager.loadInputFiles("");
    }
    interface.outputMessage("", NOTIFICATION_OUTPUT);
    iomanager.getConfiguration();

    // Initialize global Data
    initGameData();

    // -------------------------------------------- Program Start --------------------------------------------

    if (config.individualBattles) {
        interface.outputMessage("Simulating individual Fights", NOTIFICATION_OUTPUT);
        while (true) {
            Army left = iomanager.takeInstanceInput("Enter friendly lineup: ")[0].target;
            Army right = iomanager.takeInstanceInput("Enter hostile lineup: ")[0].target;
            bool leftWins = simulateFight(left, right, true);
            interface.outputMessage(to_string(leftWins) + " " + to_string(left.followerCost) + " " + to_string(right.followerCost), SOLUTION_OUTPUT);

            if (!iomanager.askYesNoQuestion("Simulate another Fight?", NOTIFICATION_OUTPUT, TOKENS.NO)) {
                break;
            }
        }
        return EXIT_SUCCESS;
    }
    // Collect the Data via Command Line
    availableHeroes = iomanager.takeHerolevelInput();
    int64_t minFollowerTemp = parseInt(iomanager.getResistantInput("Set a lower follower limit on monsters used: ", integer)[0]);
    int64_t maxFollowerTemp = parseInt(iomanager.getResistantInput("Set an upper follower limit that you want to use: ", integer)[0]);

    if (maxFollowerTemp == 0) maxFollowerTemp = 1; // 0 will cause issues with finding solutions for pure hero armies

    if (minFollowerTemp < 0) {
        minimumMonsterCost = numeric_limits<FollowerCount>::max();
    } else {
        minimumMonsterCost = (FollowerCount) minFollowerTemp; // should not overflow due to parseInt
    }
    if (maxFollowerTemp < 0) {
        userFollowerUpperBound = numeric_limits<FollowerCount>::max();
    } else {
        userFollowerUpperBound = (FollowerCount) maxFollowerTemp; // should not overflow due to parseInt
    }

    // Fill monster arrays with relevant monsters
    filterMonsterData(minimumMonsterCost, userFollowerUpperBound);

    do {
        instances = iomanager.takeInstanceInput("Enter Enemy Lineup(s): ");

        interface.outputMessage("\nCalculating with " + to_string(availableMonsters.size()) + " available Monsters and " + to_string(availableHeroes.size()) + " enabled Heroes.", BASIC_OUTPUT);

        if (config.outputLevel == DETAILED_OUTPUT && config.autoAdjustOutputLevel) {
            config.outputLevel = BASIC_OUTPUT;
        }
        if (config.outputLevel == BASIC_OUTPUT && instances.size() > 1 && config.autoAdjustOutputLevel) {
            config.outputLevel = SOLUTION_OUTPUT;
        }

        for (size_t i = 0; i < instances.size(); i++) {
            totalFightsSimulated = &(instances[i].totalFightsSimulated);

            instances[i].followerUpperBound = userFollowerUpperBound;

            solveInstance(instances[i], config.firstDominance);
            outputSolution(instances[i]);
        }
        userWantsContinue = iomanager.askYesNoQuestion("Do you want to calculate more lineups?", NOTIFICATION_OUTPUT, TOKENS.NO);
    } while (userWantsContinue);

    interface.outputMessage("", NOTIFICATION_OUTPUT);
    interface.haltExecution();
    return EXIT_SUCCESS;
}
