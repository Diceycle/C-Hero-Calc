#ifndef BATTLE_LOGIC_HEADER
#define BATTLE_LOGIC_HEADER

#include <vector>
#include <cmath>

#include "cosmosData.h"

const float elementalBoost = 1.5; // Damage Boost if element has advantage over another
extern int * totalFightsSimulated;

const int VALID_RAINBOW_CONDITION = 15; // Binary 00001111 -> means all elements were added

// Struct keeping track of everything that is only valid for one turn
struct TurnData {
    float baseDamage = 0;
    int buffDamage = 0;
    int aoeDamage = 0;
    int paoeDamage = 0;
    int protection = 0;
    int healing = 0;
    int revengeDamage = 0;
};

// Keep track of an army's condition during a fight and save some convinience data
class ArmyCondition {
    public: 
        int armySize;
        Monster * lineup[ARMY_MAX_SIZE];
        SkillType skillTypes[ARMY_MAX_SIZE];
        Element skillTargets[ARMY_MAX_SIZE];
        float skillAmounts[ARMY_MAX_SIZE];
        
        int rainbowCondition; // for rainbow ability
        int pureMonsters; // for friends ability
        
        int monstersLost;
        
        int frontDamageTaken;
        int aoeDamageTaken;
        int berserkProcs;
        
        TurnData turnData;
        
        inline void init(const Army & army);
        inline void afterDeath();
        inline bool startNewTurn();
        inline void getDamage(const int turncounter, const Element opposingElement);
        inline bool resolveDamage(TurnData & opposing);
    
};

// extract and extrapolate all necessary data from an army
inline void ArmyCondition::init(const Army & army) {
    int i;
    HeroSkill * skill;
    
    this->armySize = army.monsterAmount;
    this->monstersLost = 0;
    
    this->frontDamageTaken = 0;
    this->aoeDamageTaken = 0;
    this->berserkProcs = 0;
    
    for (i = 0; i < this->armySize; i++) {
        this->lineup[i] = &monsterReference[army.monsters[i]];
        this->rainbowCondition |= 1 << this->lineup[i]->element;
        
        skill = &(this->lineup[i]->skill);
        this->skillTypes[i] = skill->type;
        if (skill->type == RAINBOW) {
            this->rainbowCondition = 0;
        }
        this->skillTargets[i] = skill->target;
        this->skillAmounts[i] = skill->amount;
    }
}

// Handle death of the front-most monster
inline void ArmyCondition::afterDeath() {
    if (this->skillTypes[this->monstersLost] == REVENGE) {
        this->turnData.revengeDamage = (int16_t) round((float) this->lineup[this->monstersLost]->damage * this->skillAmounts[this->monstersLost]);
    }
    this->monstersLost++;
    this->berserkProcs = 0;
    this->frontDamageTaken = this->aoeDamageTaken;
}

// Resert turndata and fill it again with the hero abilities' values
// Also handles healing afterwards to avoid accidental ressurects
inline bool ArmyCondition::startNewTurn() {
    int healingTemp;
    int i;
    
    this->turnData.buffDamage = 0;
    this->turnData.protection = 0;
    this->turnData.aoeDamage = 0;
    this->turnData.paoeDamage = 0;
    this->turnData.revengeDamage = 0;
    healingTemp = this->turnData.healing;
    this->turnData.healing = 0;
    this->pureMonsters = 0;
    
    // Gather hero abilities' effects
    for (i = this->monstersLost; i < this->armySize; i++) {
        if (this->aoeDamageTaken >= this->lineup[i]->hp) { // Check for Backline Deaths
            if (i == this->monstersLost) {
                this->afterDeath();
            }
        } else {
            if (this->skillTypes[i] == NOTHING) {
                pureMonsters++; // count for friends ability
            } else if (this->skillTypes[i] == PROTECT && (this->skillTargets[i] == ALL || this->skillTargets[i] == this->lineup[this->monstersLost]->element)) {
                this->turnData.protection += (int) this->skillAmounts[i];
            } else if (this->skillTypes[i] == BUFF && (this->skillTargets[i] == ALL || this->skillTargets[i] == this->lineup[this->monstersLost]->element)) {
                this->turnData.buffDamage += (int) this->skillAmounts[i];
            } else if (this->skillTypes[i] == CHAMPION && (this->skillTargets[i] == ALL || this->skillTargets[i] == this->lineup[this->monstersLost]->element)) {
                this->turnData.buffDamage += (int) this->skillAmounts[i];
                this->turnData.protection += (int) this->skillAmounts[i];
            } else if (this->skillTypes[i] == PROTECT_L && (this->skillTargets[i] == ALL || this->skillTargets[i] == this->lineup[this->monstersLost]->element)) {
                this->turnData.protection += lineup[i]->level / (int) this->skillAmounts[i];
            } else if (this->skillTypes[i] == BUFF_L && (this->skillTargets[i] == ALL || this->skillTargets[i] == this->lineup[this->monstersLost]->element)) {
                this->turnData.buffDamage += lineup[i]->level / (int) this->skillAmounts[i];
            } else if (this->skillTypes[i] == CHAMPION_L && (this->skillTargets[i] == ALL || this->skillTargets[i] == this->lineup[this->monstersLost]->element)) {
                this->turnData.buffDamage += lineup[i]->level / (int) this->skillAmounts[i];
                this->turnData.protection += lineup[i]->level / (int) this->skillAmounts[i];
            } else if (this->skillTypes[i] == HEAL) {
                this->turnData.healing += (int) this->skillAmounts[i];
            } else if (this->skillTypes[i] == AOE) {
                this->turnData.aoeDamage += (int) this->skillAmounts[i];
            } else if (this->skillTypes[i] == P_AOE && i == this->monstersLost) {
                this->turnData.paoeDamage += (int) this->lineup[i]->damage * this->skillAmounts[i];
            }
        }
    }
    
    // heal monsters
    this->frontDamageTaken -= healingTemp;
    this->aoeDamageTaken -= healingTemp;
    if (this->frontDamageTaken < 0) {
        this->frontDamageTaken = 0;
    } 
    if (this->aoeDamageTaken < 0) {
        this->aoeDamageTaken = 0;
    }
    
    // Return if loss condition is fulfilled
    return (this->monstersLost >= this->armySize);
}

// Handle all self-centered abilites and other multipliers on damage
inline void ArmyCondition::getDamage(const int turncounter, const Element opposingElement) {
    this->turnData.baseDamage = (float) this->lineup[this->monstersLost]->damage; // Get Base damage
    
    // Handle Monsters with skills berserk or friends or training etc.
    if (this->skillTypes[this->monstersLost] == FRIENDS) {
        this->turnData.baseDamage *= (float) pow(this->skillAmounts[this->monstersLost], this->pureMonsters);
    } else if (this->skillTypes[this->monstersLost] == TRAINING) {
        this->turnData.baseDamage += this->skillAmounts[this->monstersLost] * (float) turncounter;
    } else if (this->skillTypes[this->monstersLost] == RAINBOW && this->rainbowCondition == VALID_RAINBOW_CONDITION) {
        this->turnData.baseDamage += this->skillAmounts[this->monstersLost];
    } else if (this->skillTypes[this->monstersLost] == ADAPT && opposingElement == this->skillTargets[this->monstersLost]) {
        this->turnData.baseDamage *= this->skillAmounts[this->monstersLost];
    } else if (this->skillTypes[this->monstersLost] == BERSERK) {
        this->turnData.baseDamage *= (float) pow(this->skillAmounts[this->monstersLost], this->berserkProcs);
        this->berserkProcs++;
    }
    
    this->turnData.baseDamage += (float) this->turnData.buffDamage; // Add Buff Damage
    
    if (counter[opposingElement] == this->lineup[this->monstersLost]->element) {
        this->turnData.baseDamage *= elementalBoost;
    }
}

// Add damage to the opposing side and check for deaths
inline bool ArmyCondition::resolveDamage(TurnData & opposing) {
    if (opposing.baseDamage > this->turnData.protection) {
        this->frontDamageTaken += (int) ceil(opposing.baseDamage) - this->turnData.protection; // Handle Protection
    }
    this->frontDamageTaken += opposing.aoeDamage + opposing.revengeDamage; // Also apply aoeDamage
    this->aoeDamageTaken += opposing.aoeDamage + opposing.paoeDamage + opposing.revengeDamage; // Apply aoe Damage to backline
    opposing.revengeDamage = 0; // Vital to check for additional revenge damage later
    
    // Check if the first Monster died (otherwise it will be revived next turn)
    if (this->lineup[this->monstersLost]->hp <= this->frontDamageTaken) {
        this->afterDeath();
        return true;
    } else {
        return false;
    }
}

// Function determining if a monster is strictly better than another
bool isBetter(Monster * a, Monster * b, bool considerAbilities = false);

// Simulates One fight between 2 Armies
void simulateFight(Army & left, Army & right, bool verbose = false);

#endif