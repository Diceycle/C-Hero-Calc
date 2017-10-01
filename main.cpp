// DavidL1450 - solves PvE instances in Cosmos Quest - now with heroes
// Brainisdead variant, added names of new heroes and input info comment
// input now needs to be separated by commas. To enter heroes, do <name>:<level>, for example:
// a2,e3,lady of twilight:1,e4
// Clean-up and optimization done by Diceycle

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <limits>

#include "inputProcessing.h"
#include "cosmosDefines.h"
#include "battleLogic.h"

using namespace std;

// Define global variables used to track the best result
size_t firstDominance;
int minimumMonsterCost;
size_t maxMonstersAllowed;
Army targetArmy;
size_t targetArmySize;

int followerUpperBound;
bool customFollowers;
Army best;

// Simulates fights with all armies against the target. armies will contain armies with the results written in.
void simulateMultipleFights(vector<Army> & armies) {
	bool newFound = false;

	for (size_t i = 0; i < armies.size(); i++) {
		simulateFight(armies[i], targetArmy);
		if (!armies[i].lastFightData.rightWon) {  // left (our side) wins:
			if (armies[i].followerCost < followerUpperBound) {
				if (!newFound) {
					cout << endl;
				}
				newFound = true;
				followerUpperBound = armies[i].followerCost;
				best = armies[i];
				debugOutput(time(NULL), "    " + best.toString(), true, false, true);
			}
		}
	}
	debugOutput(time(NULL), " ", newFound, false, false);
}

void expand(vector<Army> & newPureArmies, vector<Army> & newHeroArmies, 
			vector<Army> & oldPureArmies, vector<Army> & oldHeroArmies, 
			const vector<int8_t> & availableMonsters,
			size_t currentArmySize) {

	int remainingFollowers;
	size_t availableMonstersSize = availableMonsters.size();
	size_t availableHeroesSize = availableHeroes.size();
	vector<bool> usedHeroes; usedHeroes.resize(availableHeroesSize, false);
	size_t i, j, m;
	SkillType currentSkill;
	bool friendsActive;

	for (i = 0; i < oldPureArmies.size(); i++) {
		if (!oldPureArmies[i].lastFightData.dominated) {
			remainingFollowers = followerUpperBound - oldPureArmies[i].followerCost;
			for (m = 0; m < availableMonstersSize && monsterReference[availableMonsters[m]].cost < remainingFollowers; m++) {
				newPureArmies.push_back(oldPureArmies[i]);
				newPureArmies.back().add(availableMonsters[m]);
				newPureArmies.back().lastFightData.valid = true;
			}
			for (m = 0; m < availableHeroesSize; m++) {
				currentSkill = monsterReference[availableHeroes[m]].skill.type;
				newHeroArmies.push_back(oldPureArmies[i]);
				newHeroArmies.back().add(availableHeroes[m]);
				newHeroArmies.back().lastFightData.valid = (currentSkill == pAoe || currentSkill == friends || currentSkill == berserk); // These skills are self centered
			}
		}
	}

	for (i = 0; i < oldHeroArmies.size(); i++) {
		if (!oldHeroArmies[i].lastFightData.dominated) {
			friendsActive = false;
			remainingFollowers = followerUpperBound - oldHeroArmies[i].followerCost;
			for (j = 0; j < currentArmySize; j++) {
				for (m = 0; m < availableHeroesSize; m++) {
					if (oldHeroArmies[i].monsters[j] == availableHeroes[m]) {
						friendsActive |= monsterReference[oldHeroArmies[i].monsters[j]].skill.type == friends;
						usedHeroes[m] = true;
						break;
					}
				}
			}
			for (m = 0; m < availableMonstersSize && monsterReference[availableMonsters[m]].cost < remainingFollowers; m++) {
				newHeroArmies.push_back(oldHeroArmies[i]);
				newHeroArmies.back().add(availableMonsters[m]);
				newHeroArmies.back().lastFightData.valid = !friendsActive;
			}
			for (m = 0; m < availableHeroesSize; m++) {
				if (!usedHeroes[m]) {
					currentSkill = monsterReference[availableHeroes[m]].skill.type;
					newHeroArmies.push_back(oldHeroArmies[i]);
					newHeroArmies.back().add(availableHeroes[m]);
					newHeroArmies.back().lastFightData.valid = (currentSkill == pAoe || currentSkill == friends || currentSkill == berserk || currentSkill == adapt); // These skills are self centered
				}
				usedHeroes[m] = false;
			}
		}
	}
}


// Use a greedy method to get a first upper bound on follower cost for the solution
// TODO: Think of an algorithm that works on limit < targetsize semi-reliable
void getQuickSolutions(Army target, size_t limit) {
	Army tempArmy = Army();
	vector<int8_t> greedy {};
	vector<int8_t> greedyHeroes {};
	vector<int8_t> greedyTemp {};
	size_t targetSize = target.monsterAmount;
	bool invalid = false;

	cout << "Trying to find solutions greedily..." << endl;

	// Create Army that kills as many monsters as the army is big
	if (targetSize <= limit) {
		for (size_t i = 0; i < limit; i++) {
			for (size_t m = 0; m < monsterList.size(); m++) {
				tempArmy = Army(greedy);
				tempArmy.add(monsterList[m]);
				simulateFight(tempArmy, target);
				if (!tempArmy.lastFightData.rightWon || (tempArmy.lastFightData.monstersLost > (int) i && i+1 < limit)) { // the last monster has to win the encounter
					greedy.push_back(monsterList[m]);
					break;
				}
			}
			invalid = greedy.size() < limit;
		}
		if (!invalid) {
			cout << "  ";
			tempArmy.print();
			best = tempArmy;
			if (followerUpperBound > tempArmy.followerCost) {
				followerUpperBound = tempArmy.followerCost;
			}

			// Try to replace monsters in the setup with heroes to save followers
			greedyHeroes = greedy;
			for (size_t m = 0; m < availableHeroes.size(); m++) {
				for (size_t i = 0; i < greedyHeroes.size(); i++) {
					greedyTemp = greedyHeroes;
					greedyTemp[i] = availableHeroes[m];
					tempArmy = Army(greedyTemp);
					simulateFight(tempArmy, target);
					if (!tempArmy.lastFightData.rightWon) { // Setup still needs to win
						greedyHeroes = greedyTemp;
						break;
					}
				}
			}
			cout << "  ";
			tempArmy = Army(greedyHeroes);
			tempArmy.print();
			best = tempArmy;
			if (followerUpperBound > tempArmy.followerCost) {
				followerUpperBound = tempArmy.followerCost;
			}
		} else {
			cout << "  Could not find valid solution while being greedy" << endl;
		}
	} else {
		cout << "  Could not find valid solution while being greedy" << endl;
	}
}

int solveInstance(bool debugInfo) {
	Army tempArmy = Army();
	int startTime;
	int tempTime;

	size_t i, j, sj, si;

	// Get first Upper limit on followers
	if (maxMonstersAllowed > 4) {
		getQuickSolutions(targetArmy, maxMonstersAllowed);
		if (!askYesNoQuestion("Continue calculation?", "  Continuing will most likely result in a cheaper solution but could consume a lot of RAM.\n")) {return 0;}
		cout << endl;
	}

	vector<Army> pureMonsterArmies {}; // initialize with all monsters
	vector<Army> heroMonsterArmies {}; // initialize with all heroes
	for (i = 0; i < monsterList.size(); i++) {
		if (monsterReference[monsterList[i]].cost <= followerUpperBound) {
			pureMonsterArmies.push_back(Army( {monsterList[i]} ));
		}
	}
	for (i = 0; i < availableHeroes.size(); i++) { // Ignore chacking for Hero Cost
		heroMonsterArmies.push_back(Army( {availableHeroes[i]} ));
	}

	// Check if a single monster can beat the last two monsters of the target. If not, solutions that can only beat n-2 monsters need not be expanded later
	bool optimizable = (targetArmySize >= 3);
	if (optimizable) {
		tempArmy = Army({targetArmy.monsters[targetArmySize - 2], targetArmy.monsters[targetArmySize - 1]}); // Make an army from the last two monsters
	}

	if (optimizable) { // Check with normal Mobs
		for (i = 0; i < pureMonsterArmies.size(); i++) {
			simulateFight(pureMonsterArmies[i], tempArmy);
			if (!pureMonsterArmies[i].lastFightData.rightWon) { // Monster won the fight
				optimizable = false;
				break;
			}
		}
	}

	if (optimizable) { // Check with Heroes
		for (i = 0; i < heroMonsterArmies.size(); i++) {
			simulateFight(heroMonsterArmies[i], tempArmy);
			if (!heroMonsterArmies[i].lastFightData.rightWon) { // Hero won the fight
				optimizable = false;
				break;
			}
		}
	}

	// Run the Bruteforce Loop
	startTime = time(NULL);
	tempTime = startTime;
	size_t pureMonsterArmiesSize, heroMonsterArmiesSize;
	for (size_t armySize = 1; armySize <= maxMonstersAllowed; armySize++) {

		pureMonsterArmiesSize = pureMonsterArmies.size();
		heroMonsterArmiesSize = heroMonsterArmies.size();
		// Output Debug Information
		debugOutput(tempTime, "Starting loop for armies of size " + to_string(armySize), true, false, true);

		// Run Fights for non-Hero setups
		debugOutput(tempTime, "  Simulating " + to_string(pureMonsterArmiesSize) + " non-hero Fights... ", debugInfo, false, false);
		tempTime = time(NULL);
		simulateMultipleFights(pureMonsterArmies);

		// Run fights for setups with heroes
		debugOutput(tempTime, "  Simulating " + to_string(heroMonsterArmiesSize) + " hero Fights... ", debugInfo, true, false);
		tempTime = time(NULL);
		simulateMultipleFights(heroMonsterArmies);

		if (armySize < maxMonstersAllowed) { 
			// Sort the results by follower cost for some optimization
			debugOutput(tempTime, "  Sorting List... ", debugInfo, true, false);
			tempTime = time(NULL);
			sort(pureMonsterArmies.begin(), pureMonsterArmies.end(), hasFewerFollowers);
			sort(heroMonsterArmies.begin(), heroMonsterArmies.end(), hasFewerFollowers);

			if (firstDominance <= armySize) {
				// Calculate which results are strictly better than others (dominance)
				debugOutput(tempTime, "  Calculating Dominance for non-heroes... ", debugInfo, true, false);
				tempTime = time(NULL);

				int leftFollowerCost;
				FightResult * currentFightResult;
				int8_t leftHeroList[6];
				size_t leftHeroListSize;
				int8_t rightMonster;
				int8_t leftMonster;
				// First Check dominance for non-Hero setups
				for (i = 0; i < pureMonsterArmiesSize; i++) {
					leftFollowerCost = pureMonsterArmies[i].followerCost;
					currentFightResult = &pureMonsterArmies[i].lastFightData;
					// A result is obsolete if only one expansion is left but no single mob can beat the last two enemy mobs alone (optimizable)
					if (armySize == (maxMonstersAllowed - 1) && optimizable) {
						// TODO: Investigate whether this is truly correct: What if the second-to-last mob is already damaged (not from aoe) i.e. it defeated the last mob of left?
						if (currentFightResult->rightWon && currentFightResult->monstersLost < (int) (targetArmySize - 2) && currentFightResult->rightAoeDamage == 0) {
							currentFightResult->dominated = true;
						}
					}
					// A result is dominated If:
					if (!currentFightResult->dominated) { 
						// Another pureResults got farther with a less costly lineup
						for (j = i+1; j < pureMonsterArmiesSize; j++) {
							if (leftFollowerCost < pureMonsterArmies[j].followerCost) {
								break; 
							} else if (*currentFightResult <= pureMonsterArmies[j].lastFightData) { // pureResults[i] has more followers implicitly 
								currentFightResult->dominated = true;
								break;
							}
						}
						// A lineup without heroes is better than a setup with heroes even if it got just as far
						for (j = 0; j < heroMonsterArmiesSize; j++) {
							if (leftFollowerCost > heroMonsterArmies[j].followerCost) {
								break; 
							} else if (*currentFightResult >= heroMonsterArmies[j].lastFightData) { // pureResults[i] has less followers implicitly
								heroMonsterArmies[j].lastFightData.dominated = true;
							}
						}
					}
				}

				debugOutput(tempTime, "  Calculating Dominance for heroes... ", debugInfo, true, false);
				tempTime = time(NULL);
				// Domination for setups with heroes
				bool usedHeroSubset, leftUsedHero;
				for (i = 0; i < heroMonsterArmiesSize; i++) {
					leftFollowerCost = heroMonsterArmies[i].followerCost;
					currentFightResult = &heroMonsterArmies[i].lastFightData;
					leftHeroListSize = 0;
					for (si = 0; si < armySize; si++) {
						leftMonster = heroMonsterArmies[i].monsters[si];
						if (monsterReference[leftMonster].isHero) {
							leftHeroList[leftHeroListSize] = leftMonster;
							leftHeroListSize++;
						}
					}

					// A result is obsolete if only one expansion is left but no single mob can beat the last two enemy mobs alone (optimizable)
					if (armySize == (maxMonstersAllowed - 1) && optimizable && currentFightResult->rightAoeDamage == 0) {
						// TODO: Investigate whether this is truly correct: What if the second-to-last mob is already damaged (not from aoe) i.e. it defeated the last mob of left?
						if (currentFightResult->rightWon && currentFightResult->monstersLost < (int) (targetArmySize - 2)){
							currentFightResult->dominated = true;
						}
					}

					// A result is dominated If:
					if (!currentFightResult->dominated) {
						// if i costs more followers and got less far than j, then i is dominated
						for (j = i+1; j < heroMonsterArmiesSize; j++) {
							if (leftFollowerCost < heroMonsterArmies[j].followerCost) {
								break;
							} else if (*currentFightResult <= heroMonsterArmies[j].lastFightData) { // i has more followers implicitly
								usedHeroSubset = true; // If j doesn't use a strict subset of the heroes i used, it cannot dominate i
								for (sj = 0; sj < armySize; sj++) { // for every hero in j there must be the same hero in i
									leftUsedHero = false; 
									rightMonster = heroMonsterArmies[j].monsters[sj];
									if (monsterReference[rightMonster].isHero) { // mob is a hero
										for (si = 0; si < leftHeroListSize; si++) {
											if (leftHeroList[si] == rightMonster) {
												leftUsedHero = true;
												break;
											}
										}
										if (!leftUsedHero) {
											usedHeroSubset = false;
											break;
										}
									}
								}
								if (usedHeroSubset) {
									currentFightResult->dominated = true;
									break;
								}
							}
						}
					}
				}
			}
			// now we expand to add the next monster to all non-dominated armies
			debugOutput(tempTime, "  Expanding Lineups by one... ", debugInfo, true, false);
			tempTime = time(NULL);
			
			vector<Army> nextPureArmies;
			vector<Army> nextHeroArmies;
			expand(nextPureArmies, nextHeroArmies, pureMonsterArmies, heroMonsterArmies, monsterList, armySize);

			debugOutput(tempTime, "  Moving Data... ", debugInfo, true, false);
			tempTime = time(NULL);
			
			pureMonsterArmies = move(nextPureArmies);
			heroMonsterArmies = move(nextHeroArmies);
		}
		debugOutput(tempTime, "", true, true, true);
	}
	return time(NULL) - startTime;
}

int main(int argc, char** argv) {

	if (argc == 2) {
		initConfigFile(argv[1]);
		useConfigFile = true;
	} else {
		useConfigFile = false;
	}

	// Declare Variables
	vector<string> stringLineup {};
	string inputString;
	vector<int> yourHeroLevels;

	// Additional convenience Strings
	vector<string> daily {"w14", "a14", "f13", "a13", "w13"};
	vector<string> test {"a9", "e13", "nicte:25", "a5"};

	// Define User Input Data
	firstDominance = 4;             // Set this to control at which army length dominance should first be calculated. Treat with extreme caution. Not using dominance at all WILL use more RAM than you have
	maxMonstersAllowed = 6;         // Set this to how many Monsters should be in the solution (f.e 4 for X-3 Quests) 
	minimumMonsterCost = 0;    // Minimum amount a monster used in the soluiton should cost. Useful for reducing the amount of monsters when you are sure you wont need them (f.e. a1 in dq20)
	followerUpperBound = -1;        // Maximum Cost of the whole Lineup, -1 means unlimited
	stringLineup = quests[1];      // Choose against which lineup you want to fight use one from above or make your own and then change the name accordingly
	yourHeroLevels = {    // INPUT YOUR HERO LEVELS HERE (For manual editing: Names tell you which number is the level of which hero)
		 0,               // "james"
		 0, 0, 0,         // "hunter","shaman","alpha"
		 0, 0, 0,         // "carl","nimue","athos"
		 0, 0, 0,         // "jet","geron","rei"
		 0, 0, 0,         // "ailen","faefyr","auri"
		 0, 0, 0,         // "k41ry", "t4urus", "tr0n1x"
		 0, 0, 0,         // "aquortis", "aeris", "geum"
		 0, 0, 0,         // "rudean","aural","geror"
		 0, 0, 0,         // "ourea", "erebus", "pontus"
		 0, 0, 0,         // "ladyoftwilight","tiny","nebra"
		 0, 0, 0,         // "veildur", "brynhildr", "groth"
		 0, 0, 0, 0,      // "valor","rokka","pyromancer","bewat"
		 0, 0, 0, 0,      // "nicte", "forestdruid","ignitor","undine"
		 0, 0, 0          // "chroma", "petry", "zaytus"
	};

	// Flow Control Variables
	bool debugInfo = true;                              // Set this to true if you want to see how far the execution is and how lone the execution took altogether
	bool manualInput = true;                           // Set this to true if you want nothing to do with this file and just want to input stuff over the command line like you're used to

	int totalTime;

	// --------------------------------------------- Actual Program Starts here --------------------------------------------

	cout << welcomeMessage << endl;
	cout << helpMessage << endl;

	bool userWantsContinue = true;
	while (userWantsContinue) {
		// Initialize global Data
		best = Army();
		initMonsterData();

		// Collect the Data via Command Line if the user wants
		yourHeroLevels = takeHerolevelInput();
		targetArmy = takeLineupInput("Enter Enemy Lineup: ");
		targetArmySize = targetArmy.monsterAmount;
		maxMonstersAllowed = stoi(getResistantInput("Enter how many monsters are allowed in the solution: ", maxMonstersAllowedHelp, integer));
		followerUpperBound = -1;

		// Set Upper Bound Correctly
		if (followerUpperBound < 0) {
			followerUpperBound = numeric_limits<int>::max();
			customFollowers = false;
		} else {
			customFollowers = true;
		}

		filterMonsterData(minimumMonsterCost);
		initializeUserHeroes(yourHeroLevels);

		totalTime = solveInstance(debugInfo);
		// Last check to see if winning combination wins:
		if ((customFollowers && best.monsterAmount > 0) || (!customFollowers && followerUpperBound < numeric_limits<int>::max())) {
			best.lastFightData.valid = false;
			simulateFight(best, targetArmy);
			if (best.lastFightData.rightWon) {
				best.print();
				cout << "This does not beat the lineup!!!" << endl;
				for (int i = 1; i <= 10; i++) {
					cout << "ERROR";
				}
				haltExecution();
				return EXIT_FAILURE;

			} else {
				// Print the winning combination!
				cout << endl << "The optimal combination is:" << endl << "  ";
				best.print();
				cout << "  (Right-most fights first)" << endl;
			}
		} else {
			cout << endl << "Could not find a solution that beats this lineup." << endl;
		}

		cout << endl;
		cout << totalFightsSimulated << " Fights simulated." << endl;
		cout << "Total Calculation Time: " << totalTime << endl;
		userWantsContinue = askYesNoQuestion("Do you want to calculate another lineup?", "");
	}
	return EXIT_SUCCESS;
}