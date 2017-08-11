#ifndef BATTLE_LOGIC_HEADER
#define BATTLE_LOGIC_HEADER

#include <vector>
#include <cmath>

#include "cosmosClasses.h"

const float elementalBoost = 1.5;
extern int totalFightsSimulated;

// Function determining if a monster is strictly better than another
bool isBetter(Monster * a, Monster * b, bool considerAbilities = false);

// Simulates One fight between 2 Armies
void simulateFight(FightResult & result, Army & left, Army & right, bool verbose = false);

#endif