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
    
    // EV table for current round only (memory optimization)
    double* current_ev_table;  // [state_index * 252 * 3 + dice_index * 3 + turn]
    uint8_t* optimal_actions;  // [state_index * 252 * 3 + dice_index * 3 + turn]
    
    // Future round EV lookup (loaded from next round's results)
    double* future_ev_table;   // [state_index] -> best EV for that state in next round
    
    // Progress tracking
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_update_time;
    uint64_t total_calculations;
    uint64_t completed_calculations;
    int current_round;
    int total_rounds;
    
    
    // OPTIMIZATION: Memoization caches
    std::unordered_map<uint64_t, double> ev_cache;
    std::unordered_map<uint64_t, bool> validation_cache;
    uint64_t cache_hits;
    uint64_t cache_misses;
    
    // File output streams for incremental saving
    std::ofstream* actions_file;
    std::ofstream* ev_file;
    
    // Helper functions
    void allocateEVTable();
    void deallocateEVTable();
    void allocateFutureEVTable();
    void deallocateFutureEVTable();
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
    
    // OPTIMIZATION: Memoization helpers
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