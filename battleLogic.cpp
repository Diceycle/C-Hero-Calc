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

FightData::FightData(Army &army) {
	lost = 0;
	armySize = army.monsterAmount;
	lineup = army.monsters;

	frontDamageTaken = 0;
	healing = 0;
	cumAoeDamageTaken = 0;
	berserkProcs = 0;
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

	FightData leftData(left);
	FightData rightData(right);

	// If no heroes are in the army the result from the smaller army is still valid
	if (left.lastFightData.valid && !verbose) { 
		// Set pre-computed values to pick up where we left off
		leftData.lost           = leftData.armySize - 1; // All monsters of left died last fight only the new one counts
		leftData.frontDamageTaken    = left.lastFightData.leftAoeDamage;
		leftData.cumAoeDamageTaken   = left.lastFightData.leftAoeDamage;
		rightData.lost               = left.lastFightData.monstersLost;
		rightData.frontDamageTaken   = left.lastFightData.damage;
		rightData.cumAoeDamageTaken  = left.lastFightData.rightAoeDamage;
		rightData.berserkProcs       = left.lastFightData.berserk;
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
			if (leftData.cumAoeDamageTaken >= monsterReference[leftData.lineup[i]].hp) { // Check for Backline Deaths
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
			if (rightData.cumAoeDamageTaken >= monsterReference[rightData.lineup[i]].hp) { // Check for Backline Deaths
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
		leftData.frontDamageTaken -= leftData.healing; // these values are from the last iteration
		leftData.cumAoeDamageTaken -= leftData.healing;
		rightData.frontDamageTaken -= rightData.healing;
		rightData.cumAoeDamageTaken -= rightData.healing;
		if (leftData.frontDamageTaken < 0) {
			leftData.frontDamageTaken = 0;
		}
		if (leftData.cumAoeDamageTaken < 0) {
			leftData.cumAoeDamageTaken = 0;
		}
		if (rightData.frontDamageTaken < 0) {
			rightData.frontDamageTaken = 0;
		}
		if (rightData.cumAoeDamageTaken < 0) {
			rightData.cumAoeDamageTaken = 0;
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
			damageLeft *= pow(skillAmountLeft[leftData.lost], leftData.berserkProcs);
			leftData.berserkProcs++;
		}

		if (skillTypeRight[rightData.lost] == friends) {
			damageRight *= pow(skillAmountRight[rightData.lost], pureMonstersRight);
		} else if (skillTypeRight[rightData.lost] == adapt && currentMonsterRight->element == currentMonsterLeft->element) {
			damageRight *= skillAmountRight[rightData.lost];
		} else if (skillTypeRight[rightData.lost] == berserk) {
			damageRight *= pow(skillAmountRight[rightData.lost], rightData.berserkProcs);
			rightData.berserkProcs++; 
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
		rightData.frontDamageTaken += damageLeft + aoeDamageLeft;
		rightData.cumAoeDamageTaken += aoeDamageLeft + paoeDamageLeft;
		rightData.healing = healingRight;
		leftData.frontDamageTaken += damageRight + aoeDamageRight;
		leftData.cumAoeDamageTaken += aoeDamageRight + paoeDamageRight;
		leftData.healing = healingLeft;

		// Check if the first Monster died (otherwise it will be revived next turn)
		if (currentMonsterLeft->hp <= leftData.frontDamageTaken) {
			leftData.lost++;
			leftData.berserkProcs = 0;
			leftData.frontDamageTaken = leftData.cumAoeDamageTaken;
		}
		if (currentMonsterRight->hp <= rightData.frontDamageTaken) {
			rightData.lost++;
			rightData.berserkProcs = 0;
			rightData.frontDamageTaken = rightData.cumAoeDamageTaken;
		}

		// Output detailed fight Data for debugging
		if (verbose) {
			cout << setw(3) << leftData.lost << " " << setw(3) << leftData.frontDamageTaken << " " << setw(3) << rightData.lost << " " << setw(3) << rightData.frontDamageTaken << endl;
		}
	}

	// write all the results into a FightResult
	left.lastFightData.dominated = false;
	left.lastFightData.leftAoeDamage = leftData.cumAoeDamageTaken;
	left.lastFightData.rightAoeDamage = rightData.cumAoeDamageTaken;

	if (leftData.lost >= leftData.armySize) { //draws count as right wins. 
		left.lastFightData.rightWon = true;
		left.lastFightData.monstersLost = rightData.lost; 
		left.lastFightData.damage = rightData.frontDamageTaken;
		left.lastFightData.berserk = rightData.berserkProcs;
	} else {
		left.lastFightData.rightWon = false;
		left.lastFightData.monstersLost = leftData.lost; 
		left.lastFightData.damage = leftData.frontDamageTaken;
		left.lastFightData.berserk = leftData.berserkProcs;
	}
}