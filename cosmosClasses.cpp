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

Army::Army(vector<Monster*> monsters) {
    this->followerCost = 0;
    this->precomputedFight = {0,0,0,0,0,false};
    this->monsters.clear();
    
    for(size_t i = 0; i < monsters.size(); i++) {
        this->add(monsters[i]);
    }
}

void Army::add(Monster * m) {
    this->monsters.push_back(m);
    this->followerCost += m->cost;
}

void Army::print() {
    cout << "(Followers: " << setw(7) << this->followerCost << " | ";
    for (size_t i = 0; i < this->monsters.size() ; i++) {
        cout << this->monsters[this->monsters.size() -1-i]->name << " "; // Print in reversed Order
    } cout << ")" << endl; 
}

FightResult::FightResult() {}

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