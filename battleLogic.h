#ifndef BATTLE_LOGIC_HEADER
#define BATTLE_LOGIC_HEADER

#include <vector>
#include <cmath>

#include "cosmosClasses.h"

const float elementalBoost = 1.5;
extern int totalFightsSimulated;

struct TurnData {
    int16_t baseDamage = 0;
    int16_t buffDamage = 0;
    int16_t aoeDamage = 0;
    int16_t paoeDamage = 0;
    int16_t protection = 0;
    int16_t healing = 0;
};

class ArmyCondition {
    public: 
        size_t armySize;
        Monster * lineup[6];
        SkillType skillTypes[6];
        Element skillTargets[6];
        float skillAmounts[6];
        
        int8_t rainbowCondition; // for rainbow ability
        int8_t pureMonsters; // for friends ability
        
        size_t monstersLost;
        
        int16_t frontDamageTaken;
        int16_t aoeDamageTaken;
        int8_t berserkProcs;
        
        TurnData turnData;
        
        ArmyCondition();
        
        void init(Army & army);
        bool startNewTurn();
        void getDamage(const int8_t turncounter, const Element opposingElement);
        void resolveDamage(const TurnData & opposing);
    
};

// Function determining if a monster is strictly better than another
bool isBetter(Monster * a, Monster * b, bool considerAbilities = false);

// Simulates One fight between 2 Armies
void simulateFight(Army & left, Army & right, bool verbose = false);

#endif