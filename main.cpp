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

// Returns damage done depending on the elements of the fighting monsters
int getElementalAdvantage(int damage, Element sourceElement, Element targetElement) {
    if ((sourceElement == earth && targetElement == air) || 
        (sourceElement == air && targetElement == water) ||
        (sourceElement == water && targetElement == fire) ||
        (sourceElement == fire && targetElement == earth)) {
        return damage * 1.5;
    }
    return damage;
}

void one_turn(vector<int> & result, const vector<Monster *> & left, size_t leftIndex, const vector<Monster *> & right, size_t rightIndex) {
    // simulates one turn (left attacks right and right attacks left)
    // returns all the damage values
    // may seem unelegant to do both side in one function but it probably saves about 10 million function calls even on small problems
    // expects a vector of size 8 to write the results of the simulation
    
    Monster * currentLeft = left[leftIndex];
    int damageLeft = 0;
    int protectionLeft = 0;
    int aoeDamageLeft = 0;
    int paoeDamageLeft = 0;
    int healingLeft = 0;
    
    Monster * currentRight = right[rightIndex];
    int damageRight = 0;
    int protectionRight = 0;
    int aoeDamageRight = 0;
    int paoeDamageRight = 0;
    int healingRight = 0;
    
    //hero stuff
    HeroSkill * skill;
    SkillType skillType;
    Element skillTarget;
    
    for (size_t i = rightIndex; i < right.size(); i++) {
        skill = &right[i]->skill;
        skillType = skill->type;
        skillTarget = skill->target;
        if (skillType == nothing) {
        } else if (skillType == protect && (skillTarget == all || skillTarget == currentRight->element)) {
            protectionRight += skill->amount;
        } else if (skillType == heal && (skillTarget == all || skillTarget == currentRight->element)) {
            healingRight += skill->amount;
        } else if (skillType == aoe && (skillTarget == all || skillTarget == currentLeft->element)) {
            aoeDamageRight += skill->amount;
        } else if (skillType == pAoe && i == rightIndex) {
            paoeDamageRight += right[i]->damage;
        } else if (skillType== buff && (skillTarget == all || skillTarget == currentRight->element)) {
            damageRight += skill->amount;
        }
    }
    for (size_t i = leftIndex; i < left.size(); i++) {
        skill = &left[i]->skill;
        skillType = skill->type;
        skillTarget = skill->target;
        if (skillType == nothing) {
        } else if (skillType == protect && (skillTarget == all || skillTarget == currentLeft->element)) {
            protectionLeft += skill->amount;
        } else if (skillType == heal && (skillTarget == all || skillTarget == currentLeft->element)) {
            healingLeft += skill->amount;
        } else if (skillType == aoe && (skillTarget == all || skillTarget == currentRight->element)) {
            aoeDamageLeft += skill->amount;
        } else if (skillType == pAoe && i == leftIndex) {
            paoeDamageLeft += left[i]->damage;
        } else if (skillType== buff && (skillTarget == all || skillTarget == currentLeft->element)) {
            damageLeft += skill->amount;
        }
    }
    
    damageLeft = getElementalAdvantage(currentLeft->damage + damageLeft, currentLeft->element, currentRight->element);
    if (damageLeft > protectionRight) {
        damageLeft -= protectionRight;
    } else {
        damageLeft = 0;
    }
    damageRight = getElementalAdvantage(currentRight->damage + damageRight, currentRight->element, currentLeft->element);
    if (damageRight > protectionLeft) {
        damageRight -= protectionLeft;
    } else {
        damageRight = 0; 
    }

    result[0] = damageLeft; 
    result[1] = healingLeft;
    result[2] = aoeDamageLeft;
    result[3] = paoeDamageLeft;
    result[4] = damageRight;
    result[5] = healingRight;
    result[6] = aoeDamageRight;
    result[7] = paoeDamageRight;
}

// TODO: Check and potentially implement hero death - skill interaction
void simulate_fight(FightResult & result, const Army & left, const Army & right, bool verbose = false) {
    //fights left to right, so left[0] and right[0] are the first to fight
    totalFightsSimulated++;
    int leftLost = 0;
    int leftArmySize = left.monsters.size();
    int leftDamageTaken = 0;
    int leftCumAoeDamage = 0;
    int leftBerserk = 1;
    bool leftBerserkActive = false;
    
    int rightLost = 0;
    int rightArmySize = right.monsters.size();
    int rightDamageTaken = 0;
    int rightCumAoeDamage = 0;
    int rightBerserk = 1;
    bool rightBerserkActive = false;
    
    // If no heroes are in the army the result from the smaller army is still valid
    // TODO: Fix bug: if aoe kills the 'new arrival' but it was a draw before then it is not checked cause of tehe while condition
    if (left.precomputedFight.valid && !verbose) { 
        // Set pre-computed values to pick up where we left off
        leftLost = leftArmySize-1;
        leftDamageTaken = left.precomputedFight.leftAoeDamage;
        rightLost = left.precomputedFight.monstersLost;
        rightDamageTaken = left.precomputedFight.damage;
        rightCumAoeDamage = left.precomputedFight.rightAoeDamage;
        rightBerserk = left.precomputedFight.berserk;
    }
      
    vector<int> turnSimulation; turnSimulation.reserve(8);
    while (leftLost < leftArmySize && rightLost < rightArmySize) {
        //attack once
        leftBerserkActive = left.monsters[leftLost]->skill.type == berserk;
        rightBerserkActive = right.monsters[rightLost]->skill.type == berserk;
        
        one_turn(turnSimulation, left.monsters, leftLost, right.monsters, rightLost);
        rightDamageTaken += turnSimulation[0] * leftBerserk + turnSimulation[2]; // first mob takes normal damage and aoe
        rightCumAoeDamage += turnSimulation[2] + turnSimulation[3]; // normal aoe + piercing aoe
        leftDamageTaken += turnSimulation[4] * rightBerserk + turnSimulation[6]; // same values for the other side
        leftCumAoeDamage += turnSimulation[6] + turnSimulation[7];
        //check for deaths
        while (leftLost < leftArmySize && leftDamageTaken >= left.monsters[leftLost]->hp) {
            leftDamageTaken = leftCumAoeDamage;
            leftBerserkActive = false;
            leftBerserk = 1;
            leftLost++;
        }
        while (rightLost < rightArmySize && rightDamageTaken >= right.monsters[rightLost]->hp) {
            rightDamageTaken = rightCumAoeDamage;
            rightBerserkActive = false;
            rightBerserk = 1;
            rightLost++;
        }
        // healing - does not take effect on death
        leftDamageTaken = max(0, leftDamageTaken - turnSimulation[1]);
        rightDamageTaken = max(0, rightDamageTaken - turnSimulation[5]);
        
        // deal with berserkers
        if (leftBerserkActive) {
            leftBerserk *= left.monsters[leftLost]->skill.amount;
        }
        if (rightBerserkActive) {
            rightBerserk *= right.monsters[rightLost]->skill.amount;
        }
        
        // Output detailed fight Data for debugging
        if (verbose) {
            cout << leftLost << " " << leftDamageTaken << " " << rightLost << " " << rightDamageTaken << endl;
        }
    }
    
    result.dominated = false;
    result.leftAoeDamage = leftCumAoeDamage;
    result.rightAoeDamage = rightCumAoeDamage;
    
    if (leftLost == leftArmySize) { //draws count as right wins. 
        result.rightWon = true;
        result.monstersLost = rightLost; 
        result.damage = rightDamageTaken;
        result.berserk = rightBerserk;
    } else {
        result.rightWon = false;
        result.monstersLost = leftLost; 
        result.damage = leftDamageTaken;
        result.berserk = leftBerserk;
    }
}

void simulate_multiple_fights(vector<FightResult> & results, vector<Army> & armies, const Army & target) {
    //simulates all of the fights from armies and puts it into a vector of fight_results.
    results.reserve(armies.size());
    FightResult currentResult;
    
    for (size_t i = 0; i < armies.size(); i++) {
        simulate_fight(currentResult, armies[i], target);
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

void expand(vector<Army> & source, vector<FightResult> & current_armies, const vector<Monster *> & to_expand_with, bool check_heroes_repeat) {
    // expand stuff directly onto source, does not expand dominated or winning or too costly armies
    // if the bool is true, we check if we repeat any heroes
    FightResult * this_result;
    bool heroUseable;
    size_t i, j, k;
    
    for (i = 0; i < current_armies.size(); i++) {
        this_result = &current_armies[i]; 
        if (!this_result->dominated) { // if this setup isn't worse than another and hasn't won yet
            for (j = 0; j < to_expand_with.size(); j++) { // expand this setup with all available mobs
                if (this_result->source->followerCost + to_expand_with[j]->cost < followerUpperBound) {
                    
                    heroUseable = true;
                    if (check_heroes_repeat) { // check if a hero has been used before
                        for (k = 0; k < this_result->source->monsters.size(); k++) {
                            if (this_result->source->monsters[k] == to_expand_with[j]) { // since were only expanding with heroes in this case we can compare normal mobs too
                                heroUseable = false;
                                break;
                            }
                        }
                    }
                    if (heroUseable) {
                        source.push_back(*(this_result->source));
                        source[source.size() - 1].add(to_expand_with[j]);
                        if (!to_expand_with[j]->isHero) {
                            source[source.size()-1].precomputedFight = KnownFight({this_result->monstersLost, this_result->damage, this_result->berserk, this_result->leftAoeDamage, this_result->rightAoeDamage, true}); // pre-computed fight
                        } else {
                            source[source.size()-1].precomputedFight.valid = false;
                        }
                    }
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
                simulate_fight(tempResult, tempArmy, target);
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
                    simulate_fight(tempResult, tempArmy, target);
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
    }
}

void solve_instance(const vector<Monster *> & heroes_available, Army target, size_t limit, bool time_it) {
    Army tempArmy = Army();
    FightResult tempResult;
    int startTime = time(NULL);
    int tempTime = startTime;
    
    size_t i, j, sj, si, c;

    getQuickSolutions(heroes_available, target, limit);
    if (!askYesNoQuestion("Continue calculation?")) {return;}
    
    vector<Army> pureMonsterArmies {}; // initialize with all monsters
    vector<Army> heroMonsterArmies {}; // initialize with all heroes
    for (i = 0; i < monsterList.size(); i++) {
        if (monsterList[i]->cost <= followerUpperBound) {
            pureMonsterArmies.push_back(Army(vector<Monster *>{monsterList[i]}));
        }
    }
    for (i = 0; i < heroes_available.size(); i++) { // Ignore chacking for Hero Cost
        heroMonsterArmies.push_back(Army(vector<Monster *>{heroes_available[i]}));// emplace back!
    }
    
    // Check if a single monster can beat the last two monsters of the target. If not, solutions that can only beat n-2 monsters need not be expanded later
    bool optimizable = (target.monsters.size() >= 3);
    if (optimizable) {
        tempArmy = Army({target.monsters[target.monsters.size() - 2], target.monsters[target.monsters.size() - 1]}); // Make an army from the last two monsters
    }
    
    if (optimizable) { // Check with normal Mobs
        for (i = 0; i < pureMonsterArmies.size(); i++) {
            simulate_fight(tempResult, pureMonsterArmies[i], tempArmy);
            if (!tempResult.rightWon) { // Monster won the fight
                optimizable = false;
                break;
            }
        }
    }

    if (optimizable) { // Check with Heroes
        for (i = 0; i < heroMonsterArmies.size(); i++) {
            simulate_fight(tempResult, heroMonsterArmies[i], tempArmy);
            if (!tempResult.rightWon) { // Hero won the fight
                optimizable = false;
                break;
            }
        }
    }
    
    cout << optimizable << (followerUpperBound != numeric_limits<int>::max()) << endl;

    // Run the Bruteforce Loop
    size_t armySize = 0;
    size_t targetSize = target.monsters.size();
    size_t fightResultAmount, herofightResultAmount;
    for (c = 1; c <= limit; c++) { // c is the length of the list of monsters
        armySize = c;
    
        // Output Debug Information
        debugOutput(tempTime, "Starting loop for armies of size " + to_string(c), true, false, true);
        
        // Run Fights for non-Hero setups
        debugOutput(tempTime, "  Simulating " + to_string(pureMonsterArmies.size()) + " non-hero Fights... ", time_it, false, false);
        tempTime = time(NULL);
        vector<FightResult> result;
        simulate_multiple_fights(result, pureMonsterArmies, target);
        
        // Run fights for setups with heroes
        debugOutput(tempTime, "  Simulating " + to_string(heroMonsterArmies.size()) + " hero Fights... ", time_it, true, false);
        tempTime = time(NULL);
        vector<FightResult> result_heroes;
        simulate_multiple_fights(result_heroes, heroMonsterArmies, target);
        
        fightResultAmount = result.size();
        herofightResultAmount = result_heroes.size();
        if (c < limit) { 
            // Sort the results by follower cost for some optimization
            debugOutput(tempTime, "  Sorting List... ", time_it, true, false);
            tempTime = time(NULL);
            sort(result.begin(), result.end(), hasFewerFollowers);
            sort(result_heroes.begin(), result_heroes.end(), hasFewerFollowers);
            
            // Calculate which results are strictly better than others (dominance)
            debugOutput(tempTime, "  Calculating Dominance for non-heroes... ", time_it, true, false);
            tempTime = time(NULL);
            
            int leftFollowerCost;
            vector<Monster *> leftHeroList {};
            size_t leftHeroListSize;
            Monster * rightMonster;
            Monster * leftMonster;
            // First Check dominance for non-Hero setups
            for (i = 0; i < fightResultAmount; i++) {
                leftFollowerCost = result[i].source->followerCost;
                // A result is obsolete if only one expansion is left but no single mob can beat the last two enemy mobs alone (optimizable)
                if (c == (limit - 1) && optimizable) {
                    // TODO: Investigate whether this is truly correct: What if the second-to-last mob is already damaged (not from aoe) i.e. it defeated the last mob of left?
                    if (result[i].rightWon && result[i].monstersLost < targetSize - 2 && result[i].rightAoeDamage == 0) {
                        result[i].dominated = true;
                    }
                }
                // A result is dominated If:
                if (!result[i].dominated) { 
                    // Another result got farther with a less costly lineup
                    for (j = i+1; j < fightResultAmount; j++) {
                        if (leftFollowerCost < result[j].source->followerCost) {
                            break; 
                        } else if (result[i] <= result[j]) { // result[i] has more followers implicitly 
                            result[i].dominated = true;
                            break;
                        }
                    }
                    // A lineup without heroes is better than a setup with heroes even if it got just as far
                    for (j = 0; j < herofightResultAmount; j++) {
                        if (leftFollowerCost > result_heroes[j].source->followerCost) {
                            break; 
                        } else if (result[i] >= result_heroes[j] ) { // result[i] has less followers implicitly
                            result_heroes[j].dominated = true;
                        }                       
                    }
                }
            }
            
            debugOutput(tempTime, "  Calculating Dominance for heroes... ", time_it, true, false);
            tempTime = time(NULL);
            // Domination for setups with heroes
            bool usedHeroSubset, i_used_hero;
            for (i = 0; i < herofightResultAmount; i++) {
                leftFollowerCost = result_heroes[i].source->followerCost;
                leftHeroList.clear();
                for (si = 0; si < armySize; si++) {
                    leftMonster = result_heroes[i].source->monsters[si];
                    if (leftMonster->isHero) {
                        leftHeroList.push_back(leftMonster);
                    }
                }
                leftHeroListSize = leftHeroList.size();
                
                // A result is obsolete if only one expansion is left but no single mob can beat the last two enemy mobs alone (optimizable)
                if (c == (limit-1) && optimizable && result_heroes[i].rightAoeDamage == 0) {
                    // TODO: Investigate whether this is truly correct: What if the second-to-last mob is already damaged (not from aoe) i.e. it defeated the last mob of left?
                    if (result_heroes[i].rightWon && result_heroes[i].monstersLost < targetSize - 2){
                        result_heroes[i].dominated = true;
                    }
                }
                
                // A result is dominated If:
                if (!result_heroes[i].dominated) {
                    // if i costs more followers and got less far than j, then i is dominated
                    for (j = i+1; j < herofightResultAmount; j++) {
                        if (leftFollowerCost < result_heroes[j].source->followerCost) {
                            break;
                        } else if (result_heroes[i] <= result_heroes[j]) { // i has more followers implicitly
                            usedHeroSubset = true; // If j doesn't use a strict subset of the heroes i used, it cannot dominate i
                            for (sj = 0; sj < armySize; sj++) { // for every hero in j there must be the same hero in i
                                i_used_hero = false; 
                                rightMonster = result_heroes[j].source->monsters[sj];
                                if (rightMonster->isHero) { // mob is a hero
                                    for (si = 0; si < leftHeroListSize; si++) {
                                        if (leftHeroList[si] == rightMonster) {
                                            i_used_hero = true;
                                            break;
                                        }
                                    }
                                    if (!i_used_hero) {
                                        usedHeroSubset = false;
                                        break;
                                    }
                                }
                            }
                            if (usedHeroSubset) {
                                result_heroes[i].dominated = true;
                                break;
                            }                           
                        }
                    }
                }
            }
            
            // now we expand to add the next monster to all non-dominated armies
            debugOutput(tempTime, "  Expanding Lineups by one... ", time_it, true, false);
            tempTime = time(NULL);
            
            vector<Army> next_optimal;
            expand(next_optimal, result, monsterList, false);
            vector<Army> next_optimal_heroes;
            expand(next_optimal_heroes, result_heroes, monsterList, false);
            expand(next_optimal_heroes, result, heroes_available, false);
            expand(next_optimal_heroes, result_heroes, heroes_available, true);
            debugOutput(tempTime, "  Moving Data... ", time_it, true, false);
            tempTime = time(NULL);
            
            pureMonsterArmies = move(next_optimal);
            heroMonsterArmies = move(next_optimal_heroes);
        }
        debugOutput(tempTime, "", true, true, true);
    }
}
/*
void test_cases(){
    
    // test cases for dominance of fight_results 
    add_hero_string("rei:3");
    add_hero_string("hunter:2");
    add_hero_string("rei:1");
    add_hero_string("tiny:1");
    solution solv = solve_instance({all_monster_map.at("tiny:1"),all_monster_map.at("rei:1"),},
    army(vector<monster*>{
        &all_monster_map.at("a4"),&all_monster_map.at("f5"),&all_monster_map.at("e4"),&all_monster_map.at("w5"),&all_monster_map.at("a5")
    }), 6, true);
    // don't know the true value, so I can't tell. only doing this to see if there are exceptions.
    add_hero_string("lady of twilight:1");
    add_hero_string("jet:1");
    for(int i=0; i<solv.optimal.monsters.size();i++){
        cout << (solv.optimal.monsters[i] -> name) << " ";
    }
    cout << "\n";
     
//    0 16 0 8
//    0 32 0 16
//    0 48 1 0
//    1 0 1 8
//    1 12 2 0
//    1 24 2 6
//    2 0 2 12
//    2 12 2 18
//    2 24 3 0
//    3 0 3 9
//    3 27 3 13
//    4 0 3 17
//    4 16 4 0
//    4 22 4 12
//    5 0 4 24
     
    simulate_fight(army(vector<monster*>{
            &all_monster_map.at("f3"), 
            &all_monster_map.at("w1"), 
            &all_monster_map.at("w1"),
            &all_monster_map.at("e1"), 
            &all_monster_map.at("w2")   
    }), 
            
            army(vector<monster*>{
            &all_monster_map.at("hunter:2"), 
            &all_monster_map.at("f1"), 
            &all_monster_map.at("a1"),
            &all_monster_map.at("jet:1"),
            &all_monster_map.at("w1"),
            &all_monster_map.at("e2")
    
    }), true);
    //
    cout << "YAY?\n";

//    0 42 0 6
//    1 0 0 12
//    2 0 0 24
//    3 0 0 36
//    4 0 0 44
//    5 0 1 0
//    5 9 1 16
//    6 0 2 0

    simulate_fight(army(vector<monster*>{
            &all_monster_map.at("e1"), 
            &all_monster_map.at("e2"), 
            &all_monster_map.at("w2"),
            &all_monster_map.at("a1"), 
            &all_monster_map.at("w2"), 
            &all_monster_map.at("f2")    
    }), 
            
            army(vector<monster*>{
            &all_monster_map.at("rei:1"), 
            &all_monster_map.at("w1"), 
            &all_monster_map.at("e1"),
            &all_monster_map.at("f1"),
            &all_monster_map.at("a1"),
            &all_monster_map.at("e1")
    
    }), true);
    cout << "YAY!";

//    0 66 0 26
//    1 0 0 52
//    1 44 1 0
//    1 98 1 28
//    2 0 1 56
//    2 36 2 0
//    2 87 2 34
//    3 0 3 0
//    4 0 4 0
//    4 66 4 26
//    5 0 4 52
//    5 44 5 0
//    6 0 6 0
//    Draw

    simulate_fight(army(vector<monster*>{
            &all_monster_map.at("e6"), 
            &all_monster_map.at("a7"), 
            &all_monster_map.at("w7"),
            &all_monster_map.at("f7"), 
            &all_monster_map.at("e6"), 
            &all_monster_map.at("rei:1")    
    }), 
            
            army(vector<monster*>{
            &all_monster_map.at("f7"), 
            &all_monster_map.at("e7"), 
            &all_monster_map.at("a6"),
            &all_monster_map.at("w6"),
            &all_monster_map.at("f7"),
            &all_monster_map.at("w6")
    
    }), true);
    cout << "YAY!\n";
    
    // 63 damage, 9 damage
    vector<int> turnSimulation;
    one_turn(turnSimulation, {all_monster_map.at("rei:1")}, {&all_monster_map.at("rei:1").skill}, 0, {all_monster_map.at("w6")}, {&all_monster_map.at("w6").skill}, 0);
    int damage_dealt = turnSimulation[0];
    one_turn(turnSimulation, {all_monster_map.at("a2")}, {&all_monster_map.at("a2").skill}, 0, {all_monster_map.at("w2")}, {&all_monster_map.at("w2").skill}, 0);
    int damage_dealt_2 = turnSimulation[0];
    
    cout << damage_dealt << " "<< damage_dealt_2<<"\n";
    add_hero_string("faefyr:3");
    
}
*/

int main(int argc, char** argv) {
    //test_cases();
    
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
    vector<string> daily {"e10", "e10", "f10", "a10", "nebra:22"};
    vector<string> test {"a10", "tiny:92"};
    vector<string> test2 {"f10","a10","f10","a10","t4urus:99"};
    vector<string> test3 {"a9", "f8", "a8"}; // soultion should be (e7, w7, ailen:53, rei:1)(274k) or better but gets dominated {0,0,0,0,1,1,0,3,0,0,2,1,1,53,2,0,0,0,0,0,0,0,1,1,0,0,0};
    
    // Declare Hero Levels
    // INPUT YOUR HERO LEVELS HERE (For manual editing: Names tell you which number is the level of which hero)
//    yourHeroLevels = { 1, 1, 0, 0,      // "lady of twilight","tiny","nebra","james"
//                      99, 2, 1,         // "hunter","shaman","alpha"
//                       5, 2, 1,         // "carl","nimue","athos"
//                       5, 8, 0,         // "jet","geron","rei"
//                      99, 4, 0,         // "ailen","faefyr","auri"
//                       1, 0, 0,         // "k41ry", "t4urus", "tr0n1x"
//                       1, 0, 0,         // "aquortis", "aeris", "geum"
//                       1, 1, 1, 1, 1,   // "valor","rokka","pyromancer","bewat","nicte"
//    }; 
    yourHeroLevels = { 1, 1, 0, 0,      // "lady of twilight","tiny","nebra","james"
                      99, 0, 1,         // "hunter","shaman","alpha"
                       0, 0, 1,         // "carl","nimue","athos"
                       0, 0, 0,         // "jet","geron","rei"
                      99, 0, 0,         // "ailen","faefyr","auri"
                       1, 0, 0,         // "k41ry", "t4urus", "tr0n1x"
                       1, 0, 0,         // "aquortis", "aeris", "geum"
                       0, 0, 0, 0, 1,   // "valor","rokka","pyromancer","bewat","nicte"
    }; 
    
    // Use these variables to specify the fight
    bool ignoreConsole = false;                         // Disables the console question whether you want to read from file or command line
    int limit = 6;                                      // Set this to how many Monsters should be in the solution (f.e 4 for X-3 Quests) 
    hostileLineup = makeMonstersFromStrings(test2);     // Choose against which lineup you want to fight use one from above or make your own and then change the name accordingly
    bool individual = false;                            // Set this to true if you want to simulate individual fights (lineups will be promted when you run the program)
    bool time_it = true;                                // Set this to true if you want to see how far the execution is and how lone the execution took altogether
    bool manualInput = false;                           // Set this to true if you want nothing to do with this file and just want to input stuff over the command line like you're used to
    
    if (!ignoreConsole) {
        manualInput = askYesNoQuestion("Do you want to input everything via command line?");
    }
    // Collect the Data via Command Line if the user wants
    if (manualInput) {
        yourHeroLevels = takeHerolevelInput();
        hostileLineup = takeLineupInput();
        cout << "Enter how many monsters are allowed in the solution" << endl;
        cin >> inputString;
        limit = stoi(inputString);
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
            
            FightResult custom_result;
            simulate_fight(custom_result, Army(friendLineup), Army(hostileLineup), true);
            cout << custom_result.rightWon << " " << Army(friendLineup).followerCost << " " << Army(hostileLineup).followerCost << endl;
            
            if (!askYesNoQuestion("Simulate another Fight?")) {
                break;
            }
        }
        return 0;
    }
    
    int startTime = time(NULL);
    Army hostileArmy = Army(hostileLineup);
    solve_instance(availableHeroes, hostileArmy, limit, time_it);
    // Last check to see if winning combination wins:
    if (followerUpperBound < numeric_limits<int>::max()) {
        best.precomputedFight.valid = false;
        FightResult final_result;
        simulate_fight(final_result, best, hostileArmy);
        if (final_result.rightWon) {
            for (int i = 1; i <= 10; i++) {
                cout << "ERROR";
            }
            system("pause");
            return -1;
        } else {
            // Print the winning combination!
            cout << endl << "The optimal combination is:" << endl;
            best.print();
            cout << "(Right-most fights first)" << endl;
        }
    } else {
        cout << endl << "Could not find a solution that beats this lineup." << endl;
    }
    
    cout << endl;
    cout << totalFightsSimulated << " Fights simulated." << endl;
    cout << "Total Calculation Time: " << time(NULL) - startTime << endl;
    system("pause");
    return 0;
}