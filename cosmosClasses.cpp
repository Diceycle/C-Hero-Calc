#include "cosmosClasses.h"

vector<Monster> monsterReference {}; // Will be filled with leveled heroes if needed (determined by input)

Monster::Monster(int hp, int damage, int cost, string name, Element element, HeroSkill skill) {
    this->hp = hp;
    this->damage = damage;
    this->cost = cost;
    this->name = name;
    this->element = element;
    this->skill = skill;
    
    this->isHero = (skill.type != nothing);
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

Army::Army(vector<int8_t> monsters) {
    this->followerCost = 0;
    this->monsterAmount = 0;
    this->lastFightData = FightResult();
    
    for(size_t i = 0; i < monsters.size(); i++) {
        this->add(monsters[i]);
    }
}

string Army::toString() {
    stringstream s;
    s << "[Followers: " << setw(7) << this->followerCost << " | ";
    for (int i = this->monsterAmount-1; i >= 0; i--) {
        s << monsterReference[this->monsters[i]].name << " "; // Print in reversed Order
    } s << "<==]"; 
    return s.str();
}

void Army::print() {
    cout << this->toString() << endl;
}