#include "cosmosDefines.h"

std::map<std::string, int8_t> monsterMap {}; // Maps monster Names to their indices in monsterReference from cosmosClasses

std::vector<int8_t> availableMonsters {}; // Contains indices of raw Monster Data from a1 to f15, will be sorted by follower cost
std::vector<int8_t> availableHeroes {}; // Contains all user heroes' indices 

// Clean up all monster related vectors and sort the monsterBaseList
// Also fills the map used to parse strings into monsters
// Must be called before any input can be processed
void initMonsterData() {
    // Sort MonsterList by followers
    sort(monsterBaseList.begin(), monsterBaseList.end(), isCheaper);

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
    for (size_t i = 0; i < monsterBaseList.size(); i++) {
        if (minimumMonsterCost <= monsterBaseList[i].cost) {
            availableMonsters.push_back(i); // Kinda Dirty but I know that the normal mobs come first in the reference
        }
    }
}

// Initialize Hero Data. Must be called after filterMonsterData
void initializeUserHeroes(std::vector<int> levels) {
    for (size_t i = 0; i < baseHeroes.size(); i++) {
        if (levels[i] > 0) {
            availableHeroes.push_back(addLeveledHero(baseHeroes[i], levels[i]));
        }
    }
}

// Create a new hero with leveled stats and return it
Monster getLeveledHero(const Monster & m, int level) {
    HeroRarity rarity = rarities.at(m.name);
    int points = rarity * level-1;
    int value = m.hp + m.damage;
    return Monster(
        round(m.hp + points * ((double)m.hp) / value),
        m.damage + round(points * ((double)m.damage) / value),
        m.cost,
        m.name + ":" + std::to_string(level),
        m.element,
        m.skill
    );
}

// Add a leveled hero to the databse and return its corresponding index
int8_t addLeveledHero(Monster hero, int level) {
    Monster m = getLeveledHero(hero, level);
    monsterReference.emplace_back(m);
    
    return monsterReference.size() - 1;
}