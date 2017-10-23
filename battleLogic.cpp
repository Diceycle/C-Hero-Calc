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
    // left[0] and right[0] are the first monsters to fight
    // Damage Application Order: TODO: Find out exactly where wither ability triggers. probably after 6.
    //  1. Base Damage of creature
    //  2. Multiplicators of self       (friends, berserk, etc.)
    //  3. Buffs from heroes            (buff, champion)
    //  4. Elemental Advantage          (f.e. Fire vs. Earth)
    //  5. Protection of enemy Side     (protect, champion)
    //  6. AOE of friendly Side         (aoe, paoe)
    //  7. Healing of enemy Side        (healing)
    
    
    totalFightsSimulated++;
    
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
        if (!leftDied && leftCondition.skillTypes[leftCondition.monstersLost] == wither) {
            leftCondition.frontDamageTaken += (leftCondition.lineup[leftCondition.monstersLost]->hp - leftCondition.frontDamageTaken) * leftCondition.skillAmounts[leftCondition.monstersLost];
        }
        if (!rightDied && rightCondition.skillTypes[rightCondition.monstersLost] == wither) {
            rightCondition.frontDamageTaken += (rightCondition.lineup[rightCondition.monstersLost]->hp - rightCondition.frontDamageTaken) * rightCondition.skillAmounts[rightCondition.monstersLost];
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