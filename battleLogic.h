#ifndef BATTLE_LOGIC_HEADER
#define BATTLE_LOGIC_HEADER

#include <vector>
#include <cmath>

#include "cosmosData.h"

extern int * totalFightsSimulated;
extern int fightsSimulatedDefault; 

const int VALID_RAINBOW_CONDITION = 15; // Binary 00001111 -> means all elements were added

// Struct keeping track of everything that is only valid for one turn
struct TurnData {
    int64_t baseDamage = 0;
    float multiplier = 1;
    
    int buffDamage = 0;
    int protection = 0;
    int aoeDamage = 0;
    int healing = 0;
    float dampFactor = 1;

	float counter = 0;
    float valkyrieMult = 0;
    float valkyrieDamage = 0;
    bool trampleTriggered = false;
    int paoeDamage = 0;
    int witherer = -1;
};

// Keep track of an army's condition during a fight and save some convinience data
class ArmyCondition {
    public: 
        int armySize;
        Monster * lineup[ARMY_MAX_SIZE];
        int64_t remainingHealths[ARMY_MAX_SIZE];
		int64_t maxHealths[ARMY_MAX_SIZE];
        SkillType skillTypes[ARMY_MAX_SIZE];
        Element skillTargets[ARMY_MAX_SIZE];
        float skillAmounts[ARMY_MAX_SIZE];
        
        bool rainbowConditions[ARMY_MAX_SIZE]; // for rainbow ability
        int pureMonsters[ARMY_MAX_SIZE]; // for friends ability

		bool booze{ false };	// for leprechaun's ability
		int aoeZero;			// for hawking's ability
        
        int berserkProcs; // for berserk ability
        
        int monstersLost;

		bool worldboss;
        
        TurnData turnData;
        
        inline void init(const Army & army, const int oldMonstersLost, const int aoeDamage);
        inline void afterDeath();
        inline void startNewTurn();
        inline void getDamage(const int turncounter, const Element opposingElement, const int opposingProtection, const float opposingDampFactor);
        inline void resolveDamage(TurnData & opposing);
    
};

// extract and extrapolate all necessary data from an army
inline void ArmyCondition::init(const Army & army, const int oldMonstersLost, const int aoeDamage) {
    int i;
    HeroSkill * skill;
    
    int tempRainbowCondition = 0;
    int tempPureMonsters = 0;
    
    armySize = army.monsterAmount;
    monstersLost = oldMonstersLost;
    berserkProcs = 0;
    
	booze = false;
	worldboss = false;
	aoeZero = 0;

    for (i = armySize -1; i >= monstersLost; i--) {
        lineup[i] = &monsterReference[army.monsters[i]];
        
        skill = &(lineup[i]->skill);
        skillTypes[i] = skill->skillType;
        skillTargets[i] = skill->target;
        skillAmounts[i] = skill->amount;
        remainingHealths[i] = lineup[i]->hp - aoeDamage;

		worldboss |= lineup[i]->rarity == WORLDBOSS;

		maxHealths[i] = lineup[i]->hp;
		if (skill->skillType == BEER) booze = true;
		if (skill->skillType == AOEZero_L) aoeZero += skill->amount * lineup[i]->level;

        rainbowConditions[i] = tempRainbowCondition == VALID_RAINBOW_CONDITION;
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
    turnData.dampFactor = 1;
    
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
            case DAMPEN:    turnData.dampFactor *= skillAmounts[i];
                            break;
        }
    }
}

// Handle all self-centered abilites and other multipliers on damage
// Protection needs to be calculated at this point. 
inline void ArmyCondition::getDamage(const int turncounter, const Element opposingElement, const int opposingProtection, const float opposingDampFactor) {
    turnData.baseDamage = lineup[monstersLost]->damage; // Get Base damage
    
    // Handle Monsters with skills that only activate on attack.
    turnData.paoeDamage = 0;
    turnData.trampleTriggered = false;
    turnData.valkyrieMult = 0;
    turnData.witherer = -1;
    turnData.multiplier = 1;
	turnData.counter = 0;
    switch (skillTypes[monstersLost]) {
        case FRIENDS:   turnData.multiplier *= (float) pow(skillAmounts[monstersLost], pureMonsters[monstersLost]); 
                        break;
        case TRAINING:  turnData.buffDamage += (int) (skillAmounts[monstersLost] * (float) turncounter); 
                        break;
        case RAINBOW:   if (rainbowConditions[monstersLost]) {
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
        case TRAMPLE:   turnData.trampleTriggered = true;
                        break;
		case COUNTER:	turnData.counter = skillAmounts[monstersLost]; break;

		default:        break;
				
    }
    turnData.valkyrieDamage = (float) turnData.baseDamage * turnData.multiplier + (float) turnData.buffDamage;
    
    if (counter[opposingElement] == lineup[monstersLost]->element) {
        turnData.valkyrieDamage *= elementalBoost;
    }
    if (turnData.valkyrieDamage > opposingProtection) { // Handle Protection
        turnData.valkyrieDamage -= (float) opposingProtection;
    } else {
        turnData.valkyrieDamage = 0;
    }
	
	// for compiling heavyDamage version
	if (turnData.valkyrieDamage >= std::numeric_limits<int>::max())
		turnData.baseDamage = static_cast<DamageType>(turnData.valkyrieDamage);
	else
		turnData.baseDamage = castCeil(turnData.valkyrieDamage);
    
    // Handle enemy dampen ability and reduce aoe effects
    if (opposingDampFactor < 1) {
        turnData.valkyrieDamage *= opposingDampFactor;
        turnData.aoeDamage = castCeil((float) turnData.aoeDamage * opposingDampFactor);
        turnData.healing = castCeil((float) turnData.healing * opposingDampFactor);
        turnData.paoeDamage = castCeil((float) turnData.paoeDamage * opposingDampFactor);
    }
}

// Add damage to the opposing side and check for deaths
inline void ArmyCondition::resolveDamage(TurnData & opposing) {
    int i;
    int frontliner = monstersLost; // save original frontliner
    
    // Apply normal attack damage to the frontliner
	remainingHealths[frontliner] -= opposing.baseDamage; 
	
	if (opposing.counter && (worldboss || remainingHealths[frontliner] > 0))
		remainingHealths[frontliner] -= static_cast<int64_t>(ceil(turnData.baseDamage * opposing.counter));
	
    if (opposing.trampleTriggered && armySize > frontliner + 1) {
        remainingHealths[frontliner + 1] -= opposing.valkyrieDamage;
    }
    
    // Handle aoe Damage for all combatants
    for (i = frontliner; i < armySize; i++) {
        remainingHealths[i] -= opposing.aoeDamage;
        if (i > frontliner) { // Aoe that doesnt affect the frontliner
            remainingHealths[i] -= opposing.paoeDamage + castCeil(opposing.valkyrieDamage);
        }
        if (remainingHealths[i] <= 0 && !worldboss) {
            if (i == monstersLost) {
                monstersLost++;
                berserkProcs = 0;
            }
            skillTypes[i] = NOTHING; // disable dead hero's ability
        } else {
            remainingHealths[i] += turnData.healing;
            if (remainingHealths[i] > maxHealths[i]) { // Avoid overhealing
                remainingHealths[i] = maxHealths[i];
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
inline bool simulateFight(Army & left, Army & right, bool verbose = false) {
    // left[0] and right[0] are the first monsters to fight
    (*totalFightsSimulated)++;
    
    int turncounter;
    
    // Ignore lastFightData if either army-affecting heroes were added or for debugging
    if (left.lastFightData.valid && !verbose) {
        // Set pre-computed values to pick up where we left off
        leftCondition.init(left, left.monsterAmount-1, left.lastFightData.leftAoeDamage);
        rightCondition.init(right, left.lastFightData.monstersLost, left.lastFightData.rightAoeDamage);
        // Check if the new addition died to Aoe
        if (leftCondition.remainingHealths[leftCondition.monstersLost] <= 0) {
            leftCondition.monstersLost++;
        }
        
        rightCondition.remainingHealths[rightCondition.monstersLost] = left.lastFightData.frontHealth;
        rightCondition.berserkProcs        = left.lastFightData.berserk;
        turncounter                        = left.lastFightData.turncounter;
    } else {
        // Load Army data into conditions
        leftCondition.init(left, 0, 0);
        rightCondition.init(right, 0, 0);

		//----- turn zero -----

		// Apply Leprechaun's skill (Beer)
		if (leftCondition.booze && leftCondition.armySize < rightCondition.armySize)
			for (size_t i = 0; i < ARMY_MAX_SIZE; ++i) {
				rightCondition.maxHealths[i] = int(rightCondition.maxHealths[i] * leftCondition.armySize / rightCondition.armySize);
				rightCondition.remainingHealths[i] = rightCondition.maxHealths[i];
			}

		if (rightCondition.booze && rightCondition.armySize < leftCondition.armySize)
			for (size_t i = 0; i < ARMY_MAX_SIZE; ++i) {
				leftCondition.maxHealths[i] = int(leftCondition.maxHealths[i] * rightCondition.armySize / leftCondition.armySize);
				leftCondition.remainingHealths[i] = leftCondition.maxHealths[i];
			}

        // Reset Potential values in fightresults
        left.lastFightData.leftAoeDamage = 0;
        left.lastFightData.rightAoeDamage = 0;
        turncounter = 0;

		// Apply Hawking's AOE
		if (leftCondition.aoeZero || rightCondition.aoeZero) {
			TurnData turnZero;
			if (leftCondition.aoeZero) {
				left.lastFightData.rightAoeDamage += leftCondition.aoeZero;
				turnZero.aoeDamage = leftCondition.aoeZero;
				rightCondition.resolveDamage(turnZero);
			}
			if (rightCondition.aoeZero) {
				left.lastFightData.leftAoeDamage += rightCondition.aoeZero;
				turnZero.aoeDamage = rightCondition.aoeZero;
				leftCondition.resolveDamage(turnZero);
			}
		}

		//----- turn zero end -----
    }
    
    // Battle Loop. Continues until one side is out of monsters
    while (leftCondition.monstersLost < leftCondition.armySize && rightCondition.monstersLost < rightCondition.armySize) {
        leftCondition.startNewTurn();
        rightCondition.startNewTurn();
        
        // Get damage with all relevant multipliers
        leftCondition.getDamage(turncounter, rightCondition.lineup[rightCondition.monstersLost]->element, rightCondition.turnData.protection, rightCondition.turnData.dampFactor);
        rightCondition.getDamage(turncounter, leftCondition.lineup[leftCondition.monstersLost]->element, leftCondition.turnData.protection, leftCondition.turnData.dampFactor);
        
        // Handle Revenge Damage before anything else. Revenge Damage caused through aoe is ignored
        if (leftCondition.skillTypes[leftCondition.monstersLost] == REVENGE && 
            leftCondition.remainingHealths[leftCondition.monstersLost] <= rightCondition.turnData.baseDamage) {
            leftCondition.turnData.aoeDamage += (int) round((float) leftCondition.lineup[leftCondition.monstersLost]->damage * leftCondition.skillAmounts[leftCondition.monstersLost]);
        }
        if (rightCondition.skillTypes[rightCondition.monstersLost] == REVENGE && 
            rightCondition.remainingHealths[rightCondition.monstersLost] <= leftCondition.turnData.baseDamage) {
            rightCondition.turnData.aoeDamage += (int) round((float) rightCondition.lineup[rightCondition.monstersLost]->damage * rightCondition.skillAmounts[rightCondition.monstersLost]);
        }
        
        left.lastFightData.leftAoeDamage += (int16_t) (rightCondition.turnData.aoeDamage + rightCondition.turnData.paoeDamage);
        left.lastFightData.rightAoeDamage += (int16_t) (leftCondition.turnData.aoeDamage + leftCondition.turnData.paoeDamage);
        
        // Check if anything died as a result
        leftCondition.resolveDamage(rightCondition.turnData);
        rightCondition.resolveDamage(leftCondition.turnData);
        
        turncounter++;
        
        if (verbose) {
            std::cout << "After Turn " << turncounter << ":" << std::endl;
            
            std::cout << "Left:" << std::endl;
            std::cout << "  Damage: " << std::setw(4) << leftCondition.turnData.baseDamage << std::endl;
            std::cout << "  Health: ";
            for (int i = 0; i < leftCondition.armySize; i++) {
                std::cout << std::setw(4) << leftCondition.remainingHealths[i] << " ";
            } std::cout << std::endl;
            
            std::cout << "Right:" << std::endl;
            std::cout << "  Damage: " << std::setw(4) << rightCondition.turnData.baseDamage << std::endl;
            std::cout << "  Health: ";
            for (int i = 0; i < rightCondition.armySize; i++) {
                std::cout << std::setw(4) << rightCondition.remainingHealths[i] << " ";
            } std::cout << std::endl;
        }
    }
    
    // write all the results into a FightResult
    left.lastFightData.dominated = false;
    left.lastFightData.turncounter = (int8_t) turncounter;
    
    if (leftCondition.monstersLost >= leftCondition.armySize) { //draws count as right wins. 
        left.lastFightData.monstersLost = (int8_t) rightCondition.monstersLost; 
        left.lastFightData.berserk = (int8_t) rightCondition.berserkProcs;
        if (rightCondition.monstersLost < rightCondition.armySize) {
            left.lastFightData.frontHealth = (int64_t) (rightCondition.remainingHealths[rightCondition.monstersLost]);
        } else {
            left.lastFightData.frontHealth = 0;
        }
        return false;
    } else {
        left.lastFightData.monstersLost = (int8_t) leftCondition.monstersLost; 
        left.lastFightData.frontHealth = (int64_t) (leftCondition.remainingHealths[leftCondition.monstersLost]);
        left.lastFightData.berserk = (int8_t) leftCondition.berserkProcs;
        return true;
    }
}

// Function determining if a monster is strictly better than another
bool isBetter(Monster * a, Monster * b, bool considerAbilities = false);

#endif