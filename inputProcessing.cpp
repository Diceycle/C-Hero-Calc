#include "inputProcessing.h"

bool useConfigFile;
ifstream configFile;

// Initialize a config file provided by filename
void initConfigFile(string configFileName) {
	configFile.open(configFileName);
}

// Wait for user input before continuing. Used to stop program from closing outside of a command line.
void haltExecution() {
	cout << "Press enter to continue...";
	cin.get();
}

// Method for handling ALL input. Gives access to help, error resistance and config file for input.
string getResistantInput(string query, string help, QueryType queryType) {
	string inputString;
	while (true) {
		cout << query;
		if (useConfigFile) {
			useConfigFile = (bool) getline(configFile, inputString);
		} 
		if (!useConfigFile) {
			getline(cin, inputString);
		}
		inputString = split(inputString, " ")[0];
		if (useConfigFile) {
			cout << inputString << endl;
		}
		if (inputString == "help") {
			cout << help;
		} else {
			if (queryType == question && (inputString == "y" || inputString == "n")) {
				return inputString;
			}
			if (queryType == integer) {
				try {
					stoi(inputString);
					return inputString;
				} catch (const exception & e) {}
			}
			if (queryType == raw) {
				return inputString;
			}
		}
	}
}

// Ask the user a question that they can answer via command line
bool askYesNoQuestion(string questionMessage, string help) {
	string inputString = getResistantInput(questionMessage + " (y/n): ", help, question);
	if (inputString == "n") {
		return false;
	}
	if (inputString == "y") {
		return true;
	}
	return false;
}

// Output things on the command line. Using shouldOutput this can be easily controlled globally
void debugOutput(int timeStamp, string message, bool shouldOutput, bool finishLastOutput, bool finishLine) {
	if (shouldOutput) { 
		if (finishLastOutput) {
			cout << "Done! (" << right << setw(3) << time(NULL) - timeStamp << " seconds)" << endl; // Exactly 20 bytes long
		}
		if (message != "") {
			cout << left << setw(60) << message; // With 60 there is exactly enough space to fit the finish message in on a windows cmd
			if (finishLine) {
				cout << endl;
			}
		}
	}
}


// Promt the User via command line to input his hero levels and return them as a vector<int>
vector<int> takeHerolevelInput() {
	vector<string> stringLevels;
	vector<int> levels {};
	string input;
	fstream heroFile;
	heroFile.exceptions(fstream::failbit);
	bool fileInput = true;

	if (fileInput) {
		try {
			heroFile.open(heroLevelFileName, fstream::in);
			heroFile >> input;
			stringLevels = split(input, ",");
			for (size_t i = 0; i < stringLevels.size(); i++) {
				levels.push_back(stoi(stringLevels[i]));
			}
			heroFile.close();
		} catch (const exception & e) {
			cout << heroFileNotFoundErrorMessage;
			fileInput = false;
		}
	}
	if (!fileInput) {
		cout << "Enter the level of the hero, whose name is shown." << endl;
		for (size_t i = 0; i < baseHeroes.size(); i++) {
			input = getResistantInput(baseHeroes[i].name + ": ", heroInputHelp, integer);
			levels.push_back(stoi(input));
		}

		// Write Hero Levels to file to use next time
		heroFile.open(heroLevelFileName, fstream::out);
		for (size_t i = 0; i < levels.size()-1; i++) {
			heroFile << levels[i] << ',';
		}
		heroFile << levels[levels.size()-1];
		heroFile.close();
		cout << "Hero Levels have been saved in a file. Next time you use this program you can load them from file." << endl;
	}
	return levels;
}

// Promt the user via command Line to input a monster lineup and return them as a vector of pointers to those monster
vector<int8_t> takeLineupInput(string prompt) {
	vector<int8_t> lineup {};
	string questString = "quest";

	string input;

	while (true) {
		input = getResistantInput(prompt, lineupInputHelp, raw);
		try {
			if (input.compare(0, questString.length(), questString) == 0) {
				int questNumber = stoi(input.substr(questString.length(), 2));
				lineup = makeMonstersFromStrings(quests[questNumber]);
				return lineup;
			} else {
				vector<string> stringLineup = split(input, ",");
				lineup = makeMonstersFromStrings(stringLineup);
				return lineup;
			}
		} catch (const exception & e) {}
	}
}

// Parse string linup input into actual monsters if there are heroes in the input, a leveled hero is added to the database
vector<int8_t> makeMonstersFromStrings(vector<string> stringLineup) {
	vector<int8_t> lineup {};
	pair<Monster, int> heroData;

	for(size_t i = 0; i < stringLineup.size(); i++) {
		if(stringLineup[i].find(":") != stringLineup[i].npos) {
			heroData = parseHeroString(stringLineup[i]);
			lineup.push_back(addLeveledHero(heroData.first, heroData.second));
		} else {
			lineup.push_back(monsterMap.at(stringLineup[i]));
		}
	}
	return lineup;
}

// Parse hero input from a string into its name and level
pair<Monster, int> parseHeroString(string heroString) {
	string name = heroString.substr(0, heroString.find(':'));
	Monster hero;
	for (size_t i = 0; i < baseHeroes.size(); i++) {
		if (baseHeroes[i].name == name) {
			hero = baseHeroes[i];
		}
	}
	int level = stoi(heroString.substr(heroString.find(':')+1));
	return pair<Monster, int>(hero, level);
}

// Splits strings into a vector of strings. No need to optimize, only used for input.
vector<string> split(string s, string to_split) {
	vector<string> output;
	size_t x = 0;
	size_t limit = 0;
	while(limit != s.npos){
		limit = s.find(to_split, x);
		output.push_back(s.substr(x, limit-x));
		x = limit + to_split.length();
	}
	return output;
}