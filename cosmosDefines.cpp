#include "cosmosDefines.h"

vector<Monster> heroReference {}; // Will be filled with leveled heroes if needed (determined by input)

vector<Monster *> monsterList {}; // Contains pointers to raw Monster Data from a1 to f10, will be sorted by follower cost
map<string, Monster *> monsterMap {}; // Maps monster Names to their pointers (includes heroes)

vector<Monster *> availableHeroes {};

// Make sure all the values are set
void initMonsterData() {
    // Initialize Monster Data
    monsterList.clear();
    monsterMap.clear();
    for (size_t i = 0; i < monsterBaseList.size(); i++) {
        monsterList.push_back(&monsterBaseList[i]);
        monsterMap.insert(pair<string, Monster *>(monsterBaseList[i].name, &monsterBaseList[i]));
    }
    
    // Sort MonsterList by followers
    sort(monsterList.begin(), monsterList.end(), isCheaper);
    
    // Reserve some space for heroes, otherwise pointers will become invalid (Important!)
    heroReference.clear();
    availableHeroes.clear();
    heroReference.reserve(baseHeroes.size()*5);
}

// Filter MonsterList by cost. User can specify if he wants to exclude cheap monsters
void filterMonsterData(int minimumMonsterCost) {
    while (monsterList.size() > 0 && monsterList[0]->cost <= minimumMonsterCost) {
        monsterList.erase(monsterList.begin());
    }
    if (minimumMonsterCost == -1) {
        monsterList = {};
    }
}

// Initialize Hero Data
void initializeUserHeroes(vector<int> levels) {
    for (size_t i = 0; i < baseHeroes.size(); i++) {
        if (levels[i] > 0) {
            addLeveledHero(baseHeroes[i], levels[i]);
            availableHeroes.push_back(monsterMap.at(baseHeroes[i].name + ":" + to_string(levels[i])));
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
        m.name + ":" + to_string(level),
        m.element,
        m.skill
    );
}

// Add a leveled hero to the databse 
void addLeveledHero(Monster hero, int level) {
    Monster m = getLeveledHero(hero, rarities.at(hero.name), level);
    heroReference.emplace_back(m);
    
    monsterMap.insert(pair<string, Monster *>(m.name, &(heroReference[heroReference.size() -1])));
}