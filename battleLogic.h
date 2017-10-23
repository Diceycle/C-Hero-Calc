#ifndef BATTLE_LOGIC_HEADER
#define BATTLE_LOGIC_HEADER

#include <vector>
#include <cmath>

#include "cosmosClasses.h"

const float elementalBoost = 1.5;
extern int totalFightsSimulated;

struct TurnData {
    float baseDamage = 0;
    int16_t buffDamage = 0;
    int16_t aoeDamage = 0;
    int16_t paoeDamage = 0;
    int16_t protection = 0;
    int16_t healing = 0;
    int16_t revengeDamage = 0;
};

class ArmyCondition {
    public: 
        size_t armySize;
        Monster * lineup[6];
        SkillType skillTypes[6];
        Element skillTargets[6];
        float skillAmounts[6];
        
        int8_t rainbowCondition; // for rainbow ability
        int8_t pureMonsters; // for friends ability
        
        size_t monstersLost;
        
        int16_t frontDamageTaken;
        int16_t aoeDamageTaken;
        int8_t berserkProcs;
        
        TurnData turnData;
        
        ArmyCondition();
        
        inline void init(const Army & army);
        inline void afterDeath();
        inline bool startNewTurn();
        inline void getDamage(const int8_t turncounter, const Element opposingElement);
        inline bool resolveDamage(TurnData & opposing);
    
};

inline void ArmyCondition::init(const Army & army) {
    size_t i;
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
        if (skill->type == rainbow) {
            this->rainbowCondition = 0;
        }
        this->skillTargets[i] = skill->target;
        this->skillAmounts[i] = skill->amount;
    }
}

inline void ArmyCondition::afterDeath() {
    if (this->skillTypes[this->monstersLost] == revenge) {
        this->turnData.revengeDamage = (int) ceil(this->lineup[this->monstersLost]->damage * this->skillAmounts[this->monstersLost]);
    }
    this->monstersLost++;
    this->berserkProcs = 0;
    this->frontDamageTaken = this->aoeDamageTaken;
}

inline bool ArmyCondition::startNewTurn() {
    int16_t healingTemp;
    size_t i;
    
    this->turnData.buffDamage = 0;
    this->turnData.protection = 0;
    this->turnData.aoeDamage = 0;
    this->turnData.paoeDamage = 0;
    this->turnData.revengeDamage = 0;
    healingTemp = this->turnData.healing;
    this->turnData.healing = 0;
    this->pureMonsters = 0;
    
    for (i = this->monstersLost; i < this->armySize; i++) {
        if (this->aoeDamageTaken >= this->lineup[i]->hp) { // Check for Backline Deaths
            if (i == this->monstersLost) {
                this->afterDeath();
            }
        } else {
            if (this->skillTypes[i] == nothing) {
                pureMonsters++; // count for friends ability
            } else if (this->skillTypes[i] == protect && (this->skillTargets[i] == all || this->skillTargets[i] == this->lineup[this->monstersLost]->element)) {
                this->turnData.protection += this->skillAmounts[i];
            } else if (this->skillTypes[i] == buff && (this->skillTargets[i] == all || this->skillTargets[i] == this->lineup[this->monstersLost]->element)) {
                this->turnData.buffDamage += this->skillAmounts[i];
            } else if (this->skillTypes[i] == champion && (this->skillTargets[i] == all || this->skillTargets[i] == this->lineup[this->monstersLost]->element)) {
                this->turnData.buffDamage += this->skillAmounts[i];
                this->turnData.protection += this->skillAmounts[i];
            } else if (this->skillTypes[i] == heal) {
                this->turnData.healing += this->skillAmounts[i];
            } else if (this->skillTypes[i] == aoe) {
                this->turnData.aoeDamage += this->skillAmounts[i];
            } else if (this->skillTypes[i] == pAoe && i == this->monstersLost) {
                this->turnData.paoeDamage += this->lineup[i]->damage;
            }
        }
    }
    
    this->frontDamageTaken -= healingTemp;
    this->aoeDamageTaken -= healingTemp;
    if (this->frontDamageTaken < 0) {
        this->frontDamageTaken = 0;
    } 
    if (this->aoeDamageTaken < 0) {
        this->aoeDamageTaken = 0;
    }
    
    return (this->monstersLost >= this->armySize);
}

inline void ArmyCondition::getDamage(const int8_t turncounter, const Element opposingElement) {
    this->turnData.baseDamage = this->lineup[this->monstersLost]->damage; // Get Base damage
    
    // Handle Monsters with skills berserk or friends or training etc.
    if (this->skillTypes[this->monstersLost] == friends) {
        this->turnData.baseDamage *= pow(this->skillAmounts[this->monstersLost], this->pureMonsters);
    } else if (this->skillTypes[this->monstersLost] == training) {
        this->turnData.baseDamage += this->skillAmounts[this->monstersLost] * turncounter;
    } else if (this->skillTypes[this->monstersLost] == rainbow && this->rainbowCondition == 15) {
        this->turnData.baseDamage += this->skillAmounts[this->monstersLost];
    } else if (this->skillTypes[this->monstersLost] == adapt && opposingElement == this->skillTargets[this->monstersLost]) {
        this->turnData.baseDamage *= this->skillAmounts[this->monstersLost];
    } else if (this->skillTypes[this->monstersLost] == berserk) {
        this->turnData.baseDamage *= pow(this->skillAmounts[this->monstersLost], this->berserkProcs);
        this->berserkProcs++;
    }
    
    this->turnData.baseDamage += this->turnData.buffDamage; // Add Buff Damage
    
    if (counter[opposingElement] == this->lineup[this->monstersLost]->element) {
        this->turnData.baseDamage *= elementalBoost;
    }
}

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