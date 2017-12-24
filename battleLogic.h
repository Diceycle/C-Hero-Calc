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
    int valkyrieDamage = 0;
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
        
        int rainbowCondition; // for rainbow ability
        int pureMonsters; // for friends ability
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
    
    this->armySize = army.monsterAmount;
    this->monstersLost = 0;
    this->berserkProcs = 0;
    
    for (i = 0; i < this->armySize; i++) {
        this->lineup[i] = &monsterReference[army.monsters[i]];
        this->rainbowCondition |= 1 << this->lineup[i]->element;
        
        skill = &(this->lineup[i]->skill);
        this->skillTypes[i] = skill->skillType;
        if (skill->skillType == NOTHING) {
            this->pureMonsters++;
        }
        if (skill->skillType == RAINBOW) {
            this->rainbowCondition = 0; // More than 1 Rainbow Hero per lineup will not work properly
        }
        if (skill->skillType == FRIENDS) {
            this->pureMonsters = 0; // More than 1 Friends Hero per lineup will not work properly
        }
        this->skillTargets[i] = skill->target;
        this->skillAmounts[i] = skill->amount;
        this->remainingHealths[i] = this->lineup[i]->hp;
    }
}

// Handle death of the front-most monster
inline void ArmyCondition::afterDeath() {
    this->monstersLost++;
    this->berserkProcs = 0;
}

// Reset turndata and fill it again with the hero abilities' values
inline void ArmyCondition::startNewTurn() {
    int i;
    
    this->turnData.buffDamage = 0;
    this->turnData.protection = 0;
    this->turnData.aoeDamage = 0;
    this->turnData.healing = 0;
    
    // Gather all skills that trigger globally
    for (i = this->monstersLost; i < this->armySize; i++) {
        if (this->skillTypes[i] == NOTHING) { 
            // Careful when doing anything here, as dead heroes get their ability set to NOTHING
        } else if (this->skillTypes[i] == PROTECT && (this->skillTargets[i] == ALL || this->skillTargets[i] == this->lineup[this->monstersLost]->element)) {
            this->turnData.protection += (int) this->skillAmounts[i];
        } else if (this->skillTypes[i] == BUFF && (this->skillTargets[i] == ALL || this->skillTargets[i] == this->lineup[this->monstersLost]->element)) {
            this->turnData.buffDamage += (int) this->skillAmounts[i];
        } else if (this->skillTypes[i] == CHAMPION && (this->skillTargets[i] == ALL || this->skillTargets[i] == this->lineup[this->monstersLost]->element)) {
            this->turnData.buffDamage += (int) this->skillAmounts[i];
            this->turnData.protection += (int) this->skillAmounts[i];
        } else if (this->skillTypes[i] == HEAL) {
            this->turnData.healing += (int) this->skillAmounts[i];
        } else if (this->skillTypes[i] == AOE) {
            this->turnData.aoeDamage += (int) this->skillAmounts[i];
        } else if (this->skillTypes[i] == LIFESTEAL) {
            this->turnData.aoeDamage += (int) this->skillAmounts[i];
            this->turnData.healing += (int) this->skillAmounts[i];
        }
    }
}

// Handle all self-centered abilites and other multipliers on damage
inline void ArmyCondition::getDamage(const int turncounter, const Element opposingElement) {
    this->turnData.baseDamage = this->lineup[this->monstersLost]->damage; // Get Base damage
    
    // Handle Monsters with skills that only activate on attack.
    this->turnData.paoeDamage = 0;
    this->turnData.valkyrieMult = 0;
    this->turnData.witherer = -1;
    this->turnData.multiplier = 1;
    if (this->skillTypes[this->monstersLost] == FRIENDS) {
        this->turnData.multiplier *= (float) pow(this->skillAmounts[this->monstersLost], this->pureMonsters);
    } else if (this->skillTypes[this->monstersLost] == TRAINING) {
        this->turnData.buffDamage += (int) (this->skillAmounts[this->monstersLost] * (float) turncounter);
    } else if (this->skillTypes[this->monstersLost] == RAINBOW && this->rainbowCondition == VALID_RAINBOW_CONDITION) {
        this->turnData.buffDamage += (int) this->skillAmounts[this->monstersLost];
    } else if (this->skillTypes[this->monstersLost] == ADAPT && opposingElement == this->skillTargets[this->monstersLost]) {
        this->turnData.multiplier *= this->skillAmounts[this->monstersLost];
    } else if (this->skillTypes[this->monstersLost] == BERSERK) {
        this->turnData.multiplier *= (float) pow(this->skillAmounts[this->monstersLost], this->berserkProcs);
        this->berserkProcs++;
    } else if (this->skillTypes[this->monstersLost] == PIERCE) {
        this->turnData.paoeDamage = (int) ((float) this->lineup[this->monstersLost]->damage * this->skillAmounts[this->monstersLost]);
    } else if (this->skillTypes[this->monstersLost] == VALKYRIE) {
        this->turnData.valkyrieMult = this->skillAmounts[this->monstersLost]; // save valkyrie mult for later
    }
    
    if (counter[opposingElement] == this->lineup[this->monstersLost]->element) {
        this->turnData.baseDamage = castCeil(((float) this->turnData.baseDamage * this->turnData.multiplier + (float) this->turnData.buffDamage) * elementalBoost);
    } else { 
        this->turnData.baseDamage = castCeil((float) this->turnData.baseDamage * this->turnData.multiplier) + this->turnData.buffDamage;
    }
    this->turnData.valkyrieDamage = this->turnData.baseDamage;
}

// Add damage to the opposing side and check for deaths
inline void ArmyCondition::resolveDamage(TurnData & opposing) {
    int i;
    int frontliner = this->monstersLost; // save original frontliner
    
    // Apply normal attack damage to the frontliner
    if (opposing.baseDamage > this->turnData.protection) {
        this->remainingHealths[this->monstersLost] -= opposing.baseDamage - this->turnData.protection; // Handle Protection
    }
    
    // Handle aoe Damage for all combatants
    for (i = frontliner; i < this->armySize; i++) {
        remainingHealths[i] -= opposing.aoeDamage;
        if (i > frontliner) { // Aoe that doesnt affect the frontliner
            remainingHealths[i] -= opposing.paoeDamage + opposing.valkyrieDamage;
        }
        if (remainingHealths[i] <= 0) {
            if (i == this->monstersLost) {
                afterDeath();
            }
            this->skillTypes[i] = NOTHING; // set dead hero's ability to NOTHING
        } else {
            remainingHealths[i] += this->turnData.healing;
            if (remainingHealths[i] > this->lineup[i]->hp) { // Avoid overhealing
                remainingHealths[i] = this->lineup[i]->hp;
            }
        }
        opposing.valkyrieDamage = castCeil((float) opposing.valkyrieDamage * opposing.valkyrieMult);
    }
    // Handle wither ability
    if (this->monstersLost == frontliner && this->skillTypes[this->monstersLost] == WITHER) {
        this->remainingHealths[this->monstersLost] = castCeil((float) this->remainingHealths[this->monstersLost] * this->skillAmounts[this->monstersLost]);
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