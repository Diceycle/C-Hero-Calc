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
const std::string VERSION = "3.0.1.4";

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

// Worldboss Health maximum larger values get lost because FightResults only accept int16_t too
const int16_t WORLDBOSS_HEALTH = 0; //32000;

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
	AOEZero_L,	// AOE damage (equal to lvl, undampened) at turn 0 / healable / after leprechaun's skill

    BERSERK,    // Every attack this monster makes multiplies its own damage 
    FRIENDS,    // This monster receives a damage multiplicator for every NORMAL monster behind it
    ADAPT,      // This monster deals more damage vs certain elements
    RAINBOW,    // This monster receives a damage buff if monsters of every element are behind it
    TRAINING,   // This monster receives a damage buff for every turn that passed
    
    WITHER,     // This monster's hp decrease after every attack it survives
    
    REVENGE,    // After this monster dies it damages the entire opposing army
    PIERCE,     // If this monster attacks it also damages every monster behind the attacked
    VALKYRIE,   // This monsters damage is done to all monsters, the value beeing reduced for each monster it hits. Hardcoded to 50%
    TRAMPLE,    // This monsters attack damages n units from the front 
    
    BUFF_L,     // Buff ability that scales with level
    PROTECT_L,  // Protect ability that scales with level
    CHAMPION_L, // Champion ability that scales with level
    AOE_L,      // AOE ability that scales with level
    HEAL_L,     // Heal ability that scales with level
    LIFESTEAL_L,// Lifesteal ability that scales with level
    DAMPEN_L,   // Dampen Ability that scales with level

	BEER,		// Scales opponent unit health as well as max health by (no. unit in your lane / no. unit in enemy lane)
	GROW,		// Increase stats gained per lvl
	COUNTER,    // counters % of inflicted damage 
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
const float elementalBoost = 1.5; // Damage Boost if element has advantage over another

enum HeroRarity { 
    NO_HERO = 0,
    COMMON = 1, 
    RARE = 2, 
    LEGENDARY = 6, 
    ASCENDED = 12, // Values define how many stat points per level a hero of this rarity gets
    WORLDBOSS
};

// Defines Skills of Heros
struct HeroSkill {
    SkillType skillType;
    Element target;
    Element sourceElement;          // Not used anywhere
    float amount;                   // Contains various information dependend on the type
    bool violatesFightResults;      // True if a hero invalidates the data in FightResults i9f he is added to the army
    bool hasAoe;               
    bool hasHeal;
    bool hasAsymmetricAoe;          // Aoe that doesnt damage the entire enemy army equally Ex.: Valkyrie
    
    HeroSkill(SkillType aType, Element aTarget, Element aSource, float anAmount);
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
extern std::vector<Monster> baseHeroes; // Raw, unleveld Hero Data, holds actual Objects
void initBaseHeroes();
extern std::vector<std::vector<std::string>> quests; // Quest Lineups from the game
void initQuests();

// Fills all references and storages with real data.
// Must be called before any other operation on monsters or input
void initGameData();

// Filter monsters according to user input. Fills the available-references
// Must be called before any instance can be solved
void filterMonsterData(FollowerCount minimumMonsterCost, FollowerCount maximumArmyCost);

// Defines the results of a fight between two armies; monstersLost and damage desribe the condition of the winning side
// The idea behind FightResults is to save the data and be able to restore the state when the battle ended easily. 
// When solving an Instance many armies with the same 5 Monsters fight the target Army again with a different 6th Monster
// Using FightResults you only have to calculate the fight with 5 Monsters once and then pick up where you left off with the 6th monster.
// Ideally that would work for any battle. Unfortunately as Abilities grew more complex the amount of data needing to be saved outweight the benefit of not having to run the fight again.
// So a few Abilities Invalidate FightResults. Like BUFF. Since the first 5 Monsters would have had more Attack with the buff, the battle might have played out differently. 
// So the data in here can't be used if f.e. a BUFF hero is added to the Army
// Similarly some Abilities like Valkyrie produce a Fight state that can't be efficiently captured in a FightResult. 
// If one of those heroes is in an Army its FightResult is always invalid
//
// In a FightResult it is always implied that the target won against the proposed solution. 

using DamageType	= short;	// change data type here to track large damage; default = short

struct FightResult {
    DamageType frontHealth;        // how much health remaining to the current leading mob of the winning side
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
        
        Army(std::vector<MonsterIndex> someMonsters = {}) :
            followerCost(0),
            monsterAmount(0)
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

// Function for sorting Monsters by cost (ascending)
inline bool isCheaper(const Monster & a, const Monster & b) {
    return a.cost < b.cost;
}

// Add a leveled hero to the databse and return its corresponding index
MonsterIndex addLeveledHero(Monster & hero, int level);

// Returns the index of a quest if the lineup is the same. Returns -1 if not a quest
int isQuest(Army & army);

// Get the index of a monster corresponding to the unique id it is given ingame
int getRealIndex(Monster & monster);

// Custom ceil function to avoid excessive casting. Hardcoded to be effective on 16Bit ints
// The accuracy breaks down at about 1/10000 but should be accurate enough for most purposes
inline int castCeil(float f) {
    return 32768 - (int)(32768.0f - f);
}

#endif