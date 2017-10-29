#include "cosmosClasses.h"

std::vector<Monster> monsterReference {}; // Will be filled with leveled heroes if needed (determined by input)

Monster::Monster(int someHp, int someDamage, int aCost, std::string aName, Element anElement, HeroSkill aSkill) : 
    hp(someHp),
    damage(someDamage),
    cost(aCost),
    name(aName),
    baseName(aName),
    element(anElement),
    skill(aSkill)
{
    this->isHero = (skill.type != NOTHING);
}

Monster::Monster() : isHero(false) {}

// Function for sorting Monsters by cost (ascending)
bool isCheaper(const Monster & a, const Monster & b) {
    return a.cost < b.cost;
}

FightResult::FightResult() : valid(false) {}

Army::Army(std::vector<int8_t> someMonsters) :
    followerCost(0),
    monsterAmount(0)
{
    for(size_t i = 0; i < someMonsters.size(); i++) {
        this->add(someMonsters[i]);
    }
}

std::string Army::toString() {
    std::stringstream s;
    s << "[Followers: " << std::setw(7) << this->followerCost << " | ";
    for (int i = this->monsterAmount-1; i >= 0; i--) {
        s << monsterReference[this->monsters[i]].name << " "; // Print in reversed Order
    } s << "<==]"; 
    return s.str();
}

void Army::print() {
    std::cout << this->toString() << std::endl;
}