#include "battleLogic.h"

int totalFightsSimulated = 0;

// Function determining if a monster is strictly better than another
bool isBetter(Monster * a, Monster * b, bool considerAbilities) {
    if (a->element == b->element) {
        return (a->damage >= b->damage) && (a->hp >= b->hp);
    } else { // a needs to be better than b even when b has elemental advantage, or a is at disadvantage
        return !considerAbilities && (a->damage >= b->damage * elementalBoost) && (a->hp >= b->hp * elementalBoost);
    }
}

ArmyCondition::ArmyCondition() {}

inline void ArmyCondition::init(Army & army) {
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

int16_t healingTemp;
inline bool ArmyCondition::startNewTurn() {
    size_t i;
    
    this->turnData.buffDamage = 0;
    this->turnData.protection = 0;
    this->turnData.aoeDamage = 0;
    this->turnData.paoeDamage = 0;
    healingTemp = this->turnData.healing;
    this->turnData.healing = 0;
    this->pureMonsters = 0;
    
    for (i = this->monstersLost; i < this->armySize; i++) {
        if (this->aoeDamageTaken >= this->lineup[i]->hp) { // Check for Backline Deaths
            this->monstersLost += (this->monstersLost == i);
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

inline void ArmyCondition::resolveDamage(const TurnData & opposing) {
    if (opposing.baseDamage > this->turnData.protection) {
        this->frontDamageTaken += opposing.baseDamage - this->turnData.protection; // Handle Protection
    }
    this->frontDamageTaken += opposing.aoeDamage; // Also apply aoeDamage
    this->aoeDamageTaken += opposing.aoeDamage + opposing.paoeDamage; // Apply aoe Damage to backline
    
    // Check if the first Monster died (otherwise it will be revived next turn)
    if (this->lineup[this->monstersLost]->hp <= this->frontDamageTaken) {
        this->monstersLost++;
        this->berserkProcs = 0;
        this->frontDamageTaken = this->aoeDamageTaken;
    } else if (this->skillTypes[this->monstersLost] == wither) {
        this->frontDamageTaken += (this->lineup[this->monstersLost]->hp - this->frontDamageTaken) * this->skillAmounts[this->monstersLost];
    }
}

ArmyCondition leftCondition = ArmyCondition();
ArmyCondition rightCondition = ArmyCondition();
int8_t turncounter;

// TODO: Implement MAX AOE Damage to make sure nothing gets revived
// Simulates One fight between 2 Armies and writes results into left's LastFightData
void simulateFight(Army & left, Army & right, bool verbose) {
    // left[0] and right[0] are the first to fight
    // Damage Application Order:
    //  1. Base Damage of creature
    //  2. Multiplicators of self       (friends, berserk)
    //  3. Buffs from heroes            (Hunter, Rei, etc.)
    //  4. Elemental Advantage          (f.e. Fire vs. Earth)
    //  5. Protection of enemy Side     (Nimue, Athos, etc.)
    //  6. AOE of friendly Side         (Tiny, Alpha, etc.)
    //  7. Healing of enemy Side        (Auri, Aeris, etc.)
    
    totalFightsSimulated++;
    
    turncounter = 0;
    
    leftCondition.init(left);
    rightCondition.init(right);
    
    // If no heroes are in the army the result from the smaller army is still valid
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
    
    while (true) {
        if (leftCondition.startNewTurn() | rightCondition.startNewTurn()) {
            break; // startNewTurn returns if an army ran out of monsters
        }
        
        leftCondition.getDamage(turncounter, rightCondition.lineup[rightCondition.monstersLost]->element);
        rightCondition.getDamage(turncounter, leftCondition.lineup[leftCondition.monstersLost]->element);
        
        leftCondition.resolveDamage(rightCondition.turnData);
        rightCondition.resolveDamage(leftCondition.turnData);
        
        // Output detailed fight Data for debugging
        if (verbose) {
            cout << setw(3) << leftCondition.monstersLost << " " << setw(3) << leftCondition.frontDamageTaken<< " " << setw(3) << leftCondition.aoeDamageTaken << " ";
            cout << setw(3) << rightCondition.monstersLost << " " << setw(3) << rightCondition.frontDamageTaken << " " << setw(3) << rightCondition.aoeDamageTaken << endl;
        }
        turncounter++;
    }
    
    // write all the results into a FightResult
    left.lastFightData.dominated = false;
    left.lastFightData.turncounter = turncounter;
    left.lastFightData.leftAoeDamage = leftCondition.aoeDamageTaken;
    left.lastFightData.rightAoeDamage = rightCondition.aoeDamageTaken;
    
    if (leftCondition.monstersLost >= leftCondition.armySize) { //draws count as right wins. 
        left.lastFightData.rightWon = true;
        left.lastFightData.monstersLost = rightCondition.monstersLost; 
        left.lastFightData.damage = rightCondition.frontDamageTaken;
        left.lastFightData.berserk = rightCondition.berserkProcs;
    } else {
        left.lastFightData.rightWon = false;
        left.lastFightData.monstersLost = leftCondition.monstersLost; 
        left.lastFightData.damage = leftCondition.frontDamageTaken;
        left.lastFightData.berserk = leftCondition.berserkProcs;
    }
}