#include "types.h"
#include <algorithm>
#include <iostream>
#include <fstream>

#define PROB_FILE_HEADER 0x50524F42

// Global LUTs for fast dice operations
static DiceHand index_to_dice[252];
static uint8_t dice_to_index[6][6][6][6][6];
static bool luts_initialized = false;

void InitializeDiceLUTs() {
    if (luts_initialized) return;
    
    // Initialize dice_to_index with invalid marker
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 6; j++)
            for (int k = 0; k < 6; k++)
                for (int l = 0; l < 6; l++)
                    for (int m = 0; m < 6; m++)
                        dice_to_index[i][j][k][l][m] = 255;
    
    // Generate all 252 sorted dice combinations
    int idx = 0;
    for (int d1 = 1; d1 <= 6; d1++) {
        for (int d2 = d1; d2 <= 6; d2++) {
            for (int d3 = d2; d3 <= 6; d3++) {
                for (int d4 = d3; d4 <= 6; d4++) {
                    for (int d5 = d4; d5 <= 6; d5++) {
                        index_to_dice[idx].dice.d1 = d1;
                        index_to_dice[idx].dice.d2 = d2;
                        index_to_dice[idx].dice.d3 = d3;
                        index_to_dice[idx].dice.d4 = d4;
                        index_to_dice[idx].dice.d5 = d5;
                        index_to_dice[idx].dice.reserved = 0;
                        
                        dice_to_index[d1-1][d2-1][d3-1][d4-1][d5-1] = idx;
                        idx++;
                    }
                }
            }
        }
    }
    
    luts_initialized = true;
}

void DecryptHand(uint8_t hand, DiceHand& diceHand) {
    InitializeDiceLUTs();
    diceHand = index_to_dice[hand];
}

uint8_t EncryptHand(const DiceHand& diceHand) {
    InitializeDiceLUTs();
    
    // Sort dice to canonical form
    int dice[5] = {diceHand.dice.d1, diceHand.dice.d2, diceHand.dice.d3, diceHand.dice.d4, diceHand.dice.d5};
    std::sort(dice, dice + 5);
    
    return dice_to_index[dice[0]-1][dice[1]-1][dice[2]-1][dice[3]-1][dice[4]-1];
}

double CalculateRerollProbability(uint8_t start_hand, uint8_t keep_mask, uint8_t end_hand) {
    DiceHand start_dice, end_dice;
    DecryptHand(start_hand, start_dice);
    DecryptHand(end_hand, end_dice);
    
    int start[5] = {start_dice.dice.d1, start_dice.dice.d2, start_dice.dice.d3, start_dice.dice.d4, start_dice.dice.d5};
    int end[5] = {end_dice.dice.d1, end_dice.dice.d2, end_dice.dice.d3, end_dice.dice.d4, end_dice.dice.d5};
    
    // Count rerolls and check kept dice match
    int reroll_count = 0;
    for (int i = 0; i < 5; i++) {
        if ((keep_mask >> i) & 1) {
            if (start[i] != end[i]) return 0.0; // Kept dice must match
        } else {
            reroll_count++;
        }
    }
    
    if (reroll_count == 0) return (start_hand == end_hand ? 1.0 : 0.0);
    
    // Each rerolled die has 1/6 probability - simplified model
    double prob = 1.0;
    for (int i = 0; i < reroll_count; i++) prob /= 6.0;
    return prob;
}

bool GenerateRerollProbabilities(const char* filename) {
    InitializeDiceLUTs();
    std::cout << "Generating reroll probabilities..." << std::endl;
    
    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;
    
    // Write header
    uint32_t header[3] = {PROB_FILE_HEADER, 252, 32}; // 'PROB' in hex, dice_count, mask_count
    file.write(reinterpret_cast<const char*>(header), sizeof(header));
    
    // Calculate and write probabilities
    for (int start = 0; start < 252; start++) {
        if (start % 50 == 0) std::cout << "  " << start << "/252" << std::endl;
        
        for (int mask = 0; mask < 32; mask++) {
            for (int end = 0; end < 252; end++) {
                double prob = CalculateRerollProbability(start, mask, end);
                file.write(reinterpret_cast<const char*>(&prob), sizeof(prob));
            }
        }
    }
    
    std::cout << "Generated 16MB reroll table" << std::endl;
    return file.good();
}

bool LoadRerollProbabilities(const char* filename, double probabilities[252][32][252]) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    
    // Verify header
    uint32_t header[3];
    file.read(reinterpret_cast<char*>(header), sizeof(header));
    if (header[0] != PROB_FILE_HEADER || header[1] != 252 || header[2] != 32) return false;
    
    // Load probabilities
    file.read(reinterpret_cast<char*>(probabilities), 252 * 32 * 252 * sizeof(double));
    
    std::cout << "Loaded reroll probabilities" << std::endl;
    return file.good();
}