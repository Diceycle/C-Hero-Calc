#include "cosmosData.h"

// Private constructor that is called by all public ones. Fully initializes all attributes
Monster::Monster(int someHp, int someDamage, FollowerCount aCost, std::string aName, Element anElement, HeroRarity aRarity, HeroSkill aSkill, int aLevel) : 
    hp(someHp),
    damage(someDamage),
    cost(aCost),
    baseName(aName),
    element(anElement),
    rarity(aRarity),
    skill(aSkill),
    level(aLevel),
    name(aName)
{
    if (this->rarity != NO_HERO) {
        this->name = this->baseName + HEROLEVEL_SEPARATOR + std::to_string(this->level);
        if (this->rarity == WORLDBOSS) {
            int bonus = 0;
            do {
                aLevel -= ++bonus;
            } while (aLevel > 0);
            this->damage += bonus-1;
        } else {
            int points = this->rarity * (this->level-1);
            int value = this->hp + this->damage;
            this->hp = this->hp + (int) round((float) points * (float) this->hp / (float) value);
            this->damage = this->damage + (int) round((float) points * (float) this->damage / (float) value);
        }
    }
}

// Contructor for normal Monsters
Monster::Monster(int someHp, int someDamage, FollowerCount aCost, std::string aName, Element anElement) : 
    Monster(someHp, someDamage, aCost, aName, anElement, NO_HERO, NO_SKILL, 0) {}

// Constructor for Heroes
Monster::Monster(int someHp, int someDamage, std::string aName, Element anElement, HeroRarity aRarity, HeroSkill aSkill) :
    Monster(someHp, someDamage, 0, aName, anElement, aRarity, aSkill, 1) {}

// Constructor for leveled heroes
Monster::Monster(const Monster & baseHero, int aLevel) :
    Monster(baseHero.hp, baseHero.damage, baseHero.cost, baseHero.baseName, baseHero.element, baseHero.rarity, baseHero.skill, aLevel) 
{
    if (baseHero.skill.skillType == BUFF_L) {
        this->skill.skillType = BUFF;
        this->skill.amount = (float) floor((float) aLevel * baseHero.skill.amount);
    } else if (baseHero.skill.skillType == PROTECT_L) {
        this->skill.skillType = PROTECT;
        this->skill.amount = (float) floor((float) aLevel * baseHero.skill.amount);
    } else if (baseHero.skill.skillType == CHAMPION_L) {
        this->skill.skillType = CHAMPION;
        this->skill.amount = (float) floor((float) aLevel * baseHero.skill.amount);
    } else if (baseHero.skill.skillType == HEAL_L) {
        this->skill.skillType = HEAL;
        this->skill.amount = (float) floor((float) aLevel * baseHero.skill.amount);
    } else if (baseHero.skill.skillType == AOE_L) {
        this->skill.skillType = AOE;
        this->skill.amount = (float) floor((float) aLevel * baseHero.skill.amount);
    } else if (baseHero.skill.skillType == LIFESTEAL_L) {
        this->skill.skillType = LIFESTEAL;
        this->skill.amount = (float) floor((float) aLevel * baseHero.skill.amount);
    } else if (baseHero.skill.skillType == DAMPEN_L) {
        this->skill.skillType = DAMPEN;
        this->skill.amount = 1.0f - (float) aLevel * baseHero.skill.amount;
    }
}

HeroSkill::HeroSkill(SkillType aType, Element aTarget, Element aSource, float anAmount) :
    skillType(aType), 
    target(aTarget), 
    sourceElement(aSource), 
    amount(anAmount) 
{
    this->hasAsymmetricAoe = (aType == VALKYRIE);
    this->hasHeal = (aType == HEAL || aType == HEAL_L || 
                     aType == LIFESTEAL || aType == LIFESTEAL_L);
    this->hasAoe = (aType == AOE || aType == AOE_L || 
                    aType == REVENGE || aType == PIERCE ||
                    this->hasHeal || this->hasAsymmetricAoe);
    this->violatesFightResults = (aType == BUFF || aType == BUFF_L || 
                                  aType == PROTECT || aType == PROTECT_L ||
                                  aType == CHAMPION || aType == CHAMPION_L ||
                                  aType == AOE || aType == AOE_L || 
                                  aType == HEAL || aType == HEAL_L ||
                                  aType == LIFESTEAL || aType == LIFESTEAL_L);
}

std::string Monster::toJSON() {
    std::stringstream s;
    s << "{";
        s << "\"id\""  << ":" << getRealIndex(*this);
        if (this->rarity != NO_HERO) {
            s << ",";
            s << "\"level\""  << ":" << this->level;
        }
    s << "}";
    return s.str();
}

std::string Army::toString() {
    std::stringstream s;
    s << "[";
    int index = isQuest(*this);
    
    if (index != -1) {
        s << "quest" << index << " | ";
    }
    s << "Followers: " << std::setw(7) << this->followerCost << " | ";
    for (int i = this->monsterAmount-1; i >= 0; i--) {
        s << monsterReference[this->monsters[i]].name << " "; // Print in reversed Order
    } s << "<==]"; 
    return s.str();
}

std::string Army::toJSON() {
    if (this->isEmpty()) {return "null";}
    
    std::stringstream s;
    s << "{";
        s << "\"followers\"" << ":" << this->followerCost << ",";
        s << "\"monsters\"" << ":";
        s << "[";
        for (int i = this->monsterAmount-1; i >= 0; i--) {
            s << monsterReference[this->monsters[i]].toJSON();
            if (i > 0) {
                s << ",";
            }
        }
        s << "]";
    s << "}";
    return s.str();
}

void Instance::setTarget(Army aTarget) {
    this->target = aTarget;
    this->targetSize = aTarget.monsterAmount;
    this->lowestBossHealth = -1;
    
    HeroSkill currentSkill;
    this->hasAoe = false;
    this->hasHeal = false;
    this->hasAsymmetricAoe = false;
    this->hasWorldBoss = false;
    for (size_t i = 0; i < this->targetSize; i++) {
        currentSkill = monsterReference[this->target.monsters[i]].skill;
        this->hasAoe |= currentSkill.hasAoe;
        this->hasHeal |= currentSkill.hasHeal;
        this->hasAsymmetricAoe |= currentSkill.hasAsymmetricAoe;
        this->hasWorldBoss |= monsterReference[this->target.monsters[i]].rarity == WORLDBOSS;
    }
    
    // Check which monsters can survive a hit from the final monster on the target. This helps reduce the amoutn of potential soluitons in the last expand
    // Heroes with global Abilities also get accepted. 
    // This produces only false positives not false negatives -> no correct solutions lost
    Monster lastMonster = monsterReference[this->target.monsters[this->targetSize - 1]];
    for (size_t i = 0; i < monsterReference.size(); i++) {
        Monster currentMonster = monsterReference[i];
        int attack = lastMonster.damage;
        if (counter[currentMonster.element] == lastMonster.element) {
            attack = castCeil((float) attack * elementalBoost);
        }
        this->monsterUsefulLast.push_back(currentMonster.hp > attack || currentMonster.skill.violatesFightResults);
    }
}

// Returns the index of a quest if the lineup is the same. Returns -1 if not a quest
int isQuest(Army & army) {
    bool match;
    for (size_t i = 0; i < quests.size(); i++) {
        match = false;
        if ((int) quests[i].size() == army.monsterAmount) {
            match = true;
            for (int j = 0; j < army.monsterAmount; j++) {
                if (quests[i][j] != monsterReference[army.monsters[j]].name) {
                    match = false;
                }
            }
        }
        if (match) {
            return (int) i;
        }
    }
    return -1;
}

// Access tools for monsters 
std::map<std::string, MonsterIndex> monsterMap; // Maps monster Names to their indices in monsterReference
std::vector<Monster> monsterReference; // Global lookup for monster stats indices of monsters here can be used instead of the objects
std::vector<MonsterIndex> availableMonsters; // Contains indices of all monsters the user allows. Is affected by filters
std::vector<MonsterIndex> availableHeroes; // Contains all user heroes' indices 

// Storage for Game Data
std::vector<Monster> monsterBaseList; // Raw Monster Data, holds the actual Objects
std::vector<Monster> baseHeroes; // Raw, unleveld Hero Data, holds actual Objects
std::vector<std::vector<std::string>> quests; // Quest Lineup from the game

void initMonsterData() {
    monsterBaseList.push_back(Monster( 20,   8,      1000,  "a1", AIR));
    monsterBaseList.push_back(Monster( 44,   4,      1300,  "e1", EARTH));
    monsterBaseList.push_back(Monster( 16,  10,      1000,  "f1", FIRE));
    monsterBaseList.push_back(Monster( 30,   6,      1400,  "w1", WATER));
                                                  
    monsterBaseList.push_back(Monster( 48,   6,      3900,  "a2", AIR));
    monsterBaseList.push_back(Monster( 30,   8,      2700,  "e2", EARTH));
    monsterBaseList.push_back(Monster( 18,  16,      3900,  "f2", FIRE));
    monsterBaseList.push_back(Monster( 24,  12,      3900,  "w2", WATER));
                                                  
    monsterBaseList.push_back(Monster( 36,  12,      8000,  "a3", AIR));
    monsterBaseList.push_back(Monster( 26,  16,      7500,  "e3", EARTH));
    monsterBaseList.push_back(Monster( 54,   8,      8000,  "f3", FIRE));
    monsterBaseList.push_back(Monster( 18,  24,      8000,  "w3", WATER));
                                                  
    monsterBaseList.push_back(Monster( 24,  26,     15000,  "a4", AIR));
    monsterBaseList.push_back(Monster( 72,  10,     18000,  "e4", EARTH));
    monsterBaseList.push_back(Monster( 52,  16,     23000,  "f4", FIRE));
    monsterBaseList.push_back(Monster( 36,  20,     18000,  "w4", WATER));
                                                  
    monsterBaseList.push_back(Monster( 60,  20,     41000,  "a5", AIR));
    monsterBaseList.push_back(Monster( 36,  40,     54000,  "e5", EARTH));
    monsterBaseList.push_back(Monster( 42,  24,     31000,  "f5", FIRE));
    monsterBaseList.push_back(Monster( 78,  18,     52000,  "w5", WATER));
                                                  
    monsterBaseList.push_back(Monster( 62,  34,     96000,  "a6", AIR));
    monsterBaseList.push_back(Monster( 72,  24,     71000,  "e6", EARTH));
    monsterBaseList.push_back(Monster(104,  20,     94000,  "f6", FIRE));
    monsterBaseList.push_back(Monster( 44,  44,     84000,  "w6", WATER));
                                                  
    monsterBaseList.push_back(Monster(106,  26,    144000,  "a7", AIR));
    monsterBaseList.push_back(Monster( 66,  36,    115000,  "e7", EARTH));
    monsterBaseList.push_back(Monster( 54,  44,    115000,  "f7", FIRE));
    monsterBaseList.push_back(Monster( 92,  32,    159000,  "w7", WATER));
                                                  
    monsterBaseList.push_back(Monster( 78,  52,    257000,  "a8", AIR));
    monsterBaseList.push_back(Monster( 60,  60,    215000,  "e8", EARTH));
    monsterBaseList.push_back(Monster( 94,  50,    321000,  "f8", FIRE));
    monsterBaseList.push_back(Monster(108,  36,    241000,  "w8", WATER));
                                                  
    monsterBaseList.push_back(Monster(116,  54,    495000,  "a9", AIR));
    monsterBaseList.push_back(Monster(120,  48,    436000,  "e9", EARTH));
    monsterBaseList.push_back(Monster(102,  58,    454000,  "f9", FIRE));
    monsterBaseList.push_back(Monster( 80,  70,    418000,  "w9", WATER));
                                                  
    monsterBaseList.push_back(Monster(142,  60,    785000, "a10", AIR));
    monsterBaseList.push_back(Monster(122,  64,    689000, "e10", EARTH));
    monsterBaseList.push_back(Monster(104,  82,    787000, "f10", FIRE));
    monsterBaseList.push_back(Monster(110,  70,    675000, "w10", WATER));
                                                  
    monsterBaseList.push_back(Monster(114, 110,   1403000, "a11", AIR));
    monsterBaseList.push_back(Monster(134,  81,   1130000, "e11", EARTH));
    monsterBaseList.push_back(Monster(164,  70,   1229000, "f11", FIRE));
    monsterBaseList.push_back(Monster(152,  79,   1315000, "w11", WATER));
                                                  
    monsterBaseList.push_back(Monster(164,  88,   1733000, "a12", AIR));
    monsterBaseList.push_back(Monster(128, 120,   1903000, "e12", EARTH));
    monsterBaseList.push_back(Monster(156,  92,   1718000, "f12", FIRE));
    monsterBaseList.push_back(Monster(188,  78,   1775000, "w12", WATER));
                                                  
    monsterBaseList.push_back(Monster(210,  94,   2772000, "a13", AIR));
    monsterBaseList.push_back(Monster(190, 132,   3971000, "e13", EARTH));
    monsterBaseList.push_back(Monster(166, 130,   3169000, "f13", FIRE));
    monsterBaseList.push_back(Monster(140, 128,   2398000, "w13", WATER));
                                                  
    monsterBaseList.push_back(Monster(200, 142,   4785000, "a14", AIR));
    monsterBaseList.push_back(Monster(244, 136,   6044000, "e14", EARTH));
    monsterBaseList.push_back(Monster(168, 168,   4741000, "f14", FIRE));
    monsterBaseList.push_back(Monster(212, 122,   4159000, "w14", WATER));
                                                  
    monsterBaseList.push_back(Monster(226, 190,   8897000, "a15", AIR));
    monsterBaseList.push_back(Monster(200, 186,   7173000, "e15", EARTH));
    monsterBaseList.push_back(Monster(234, 136,   5676000, "f15", FIRE));
    monsterBaseList.push_back(Monster(276, 142,   7758000, "w15", WATER));
    
    monsterBaseList.push_back(Monster(280, 196,  12855000, "a16", AIR));
    monsterBaseList.push_back(Monster(284, 190,  12534000, "e16", EARTH));
    monsterBaseList.push_back(Monster(288, 192,  13001000, "f16", FIRE));
    monsterBaseList.push_back(Monster(286, 198,  13475000, "w16", WATER));	
                                                 
    monsterBaseList.push_back(Monster(318, 206,  16765000, "a17", AIR));
    monsterBaseList.push_back(Monster(338, 192,  16531000, "e17", EARTH));
    monsterBaseList.push_back(Monster(236, 292,  18090000, "f17", FIRE));
    monsterBaseList.push_back(Monster(262, 258,  17573000, "w17", WATER));
                                                 
    monsterBaseList.push_back(Monster(280, 280,  21951000, "a18", AIR));
    monsterBaseList.push_back(Monster(330, 242,  22567000, "e18", EARTH));
    monsterBaseList.push_back(Monster(392, 200,  21951000, "f18", FIRE));
    monsterBaseList.push_back(Monster(330, 230,  20909000, "w18", WATER));
                                                 
    monsterBaseList.push_back(Monster(440, 206,  27288000, "a19", AIR));
    monsterBaseList.push_back(Monster(320, 282,  27107000, "e19", EARTH));
    monsterBaseList.push_back(Monster(352, 244,  25170000, "f19", FIRE));
    monsterBaseList.push_back(Monster(360, 238,  25079000, "w19", WATER));
                                                 
    monsterBaseList.push_back(Monster(378, 268,  32242000, "a20", AIR));
    monsterBaseList.push_back(Monster(382, 264,  32025000, "e20", EARTH));
    monsterBaseList.push_back(Monster(388, 266,  33155000, "f20", FIRE));
    monsterBaseList.push_back(Monster(454, 232,  34182000, "w20", WATER));
                                                 
    monsterBaseList.push_back(Monster(428, 286,  42826000, "a21", AIR));
    monsterBaseList.push_back(Monster(446, 272,  42252000, "e21", EARTH));
    monsterBaseList.push_back(Monster(362, 338,  42798000, "f21", FIRE));
    monsterBaseList.push_back(Monster(416, 290,  41901000, "w21", WATER));
                                                 
    monsterBaseList.push_back(Monster(454, 320,  55373000, "a22", AIR));
    monsterBaseList.push_back(Monster(450, 324,  55671000, "e22", EARTH));
    monsterBaseList.push_back(Monster(458, 318,  55582000, "f22", FIRE));
    monsterBaseList.push_back(Monster(440, 340,  55877000, "w22", WATER));
                                                 
    monsterBaseList.push_back(Monster(500, 348,  72580000, "a23", AIR));
    monsterBaseList.push_back(Monster(516, 340,  73483000, "e23", EARTH));
    monsterBaseList.push_back(Monster(424, 410,  72480000, "f23", FIRE));
    monsterBaseList.push_back(Monster(490, 354,  72243000, "w23", WATER));
                                                 
    monsterBaseList.push_back(Monster(554, 374,  94312000, "a24", AIR));
    monsterBaseList.push_back(Monster(458, 458,  96071000, "e24", EARTH));
    monsterBaseList.push_back(Monster(534, 392,  95772000, "f24", FIRE));
    monsterBaseList.push_back(Monster(540, 388,  95903000, "w24", WATER));

    monsterBaseList.push_back(Monster(580, 430, 124000000, "a25", AIR));
    monsterBaseList.push_back(Monster(592, 418, 123000000, "e25", EARTH));
    monsterBaseList.push_back(Monster(764, 328, 125000000, "f25", FIRE));
    monsterBaseList.push_back(Monster(500, 506, 127000000, "w25", WATER));

    monsterBaseList.push_back(Monster(496, 506, 155000000, "a26", AIR));
    monsterBaseList.push_back(Monster(622, 468, 157000000, "e26", EARTH));
    monsterBaseList.push_back(Monster(638, 462, 160000000, "f26", FIRE));
    monsterBaseList.push_back(Monster(700, 416, 156000000, "w26", WATER));

    monsterBaseList.push_back(Monster(712, 484, 202000000, "a27", AIR));
    monsterBaseList.push_back(Monster(580, 602, 206000000, "e27", EARTH));
    monsterBaseList.push_back(Monster(690, 498, 201000000, "f27", FIRE));
    monsterBaseList.push_back(Monster(682, 500, 199000000, "w27", WATER));

    monsterBaseList.push_back(Monster(644, 642, 265000000, "a28", AIR));
    monsterBaseList.push_back(Monster(770, 540, 268000000, "e28", EARTH));
    monsterBaseList.push_back(Monster(746, 552, 264000000, "f28", FIRE));
    monsterBaseList.push_back(Monster(762, 536, 261000000, "w28", WATER));

    monsterBaseList.push_back(Monster(834, 616, 368000000, "a29", AIR));
    monsterBaseList.push_back(Monster(830, 614, 363000000, "e29", EARTH));
    monsterBaseList.push_back(Monster(746, 676, 358000000, "f29", FIRE));
    monsterBaseList.push_back(Monster(1008, 512, 370000000, "w29", WATER));

    monsterBaseList.push_back(Monster(700, 906, 505000000, "a30", AIR));
    monsterBaseList.push_back(Monster(1022, 614, 497000000, "e30", EARTH));
    monsterBaseList.push_back(Monster(930, 690, 514000000, "f30", FIRE));
    monsterBaseList.push_back(Monster(802, 802, 515000000, "w30", WATER));
}

void initBaseHeroes() {
    baseHeroes.push_back(Monster( 45, 20, "ladyoftwilight",     AIR,   COMMON,    {PROTECT,       ALL, AIR, 1}));
    baseHeroes.push_back(Monster( 70, 30, "tiny",               EARTH, RARE,      {AOE,           ALL, EARTH, 2}));
    baseHeroes.push_back(Monster( 90, 40, "nebra",              FIRE,  LEGENDARY, {BUFF,          ALL, FIRE, 8}));

    baseHeroes.push_back(Monster( 20, 10, "valor",              AIR,   COMMON,    {PROTECT,       AIR, AIR, 1}));
    baseHeroes.push_back(Monster( 30,  8, "rokka",              EARTH, COMMON,    {PROTECT,       EARTH, EARTH, 1}));
    baseHeroes.push_back(Monster( 24, 12, "pyromancer",         FIRE,  COMMON,    {PROTECT,       FIRE, FIRE, 1}));
    baseHeroes.push_back(Monster( 50,  6, "bewat",              WATER, COMMON,    {PROTECT,       WATER, WATER, 1}));

    baseHeroes.push_back(Monster( 22, 14, "hunter",             AIR,   COMMON,    {BUFF,          AIR, AIR, 2}));
    baseHeroes.push_back(Monster( 40, 20, "shaman",             EARTH, RARE,      {PROTECT,       EARTH, EARTH , 2}));
    baseHeroes.push_back(Monster( 82, 22, "alpha",              FIRE,  LEGENDARY, {AOE,           ALL, FIRE, 1}));

    baseHeroes.push_back(Monster( 28, 12, "carl",               WATER, COMMON,    {BUFF,          WATER, WATER , 2}));
    baseHeroes.push_back(Monster( 38, 22, "nimue",              AIR,   RARE,      {PROTECT,       AIR, AIR, 2}));
    baseHeroes.push_back(Monster( 70, 26, "athos",              EARTH, LEGENDARY, {PROTECT,       ALL, EARTH, 2}));

    baseHeroes.push_back(Monster( 24, 16, "jet",                FIRE,  COMMON,    {BUFF,          FIRE, FIRE, 2}));
    baseHeroes.push_back(Monster( 36, 24, "geron",              WATER, RARE,      {PROTECT,       WATER, WATER, 2}));
    baseHeroes.push_back(Monster( 46, 40, "rei",                AIR,   LEGENDARY, {BUFF,          ALL, AIR, 2}));

    baseHeroes.push_back(Monster( 19, 22, "ailen",              EARTH, COMMON,    {BUFF,          EARTH, EARTH, 2}));
    baseHeroes.push_back(Monster( 50, 18, "faefyr",             FIRE,  RARE,      {PROTECT,       FIRE, FIRE, 2}));
    baseHeroes.push_back(Monster( 60, 32, "auri",               WATER, LEGENDARY, {HEAL,          ALL, WATER, 2}));

    baseHeroes.push_back(Monster( 22, 32, "nicte",              AIR,   RARE,      {BUFF,          AIR, AIR, 4}));

    baseHeroes.push_back(Monster( 50, 12, "james",              EARTH, LEGENDARY, {PIERCE,        ALL, EARTH, 1}));

    baseHeroes.push_back(Monster( 28, 16, "k41ry",              AIR,   COMMON,    {BUFF,          AIR, AIR, 3}));
    baseHeroes.push_back(Monster( 46, 20, "t4urus",             EARTH, RARE,      {BUFF,          ALL, EARTH, 1}));
    baseHeroes.push_back(Monster(100, 20, "tr0n1x",             FIRE,  LEGENDARY, {AOE,           ALL, FIRE, 3}));
 
    baseHeroes.push_back(Monster( 58,  8, "aquortis",           WATER, COMMON,    {BUFF,          WATER, WATER, 3}));
    baseHeroes.push_back(Monster( 30, 32, "aeris",              AIR,   RARE,      {HEAL,          ALL, AIR, 1}));
    baseHeroes.push_back(Monster( 75,  2, "geum",               EARTH, LEGENDARY, {BERSERK,       SELF, EARTH, 2}));

    baseHeroes.push_back(Monster( 46, 16, "forestdruid",        EARTH, RARE,      {BUFF,          EARTH, EARTH, 4}));
    baseHeroes.push_back(Monster( 32, 24, "ignitor",            FIRE,  RARE,      {BUFF,          FIRE, FIRE, 4}));
    baseHeroes.push_back(Monster( 58, 14, "undine",             WATER, RARE,      {BUFF,          WATER, WATER, 4}));

    baseHeroes.push_back(Monster( 38, 12, "rudean",             FIRE,  COMMON,    {BUFF,          FIRE, FIRE, 3}));
    baseHeroes.push_back(Monster( 18, 50, "aural",              WATER, RARE,      {BERSERK,       SELF, WATER, 1.2f}));
    baseHeroes.push_back(Monster( 46, 46, "geror",              AIR,   LEGENDARY, {FRIENDS,       SELF, AIR, 1.2f}));

    baseHeroes.push_back(Monster( 66, 44, "veildur",            EARTH, LEGENDARY, {CHAMPION,      ALL, EARTH, 3}));
    baseHeroes.push_back(Monster( 72, 48, "brynhildr",          AIR,   LEGENDARY, {CHAMPION,      ALL, AIR, 4}));
    baseHeroes.push_back(Monster( 78, 52, "groth",              FIRE,  LEGENDARY, {CHAMPION,      ALL, FIRE, 5}));

    baseHeroes.push_back(Monster( 30, 16, "ourea",              EARTH, COMMON,    {BUFF,          EARTH, EARTH, 3}));
    baseHeroes.push_back(Monster( 48, 20, "erebus",             FIRE,  RARE,      {CHAMPION,      FIRE, FIRE, 2}));
    baseHeroes.push_back(Monster( 62, 36, "pontus",             WATER, LEGENDARY, {ADAPT,         WATER, WATER, 2}));

    baseHeroes.push_back(Monster( 52, 20, "chroma",             AIR,   RARE,      {PROTECT,       AIR, AIR, 4}));
    baseHeroes.push_back(Monster( 26, 44, "petry",              EARTH, RARE,      {PROTECT,       EARTH, EARTH, 4}));
    baseHeroes.push_back(Monster( 58, 22, "zaytus",             FIRE,  RARE,      {PROTECT,       FIRE, FIRE, 4}));

    baseHeroes.push_back(Monster( 75, 45, "spyke",              AIR,   LEGENDARY, {TRAINING,      SELF, AIR, 5}));
    baseHeroes.push_back(Monster( 70, 55, "aoyuki",             WATER, LEGENDARY, {RAINBOW,       SELF, WATER, 50}));
    baseHeroes.push_back(Monster( 50,100, "gaiabyte",           EARTH, LEGENDARY, {WITHER,        SELF, EARTH, 0.5f}));

    baseHeroes.push_back(Monster( 36, 14, "oymos",              AIR,   COMMON,    {BUFF,          AIR, AIR, 4}));
    baseHeroes.push_back(Monster( 32, 32, "xarth",              EARTH, RARE,      {CHAMPION,      EARTH, EARTH, 2}));
    baseHeroes.push_back(Monster( 76, 32, "atzar",              FIRE,  LEGENDARY, {ADAPT,         FIRE, FIRE, 2}));

    baseHeroes.push_back(Monster( 70, 42, "zeth",               WATER, LEGENDARY, {REVENGE,       ALL, WATER, 0.1f}));
    baseHeroes.push_back(Monster( 76, 46, "koth",               EARTH, LEGENDARY, {REVENGE,       ALL, EARTH, 0.15f}));
    baseHeroes.push_back(Monster( 82, 50, "gurth",              AIR,   LEGENDARY, {REVENGE,       ALL, AIR, 0.2f}));

    baseHeroes.push_back(Monster( 35, 25, "werewolf",           EARTH, COMMON,    {PROTECT_L,     ALL, EARTH, 0.112f}));
    baseHeroes.push_back(Monster( 55, 35, "jackoknight",        AIR,   RARE,      {BUFF_L,        ALL, AIR, 0.112f}));
    baseHeroes.push_back(Monster( 75, 45, "dullahan",           FIRE,  LEGENDARY, {CHAMPION_L,    ALL, FIRE, 0.112f}));

    baseHeroes.push_back(Monster( 36, 36, "ladyodelith",        WATER, RARE,      {PROTECT,       WATER, WATER, 4}));

    baseHeroes.push_back(Monster( 34, 54, "shygu",              AIR,   LEGENDARY, {PROTECT_L,     AIR, AIR, 0.112f}));
    baseHeroes.push_back(Monster( 72, 28, "thert",              EARTH, LEGENDARY, {PROTECT_L,     EARTH, EARTH, 0.112f}));
    baseHeroes.push_back(Monster( 32, 64, "lordkirk",           FIRE,  LEGENDARY, {PROTECT_L,     FIRE, FIRE, 0.112f}));
    baseHeroes.push_back(Monster( 30, 70, "neptunius",          WATER, LEGENDARY, {PROTECT_L,     WATER, WATER, 0.112f}));
    
    baseHeroes.push_back(Monster( 65, 12, "sigrun",             FIRE,  LEGENDARY, {VALKYRIE,      ALL, FIRE, 0.5f}));
    baseHeroes.push_back(Monster( 70, 14, "koldis",             WATER, LEGENDARY, {VALKYRIE,      ALL, WATER, 0.5f}));
    baseHeroes.push_back(Monster( 75, 16, "alvitr",             EARTH, LEGENDARY, {VALKYRIE,      ALL, EARTH, 0.5f}));
    
    baseHeroes.push_back(Monster( 30, 18, "hama",               WATER, COMMON,    {BUFF,          WATER, WATER, 4}));
    baseHeroes.push_back(Monster( 34, 34, "hallinskidi",        AIR,   RARE,      {CHAMPION,      AIR, AIR, 2}));
    baseHeroes.push_back(Monster( 60, 42, "rigr",               EARTH, LEGENDARY, {ADAPT,         EARTH, EARTH, 2}));
    
    baseHeroes.push_back(Monster(174, 46, "aalpha",             FIRE,  ASCENDED,  {AOE_L,         ALL, FIRE, 0.304f}));
    baseHeroes.push_back(Monster(162, 60, "aathos",             EARTH, ASCENDED,  {PROTECT_L,     ALL, EARTH, 0.304f}));
    baseHeroes.push_back(Monster(120,104, "arei",               AIR,   ASCENDED,  {BUFF_L,        ALL, AIR, 0.304f}));
    baseHeroes.push_back(Monster(148, 78, "aauri",              WATER, ASCENDED,  {HEAL_L,        ALL, WATER, 0.152f}));
    baseHeroes.push_back(Monster(190, 38, "atr0n1x",            FIRE,  ASCENDED,  {VALKYRIE,      ALL, FIRE, 0.75f}));
    baseHeroes.push_back(Monster(222,  8, "ageum",              EARTH, ASCENDED,  {BERSERK,       SELF, EARTH, 2}));
    baseHeroes.push_back(Monster(116,116, "ageror",             AIR,   ASCENDED,  {FRIENDS,       SELF, AIR, 1.3f}));
    
    baseHeroes.push_back(Monster(WORLDBOSS_HEALTH, 60, "lordofchaos", FIRE, WORLDBOSS, {AOE,      ALL, FIRE, 20}));
    baseHeroes.push_back(Monster(WORLDBOSS_HEALTH, 125, "mother", EARTH, WORLDBOSS, {DAMPEN,      ALL, EARTH, .5}));
    
    baseHeroes.push_back(Monster( 38, 24, "christmaself",       WATER, COMMON,    {HEAL_L,        ALL, WATER, 0.112f}));
    baseHeroes.push_back(Monster( 54, 36, "reindeer",           AIR,   RARE,      {AOE_L,         ALL, AIR, 0.112f}));
    baseHeroes.push_back(Monster( 72, 48, "santaclaus",         FIRE,  LEGENDARY, {LIFESTEAL_L,   ALL, FIRE, 0.112f}));
    baseHeroes.push_back(Monster( 44, 44, "sexysanta",          EARTH, RARE,      {VALKYRIE,      ALL, EARTH, 0.66f}));    
    
    baseHeroes.push_back(Monster( 24, 24, "toth",               FIRE,  COMMON,    {BUFF,          FIRE, FIRE, 4}));
    baseHeroes.push_back(Monster( 40, 30, "ganah",              WATER, RARE,      {CHAMPION,      WATER, WATER, 2}));
    baseHeroes.push_back(Monster( 58, 46, "dagda",              AIR,   LEGENDARY, {ADAPT,         AIR, AIR, 2}));
    
    baseHeroes.push_back(Monster(200, 60, "bubbles",            WATER, ASCENDED,  {DAMPEN_L,      ALL, WATER, 0.0025f}));
    
    baseHeroes.push_back(Monster(150, 86, "apontus",            WATER, ASCENDED,  {ADAPT,         WATER, WATER, 3}));
    baseHeroes.push_back(Monster(201, 37, "aatzar",             FIRE,  ASCENDED,  {ADAPT,         FIRE, FIRE, 3}));
    
    baseHeroes.push_back(Monster( 74, 36, "arshen",             AIR,   LEGENDARY, {TRAMPLE,       ALL, AIR, 2}));
    baseHeroes.push_back(Monster( 78, 40, "rua",                FIRE,  LEGENDARY, {TRAMPLE,       ALL, FIRE, 2}));
    baseHeroes.push_back(Monster( 82, 44, "dorth",              WATER, LEGENDARY, {TRAMPLE,       ALL, WATER, 2}));
}

void initQuests() {
    quests.push_back({""});
    quests.push_back({"w5"});
    quests.push_back({"f1", "a1", "f1", "a1", "f1", "a1"});
    quests.push_back({"f5", "a5"});
    quests.push_back({"f2", "a2", "e2", "w2", "f3", "a3"});
    quests.push_back({"w3", "e3", "w3", "e3", "w3", "e3"});       //5
    quests.push_back({"w4", "e1", "a4", "f4", "w1", "e4"});
    quests.push_back({"f5", "a5", "f4", "a3", "f2", "a1"});
    quests.push_back({"e4", "w4", "w5", "e5", "w4", "e4"});
    quests.push_back({"w5", "f5", "e5", "a5", "w4", "f4"});
    quests.push_back({"w5", "e5", "a5", "f5", "e5", "w5"});       //10
    quests.push_back({"f5", "f6", "e5", "e6", "a5", "a6"});
    quests.push_back({"e5", "w5", "f5", "e6", "f6", "w6"});
    quests.push_back({"a8", "a7", "a6", "a5", "a4", "a3"});
    quests.push_back({"f7", "f6", "f5", "e7", "e6", "e6"});
    quests.push_back({"w5", "e6", "w6", "e8", "w8"});             //15
    quests.push_back({"a9", "f8", "a8"});
    quests.push_back({"w5", "e6", "w7", "e8", "w8"});
    quests.push_back({"f7", "f6", "a6", "f5", "a7", "a8"});
    quests.push_back({"e7", "w9", "f9", "e9"});
    quests.push_back({"f2", "a4", "f5", "a7", "f8", "a10"});      //20
    quests.push_back({"w10", "a10", "w10"});
    quests.push_back({"w9", "e10", "f10"});
    quests.push_back({"e9", "a9", "w8", "f8", "e8"});
    quests.push_back({"f6", "a7", "f7", "a8", "f8", "a9"});
    quests.push_back({"w8", "w7", "w8", "w8", "w7", "w8"});       //25
    quests.push_back({"a9", "w7", "w8", "e7", "e8", "f10"});
    quests.push_back({"e9", "f9", "w9", "f7", "w7", "w7"});
    quests.push_back({"a10", "a8", "a9", "a10", "a9"});
    quests.push_back({"a10", "w7", "f7", "e8", "a9", "a9"});
    quests.push_back({"e10", "e10", "e10", "f10"});               //30
    quests.push_back({"e9", "f10", "f9", "f9", "a10", "a7"});
    quests.push_back({"w1", "a9", "f10", "e9", "a10", "w10"});
    quests.push_back({"e9", "a9", "a9", "f9", "a9", "f10"});
    quests.push_back({"f8", "e9", "w9", "a9", "a10", "a10"});
    quests.push_back({"w8", "w8", "w10", "a10", "a10", "f10"});   //35
    quests.push_back({"a8", "a10", "f10", "a10", "a10", "a10"});
    quests.push_back({"e8", "a10", "e10", "f10", "f10", "e10"});
    quests.push_back({"f10", "e10", "w10", "a10", "w10", "w10"});
    quests.push_back({"w9", "a10", "w10", "e10", "a10", "a10"});
    quests.push_back({"w10", "a10", "w10", "a10", "w10", "a10"}); //40
    quests.push_back({"e12", "e11", "a11", "f11", "a12"});
    quests.push_back({"a11", "a11", "e11", "a11", "e11", "a11"});
    quests.push_back({"a8", "a11", "a10", "w10", "a12", "e12"});
    quests.push_back({"a10", "f10", "a12", "f10", "a10", "f12"});
    quests.push_back({"w4", "e11", "a12", "a12", "w11", "a12"});  //45
    quests.push_back({"a11", "a12", "a11", "f11", "a11", "f10"});
    quests.push_back({"f12", "w11", "e12", "a12", "w12"});
    quests.push_back({"a11", "a11", "e12", "a11", "a11", "a13"});
    quests.push_back({"a13", "f13", "f13", "f13"});
    quests.push_back({"f12", "f12", "f12", "f12", "f12", "f12"}); //50
    quests.push_back({"a11", "e11", "a13", "a11", "e11", "a13"});
    quests.push_back({"f13", "w13", "a13", "f12", "f12"});
    quests.push_back({"a9", "f13", "f13", "f12", "a12", "a12"});
    quests.push_back({"a13", "a13", "a12", "a12", "f11", "f12"});
    quests.push_back({"a11", "f10", "a11", "e14", "f13", "a11"}); //55
    quests.push_back({"f13", "a13", "f13", "e13", "w12"});
    quests.push_back({"e10", "a13", "w12", "f13", "f13", "f13"});
    quests.push_back({"f7", "w11", "w13", "e14", "f13", "a14"});
    quests.push_back({"a8", "f15", "a14", "f14", "w14"});
    quests.push_back({"f12", "w13", "a14", "f13", "a13", "e10"}); //60
    quests.push_back({"f13", "e13", "a13", "w12", "f12", "a12"});
    quests.push_back({"w13", "e12", "w12", "a14", "a12", "f13"});
    quests.push_back({"e15", "f14", "w14", "a15"});
    quests.push_back({"e12", "a14", "e14", "w13", "e12", "f13"});
    quests.push_back({"e13", "f12", "w11", "w12", "a14", "e14"}); //65
    quests.push_back({"a14", "e13", "a11", "a14", "f13", "e13"});
    quests.push_back({"f13", "w13", "e14", "f13", "f14", "a14"});
    quests.push_back({"a15", "e15", "f15", "w15"});
    quests.push_back({"f13", "a14", "e14", "f13", "a14", "f13"});
    quests.push_back({"a11", "a14", "w13", "e14", "a14", "f14"}); //70
    quests.push_back({"e13", "a14", "f14", "w13", "f14", "e14"});
    quests.push_back({"w10", "a14", "a14", "a14", "a14", "w14"});
    quests.push_back({"w13", "w13", "f14", "a15", "a15", "e13"});
    quests.push_back({"a14", "e14", "e14", "e14", "e14", "e14"});
    quests.push_back({"w15", "w15", "e15", "w15", "f15"});        //75
    quests.push_back({"f14", "e15", "a15", "w14", "a14", "e15"});
    quests.push_back({"w14", "a15", "w14", "e15", "a15", "w14"});
    quests.push_back({"w15", "w15", "w15", "w15", "f15", "f15"});
    quests.push_back({"a15", "a15", "a15", "a15", "a15", "w14"});
    quests.push_back({"f15", "w15", "w15", "w15", "w15", "w15"});
}

// Fills all references and storages with real data.
// Must be called before any other operation on monsters or input
void initGameData() {
    // Initialize Monster Data
    initMonsterData();
    initBaseHeroes();
    initQuests();

    for (size_t i = 0; i < monsterBaseList.size(); i++) {
        monsterReference.push_back(monsterBaseList[i]);
        monsterMap.insert(std::pair<std::string, MonsterIndex>(monsterBaseList[i].name, i));
    }
}

// Filter monsters according to user input. Fills the available-references
// Must be called before any instance can be solved
void filterMonsterData(FollowerCount minimumMonsterCost, FollowerCount maximumArmyCost) {
    std::vector<Monster> tempMonsterList = monsterBaseList; // Get a temporary list to sort
    sort(tempMonsterList.begin(), tempMonsterList.end(), isCheaper);
    
    for (size_t i = 0; i < tempMonsterList.size(); i++) {
        if (minimumMonsterCost <= tempMonsterList[i].cost && maximumArmyCost >= tempMonsterList[i].cost) {
            availableMonsters.push_back(monsterMap[tempMonsterList[i].name]);
        }
    }
}

// Add a leveled hero to the databse and return its corresponding index
MonsterIndex addLeveledHero(Monster & hero, int level) {
    Monster m(hero, level);
    monsterReference.emplace_back(m);
    
    return (MonsterIndex) (monsterReference.size() - 1);
}

// Get Index corresponding to the id used ingame. monsters >= 0, heroes <= -2, empty spot = -1
int getRealIndex(Monster & monster) {
    int index = INDEX_NO_MONSTER;
    size_t i;
    if (monster.rarity != NO_HERO) {
        for (i = 0; i < baseHeroes.size(); i++) {
            if (baseHeroes[i].baseName == monster.baseName) {
                index = (int) (-i - 2);
            }
        }
    } else {
        for (i = 0; i < monsterBaseList.size(); i++) {
            if (monster.name == monsterBaseList[i].name) {
                index = (int) i;
            }
        }
    }
    return index;
}
