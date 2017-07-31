#ifndef COSMOS_CLASSES_HEADER
#define COSMOS_CLASSES_HEADER

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

using namespace std;

// Define types of HeroSkills and Elements
enum SkillType {nothing, buff, protect, aoe, pAoe, heal, berserk};
enum Element {earth, air, water, fire, all, self}; // also used for hero skill targets

// Defines Skills of Heros
struct HeroSkill {
    SkillType type;
    Element target;
    Element sourceElement;
    int amount;
};
static HeroSkill none = HeroSkill({nothing, air, air, 1}); // base skill used for normal monsters

// Defines a fight that was run before. Used for skipping parts of a fight with no new heroSkills involved
struct KnownFight {
    size_t monstersLost; 
    int damage;
    int berserk;
    int leftAoeDamage;
    int rightAoeDamage;
    bool valid;
};

// Defines a Monster or Hero
class Monster{
    public :
        int hp;
        int damage;
        int cost;
        bool isHero;
        string name;
        Element element;
        HeroSkill skill;
        
        Monster(int hp, int damage, int cost, string name, Element element, HeroSkill skill = none);
        Monster();
};

// Defines a single lineup of monsters
class Army {
    public:
        vector<Monster *> monsters;
        KnownFight precomputedFight;
        int followerCost;
        
        void add(Monster * m);
        void print();
        Army(vector<Monster *> monsters = {});
};

// Defines the results of a fight between two armies; monstersLost and damage desribe the condition of the winning side
class FightResult {
    public :
        Army * source; // this desrcribes the left side
        bool rightWon; //false -> left win, true -> right win.
        size_t monstersLost; // how many mobs lost on the winning side (the other side lost all)
        int damage; // how much damage dealt to the current leading mob of the winning side
        int berserk; // berserk multiplier, if there is a berserker in the front
        bool dominated;
        int leftAoeDamage; // how much aoe damage left took
        int rightAoeDamage; // how much aoe damage right took
            
        FightResult();
        
    // Comparator for fightResults. Heavily used TODO: Optimize
    bool operator <= (FightResult & toCompare);
    bool operator >= (FightResult & toCompare);
};

#endif