#include "inputProcessing.h"

// Wait for user input before continuing. Used to stop program from colsing outside of a command line.
void haltExecution() {
    cout << "Press enter to continue...";
    cin.sync();
    cin.get();
}

// Ask the user a question that they can answer via command line
bool askYesNoQuestion(string question, string autoc) {
    string inputString;
    while (true) {
		cout << question;
		if (autoc == "n" || autoc == "N") {
            cout  << " ( y/[N] ) : ";
			}
		if (autoc == "y" || autoc == "Y") {
            cout << " ( [Y]/n ) : ";
			}
		getline(cin, inputString);

        if (inputString.size() == 0)
			inputString = autoc;
		if (inputString == "n" || inputString == "N") {
            return false;
        }
        if (inputString == "y" || inputString == "Y") {
            return true;
        }
    }
    return false;
}

string askQuestion(string question, string autoc) {
    string inputString;
	cout << question << " [" << autoc << "] : ";
	getline(cin, inputString);
	if (inputString.size() == 0)
		inputString = autoc;
	return inputString;
}
// Output things on the command line. Using shouldOutput this can be easily controlled globally
void debugOutput(int timeStamp, string message, bool shouldOutput, bool finishLastOutput, bool finishLine) {
    if (shouldOutput) { 
        if (finishLastOutput) {
            cout << "Done! (" << right << setw(3) << time(NULL) - timeStamp << " seconds) " << endl; 
        }
        if (message != "") {
            cout << left << setw(50) << message;
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
    
    if (askYesNoQuestion("Do you want to load hero levels from file?")) {
        try {
            heroFile.open("heroLevels", fstream::in);
            heroFile >> input;
            stringLevels = split(input, ",");
            for (size_t i = 0; i < stringLevels.size(); i++) {
                levels.push_back(stoi(stringLevels[i]));
            }
            heroFile.close();
        } catch (const exception & e) {
            cout << "Could not open File. Make sure you input the hero Levels manually at least once." << endl;
            throw runtime_error("Hero File not found");
        }
    } else {
        cout << "Enter the level of the hero, whose name is shown (Enter 0 if you don't own the Hero)" << endl;
        for (size_t i = 0; i < baseHeroes.size(); i++) {
            cout << baseHeroes[i].name << ": ";
            getline(cin, input);
            levels.push_back(stoi(input));
        }
        
        // Write Hero Levels to file to use next time
        heroFile.open("heroLevels", fstream::out);
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
vector<Monster *> takeLineupInput() {
    vector<Monster *> lineup {};
    string questString = "quest";
    
    string input;
    cout << "Enter desired Lineup separated with commas (no spaces!)" << endl;
    cout << "Alternatively: type f.e. quest17 to get the lineup for quest 17." << endl;
    getline(cin, input);
    
    if (input.compare(0, questString.length(), questString) == 0) {
        int questNumber = stoi(input.substr(questString.length(), 2));
        lineup = makeMonstersFromStrings(quests[questNumber-1]);
    } else {
        vector<string> stringLineup = split(input, ",");
        lineup = makeMonstersFromStrings(stringLineup);
    }
    
    return lineup;
}

// Parse string linup input into actual monsters if there are heroes in the input, a leveled hero is added to the database
vector<Monster *> makeMonstersFromStrings(vector<string> stringLineup) {
    vector<Monster *> lineup {};
    pair<Monster, int> heroData;
    
    for(size_t i = 0; i < stringLineup.size(); i++) {
        if(stringLineup[i].find(":") != stringLineup[i].npos) {
            heroData = parseHeroString(stringLineup[i]);
            addLeveledHero(heroData.first, heroData.second);
        }
        lineup.push_back(monsterMap.at(stringLineup[i]));
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

// Add a leveled hero to the databse 
void addLeveledHero(Monster hero, int level) {
    Monster m = getLeveledHero(hero, rarities.at(hero.name), level);
    heroReference.emplace_back(m);
    
    monsterMap.insert(pair<string, Monster *>(m.name, &(heroReference[heroReference.size() -1])));
}

// Create a new hero with leveled stats and return it
Monster getLeveledHero(const Monster & m, int rarity, int level) {
    int points = level-1;
    if (rarity == 1) {
        points = 2 * points;
    } else if (rarity == 2) {
        points = 6 * points;
    }
    int value = m.hp + m.damage;
    return Monster(
        round(m.hp + points * ((double)m.hp) / value),
        m.damage + round(points * ((double)m.damage) / value),
        m.cost,
        m.name + ":" + to_string(level),
        m.element,
        m.skill
    );
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
