#ifndef BATTLE_LOGIC_HEADER
#define BATTLE_LOGIC_HEADER

#include <vector>
#include <cmath>

#include "cosmosData.h"

const int VALID_RAINBOW_CONDITION = 15; // Binary 00001111 -> means all elements were added

// Struct keeping track of everything that is only valid for one turn
struct TurnData {
    int64_t baseDamage = 0;
    double multiplier = 1;

    int buffDamage = 0;
    int protection = 0;
    int aoeDamage = 0;
    int healing = 0;
    double dampFactor = 1;

    double counter = 0;
    double valkyrieMult = 0;
    double valkyrieDamage = 0;
    double absorbMult = 0;
    double absorbDamage = 0;
    int explodeDamage = 0;
    bool trampleTriggered = false;
    bool guyActive = false;
    int direct_target = 0;
    int counter_target = 0;
    double critMult = 1;
    double hate = 0;
    double leech = 0;
    bool immunity5K = false ;
};

// Keep track of an army's condition during a fight and save some convenience data
class ArmyCondition {
    public:
        int armySize;
        Monster * lineup[ARMY_MAX_SIZE];
        int64_t remainingHealths[ARMY_MAX_SIZE];
        int64_t maxHealths[ARMY_MAX_SIZE];
        SkillType skillTypes[ARMY_MAX_SIZE];
        Element skillTargets[ARMY_MAX_SIZE];
        double skillAmounts[ARMY_MAX_SIZE];

        bool rainbowConditions[ARMY_MAX_SIZE]; // for rainbow ability
        //int pureMonsters[ARMY_MAX_SIZE]; // for friends ability

        int dice;
        bool booze; // for leprechaun's ability
        int aoeZero; // for hawking's ability

        int64_t seed;

        int berserkProcs; // for berserk ability
        double evolveTotal; //for evolve ability

        int monstersLost;

        bool worldboss;

        TurnData turnData;

        inline void init(const Army & army, const int oldMonstersLost, const int aoeDamage);
        inline void afterDeath();
        inline void startNewTurn();
        inline void getDamage(const int turncounter, const ArmyCondition & opposingCondition);
        inline void resolveDamage(TurnData & opposing);
        inline int64_t getTurnSeed(int64_t seed, int turncounter) {
            // From Alya
            for (int i = 0; i < turncounter; ++i) {
              seed = (16807 * seed) % 2147483647;
            }
            return seed;
            //return (seed + (101 - turncounter)*(101 - turncounter)*(101 - turncounter)) % (int64_t)round((double)seed / (101 - turncounter) + (101 - turncounter)*(101 - turncounter));
        }
        inline int findMaxHP();
        inline int64_t getNewTurnSeed(const ArmyCondition & opposingCondition, int turncounter);
        inline int getLuxTarget(const ArmyCondition & opposingCondition, int64_t seed);
};

// extract and extrapolate all necessary data from an army
inline void ArmyCondition::init(const Army & army, const int oldMonstersLost, const int aoeDamage) {
    HeroSkill * skill;

    int tempRainbowCondition = 0;
    int tempPureMonsters = 0;

    seed = army.seed;
    armySize = army.monsterAmount;
    monstersLost = oldMonstersLost;
    berserkProcs = 0;
    evolveTotal = 0;

    dice = -1;
    booze = false;
    worldboss = false;
    aoeZero = 0;

    for (int i = armySize -1; i >= monstersLost; i--) {
        lineup[i] = &monsterReference[army.monsters[i]];

        skill = &(lineup[i]->skill);
        skillTypes[i] = skill->skillType;
        skillTargets[i] = skill->target;
        skillAmounts[i] = skill->amount;
        remainingHealths[i] = lineup[i]->hp - aoeDamage;

        worldboss |= lineup[i]->rarity == WORLDBOSS;

        maxHealths[i] = lineup[i]->hp;
        if (skill->skillType == DICE) dice = i; //assumes only 1 unit per side can have dice ability, have to change to bool and loop at turn zero if this changes
        if (skill->skillType == BEER) booze = true;
        if (skill->skillType == AOEZERO) aoeZero += skill->amount;

        rainbowConditions[i] = tempRainbowCondition == VALID_RAINBOW_CONDITION;
        //pureMonsters[i] = tempPureMonsters;

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
    turnData.absorbMult = 0;
    turnData.absorbDamage = 0;

    if( skillTypes[monstersLost] == DODGE )
    {
        turnData.immunity5K = true ;
    }
    else
    {
        turnData.immunity5K = false ;
    }

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
            case ABSORB:    if (i != monstersLost) turnData.absorbMult += skillAmounts[i];
                            break;
        }
    }
}

// Handle all self-centered abilities and other multipliers on damage
// Protection needs to be calculated at this point.
inline void ArmyCondition::getDamage(const int turncounter, const ArmyCondition & opposingCondition) {

    turnData.baseDamage = lineup[monstersLost]->damage; // Get Base damage

    const Element opposingElement = opposingCondition.lineup[opposingCondition.monstersLost]->element;
    const int opposingProtection = opposingCondition.turnData.protection;
    const double opposingDampFactor = opposingCondition.turnData.dampFactor;
    const double opposingAbsorbMult = opposingCondition.turnData.absorbMult;
    const bool opposingImmunityDamage = opposingCondition.turnData.immunity5K;
    const double opposingDamage = opposingCondition.lineup[opposingCondition.monstersLost]->damage;
    ArmyCondition tempArmy;

    // Handle Monsters with skills that only activate on attack.
    turnData.trampleTriggered = false;
    turnData.explodeDamage = 0;
    turnData.valkyrieMult = 0;
    turnData.multiplier = 1; // Not used outside this function, does it need to be stored in turnData?
    turnData.critMult = 1; // same as above
    turnData.hate = 0; // same as above
    turnData.counter = 0;
    turnData.direct_target = 0;
    turnData.counter_target = 0;
    turnData.leech = 0;

    double friendsDamage = 0;

    switch (skillTypes[monstersLost]) {
        case FRIENDS:   friendsDamage = turnData.baseDamage;
                        for (int i = monstersLost + 1; i < armySize; i++) {
                            if (skillTypes[i] == NOTHING && remainingHealths[i] > 0)
                                friendsDamage *= skillAmounts[monstersLost];
                            else if ((skillTypes[i] == BUFF || skillTypes[i] == CHAMPION) && (skillTargets[i] == ALL || skillTargets[i] == lineup[monstersLost]->element))
                                friendsDamage += skillAmounts[i];
                        }
                        break;
        case TRAINING:  turnData.buffDamage += (int) (skillAmounts[monstersLost] * (double) turncounter);
                        break;
        case RAINBOW:   if (rainbowConditions[monstersLost]) {
                            turnData.buffDamage += (int) skillAmounts[monstersLost];
                        } break;
        case ADAPT:     if (opposingElement == skillTargets[monstersLost]) {
                            turnData.multiplier *= skillAmounts[monstersLost];
                        } break;
        case BERSERK:   turnData.multiplier *= (double) pow(skillAmounts[monstersLost], berserkProcs); berserkProcs++;
                        break;
        case VALKYRIE:  turnData.valkyrieMult = skillAmounts[monstersLost];
                        break;
        case TRAMPLE:   turnData.trampleTriggered = true;
                        break;
        case COUNTER:   turnData.counter = skillAmounts[monstersLost];
                        break;
        case EXPLODE:   turnData.explodeDamage = skillAmounts[monstersLost]; // Explode damage gets added here, but still won't apply unless enemy frontliner dies
                        break;
        case DICE:      turnData.baseDamage += opposingCondition.seed % (int)(skillAmounts[monstersLost] + 1); // Only adds dice attack effect if dice is in front, max health is done before battle
                        break;
        // Pick a target, Bubbles currently dampens lux damage if not targeting first according to game code, interaction should be added if this doesn't change
        case LUX:       // turnData.direct_target = getLuxTarget(opposingCondition, getNewTurnSeed(opposingCondition, turncounter + 1));
                        // Lux targetting still busted if it gets multiple hits, deducts from dead unit
                        turnData.direct_target = getLuxTarget(opposingCondition, getTurnSeed(opposingCondition.seed, 99 -turncounter));
                        // turncounter is number of turns completed, game calculates on current turn
                        // turnData.direct_target = getTurnSeed(opposingCondition.seed, turncounter) % (opposingCondition.armySize - opposingCondition.monstersLost);
                        break;
        case CRIT:      // turnData.critMult *= getTurnSeed(opposingCondition.seed, turncounter) % 2 == 1 ? skillAmounts[monstersLost] : 1;
                        turnData.critMult *= getTurnSeed(opposingCondition.seed, 99 - turncounter) % 2 == 0 ? skillAmounts[monstersLost] : 1;
                        break;
        case HATE:      turnData.hate = skillAmounts[monstersLost];
                        break;
        case LEECH:     turnData.leech = skillAmounts[monstersLost];
                        break;
        case EVOLVE:    turnData.buffDamage += evolveTotal;
                        evolveTotal += opposingDamage * skillAmounts[monstersLost];
                        break;
        case COUNTER_MAX_HP: turnData.counter = skillAmounts[monstersLost];
                        tempArmy = opposingCondition;
                        turnData.counter_target = tempArmy.findMaxHP();
                        turnData.guyActive = true;
                        break;
        default:        break;

    }
    turnData.valkyrieDamage = turnData.baseDamage;
    if (friendsDamage == 0) {
        if (turnData.multiplier > 1) {
            turnData.valkyrieDamage *= turnData.multiplier;
        }
        if (turnData.buffDamage != 0) {
            turnData.valkyrieDamage += turnData.buffDamage;
        }
    }
    else {
        turnData.valkyrieDamage = friendsDamage;
    }

    if (counter[opposingElement] == lineup[monstersLost]->element) {
        turnData.valkyrieDamage *= elementalBoost + turnData.hate;
    }
    if (turnData.valkyrieDamage > opposingProtection) { // Handle Protection, when this takes place currently varies based on the side the army is on according to game code
        turnData.valkyrieDamage -= (double) opposingProtection;
    } else {
        turnData.valkyrieDamage = 0;
    }

    if (turnData.critMult > 1) {
        turnData.valkyrieDamage *= turnData.critMult;
    }

    //absorb damage, damage rounded up later
    if (opposingAbsorbMult != 0) {
        turnData.absorbDamage = turnData.valkyrieDamage * opposingAbsorbMult;
        turnData.valkyrieDamage = turnData.valkyrieDamage - turnData.absorbDamage;
    }
    //leech healing based on damage dealt
    if (turnData.leech != 0)
        turnData.leech *= turnData.valkyrieDamage;

    // for compiling heavyDamage version
    if (turnData.valkyrieDamage >= std::numeric_limits<int>::max())
        turnData.baseDamage = static_cast<DamageType>(ceil(turnData.valkyrieDamage));
    else
        // turnData.baseDamage = castCeil(turnData.valkyrieDamage);
        turnData.baseDamage = round(turnData.valkyrieDamage);

    // Handle enemy dampen ability and reduce aoe effects
    if (opposingDampFactor < 1) {
        turnData.valkyrieDamage *= opposingDampFactor;
        // turnData.explodeDamage = castCeil((double) turnData.explodeDamage * opposingDampFactor);
        // turnData.aoeDamage = castCeil((double) turnData.aoeDamage * opposingDampFactor);
        // turnData.healing = castCeil((double) turnData.healing * opposingDampFactor);
        turnData.explodeDamage = round((double) turnData.explodeDamage * opposingDampFactor);
        turnData.aoeDamage = round((double) turnData.aoeDamage * opposingDampFactor);
        turnData.healing = round((double) turnData.healing * opposingDampFactor);
    }

    if( opposingImmunityDamage && (turnData.valkyrieDamage >= 5000 || turnData.baseDamage >= 5000 ) ) {
        turnData.valkyrieDamage = 0 ;
        turnData.baseDamage = 0 ;
    }
}

// Add damage to the opposing side and check for deaths
inline void ArmyCondition::resolveDamage(TurnData & opposing) {
    int frontliner = monstersLost; // save original frontliner

    // Apply normal attack damage to the frontliner
    // If direct_target is non-zero that means Lux is hitting something not the front liner
    // Also, needed to handle here is the case where there are dead units behind the frontliner
    if(opposing.direct_target > 0) {
      int actual_target = frontliner;
      int alive = 0;
      for(int i = frontliner + 1; i < ARMY_MAX_SIZE; i++) {
        // std::cout << "I is " << i << std::endl;
        if(remainingHealths[i] > 0) {
          alive++;
          // std::cout << "Alive incremented to " << alive << std::endl;
        }
        if(alive >= opposing.direct_target) {
          actual_target = i;
          // std::cout << "Setting actual target to " << i << std::endl;
          break;
        }
      }
      // std::cout << " BASE " << opposing.baseDamage << " to " << actual_target << std::endl;
      remainingHealths[actual_target] -= opposing.baseDamage;
    } else {
      // std::cout << " BASE " << opposing.baseDamage << " to " << frontliner + opposing.direct_target << std::endl;
      remainingHealths[frontliner + opposing.direct_target] -= opposing.baseDamage;
    }

    // Lee and Fawkes can only counter if they are hit directly, so if they are opposing Lux and Lux
    // hits another units, they do not counter
    int counter_eligible = 1;
    if(skillTypes[monstersLost] == LUX && turnData.direct_target > 0) {
      counter_eligible = 0;
      // std::cout << "LUX DID NOT HIT FRONTLINER" << std::endl;
    }

    // Add opposing.counter_target to handle fawkes not targetting the frontliner
    if (opposing.counter && counter_eligible && (worldboss || remainingHealths[frontliner] > 0 || opposing.guyActive)) {
      // std::cout << "COUNTER " << static_cast<int64_t>(ceil(turnData.baseDamage * opposing.counter)) << " damage " << " to " << frontliner + opposing.counter_target << std::endl;
      // Game has been updated to remove units once dead but remainingHealths still has all units
      // So, if the counter target is not the frontliner, find the real one
      // So, counter_target has to skip over dead units
      if(opposing.counter_target <= 0){
        // std::cout << "COUNTER " << static_cast<int64_t>(ceil(turnData.baseDamage * opposing.counter)) << " damage " << " to " << frontliner << std::endl;
        remainingHealths[frontliner] -= static_cast<int64_t>(ceil(turnData.baseDamage * opposing.counter));
      } else {
        // Find the (counter_target)th alive monster in remainingHealths
        int actual_target = opposing.counter_target;
        // std::cout << "Counter target is " << opposing.counter_target << std::endl;
        int alive = 0;
        for(int i = frontliner + 1; i < ARMY_MAX_SIZE; i++) {
          // std::cout << "I is " << i << std::endl;
          if(remainingHealths[i] > 0) {
            alive++;
            // std::cout << "Alive incremented to " << alive << std::endl;
          }
          if(alive >= opposing.counter_target) {
            actual_target = i;
            // std::cout << "Setting actual target to " << i << std::endl;
            break;
          }
        }
        // std::cout << "COUNTER " << static_cast<int64_t>(ceil(turnData.baseDamage * opposing.counter)) << " damage " << " to " << actual_target << std::endl;
        remainingHealths[actual_target] -= static_cast<int64_t>(ceil(turnData.baseDamage * opposing.counter));
      }

    }

    if (opposing.trampleTriggered && armySize > frontliner + 1) {
        // std::cout << "TRAMPLE" << std::endl;
        remainingHealths[frontliner + 1] -= opposing.valkyrieDamage;
    }

    if (opposing.explodeDamage != 0 && remainingHealths[frontliner] <= 0 && !worldboss) {
        // std::cout << "EXPLODE" << std::endl;
        opposing.aoeDamage += opposing.explodeDamage;
    }

    // Handle aoe Damage for all combatants
    for (int i = frontliner; i < armySize; i++) {
      int aliveAtBeginning = 0;
      if(remainingHealths[i] > 0 || i == frontliner) {
        aliveAtBeginning = 1;
      }
      // handle absorbed damage
      if (skillTypes[i] == ABSORB && i > frontliner) {
        // remainingHealths[i] -= castCeil(opposing.absorbDamage);
        remainingHealths[i] -= round(opposing.absorbDamage);
      }

      remainingHealths[i] -= opposing.aoeDamage;

      if (i > frontliner) { // Aoe that doesnt affect the frontliner
        // remainingHealths[i] -= castCeil(opposing.valkyrieDamage);
        remainingHealths[i] -= round(opposing.valkyrieDamage);
      }
      if (remainingHealths[i] <= 0 && !worldboss) {
        if (i == monstersLost) {
          monstersLost++;
          berserkProcs = 0;
          evolveTotal = 0;
        }
        skillTypes[i] = NOTHING; // disable dead hero's ability
      } else {
          remainingHealths[i] += turnData.healing;
        if (i == frontliner)
          remainingHealths[i] += turnData.leech;
        if (remainingHealths[i] > maxHealths[i]) { // Avoid overhealing
          remainingHealths[i] = maxHealths[i];
        }
      }

      // Always apply the valkyrieMult if it is zero. Otherwise, given the way
      // that riochet is implemented it will cause melee attacks to turn into
      // ricochet
      if(opposing.valkyrieMult > 0) {
        // Only reduce the damage if it hit an alive unit
        if(aliveAtBeginning) {
          opposing.valkyrieDamage *= opposing.valkyrieMult;
        }
      }  else {
        opposing.valkyrieDamage *= opposing.valkyrieMult;
      }
    }
    // Handle wither ability
    if (skillTypes[monstersLost] == WITHER && monstersLost == frontliner) {
        // remainingHealths[monstersLost] = castCeil((double) remainingHealths[monstersLost] * skillAmounts[monstersLost]);
        remainingHealths[monstersLost] = round((double) remainingHealths[monstersLost] * skillAmounts[monstersLost]);
    }
}

inline int ArmyCondition::findMaxHP() {
    // go through alive monsters to determine most hp
    // If there is a tie...
    // Use the same loop as in ResolveDamage
    int max_hp = 0;
    int index_max_hp = 0;
    for (int i = monstersLost; i < armySize; i++) {
        if(remainingHealths[i] > max_hp) {
            max_hp = remainingHealths[i];
            index_max_hp = i;
        }
    }
    // This is an absolute index, the rest of the code expects a relative index
    // so return it relative to the starting point
    // std::cout << std::endl << "Choosing " << index_max_hp << " with " << remainingHealths[index_max_hp] << std::endl;
    return index_max_hp - monstersLost;
}

inline int64_t ArmyCondition::getNewTurnSeed(const ArmyCondition & opposingCondition, int turncounter) {
  /* Javascript code from version 3.2.0.4
  function calcSeed(A, times) {
    for (var seed = 1, i = 0; i < A.length; ++i) seed = (seed * Math.abs(A[i]) + 1) % 2147483647;
    for (i = 0; i < times; ++i) seed = 16807 * seed % 2147483647;
    return seed
  }
  */
  //std::cout << "\nEntering getNewTurnSeed with turn " << turncounter << std::endl;
  int64_t seed = 1;
  for(int i = 0; i < opposingCondition.armySize; i++) {
    // std::cout << "Index " << i << std::endl;
    // New CQ code removes dead units, so simulate that here by checking for health
    if(opposingCondition.remainingHealths[i] > 0) {
      // std::cout << "\tIn if\n";
      //std::cout << "Lineup is " << opposingCondition.lineup[i]->index << std::endl;
      seed = (seed * abs(opposingCondition.lineup[i]->index) + 1) % 2147483647;
      // std::cout << "\tSeed is " << seed << std::endl;
    }
  }

  // std::cout << "Out of loop with seed value " << seed << std::endl;

  for (int i = 0; i < 100 - turncounter; i++) {
    seed = 16807 * seed % 2147483647;
  }
  std::cout << "Returning from getNewTurnSeed " << seed << std::endl << std::endl;
  return seed;
}

inline int ArmyCondition::getLuxTarget(const ArmyCondition & opposingCondition, int64_t seed) {
  /* Javascript code from 3.2.0.4
  function shuffleBySeed(arr, seed) {
    for (var size = arr.length, mapa = new Array(size), x = 0; x < size; x++) mapa[x] = (seed = (9301 * seed + 49297) % 233280) / 233280 * size | 0;
    for (var i = size - 1; i > 0; i--) arr[i] = arr.splice(mapa[size - 1 - i], 1, arr[i])[0]
}
  called like `else if ("rtrg" == skill.type) shuffleBySeed(turn.atk.damageFactor, seed);`
  damageFactor appears to be initialized to an array of one element consisting of [1]
  function getTurnData(AL, BL) {
    var df = [1];
    ...
    damageFactor: df,
    ...
  So looking at things that probably means that we need an array of size the of number alive monsters
  with the first element initialized to 1
  815500433
  */

  // Count the number of alive monsters;
  int alive_count = 0;
  for(int i = 0; i < opposingCondition.armySize; i++) {
    // New CQ code removes dead units, so simulate that here by checking for health
    // std::cout << i << ". Remaining health is " << opposingCondition.remainingHealths[i] << std::endl;
    if(opposingCondition.remainingHealths[i] > 0) {
      alive_count++;
    }
  }
  // std::cout << "Alive count is " << alive_count << std::endl;

  // Initialize the arr equivalent
  int arr[alive_count];
  arr[0] = 1;
  for(int i = 1; i < alive_count; i++) {
    arr[i] = 0;
  }

  // Initialize mapa
  int mapa[alive_count];
  for(int x = 0; x < alive_count; x++) {
    // mapa[x] = (seed = (9301 * seed + 49297) % 233280) / 233280 * alive_count | 0;
    seed = (9301 * seed + 49297) % 233280;
    // std::cout << "Seed is " << seed << std::endl;
    // From debugging the JS code the in between operation being a double is important
    double temp = seed;
    temp = temp/ 233280;
    //std::cout << "Temp is " << temp << std::endl;
    temp = temp * alive_count;
    //std::cout << "Temp is " << temp << std::endl;
    temp = int64_t(temp) | 0;
    //std::cout << "Temp is " << temp << std::endl;
    mapa[x] = temp;
    // std::cout << x << " => " << mapa[x] << std::endl << std::endl;
  }

  /* std::cout << "IN:";
  for(int j = 0; j < alive_count ; j++) {
    std::cout << arr[j] << ",";
  }
  std::cout << std::endl;
*/
  // Shuffle
  for(int i = alive_count - 1; i > 0; i--) {
    // arr[i] = arr.splice(mapa[size - 1 - i], 1, arr[i])[0]
    // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array/splice
    // array.splice(start[, deleteCount[, item1[, item2[, ...]]]])
    // so at the value contained in mapa[size -1 -i] in arr, remove one element, put the value
    // at arr[i] in its place and set arr[i] to the deleted value

    int mapa_index  = alive_count - 1 - i;
    int index_to_remove = mapa[mapa_index];
    int removed_value = arr[index_to_remove];
    // overwrite the removed value
    // std::cout << "Mapa index: " << mapa_index << std::endl;
    arr[index_to_remove] = arr[i];
    arr[i] = removed_value;

    /*
    for(int j = 0; j < alive_count ; j++) {
      std::cout << arr[j] << ",";
    }
    std::cout << std::endl;
    */
  }

  // now figure out which target to return
  int return_value;
  for(int i = 0; i < alive_count; i++) {
    // std::cout << "Index " << i << " has value " << arr[i] << std::endl;
    if(arr[i] > 0) {
      // std::cout << "Setting return_value to " << i << std::endl;
      return_value = i;
    }
  }
  return return_value;
}
// Simulates One fight between 2 Armies and writes results into left's LastFightData
inline bool simulateFight(Army & left, Army & right, bool verbose = false) {
    // left[0] and right[0] are the first monsters to fight
    ArmyCondition leftCondition;
    ArmyCondition rightCondition;

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

        // Apply Dicemaster max health bonus here, attack bonus applied during battle
        if (leftCondition.dice > -1) {
            leftCondition.maxHealths[leftCondition.dice] += rightCondition.seed % ((int)leftCondition.skillAmounts[leftCondition.dice] + 1);
            leftCondition.remainingHealths[leftCondition.dice] = leftCondition.maxHealths[leftCondition.dice];
        }

        if (rightCondition.dice > -1) {
            rightCondition.maxHealths[rightCondition.dice] += leftCondition.seed % ((int)rightCondition.skillAmounts[rightCondition.dice] + 1);
            rightCondition.remainingHealths[rightCondition.dice] = rightCondition.maxHealths[rightCondition.dice];
        }

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
    //TODO: handle 100 turn limit for non-wb, also handle it for wb better maybe
    while (leftCondition.monstersLost < leftCondition.armySize && rightCondition.monstersLost < rightCondition.armySize && turncounter < 100) {
        leftCondition.startNewTurn();
        rightCondition.startNewTurn();

        // Get damage with all relevant multipliers
        leftCondition.getDamage(turncounter, rightCondition);
        rightCondition.getDamage(turncounter, leftCondition);

        // Handle Revenge Damage before anything else. Revenge Damage caused through aoe is ignored
        if (leftCondition.skillTypes[leftCondition.monstersLost] == REVENGE &&
            leftCondition.remainingHealths[leftCondition.monstersLost] <= rightCondition.turnData.baseDamage) {
            leftCondition.turnData.aoeDamage += (int) round((double) leftCondition.lineup[leftCondition.monstersLost]->damage * leftCondition.skillAmounts[leftCondition.monstersLost]);
        }
        if (rightCondition.skillTypes[rightCondition.monstersLost] == REVENGE &&
            rightCondition.remainingHealths[rightCondition.monstersLost] <= leftCondition.turnData.baseDamage) {
            rightCondition.turnData.aoeDamage += (int) round((double) rightCondition.lineup[rightCondition.monstersLost]->damage * rightCondition.skillAmounts[rightCondition.monstersLost]);
        }
        // Handle Deathstrike Damage before anything else. Deathstrike Damage caused through aoe is ignored
        if (leftCondition.skillTypes[leftCondition.monstersLost] == DEATHSTRIKE &&
            leftCondition.remainingHealths[leftCondition.monstersLost] <= rightCondition.turnData.baseDamage) {
            leftCondition.turnData.baseDamage += leftCondition.skillAmounts[leftCondition.monstersLost];
        }
        if (rightCondition.skillTypes[rightCondition.monstersLost] == DEATHSTRIKE &&
            rightCondition.remainingHealths[rightCondition.monstersLost] <= leftCondition.turnData.baseDamage) {
            rightCondition.turnData.baseDamage += rightCondition.skillAmounts[rightCondition.monstersLost];
        }

        left.lastFightData.leftAoeDamage += rightCondition.turnData.aoeDamage;
        left.lastFightData.rightAoeDamage += leftCondition.turnData.aoeDamage;

        // Check if anything died as a result
        leftCondition.resolveDamage(rightCondition.turnData);
        rightCondition.resolveDamage(leftCondition.turnData);

        turncounter++;

        if (verbose) {
            std::cout << std::endl << "After Turn " << turncounter << ":" << std::endl;

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

    // how 100 turn limit is handled for WB
    if (turncounter >= 100 && rightCondition.worldboss == true) {
        leftCondition.monstersLost = leftCondition.armySize;
    }

    // write all the results into a FightResult
    left.lastFightData.dominated = false;
    left.lastFightData.turncounter = turncounter;

    if (leftCondition.monstersLost >= leftCondition.armySize) { //draws count as right wins.
        left.lastFightData.monstersLost = rightCondition.monstersLost;
        left.lastFightData.berserk = rightCondition.berserkProcs;
        if (rightCondition.monstersLost < rightCondition.armySize) {
            left.lastFightData.frontHealth = (int64_t) (rightCondition.remainingHealths[rightCondition.monstersLost]);
        } else {
            left.lastFightData.frontHealth = 0;
        }
        return false;
    } else {
        left.lastFightData.monstersLost = leftCondition.monstersLost;
        left.lastFightData.frontHealth = (int64_t) (leftCondition.remainingHealths[leftCondition.monstersLost]);
        left.lastFightData.berserk = leftCondition.berserkProcs;
        return true;
    }
}

// Function determining if a monster is strictly better than another
bool isBetter(Monster * a, Monster * b, bool considerAbilities = false);

#endif
