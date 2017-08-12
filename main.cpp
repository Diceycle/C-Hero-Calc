// DavidL1450 - solves PvE instances in Cosmos Quest - now with heroes
// Brainisdead variant, added names of new heroes and input info comment
// input now needs to be separated by commas. To enter heroes, do <name>:<level>, for example:
// a2,e3,lady of twilight:1,e4
// Clean-up and optimization done by Diceycle

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <ctime>
#include <limits>

#include "inputProcessing.h"
#include "cosmosDefines.h"
#include "battleLogic.h"

using namespace std;

// Define global variables used to track the best result
int followerUpperBound;
Army best;

// Simulates fights with all armies against the target. armies will contain armies with the results written in.
void simulateMultipleFights(vector<Army> & armies, Army & target) {
    for (size_t i = 0; i < armies.size(); i++) {
        simulateFight(armies[i], target);
        if (!armies[i].lastFightData.rightWon) {  // left (our side) wins:
            if (armies[i].followerCost < followerUpperBound) {
                followerUpperBound = armies[i].followerCost;
                best = Army(armies[i].monsters);
//                cout << endl << "    New Solution: " << endl << "  "; 
//                best.print();
            }
        }
    }
}

// TODO: Fix Bug that might causes geror to be undervalued because of precomputed fights
void expand(vector<Army> & newPureArmies, vector<Army> & newHeroArmies, 
            vector<Army> & oldPureArmies, vector<Army> & oldHeroArmies, 
            const vector<Monster *> & availableMonsters, const vector<Monster *> & availableHeroes,
            size_t currentArmySize, size_t maxArmySize) {

    int remainingFollowers;
    size_t availableMonstersSize = availableMonsters.size();
    size_t availableHeroesSize = availableHeroes.size();
    size_t i, j, m;
    bool heroUsed;
    
    for (i = 0; i < oldPureArmies.size(); i++) {
        if (!oldPureArmies[i].lastFightData.dominated) {
            remainingFollowers = followerUpperBound - oldPureArmies[i].followerCost;
            for (m = 0; m < availableMonstersSize && availableMonsters[m]->cost < remainingFollowers; m++) {
                newPureArmies.push_back(oldPureArmies[i]);
                newPureArmies.back().add(availableMonsters[m]);
                newPureArmies.back().lastFightData.valid = true;
            }
            for (m = 0; m < availableHeroesSize; m++) {
                newHeroArmies.push_back(oldPureArmies[i]);
                newHeroArmies.back().add(availableHeroes[m]);
                newHeroArmies.back().lastFightData.valid = false; // Refine PF Stuff
            }
        }
    }
    
    for (i = 0; i < oldHeroArmies.size(); i++) {
        if (!oldHeroArmies[i].lastFightData.dominated) {
            remainingFollowers = followerUpperBound - oldHeroArmies[i].followerCost;
            for (m = 0; m < availableMonstersSize && availableMonsters[m]->cost < remainingFollowers; m++) {
                newHeroArmies.push_back(oldHeroArmies[i]);
                newHeroArmies.back().add(availableMonsters[m]);
                newHeroArmies.back().lastFightData.valid = true;
            }
            for (m = 0; m < availableHeroesSize; m++) {
                heroUsed = false;
                for (j = 0; j < currentArmySize; j++) {
                    if (oldHeroArmies[i].monsters[j] == availableHeroes[m]) {
                        heroUsed = true;
                        break;
                    }
                }
                if (!heroUsed) {
                    newHeroArmies.push_back(oldHeroArmies[i]);
                    newHeroArmies.back().add(availableHeroes[m]);
                    newHeroArmies.back().lastFightData.valid = false; // Refine PF Stuff
                }
            }
        }
    }
}


// Use a greedy method to get a first upper bound on follower cost for the solution
// TODO: Think of an algorithm that works on limit < targetsize semi-reliable
void getQuickSolutions(const vector<Monster *> & availableHeroes, Army target, size_t limit) {
    Army tempArmy = Army();
    vector<Monster *> greedy {};
    vector<Monster *> greedyHeroes {};
    vector<Monster *> greedyTemp {};
    size_t targetSize = target.monsters.size();
    bool invalid = false;
    
    cout << "Trying to find solutions greedily..." << endl;
    
    // Create Army that kills as many monsters as the army is big
    if (targetSize <= limit) {
        for (size_t i = 0; i < limit; i++) {
            for (size_t m = 0; m < monsterList.size(); m++) {
                tempArmy = Army(greedy);
                tempArmy.add(monsterList[m]);
                simulateFight(tempArmy, target);
                if (!tempArmy.lastFightData.rightWon || (tempArmy.lastFightData.monstersLost > i && i+1 < limit)) { // the last monster has to win the encounter
                    greedy.push_back(monsterList[m]);
                    break;
                }
            }
            invalid = greedy.size() < limit;
        }
        if (!invalid) {
            cout << "  ";
            tempArmy.print();
            best = tempArmy;
            followerUpperBound = tempArmy.followerCost;
            
            // Try to replace monsters in the setup with heroes to save followers
            greedyHeroes = greedy;
            for (size_t m = 0; m < availableHeroes.size(); m++) {
                for (size_t i = 0; i < greedyHeroes.size(); i++) {
                    greedyTemp = greedyHeroes;
                    greedyTemp[i] = availableHeroes[m];
                    tempArmy = Army(greedyTemp);
                    simulateFight(tempArmy, target);
                    if (!tempArmy.lastFightData.rightWon) { // Setup still needs to win
                        greedyHeroes = greedyTemp;
                        break;
                    }
                }
            }
            cout << "  ";
            tempArmy = Army(greedyHeroes);
            best = tempArmy;
            followerUpperBound = tempArmy.followerCost;
            tempArmy.print();
        } else {
            cout << "  Could not find valid solution while being greedy" << endl;
        }
    } else {
        cout << "  Could not find valid solution while being greedy" << endl;
    }
}

int solveInstance(const vector<Monster *> & availableHeroes, Army target, size_t limit, bool debugInfo) {
    Army tempArmy = Army();
    int startTime;
    int tempTime;
    
    size_t i, j, sj, si;

    // Get first Upper limit on followers
    getQuickSolutions(availableHeroes, target, limit);
    if (!askYesNoQuestion("Continue calculation?")) {return 0;}
    cout << endl;
    
    vector<Army> pureMonsterArmies {}; // initialize with all monsters
    vector<Army> heroMonsterArmies {}; // initialize with all heroes
    for (i = 0; i < monsterList.size(); i++) {
        if (monsterList[i]->cost <= followerUpperBound) {
            pureMonsterArmies.push_back(Army(vector<Monster *>{monsterList[i]}));
        }
    }
    for (i = 0; i < availableHeroes.size(); i++) { // Ignore chacking for Hero Cost
        heroMonsterArmies.push_back(Army(vector<Monster *>{availableHeroes[i]}));
    }
    
    // Check if a single monster can beat the last two monsters of the target. If not, solutions that can only beat n-2 monsters need not be expanded later
    bool optimizable = (target.monsters.size() >= 3);
    if (optimizable) {
        tempArmy = Army({target.monsters[target.monsters.size() - 2], target.monsters[target.monsters.size() - 1]}); // Make an army from the last two monsters
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
    size_t targetSize = target.monsters.size();
    size_t pureMonsterArmiesSize, heroMonsterArmiesSize;
    for (size_t armySize = 1; armySize <= limit; armySize++) {
    
        pureMonsterArmiesSize = pureMonsterArmies.size();
        heroMonsterArmiesSize = heroMonsterArmies.size();
        // Output Debug Information
        debugOutput(tempTime, "Starting loop for armies of size " + to_string(armySize), true, false, true);
        
        // Run Fights for non-Hero setups
        debugOutput(tempTime, "  Simulating " + to_string(pureMonsterArmiesSize) + " non-hero Fights... ", debugInfo, false, false);
        tempTime = time(NULL);
        simulateMultipleFights(pureMonsterArmies, target);
        
        // Run fights for setups with heroes
        debugOutput(tempTime, "  Simulating " + to_string(heroMonsterArmiesSize) + " hero Fights... ", debugInfo, true, false);
        tempTime = time(NULL);
        simulateMultipleFights(heroMonsterArmies, target);
        
        if (armySize < limit) { 
            // Sort the results by follower cost for some optimization
            debugOutput(tempTime, "  Sorting List... ", debugInfo, true, false);
            tempTime = time(NULL);
            sort(pureMonsterArmies.begin(), pureMonsterArmies.end(), hasFewerFollowers);
            sort(heroMonsterArmies.begin(), heroMonsterArmies.end(), hasFewerFollowers);
            
            // Calculate which results are strictly better than others (dominance)
            debugOutput(tempTime, "  Calculating Dominance for non-heroes... ", debugInfo, true, false);
            tempTime = time(NULL);
            
            int leftFollowerCost;
            FightResult * currentFightResult;
            vector<Monster *> leftHeroList {};
            size_t leftHeroListSize;
            Monster * rightMonster;
            Monster * leftMonster;
            // First Check dominance for non-Hero setups
            for (i = 0; i < pureMonsterArmiesSize; i++) {
                leftFollowerCost = pureMonsterArmies[i].followerCost;
                currentFightResult = &pureMonsterArmies[i].lastFightData;
                // A result is obsolete if only one expansion is left but no single mob can beat the last two enemy mobs alone (optimizable)
                if (armySize == (limit - 1) && optimizable) {
                    // TODO: Investigate whether this is truly correct: What if the second-to-last mob is already damaged (not from aoe) i.e. it defeated the last mob of left?
                    if (currentFightResult->rightWon && currentFightResult->monstersLost < targetSize - 2 && currentFightResult->rightAoeDamage == 0) {
                        currentFightResult->dominated = true;
                    }
                }
                // A result is dominated If:
                if (!currentFightResult->dominated) { 
                    // Another pureResults got farther with a less costly lineup
                    for (j = i+1; j < pureMonsterArmiesSize; j++) {
                        if (leftFollowerCost < pureMonsterArmies[j].followerCost) {
                            break; 
                        } else if (*currentFightResult <= pureMonsterArmies[j].lastFightData) { // pureResults[i] has more followers implicitly 
                            currentFightResult->dominated = true;
                            break;
                        }
                    }
                    // A lineup without heroes is better than a setup with heroes even if it got just as far
                    for (j = 0; j < heroMonsterArmiesSize; j++) {
                        if (leftFollowerCost > heroMonsterArmies[j].followerCost) {
                            break; 
                        } else if (*currentFightResult >= heroMonsterArmies[j].lastFightData) { // pureResults[i] has less followers implicitly
                            heroMonsterArmies[j].lastFightData.dominated = true;
                        }                       
                    }
                }
            }
            
            debugOutput(tempTime, "  Calculating Dominance for heroes... ", debugInfo, true, false);
            tempTime = time(NULL);
            // Domination for setups with heroes
            bool usedHeroSubset, leftUsedHero;
            for (i = 0; i < heroMonsterArmiesSize; i++) {
                leftFollowerCost = heroMonsterArmies[i].followerCost;
                currentFightResult = &heroMonsterArmies[i].lastFightData;
                leftHeroList.clear();
                for (si = 0; si < armySize; si++) {
                    leftMonster = heroMonsterArmies[i].monsters[si];
                    if (leftMonster->isHero) {
                        leftHeroList.push_back(leftMonster);
                    }
                }
                leftHeroListSize = leftHeroList.size();
                
                // A result is obsolete if only one expansion is left but no single mob can beat the last two enemy mobs alone (optimizable)
                if (armySize == (limit-1) && optimizable && currentFightResult->rightAoeDamage == 0) {
                    // TODO: Investigate whether this is truly correct: What if the second-to-last mob is already damaged (not from aoe) i.e. it defeated the last mob of left?
                    if (currentFightResult->rightWon && currentFightResult->monstersLost < targetSize - 2){
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
                                if (rightMonster->isHero) { // mob is a hero
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
            
            // now we expand to add the next monster to all non-dominated armies
            debugOutput(tempTime, "  Expanding Lineups by one... ", debugInfo, true, false);
            tempTime = time(NULL);
            
            vector<Army> nextPureArmies;
            vector<Army> nextHeroArmies;
            expand(nextPureArmies, nextHeroArmies, pureMonsterArmies, heroMonsterArmies,
                    monsterList, availableHeroes, armySize, limit);

            debugOutput(tempTime, "  Moving Data... ", debugInfo, true, false);
            tempTime = time(NULL);
            
            pureMonsterArmies = move(nextPureArmies);
            heroMonsterArmies = move(nextHeroArmies);
        }
        debugOutput(tempTime, "", true, true, true);
    }
    return time(NULL) - startTime;
}

int main(int argc, char** argv) {
    // Initialize global Data
    followerUpperBound = numeric_limits<int>::max();
    best = Army();
    initMonsterData();
    
    // Declare Variables
    vector<Monster *> friendLineup {};
    vector<Monster *> hostileLineup {};
    string inputString;
    vector<int> yourHeroLevels;
    
    // Additional convienience Strings
    vector<string> daily {"w10", "e10", "a10", "w10", "shaman:99"};
    vector<string> test3 {"a9", "f8", "a8"}; 
    // Declare Hero Levels
    // INPUT YOUR HERO LEVELS HERE (For manual editing: Names tell you which number is the level of which hero)
    yourHeroLevels = { 0, 0, 0, 0,      // "lady of twilight","tiny","nebra","james"
                       0, 0, 0,         // "hunter","shaman","alpha"
                       0, 0, 0,         // "carl","nimue","athos"
                       0, 0, 0,         // "jet","geron","rei"
                       0, 0, 0,         // "ailen","faefyr","auri"
                       0, 0, 0,         // "k41ry", "t4urus", "tr0n1x"
                       0, 0, 0,         // "aquortis", "aeris", "geum"
                       0, 0, 0,         // "rudean","aural","geror"
                       0, 0, 0, 0,      // "valor","rokka","pyromancer","bewat"
                       0, 0, 0, 0       // "nicte", "forest druid","ignitor","undine"
    }; 
    
    // Use these variables to specify the fight
    bool ignoreConsole = false;                         // Disables the console question whether you want to read from file or command line
    int limit = 6;                                      // Set this to how many Monsters should be in the solution (f.e 4 for X-3 Quests) 
    hostileLineup = makeMonstersFromStrings(quests[1]); // Choose against which lineup you want to fight use one from above or make your own and then change the name accordingly
    bool individual = false;                            // Set this to true if you want to simulate individual fights (lineups will be promted when you run the program)
    bool debugInfo = true;                              // Set this to true if you want to see how far the execution is and how lone the execution took altogether
    bool manualInput = false;                           // Set this to true if you want nothing to do with this file and just want to input stuff over the command line like you're used to
    
    cout << "Welcome to Diceycle's PvE Instance Solver!" << endl;
    
    if (!ignoreConsole) {
        manualInput = askYesNoQuestion("Do you want to input everything via command line?");
    }
    // Collect the Data via Command Line if the user wants
    if (manualInput) {
        try {
            yourHeroLevels = takeHerolevelInput();
        } catch (const exception & e) {
            haltExecution();
            return EXIT_FAILURE;
        }
        hostileLineup = takeLineupInput("Enter Enemy Lineup");
        cout << "Enter how many monsters are allowed in the solution" << endl;
        getline(cin, inputString);
        limit = stoi(inputString);
    } else {
        cout << "Taking data from script" << endl;
    }
    
    initializeUserHeroes(yourHeroLevels);
    
    
    if (individual) { // custom input mode
        cout << "Simulating individual Figths" << endl;
        while (true) {
            friendLineup = takeLineupInput("Enter friendly lineup:");
            hostileLineup = takeLineupInput("Enter hostile lineup");
            
            Army left = Army(friendLineup);
            Army right = Army(hostileLineup);
            simulateFight(left, right, true);
            cout << left.lastFightData.rightWon << " " << left.followerCost << " " << right.followerCost << endl;
            
            if (!askYesNoQuestion("Simulate another Fight?")) {
                break;
            }
        }
        return 0;
    }
    
    int totalTime;
    Army hostileArmy = Army(hostileLineup);
    totalTime = solveInstance(availableHeroes, hostileArmy, limit, debugInfo);
    // Last check to see if winning combination wins:
    if (followerUpperBound < numeric_limits<int>::max()) {
        best.lastFightData.valid = false;
        simulateFight(best, hostileArmy);
        if (best.lastFightData.rightWon) {
            best.print();
            cout << "This does not beat the lineup!!!" << endl;
            for (int i = 1; i <= 10; i++) {
                cout << "ERROR";
            }
            haltExecution();
            return EXIT_FAILURE;
            
        } else {
            // Print the winning combination!
            cout << endl << "The optimal combination is:" << endl << "  ";
            best.print();
            cout << "  (Right-most fights first)" << endl;
        }
    } else {
        cout << endl << "Could not find a solution that beats this lineup." << endl;
    }
    
    cout << endl;
    cout << totalFightsSimulated << " Fights simulated." << endl;
    cout << "Total Calculation Time: " << totalTime << endl;
    haltExecution();
    return EXIT_SUCCESS;
}