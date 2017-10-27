#ifndef COSMOS_CLASSES_HEADER
#define COSMOS_CLASSES_HEADER

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <sstream>

const size_t ARMY_MAX_SIZE = 6;
const size_t ARMY_MAX_BRUTEFORCEABLE_SIZE = 4;

const std::string HEROLEVEL_SEPARATOR = ":";

// Define types of HeroSkills and Elements
enum SkillType {
    NOTHING,    // Base Skill used by normal monsters
    BUFF,       // Increases Damage of own army
    PROTECT,    // Reduces incoming damage vs the own army
    AOE,        // Damages the entire opposing army every turn
    P_AOE,       // If this monster attacks it also damages every monster behind the attacked
    HEAL,       // Heals the entire own army every turn
    BERSERK,    // Every attack this monster makes multiplies its own damage 
    FRIENDS,    // This monster receives a damage multiplicator for every NORMAL monster behind it
    CHAMPION,   // This monster has the buff and protect ability at the same time
    ADAPT,      // This monster deals more damage vs certain elements
    RAINBOW,    // This monster receives a damage buff if monsters of every element are behind it
    TRAINING,   // This monster receives a damage buff for every turn that passed
    WITHER,     // This monster's hp decrease after every attack it survives
    REVENGE     // After this onster dies it damages the entire opposing army
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

// Defines Skills of Heros
struct HeroSkill {
    SkillType type;
    Element target;
    Element sourceElement;
    float amount;
};
static HeroSkill NONE = HeroSkill({NOTHING, AIR, AIR, 1}); // base skill used for normal monsters

// Defines a Monster or Hero
class Monster {
    public :
        int hp;
        int damage;
        int cost;
        bool isHero;
        std::string baseName;
        std::string name;
        Element element;
        HeroSkill skill;
        
        Monster(int hp, int damage, int cost, std::string name, Element element, HeroSkill skill = NONE);
        Monster();
};

// Function for sorting Monsters by cost (ascending)
bool isCheaper(const Monster & a, const Monster & b);

// Defines the results of a fight between two armies; monstersLost and damage desribe the condition of the winning side
class FightResult {
    public :
        int16_t damage;             // how much damage dealt to the current leading mob of the winning side
        int16_t leftAoeDamage;      // how much aoe damage left took
        int16_t rightAoeDamage;     // how much aoe damage right took
        int8_t berserk;            // berserk multiplier, if there is a berserker in the front
        int8_t monstersLost;    // how many mobs lost on the winning side (the other side lost all)
        int8_t turncounter;     // how many turns have passed since the battle started
        bool valid;             // If the result is valid
        bool rightWon;          // false -> left win, true -> right win.
        bool dominated;         // If the result is worse than another
            
        FightResult();
        
    inline bool operator <= (const FightResult & toCompare) const;
};

inline bool FightResult::operator <=(const FightResult & toCompare) const { // both results are expected to not have won
    if(this->leftAoeDamage < toCompare.leftAoeDamage || this->rightAoeDamage > toCompare.rightAoeDamage) {
        return false; // left is not certainly worse than right
    }
    if (this->monstersLost == toCompare.monstersLost) {
        return this->damage <= toCompare.damage; // less damage dealt to the enemy -> left is worse
    } else {
        return this->monstersLost < toCompare.monstersLost; // less monsters destroyed on the enemy side -> left is worse
    }
} 

// Defines a single lineup of monsters
class Army {
    public:
        FightResult lastFightData;
        int32_t followerCost;
        int8_t monsters[ARMY_MAX_SIZE];
        int8_t monsterAmount;
        
        inline void add(const int8_t m);
        std::string toString();
        void print();
        Army(std::vector<int8_t> monsters = {});
};

extern std::vector<Monster> monsterReference; // Will be filled with leveled heroes if needed (determined by input)

// Function for sorting FightResults by followers (ascending)
inline bool hasFewerFollowers(const Army & a, const Army & b) {
    return (a.followerCost < b.followerCost);
}

// Add monster to the back of the army
inline void Army::add(const int8_t m) {
    this->monsters[monsterAmount] = m;
    this->followerCost += monsterReference[m].cost;
    this->monsterAmount++;
}

#endif