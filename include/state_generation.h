#ifndef STATE_GENERATION_H
#define STATE_GENERATION_H

#include "types.h"

// Generate all possible bonus states (categories 1-6 + bonus field)
uint16_t GenerateBonusStates(GameState* bonusStates);

// Generate all possible score states (score field only)
uint16_t GenerateScoreStates(GameState* scoreStates);

// Generate all possible remaining states (remaining categories + other fields)
uint16_t GenerateRemainingStates(GameState* remainingStates);

// Generate all possible complete game states (cartesian product of all components)
uint32_t GenerateAllStates(GameState* allStates);

#endif // STATE_GENERATION_H