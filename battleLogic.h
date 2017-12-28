#ifndef BATTLE_LOGIC_HEADER
#define BATTLE_LOGIC_HEADER

#include <vector>
#include <cmath>

#include "cosmosData.h"

const float elementalBoost = 1.5; // Damage Boost if element has advantage over another
extern int * totalFightsSimulated;
extern int fightsSimulatedDefault; 

const int VALID_RAINBOW_CONDITION = 15; // Binary 00001111 -> means all elements were added

// Struct keeping track of everything that is only valid for one turn
struct TurnData {
    int baseDamage = 0;
    float multiplier = 1;
    
    int buffDamage = 0;
    int protection = 0;
    int aoeDamage = 0;
    int healing = 0;
    
    float valkyrieMult = 0;
    float valkyrieDamage = 0;
    int paoeDamage = 0;
    int witherer = -1;
};

// Keep track of an army's condition during a fight and save some convinience data
class ArmyCondition {
    public: 
        int armySize;
        Monster * lineup[ARMY_MAX_SIZE];
        int remainingHealths[ARMY_MAX_SIZE];
        SkillType skillTypes[ARMY_MAX_SIZE];
        Element skillTargets[ARMY_MAX_SIZE];
        float skillAmounts[ARMY_MAX_SIZE];
        
        int rainbowConditions[ARMY_MAX_SIZE]; // for rainbow ability
        int pureMonsters[ARMY_MAX_SIZE]; // for friends ability
        
        int berserkProcs; // for berserk ability
        
        int monstersLost;
        
        TurnData turnData;
        
        inline void init(const Army & army);
        inline void afterDeath();
        inline void startNewTurn();
        inline void getDamage(const int turncounter, const Element opposingElement);
        inline void resolveDamage(TurnData & opposing);
    
};

// extract and extrapolate all necessary data from an army
inline void ArmyCondition::init(const Army & army) {
    int i;
    HeroSkill * skill;
    
    int tempRainbowCondition = 0;
    int tempPureMonsters = 0;
    
    armySize = army.monsterAmount;
    monstersLost = 0;
    berserkProcs = 0;
    
    for (i = armySize -1; i >= 0; i--) {
        lineup[i] = &monsterReference[army.monsters[i]];
        
        skill = &(lineup[i]->skill);
        skillTypes[i] = skill->skillType;
        skillTargets[i] = skill->target;
        skillAmounts[i] = skill->amount;
        remainingHealths[i] = lineup[i]->hp;
        
        rainbowConditions[i] = tempRainbowCondition;
        pureMonsters[i] = tempPureMonsters;
        tempRainbowCondition |= 1 << lineup[i]->element;
        if (skill->skillType == NOTHING) {
            tempPureMonsters++;
        }
    }
}

// Reset turndata and fill it again with the hero abilities' values
inline void ArmyCondition::startNewTurn() {
    int i;
    
    turnData.buffDamage = 0;
    turnData.protection = 0;
    turnData.aoeDamage = 0;
    turnData.healing = 0;
    
    // Gather all skills that trigger globally
    for (i = monstersLost; i < armySize; i++) {
        switch (skillTypes[i]) {
            default:        break;
            case PROTECT:   if (skillTargets[i] == ALL || skillTargets[i] == lineup[monstersLost]->element) {
                                turnData.protection += (int) skillAmounts[i];
                            } break;
            case BUFF:      if (skillTargets[i] == ALL || skillTargets[i] == lineup[monstersLost]->element) {
                                turnData.buffDamage += (int) skillAmounts[i];
                            } break;
            case CHAMPION:  if (skillTargets[i] == ALL || skillTargets[i] == lineup[monstersLost]->element) {
                                turnData.buffDamage += (int) skillAmounts[i];
                                turnData.protection += (int) skillAmounts[i];
                            } break;
            case HEAL:      turnData.healing += (int) skillAmounts[i];
                            break;
            case AOE:       turnData.aoeDamage += (int) skillAmounts[i];
                            break;
            case LIFESTEAL: turnData.aoeDamage += (int) skillAmounts[i];
                            turnData.healing += (int) skillAmounts[i];
                            break;
        }
    }
}

// Handle all self-centered abilites and other multipliers on damage
inline void ArmyCondition::getDamage(const int turncounter, const Element opposingElement) {
    turnData.baseDamage = lineup[monstersLost]->damage; // Get Base damage
    
    // Handle Monsters with skills that only activate on attack.
    turnData.paoeDamage = 0;
    turnData.valkyrieMult = 0;
    turnData.witherer = -1;
    turnData.multiplier = 1;
    switch (skillTypes[monstersLost]) {
        default:        break;
        case FRIENDS:   turnData.multiplier *= (float) pow(skillAmounts[monstersLost], pureMonsters[monstersLost]); 
                        break;
        case TRAINING:  turnData.buffDamage += (int) (skillAmounts[monstersLost] * (float) turncounter); 
                        break;
        case RAINBOW:   if (rainbowConditions[monstersLost] == VALID_RAINBOW_CONDITION) {
                            turnData.buffDamage += (int) skillAmounts[monstersLost];
                        } break;
        case ADAPT:     if (opposingElement == skillTargets[monstersLost]) {
                            turnData.multiplier *= skillAmounts[monstersLost];
                        } break;
        case BERSERK:   turnData.multiplier *= (float) pow(skillAmounts[monstersLost], berserkProcs); berserkProcs++; 
                        break;
        case PIERCE:    turnData.paoeDamage = (int) ((float) lineup[monstersLost]->damage * skillAmounts[monstersLost]); 
                        break;
        case VALKYRIE:  turnData.valkyrieMult = skillAmounts[monstersLost]; 
                        break;
    }
    turnData.valkyrieDamage = (float) turnData.baseDamage * turnData.multiplier + (float) turnData.buffDamage;
    
    if (counter[opposingElement] == lineup[monstersLost]->element) {
        turnData.valkyrieDamage *= elementalBoost;
    }
    turnData.baseDamage = castCeil(turnData.valkyrieDamage);
}

// Add damage to the opposing side and check for deaths
inline void ArmyCondition::resolveDamage(TurnData & opposing) {
    int i;
    int frontliner = monstersLost; // save original frontliner
    
    // Apply normal attack damage to the frontliner
    if (opposing.baseDamage > turnData.protection) {
        remainingHealths[monstersLost] -= opposing.baseDamage - turnData.protection; // Handle Protection
    }
    
    // Handle aoe Damage for all combatants
    for (i = frontliner; i < armySize; i++) {
        remainingHealths[i] -= opposing.aoeDamage;
        if (i > frontliner) { // Aoe that doesnt affect the frontliner
            remainingHealths[i] -= opposing.paoeDamage + castCeil(opposing.valkyrieDamage);
        }
        if (remainingHealths[i] <= 0) {
            if (i == monstersLost) {
                monstersLost++;
                berserkProcs = 0;
            }
            skillTypes[i] = NOTHING; // disable dead hero's ability
        } else {
            remainingHealths[i] += turnData.healing;
            if (remainingHealths[i] > lineup[i]->hp) { // Avoid overhealing
                remainingHealths[i] = lineup[i]->hp;
            }
        }
        opposing.valkyrieDamage *= opposing.valkyrieMult;
    }
    // Handle wither ability
    if (monstersLost == frontliner && skillTypes[monstersLost] == WITHER) {
        remainingHealths[monstersLost] = castCeil((float) remainingHealths[monstersLost] * skillAmounts[monstersLost]);
    }
}

extern ArmyCondition leftCondition;
extern ArmyCondition rightCondition;

// Simulates One fight between 2 Armies and writes results into left's LastFightData
inline void simulateFight(Army & left, Army & right, bool verbose = false) {
    // left[0] and right[0] are the first monsters to fight
    (*totalFightsSimulated)++;
    
    int turncounter;
    
    // Load Army data into conditions
    leftCondition.init(left);
    rightCondition.init(right);
    
    // Ignore lastFightData if either army-affecting heroes were added or for debugging
    // Set pre-computed values to pick up where we left off
    if (left.lastFightData.valid && !verbose) { 
        leftCondition.monstersLost         = left.monsterAmount-1; // All monsters of left died last fight only the new one counts
        rightCondition.monstersLost        = left.lastFightData.monstersLost;
        rightCondition.remainingHealths[rightCondition.monstersLost] = left.lastFightData.frontHealth;
        rightCondition.berserkProcs        = left.lastFightData.berserk;
        turncounter                        = left.lastFightData.turncounter;
    } else {
        // Reset Potential values in fightresults
        left.lastFightData.leftAoeDamage = 0;
        left.lastFightData.rightAoeDamage = 0;
        turncounter = 0;
    }
    
    // Battle Loop. Continues until one side is out of monsters
    while (leftCondition.monstersLost < leftCondition.armySize && rightCondition.monstersLost < rightCondition.armySize) {
        leftCondition.startNewTurn();
        rightCondition.startNewTurn();
        
        // Get damage with all relevant multipliers
        leftCondition.getDamage(turncounter, rightCondition.lineup[rightCondition.monstersLost]->element);
        rightCondition.getDamage(turncounter, leftCondition.lineup[leftCondition.monstersLost]->element);
        
        // Handle Revenge Damage before anything else. Revenge Damage caused through aoe is ignored
        if (leftCondition.skillTypes[leftCondition.monstersLost] == REVENGE && 
            leftCondition.remainingHealths[leftCondition.monstersLost] <= rightCondition.turnData.baseDamage - leftCondition.turnData.protection) {
            leftCondition.turnData.aoeDamage += (int) round((float) leftCondition.lineup[leftCondition.monstersLost]->damage * leftCondition.skillAmounts[leftCondition.monstersLost]);
        }
        if (rightCondition.skillTypes[rightCondition.monstersLost] == REVENGE && 
            rightCondition.remainingHealths[rightCondition.monstersLost] <= leftCondition.turnData.baseDamage - rightCondition.turnData.protection) {
            rightCondition.turnData.aoeDamage += (int) round((float) rightCondition.lineup[rightCondition.monstersLost]->damage * rightCondition.skillAmounts[rightCondition.monstersLost]);
        }
        
        left.lastFightData.leftAoeDamage += (int16_t) (leftCondition.turnData.aoeDamage + leftCondition.turnData.paoeDamage);
        left.lastFightData.rightAoeDamage += (int16_t) (rightCondition.turnData.aoeDamage + rightCondition.turnData.paoeDamage);
        
        // Check if anything died as a result
        leftCondition.resolveDamage(rightCondition.turnData);
        rightCondition.resolveDamage(leftCondition.turnData);
        
        turncounter++;
        
        if (verbose) {
            std::cout << "After Turn " << turncounter << ":" << std::endl;
            int i;
            std::cout << "  Left: ";
            for (i = 0; i < leftCondition.armySize; i++) {
                std::cout << leftCondition.remainingHealths[i] << " ";
            } std::cout << std::endl;
            std::cout << "  Right: ";
            for (i = 0; i < rightCondition.armySize; i++) {
                std::cout << rightCondition.remainingHealths[i] << " ";
            } std::cout << std::endl;
        }
    }
    
    // write all the results into a FightResult
    left.lastFightData.dominated = false;
    left.lastFightData.turncounter = (int8_t) turncounter;
    
    if (leftCondition.monstersLost >= leftCondition.armySize) { //draws count as right wins. 
        left.lastFightData.rightWon = true;
        left.lastFightData.monstersLost = (int8_t) rightCondition.monstersLost; 
        left.lastFightData.berserk = (int8_t) rightCondition.berserkProcs;
        if (rightCondition.monstersLost < rightCondition.armySize) {
            left.lastFightData.frontHealth = (int16_t) (rightCondition.remainingHealths[rightCondition.monstersLost]);
        } else {
            left.lastFightData.frontHealth = 0;
        }
    } else {
        left.lastFightData.rightWon = false;
        left.lastFightData.monstersLost = (int8_t) leftCondition.monstersLost; 
        left.lastFightData.frontHealth = (int16_t) (leftCondition.remainingHealths[leftCondition.monstersLost]);
        left.lastFightData.berserk = (int8_t) leftCondition.berserkProcs;
    }
}

// Function determining if a monster is strictly better than another
bool isBetter(Monster * a, Monster * b, bool considerAbilities = false);

#endif