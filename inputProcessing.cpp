#include "inputProcessing.h"

bool useMacroFile;
bool showQueries = true;
ifstream macroFile;

// Initialize a config file provided by filename
void initMacroFile(string macroFileName, bool showInput) {
    macroFile.open(macroFileName);
    
    useMacroFile = macroFile.good();
    showQueries = macroFile.good() && showInput;
    if (!macroFile.good()) {
        cout << "Could not find Macro File. Switching to Manual Input." << endl;
    }
}

// Wait for user input before continuing. Used to stop program from closing outside of a command line.
void haltExecution() {
    cout << "Press enter to exit...";
    cin.get();
}

// Method for handling ALL input. Gives access to help, error resistance and config file for input.
string getResistantInput(string query, string help, QueryType queryType) {
    string inputString;
    string firstElement;
    while (true) {
        // Check first if there is still a line in the macro file
        if (useMacroFile) {
            useMacroFile = (bool) getline(macroFile, inputString);
        } 
        // Print the query only if no macro file is used or specifically asked for
        if (!useMacroFile || showQueries) {
            cout << query;
        }
        // Ask for user input
        if (!useMacroFile) {
            getline(cin, inputString);
        }
        
        // Process Input
        inputString = split(inputString, "//")[0]; // trim potential comments in a macrofile
        firstElement = split(inputString, " ")[0]; // except for rare input only the first string till a space is used
        if (useMacroFile && showQueries) {
            cout << inputString << endl; // Show input if a macro file is used
        }
        if (firstElement == "help") {
            cout << help;
        } else {
            if (queryType == question && (firstElement == "y" || firstElement == "n")) {
                return firstElement;
            }
            if (queryType == integer) {
                try {
                    stoi(firstElement);
                    return firstElement;
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
    bool fileInput;
    
    fileInput = askYesNoQuestion(heroInputModeQuestion, heroInputModeHelp);
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
        if (!useMacroFile || showQueries) {
            cout << "Enter the level of the hero, whose name is shown." << endl;
        }
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
        if (!useMacroFile || showQueries) {
            cout << "Hero Levels have been saved in a file. Next time you use this program you can load them from file." << endl;
        }
    }
    return levels;
}

// Returns multiple Instaces parse from command line input
vector<Instance> takeInstanceInput(string prompt) {
    vector<Instance> instances;
    vector<string> instanceStrings;
    
    string input;
    
    while (true) {
        input = getResistantInput(prompt, lineupInputHelp, raw);
        instanceStrings = split(input, " ");
        try {
            for (size_t i = 0; i < instanceStrings.size(); i++) {
                instances.push_back(makeInstanceFromString(instanceStrings[i]));
            }
            return instances;
        } catch (const exception & e) {}
    }
}

// Convert a lineup string into an actual instance to solve
Instance makeInstanceFromString(string instanceString) {
    Instance instance;
    string questString = "quest";
    int dashPosition = instanceString.find("-");
    
    if (instanceString.compare(0, questString.length(), questString) == 0) {
        int questNumber = stoi(instanceString.substr(questString.length(), dashPosition-questString.length()));
        instance.target = makeArmyFromStrings(quests[questNumber]);
        instance.maxCombatants = 7 - stoi(instanceString.substr(dashPosition+1, 1));
    } else {
        vector<string> stringLineup = split(instanceString, ",");
        instance.target = makeArmyFromStrings(stringLineup);
        instance.maxCombatants = 6;
    }
    instance.targetSize = instance.target.monsterAmount;
    return instance;
}

// Parse string linup input into actual monsters if there are heroes in the input, a leveled hero is added to the database
Army makeArmyFromStrings(vector<string> stringMonsters) {
    Army army;
    pair<Monster, int> heroData;
    
    for(size_t i = 0; i < stringMonsters.size(); i++) {
        if(stringMonsters[i].find(":") != stringMonsters[i].npos) {
            heroData = parseHeroString(stringMonsters[i]);
            army.add(addLeveledHero(heroData.first, heroData.second));
        } else {
            army.add(monsterMap.at(stringMonsters[i]));
        }
    }
    return army;
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
vector<string> split(string target, string separator) {
    vector<string> output;
    string substring;
    size_t start = 0;
    size_t occurrence = 0;
    while(occurrence != target.npos) {
        occurrence = target.find(separator, start);
        substring = target.substr(start, occurrence-start);
        if (start == 0 || substring.length() > 0) {
            cout << substring << endl;
            output.push_back(substring);
        }
        start = occurrence + separator.length();
    }
    return output;
}