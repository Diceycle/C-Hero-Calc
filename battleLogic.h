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
                this->turnData.paoeDamage += (int) ((float) this->lineup[i]->damage * this->skillAmounts[i]);
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

extern ArmyCondition leftCondition;
extern ArmyCondition rightCondition;
extern int turncounter;
extern bool leftDied;
extern bool rightDied;

// TODO: Implement MAX AOE Damage to make sure nothing gets revived
// Simulates One fight between 2 Armies and writes results into left's LastFightData
inline void simulateFight(Army & left, Army & right, bool verbose = false) {
    // left[0] and right[0] are the first monsters to fight
    // Damage Application Order: TODO: Find out exactly where wither ability triggers. probably after 6.
    //  1. Base Damage of creature
    //  2. Multiplicators of self       (friends, berserk, etc.)
    //  3. Buffs from heroes            (buff, champion)
    //  4. Elemental Advantage          (f.e. Fire vs. Earth)
    //  5. Protection of enemy Side     (protect, champion)
    //  6. AOE of friendly Side         (aoe, paoe)
    //  7. Healing of enemy Side        (healing)
    (*totalFightsSimulated)++;
    
    turncounter = 0;
    
    // Load Army data into conditions
    leftCondition.init(left);
    rightCondition.init(right);
    
    // Ignore lastFightData if either army-affecting heroes were added or for debugging
    if (left.lastFightData.valid && !verbose) { 
        // Set pre-computed values to pick up where we left off
        leftCondition.monstersLost      = left.monsterAmount-1; // All monsters of left died last fight only the new one counts
        leftCondition.frontDamageTaken  = left.lastFightData.leftAoeDamage;
        leftCondition.aoeDamageTaken    = left.lastFightData.leftAoeDamage;
        rightCondition.monstersLost     = left.lastFightData.monstersLost;
        rightCondition.frontDamageTaken = left.lastFightData.damage;
        rightCondition.aoeDamageTaken   = left.lastFightData.rightAoeDamage;
        rightCondition.berserkProcs     = left.lastFightData.berserk;
        turncounter                     = left.lastFightData.turncounter;
    }
    
    // Battle Loop. Continues until one side is out of monsters
    while (true) {
        if (leftCondition.startNewTurn() | rightCondition.startNewTurn()) {
            break; // startNewTurn returns if an army ran out of monsters
        }
        
        // Get damage with all relevant multipliers
        leftCondition.getDamage(turncounter, rightCondition.lineup[rightCondition.monstersLost]->element);
        rightCondition.getDamage(turncounter, leftCondition.lineup[leftCondition.monstersLost]->element);
        
        // Check if anything died as a result
        leftDied = leftCondition.resolveDamage(rightCondition.turnData);
        rightDied = rightCondition.resolveDamage(leftCondition.turnData); // This already takes potential revenge damage into account
        
        // Handle revenge ability. Easily the messiest thing to do if you dont rely on a function based approach. The things you do for performance
        if (rightCondition.turnData.revengeDamage != 0) { // Means the right frontline had revenge
            leftCondition.aoeDamageTaken += rightCondition.turnData.revengeDamage;
            leftCondition.frontDamageTaken += rightCondition.turnData.revengeDamage;
            
            // Only do this if left died as a result of added revenge damage of right
            if (!leftDied && leftCondition.lineup[leftCondition.monstersLost]->hp <= leftCondition.frontDamageTaken) { 
                // Any additional damage can be handled next turn 
                // TODO: Check if there can really be no faulty interactions if there are revenge monsters in the backline that die as a result
                leftCondition.afterDeath();
                rightCondition.aoeDamageTaken += leftCondition.turnData.revengeDamage;
                rightCondition.frontDamageTaken += leftCondition.turnData.revengeDamage;
                leftDied = true; 
            } 
        }
        
        // Handle wither ability
        if (!leftDied && leftCondition.skillTypes[leftCondition.monstersLost] == WITHER) {
            leftCondition.frontDamageTaken += (int) ((float) (leftCondition.lineup[leftCondition.monstersLost]->hp - leftCondition.frontDamageTaken) * leftCondition.skillAmounts[leftCondition.monstersLost]);
        }
        if (!rightDied && rightCondition.skillTypes[rightCondition.monstersLost] == WITHER) {
            rightCondition.frontDamageTaken += (int) ((float) (rightCondition.lineup[rightCondition.monstersLost]->hp - rightCondition.frontDamageTaken) * rightCondition.skillAmounts[rightCondition.monstersLost]);
        }
        
        // Output detailed fight Data for debugging
        if (verbose) {
            std::cout << std::setw(3) << leftCondition.monstersLost << " " << std::setw(3) << leftCondition.frontDamageTaken<< " " << std::setw(3) << leftCondition.aoeDamageTaken << " ";
            std::cout << std::setw(3) << rightCondition.monstersLost << " " << std::setw(3) << rightCondition.frontDamageTaken << " " << std::setw(3) << rightCondition.aoeDamageTaken << std::endl;
        }
        turncounter++;
    }
    
    // write all the results into a FightResult
    left.lastFightData.dominated = false;
    left.lastFightData.turncounter = (int8_t) turncounter;
    left.lastFightData.leftAoeDamage = (int16_t) leftCondition.aoeDamageTaken;
    left.lastFightData.rightAoeDamage = (int16_t) rightCondition.aoeDamageTaken;
    
    if (leftCondition.monstersLost >= leftCondition.armySize) { //draws count as right wins. 
        left.lastFightData.rightWon = true;
        left.lastFightData.monstersLost = (int8_t) rightCondition.monstersLost; 
        left.lastFightData.damage = (int16_t) rightCondition.frontDamageTaken;
        left.lastFightData.berserk = (int8_t) rightCondition.berserkProcs;
    } else {
        left.lastFightData.rightWon = false;
        left.lastFightData.monstersLost = (int8_t) leftCondition.monstersLost; 
        left.lastFightData.damage = (int16_t) leftCondition.frontDamageTaken;
        left.lastFightData.berserk = (int8_t) leftCondition.berserkProcs;
    }
}

// Function determining if a monster is strictly better than another
bool isBetter(Monster * a, Monster * b, bool considerAbilities = false);

#endif