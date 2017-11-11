#ifndef COSMOS_DATA_HEADER
#define COSMOS_DATA_HEADER

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "cosmosClasses.h"

extern std::map<std::string, int8_t> monsterMap; // // Maps monster Names to their indices in monsterReference from cosmosClasses

extern std::vector<int8_t> availableMonsters; // Contains indices of raw Monster Data from a1 to f15, will be sorted by follower cost
extern std::vector<int8_t> availableHeroes; // Contains all user heroes' indices 

const int INDEX_NO_MONSTER = -1;

static const std::vector<Monster> monsterBaseList { // Raw Monster Data, holds the actual Objects
    Monster( 20,   8,    1000,  "a1", AIR),
    Monster( 44,   4,    1300,  "e1", EARTH),
    Monster( 16,  10,    1000,  "f1", FIRE),
    Monster( 30,   6,    1400,  "w1", WATER),
    
    Monster( 48,   6,    3900,  "a2", AIR),
    Monster( 30,   8,    2700,  "e2", EARTH),
    Monster( 18,  16,    3900,  "f2", FIRE),
    Monster( 24,  12,    3900,  "w2", WATER),
    
    Monster( 36,  12,    8000,  "a3", AIR),
    Monster( 26,  16,    7500,  "e3", EARTH),
    Monster( 54,   8,    8000,  "f3", FIRE),
    Monster( 18,  24,    8000,  "w3", WATER),
    
    Monster( 24,  26,   15000,  "a4", AIR),
    Monster( 72,  10,   18000,  "e4", EARTH),
    Monster( 52,  16,   23000,  "f4", FIRE),
    Monster( 36,  20,   18000,  "w4", WATER),
    
    Monster( 60,  20,   41000,  "a5", AIR),
    Monster( 36,  40,   54000,  "e5", EARTH),
    Monster( 42,  24,   31000,  "f5", FIRE),
    Monster( 78,  18,   52000,  "w5", WATER),
    
    Monster( 62,  34,   96000,  "a6", AIR),
    Monster( 72,  24,   71000,  "e6", EARTH),
    Monster(104,  20,   94000,  "f6", FIRE),
    Monster( 44,  44,   84000,  "w6", WATER),
    
    Monster(106,  26,  144000,  "a7", AIR),
    Monster( 66,  36,  115000,  "e7", EARTH),
    Monster( 54,  44,  115000,  "f7", FIRE),
    Monster( 92,  32,  159000,  "w7", WATER),
    
    Monster( 78,  52,  257000,  "a8", AIR),
    Monster( 60,  60,  215000,  "e8", EARTH),
    Monster( 94,  50,  321000,  "f8", FIRE),
    Monster(108,  36,  241000,  "w8", WATER),
    
    Monster(116,  54,  495000,  "a9", AIR),
    Monster(120,  48,  436000,  "e9", EARTH),
    Monster(102,  58,  454000,  "f9", FIRE),
    Monster( 80,  70,  418000,  "w9", WATER),
    
    Monster(142,  60,  785000, "a10", AIR),
    Monster(122,  64,  689000, "e10", EARTH),
    Monster(104,  82,  787000, "f10", FIRE),
    Monster(110,  70,  675000, "w10", WATER),
    
    Monster(114, 110, 1403000, "a11", AIR),
    Monster(134,  81, 1130000, "e11", EARTH),
    Monster(164,  70, 1229000, "f11", FIRE),
    Monster(152,  79, 1315000, "w11", WATER),
    
    Monster(164,  88, 1733000, "a12", AIR),
    Monster(128, 120, 1903000, "e12", EARTH),
    Monster(156,  92, 1718000, "f12", FIRE),
    Monster(188,  78, 1775000, "w12", WATER),
    
    Monster(210,  94, 2772000, "a13", AIR),
    Monster(190, 132, 3971000, "e13", EARTH),
    Monster(166, 130, 3169000, "f13", FIRE),
    Monster(140, 128, 2398000, "w13", WATER),
    
    Monster(200, 142, 4785000, "a14", AIR),
    Monster(244, 136, 6044000, "e14", EARTH),
    Monster(168, 168, 4741000, "f14", FIRE),
    Monster(212, 122, 4159000, "w14", WATER),
    
    Monster(226, 190, 8897000, "a15", AIR),
    Monster(200, 186, 7173000, "e15", EARTH),
    Monster(234, 136, 5676000, "f15", FIRE),
    Monster(276, 142, 7758000, "w15", WATER)
};

static const std::vector<Monster> baseHeroes { // Raw, unleveld Hero Data, holds actual Objects
    Monster( 45, 20, "ladyoftwilight",    AIR,   COMMON,    {PROTECT,       ALL, AIR, 1}),
    Monster( 70, 30, "tiny",              EARTH, RARE,      {AOE,           ALL, EARTH, 2}),
    Monster( 90, 40, "nebra",             FIRE,  LEGENDARY, {BUFF,          ALL, FIRE, 8}),
 
    Monster( 20, 10, "valor",             AIR,   COMMON,    {PROTECT,       AIR, AIR, 1}),
    Monster( 30,  8, "rokka",             EARTH, COMMON,    {PROTECT,       EARTH, EARTH, 1}),
    Monster( 24, 12, "pyromancer",        FIRE,  COMMON,    {PROTECT,       FIRE, FIRE, 1}),
    Monster( 50,  6, "bewat",             WATER, COMMON,    {PROTECT,       WATER, WATER, 1}),
   
    Monster( 22, 14, "hunter",            AIR,   COMMON,    {BUFF,          AIR, AIR, 2}),
    Monster( 40, 20, "shaman",            EARTH, RARE,      {PROTECT,       EARTH, EARTH , 2}),
    Monster( 82, 22, "alpha",             FIRE,  LEGENDARY, {AOE,           ALL, FIRE, 1}),
    
    Monster( 28, 12, "carl",              WATER, COMMON,    {BUFF,          WATER, WATER , 2}),
    Monster( 38, 22, "nimue",             AIR,   RARE,      {PROTECT,       AIR, AIR, 2}),
    Monster( 70, 26, "athos",             EARTH, LEGENDARY, {PROTECT,       ALL, EARTH, 2}),
    
    Monster( 24, 16, "jet",               FIRE,  COMMON,    {BUFF,          FIRE, FIRE, 2}),
    Monster( 36, 24, "geron",             WATER, RARE,      {PROTECT,       WATER, WATER, 2}),
    Monster( 46, 40, "rei",               AIR,   LEGENDARY, {BUFF,          ALL, AIR, 2}),
    
    Monster( 19, 22, "ailen",             EARTH, COMMON,    {BUFF,          EARTH, EARTH, 2}),
    Monster( 50, 18, "faefyr",            FIRE,  RARE,      {PROTECT,       FIRE, FIRE, 2}),
    Monster( 60, 32, "auri",              WATER, LEGENDARY, {HEAL,          ALL, WATER, 2}),
    
    Monster( 22, 32, "nicte",             AIR,   RARE,      {BUFF,          AIR, AIR, 4}),
   
    Monster( 50, 12, "james",             EARTH, LEGENDARY, {P_AOE,          ALL, EARTH, 1}),
   
    Monster( 28, 16, "k41ry",             AIR,   COMMON,    {BUFF,          AIR, AIR, 3}),
    Monster( 46, 20, "t4urus",            EARTH, RARE,      {BUFF,          ALL, EARTH, 1}),
    Monster(100, 20, "tr0n1x",            FIRE,  LEGENDARY, {AOE,           ALL, FIRE, 3}),
       
    Monster( 58,  8, "aquortis",          WATER, COMMON,    {BUFF,          WATER, WATER, 3}),
    Monster( 30, 32, "aeris",             AIR,   RARE,      {HEAL,          ALL, AIR, 1}),
    Monster( 75,  2, "geum",              EARTH, LEGENDARY, {BERSERK,       SELF, EARTH, 2}),
    
    Monster( 46, 16, "forestdruid",       EARTH, RARE,      {BUFF,          EARTH, EARTH, 4}),
    Monster( 32, 24, "ignitor",           FIRE,  RARE,      {BUFF,          FIRE, FIRE, 4}),
    Monster( 58, 14, "undine",            WATER, RARE,      {BUFF,          WATER, WATER, 4}),
    
    Monster( 38, 12, "rudean",            FIRE,  COMMON,    {BUFF,          FIRE, FIRE, 3}),
    Monster( 18, 50, "aural",             WATER, RARE,      {BERSERK,       SELF, WATER, 1.2f}),
    Monster( 46, 46, "geror",             AIR,   LEGENDARY, {FRIENDS,       SELF, AIR, 1.2f}),
    
    Monster( 66, 44, "veildur",           EARTH, LEGENDARY, {CHAMPION,      ALL, EARTH, 3}),
    Monster( 72, 48, "brynhildr",         AIR,   LEGENDARY, {CHAMPION,      ALL, AIR, 4}),
    Monster( 78, 52, "groth",             FIRE,  LEGENDARY, {CHAMPION,      ALL, FIRE, 5}),
    
    Monster( 30, 16, "ourea",             EARTH, COMMON,    {BUFF,          EARTH, EARTH, 3}),
    Monster( 48, 20, "erebus",            FIRE,  RARE,      {CHAMPION,      FIRE, FIRE, 2}),
    Monster( 62, 36, "pontus",            WATER, LEGENDARY, {ADAPT,         WATER, WATER, 2}),

    Monster( 52, 20, "chroma",            AIR,   RARE,      {PROTECT,       AIR, AIR, 4}),
    Monster( 26, 44, "petry",             EARTH, RARE,      {PROTECT,       EARTH, EARTH, 4}),
    Monster( 58, 22, "zaytus",            FIRE,  RARE,      {PROTECT,       FIRE, FIRE, 4}),

    Monster( 75, 45, "spyke",             AIR,   LEGENDARY, {TRAINING,      SELF, AIR, 5}),
    Monster( 70, 55, "aoyuki",            WATER, LEGENDARY, {RAINBOW,       SELF, WATER, 50}),
    Monster( 50,100, "gaiabyte",          EARTH, LEGENDARY, {WITHER,        SELF, EARTH, 0.5f}),
    
    Monster( 36, 14, "oymos",             AIR,   COMMON,    {BUFF,          AIR, AIR, 4}),
    Monster( 32, 32, "xarth",             EARTH, RARE,      {CHAMPION,      EARTH, EARTH, 2}),
    Monster( 76, 32, "atzar",             FIRE,  LEGENDARY, {ADAPT,         FIRE, FIRE, 2}),
    
    Monster( 70, 42, "zeth",              WATER, LEGENDARY, {REVENGE,       ALL, WATER, 0.1f}),
    Monster( 76, 46, "koth",              EARTH, LEGENDARY, {REVENGE,       ALL, EARTH, 0.15f}),
    Monster( 82, 50, "gurth",             AIR,   LEGENDARY, {REVENGE,       ALL, AIR, 0.2f}),
    
    Monster( 35, 25, "werewolf",          EARTH, COMMON,    {PROTECT_L,     ALL, EARTH, 9}),
    Monster( 55, 35, "jackoknight",       AIR,   RARE,      {BUFF_L,        ALL, AIR, 9}),
    Monster( 75, 45, "dullahan",          FIRE,  LEGENDARY, {CHAMPION_L,    ALL, FIRE, 9}),
};

static const std::vector<std::vector<std::string>> quests { // Contains all quest lineups for easy referencing
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

// Clean up all monster related vectors and sort the monsterBaseList
// Also fills the map used to parse strings into monsters
// Must be called before any input can be processed
void initMonsterData();

// Filter MonsterList by cost. User can specify if he wants to exclude cheap monsters
void filterMonsterData(int minimumMonsterCost);

// Add a leveled hero to the databse and return its corresponding index
int8_t addLeveledHero(Monster & hero, int level);

#endif
