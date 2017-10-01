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
void simulateFight(Army & left, Army & right, bool verbose = false);

class FightData {
public:
	FightData(Army &army);

	size_t lost;
	size_t armySize;
	int8_t *lineup;

	int frontDamageTaken;
	int healing;
	int cumAoeDamageTaken;
	float berserkProcs;
};

#endif