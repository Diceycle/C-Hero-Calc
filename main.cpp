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
#include <cmath>
#include <limits>

#include "inputProcessing.h"
#include "cosmosDefines.h"
#include "inputProcessing.cpp"
#include "cosmosClasses.cpp"

using namespace std;

int totalFightsSimulated;

// Define global variables used to track the best result
int followerUpperBound;
Army best;

vector<Monster> heroReference; // Will be filled with leveled heroes if needed (determined by input)

vector<Monster *> monsterList; // Contains pointers to raw Monster Data from a1 to f10
map<string, Monster *> monsterMap; // Maps monster Names to their pointers (includes heroes)

// Function for sorting FightResults by followers (ascending)
bool hasFewerFollowers(FightResult & a, FightResult & b) { // used for sorting.
    return (a.source->followerCost < b.source->followerCost);
}

// Function for sorting Monsters by cost (ascending)
bool isCheaper(Monster * a, Monster * b) {
    return a->cost < b->cost;
}

// Function determining if a monster is strictly better than another
bool isBetter(Monster * a, Monster * b, bool considerAbilities = false) {
    if (a->element == b->element) {
        return (a->damage >= b->damage) && (a->hp >= b->hp);
    } else { // a needs to be better than b even when b has elemental advantage, or a is at disadvantage
        return !considerAbilities && (a->damage >= b->damage * elementalBoost) && (a->hp >= b->hp * elementalBoost);
    }
}

// TODO: Implement MAX AOE DAmage to make sure nothing gets revived
void simulateFight(FightResult & result, Army & left, Army & right, bool verbose = false) {
    //fights left to right, so left[0] and right[0] are the first to fight
    // Damage Application Order:
    //  1. Base Damage of creature
    //  2. Multiplicators of self       (friends, berserk)
    //  3. Buffs from heroes            (Hunter, Rei, etc.)
    //  4. Elemental Advantage          (f.e. Fire vs. Earth)
    //  5. Protection of enemy Side     (Nimue, Athos, etc.)
    //  6. AOE of friendly Side         (Tiny, Alpha, etc.)
    //  7. Healing of enemy Side        (Auri, Aeris, etc.)
    
    totalFightsSimulated++;
    
    size_t leftLost = 0;
    size_t leftArmySize = left.monsters.size();
    vector<Monster *> & leftLineup = left.monsters;
    
    int leftFrontDamageTaken = 0;
    int leftHealing = 0;
    int leftCumAoeDamageTaken = 0;
    float leftBerserkMult = 1;
    
    size_t rightLost = 0;
    size_t rightArmySize = right.monsters.size();
    vector<Monster *> & rightLineup = right.monsters;
    
    int rightFrontDamageTaken = 0;
    int rightHealing = 0;
    int rightCumAoeDamageTaken = 0;
    float rightBerserkMult = 1;
    
    // If no heroes are in the army the result from the smaller army is still valid
    if (left.precomputedFight.valid && !verbose) { 
        // Set pre-computed values to pick up where we left off
        leftLost                = leftArmySize-1; // All monsters of left died last fight only the new one counts
        leftFrontDamageTaken    = left.precomputedFight.leftAoeDamage;
        leftCumAoeDamageTaken   = left.precomputedFight.leftAoeDamage;
        rightLost               = left.precomputedFight.monstersLost;
        rightFrontDamageTaken   = left.precomputedFight.damage;
        rightCumAoeDamageTaken  = left.precomputedFight.rightAoeDamage;
        rightBerserkMult        = left.precomputedFight.berserk;
    }
    
    // Values for skills  
    int damageLeft, damageRight;
    int damageBuffLeft, damageBuffRight;
    int protectionLeft, protectionRight;
    int aoeDamageLeft, aoeDamageRight;
    int paoeDamageLeft, paoeDamageRight;
    int healingLeft, healingRight;
    int pureMonstersLeft, pureMonstersRight;
    int elementalDifference;
    
    // hero temp Variables
    Monster * currentMonsterLeft;
    Monster * currentMonsterRight;
    HeroSkill * skill;
    SkillType skillType;
    Element skillTarget;
    
    while (true) {
        // Get all hero influences
        damageBuffLeft = 0;
        protectionLeft = 0;
        aoeDamageLeft = 0;
        paoeDamageLeft = 0;
        healingLeft = 0;
        pureMonstersLeft = 0;
        for (size_t i = leftLost; i < leftArmySize; i++) {
            if (leftCumAoeDamageTaken >= leftLineup[i]->hp) {
                leftLost += (leftLost == i);
            } else {
                skill = &leftLineup[i]->skill;
                skillType = skill->type;
                skillTarget = skill->target;
                if (skillType == nothing) {
                    pureMonstersLeft++;
                } else if (skillType == protect && (skillTarget == all || skillTarget == leftLineup[leftLost]->element)) {
                    protectionLeft += skill->amount;
                } else if (skillType == buff && (skillTarget == all || skillTarget == leftLineup[leftLost]->element)) {
                    damageBuffLeft += skill->amount;
                } else if (skillType == heal) {
                    healingLeft += skill->amount;
                } else if (skillType == aoe) {
                    aoeDamageLeft += skill->amount;
                } else if (skillType == pAoe && i == leftLost) {
                    paoeDamageLeft += leftLineup[i]->damage;
                }
            }
        }
        
        damageBuffRight = 0;
        protectionRight = 0;
        aoeDamageRight = 0;
        paoeDamageRight = 0;
        healingRight = 0;
        pureMonstersRight = 0;
        for (size_t i = rightLost; i < rightArmySize; i++) {
            if (rightCumAoeDamageTaken >= rightLineup[i]->hp) {
                rightLost += (i == rightLost);
            } else {
                skill = &rightLineup[i]->skill;
                skillType = skill->type;
                skillTarget = skill->target;
                if (skillType == nothing) {
                    pureMonstersRight++;
                } else if (skillType == protect && (skillTarget == all || skillTarget == rightLineup[rightLost]->element)) {
                    protectionRight += skill->amount;
                } else if (skillType == buff && (skillTarget == all || skillTarget == rightLineup[rightLost]->element)) {
                    damageBuffRight += skill->amount;
                } else if (skillType == heal) {
                    healingRight += skill->amount;
                } else if (skillType == aoe) {
                    aoeDamageRight += skill->amount;
                } else if (skillType == pAoe && i == rightLost) {
                    paoeDamageRight += rightLineup[i]->damage;
                }
            }
        }
        
        // Add last effects of abilities and start resolving the turn
        if (leftLost >= leftArmySize || rightLost >= rightArmySize) {
            break; // At least One army was beaten
        }
        
        // Heal everything that hasnt died
        leftFrontDamageTaken -= leftHealing; // these values are from the last iteration
        leftCumAoeDamageTaken -= leftHealing;
        rightFrontDamageTaken -= rightHealing;
        rightCumAoeDamageTaken -= rightHealing;
        if (leftFrontDamageTaken < 0) {
            leftFrontDamageTaken = 0;
        }
        if (leftCumAoeDamageTaken < 0) {
            leftCumAoeDamageTaken = 0;
        }
        if (rightFrontDamageTaken < 0) {
            rightFrontDamageTaken = 0;
        }
        if (rightCumAoeDamageTaken < 0) {
            rightCumAoeDamageTaken = 0;
        }
        
        currentMonsterLeft = leftLineup[leftLost];
        currentMonsterRight = rightLineup[rightLost];
        
        // Handle Monsters with skills berserk or friends
        damageLeft = currentMonsterLeft->damage;
        if (currentMonsterLeft->skill.type == berserk) {
            damageLeft *= leftBerserkMult;
            leftBerserkMult *= currentMonsterLeft->skill.amount;
        } else {
            leftBerserkMult = 1;
        }
        if (currentMonsterLeft->skill.type == friends) {
            damageLeft *= pow(currentMonsterLeft->skill.amount, pureMonstersLeft);
        }
        
        damageRight = currentMonsterRight->damage;
        if (currentMonsterRight->skill.type == berserk) {
            damageRight *= rightBerserkMult;
            rightBerserkMult *= currentMonsterRight->skill.amount;
        } else {
            rightBerserkMult = 1;
        }
        if (currentMonsterRight->skill.type == friends) {
            damageRight *= pow(currentMonsterRight->skill.amount, pureMonstersRight);
        }
        
        // Add Buff Damage
        damageLeft += damageBuffLeft;
        damageRight += damageBuffRight;
        
        // Handle Elemental advantage
        elementalDifference = (currentMonsterLeft->element - currentMonsterRight->element);
        if (elementalDifference == -1 || elementalDifference == 3) {
            damageLeft *= elementalBoost;
        } else if (elementalDifference == 1 || elementalDifference == -3) {
            damageRight *= elementalBoost;
        }
        
        // Handle Protection on the enemy Side
        if (damageLeft > protectionRight) {
            damageLeft -= protectionRight;
        } else {
            damageLeft = 0;
        }
        
        if (damageRight > protectionLeft) {
            damageRight -= protectionLeft;
        } else {
            damageRight = 0; 
        }
        
        rightFrontDamageTaken += damageLeft + aoeDamageLeft;
        rightCumAoeDamageTaken += aoeDamageLeft + paoeDamageLeft;
        rightHealing = healingRight;
        leftFrontDamageTaken += damageRight + aoeDamageRight;
        leftCumAoeDamageTaken += aoeDamageRight + paoeDamageRight;
        leftHealing = healingLeft;
        
        if (currentMonsterLeft->hp <= leftFrontDamageTaken) {
            leftLost++;
            leftBerserkMult = 1;
            leftFrontDamageTaken = leftCumAoeDamageTaken;
        }
        if (currentMonsterRight->hp <= rightFrontDamageTaken) {
            rightLost++;
            rightBerserkMult = 1;
            rightFrontDamageTaken = rightCumAoeDamageTaken;
        }
        
        // Output detailed fight Data for debugging
        if (verbose) {
            cout << setw(3) << leftLost << " " << setw(3) << leftFrontDamageTaken << " " << setw(3) << rightLost << " " << setw(3) << rightFrontDamageTaken << endl;
        }
    }
    
    result.dominated = false;
    result.leftAoeDamage = leftCumAoeDamageTaken;
    result.rightAoeDamage = rightCumAoeDamageTaken;
    
    if (leftLost >= leftArmySize) { //draws count as right wins. 
        result.rightWon = true;
        result.monstersLost = rightLost; 
        result.damage = rightFrontDamageTaken;
        result.berserk = rightBerserkMult;
    } else {
        result.rightWon = false;
        result.monstersLost = leftLost; 
        result.damage = leftFrontDamageTaken;
        result.berserk = leftBerserkMult;
    }
}

void simulateMultipleFights(vector<FightResult> & results, vector<Army> & armies, Army & target) {
    //simulates all of the fights from armies and puts it into a vector of fightResults.
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
    if (!askYesNoQuestion("Continue calculation?","y")) {return;}
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
    totalFightsSimulated = 0;
    followerUpperBound = numeric_limits<int>::max();
    best = Army();
    
    // Initialize Monster Data
    for (size_t i = 0; i < monsterBaseList.size(); i++) {
        monsterList.push_back(&monsterBaseList[i]);
        monsterMap.insert(pair<string, Monster *>(monsterBaseList[i].name, &monsterBaseList[i]));
    }
    
    // Sort MonsterList by followers
    sort(monsterList.begin(), monsterList.end(), isCheaper);
    
    // Reserve some space for heroes, otherwise pointers will become invalid (Important!)
    heroReference.reserve(baseHeroes.size()*2);
    
    // Declare Variables
    vector<Monster *> friendLineup {};
    vector<Monster *> hostileLineup {};
    vector<Monster *> availableHeroes {};
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
        manualInput = askYesNoQuestion("Do you want to input everything via command line?","y");
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
        /*cout << "Enter how many monsters are allowed in the solution" << endl;
        getline(cin, inputString);
        limit = stoi(inputString);*/
		limit = stoi(askQuestion("Enter how many monsters are allowed in the solution","6"));
    } else {
        cout << "Taking data from script" << endl;
    }

    // Initialize Hero Data
    for (size_t i = 0; i < baseHeroes.size(); i++) {
        if (yourHeroLevels[i] > 0) {
            addLeveledHero(baseHeroes[i], yourHeroLevels[i]);
            availableHeroes.push_back(monsterMap.at(baseHeroes[i].name + ":" + to_string(yourHeroLevels[i])));
        }
    }
    
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
            
            if (!askYesNoQuestion("Simulate another Fight?","n")) {
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
            /*cout << endl << "The optimal combination is:" << endl << "  ";
            best.print();
            cout << "  (Right-most fights first)" << endl;*/
			cout << endl << "Hostile LineUp was : " ;
			for (size_t i = 0; i < Army(hostileLineup).monsters.size(); i++) {
                cout << Army(hostileLineup).monsters[Army(hostileLineup).monsters.size() - 1 - i]->name << " "; // backwards
            }
			cout << " (left-most fights first)." << endl;
			cout << "The minimum amount of followers is : " << best.followerCost << endl;
            cout << "The optimal combination is: " << endl << "         ";
            for (size_t i = 0; i < best.monsters.size(); i++) {
                cout << best.monsters[best.monsters.size() - 1 - i]->name << "   "; // backwards
            } cout << endl;
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
