#include "cosmosClasses.h"

std::vector<Monster> monsterReference {}; // Will be filled with leveled heroes if needed (determined by input)

// Necessary to put in a functions to avoid static initilazation order fiasco
std::string& HEROLEVEL_SEPARATOR() {
    static std::string * separator = new std::string(":");
    return *separator;
}

// Private constructor that is called by all public ones. Fully initializes all attributes
Monster::Monster(int someHp, int someDamage, int aCost, std::string aName, Element anElement, HeroRarity aRarity, HeroSkill aSkill, int aLevel) : 
    hp(someHp),
    damage(someDamage),
    cost(aCost),
    baseName(aName),
    element(anElement),
    rarity(aRarity),
    skill(aSkill),
    level(aLevel),
    name(aName)
{
    if (this->rarity != NO_HERO) {
        int points = this->rarity * (this->level-1);
        int value = this->hp + this->damage;
        this->name = this->baseName + HEROLEVEL_SEPARATOR() + std::to_string(this->level);
        this->hp = this->hp + (int) round((float) points * (float) this->hp / (float) value);
        this->damage = this->damage + (int) round((float) points * (float) this->damage / (float) value);
    }
}

// Contructor for normal Monsters
Monster::Monster(int someHp, int someDamage, int aCost, std::string aName, Element anElement) : 
    Monster(someHp, someDamage, aCost, aName, anElement, NO_HERO, NO_SKILL, 0) {}

// Constructor for Heroes
Monster::Monster(int someHp, int someDamage, std::string aName, Element anElement, HeroRarity aRarity, HeroSkill aSkill) :
    Monster(someHp, someDamage, 0, aName, anElement, aRarity, aSkill, 1) {}

// Constructor for leveled heroes
Monster::Monster(const Monster & baseHero, int aLevel) :
    Monster(baseHero.hp, baseHero.damage, baseHero.cost, baseHero.baseName, baseHero.element, baseHero.rarity, baseHero.skill, aLevel) {}

Monster::Monster() {}

// Function for sorting Monsters by cost (ascending)
bool isCheaper(const Monster & a, const Monster & b) {
    return a.cost < b.cost;
}

bool Army::isEmpty() {
    return (this->monsterAmount == 0);
}

std::string Army::toString() {
    std::stringstream s;
    s << "[Followers: " << std::setw(7) << this->followerCost << " | ";
    for (int i = this->monsterAmount-1; i >= 0; i--) {
        s << monsterReference[this->monsters[i]].name << " "; // Print in reversed Order
    } s << "<==]"; 
    return s.str();
}

}