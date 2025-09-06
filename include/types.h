

#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <cstdint>

#define TURN_DICE_ARRAY_SIZE 252 * 3 // 252 possible dice combinations per turn
#define GAME_STATE_ARRAY_SIZE 500000 // 500000 possible game states

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


#endif // TYPES_H