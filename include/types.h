

#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <cstdint>

#define TURN_DICE_ARRAY_SIZE 252 * 3 // 252 possible dice combinations per turn
#define GAME_STATE_ARRAY_SIZE 1599264 // 1599264 possible game states

typedef union {
    uint16_t value;
    struct {
        uint8_t d1 : 3;
        uint8_t d2 : 3;
        uint8_t d3 : 3;
        uint8_t d4 : 3;
        uint8_t d5 : 3;
        uint8_t reserved : 1;
    } dice;
} DiceHand;

typedef struct {
    DiceHand hand;
    uint8_t turn;
} TurnDice;

typedef union {
    uint8_t value;
    union {
        struct {
            uint8_t d1:1;
            uint8_t d2:1;
            uint8_t d3:1;
            uint8_t d4:1;
            uint8_t d5:1;
            uint8_t action_type:1;
            uint8_t reserved:2;
        } dice;
        struct {
            uint8_t category_idx:4;
            uint8_t action_type:1;
            uint8_t reserved:3;
        } category;
    } bits;
} Action;

typedef union {
    uint16_t value;
    struct {
        uint8_t hand; // encrypted dice hand, use _FILL_IN_() to decrypt
        uint8_t turn: 2;
        uint8_t reserved: 6;
    } bits;
} TurnDiceIndex;

typedef union {
    uint32_t value;
    struct {
        union {
            uint16_t value: 12;
            struct {
                uint8_t ones: 1;
                uint8_t twos: 1;
                uint8_t threes: 1;
                uint8_t fours: 1;
                uint8_t fives: 1;
                uint8_t sixes: 1;
                uint8_t lowScore: 1;
                uint8_t highScore: 1;
                uint8_t lowStraight: 1;
                uint8_t highStraight: 1;
                uint8_t fullHouse: 1;
                uint8_t yum: 1;
            } bits;
        } categories;
        uint8_t bonus: 6;
        uint8_t score: 5;
        uint16_t reserved: 9;
    } bits;
} GameState;

void DecryptHand(uint8_t hand, DiceHand& diceHand);

uint8_t EncryptHand(const DiceHand& diceHand);

// Generate and save reroll probabilities table
bool GenerateRerollProbabilities(const char* filename);

// Load reroll probabilities table
bool LoadRerollProbabilities(const char* filename, double probabilities[252][32][252]);


#endif // TYPES_H