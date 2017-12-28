#ifndef COSMOS_INPUT_HEADER
#define COSMOS_INPUT_HEADER

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include <stdexcept>

#include "cosmosData.h"
#include "base64.h"

const std::string VERSION = "2.9.1.6";

const size_t STANDARD_CMD_WIDTH = 80;
const int INDENT_WIDTH = 2;

const std::string welcomeMessage = "Welcome to Diceycle's PvE Instance Solver!";
const std::string helpMessage = "If you don't know what to do you can type help at any time to get an explanation about the current step.";

enum QueryType {question, integer, raw, rawFirst}; // Types of command line promts

const std::string POSITIVE_ANSWER = "y";
const std::string NEGATIVE_ANSWER = "n";

const std::string TOKEN_SEPARATOR = " ";
const std::string ELEMENT_SEPARATOR = ",";
const std::string COMMENT_DELIMITOR = "//";
const std::string QUEST_PREFIX = "quest";
const std::string QUEST_NUMBER_SEPARTOR = "-";

// Enum to control the amount of output generate
enum OutputLevel {
    VITAL_OUTPUT    = 0,
    SERVER_OUTPUT   = 1,
    CMD_OUTPUT      = 2,
    SOLUTION_OUTPUT = 3,
    BASIC_OUTPUT    = 4,
    DETAILED_OUTPUT = 5
};

// An instance to be solved by the program
struct Instance {
    Army target;
    size_t targetSize;
    size_t maxCombatants;
    
    int followerUpperBound;
    Army bestSolution;
    
    time_t calculationTime;
    int totalFightsSimulated = 0;
    
    bool hasAoe;
    bool hasAsymmetricAoe;
    bool hasWorldBoss;
    int lowestBossHealth;
    
    void setTarget(Army aTarget);
    std::string toString(bool valid, bool showReplayString = true);
    std::string toJSON(bool valid);
};

// TODO: Detect double hero inputs
enum InputException {
    MONSTER_PARSE,
    HERO_PARSE,
    QUEST_PARSE,
    NUMBER_PARSE,
    MACROFILE_MISSING,
    MACROFILE_USED_UP
};

class IOManager {
    private:
        bool useMacroFile;
        bool showQueries = true;
        std::ifstream macroFile;

        time_t lastTimedOutput = -1;
        std::ostringstream outputStream;
        
        std::string getIndent(int indent);
        void printBuffer(OutputLevel urgency);
        bool shouldOutput(OutputLevel urgency);
        void handleInputException(InputException e);
        
    public:
        OutputLevel outputLevel;
        
        IOManager();
        
        void initMacroFile(std::string macroFileName, bool showInput);
        std::string getResistantInput(std::string query, QueryType queryType = raw);
        bool askYesNoQuestion(std::string question, OutputLevel urgency, std::string defaultAnswer);
        std::vector<int8_t> takeHerolevelInput();
        std::vector<Instance> takeInstanceInput(std::string promt);
        
        void outputMessage(std::string message, OutputLevel urgency, int indent = 0, bool linebreak = true);
        void timedOutput(std::string message, OutputLevel urgency, int indent = 0, bool reset = false);
        void suspendTimedOutputs(OutputLevel urgency);
        void resumeTimedOutputs(OutputLevel urgency);
        void finishTimedOutput(OutputLevel urgency);
        
        std::string getJSONError(InputException e);
        
        void haltExecution();
};

// Convert a lineup string into an actual instance to solve
Instance makeInstanceFromString(std::string instanceString);

// Parse string linup input into actual monsters. If there are heroes in the input, a leveled hero is added to the database
Army makeArmyFromStrings(std::vector<std::string> stringMonsters);

// Parse hero input from a string into its name and level
std::pair<Monster, int> parseHeroString(std::string heroString);

// Functions for making a valid ingame replay string
std::string makeBattleReplay(Army friendly, Army hostile);
std::string getReplaySetup(Army setup);
std::string getReplayHeroes(Army setup);

// Splits strings into a vector of strings. No need to optimize, only used for input.
std::vector<std::string> split(std::string target, std::string separator);

// Convert a string to lowercase where available
std::string toLower(std::string input);

#endif