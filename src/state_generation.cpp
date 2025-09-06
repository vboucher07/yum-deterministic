
#include "state_generation.h"
#include <cstdint>

#define NUM_BONUS_STATES 1851       // unique possible bonus states
#define NUM_SCORE_STATES 54         // 2 + (2 * 26) = 54 score states  
#define NUM_REMAINING_STATES 16     // 4 bits so 16 states
#define NUM_ALL_STATES 1599264    // 1851 * 54 * 16 = 1,599,264 total states

uint16_t GenerateBonusStates(GameState bonusStates[]) {
    uint16_t count = 0;
    
    // generate all dice configs (nested loop for each die)
    // using 6 to mean not filled, 0-5 is value
    for (int c1 = 0; c1 <= 6; c1++) {
        for (int c2 = 0; c2 <= 6; c2++) {
            for (int c3 = 0; c3 <= 6; c3++) {
                for (int c4 = 0; c4 <= 6; c4++) {
                    for (int c5 = 0; c5 <= 6; c5++) {
                        for (int c6 = 0; c6 <= 6; c6++) {
                            int categories[6] = {c1, c2, c3, c4, c5, c6};
                            
                            // get bonus amount from filled categories
                            uint8_t currentBonus = 0;
                            for (int i = 0; i < 6; i++) {
                                if (categories[i] != 6) { // if filled
                                    currentBonus += categories[i] * (i + 1);
                                }
                            }
                            
                            // Calculate max possible from unfilled categories
                            uint8_t maxFromUnfilled = 0;
                            for (int i = 0; i < 6; i++) {
                                if (categories[i] == 6) { // if unfilled
                                    maxFromUnfilled += 5 * (i + 1);
                                }
                            }
                            
                            GameState state = {0};
                            
                            // set game state categories bits
                            state.bits.categories.bits.ones = (c1 != 6) ? 1 : 0;
                            state.bits.categories.bits.twos = (c2 != 6) ? 1 : 0;
                            state.bits.categories.bits.threes = (c3 != 6) ? 1 : 0;
                            state.bits.categories.bits.fours = (c4 != 6) ? 1 : 0;
                            state.bits.categories.bits.fives = (c5 != 6) ? 1 : 0;
                            state.bits.categories.bits.sixes = (c6 != 6) ? 1 : 0;
                            
                            // Set bonus value
                            if (currentBonus >= 63) {
                                state.bits.bonus = 0; // bonus already achieved
                            } else if (currentBonus + maxFromUnfilled < 63) {
                                state.bits.bonus = 0; // bonus unreachable
                            } else {
                                state.bits.bonus = 63 - currentBonus; // points still needed
                            }
                            
                            // Check for duplicates
                            bool duplicate = false;
                            for (uint16_t j = 0; j < count; j++) {
                                if (bonusStates[j].value == state.value) {
                                    duplicate = true;
                                    break;
                                }
                            }
                            
                            if (!duplicate && count < NUM_BONUS_STATES) {
                                bonusStates[count++] = state;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return count;
}

uint16_t GenerateScoreStates(GameState scoreStates[]) {
    uint16_t count = 0;
    
    // Generate all valid score combinations:
    // - lowScore and highScore bits (4 combinations: 00, 01, 10, 11)
    // - score values: 0 if both bits same, 5-30 if exactly one bit set
    for (uint8_t lowScore = 0; lowScore <= 1; lowScore++) {
        for (uint8_t highScore = 0; highScore <= 1; highScore++) {
            GameState state = {0};
            
            // Set the score category bits
            state.bits.categories.bits.lowScore = lowScore;
            state.bits.categories.bits.highScore = highScore;
            
            if (lowScore == highScore) {
                // Both filled or both unfilled means score = 0
                state.bits.score = 0;
                if (count < NUM_SCORE_STATES) {
                    scoreStates[count++] = state;
                }
            } else {
                // Exactly one is filled -> scores 5-30 are possible
                for (uint8_t score = 5; score <= 30; score++) {
                    GameState scoreState = state;
                    scoreState.bits.score = score;
                    if (count < NUM_SCORE_STATES) {
                        scoreStates[count++] = scoreState;
                    }
                }
            }
        }
    }
    
    return count;
}

uint16_t GenerateRemainingStates(GameState remainingStates[]) {
    uint16_t count = 0;
    
    // Generate all combinations of straights, full house, and yum (4 bits = 16 combinations)
    for (uint8_t categoryMask = 0; categoryMask < 16; categoryMask++) {
        GameState state = {0};
        
        // Set the remaining category bits (lowStraight, highStraight, fullHouse, yum)
        state.bits.categories.bits.lowStraight = (categoryMask & 0x01) ? 1 : 0;
        state.bits.categories.bits.highStraight = (categoryMask & 0x02) ? 1 : 0;
        state.bits.categories.bits.fullHouse = (categoryMask & 0x04) ? 1 : 0;
        state.bits.categories.bits.yum = (categoryMask & 0x08) ? 1 : 0;
        
        if (count < NUM_REMAINING_STATES) {
            remainingStates[count++] = state;
        }
    }
    
    return count;
}

uint32_t GenerateAllStates(GameState allStates[]) {
    // Generate component states
    GameState bonusStates[NUM_BONUS_STATES];
    GameState scoreStates[NUM_SCORE_STATES];
    GameState remainingStates[NUM_REMAINING_STATES];
    
    uint16_t bonusCount = GenerateBonusStates(bonusStates);
    uint16_t scoreCount = GenerateScoreStates(scoreStates);
    uint16_t remainingCount = GenerateRemainingStates(remainingStates);
    
    uint32_t count = 0;
    
    // Cartesian product of all component states
    for (uint16_t b = 0; b < bonusCount; b++) {
        for (uint16_t s = 0; s < scoreCount; s++) {
            for (uint16_t r = 0; r < remainingCount; r++) {
                if (count < NUM_ALL_STATES) {
                    GameState state = {0};
                    
                    // Combine states using bitwise OR (since they use different bit fields)
                    state.value = bonusStates[b].value | scoreStates[s].value | remainingStates[r].value;
                    
                    allStates[count++] = state;
                }
            }
        }
    }
    
    return count;
}