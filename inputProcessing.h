#ifndef COSMOS_INPUT_HEADER
#define COSMOS_INPUT_HEADER

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "cosmosDefines.h"

using namespace std;

const string heroVersion = "2.5.1";

// Wait for user input before continuing. Used to stop program from colsing outside of a command line.
void haltExecution();

// Ask the user a question that they can answer via command line
bool askYesNoQuestion(string question);

// Output things on the command line. Using shouldOutput this can be easily controlled globally
void debugOutput(int timeStamp, string message, bool shouldOutput, bool finishLastOutput, bool finishLine);

// Promt the User via command line to input his hero levels and return them as a vector<int>
vector<int> takeHerolevelInput();

// Promt the user via command Line to input a monster lineup and return them as a vector of pointers to those monster
vector<Monster *> takeLineupInput(string promt);

// Parse string linup input into actual monsters if there are heroes in the input, a leveled hero is added to the database
vector<Monster *> makeMonstersFromStrings(vector<string> stringLineup);

// Parse hero input from a string into its name and level
pair<Monster, int> parseHeroString(string heroString);

// Splits strings into a vector of strings. No need to optimize, only used for input.
vector<string> split(string s, string to_split);

#endif