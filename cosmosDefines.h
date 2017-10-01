#ifndef COSMOS_DATA_HEADER
#define COSMOS_DATA_HEADER

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>

#include "cosmosClasses.h"

using namespace std;

extern vector<int8_t> availableHeroes; // Contains all User Heroes, readily Leveled

extern vector<int8_t> monsterList; // Contains pointers to raw Monster Data from a1 to f15, will be sorted by follower cost
extern map<string, int8_t> monsterMap; // Maps monster Names to their pointers (includes heroes)

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
	Monster(114, 110, 1403000, "a11", air),
	Monster(164,  88, 1733000, "a12", air),
	Monster(210,  94, 2772000, "a13", air),
	Monster(200, 142, 4785000, "a14", air),
	Monster(226, 190, 8897000, "a15", air),

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
	Monster(152,  79, 1315000, "w11", water),
	Monster(188,  78, 1775000, "w12", water),
	Monster(140, 128, 2398000, "w13", water),
	Monster(212, 122, 4159000, "w14", water),
	Monster(276, 142, 7758000, "w15", water),

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
	Monster(134,  81, 1130000, "e11", earth),
	Monster(128, 120, 1903000, "e12", earth),
	Monster(190, 132, 3971000, "e13", earth),
	Monster(244, 136, 6044000, "e14", earth),
	Monster(200, 186, 7173000, "e15", earth),

	Monster( 16,  10,    1000,  "f1", fire),
	Monster( 18,  16,    3900,  "f2", fire),
	Monster( 54,   8,    8000,  "f3", fire),
	Monster( 52,  16,   23000,  "f4", fire),
	Monster( 42,  24,   31000,  "f5", fire),
	Monster(104,  20,   94000,  "f6", fire),
	Monster( 54,  44,  115000,  "f7", fire),
	Monster( 94,  50,  321000,  "f8", fire),
	Monster(102,  58,  454000,  "f9", fire),
	Monster(104,  82,  787000, "f10", fire),
	Monster(164,  70, 1229000, "f11", fire),
	Monster(156,  92, 1718000, "f12", fire),
	Monster(166, 130, 3169000, "f13", fire),
	Monster(168, 168, 4741000, "f14", fire),
	Monster(234, 136, 5676000, "f15", fire)
};
static vector<Monster> baseHeroes { // Raw, unleveld Hero Data, holds actual Objects
	Monster( 50, 12, 0, "james",             earth, {pAoe,    all, 1}),

	Monster( 22, 14, 0, "hunter",            air,   {buff,    air, 2}),
	Monster( 40, 20, 0, "shaman",            earth, {protect, earth , 2}),
	Monster( 82, 22, 0, "alpha",             fire,  {aoe,     all, 1}),

	Monster( 28, 12, 0, "carl",              water, {buff,    water , 2}),
	Monster( 38, 22, 0, "nimue",             air,   {protect, air, 2}),
	Monster( 70, 26, 0, "athos",             earth, {protect, all, 2}),

	Monster( 24, 16, 0, "jet",               fire,  {buff,    fire, 2}),
	Monster( 36, 24, 0, "geron",             water, {protect, water, 2}),
	Monster( 46, 40, 0, "rei",               air,   {buff,    all, 2}),

	Monster( 19, 22, 0, "ailen",             earth, {buff,    earth, 2}),
	Monster( 50, 18, 0, "faefyr",            fire,  {protect, fire, 2}),
	Monster( 60, 32, 0, "auri",              water, {heal,    all, 2}),

	Monster( 28, 16, 0, "k41ry",             air,   {buff,    air, 3}),
	Monster( 46, 20, 0, "t4urus",            earth, {buff,    all, 1}),
	Monster(100, 20, 0, "tr0n1x",            fire,  {aoe,     all, 3}),

	Monster( 58,  8, 0, "aquortis",          water, {buff,    water, 3}),
	Monster( 30, 32, 0, "aeris",             air,   {heal,    all, 1}),
	Monster( 75,  2, 0, "geum",              earth, {berserk, self, 2}),

	Monster( 38, 12, 0, "rudean",            fire,  {buff,    fire, 3}),
	Monster( 18, 50, 0, "aural",             water, {berserk, self, 1.2}),
	Monster( 46, 46, 0, "geror",             air,   {friends, self, 1.2}),

	Monster( 30, 16, 0, "ourea",             earth, {buff,    earth, 3}),
	Monster( 48, 20, 0, "erebus",            fire,  {champion,fire, 2}),
	Monster( 62, 36, 0, "pontus",            water, {adapt,   self, 2}),

	Monster( 45, 20, 0, "ladyoftwilight",    air,   {protect, all, 1}),
	Monster( 70, 30, 0, "tiny",              earth, {aoe,     all, 2}),
	Monster( 90, 40, 0, "nebra",             fire,  {buff,    all, 8}),

	Monster( 66, 44, 0, "veildur",           earth, {champion,all, 3}),
	Monster( 72, 48, 0, "brynhildr",         air,   {champion,all, 4}),
	Monster( 78, 52, 0, "groth",             fire,  {champion,all, 5}),

	Monster( 20, 10, 0, "valor",             air,   {protect, air, 1}),
	Monster( 30,  8, 0, "rokka",             earth, {protect, earth, 1}),
	Monster( 24, 12, 0, "pyromancer",        fire,  {protect, fire, 1}),
	Monster( 50,  6, 0, "bewat",             water, {protect, water, 1}),

	Monster( 22, 32, 0, "nicte",             air,   {buff,    air, 4}),
	Monster( 46, 16, 0, "forestdruid",       earth, {buff,    earth, 4}),
	Monster( 32, 24, 0, "ignitor",           fire,  {buff,    fire, 4}),
	Monster( 58, 14, 0, "undine",            water, {buff,    water, 4}),

	Monster( 52, 20, 0, "chroma",            air,   {protect, air, 4}),
	Monster( 26, 44, 0, "petry",             earth, {protect, earth, 4}),
	Monster( 58, 22, 0, "zaytus",            fire,  {protect, fire, 4}),
};

static map<string, int> rarities { // hero rarities
	{"james", 2},  

	{"hunter", 0},
	{"shaman", 1},
	{"alpha", 2},

	{"carl", 0},
	{"nimue", 1},
	{"athos", 2},

	{"jet", 0},
	{"geron", 1},
	{"rei", 2},

	{"ailen", 0},
	{"faefyr", 1},
	{"auri", 2},

	{"k41ry", 0},
	{"t4urus", 1},
	{"tr0n1x", 2},

	{"aquortis", 0},
	{"aeris", 1},
	{"geum", 2},

	{"rudean", 0},
	{"aural", 1},
	{"geror", 2},

	{"ourea", 0},
	{"erebus", 1},
	{"pontus", 2},

	{"ladyoftwilight", 0},
	{"tiny", 1},
	{"nebra", 2},

	{"veildur", 2},
	{"brynhildr", 2},
	{"groth", 2},

	{"valor", 0},
	{"rokka", 0},
	{"pyromancer", 0},
	{"bewat", 0},

	{"nicte",  1},
	{"forestdruid", 1},
	{"ignitor", 1},
	{"undine",1},

	{"chroma", 1},
	{"petry", 1},
	{"zaytus", 1}
};

static vector<vector<string>> quests { // Contains all quest lineups for easy referencing
	{""},
	{"w5"},
	{"f1", "a1", "f1", "a1", "f1", "a1"},
	{"f5", "a5"},
	{"f2", "a2", "e2", "w2", "f3", "a3"},
	{"w3", "e3", "w3", "e3", "w3", "e3"},       //5
	{"w4", "e1", "a4", "f4", "w1", "e4"},
	{"f5", "a5", "f4", "a3", "f2", "a1"},
	{"e4", "w4", "w5", "e5", "w4", "e4"},
	{"w5", "f5", "e5", "a5", "w4", "f4"},
	{"w5", "e5", "a5", "f5", "e5", "w5"},       //10
	{"f5", "f6", "e5", "e6", "a5", "a6"},
	{"e5", "w5", "f5", "e6", "f6", "w6"},
	{"a8", "a7", "a6", "a5", "a4", "a3"},
	{"f7", "f6", "f5", "e7", "e6", "e6"},
	{"w5", "e6", "w6", "e8", "w8"},             //15
	{"a9", "f8", "a8"},
	{"w5", "e6", "w7", "e8", "w8"},
	{"f7", "f6", "a6", "f5", "a7", "a8"},
	{"e7", "w9", "f9", "e9"},
	{"f2", "a4", "f5", "a7", "f8", "a10"},      //20
	{"w10", "a10", "w10"},
	{"w9", "e10", "f10"},
	{"e9", "a9", "w8", "f8", "e8"},
	{"f6", "a7", "f7", "a8", "f8", "a9"},
	{"w8", "w7", "w8", "w8", "w7", "w8"},       //25
	{"a9", "w7", "w8", "e7", "e8", "f10"},
	{"e9", "f9", "w9", "f7", "w7", "w7"},
	{"a10", "a8", "a9", "a10", "a9"},
	{"a10", "w7", "f7", "e8", "a9", "a9"},
	{"e10", "e10", "e10", "f10"},               //30
	{"e9", "f10", "f9", "f9", "a10", "a7"},
	{"w1", "a9", "f10", "e9", "a10", "w10"},
	{"e9", "a9", "a9", "f9", "a9", "f10"},
	{"f8", "e9", "w9", "a9", "a10", "a10"},
	{"w8", "w8", "w10", "a10", "a10", "f10"},   //35
	{"a8", "a10", "f10", "a10", "a10", "a10"},
	{"e8", "a10", "e10", "f10", "f10", "e10"},
	{"f10", "e10", "w10", "a10", "w10", "w10"},
	{"w9", "a10", "w10", "e10", "a10", "a10"},
	{"w10", "a10", "w10", "a10", "w10", "a10"}, //40
	{"e12", "e11", "a11", "f11", "a12"},
	{"a11", "a11", "e11", "a11", "e11", "a11"},
	{"a8", "a11", "a10", "w10", "a12", "e12"},
	{"a10", "f10", "a12", "f10", "a10", "f12"},
	{"w4", "e11", "a12", "a12", "w11", "a12"},  //45
	{"a11", "a12", "a11", "f11", "a11", "f10"},
	{"f12", "w11", "e12", "a12", "w12"},
	{"a11", "a11", "e12", "a11", "a11", "a13"},
	{"a13", "f13", "f13", "f13"},
	{"f12", "f12", "f12", "f12", "f12", "f12"}, //50
	{"a11", "e11", "a13", "a11", "e11", "a13"},
	{"f13", "w13", "a13", "f12", "f12"},
	{"a9", "f13", "f13", "f12", "a12", "a12"},
	{"a13", "a13", "a12", "a12", "f11", "f12"},
	{"a11", "f10", "a11", "e14", "f13", "a11"}, //55
};

// Make sure all the values are set
void initMonsterData();

// Filter MonsterList by cost. User can specify if he wants to exclude cheap monsters
void filterMonsterData(int minimumMonsterCost);

// Initialize Hero Data
void initializeUserHeroes(vector<int> levels);

// Add a leveled hero to the databse 
int8_t addLeveledHero(Monster hero, int level);

// Create a new hero with leveled stats and return it
Monster getLeveledHero(const Monster & m, int rarity, int level);

#endif
