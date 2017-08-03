#ifndef COSMOS_DATA_HEADER
#define COSMOS_DATA_HEADER

#include <string>
#include <vector>
#include <map>

#include "cosmosClasses.h"

using namespace std;

extern int totalFightsSimulated;

// Define global variables used to track the best result
extern int followerUpperBound;
extern Army best;

extern vector<Monster> heroReference; // Will be filled with leveled heroes if needed (determined by input)

extern vector<Monster *> monsterList; // Contains pointers to raw Monster Data from a1 to f10, will be sorted by follower cost
extern map<string, Monster *> monsterMap; // Maps monster Names to their pointers (includes heroes)

static vector<Monster> monsterBaseList { // Raw Monster Data, holds the actual Objects
    Monster( 20,   8,    1000,  "a1", air),
    Monster( 48,   6,    3900,  "a2", air),
    Monster( 36,  12,    8000,  "a3", air),
    Monster( 24,  26,   15000,  "a4", air),
    Monster( 60,  20,   41000,  "a5", air),
    Monster( 62,  34,   96000,  "a6", air),
    Monster(106,  26,  144000,  "a7", air),
    Monster( 78,  52,  257000,  "a8", air),
    Monster(116,  54,  495000,  "a9", air),
    Monster(142,  60,  785000, "a10", air),

    Monster( 30,   6,    1400,  "w1", water),
    Monster( 24,  12,    3900,  "w2", water),
    Monster( 18,  24,    8000,  "w3", water),
    Monster( 36,  20,   18000,  "w4", water),
    Monster( 78,  18,   52000,  "w5", water),
    Monster( 44,  44,   84000,  "w6", water),
    Monster( 92,  32,  159000,  "w7", water),
    Monster(108,  36,  241000,  "w8", water),
    Monster( 80,  70,  418000,  "w9", water),
    Monster(110,  70,  675000, "w10", water),

    Monster( 44,   4,    1300,  "e1", earth),
    Monster( 30,   8,    2700,  "e2", earth),
    Monster( 26,  16,    7500,  "e3", earth),
    Monster( 72,  10,   18000,  "e4", earth),
    Monster( 36,  40,   54000,  "e5", earth),
    Monster( 72,  24,   71000,  "e6", earth),
    Monster( 66,  36,  115000,  "e7", earth),
    Monster( 60,  60,  215000,  "e8", earth),
    Monster(120,  48,  436000,  "e9", earth),
    Monster(122,  64,  689000, "e10", earth),

    Monster( 16,  10,    1000,  "f1", fire),
    Monster( 18,  16,    3900,  "f2", fire),
    Monster( 54,   8,    8000,  "f3", fire),
    Monster( 52,  16,   23000,  "f4", fire),
    Monster( 42,  24,   31000,  "f5", fire),
    Monster(104,  20,   94000,  "f6", fire),
    Monster( 54,  44,  115000,  "f7", fire),
    Monster( 94,  50,  321000,  "f8", fire),
    Monster(102,  58,  454000,  "f9", fire),
    Monster(104,  82,  787000, "f10", fire)
};
static vector<Monster> baseHeroes { // Raw, unleveld Hero Data, holds actual Objects
    Monster( 45, 20, 0, "lady of twilight",  air,   {protect, all, air, 1}),
    Monster( 70, 30, 0, "tiny",              earth, {aoe,     all, earth, 2}),
    Monster( 90, 40, 0, "nebra",             fire,  {buff,    all, fire, 4}),
    Monster( 50, 12, 0, "james",             earth, {pAoe,    all, earth, 1}),
    Monster( 22, 14, 0, "hunter",            air,   {buff,    air, air, 2}),
    Monster( 40, 20, 0, "shaman",            earth, {protect, earth, earth , 2}),
    Monster( 82, 22, 0, "alpha",             fire,  {aoe,     all, fire, 1}),
    Monster( 28, 12, 0, "carl",              water, {buff,    water, water , 2}),
    Monster( 38, 22, 0, "nimue",             air,   {protect, air, air, 2}),
    Monster( 70, 26, 0, "athos",             earth, {protect, all, earth, 2}),
    Monster( 24, 16, 0, "jet",               fire,  {buff,    fire, fire, 2}),
    Monster( 36, 24, 0, "geron",             water, {protect, water, water, 2}),
    Monster( 46, 40, 0, "rei",               air,   {buff,    all, air, 2}),
    Monster( 19, 22, 0, "ailen",             earth, {buff,    earth, earth, 2}),
    Monster( 50, 18, 0, "faefyr",            fire,  {protect, fire, fire, 2}),
    Monster( 60, 32, 0, "auri",              water, {heal,    all, water, 2}),
    Monster( 28, 16, 0, "k41ry",             air,   {protect, air, air, 3}),
    Monster( 46, 20, 0, "t4urus",            earth, {buff,    all, earth, 1}),
    Monster(100, 20, 0, "tr0n1x",            fire,  {aoe,     all, fire, 3}),
    Monster( 58,  8, 0, "aquortis",          water, {protect, water, water, 3}),
    Monster( 30, 32, 0, "aeris",             air,   {heal,    all, air, 1}),
    Monster( 75,  2, 0, "geum",              earth, {berserk, self, earth, 2}),
    Monster( 20, 10, 0, "valor",             air,   {protect, air, air, 1}),
    Monster( 30,  8, 0, "rokka",             earth, {protect, earth, earth, 1}),
    Monster( 24, 12, 0, "pyromancer",        fire,  {protect, fire, fire, 1}),
    Monster( 50,  6, 0, "bewat",             water, {protect, water, water, 1}),
    Monster( 22, 32, 0, "nicte",             air,   {buff,    air, air, 4}),
};

static map<string, int> rarities { // hero rarities
    {"lady of twilight",  0},
    {"tiny",  1},
    {"nebra",  2},
    {"james",  2},    
    {"hunter",  0},
    {"shaman",  1},
    {"alpha",  2},
    {"carl",  0},
    {"nimue",  1},
    {"athos",  2},
    {"jet",  0},
    {"geron",  1},
    {"rei",  2},
    {"ailen",  0},
    {"faefyr",  1},
    {"auri",  2},
    {"k41ry", 0},
    {"t4urus", 1},
    {"tr0n1x", 2},
    {"aquortis", 0},
    {"aeris", 1},
    {"geum", 2},
    {"valor",  0},
    {"rokka",  0},
    {"pyromancer",  0},
    {"bewat",  0},
    {"nicte",  1},
};

static vector<vector<string>> quests { // Contains all quest lineups for easy referencing
	{""},
	{"w5"},
	{"f1", "a1", "f1", "a1", "f1", "a1"},
	{"f5", "a5"},
	{"f2", "a2", "e2", "w2", "f3", "a3"},
	{"w3", "e3", "w3", "e3", "w3", "e3"},   //5
	{"w4", "e1", "a4", "f4", "w1", "e4"},
	{"f5", "a5", "f4", "a3", "f2", "a1"},
	{"e4", "w4", "w5", "e5", "w4", "e4"},
	{"w5", "f5", "e5", "a5", "w4", "f4"},
	{"w5", "e5", "a5", "f5", "e5", "w5"},   //10
	{"f5", "f6", "e5", "e6", "a5", "a6"},
	{"e5", "w5", "f5", "e6", "f6", "w6"},
	{"a8", "a7", "a6", "a5", "a4", "a3"},
	{"f7", "f6", "f5", "e7", "e6", "e6"},
	{"w5", "e6", "w6", "e8", "w8"},         //15
	{"a9", "f8", "a8"},
	{"w5", "e6", "w7", "e8", "w8"},
	{"f7", "f6", "a6", "f5", "a7", "a8"},
	{"e7", "w9", "f9", "e9"},
	{"f2", "a4", "f5", "a7", "f8", "a10"},  //20
	{"w10", "a10", "w10"},
	{"w9", "e10", "f10"},
	{"e9", "a9", "w8", "f8", "e8"},
	{"f6", "a7", "f7", "a8", "f8", "a9"},
	{"w8", "w7", "w8", "w8", "w7", "w8"},   //25
};

#endif
