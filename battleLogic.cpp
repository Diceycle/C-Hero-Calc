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

void InitFightData(FightData &fightData, Army &army) {
	fightData.lost = 0;
	fightData.armySize = army.monsterAmount;
	fightData.lineup = army.monsters;
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

	FightData leftData;
	FightData rightData;

	InitFightData(leftData, left);
	InitFightData(rightData, right);

	int leftFrontDamageTaken = 0;
	int leftHealing = 0;
	int leftCumAoeDamageTaken = 0;
	float leftBerserkProcs = 0;

	int rightFrontDamageTaken = 0;
	int rightHealing = 0;
	int rightCumAoeDamageTaken = 0;
	float rightBerserkProcs = 0;

	// If no heroes are in the army the result from the smaller army is still valid
	if (left.lastFightData.valid && !verbose) { 
		// Set pre-computed values to pick up where we left off
		leftData.lost           = leftData.armySize - 1; // All monsters of left died last fight only the new one counts
		leftFrontDamageTaken    = left.lastFightData.leftAoeDamage;
		leftCumAoeDamageTaken   = left.lastFightData.leftAoeDamage;
		rightData.lost               = left.lastFightData.monstersLost;
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

	for (i = leftData.lost; i < leftData.armySize; i++) {
		skill = &monsterReference[leftData.lineup[i]].skill;
		skillTypeLeft[i] = skill->type;
		skillTargetLeft[i] = skill->target;
		skillAmountLeft[i] = skill->amount;
	}

	for (i = rightData.lost; i < rightData.armySize; i++) {
		skill = &monsterReference[rightData.lineup[i]].skill;
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
		for (i = leftData.lost; i < leftData.armySize; i++) {
			if (leftCumAoeDamageTaken >= monsterReference[leftData.lineup[i]].hp) { // Check for Backline Deaths
				leftData.lost += (leftData.lost == i);
			} else {
				if (skillTypeLeft[i] == nothing) {
					pureMonstersLeft++; // count for friends ability
				} else if (skillTypeLeft[i] == protect && (skillTargetLeft[i] == all || skillTargetLeft[i] == monsterReference[leftData.lineup[leftData.lost]].element)) {
					protectionLeft += skillAmountLeft[i];
				} else if (skillTypeLeft[i] == buff && (skillTargetLeft[i] == all || skillTargetLeft[i] == monsterReference[leftData.lineup[leftData.lost]].element)) {
					damageBuffLeft += skillAmountLeft[i];
				} else if (skillTypeLeft[i] == champion && (skillTargetLeft[i] == all || skillTargetLeft[i] == monsterReference[leftData.lineup[leftData.lost]].element)) {
					damageBuffLeft += skillAmountLeft[i];
					protectionLeft += skillAmountLeft[i];
				} else if (skillTypeLeft[i] == heal) {
					healingLeft += skillAmountLeft[i];
				} else if (skillTypeLeft[i] == aoe) {
					aoeDamageLeft += skillAmountLeft[i];
				} else if (skillTypeLeft[i] == pAoe && i == leftData.lost) {
					paoeDamageLeft += monsterReference[leftData.lineup[i]].damage;
				}
			}
		}

		damageBuffRight = 0;
		protectionRight = 0;
		aoeDamageRight = 0;
		paoeDamageRight = 0;
		healingRight = 0;
		pureMonstersRight = 0;
		for (i = rightData.lost; i < rightData.armySize; i++) {
			if (rightCumAoeDamageTaken >= monsterReference[rightData.lineup[i]].hp) { // Check for Backline Deaths
				rightData.lost += (i == rightData.lost);
			} else {
				if (skillTypeRight[i] == nothing) {
					pureMonstersRight++;  // count for friends ability
				} else if (skillTypeRight[i] == protect && (skillTargetRight[i] == all || skillTargetRight[i] == monsterReference[rightData.lineup[rightData.lost]].element)) {
					protectionRight += skillAmountRight[i];
				} else if (skillTypeRight[i] == buff && (skillTargetRight[i] == all || skillTargetRight[i] == monsterReference[rightData.lineup[rightData.lost]].element)) {
					damageBuffRight += skillAmountRight[i];
				} else if (skillTypeRight[i] == champion && (skillTargetRight[i] == all || skillTargetRight[i] == monsterReference[rightData.lineup[rightData.lost]].element)) {
					damageBuffRight += skillAmountRight[i];
					protectionRight += skillAmountRight[i];
				} else if (skillTypeRight[i] == heal) {
					healingRight += skillAmountRight[i];
				} else if (skillTypeRight[i] == aoe) {
					aoeDamageRight += skillAmountRight[i];
				} else if (skillTypeRight[i] == pAoe && i == rightData.lost) {
					paoeDamageRight += monsterReference[rightData.lineup[i]].damage;
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
		if (leftData.lost >= leftData.armySize || rightData.lost >= rightData.armySize) {
			break; // At least One army was beaten
		}

		// Get Base Damage for this Turn
		currentMonsterLeft = &monsterReference[leftData.lineup[leftData.lost]];
		currentMonsterRight = &monsterReference[rightData.lineup[rightData.lost]];
		damageLeft = currentMonsterLeft->damage;
		damageRight = currentMonsterRight->damage;

		// Handle Monsters with skills berserk or friends
		if (skillTypeLeft[leftData.lost] == friends) {
			damageLeft *= pow(skillAmountLeft[leftData.lost], pureMonstersLeft);
		} else if (skillTypeLeft[leftData.lost] == adapt && currentMonsterLeft->element == currentMonsterRight->element) {
			damageLeft *= skillAmountLeft[leftData.lost];
		} else if (skillTypeLeft[leftData.lost] == berserk) {
			damageLeft *= pow(skillAmountLeft[leftData.lost], leftBerserkProcs);
			leftBerserkProcs++;
		}

		if (skillTypeRight[rightData.lost] == friends) {
			damageRight *= pow(skillAmountRight[rightData.lost], pureMonstersRight);
		} else if (skillTypeRight[rightData.lost] == adapt && currentMonsterRight->element == currentMonsterLeft->element) {
			damageRight *= skillAmountRight[rightData.lost];
		} else if (skillTypeRight[rightData.lost] == berserk) {
			damageRight *= pow(skillAmountRight[rightData.lost], rightBerserkProcs);
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
			leftData.lost++;
			leftBerserkProcs = 0;
			leftFrontDamageTaken = leftCumAoeDamageTaken;
		}
		if (currentMonsterRight->hp <= rightFrontDamageTaken) {
			rightData.lost++;
			rightBerserkProcs = 0;
			rightFrontDamageTaken = rightCumAoeDamageTaken;
		}

		// Output detailed fight Data for debugging
		if (verbose) {
			cout << setw(3) << leftData.lost << " " << setw(3) << leftFrontDamageTaken << " " << setw(3) << rightData.lost << " " << setw(3) << rightFrontDamageTaken << endl;
		}
	}

	// write all the results into a FightResult
	left.lastFightData.dominated = false;
	left.lastFightData.leftAoeDamage = leftCumAoeDamageTaken;
	left.lastFightData.rightAoeDamage = rightCumAoeDamageTaken;

	if (leftData.lost >= leftData.armySize) { //draws count as right wins. 
		left.lastFightData.rightWon = true;
		left.lastFightData.monstersLost = rightData.lost; 
		left.lastFightData.damage = rightFrontDamageTaken;
		left.lastFightData.berserk = rightBerserkProcs;
	} else {
		left.lastFightData.rightWon = false;
		left.lastFightData.monstersLost = leftData.lost; 
		left.lastFightData.damage = leftFrontDamageTaken;
		left.lastFightData.berserk = leftBerserkProcs;
	}
}