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

void FightData::LoadHeroInfluences() {
	damageBuff = 0;
	protection = 0;
	aoeDamage = 0;
	paoeDamage = 0;
	healingSkill = 0;
	pureMonsters = 0;

	for (int i = lost; i < armySize; i++) {
		if (cumAoeDamageTaken >= monsterReference[lineup[i]].hp) { // Check for Backline Deaths
			lost += (lost == i);
		} else {
			if (skillType[i] == nothing) {
				pureMonsters++; // count for friends ability
			} else if (skillType[i] == protect && (skillTarget[i] == all || skillTarget[i] == monsterReference[lineup[lost]].element)) {
				protection += skillAmount[i];
			} else if (skillType[i] == buff && (skillTarget[i] == all || skillTarget[i] == monsterReference[lineup[lost]].element)) {
				damageBuff += skillAmount[i];
			} else if (skillType[i] == champion && (skillTarget[i] == all || skillTarget[i] == monsterReference[lineup[lost]].element)) {
				damageBuff += skillAmount[i];
				protection += skillAmount[i];
			} else if (skillType[i] == heal) {
				healingSkill += skillAmount[i];
			} else if (skillType[i] == aoe) {
				aoeDamage += skillAmount[i];
			} else if (skillType[i] == pAoe && i == lost) {
				paoeDamage += monsterReference[lineup[i]].damage;
			}
		}
	}
}

void FightData::Heal() {
	// these values are from the last iteration
	frontDamageTaken -= healing;
	cumAoeDamageTaken -= healing;

	if (frontDamageTaken < 0) {
		frontDamageTaken = 0;
	}
	if (cumAoeDamageTaken < 0) {
		cumAoeDamageTaken = 0;
	}
}

void FightData::CalcDamage() {
	// Get Base Damage for this Turn
	currentMonster = &monsterReference[lineup[lost]];
	damage = currentMonster->damage;
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
		leftData.LoadHeroInfluences();
		rightData.LoadHeroInfluences();

		// Heal everything that hasnt died
		leftData.Heal();
		rightData.Heal();

		// Add last effects of abilities and start resolving the turn
		if (leftData.lost >= leftData.armySize || rightData.lost >= rightData.armySize) {
			break; // At least One army was beaten
		}

		leftData.CalcDamage();
		rightData.CalcDamage();

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