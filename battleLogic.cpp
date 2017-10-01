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

	damage = 0;
	damageBuff = 0;
	protection = 0;
	aoeDamage = 0;
	paoeDamage = 0;
	healingSkill = 0;
	pureMonsters = 0;
}

void FightData::LoadSkills() {
	HeroSkill * skill;

	for (int i = lost; i < armySize; i++) {
		skill = &monsterReference[lineup[i]].skill;
		skillType[i] = skill->type;
		skillTarget[i] = skill->target;
		skillAmount[i] = skill->amount;
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
	int elementalDifference;

	FightData leftData(left);
	FightData rightData(right);

	// If no heroes are in the army the result from the smaller army is still valid
	if (left.lastFightData.valid && !verbose) { 
		// Set pre-computed values to pick up where we left off
		leftData.lost                = leftData.armySize - 1; // All monsters of left died last fight only the new one counts
		leftData.frontDamageTaken    = left.lastFightData.leftAoeDamage;
		leftData.cumAoeDamageTaken   = left.lastFightData.leftAoeDamage;
		rightData.lost               = left.lastFightData.monstersLost;
		rightData.frontDamageTaken   = left.lastFightData.damage;
		rightData.cumAoeDamageTaken  = left.lastFightData.rightAoeDamage;
		rightData.berserkProcs       = left.lastFightData.berserk;
	}

	leftData.LoadSkills();
	rightData.LoadSkills();

	while (true) {
		// Get all hero influences
		leftData.damageBuff = 0;
		leftData.protection = 0;
		leftData.aoeDamage = 0;
		leftData.paoeDamage = 0;
		leftData.healingSkill = 0;
		leftData.pureMonsters = 0;
		for (i = leftData.lost; i < leftData.armySize; i++) {
			if (leftData.cumAoeDamageTaken >= monsterReference[leftData.lineup[i]].hp) { // Check for Backline Deaths
				leftData.lost += (leftData.lost == i);
			} else {
				if (leftData.skillType[i] == nothing) {
					leftData.pureMonsters++; // count for friends ability
				} else if (leftData.skillType[i] == protect && (leftData.skillTarget[i] == all || leftData.skillTarget[i] == monsterReference[leftData.lineup[leftData.lost]].element)) {
					leftData.protection += leftData.skillAmount[i];
				} else if (leftData.skillType[i] == buff && (leftData.skillTarget[i] == all || leftData.skillTarget[i] == monsterReference[leftData.lineup[leftData.lost]].element)) {
					leftData.damageBuff += leftData.skillAmount[i];
				} else if (leftData.skillType[i] == champion && (leftData.skillTarget[i] == all || leftData.skillTarget[i] == monsterReference[leftData.lineup[leftData.lost]].element)) {
					leftData.damageBuff += leftData.skillAmount[i];
					leftData.protection += leftData.skillAmount[i];
				} else if (leftData.skillType[i] == heal) {
					leftData.healingSkill += leftData.skillAmount[i];
				} else if (leftData.skillType[i] == aoe) {
					leftData.aoeDamage += leftData.skillAmount[i];
				} else if (leftData.skillType[i] == pAoe && i == leftData.lost) {
					leftData.paoeDamage += monsterReference[leftData.lineup[i]].damage;
				}
			}
		}

		rightData.damageBuff = 0;
		rightData.protection = 0;
		rightData.aoeDamage = 0;
		rightData.paoeDamage = 0;
		rightData.healingSkill = 0;
		rightData.pureMonsters = 0;
		for (i = rightData.lost; i < rightData.armySize; i++) {
			if (rightData.cumAoeDamageTaken >= monsterReference[rightData.lineup[i]].hp) { // Check for Backline Deaths
				rightData.lost += (i == rightData.lost);
			} else {
				if (rightData.skillType[i] == nothing) {
					rightData.pureMonsters++;  // count for friends ability
				} else if (rightData.skillType[i] == protect && (rightData.skillTarget[i] == all || rightData.skillTarget[i] == monsterReference[rightData.lineup[rightData.lost]].element)) {
					rightData.protection += rightData.skillAmount[i];
				} else if (rightData.skillType[i] == buff && (rightData.skillTarget[i] == all || rightData.skillTarget[i] == monsterReference[rightData.lineup[rightData.lost]].element)) {
					rightData.damageBuff += rightData.skillAmount[i];
				} else if (rightData.skillType[i] == champion && (rightData.skillTarget[i] == all || rightData.skillTarget[i] == monsterReference[rightData.lineup[rightData.lost]].element)) {
					rightData.damageBuff += rightData.skillAmount[i];
					rightData.protection += rightData.skillAmount[i];
				} else if (rightData.skillType[i] == heal) {
					rightData.healingSkill += rightData.skillAmount[i];
				} else if (rightData.skillType[i] == aoe) {
					rightData.aoeDamage += rightData.skillAmount[i];
				} else if (rightData.skillType[i] == pAoe && i == rightData.lost) {
					rightData.paoeDamage += monsterReference[rightData.lineup[i]].damage;
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
		leftData.currentMonster = &monsterReference[leftData.lineup[leftData.lost]];
		rightData.currentMonster = &monsterReference[rightData.lineup[rightData.lost]];
		leftData.damage = leftData.currentMonster->damage;
		rightData.damage = rightData.currentMonster->damage;

		// Handle Monsters with skills berserk or friends
		if (leftData.skillType[leftData.lost] == friends) {
			leftData.damage *= pow(leftData.skillAmount[leftData.lost], leftData.pureMonsters);
		} else if (leftData.skillType[leftData.lost] == adapt && leftData.currentMonster->element == rightData.currentMonster->element) {
			leftData.damage *= leftData.skillAmount[leftData.lost];
		} else if (leftData.skillType[leftData.lost] == berserk) {
			leftData.damage *= pow(leftData.skillAmount[leftData.lost], leftData.berserkProcs);
			leftData.berserkProcs++;
		}

		if (rightData.skillType[rightData.lost] == friends) {
			rightData.damage *= pow(rightData.skillAmount[rightData.lost], rightData.pureMonsters);
		} else if (rightData.skillType[rightData.lost] == adapt && rightData.currentMonster->element == leftData.currentMonster->element) {
			rightData.damage *= rightData.skillAmount[rightData.lost];
		} else if (rightData.skillType[rightData.lost] == berserk) {
			rightData.damage *= pow(rightData.skillAmount[rightData.lost], rightData.berserkProcs);
			rightData.berserkProcs++; 
		}

		// Add Buff Damage
		leftData.damage += leftData.damageBuff;
		rightData.damage += rightData.damageBuff;

		// Handle Elemental advantage
		elementalDifference = (leftData.currentMonster->element - rightData.currentMonster->element);
		if (elementalDifference == -1 || elementalDifference == 3) {
			leftData.damage *= elementalBoost;
		} else if (elementalDifference == 1 || elementalDifference == -3) {
			rightData.damage *= elementalBoost;
		}

		// Handle Protection
		if (leftData.damage > rightData.protection) {
			leftData.damage -= rightData.protection;
		} else {
			leftData.damage = 0;
		}

		if (rightData.damage > leftData.protection) {
			rightData.damage -= leftData.protection;
		} else {
			rightData.damage = 0; 
		}

		// Write values into permanent Variables for the next iteration
		rightData.frontDamageTaken += leftData.damage + leftData.aoeDamage;
		rightData.cumAoeDamageTaken += leftData.aoeDamage + leftData.paoeDamage;
		rightData.healing = rightData.healingSkill;
		leftData.frontDamageTaken += rightData.damage + rightData.aoeDamage;
		leftData.cumAoeDamageTaken += rightData.aoeDamage + rightData.paoeDamage;
		leftData.healing = leftData.healingSkill;

		// Check if the first Monster died (otherwise it will be revived next turn)
		if (leftData.currentMonster->hp <= leftData.frontDamageTaken) {
			leftData.lost++;
			leftData.berserkProcs = 0;
			leftData.frontDamageTaken = leftData.cumAoeDamageTaken;
		}
		if (rightData.currentMonster->hp <= rightData.frontDamageTaken) {
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