#include "battleLogic.h"

int totalFightsSimulated = 0;

// Prototype function! Currently not used. Function determining if a monster is strictly better than another
bool isBetter(Monster * a, Monster * b, bool considerAbilities) {
    if (a->element == b->element) {
        return (a->damage >= b->damage) && (a->hp >= b->hp);
    } else { // a needs to be better than b even when b has elemental advantage, or a is at disadvantage
        return !considerAbilities && (a->damage >= b->damage * elementalBoost) && (a->hp >= b->hp * elementalBoost);
    }
}

ArmyCondition::ArmyCondition() {}

ArmyCondition leftCondition = ArmyCondition();
ArmyCondition rightCondition = ArmyCondition();
int8_t turncounter;
bool leftDied, rightDied;

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
        
        leftDied = leftCondition.resolveDamage(rightCondition.turnData);
        rightDied = rightCondition.resolveDamage(leftCondition.turnData);
        if (rightCondition.turnData.revengeDamage != 0) { // Means the frontline had revenge
            leftCondition.aoeDamageTaken += rightCondition.turnData.revengeDamage;
            leftCondition.frontDamageTaken += rightCondition.turnData.revengeDamage;
            
            if (leftCondition.lineup[leftCondition.monstersLost]->hp <= leftCondition.frontDamageTaken) {
                leftCondition.afterDeath();
                rightCondition.aoeDamageTaken += leftCondition.turnData.revengeDamage;
                rightCondition.frontDamageTaken += leftCondition.turnData.revengeDamage;
                leftDied = true;
            } 
        }
        if (!leftDied && leftCondition.skillTypes[leftCondition.monstersLost] == wither) {
            leftCondition.frontDamageTaken += (leftCondition.lineup[leftCondition.monstersLost]->hp - leftCondition.frontDamageTaken) * leftCondition.skillAmounts[leftCondition.monstersLost];
        }
        if (!rightDied && rightCondition.skillTypes[rightCondition.monstersLost] == wither) {
            rightCondition.frontDamageTaken += (rightCondition.lineup[rightCondition.monstersLost]->hp - rightCondition.frontDamageTaken) * rightCondition.skillAmounts[rightCondition.monstersLost];
        }
        
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