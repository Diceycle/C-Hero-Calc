#ifndef COSMOS_INPUT_HEADER
#define COSMOS_INPUT_HEADER

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include <stdexcept>

#include "cosmosDefines.h"
#include "base64.h"

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

const int REPLAY_EMPTY_SPOT = -1;

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
        
    public:
        OutputLevel outputLevel;
        
        IOManager();
        
        void initMacroFile(std::string macroFileName, bool showInput);
        std::string getResistantInput(std::string query, std::string help, QueryType queryType = raw);
        bool askYesNoQuestion(std::string question, std::string help, OutputLevel urgency, std::string defaultAnswer);
        std::vector<int8_t> takeHerolevelInput();
        std::vector<Instance> takeInstanceInput(std::string promt);
        
        void outputMessage(std::string message, OutputLevel urgency, int indent = 0, bool linebreak = true);
        void timedOutput(std::string message, OutputLevel urgency, int indent = 0, bool reset = false);
        void suspendTimedOutputs(OutputLevel urgency);
        void resumeTimedOutputs(OutputLevel urgency);
        void finishTimedOutput(OutputLevel urgency);
        
        void haltExecution();
};
    
const std::string heroInputHelp = 
    "  Enter any heroes you want to enable. in the format name" + HEROLEVEL_SEPARATOR() + "level. Press enter after every hero.\n"
    "  To finish entering heroes, simply press enter twice or type done and press enter.\n";

const std::string lineupInputHelp = 
    "  Enter Monsters separated by " + ELEMENT_SEPARATOR + " .\n"
    "  Normal monsters are written with their element (a,e,w,f) and their tier number. So the level 5 water monster is w5.\n"
    "  Heroes are written first with their full name and a " + HEROLEVEL_SEPARATOR() + " followed by their level. For example, forestdruid" + HEROLEVEL_SEPARATOR() + "50\n"
    "  Full example: a1" + ELEMENT_SEPARATOR + "geror" + HEROLEVEL_SEPARATOR() + "22" + ELEMENT_SEPARATOR + "f13" + ELEMENT_SEPARATOR + "w2" + ELEMENT_SEPARATOR + "ladyoftwilight" + HEROLEVEL_SEPARATOR() + "1\n"
    "  The other alternative is selecting a quest from the game. "
    "For example: Typing " + QUEST_PREFIX + "23" + QUEST_NUMBER_SEPARTOR + "3 loads the lineup for the 23rd quest and tries to beat it with 4 monsters or less.\n"
    "  You can also enter multiple lineups at once. Do so by separating them with spaces.\n"
    "  Example: " + QUEST_PREFIX + "5" + QUEST_NUMBER_SEPARTOR + "3 a1" + ELEMENT_SEPARATOR + "a2" + ELEMENT_SEPARATOR + "a3 " + QUEST_PREFIX + "8" + QUEST_NUMBER_SEPARTOR + "1\n"
    "  In this Example the program will calculate those three lineups one after another.\n";
    
const std::string minimumMonsterCostHelp = 
    "  This determines how expensive a monster needs to be in order for the calculator to consider it for a solution.\n"
    "  This feature is intended for users with a lot of followers or good heroes to ignore monsters like a1.\n"
    "  Example: Entering 215000 will exclude e8 and cheaper monsters in the solution.\n"
    "  Special Values are: 0 for ALL monsters considered and -1 for NO monsters considered.\n";
    
const std::string maxFollowerHelp = 
    "  This determines how expensive the entire solution is allowed to be.\n"
    "  I only reluctantly put this option in because a lot of people asked for it. "
    "Note that as soon as calculation starts an upper bound is automatically generated by a greedy approach.\n"
    "  You can enter your followers here if you think that it speeds up calculation. "
    "But then you won't be able to know how many followers you are missing to beat the lineup. Your choice.\n"
    "  Enter -1 if you don't want to set the limit yourself.\n";

// Convert a lineup string into an actual instance to solve
Instance makeInstanceFromString(std::string instanceString);

// Parse string linup input into actual monsters. If there are heroes in the input, a leveled hero is added to the database
Army makeArmyFromStrings(std::vector<std::string> stringMonsters);

// Parse hero input from a string into its name and level
std::pair<Monster, int> parseHeroString(std::string heroString);

// Functions for making a valid ingame replay string
std::string makeBattleReplay(Army friendly, Army hostile);
std::string getReplaySetup(Army setup);
std::string getReplayMonsterNumber(Monster monster);
std::string getReplayHeroes(Army setup);

// Splits strings into a vector of strings. No need to optimize, only used for input.
std::vector<std::string> split(std::string target, std::string separator);

// Convert a string to lowercase where available
std::string toLower(std::string input);

#endif