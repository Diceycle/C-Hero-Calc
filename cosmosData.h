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

// Constants defining the basic structure of armies
const size_t ARMY_MAX_SIZE = 6;
const size_t TOURNAMENT_LINES = 5;
const size_t ARMY_MAX_BRUTEFORCEABLE_SIZE = 4;
const std::string HEROLEVEL_SEPARATOR = ":";
const int INDEX_NO_MONSTER = -1;

const int16_t WORLDBOSS_HEALTH = 32000;

// Define types of HeroSkills, Elements and Rarities
enum SkillType {
    NOTHING,    // Base Skill used by normal monsters
    
    BUFF,       // Increases Damage of own army
    PROTECT,    // Reduces incoming damage vs the own army
    CHAMPION,   // This monster has the buff and protect ability at the same time
    
    AOE,        // Damages the entire opposing army every turn
    HEAL,       // Heals the entire own army every turn
    LIFESTEAL,  // Combines the Aoe and Heal ability into one
    
    BERSERK,    // Every attack this monster makes multiplies its own damage 
    FRIENDS,    // This monster receives a damage multiplicator for every NORMAL monster behind it
    ADAPT,      // This monster deals more damage vs certain elements
    RAINBOW,    // This monster receives a damage buff if monsters of every element are behind it
    TRAINING,   // This monster receives a damage buff for every turn that passed
    
    WITHER,     // This monster's hp decrease after every attack it survives
    
    REVENGE,    // After this monster dies it damages the entire opposing army
    PIERCE,     // If this monster attacks it also damages every monster behind the attacked
    VALKYRIE,   // This monsters damage is done to all monsters, the value beeing reduced for each monster it hits. Hardcoded to 50%
    
    BUFF_L,     // Buff ability that scales with level
    PROTECT_L,  // Protect ability that scales with level
    CHAMPION_L, // Champion ability that scales with level
    AOE_L,      // AOE ability that scales with level
    HEAL_L,     // Heal ability that scales with level
    LIFESTEAL_L,// Lifesteal ability that scales with level
};

enum Element {
    EARTH   = 0,
    AIR     = 1, 
    WATER   = 2, 
    FIRE    = 3, // Discrete Values needed to quickly determine counters
    ALL,         
    SELF         // These Values are used to specify targets of hero skills
};
const Element counter [] { FIRE, EARTH, AIR, WATER, SELF, SELF }; // Elemental Advantages earth = 0 -> counter[0] = fire -> fire has advantage over earth

enum HeroRarity { 
    NO_HERO = 0,
    COMMON = 1, 
    RARE = 2, 
    LEGENDARY = 6, // Values define how many stat points per level a hero of this rarity gets
    ASCENDED = 12,
    WORLDBOSS
};

// Defines Skills of Heros
struct HeroSkill {
    SkillType skillType;
    Element target;
    Element sourceElement;
    float amount;
    bool violatesFightResults;
    bool hasAoe;
    bool hasAsymmetricAoe;
    
    HeroSkill(SkillType aType, Element aTarget, Element aSource, float anAmount);
    HeroSkill() {};
};
const HeroSkill NO_SKILL = HeroSkill({NOTHING, AIR, AIR, 1}); // base skill used for normal monsters

// Defines a Monster or Hero
class Monster {
    private:
        Monster(int hp, int damage, int cost, std::string name, Element element, HeroRarity rarity, HeroSkill skill, int level);
        
    public :
        int hp;
        int damage;
        int cost;
        std::string baseName; // Hero names without levels
        Element element;
        
        // Hero Stuff
        HeroRarity rarity;
        HeroSkill skill;
        int level;
        
        std::string name; // display name
        
        Monster(int hp, int damage, int cost, std::string name, Element element);
        Monster(int hp, int damage, std::string name, Element element, HeroRarity rarity, HeroSkill skill);
        Monster(const Monster & baseHero, int level);
        Monster() {};
        
        std::string toJSON();
};

// Access tools for monsters 
extern std::map<std::string, int8_t> monsterMap; // Maps monster Names to their indices in monsterReference
extern std::vector<Monster> monsterReference; // Global lookup for monster stats indices of monsters here can be used instead of the objects
extern std::vector<int8_t> availableMonsters; // Contains indices of all monsters the user allows. Is affected by filters
extern std::vector<int8_t> availableHeroes; // Contains all user heroes' indices 

// Storage for Game Data
extern std::vector<Monster> monsterBaseList; // Raw Monster Data, holds the actual Objects
void initMonsters();
extern std::vector<Monster> baseHeroes; // Raw, unleveld Hero Data, holds actual Objects
void initBaseHeroes();
extern std::vector<std::vector<std::string>> quests; // Quest Lineup from the game
void initQuests();

// Fills all references and storages with real data.
// Must be called before any other operation on monsters or input
void initGameData();

// Filter monsters according to user input. Fills the available-references
// Must be called before any instance can be solved
void filterMonsterData(int minimumMonsterCost);

// Defines the results of a fight between two armies; monstersLost and damage desribe the condition of the winning side
struct FightResult {
    int16_t frontHealth;        // how much health remaining to the current leading mob of the winning side
    int16_t leftAoeDamage;      // how much aoe damage left took
    int16_t rightAoeDamage;     // how much aoe damage right took
    int8_t berserk;            // berserk multiplier, if there is a berserker in the front
    int8_t monstersLost;    // how many mobs lost on the winning side (the other side lost all)
    int8_t turncounter;     // how many turns have passed since the battle started
    bool rightWon;          // false -> left win, true -> right win.
    bool valid;             // If the result is valid
    bool dominated;         // If the result is worse than another
                
    FightResult() : valid(false) {}
    
    bool operator <=(const FightResult & toCompare) const { // both results are expected to not have won
        if(this->leftAoeDamage < toCompare.leftAoeDamage || this->rightAoeDamage > toCompare.rightAoeDamage) {
            return false; // left is not certainly worse than right
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
        int32_t followerCost;
        int8_t monsters[ARMY_MAX_SIZE];
        int8_t monsterAmount;
        
        Army(std::vector<int8_t> someMonsters = {}) :
            followerCost(0),
            monsterAmount(0)
        {
            for(size_t i = 0; i < someMonsters.size(); i++) {
                this->add(someMonsters[i]);
            }
        }
        
        // Add monster to the back of the army
        void add(const int8_t m) {
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
int8_t addLeveledHero(Monster & hero, int level);

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