#include "cosmosClasses.h"

std::vector<Monster> monsterReference {}; // Will be filled with leveled heroes if needed (determined by input)

Monster::Monster(int hp, int damage, int cost, std::string name, Element element, HeroSkill skill) {
    this->hp = hp;
    this->damage = damage;
    this->cost = cost;
    this->name = name;
    this->element = element;
    this->skill = skill;
    
    this->isHero = (skill.type != NOTHING);
};

Monster::Monster() {
    isHero = false;
}

// Function for sorting Monsters by cost (ascending)
bool isCheaper(const Monster & a, const Monster & b) {
    return a.cost < b.cost;
}

FightResult::FightResult() {
    this->valid = false;
}

Army::Army(std::vector<int8_t> monsters) {
    this->followerCost = 0;
    this->monsterAmount = 0;
    this->lastFightData = FightResult();
    
    for(size_t i = 0; i < monsters.size(); i++) {
        this->add(monsters[i]);
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