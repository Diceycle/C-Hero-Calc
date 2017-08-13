#include "cosmosClasses.h"

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
bool isCheaper(Monster * a, Monster * b) {
    return a->cost < b->cost;
}

FightResult::FightResult() {
    this->valid = false;
}

bool FightResult::operator <=(FightResult & toCompare) { // both results are expected to not have won
    if(this->leftAoeDamage < toCompare.leftAoeDamage || this->rightAoeDamage > toCompare.rightAoeDamage) {
        return false; // left is not certainly worse thn right
    }
    if (this->monstersLost == toCompare.monstersLost) {
        return this->damage <= toCompare.damage; // less damage dealt to the enemy -> left is worse
    } else {
        return this->monstersLost < toCompare.monstersLost; // less monsters destroyed on the enemy side -> left is worse
    }
}

bool FightResult::operator >=(FightResult & toCompare) {
    return toCompare <= *this;
}

Army::Army(vector<Monster*> monsters) {
    this->followerCost = 0;
    this->monsterAmount = 0;
    this->lastFightData = FightResult();
    
    for(size_t i = 0; i < monsters.size(); i++) {
        this->add(monsters[i]);
    }
}

void Army::add(Monster * m) {
    this->monsters[monsterAmount] = m;
    this->followerCost += m->cost;
    this->monsterAmount++;
}

void Army::print() {
    cout << "(Followers: " << setw(7) << this->followerCost << " | ";
    for (int i = this->monsterAmount-1; i >= 0; i--) {
        cout << this->monsters[i]->name << " "; // Print in reversed Order
    } cout << ")" << endl; 
}

// Function for sorting FightResults by followers (ascending)
bool hasFewerFollowers(const Army & a, const Army & b) {
    return (a.followerCost < b.followerCost);
}