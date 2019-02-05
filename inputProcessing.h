#ifndef COSMOS_INPUT_HEADER
#define COSMOS_INPUT_HEADER

#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include <stdexcept>

#include "cosmosData.h"
#include "base64.h"

const size_t STANDARD_CMD_WIDTH = 80;
const int INDENT_WIDTH = 2;

const std::string welcomeMessage = "Welcome to Diceycle's PvE Instance Solver!";
const std::string helpMessage = "If you are new to using this Calculator I recommend reading the Github README.";

enum QueryType {question, integer, raw, rawFirst}; // Types of command line promts

const std::string TOKEN_SEPARATOR = " ";
const std::string ELEMENT_SEPARATOR = ",";
const std::string COMMENT_DELIMITOR = "//";
const std::string QUEST_PREFIX = "quest";
const std::string QUEST_NUMBER_SEPARTOR = "-";

// Enum to control the amount of output generate
enum OutputLevel {
    VITAL_OUTPUT        = 0,
    SOLUTION_OUTPUT     = 1,
    NOTIFICATION_OUTPUT = 2,
    BASIC_OUTPUT        = 3,
    DETAILED_OUTPUT     = 4,
    QUERY_OUTPUT
};

enum InputException {
    MONSTER_PARSE,
    HERO_PARSE,
    QUEST_PARSE,
    NUMBER_PARSE,
    MACROFILE_MISSING,
    MACROFILE_USED_UP
};

struct ParserTokens {
    const std::string START_CONFIG =        "config";
    const std::string START_ENTITIES =      "entities";
    const std::string HEROES_FINISHED =     "done";
    const std::string NEXT_FILE =           "next_file";
    const std::string TRUE =                "true";
    const std::string FALSE =               "false";
    const std::string YES =                 "y";
    const std::string NO =                  "n";
    const std::string EMPTY =               "";

    const std::string SHOW_QUERIES =        "show_queries";
    const std::string FIRST_DOMINANCE =     "first_dominance";
    const std::string OUTPUT_LEVEL =        "output_level";
    const std::string AUTO_ADJUST_OUTPUT =  "auto_adjust_output";
    const std::string SHOW_REPLAY_STRINGS = "show_replays";
    const std::string IGNORE_EMPTY =        "ignore_empty_lines";
    const std::string IGNORE_EXEC_HALT =    "ignore_exec_halt";
    const std::string STOP_FIRST_SOLUTION = "stop_first_solution";
    const std::string NUM_THREADS =         "num_threads";
    const std::string INDIVIDUAL_BATTLES =  "individual_battles";
    const std::string SKIP_CONTINUE = "skip_continue";

    const std::string T_SOLUTION_OUTPUT =   "solution";
    const std::string T_BASIC_OUTPUT =      "basic";
    const std::string T_DETAILED_OUPUT =    "detailed";
}; static const ParserTokens TOKENS;

struct Configuration {
    bool allowConfig = true;
    bool showReplayStrings = true;
    bool showQueries = true;
    bool ignoreEmptyLines = false;
    bool ignoreQuestions = false; //
    bool ignoreExecutionHalt = false;
    bool skipContinue = false;
    bool JSONOutput = false; //
    int firstDominance = ARMY_MAX_BRUTEFORCEABLE_SIZE;
    OutputLevel outputLevel = BASIC_OUTPUT;
    bool autoAdjustOutputLevel = true;
    bool individualBattles = false; //
    bool unlimitedWorldbossHealth = false; //
    bool stopFirstSolution = true; // false means continue to search for cheaper solution if non-zero cost solution was found
    int numThreads = 6;

    size_t branchwiseExpansionLimit = 20;
};
extern Configuration config;

class UserInterface {
    private:
        time_t lastTimedOutput = -1;
        std::ostringstream outputStream;

        void printBuffer(OutputLevel urgency);
        std::string getIndent(int indent);

    public:
        void outputMessage(std::string message, OutputLevel urgency, int indent = 0, bool linebreak = true);
        void timedOutput(std::string message, OutputLevel urgency, int indent = 0, bool reset = false);
        void suspendTimedOutputs(OutputLevel urgency);
        void resumeTimedOutputs(OutputLevel urgency);
        void finishTimedOutput(OutputLevel urgency);
        void haltExecution();

        std::vector<std::string> parseInput(std::string input);
};
extern UserInterface interface;

class InputFileManager {
    private:
        std::queue<std::vector<std::string>> inputLines;

    public:
        void init(std::string fileName);
        bool readFile(std::string fileName, bool important);

        bool checkLine(std::string token);
        std::vector<std::string> getLine();
        bool hasLine();
};

class IOManager {
    private:
        InputFileManager fileInput;

        void handleInputException(InputException e);

    public:
        void loadInputFiles(std::string fileName);
        void getConfiguration();
        bool askYesNoQuestion(std::string question, OutputLevel urgency, std::string defaultAnswer);
        std::vector<std::string> getResistantInput(std::string query, QueryType queryType = raw);
        std::vector<MonsterIndex> takeHerolevelInput();
        std::vector<Instance> takeInstanceInput(std::string promt);

        std::string getJSONError(InputException e);
};

bool shouldOutput(OutputLevel urgency);

// Convert a lineup string into an actual instance to solve
Instance makeInstanceFromString(std::string instanceString);

std::string makeStringFromInstance(Instance instance, bool valid, bool showReplayString = true);
std::string makeJSONFromInstance(Instance instance, bool valid);

// Parse string linup input into actual monsters. If there are heroes in the input, a leveled hero is added to the database
Army makeArmyFromStrings(std::vector<std::string> stringMonsters);

// Parse hero input from a string into its name and level
std::pair<Monster, int> parseHeroString(std::string heroString);

// Functions for making a valid ingame replay string
std::string makeBattleReplay(Army friendly, Army hostile);
std::string getReplaySetup(Army setup);
std::string getReplayHeroes(Army setup);

bool parseBool(std::string toParse);
int64_t parseInt(std::string toParse);
OutputLevel parseOutputLevel(std::string toParse);

// Splits strings into a vector of strings. No need to optimize, only used for input.
std::vector<std::string> split(std::string target, std::string separator);

// Convert a string to lowercase where available
std::string toLower(std::string input);

const char thousandSeparator = ',';
// display a large number with thousand separators
std::string numberWithSeparators(const uint64_t& largeNumber);

#endif
