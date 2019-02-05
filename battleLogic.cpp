#include "battleLogic.h"

// Prototype function! Currently not used. Function determining if a monster is strictly better than another
bool isBetter(Monster * a, Monster * b, bool considerAbilities) {
    if (a->element == b->element) {
        return (a->damage >= b->damage) && (a->hp >= b->hp);
    } else { // a needs to be better than b even when b has elemental advantage, or a is at disadvantage
        return !considerAbilities && (a->damage >= (int16_t) ((double) b->damage * elementalBoost)) && (a->hp >= (int16_t) ((double) b->hp * elementalBoost));
    }
}
