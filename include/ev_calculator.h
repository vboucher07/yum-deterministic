#ifndef EV_CALCULATOR_H
#define EV_CALCULATOR_H

#include "types.h"
#include "state_index_map.h"
#include <vector>
#include <chrono>
#include <unordered_map>
#include <fstream>

class EVCalculator {
private:
    StateIndexMap* state_map;
    static double reroll_probabilities[252][32][252];  // Static array in data segment
    static bool probabilities_loaded;
    
    // STATIC ALLOCATION: All rounds allocated upfront using floats for 16GB budget
    static float* all_ev_tables;        // [round][state * 252 * 3] - all rounds at once
    static uint8_t* all_action_tables;  // [round][state * 252 * 3] - all rounds at once
    static bool tables_allocated;
    
    // Current round pointers (point into static arrays)
    float* current_ev_table;
    uint8_t* current_action_table;
    float* future_ev_table;
    
    // Progress tracking
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_update_time;
    uint64_t total_calculations;
    uint64_t completed_calculations;
    int current_round;
    int total_rounds;
    
    
    // STATIC CACHE: Fixed size to prevent memory leaks
    static const size_t CACHE_SIZE = 1000000;  // 1M entries max
    static uint64_t cache_keys[CACHE_SIZE];
    static double cache_values[CACHE_SIZE];
    static bool cache_valid[CACHE_SIZE];
    static size_t cache_next_slot;
    uint64_t cache_hits;
    uint64_t cache_misses;
    
    // File output streams for incremental saving
    std::ofstream* actions_file;
    std::ofstream* ev_file;
    
    // Static allocation management
    static void allocateAllTables(uint32_t num_states);
    static void deallocateAllTables();
    void setCurrentRoundPointers(int round, uint32_t num_states);
    size_t getEVIndex(uint32_t state_index, uint8_t dice_index, int turn);
    
    // Round management
    bool loadFutureEVFromRound(int round);
    bool saveCurrentRoundEV(int round);
    bool saveOptimalActions(const char* output_filename, int round);
    
    // Progress tracking
    void initializeProgress();
    void updateProgress(uint64_t calculations_completed);
    void displayProgressBar();
    std::string formatTime(double seconds);
    std::string formatNumber(uint64_t number);
    
    // STATIC CACHE: Memoization helpers
    uint64_t getCacheKey(uint32_t state_index, uint8_t dice_index, int turn);
    void clearCache();
    void printCacheStats();
    
    // File output helpers
    bool openOutputFiles(const char* actions_filename, const char* ev_filename);
    void closeOutputFiles();
    void writeRoundData(int round, const std::vector<uint32_t>& states);
    
    
    
public:
    EVCalculator(StateIndexMap* map);
    ~EVCalculator();
    
    // Get all state indices that have exactly 'count' categories filled
    std::vector<uint32_t> getStatesWithCategoriesFilled(int count);
    
    // Calculate score for a specific category given dice
    int calculateCategoryScore(uint8_t dice_index, int category);
    
    // Check if a category can be validly scored
    bool isValidCategoryChoice(const GameState& state, int category, uint8_t dice_index);
    
    // Apply a score to a GameState, returning the new state
    GameState applyScore(const GameState& current, int category, uint8_t dice_index);
    
    // Load reroll probabilities from file
    bool loadRerollProbabilities(const char* filename);
    
    // Get reroll probability for specific transition
    double getRerollProbability(uint8_t start_dice, uint8_t keep_mask, uint8_t end_dice);
    
    // EV calculation functions
    void calculateOptimalActions(const char* output_filename, bool save_ev = false);
    double calculateEV(uint32_t state_index, uint8_t dice_index, int turn);
    double calculateTurn3EV(uint32_t state_index, uint8_t dice_index);
    double calculateTurn12EV(uint32_t state_index, uint8_t dice_index, int turn);
    
};

#endif // EV_CALCULATOR_H