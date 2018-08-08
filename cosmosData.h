#ifndef COSMOS_CLASSES_HEADER
#define COSMOS_CLASSES_HEADER

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <map>

// Version number not used anywhere except in output to know immediately which version the user is running
const std::string VERSION = "3.0.2.0";

const size_t GIGABYTE = ((size_t) (1) << 30);

// Alias for dataTypes makes Code more readable
// An index describing a spot in the monsterReference.
using MonsterIndex = uint8_t;
// A type used to denote FollowerCounts.
using FollowerCount = uint32_t;


// Constants defining the basic structure of armies
const size_t ARMY_MAX_SIZE = 6;
const size_t ARMY_MAX_BRUTEFORCEABLE_SIZE = 4;
const std::string HEROLEVEL_SEPARATOR = ":";

// Needed for BattleReplays
const size_t TOURNAMENT_LINES = 5;
const int INDEX_NO_MONSTER = -1;

// Worldboss Health maximum larger values, maximum value is decided by DamageType
const int64_t WORLDBOSS_HEALTH = 0;

// Define types of HeroSkills, Elements and Rarities
enum SkillType {
    NOTHING,    // Base Skill used by normal monsters

    BUFF,       // Increases Damage of own army
    PROTECT,    // Reduces incoming damage vs the own army
    CHAMPION,   // This monster has the buff and protect ability at the same time

    AOE,        // Damages the entire opposing army every turn
    HEAL,       // Heals the entire own army every turn
    LIFESTEAL,  // Combines the Aoe and Heal ability into one
    DAMPEN,     // Reduces the Effects of AOE percentually
    AOEZero_L,  // AOE damage (equal to lvl, undampened) at turn 0 / healable / after leprechaun's skill

    BERSERK,    // Every attack this monster makes multiplies its own damage
    FRIENDS,    // This monster receives a damage multiplicator for every NORMAL monster behind it
    ADAPT,      // This monster deals more damage vs certain elements
    RAINBOW,    // This monster receives a damage buff if monsters of every element are behind it
    TRAINING,   // This monster receives a damage buff for every turn that passed

    WITHER,     // This monster's hp decrease after every attack it survives

    REVENGE,    // After this monster dies it damages the entire opposing army
    PIERCE,     // If this monster attacks it also damages every monster behind the attacked
    VALKYRIE,   // This monsters damage is done to all monsters, the value being reduced for each monster it hits.
    TRAMPLE,    // This monsters attack damages n units from the front

    BUFF_L,     // Buff ability that scales with level
    PROTECT_L,  // Protect ability that scales with level
    CHAMPION_L, // Champion ability that scales with level
    AOE_L,      // AOE ability that scales with level
    HEAL_L,     // Heal ability that scales with level
    LIFESTEAL_L,// Lifesteal ability that scales with level
    DAMPEN_L,   // Dampen Ability that scales with level

    BEER,       // Scales opponent unit health as well as max health by (no. unit in your lane / no. unit in enemy lane)
    GROW,       // Increase stats gained per lvl
    COUNTER,    // counters % of inflicted damage
    DICE,       // adds attack and defense at the start of battle from 0 to ability strength based on enemy starting lineup
    LUX,        // attacks an enemy based on turn number, enemy starting lineup, and number of enemies remaining
    CRIT,       // deals bonus damage based on enemy starting lineup and turn count
    EXPLODE,    // deals aoe damage when it kill an enemy
    ABSORB,     // prevents and takes a percentage of damage
    HATE,       // has extra elemental bonus, can't be treated as adapt due to order
};

enum Element {
    EARTH   = 0,
    AIR     = 1,
    WATER   = 2,
    FIRE    = 3,
    ALL     = 4, // Discrete Values needed to quickly determine counters
    SELF         // These Values are also used to specify targets of hero skills
};
const Element counter [] { FIRE, EARTH, AIR, WATER, SELF, SELF }; // Elemental Advantages Ex.: earth = 0 -> counter[0] = fire -> fire has advantage over earth
const double elementalBoost = 1.5; // Damage Boost if element has advantage over another

enum HeroRarity {
    NO_HERO = 0,
    COMMON = 1,
    RARE = 2,
    LEGENDARY = 6,
    ASCENDED = 12, // Values define how many stat points per level a hero of this rarity gets
    WORLDBOSS
};

// Defines Skills of Heroes
struct HeroSkill {
    SkillType skillType;
    Element target;
    Element sourceElement;          // Not used anywhere
    double amount;                   // Contains various information depending on the type
    bool violatesFightResults;      // True if a hero invalidates the data in FightResults if he is added to the army
    bool hasAoe;
    bool hasHeal;
    bool hasAsymmetricAoe;          // Aoe that doesn't damage the entire enemy army equally Ex.: Valkyrie

    HeroSkill(SkillType aType, Element aTarget, Element aSource, double anAmount);
    HeroSkill() {};
};
const HeroSkill NO_SKILL = HeroSkill({NOTHING, AIR, AIR, 1}); // base skill used for normal monsters

// Defines a Monster or Hero
class Monster {
    private:
        Monster(int hp, int damage, FollowerCount cost, std::string name, Element element, HeroRarity rarity, HeroSkill skill, int level);

    public :
        int hp;
        int damage;
        FollowerCount cost;
        std::string baseName; // Hero names without levels
        Element element;

        // Hero Stuff
        HeroRarity rarity;
        HeroSkill skill;
        int level;

        std::string name; // display name

        Monster(int hp, int damage, FollowerCount cost, std::string name, Element element);
        Monster(int hp, int damage, std::string name, Element element, HeroRarity rarity, HeroSkill skill);
        Monster(const Monster & baseHero, int level);
        Monster() {};

        std::string toJSON();
};

// Access tools for monsters
extern std::map<std::string, MonsterIndex> monsterMap; // Maps monster Names to their indices in monsterReference used to parse input
extern std::vector<Monster> monsterReference; // Global lookup for monster stats. Enables using indices of monsters instead of the objects. Saves tons of memory. Also consumes less memory than pointers
extern std::vector<MonsterIndex> availableMonsters; // Contains indices of all monsters the user allows. Is affected by filters
extern std::vector<MonsterIndex> availableHeroes; // Contains all user heroes' indices

// Storage for Game Data
extern std::vector<Monster> monsterBaseList; // Raw Monster Data, holds the actual Objects
void initMonsters();
extern std::vector<Monster> baseHeroes; // Raw, unleveled Hero Data, holds actual Objects
void initBaseHeroes();
extern std::map<std::string, std::string> heroAliases; //Alternate or shorthand names for heroes
void initHeroAliases();
extern std::vector<std::vector<std::string>> quests; // Quest Lineups from the game
void initQuests();

// Fills all references and storages with real data.
// Must be called before any other operation on monsters or input
void initGameData();

// Filter monsters according to user input. Fills the available-references
// Must be called before any instance can be solved
void filterMonsterData(FollowerCount minimumMonsterCost, FollowerCount maximumArmyCost);

// Get the index of a monster corresponding to the unique id it is given ingame
int getRealIndex(Monster & monster);

// Defines the results of a fight between two armies; monstersLost and damage describe the condition of the winning side
// The idea behind FightResults is to save the data and be able to restore the state when the battle ended easily.
// When solving an Instance many armies with the same 5 Monsters fight the target Army again with a different 6th Monster
// Using FightResults you only have to calculate the fight with 5 Monsters once and then pick up where you left off with the 6th monster.
// Ideally that would work for any battle. Unfortunately as Abilities grew more complex the amount of data needing to be saved outweigh the benefit of not having to run the fight again.
// So a few Abilities Invalidate FightResults. Like BUFF. Since the first 5 Monsters would have had more Attack with the buff, the battle might have played out differently.
// So the data in here can't be used if f.e. a BUFF hero is added to the Army
// Similarly some Abilities like Valkyrie produce a Fight state that can't be efficiently captured in a FightResult.
// If one of those heroes is in an Army its FightResult is always invalid
//
// In a FightResult it is always implied that the target won against the proposed solution.

using DamageType    = long long;    // change data type here to track large damage; default = short

struct FightResult {
    DamageType frontHealth;     // how much health remaining to the current leading mob of the winning side
    int16_t leftAoeDamage;      // how much aoe damage left took
    int16_t rightAoeDamage;     // how much aoe damage right took
    int8_t berserk;             // berserk multiplier, if there is a berserker in the front
    int8_t monstersLost;        // how many mobs lost on the winning side (the other side lost all)
    int8_t turncounter;         // how many turns have passed since the battle started
    bool valid;                 // If the result is valid
    bool dominated;             // If the result is worse than another

    FightResult() : valid(false) {}

    // Comparator for FightResults Used to do dominance.
    bool operator <=(const FightResult & toCompare) const { // both results are expected to not have won against the target
        if(this->leftAoeDamage < toCompare.leftAoeDamage || this->rightAoeDamage > toCompare.rightAoeDamage) {
            return false; // left is not certainly worse than right because the AOE damage is different
        }
        if (this->monstersLost == toCompare.monstersLost) {
            return this->frontHealth > toCompare.frontHealth; // less damage dealt to the enemy -> left is worse
        } else {
            return this->monstersLost < toCompare.monstersLost; // less monsters destroyed on the enemy side -> left is worse
        }
    }
};

// Defines a single lineup of monsters
class Army {
    public:
        FightResult lastFightData;
        FollowerCount followerCost;
        MonsterIndex monsters[ARMY_MAX_SIZE];
        int8_t monsterAmount;
        int64_t seed;
        int64_t strength;

        Army(std::vector<MonsterIndex> someMonsters = {}) :
            followerCost(0),
            monsterAmount(0),
            strength(0)
        {
            for(size_t i = 0; i < someMonsters.size(); i++) {
                this->add(someMonsters[i]);
            }
        }

        // Add monster to the back of the army
        void add(const MonsterIndex m) {
            this->monsters[monsterAmount] = m;
            this->followerCost += monsterReference[m].cost;
            this->monsterAmount++;
            strength += pow(monsterReference[m].hp * monsterReference[m].damage, 1.5);

            // Seed takes into account empty spaces with lane size 6, recalculated each time monster is added
            // Any empty spaces are considered to be contiguous and frontmost as they are in DQ and quests
            int64_t newSeed = 1;
            for (int i = monsterAmount - 1; i >= 0; i--) {
                newSeed = newSeed * abs(getRealIndex(monsterReference[monsters[i]])) + 1;
            }
            // Simplification of loop for empty monsters (id: -1) contiguous and frontmost
            newSeed += 6 - monsterAmount;
            this->seed = newSeed;
        }
        bool isEmpty() {
            return (this->monsterAmount == 0);
        }

        std::string toString();
        std::string toJSON();
};
const size_t ARMY_BUFFER_MAX_SIZE = GIGABYTE / sizeof(Army);

// An instance to be solved by the program
struct Instance {
    Army target;
    size_t targetSize;
    size_t maxCombatants; // Used for Quest Difficulties

    FollowerCount followerUpperBound; // Contains either a used defined limit or the cost of bestSolution
    Army bestSolution;

    // Stats for Benchmarking
    time_t calculationTime;
    int totalFightsSimulated = 0;

    // Propagates from Hero Abilities
    bool hasAoe;
    bool hasHeal;
    bool hasAsymmetricAoe;
    bool hasBeer;
    bool hasGambler;
    bool hasWorldBoss;
    int64_t lowestBossHealth;

    std::vector<bool> monsterUsefulLast;

    void setTarget(Army aTarget);
};

// Function for sorting FightResults by followers (ascending)
inline bool hasFewerFollowers(const Army & a, const Army & b) {
    return ((!a.lastFightData.dominated && b.lastFightData.dominated) ||
            (a.lastFightData.dominated == b.lastFightData.dominated && a.followerCost < b.followerCost));
}

// Function for sorting armies to place better results first, then sorting by army strength for equal results
inline bool isMoreEfficient(const Army & a, const Army & b) {
    if (a.lastFightData.monstersLost != b.lastFightData.monstersLost) {
        return a.lastFightData.monstersLost > b.lastFightData.monstersLost;
    }
    else if (a.lastFightData.frontHealth != b.lastFightData.frontHealth) {
        return a.lastFightData.frontHealth > b.lastFightData.frontHealth;
    }
    else if (a.lastFightData.rightAoeDamage != b.lastFightData.rightAoeDamage) {
        return a.lastFightData.rightAoeDamage > b.lastFightData.rightAoeDamage;
    }
    else if (a.lastFightData.leftAoeDamage != b.lastFightData.leftAoeDamage) {
        return a.lastFightData.leftAoeDamage < b.lastFightData.leftAoeDamage;
    }
    else {
        return a.strength < b.strength;
    }
}

// Function for sorting Monsters by cost (ascending)
inline bool isCheaper(const Monster & a, const Monster & b) {
    return a.cost < b.cost;
}

// Add a leveled hero to the databse and return its corresponding index
MonsterIndex addLeveledHero(Monster & hero, int level);

// Returns the index of a quest if the lineup is the same. Returns -1 if not a quest
int isQuest(Army & army);

// Custom ceil function to avoid excessive casting. Hardcoded to be effective on 32bit ints
inline int castCeil(double f) {
    return 2147483647 - (int)(2147483647.0 - f);
}

#endif
