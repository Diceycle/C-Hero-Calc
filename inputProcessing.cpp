#include "inputProcessing.h"

using namespace std;

// Length of timestamp in debug messages
const size_t FINISH_MESSAGE_LENGTH = 20;

IOManager::IOManager() {
    this->useMacroFile = false;
    this->showQueries = true;
    this->outputLevel = BASIC_OUTPUT;
    
    this->lastTimedOutput = -1;
}

bool IOManager::shouldOutput(OutputLevel urgency) {
    return (this->outputLevel >= urgency);
}

// Initialize a macro file provided by filename
void IOManager::initMacroFile(string macroFileName, bool showInput) {
    this->macroFile.open(macroFileName);
    
    if (!this->macroFile.good()) {
        this->useMacroFile = false;
        throw MACROFILE_MISSING;
    } else {    
        this->useMacroFile = true;
        this->showQueries = showInput;
    }
}

// Output method called only by class. takes an output level to determine if it should be printed or not
void IOManager::printBuffer(OutputLevel urgency) {
    if (this->shouldOutput(urgency)) {
        cout << this->outputStream.str();
    } 
    this->outputStream.str("");
    this->outputStream.clear();
}

// Returns the correct amount of spaces for indentation
string IOManager::getIndent(int indent) {
    return string(indent * INDENT_WIDTH, ' ');
}

// Output simple message
void IOManager::outputMessage(string message, OutputLevel urgency, int indent, bool linebreak) {
    this->outputStream << this->getIndent(indent) + message;
    if (linebreak) {
        this->outputStream << endl;
    }
    this->printBuffer(urgency);
}

// Output message that will be terminated by a timestamp by the next timed message
void IOManager::timedOutput(string message, OutputLevel urgency, int indent, bool reset) {
    if (this->lastTimedOutput >= 0 && !reset) {
        this->finishTimedOutput(urgency);
    }
    lastTimedOutput = time(NULL);
    this->outputStream << left << setw(STANDARD_CMD_WIDTH - FINISH_MESSAGE_LENGTH) << this->getIndent(indent) + message;
    this->printBuffer(urgency);
}

// Finish the final timed message without adding another
void IOManager::finishTimedOutput(OutputLevel urgency) {
    this->outputStream << "Done! (" << right << setw(3) << time(NULL) - this->lastTimedOutput << " seconds)" << endl; // Exactly 20 characters long
    this->printBuffer(urgency);
}

// Stop timed messages for a time. used to have properly formatted output when outputting substeps
void IOManager::suspendTimedOutputs(OutputLevel urgency) {
    this->outputStream << endl;
    this->printBuffer(urgency);
}

// Start timed outputs again.
void IOManager::resumeTimedOutputs(OutputLevel urgency) {
    this->outputStream << left << setw(STANDARD_CMD_WIDTH - FINISH_MESSAGE_LENGTH) << "";
    this->printBuffer(urgency);
}

// Wait for user input before continuing. Used to stop program from closing outside of a command line.
void IOManager::haltExecution() {
    if (this->shouldOutput(CMD_OUTPUT)) {
        cout << "Press enter to exit...";
        cin.get();
    }
}

// Method for handling ALL input. Gives access to help, error resistance and macro file for input.
string IOManager::getResistantInput(string query, QueryType queryType) {
    string inputString;
    string firstToken;
    while (true) {
        // Check first if there is still a line in the macro file
        if (this->useMacroFile) {
            this->useMacroFile = (bool) getline(this->macroFile, inputString);
        } 
        // in sever mode the macro file has to be complete
        if (!this->useMacroFile && this->outputLevel == SERVER_OUTPUT) {
            throw MACROFILE_USED_UP;
        }
        // Print the query only if no macro file is used or specifically asked for
        if (!this->useMacroFile || this->showQueries) {
            cout << query;
        }
        // Ask for user input
        if (!this->useMacroFile) {
            getline(cin, inputString);
        }
        
        // Process Input
        inputString = split(toLower(inputString), COMMENT_DELIMITOR)[0]; // trim potential comments in a macrofile and convert to lowercase
        firstToken = split(inputString, TOKEN_SEPARATOR)[0]; // except for rare input only the first string till a space is used
        if (this->useMacroFile && this->showQueries) {
            cout << inputString << endl; // Show input if a macro file is used
        }
        if (queryType == question && (firstToken == POSITIVE_ANSWER || firstToken == NEGATIVE_ANSWER)) {
            return firstToken;
        }
        if (queryType == integer) {
            try {
                stoi(firstToken);
                return firstToken;
            } catch (const exception & e) {
                this->handleInputException(NUMBER_PARSE);
            }
        }
        if (queryType == raw) {
            return inputString;
        }
        if (queryType == rawFirst) {
            return firstToken;
        }
    }
}

// Ask the user a question that they can answer via command line
bool IOManager::askYesNoQuestion(string questionMessage, OutputLevel urgency, string defaultAnswer) {
    string inputString;
    if (!this->shouldOutput(urgency)) {
        inputString = defaultAnswer;
    } else {
        inputString = this->getResistantInput(questionMessage + " (" + POSITIVE_ANSWER + "/" + NEGATIVE_ANSWER + "): ", question);
    }
    
    if (inputString == NEGATIVE_ANSWER) {
        return false;
    }
    if (inputString == POSITIVE_ANSWER) {
        return true;
    }
    return false;
}

// Promt the User via command line to input his hero levels and return a vector of their indices
vector<int8_t> IOManager::takeHerolevelInput() {
    vector<int8_t> heroes {};
    string input;
    pair<Monster, int> heroData;
    
    if (!this->useMacroFile || this->showQueries) {
        cout << endl << "Enter your Heroes with levels. Press enter after every Hero." << endl;
        cout << "Press enter twice or type done to proceed without inputting additional Heroes." << endl;
    }
    int cancelCounter = 0;
    do {
        input = this->getResistantInput("Enter Hero " + to_string(heroes.size()+1) + ": ", rawFirst);
        if (input == "") {
            cancelCounter++;
        } else {
            cancelCounter = 0;
            try {
                heroData = parseHeroString(input);
                heroes.push_back(addLeveledHero(heroData.first, heroData.second));
            } catch (InputException e) {
                this->handleInputException(e);
            };
        }
    } while (input != "done" && cancelCounter < 2);
    
    return heroes;
}

// Promts the user to input instance(s) to be solved 
vector<Instance> IOManager::takeInstanceInput(string prompt) {
    vector<Instance> instances;
    vector<string> instanceStrings;
    
    string input;
    
    while (true) {
        input = this->getResistantInput(prompt, raw);
        instances.clear();
        instanceStrings = split(input, TOKEN_SEPARATOR);
        try {
            for (size_t i = 0; i < instanceStrings.size(); i++) {
                instances.push_back(makeInstanceFromString(instanceStrings[i]));
            }
            return instances;
        } catch (InputException e) {
            this->handleInputException(e);
        }
    }
}

void IOManager::handleInputException(InputException e) {
//    switch (e) {
//        case MONSTER_PARSE: cout << "monster"; break;
//        case HERO_PARSE: cout << "hero"; break;
//        case QUEST_PARSE: cout << "quest"; break;
//        case NUMBER_PARSE: cout << "number"; break;
//        case MACROFILE_MISSING: cout << "mfMissing"; break;
//        case MACROFILE_USED_UP: cout << "mfempty"; break;
//    } cout << endl;
}

// Output an error in JSON Fromat for server mode operation
string IOManager::getJSONError(InputException e) {
    stringstream s;
    string message;
    string errorType;
    switch (e) {
        case MACROFILE_MISSING: 
            message = "Could not find Macro File!"; 
            errorType = "MACROFILE_MISSING";
            break;
        case MACROFILE_USED_UP: 
            message = "Macro File does not provide enough input!";
            errorType = "MARCOFILE_USED_UP";
            break;
        default:
            message = "Unexpected Error";
            errorType = "UNKNOWN";
            break;
    }
    s << "{\"error\" : {";
        s << "\"message\""  << ":" << "\"" << message << "\"" <<",";
        s << "\"errorType\""  << ":" << "\"" << errorType << "\"";
    s << "}}";
    return s.str();
}


void Instance::setTarget(Army aTarget) {
    this->target = aTarget;
    this->targetSize = aTarget.monsterAmount;
    this->lowestBossHealth = -1;
    
    HeroSkill currentSkill;
    this->hasAoe = false;
    this->hasAsymmetricAoe = false;
    this->hasWorldBoss = false;
    for (size_t i = 0; i < this->targetSize; i++) {
        currentSkill = monsterReference[this->target.monsters[i]].skill;
        this->hasAoe |= currentSkill.hasAoe;
        this->hasAsymmetricAoe |= currentSkill.hasAsymmetricAoe;
        this->hasWorldBoss |= monsterReference[this->target.monsters[i]].rarity == WORLDBOSS;
    }
}

// Convert a lineup string into an actual instance to solve
Instance makeInstanceFromString(string instanceString) {
    Instance instance;
    int dashPosition = (int) instanceString.find(QUEST_NUMBER_SEPARTOR);
    
    if (instanceString.compare(0, QUEST_PREFIX.length(), QUEST_PREFIX) == 0) {
        try {
            int questNumber = stoi(instanceString.substr(QUEST_PREFIX.length(), dashPosition-QUEST_PREFIX.length()));
            instance.setTarget(makeArmyFromStrings(quests[questNumber]));
            instance.maxCombatants = ARMY_MAX_SIZE - (stoi(instanceString.substr(dashPosition+1, 1)) - 1);
        } catch (const exception & e) {
            throw QUEST_PARSE;
        }
    } else {
        vector<string> stringLineup = split(instanceString, ELEMENT_SEPARATOR);
        instance.setTarget(makeArmyFromStrings(stringLineup));
        instance.maxCombatants = ARMY_MAX_SIZE;
    }
    return instance;
}

// Parse string linup input into actual monsters. If there are heroes in the input, a leveled hero is added to the database
Army makeArmyFromStrings(vector<string> stringMonsters) {
    Army army;
    pair<Monster, int> heroData;
    
    for(size_t i = 0; i < stringMonsters.size(); i++) {
        if(stringMonsters[i].find(HEROLEVEL_SEPARATOR) != stringMonsters[i].npos) {
            heroData = parseHeroString(stringMonsters[i]);
            army.add(addLeveledHero(heroData.first, heroData.second));
        } else {
            try {
                army.add(monsterMap.at(stringMonsters[i]));
            } catch (const exception & e) {
                throw MONSTER_PARSE;
            }
        }
    }
    return army;
}

// Parse hero input from a string into its name and level
pair<Monster, int> parseHeroString(string heroString) {
    string name = heroString.substr(0, heroString.find(HEROLEVEL_SEPARATOR));
    int level;
    try {
        level = stoi(heroString.substr(heroString.find(HEROLEVEL_SEPARATOR)+1));
    } catch (const exception & e) {
        throw HERO_PARSE;
    }
    
	Monster hero;
	for (size_t i = 0; i < baseHeroes.size(); i++) {
		if (baseHeroes[i].baseName == name) {
            return pair<Monster, int>(baseHeroes[i], level);
		}
	}
    throw HERO_PARSE;
}

// Create valid string to be used ingame to view the battle between armies friendly and hostile
string makeBattleReplay(Army friendly, Army hostile) {
    stringstream replay;
    replay << "{";
        replay << "\"winner\""  << ":" << "\"Unknown\"" << ",";
        replay << "\"left\""    << ":" << "\"Solution\"" << ",";
        replay << "\"right\""   << ":" << "\"Instance\"" << ",";
        replay << "\"date\""    << ":" << time(NULL) << ",";
        replay << "\"title\""   << ":" << "\"Proposed Solution\"" << ",";
        replay << "\"setup\""   << ":" << getReplaySetup(friendly) << ",";
        replay << "\"shero\""   << ":" << getReplayHeroes(friendly) << ",";
        replay << "\"player\""  << ":" << getReplaySetup(hostile) << ",";
        replay << "\"phero\""   << ":" << getReplayHeroes(hostile);
    replay << "}";
    string unencoded = replay.str();
    return base64_encode((const unsigned char*) unencoded.c_str(), (int) unencoded.size());
}

// Get lineup in ingame indices
string getReplaySetup(Army setup) {
    stringstream stringSetup;
    size_t i;
    stringSetup << "[";
    for (i = 0; i < ARMY_MAX_SIZE * TOURNAMENT_LINES; i++) {
        if ((int) (i % ARMY_MAX_SIZE) < setup.monsterAmount) {
            stringSetup << getRealIndex(monsterReference[setup.monsters[setup.monsterAmount - (i % ARMY_MAX_SIZE) - 1]]);
        } else {
            stringSetup << to_string(INDEX_NO_MONSTER);
        }
        if (i < ARMY_MAX_SIZE * TOURNAMENT_LINES - 1) {
            stringSetup << ",";
        }
    }
    stringSetup << "]";
    return stringSetup.str();
}

// Get list of relevant herolevels in ingame format
string getReplayHeroes(Army setup) {
    stringstream heroes;
    Monster monster;
    int level;
    heroes << "[";
    for (size_t i = 0; i < baseHeroes.size(); i++) {
        level = 0;
        for (int j = 0; j < setup.monsterAmount; j++) {
            monster = monsterReference[setup.monsters[j]];
            if (monster.rarity != NO_HERO && monster.baseName == baseHeroes[i].baseName) {
                level = monster.level;
                break;
            }
        }
        heroes << level;
        if (i < baseHeroes.size()-1) {
            heroes << ",";
        }
    }
    heroes << "]";
    return heroes.str();
}

string Instance::toJSON(bool valid) {
    stringstream s;
    s << "{\"validSolution\" : {";
    if (this->hasWorldBoss) {
        s << "\"bossdamage\"" << ":" << WORLDBOSS_HEALTH - this->lowestBossHealth << ",";
    }
        s << "\"target\""  << ":" << this->target.toJSON() << ",";
        s << "\"solution\""  << ":" << this->bestSolution.toJSON() << ",";
        s << "\"time\""  << ":" << this->calculationTime << ",";
        s << "\"fights\"" << ":" << this->totalFightsSimulated << ",";
        s << "\"replay\"" << ":" << "\"" << makeBattleReplay(this->bestSolution, this->target) << "\"";
    
    s << "}";
    if (!valid) {
        s << ",\"error\" : {";
            s << "\"message\"" << ":" << "\"Fatal Internal Error: Solution not valid!!!\"" << ",";
            s << "\"errorType\"" << ":" << "\"SANITY_CHECK_FAILED\"";
        s << "}";
    }
    s << "}";
    return s.str();
}

string Instance::toString(bool valid, bool showReplayString) {
    stringstream s;
        
    s << endl << "Solution for " << this->target.toString() << ":" << endl;
    // Announce the result
    if (!this->bestSolution.isEmpty()) {
        s << "  " << this->bestSolution.toString() << endl;
    } else {
        s << endl << "Could not find a solution that beats this lineup." << endl;
    }
    if (this->hasWorldBoss) {
        s << "  Boss Damage Done: " << WORLDBOSS_HEALTH - this->lowestBossHealth << endl;
    }
    s << "  " << this->totalFightsSimulated << " Fights simulated." << endl;
    s << "  Total Calculation Time: " << this->calculationTime << endl;
    s << "  Calc Version: " << VERSION << endl << endl;
    if (!this->bestSolution.isEmpty() && showReplayString) {
        s << "Battle Replay (Use on Ingame Tournament Page):" << endl << makeBattleReplay(this->bestSolution, this->target) << endl << endl;
    }
    if (!valid) {
        s << "This does not beat the lineup!!!" << endl;
        s << "FATAL ERROR!!! Please comment this output in the Forums!" << endl;
    }
    return s.str();
}

// Splits strings into a vector of strings.
vector<string> split(string target, string separator) {
    vector<string> output;
    string substring;
    size_t start = 0;
    size_t occurrence = 0;
    while(occurrence != target.npos) {
        occurrence = target.find(separator, start);
        substring = target.substr(start, occurrence-start);
        if (start == 0 || substring.length() > 0) {
            output.push_back(substring);
        }
        start = occurrence + separator.length();
    }
    return output;
}

// Convert a string to lowercase where available
string toLower(string input) {
    for (size_t i = 0; i < input.length(); i++) {
        input[i] = tolower(input[i], locale());
    }
    return input;
}