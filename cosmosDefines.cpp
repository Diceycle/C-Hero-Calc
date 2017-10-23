#include "cosmosDefines.h"

std::map<std::string, int8_t> monsterMap {}; // Maps monster Names to their pointers (includes heroes)

std::vector<int8_t> availableMonsters {}; // Contains pointers to raw Monster Data from a1 to f10, will be sorted by follower cost
std::vector<int8_t> availableHeroes {};

// Make sure all the values are set
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

// Initialize Hero Data
void initializeUserHeroes(std::vector<int> levels) {
    for (size_t i = 0; i < baseHeroes.size(); i++) {
        if (levels[i] > 0) {
            availableHeroes.push_back(addLeveledHero(baseHeroes[i], levels[i]));
        }
    }
}

// Create a new hero with leveled stats and return it
Monster getLeveledHero(const Monster & m, int rarity, int level) {
    int points = level-1;
    if (rarity == 1) {
        points = 2 * points;
    } else if (rarity == 2) {
        points = 6 * points;
    }
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

// Add a leveled hero to the databse 
int8_t addLeveledHero(Monster hero, int level) {
    Monster m = getLeveledHero(hero, rarities.at(hero.name), level);
    monsterReference.emplace_back(m);
    
    return monsterReference.size() - 1;
}