#include "battleLogic.h"

int totalFightsSimulated = 0;

// Function determining if a monster is strictly better than another
bool isBetter(Monster * a, Monster * b, bool considerAbilities) {
	if (a->element == b->element) {
		return (a->damage >= b->damage) && (a->hp >= b->hp);
	} else { // a needs to be better than b even when b has elemental advantage, or a is at disadvantage
		return !considerAbilities && (a->damage >= b->damage * elementalBoost) && (a->hp >= b->hp * elementalBoost);
	}
}

// TODO: Implement MAX AOE Damage to make sure nothing gets revived
// Simulates One fight between 2 Armies and writes results into left's LastFightData
void simulateFight(Army & left, Army & right, bool verbose) {
	// left[0] and right[0] are the first to fight
	// Damage Application Order:
	//  1. Base Damage of creature
	//  2. Multiplicators of self       (friends, berserk)
	//  3. Buffs from heroes            (Hunter, Rei, etc.)
	//  4. Elemental Advantage          (f.e. Fire vs. Earth)
	//  5. Protection of enemy Side     (Nimue, Athos, etc.)
	//  6. AOE of friendly Side         (Tiny, Alpha, etc.)
	//  7. Healing of enemy Side        (Auri, Aeris, etc.)

	totalFightsSimulated++;

	size_t i;

	size_t leftLost = 0;
	size_t leftArmySize = left.monsterAmount;
	int8_t* leftLineup = left.monsters;

	int leftFrontDamageTaken = 0;
	int leftHealing = 0;
	int leftCumAoeDamageTaken = 0;
	float leftBerserkProcs = 0;

	size_t rightLost = 0;
	size_t rightArmySize = right.monsterAmount;
	int8_t* rightLineup = right.monsters;

	int rightFrontDamageTaken = 0;
	int rightHealing = 0;
	int rightCumAoeDamageTaken = 0;
	float rightBerserkProcs = 0;

	// If no heroes are in the army the result from the smaller army is still valid
	if (left.lastFightData.valid && !verbose) { 
		// Set pre-computed values to pick up where we left off
		leftLost                = leftArmySize-1; // All monsters of left died last fight only the new one counts
		leftFrontDamageTaken    = left.lastFightData.leftAoeDamage;
		leftCumAoeDamageTaken   = left.lastFightData.leftAoeDamage;
		rightLost               = left.lastFightData.monstersLost;
		rightFrontDamageTaken   = left.lastFightData.damage;
		rightCumAoeDamageTaken  = left.lastFightData.rightAoeDamage;
		rightBerserkProcs       = left.lastFightData.berserk;
	}

	// Values for skills  
	int damageLeft, damageRight;
	int damageBuffLeft, damageBuffRight;
	int protectionLeft, protectionRight;
	int aoeDamageLeft, aoeDamageRight;
	int paoeDamageLeft, paoeDamageRight;
	int healingLeft, healingRight;
	int pureMonstersLeft, pureMonstersRight;
	int elementalDifference;

	// hero temp Variables
	Monster * currentMonsterLeft;
	Monster * currentMonsterRight;
	HeroSkill * skill;
	SkillType skillTypeLeft[6];
	Element skillTargetLeft[6];
	float skillAmountLeft[6];
	SkillType skillTypeRight[6];
	Element skillTargetRight[6];
	float skillAmountRight[6];

	for (i = leftLost; i < leftArmySize; i++) {
		skill = &monsterReference[leftLineup[i]].skill;
		skillTypeLeft[i] = skill->type;
		skillTargetLeft[i] = skill->target;
		skillAmountLeft[i] = skill->amount;
	}

	for (i = rightLost; i < rightArmySize; i++) {
		skill = &monsterReference[rightLineup[i]].skill;
		skillTypeRight[i] = skill->type;
		skillTargetRight[i] = skill->target;
		skillAmountRight[i] = skill->amount;
	}

	while (true) {
		// Get all hero influences
		damageBuffLeft = 0;
		protectionLeft = 0;
		aoeDamageLeft = 0;
		paoeDamageLeft = 0;
		healingLeft = 0;
		pureMonstersLeft = 0;
		for (i = leftLost; i < leftArmySize; i++) {
			if (leftCumAoeDamageTaken >= monsterReference[leftLineup[i]].hp) { // Check for Backline Deaths
				leftLost += (leftLost == i);
			} else {
				if (skillTypeLeft[i] == nothing) {
					pureMonstersLeft++; // count for friends ability
				} else if (skillTypeLeft[i] == protect && (skillTargetLeft[i] == all || skillTargetLeft[i] == monsterReference[leftLineup[leftLost]].element)) {
					protectionLeft += skillAmountLeft[i];
				} else if (skillTypeLeft[i] == buff && (skillTargetLeft[i] == all || skillTargetLeft[i] == monsterReference[leftLineup[leftLost]].element)) {
					damageBuffLeft += skillAmountLeft[i];
				} else if (skillTypeLeft[i] == champion && (skillTargetLeft[i] == all || skillTargetLeft[i] == monsterReference[leftLineup[leftLost]].element)) {
					damageBuffLeft += skillAmountLeft[i];
					protectionLeft += skillAmountLeft[i];
				} else if (skillTypeLeft[i] == heal) {
					healingLeft += skillAmountLeft[i];
				} else if (skillTypeLeft[i] == aoe) {
					aoeDamageLeft += skillAmountLeft[i];
				} else if (skillTypeLeft[i] == pAoe && i == leftLost) {
					paoeDamageLeft += monsterReference[leftLineup[i]].damage;
				}
			}
		}

		damageBuffRight = 0;
		protectionRight = 0;
		aoeDamageRight = 0;
		paoeDamageRight = 0;
		healingRight = 0;
		pureMonstersRight = 0;
		for (i = rightLost; i < rightArmySize; i++) {
			if (rightCumAoeDamageTaken >= monsterReference[rightLineup[i]].hp) { // Check for Backline Deaths
				rightLost += (i == rightLost);
			} else {
				if (skillTypeRight[i] == nothing) {
					pureMonstersRight++;  // count for friends ability
				} else if (skillTypeRight[i] == protect && (skillTargetRight[i] == all || skillTargetRight[i] == monsterReference[rightLineup[rightLost]].element)) {
					protectionRight += skillAmountRight[i];
				} else if (skillTypeRight[i] == buff && (skillTargetRight[i] == all || skillTargetRight[i] == monsterReference[rightLineup[rightLost]].element)) {
					damageBuffRight += skillAmountRight[i];
				} else if (skillTypeRight[i] == champion && (skillTargetRight[i] == all || skillTargetRight[i] == monsterReference[rightLineup[rightLost]].element)) {
					damageBuffRight += skillAmountRight[i];
					protectionRight += skillAmountRight[i];
				} else if (skillTypeRight[i] == heal) {
					healingRight += skillAmountRight[i];
				} else if (skillTypeRight[i] == aoe) {
					aoeDamageRight += skillAmountRight[i];
				} else if (skillTypeRight[i] == pAoe && i == rightLost) {
					paoeDamageRight += monsterReference[rightLineup[i]].damage;
				}
			}
		}

		// Heal everything that hasnt died
		leftFrontDamageTaken -= leftHealing; // these values are from the last iteration
		leftCumAoeDamageTaken -= leftHealing;
		rightFrontDamageTaken -= rightHealing;
		rightCumAoeDamageTaken -= rightHealing;
		if (leftFrontDamageTaken < 0) {
			leftFrontDamageTaken = 0;
		}
		if (leftCumAoeDamageTaken < 0) {
			leftCumAoeDamageTaken = 0;
		}
		if (rightFrontDamageTaken < 0) {
			rightFrontDamageTaken = 0;
		}
		if (rightCumAoeDamageTaken < 0) {
			rightCumAoeDamageTaken = 0;
		}

		// Add last effects of abilities and start resolving the turn
		if (leftLost >= leftArmySize || rightLost >= rightArmySize) {
			break; // At least One army was beaten
		}

		// Get Base Damage for this Turn
		currentMonsterLeft = &monsterReference[leftLineup[leftLost]];
		currentMonsterRight = &monsterReference[rightLineup[rightLost]];
		damageLeft = currentMonsterLeft->damage;
		damageRight = currentMonsterRight->damage;

		// Handle Monsters with skills berserk or friends
		if (skillTypeLeft[leftLost] == friends) {
			damageLeft *= pow(skillAmountLeft[leftLost], pureMonstersLeft);
		} else if (skillTypeLeft[leftLost] == adapt && currentMonsterLeft->element == currentMonsterRight->element) {
			damageLeft *= skillAmountLeft[leftLost];
		} else if (skillTypeLeft[leftLost] == berserk) {
			damageLeft *= pow(skillAmountLeft[leftLost], leftBerserkProcs);
			leftBerserkProcs++;
		}

		if (skillTypeRight[rightLost] == friends) {
			damageRight *= pow(skillAmountRight[rightLost], pureMonstersRight);
		} else if (skillTypeRight[rightLost] == adapt && currentMonsterRight->element == currentMonsterLeft->element) {
			damageRight *= skillAmountRight[rightLost];
		} else if (skillTypeRight[rightLost] == berserk) {
			damageRight *= pow(skillAmountRight[rightLost], rightBerserkProcs);
			rightBerserkProcs++; 
		}

		// Add Buff Damage
		damageLeft += damageBuffLeft;
		damageRight += damageBuffRight;

		// Handle Elemental advantage
		elementalDifference = (currentMonsterLeft->element - currentMonsterRight->element);
		if (elementalDifference == -1 || elementalDifference == 3) {
			damageLeft *= elementalBoost;
		} else if (elementalDifference == 1 || elementalDifference == -3) {
			damageRight *= elementalBoost;
		}

		// Handle Protection
		if (damageLeft > protectionRight) {
			damageLeft -= protectionRight;
		} else {
			damageLeft = 0;
		}

		if (damageRight > protectionLeft) {
			damageRight -= protectionLeft;
		} else {
			damageRight = 0; 
		}

		// Write values into permanent Variables for the next iteration
		rightFrontDamageTaken += damageLeft + aoeDamageLeft;
		rightCumAoeDamageTaken += aoeDamageLeft + paoeDamageLeft;
		rightHealing = healingRight;
		leftFrontDamageTaken += damageRight + aoeDamageRight;
		leftCumAoeDamageTaken += aoeDamageRight + paoeDamageRight;
		leftHealing = healingLeft;

		// Check if the first Monster died (otherwise it will be revived next turn)
		if (currentMonsterLeft->hp <= leftFrontDamageTaken) {
			leftLost++;
			leftBerserkProcs = 0;
			leftFrontDamageTaken = leftCumAoeDamageTaken;
		}
		if (currentMonsterRight->hp <= rightFrontDamageTaken) {
			rightLost++;
			rightBerserkProcs = 0;
			rightFrontDamageTaken = rightCumAoeDamageTaken;
		}

		// Output detailed fight Data for debugging
		if (verbose) {
			cout << setw(3) << leftLost << " " << setw(3) << leftFrontDamageTaken << " " << setw(3) << rightLost << " " << setw(3) << rightFrontDamageTaken << endl;
		}
	}

	// write all the results into a FightResult
	left.lastFightData.dominated = false;
	left.lastFightData.leftAoeDamage = leftCumAoeDamageTaken;
	left.lastFightData.rightAoeDamage = rightCumAoeDamageTaken;

	if (leftLost >= leftArmySize) { //draws count as right wins. 
		left.lastFightData.rightWon = true;
		left.lastFightData.monstersLost = rightLost; 
		left.lastFightData.damage = rightFrontDamageTaken;
		left.lastFightData.berserk = rightBerserkProcs;
	} else {
		left.lastFightData.rightWon = false;
		left.lastFightData.monstersLost = leftLost; 
		left.lastFightData.damage = leftFrontDamageTaken;
		left.lastFightData.berserk = leftBerserkProcs;
	}
}