#include "cosmosDefines.h"

std::map<std::string, int8_t> monsterMap {}; // Maps monster Names to their indices in monsterReference from cosmosClasses

std::vector<int8_t> availableMonsters {}; // Contains indices of raw Monster Data from a1 to f15, will be sorted by follower cost
std::vector<int8_t> availableHeroes {}; // Contains all user heroes' indices 

// Clean up all monster related vectors and sort the monsterBaseList
// Also fills the map used to parse strings into monsters
// Must be called before any input can be processed
void initMonsterData() {
    // Initialize Monster Data
    monsterReference.clear();
    monsterMap.clear();
    for (size_t i = 0; i < monsterBaseList.size(); i++) {
        monsterReference.push_back(monsterBaseList[i]);
        monsterMap.insert(std::pair<std::string, int8_t>(monsterBaseList[i].name, i));
    }

    availableMonsters.clear();
    availableHeroes.clear();
}

// Filter MonsterList by cost. User can specify if he wants to exclude cheap monsters
void filterMonsterData(int minimumMonsterCost) {
    std::vector<Monster> tempMonsterList = {monsterBaseList}; // Get a temporary list to sort
    sort(tempMonsterList.begin(), tempMonsterList.end(), isCheaper);
    for (size_t i = 0; i < tempMonsterList.size(); i++) {
        if (minimumMonsterCost <= tempMonsterList[i].cost) {
            availableMonsters.push_back((int8_t) i); // Kinda Dirty but I know that the normal mobs come first in the reference
        }
    }
}

// Add a leveled hero to the databse and return its corresponding index
int8_t addLeveledHero(Monster & hero, int level) {
    Monster m(hero, level);
    monsterReference.emplace_back(m);
    
    return (int8_t) (monsterReference.size() - 1);
}