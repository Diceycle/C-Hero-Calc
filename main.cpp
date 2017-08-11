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

// Simulates all of the fights from armies and puts it into a vector of fightResults.
void simulateMultipleFights(vector<FightResult> & results, vector<Army> & armies, Army & target) {
    results.reserve(armies.size());
    FightResult currentResult;
    
    for (size_t i = 0; i < armies.size(); i++) {
        simulateFight(currentResult, armies[i], target);
        currentResult.source = &armies[i];
        if (!currentResult.rightWon) {  // left (our side) wins:
            if (currentResult.source->followerCost < followerUpperBound) {
                followerUpperBound = currentResult.source->followerCost;
                best = Army();
//                cout << endl << "    New Solution: "; 
                for(size_t j = 0; j < armies[i].monsters.size(); j++) {
                    best.add(armies[i].monsters[j]);
//                    cout << armies[i].monsters[j]->name << " ";
                } 
//                cout << currentResult.source->followerCost;
            }
        } else { // only look at results that havent won yet
            results.push_back(currentResult);
        }
    }
}

void expand(vector<Army> & newArmies, vector<FightResult> & fightResults, const vector<Monster *> & toExpandWith, bool checkHeroRepeat) {
    // expand stuff directly onto source, does not expand dominated or winning or too costly armies
    // if the bool is true, we check if we repeat any heroes
    FightResult * currentResult;
    bool heroUseable;
    size_t i, j, k;
    
    for (i = 0; i < fightResults.size(); i++) {
        currentResult = &fightResults[i]; 
        if (!currentResult->dominated) { // if this setup isn't worse than another and hasn't won yet
            for (j = 0; j < toExpandWith.size(); j++) { // expand this setup with all available mobs
                if (currentResult->source->followerCost + toExpandWith[j]->cost < followerUpperBound) {
                    
                    heroUseable = true;
                    if (checkHeroRepeat) { // check if a hero has been used before
                        for (k = 0; k < currentResult->source->monsters.size(); k++) {
                            if (currentResult->source->monsters[k] == toExpandWith[j]) { // since were only expanding with heroes in this case we can compare normal mobs too
                                heroUseable = false;
                                break;
                            }
                        }
                    }
                    if (heroUseable) {
                        newArmies.push_back(*(currentResult->source));
                        newArmies[newArmies.size() - 1].add(toExpandWith[j]);
                        if (!toExpandWith[j]->isHero) {
                            newArmies[newArmies.size()-1].precomputedFight = KnownFight({currentResult->monstersLost, 
                                                                                         currentResult->damage, 
                                                                                         currentResult->berserk, 
                                                                                         currentResult->leftAoeDamage, 
                                                                                         currentResult->rightAoeDamage, 
                                                                                         true}); // pre-computed fight
                        } else {
                            newArmies[newArmies.size()-1].precomputedFight.valid = false;
                        }
                    }
                } else {
                    break; // since list is sorted by follower cost (and heroes don't cost anything)
                }
            }
        }
    }
}

// Use a greedy method to get a first upper bound on follower cost for the solution
// TODO: Think of an algorithm that works on limit < targetsize semi-reliable
void getQuickSolutions(const vector<Monster *> & availableHeroes, Army target, size_t limit) {
    Army tempArmy = Army();
    FightResult tempResult;
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
                simulateFight(tempResult, tempArmy, target);
                if (!tempResult.rightWon || (tempResult.monstersLost > i && i+1 < limit)) { // the last monster has to win the encounter
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
                    simulateFight(tempResult, tempArmy, target);
                    if (!tempResult.rightWon) { // Setup still needs to win
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

void solveInstance(const vector<Monster *> & availableHeroes, Army target, size_t limit, bool debugInfo) {
    Army tempArmy = Army();
    FightResult tempResult;
    int startTime;
    int tempTime;
    
    size_t i, j, sj, si, c;

    // Get first Upper limit on followers
    getQuickSolutions(availableHeroes, target, limit);
    if (!askYesNoQuestion("Continue calculation?")) {return;}
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
            simulateFight(tempResult, pureMonsterArmies[i], tempArmy);
            if (!tempResult.rightWon) { // Monster won the fight
                optimizable = false;
                break;
            }
        }
    }

    if (optimizable) { // Check with Heroes
        for (i = 0; i < heroMonsterArmies.size(); i++) {
            simulateFight(tempResult, heroMonsterArmies[i], tempArmy);
            if (!tempResult.rightWon) { // Hero won the fight
                optimizable = false;
                break;
            }
        }
    }

    // Run the Bruteforce Loop
    startTime = time(NULL);
    tempTime = startTime;
    size_t armySize = 0;
    size_t targetSize = target.monsters.size();
    size_t fightResultAmount, herofightResultAmount;
    for (c = 1; c <= limit; c++) { // c is the length of the list of monsters
        armySize = c;
    
        // Output Debug Information
        debugOutput(tempTime, "Starting loop for armies of size " + to_string(c), true, false, true);
        
        // Run Fights for non-Hero setups
        debugOutput(tempTime, "  Simulating " + to_string(pureMonsterArmies.size()) + " non-hero Fights... ", debugInfo, false, false);
        tempTime = time(NULL);
        vector<FightResult> pureResults;
        simulateMultipleFights(pureResults, pureMonsterArmies, target);
        
        // Run fights for setups with heroes
        debugOutput(tempTime, "  Simulating " + to_string(heroMonsterArmies.size()) + " hero Fights... ", debugInfo, true, false);
        tempTime = time(NULL);
        vector<FightResult> heroResults;
        simulateMultipleFights(heroResults, heroMonsterArmies, target);
        
        fightResultAmount = pureResults.size();
        herofightResultAmount = heroResults.size();
        if (c < limit) { 
            // Sort the results by follower cost for some optimization
            debugOutput(tempTime, "  Sorting List... ", debugInfo, true, false);
            tempTime = time(NULL);
            sort(pureResults.begin(), pureResults.end(), hasFewerFollowers);
            sort(heroResults.begin(), heroResults.end(), hasFewerFollowers);
            
            // Calculate which results are strictly better than others (dominance)
            debugOutput(tempTime, "  Calculating Dominance for non-heroes... ", debugInfo, true, false);
            tempTime = time(NULL);
            
            int leftFollowerCost;
            vector<Monster *> leftHeroList {};
            size_t leftHeroListSize;
            Monster * rightMonster;
            Monster * leftMonster;
            // First Check dominance for non-Hero setups
            for (i = 0; i < fightResultAmount; i++) {
                leftFollowerCost = pureResults[i].source->followerCost;
                // A result is obsolete if only one expansion is left but no single mob can beat the last two enemy mobs alone (optimizable)
                if (c == (limit - 1) && optimizable) {
                    // TODO: Investigate whether this is truly correct: What if the second-to-last mob is already damaged (not from aoe) i.e. it defeated the last mob of left?
                    if (pureResults[i].rightWon && pureResults[i].monstersLost < targetSize - 2 && pureResults[i].rightAoeDamage == 0) {
                        pureResults[i].dominated = true;
                    }
                }
                // A result is dominated If:
                if (!pureResults[i].dominated) { 
                    // Another pureResults got farther with a less costly lineup
                    for (j = i+1; j < fightResultAmount; j++) {
                        if (leftFollowerCost < pureResults[j].source->followerCost) {
                            break; 
                        } else if (pureResults[i] <= pureResults[j]) { // pureResults[i] has more followers implicitly 
                            pureResults[i].dominated = true;
                            break;
                        }
                    }
                    // A lineup without heroes is better than a setup with heroes even if it got just as far
                    for (j = 0; j < herofightResultAmount; j++) {
                        if (leftFollowerCost > heroResults[j].source->followerCost) {
                            break; 
                        } else if (pureResults[i] >= heroResults[j] ) { // pureResults[i] has less followers implicitly
                            heroResults[j].dominated = true;
                        }                       
                    }
                }
            }
            
            debugOutput(tempTime, "  Calculating Dominance for heroes... ", debugInfo, true, false);
            tempTime = time(NULL);
            // Domination for setups with heroes
            bool usedHeroSubset, leftUsedHero;
            for (i = 0; i < herofightResultAmount; i++) {
                leftFollowerCost = heroResults[i].source->followerCost;
                leftHeroList.clear();
                for (si = 0; si < armySize; si++) {
                    leftMonster = heroResults[i].source->monsters[si];
                    if (leftMonster->isHero) {
                        leftHeroList.push_back(leftMonster);
                    }
                }
                leftHeroListSize = leftHeroList.size();
                
                // A result is obsolete if only one expansion is left but no single mob can beat the last two enemy mobs alone (optimizable)
                if (c == (limit-1) && optimizable && heroResults[i].rightAoeDamage == 0) {
                    // TODO: Investigate whether this is truly correct: What if the second-to-last mob is already damaged (not from aoe) i.e. it defeated the last mob of left?
                    if (heroResults[i].rightWon && heroResults[i].monstersLost < targetSize - 2){
                        heroResults[i].dominated = true;
                    }
                }
                
                // A result is dominated If:
                if (!heroResults[i].dominated) {
                    // if i costs more followers and got less far than j, then i is dominated
                    for (j = i+1; j < herofightResultAmount; j++) {
                        if (leftFollowerCost < heroResults[j].source->followerCost) {
                            break;
                        } else if (heroResults[i] <= heroResults[j]) { // i has more followers implicitly
                            usedHeroSubset = true; // If j doesn't use a strict subset of the heroes i used, it cannot dominate i
                            for (sj = 0; sj < armySize; sj++) { // for every hero in j there must be the same hero in i
                                leftUsedHero = false; 
                                rightMonster = heroResults[j].source->monsters[sj];
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
                                heroResults[i].dominated = true;
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
            expand(nextPureArmies, pureResults, monsterList, false);
            vector<Army> nextHeroArmies;
            expand(nextHeroArmies, heroResults, monsterList, false);
            expand(nextHeroArmies, pureResults, availableHeroes, false);
            expand(nextHeroArmies, heroResults, availableHeroes, true);
            debugOutput(tempTime, "  Moving Data... ", debugInfo, true, false);
            tempTime = time(NULL);
            
            pureMonsterArmies = move(nextPureArmies);
            heroMonsterArmies = move(nextHeroArmies);
        }
        debugOutput(tempTime, "", true, true, true);
    }
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
        hostileLineup = takeLineupInput();
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
            cout << "Friendly Lineup" << endl;
            friendLineup = takeLineupInput();
            
            cout << "Hostile Lineup" << endl;
            hostileLineup = takeLineupInput();
            
            FightResult customResult;
            Army left = Army(friendLineup);
            Army right = Army(hostileLineup);
            simulateFight(customResult, left, right, true);
            cout << customResult.rightWon << " " << left.followerCost << " " << right.followerCost << endl;
            
            if (!askYesNoQuestion("Simulate another Fight?")) {
                break;
            }
        }
        return 0;
    }
    
    int startTime = time(NULL);
    Army hostileArmy = Army(hostileLineup);
    solveInstance(availableHeroes, hostileArmy, limit, debugInfo);
    // Last check to see if winning combination wins:
    if (followerUpperBound < numeric_limits<int>::max()) {
        best.precomputedFight.valid = false;
        FightResult finalResult;
        simulateFight(finalResult, best, hostileArmy);
        if (finalResult.rightWon) {
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
    cout << "Total Calculation Time: " << time(NULL) - startTime << endl;
    haltExecution();
    return EXIT_SUCCESS;
}