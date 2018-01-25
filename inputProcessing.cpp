#include "inputProcessing.h"

using namespace std;

// Length of timestamp in debug messages
const size_t FINISH_MESSAGE_LENGTH = 20;
const string DEFAULT_MACRO = "default.cqinput";
const string DEFAULT_CONFIG = "default.cqconfig";
Configuration config;
UserInterface interface;

// Output the buffer to command line
void UserInterface::printBuffer(OutputLevel urgency) {
    if (shouldOutput(urgency)) {
        cout << this->outputStream.str();
    } 
    this->outputStream.str("");
    this->outputStream.clear();
}

// Output simple message
void UserInterface::outputMessage(string message, OutputLevel urgency, int indent, bool linebreak) {
    this->outputStream << this->getIndent(indent) + message;
    if (linebreak) {
        this->outputStream << endl;
    }
    this->printBuffer(urgency);
}

// Output message that will be terminated by a timestamp by the next timed message
void UserInterface::timedOutput(string message, OutputLevel urgency, int indent, bool reset) {
    if (this->lastTimedOutput >= 0 && !reset) {
        this->finishTimedOutput(urgency);
    }
    lastTimedOutput = time(NULL);
    this->outputStream << left << setw(STANDARD_CMD_WIDTH - FINISH_MESSAGE_LENGTH) << this->getIndent(indent) + message;
    this->printBuffer(urgency);
}

// Finish the final timed message without adding another
void UserInterface::finishTimedOutput(OutputLevel urgency) {
    this->outputStream << "Done! (" << right << setw(3) << time(NULL) - this->lastTimedOutput << " seconds)" << endl; // Exactly 20 characters long
    this->printBuffer(urgency);
}

// Stop timed messages for a time. used to have properly formatted output when outputting substeps
void UserInterface::suspendTimedOutputs(OutputLevel urgency) {
    this->outputStream << endl;
    this->printBuffer(urgency);
}

// Start timed outputs again.
void UserInterface::resumeTimedOutputs(OutputLevel urgency) {
    this->outputStream << left << setw(STANDARD_CMD_WIDTH - FINISH_MESSAGE_LENGTH) << "";
    this->printBuffer(urgency);
}

// Wait for user input before continuing. Used to stop program from closing outside of a command line.
void UserInterface::haltExecution() {
    if (shouldOutput(NOTIFICATION_OUTPUT)) {
        cout << "Press enter to exit...";
        cin.get();
    }
}

// Takes a raw line from an Input file or command line input and splits it into tokens
vector<string> UserInterface::parseInput(string input) {
    input = split(toLower(input), COMMENT_DELIMITOR)[0]; 
    return split(input, TOKEN_SEPARATOR);
}

// Returns the correct amount of spaces for indentation
string UserInterface::getIndent(int indent) {
    return string(indent * INDENT_WIDTH, ' ');
}

// Try to get input from a file by heirarchy
void InputFileManager::init(string fileName) {
    // Try file specified on command line first
    if (fileName != "" && this->readFile(fileName, true)) {
        interface.outputMessage("Using specified File...", NOTIFICATION_OUTPUT);
    } else {
        if (this->readFile(DEFAULT_CONFIG, true)) {
            interface.outputMessage("Using default Configfile...", NOTIFICATION_OUTPUT);
        } else {
            if (this->readFile(DEFAULT_MACRO, true)) {
                interface.outputMessage("Using default Macrofile...", NOTIFICATION_OUTPUT);
            } else {
                interface.outputMessage("Switching to manual Input...", NOTIFICATION_OUTPUT);
            }
        }
    }
}

// Open a file by name, read and parse its contents and follow recursive links to other files
// Put results in inputLines which is used by other methods
bool InputFileManager::readFile(string fileName, bool important) {
    ifstream fileStream;
    fileStream.open(fileName);
    if (!fileStream.good()) {
        string message = "Could not find or open "+ fileName +"!";
        if (!important) {
            message += " Ignoring...";
        }
        interface.outputMessage(message, NOTIFICATION_OUTPUT);
        return false;
    }
    string input;
    vector<string> tokens;
    while (getline(fileStream, input)) {
        tokens = interface.parseInput(input);
        if (tokens[0] == TOKENS.NEXT_FILE) {
            this->readFile(tokens[1], false);
        } else {
            this->inputLines.push(tokens);
        }
    }
    return true;
}

// Check if the first line has a specific token. If so remove it and return true
bool InputFileManager::checkLine(string token) {
    if (this->hasLine() && this->inputLines.front()[0] == token) {
        this->inputLines.pop();
        return true;
    } else {
        return false;
    }
}

// Pop and return the first line from inputLines
vector<string> InputFileManager::getLine() {
    vector<string> tokens = this->inputLines.front();
    this->inputLines.pop();
    if (!this->hasLine()) {
        config.showQueries = true;
    }
    return tokens;
}

bool InputFileManager::hasLine() {
    return !this->inputLines.empty();
}

// Wrapper method to initialize InputFileManager
void IOManager::loadInputFiles(string fileName) {
    this->fileInput.init(fileName);
}

// Read from the input and set configuration values accordingly. 
// Outputs warnings to the command line if something is out of order
void IOManager::getConfiguration() {
    vector<string> tokens;
    
    if (this->fileInput.checkLine(TOKENS.START_CONFIG)) {
        while (this->fileInput.hasLine()) {
            tokens = this->fileInput.getLine();
            if (tokens[0] == TOKENS.START_ENTITIES) {
                return;
            } else if (config.allowConfig) {
                try {
                    if (tokens[0] == TOKENS.AUTO_ADJUST_OUTPUT) {
                        config.autoAdjustOutputLevel = parseBool(tokens.at(1));
                    } else if (tokens[0] == TOKENS.FIRST_DOMINANCE) {
                        config.firstDominance = parseInt(tokens.at(1));
                    } else if (tokens[0] == TOKENS.IGNORE_EMPTY) {
                        config.ignoreEmptyLines = parseBool(tokens.at(1));
                    } else if (tokens[0] == TOKENS.OUTPUT_LEVEL) {
                        config.outputLevel = parseOutputLevel(tokens.at(1));
                    } else if (tokens[0] == TOKENS.SHOW_QUERIES) {
                        config.showQueries = parseBool(tokens.at(1));
                    } else if (tokens[0] == TOKENS.SHOW_REPLAY_STRINGS) {
                        config.showReplayStrings = parseBool(tokens.at(1));
                    } else if (tokens[0] != TOKENS.EMPTY) {
                        interface.outputMessage("Unrecognized option '" + tokens[0] + "'", NOTIFICATION_OUTPUT);
                    }
                } catch (const invalid_argument &e) {
                    interface.outputMessage((string) (e.what()) + " Ignoring option '" + tokens[0] + "'!", NOTIFICATION_OUTPUT);
                } catch (const out_of_range &e) {
                    interface.outputMessage("Didn't find enough arguments for '" + tokens[0] + "'! Ignoring...", NOTIFICATION_OUTPUT);
                }
            }
        }
    }
}

// Method for handling ALL input. Gives access to error resistance and input files.
vector<string> IOManager::getResistantInput(string query, QueryType queryType) {
    string inputString;
    vector<string> tokens;
    string firstToken;
    while (true) {
        // in sever mode the macro file has to be complete TODO Server mode

        // Ask for user input
        if (!this->fileInput.hasLine()) {
            interface.outputMessage(query, QUERY_OUTPUT, 0, false);
            getline(cin, inputString);
            tokens = interface.parseInput(inputString);
        } else {
            tokens = this->fileInput.getLine();
            if (config.ignoreEmptyLines && tokens[0] == TOKENS.EMPTY) {
                continue;
            }
            if (config.showQueries) {
                interface.outputMessage(query, QUERY_OUTPUT, 0, false);
                for (size_t i = 0; i < tokens.size(); i++) {
                    interface.outputMessage(tokens[i] + TOKEN_SEPARATOR, QUERY_OUTPUT, 0, false);;
                } interface.outputMessage("", QUERY_OUTPUT);
            }
        }

        if (queryType == question && (tokens[0] == TOKENS.YES || tokens[0] == TOKENS.NO)) {
            return tokens;
        }
        if (queryType == integer) {
            try {
                stoi(tokens[0]);
                return tokens;
            } catch (const exception & e) {
                this->handleInputException(NUMBER_PARSE);
            }
        }
        if (queryType == raw) {
            return tokens;
        }
        if (queryType == rawFirst) {
            return tokens;
        }
    }
}

// Ask the user a question that they can answer via command line
bool IOManager::askYesNoQuestion(string questionMessage, OutputLevel urgency, string defaultAnswer) {
    string inputString;
    if (!shouldOutput(urgency)) {
        inputString = defaultAnswer;
    } else {
        inputString = this->getResistantInput(questionMessage + " (" + TOKENS.YES + "/" + TOKENS.NO + "): ", question)[0];
    }
    
    if (inputString == TOKENS.NO) {
        return false;
    }
    if (inputString == TOKENS.YES) {
        return true;
    }
    return false;
}

// Promt the User via command line to input his hero levels and return a vector of their indices in the monster reference
vector<int8_t> IOManager::takeHerolevelInput() {
    vector<int8_t> heroes {};
    vector<string> input;
    pair<Monster, int> heroData;
    
    interface.outputMessage("\nEnter your Heroes with levels. Press enter after every Hero.", QUERY_OUTPUT);
    interface.outputMessage("Press enter twice or type" + TOKENS.HEROES_FINISHED + "to proceed without inputting additional Heroes.", QUERY_OUTPUT);
        
    int cancelCounter = 0;
    do {
        input = this->getResistantInput("Enter Hero " + to_string(heroes.size()+1) + ": ", rawFirst);
        if (input[0] == TOKENS.EMPTY) {
            cancelCounter++;
        } else {
            cancelCounter = 0;
            try {
                heroData = parseHeroString(input[0]);
                heroes.push_back(addLeveledHero(heroData.first, heroData.second));
            } catch (InputException e) {
                this->handleInputException(e);
            };
        }
    } while (input[0] != TOKENS.HEROES_FINISHED && cancelCounter < 2);
    
    return heroes;
}

// Promts the user to input instance(s) to be solved 
vector<Instance> IOManager::takeInstanceInput(string prompt) {
    vector<Instance> instances;
    vector<string> tokens;
    
    while (true) {
        tokens = this->getResistantInput(prompt, raw);
        instances.clear();
        try {
            for (size_t i = 0; i < tokens.size(); i++) {
                instances.push_back(makeInstanceFromString(tokens[i]));
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

// Determine whether a message should be printed depending on the outputlevel
bool shouldOutput(OutputLevel urgency) {
    if (urgency == QUERY_OUTPUT) {
        return config.showQueries;
    } else {
        return (config.outputLevel >= urgency);
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

string makeJSONFromInstance(Instance instance, bool valid) {
    stringstream s;
    s << "{\"validSolution\" : {";
    if (instance.hasWorldBoss) {
        s << "\"bossdamage\"" << ":" << WORLDBOSS_HEALTH - instance.lowestBossHealth << ",";
    }
        s << "\"target\""  << ":" << instance.target.toJSON() << ",";
        s << "\"solution\""  << ":" << instance.bestSolution.toJSON() << ",";
        s << "\"time\""  << ":" << instance.calculationTime << ",";
        s << "\"fights\"" << ":" << instance.totalFightsSimulated << ",";
        s << "\"replay\"" << ":" << "\"" << makeBattleReplay(instance.bestSolution, instance.target) << "\"";
    
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

string makeStringFromInstance(Instance instance, bool valid, bool showReplayString) {
    stringstream s;
        
    s << endl << "Solution for " << instance.target.toString() << ":" << endl;
    // Announce the result
    if (!instance.bestSolution.isEmpty()) {
        s << "  " << instance.bestSolution.toString() << endl;
    } else {
        s << "  Could not find a solution that beats this lineup." << endl;
    } s << endl;
    
    // Aditional Statistics
    if (instance.hasWorldBoss) {
        s << "  Boss Damage Done: " << WORLDBOSS_HEALTH - instance.lowestBossHealth << endl;
    }
    s << "  " << instance.totalFightsSimulated << " Fights simulated." << endl;
    s << "  Total Calculation Time: " << instance.calculationTime << endl;
    s << "  Calc Version: " << VERSION << endl << endl;
    
    // Replay for debugging and confirming interactions
    if (!instance.bestSolution.isEmpty() && showReplayString) {
        s << "Battle Replay (Use on Ingame Tournament Page):" << endl << makeBattleReplay(instance.bestSolution, instance.target) << endl << endl;
    }
    
    // Sanity Check
    if (!valid) {
        s << "This does not beat the lineup!!!" << endl;
        s << "FATAL ERROR!!! Please comment this output in the Forums!" << endl;
    }
    return s.str();
}

bool parseBool(string toParse) {
    if (toParse == TOKENS.FALSE) {
        return false;
    } else if (toParse == TOKENS.TRUE) {
        return true;
    } else {
        throw invalid_argument("Could not parse boolean from '" + toParse + "'!");
    }
}

int parseInt(string toParse) {
    try {
        return stoi(toParse);
    } catch (const invalid_argument &e) {
        throw invalid_argument("Could not parse number from '" + toParse + "'!");
    }
}

OutputLevel parseOutputLevel(string toParse) {
    if (toParse == TOKENS.T_BASIC_OUTPUT) {
        return BASIC_OUTPUT;
    } else if (toParse == TOKENS.T_DETAILED_OUPUT) {
        return DETAILED_OUTPUT;
    } else if (toParse == TOKENS.T_SOLUTION_OUTPUT) {
        return SOLUTION_OUTPUT;
    } else {
        throw invalid_argument("Could not parse OutputLevel from '" + toParse + "'!");
    }
}

// Splits strings into a vector of strings. Always returns at least 1 empty string
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