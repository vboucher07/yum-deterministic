#include "ev_calculator.h"
#include "state_generation.h"
#include "utils.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <string>

// Define static members
double EVCalculator::reroll_probabilities[252][32][252];
bool EVCalculator::probabilities_loaded = false;

// Static allocation for all rounds
float* EVCalculator::all_ev_tables = nullptr;
uint8_t* EVCalculator::all_action_tables = nullptr;
bool EVCalculator::tables_allocated = false;

// Static cache arrays (fixed size, no dynamic allocation)
uint64_t EVCalculator::cache_keys[EVCalculator::CACHE_SIZE];
double EVCalculator::cache_values[EVCalculator::CACHE_SIZE];
bool EVCalculator::cache_valid[EVCalculator::CACHE_SIZE];
size_t EVCalculator::cache_next_slot = 0;

EVCalculator::EVCalculator(StateIndexMap* map) : state_map(map), current_ev_table(nullptr), current_action_table(nullptr), future_ev_table(nullptr), cache_hits(0), cache_misses(0), actions_file(nullptr), ev_file(nullptr) {}

EVCalculator::~EVCalculator() {
    closeOutputFiles();
}

int countCategoriesFilled(const GameState& state) {
    int count = 0;
    count += state.bits.categories.bits.ones;
    count += state.bits.categories.bits.twos;
    count += state.bits.categories.bits.threes;
    count += state.bits.categories.bits.fours;
    count += state.bits.categories.bits.fives;
    count += state.bits.categories.bits.sixes;
    count += state.bits.categories.bits.lowScore;
    count += state.bits.categories.bits.highScore;
    count += state.bits.categories.bits.lowStraight;
    count += state.bits.categories.bits.highStraight;
    count += state.bits.categories.bits.fullHouse;
    count += state.bits.categories.bits.yum;
    return count;
}

int calculateCurrentSubtotal(const GameState& state) {
    // Calculate current subtotal from the bonus field
    // bonus = 63 - subtotal (or 0 if reached/unreachable)
    if (state.bits.bonus == 0) {
        // Either reached (>=63) or unreachable - need to check which
        int categoriesFilled = 0;
        if (state.bits.categories.bits.ones) categoriesFilled++;
        if (state.bits.categories.bits.twos) categoriesFilled++;
        if (state.bits.categories.bits.threes) categoriesFilled++;
        if (state.bits.categories.bits.fours) categoriesFilled++;
        if (state.bits.categories.bits.fives) categoriesFilled++;
        if (state.bits.categories.bits.sixes) categoriesFilled++;
        
        if (categoriesFilled == 6) {
            return 63; // All filled, bonus reached
        } else {
            // Unreachable - calculate max possible
            int maxPossible = categoriesFilled * 18; // Max 6*3 per unfilled category
            if (maxPossible < 63) {
                return maxPossible; // Unreachable
            } else {
                return 63; // Actually reached
            }
        }
    } else {
        return 63 - state.bits.bonus;
    }
}

bool canReach63FromState(const GameState& state, int additionalScore, int categoryToFill) {
    int currentSubtotal = calculateCurrentSubtotal(state);
    if (categoryToFill >= 0 && categoryToFill <= 5) {
        currentSubtotal += additionalScore;
    }
    
    if (currentSubtotal >= 63) return true;
    
    // Count unfilled categories (1-6)
    int unfilledCategories = 0;
    if (!state.bits.categories.bits.ones && categoryToFill != 0) unfilledCategories++;
    if (!state.bits.categories.bits.twos && categoryToFill != 1) unfilledCategories++;
    if (!state.bits.categories.bits.threes && categoryToFill != 2) unfilledCategories++;
    if (!state.bits.categories.bits.fours && categoryToFill != 3) unfilledCategories++;
    if (!state.bits.categories.bits.fives && categoryToFill != 4) unfilledCategories++;
    if (!state.bits.categories.bits.sixes && categoryToFill != 5) unfilledCategories++;
    
    // Max possible from unfilled categories (6*3 = 18 each)
    int maxFromUnfilled = unfilledCategories * 18;
    
    return (currentSubtotal + maxFromUnfilled) >= 63;
}

std::vector<uint32_t> EVCalculator::getStatesWithCategoriesFilled(int count) {
    std::vector<uint32_t> result;
    
    // Iterate through all states in the state map
    uint32_t num_states = state_map->getNumStates();
    
    // Removed debug output
    
    for (uint32_t state_index = 0; state_index < num_states; state_index++) {
        GameState state = state_map->getStateFromIndex(state_index);
        if (countCategoriesFilled(state) == count) {
            result.push_back(state_index);
        }
    }
    
    return result;
}

int EVCalculator::calculateCategoryScore(uint8_t dice_index, int category) {
    // Convert dice index to actual dice values
    DiceHand dice;
    DecryptHand(dice_index, dice);
    
    int dice_values[5] = {dice.dice.d1, dice.dice.d2, dice.dice.d3, dice.dice.d4, dice.dice.d5}; // Already 1-6
    int dice_counts[7] = {0}; // Count of each die value (1-6)
    int total_sum = 0;
    
    // Count occurrences and calculate sum
    for (int i = 0; i < 5; i++) {
        dice_counts[dice_values[i]]++;
        total_sum += dice_values[i];
    }
    
    switch (category) {
        case 0: // 1's
        case 1: // 2's  
        case 2: // 3's
        case 3: // 4's
        case 4: // 5's
        case 5: // 6's
            return dice_counts[category + 1] * (category + 1);
            
        case 6: // Low Score - sum of all dice (must be possible to be lower than High Score)
            // return sum
            return total_sum;
            
        case 7: // High Score - sum of all dice (must be possible to be higher than Low Score)  
            // return sum
            return total_sum;
            
        case 8: // Low Straight (1,2,3,4,5) = 15 points
            if (dice_counts[1] >= 1 && dice_counts[2] >= 1 && dice_counts[3] >= 1 && 
                dice_counts[4] >= 1 && dice_counts[5] >= 1) {
                return 15;
            }
            return 0;
            
        case 9: // High Straight (2,3,4,5,6) = 20 points
            if (dice_counts[2] >= 1 && dice_counts[3] >= 1 && dice_counts[4] >= 1 && 
                dice_counts[5] >= 1 && dice_counts[6] >= 1) {
                return 20;
            }
            return 0;
            
        case 10: // Full House (3 of one kind + 2 of another) = 25 points
            {
                bool has_three = false, has_two = false;
                for (int i = 1; i <= 6; i++) {
                    if (dice_counts[i] == 3) has_three = true;
                    if (dice_counts[i] == 2) has_two = true;
                }
                if (has_three && has_two) return 25;
                return 0;
            }
            
        case 11: // Yum (5 of a kind) = 30 points
            for (int i = 1; i <= 6; i++) {
                if (dice_counts[i] == 5) return 30;
            }
            return 0;
            
        default:
            return 0;
    }
}

bool EVCalculator::isValidCategoryChoice(const GameState& state, int category, uint8_t dice_index) {
    switch (category) {
        case 0: return !state.bits.categories.bits.ones;
        case 1: return !state.bits.categories.bits.twos;
        case 2: return !state.bits.categories.bits.threes;
        case 3: return !state.bits.categories.bits.fours;
        case 4: return !state.bits.categories.bits.fives;
        case 5: return !state.bits.categories.bits.sixes;
        case 6: return !state.bits.categories.bits.lowScore && 
                       (!state.bits.categories.bits.highScore || 
                        calculateCategoryScore(dice_index, 6) < state.bits.score);
        case 7: return !state.bits.categories.bits.highScore && 
                       (!state.bits.categories.bits.lowScore || 
                        calculateCategoryScore(dice_index, 7) > state.bits.score);
        case 8: return !state.bits.categories.bits.lowStraight;
        case 9: return !state.bits.categories.bits.highStraight;
        case 10: return !state.bits.categories.bits.fullHouse;
        case 11: return !state.bits.categories.bits.yum;
        default: return false;
    }
}

GameState EVCalculator::applyScore(const GameState& current, int category, uint8_t dice_index) {
    GameState newState = current;
    int score = calculateCategoryScore(dice_index, category);
    
    // Set the category as filled
    switch (category) {
        case 0: newState.bits.categories.bits.ones = 1; break;
        case 1: newState.bits.categories.bits.twos = 1; break;
        case 2: newState.bits.categories.bits.threes = 1; break;
        case 3: newState.bits.categories.bits.fours = 1; break;
        case 4: newState.bits.categories.bits.fives = 1; break;
        case 5: newState.bits.categories.bits.sixes = 1; break;
        case 6: newState.bits.categories.bits.lowScore = 1; break;
        case 7: newState.bits.categories.bits.highScore = 1; break;
        case 8: newState.bits.categories.bits.lowStraight = 1; break;
        case 9: newState.bits.categories.bits.highStraight = 1; break;
        case 10: newState.bits.categories.bits.fullHouse = 1; break;
        case 11: newState.bits.categories.bits.yum = 1; break;
    }
    
    // Update bonus for categories 1-6
    if (category <= 5) {
        int currentSubtotal = calculateCurrentSubtotal(current);
        int newSubtotal = currentSubtotal + score;
        
        // Update bonus: 0 if reached/unreachable, 63-subtotal otherwise
        if (newSubtotal >= 63) {
            newState.bits.bonus = 0; // Bonus achieved
        } else {
            // Check if bonus is still achievable
            if (canReach63FromState(newState, 0, -1)) {
                newState.bits.bonus = 63 - newSubtotal;
            } else {
                newState.bits.bonus = 0; // Unreachable
            }
        }
    }
    
    // Update score field for Low/High Score categories
    if (category == 6 || category == 7) {
        newState.bits.score = score;
    }
    
    return newState;
}

bool EVCalculator::loadRerollProbabilities(const char* filename) {
    if (probabilities_loaded) return true;
    
    if (!LoadRerollProbabilities(filename, reroll_probabilities)) {
        return false;
    }
    
    probabilities_loaded = true;
    return true;
}

double EVCalculator::getRerollProbability(uint8_t start_dice, uint8_t keep_mask, uint8_t end_dice) {
    if (!probabilities_loaded || start_dice >= 252 || keep_mask >= 32 || end_dice >= 252) {
        return 0.0;
    }
    return reroll_probabilities[start_dice][keep_mask][end_dice];
}

// Smart allocation: Only current round + future round (much more reasonable!)
void EVCalculator::allocateAllTables(uint32_t num_states) {
    if (tables_allocated) return;
    
    size_t entries_per_round = num_states * 252 * 3;
    size_t total_ev_entries = entries_per_round * 2;  // Only 2 rounds: current + future
    size_t total_action_entries = entries_per_round * 2;
    
    size_t ev_memory_mb = (total_ev_entries * sizeof(float)) / (1024 * 1024);
    size_t action_memory_mb = total_action_entries / (1024 * 1024);
    size_t total_mb = ev_memory_mb + action_memory_mb;
    
    print("🧠 Smart allocation (current + future rounds only):");
    print("   EV tables: " + std::to_string(ev_memory_mb) + "MB (float)");
    print("   Action tables: " + std::to_string(action_memory_mb) + "MB");
    print("   Total: " + std::to_string(total_mb) + "MB (~" + std::to_string(total_mb/1024) + "GB)");
    
    // Allocate only 2 rounds worth
    all_ev_tables = new float[total_ev_entries];
    all_action_tables = new uint8_t[total_action_entries];
    
    // Initialize to 0
    for (size_t i = 0; i < total_ev_entries; i++) {
        all_ev_tables[i] = 0.0f;
    }
    for (size_t i = 0; i < total_action_entries; i++) {
        all_action_tables[i] = 0;
    }
    
    tables_allocated = true;
    print("✅ Smart allocation complete!");
}

void EVCalculator::deallocateAllTables() {
    if (tables_allocated) {
        delete[] all_ev_tables;
        delete[] all_action_tables;
        all_ev_tables = nullptr;
        all_action_tables = nullptr;
        tables_allocated = false;
    }
}

// Set pointers for current round with 2-round swapping
void EVCalculator::setCurrentRoundPointers(int round, uint32_t num_states) {
    if (!tables_allocated) return;
    
    size_t entries_per_round = num_states * 252 * 3;
    
    // Use round parity to decide which half of the buffer to use
    bool use_first_half = (round % 2 == 0);
    
    if (use_first_half) {
        current_ev_table = all_ev_tables;  // First half
        current_action_table = all_action_tables;
        future_ev_table = all_ev_tables + entries_per_round;  // Second half
    } else {
        current_ev_table = all_ev_tables + entries_per_round;  // Second half
        current_action_table = all_action_tables + entries_per_round;
        future_ev_table = all_ev_tables;  // First half
    }
    
    // Clear current round data to 0
    for (size_t i = 0; i < entries_per_round; i++) {
        current_ev_table[i] = 0.0f;
        current_action_table[i] = 0;
    }
    
    print("  Round " + std::to_string(round) + " using " + (use_first_half ? "first" : "second") + " buffer half");
}

size_t EVCalculator::getEVIndex(uint32_t state_index, uint8_t dice_index, int turn) {
    return state_index * 252 * 3 + dice_index * 3 + turn;
}

// Load EV values from the next round (round + 1)
bool EVCalculator::loadFutureEVFromRound(int round) {
    if (round >= 12) {
        // No future rounds after round 12 - terminal states
        return true;
    }
    
    // Future EV table now handled by static allocation
    
    // For now, keep future EV as 0 (terminal calculation)
    // TODO: Load from file when implementing multi-round EV
    uint32_t num_states = state_map->getNumStates();
    for (uint32_t i = 0; i < num_states; i++) {
        future_ev_table[i] = 0.0;
    }
    
    return true;
}

// Save the best EV for each state from current round
bool EVCalculator::saveCurrentRoundEV(int round) {
    (void)round; // TODO: Use round number for file naming
    if (!current_ev_table) return false;
    
    // Find the best EV for each state across all dice and turns
    uint32_t num_states = state_map->getNumStates();
    std::vector<double> best_ev_per_state(num_states, 0.0);
    
    for (uint32_t state_idx = 0; state_idx < num_states; state_idx++) {
        double best_ev = 0.0;
        
        // Check all dice combinations and all turns for this state
        for (uint8_t dice_idx = 0; dice_idx < 252; dice_idx++) {
            for (int turn = 0; turn < 3; turn++) {
                double ev = current_ev_table[getEVIndex(state_idx, dice_idx, turn)];
                if (ev > best_ev) {
                    best_ev = ev;
                }
            }
        }
        
        best_ev_per_state[state_idx] = best_ev;
    }
    
    // TODO: Save to file for next round to load
    // For now, just store in memory for next iteration
    
    print("  Calculated best EV for " + std::to_string(num_states) + " states");
    return true;
}

// Legacy method - replaced by writeRoundData
bool EVCalculator::saveOptimalActions(const char* output_filename, int round) {
    (void)output_filename;
    (void)round;
    return true;
}

// Initialize progress tracking
void EVCalculator::initializeProgress() {
    start_time = std::chrono::steady_clock::now();
    last_update_time = start_time;
    completed_calculations = 0;
    current_round = 12;
    total_rounds = 12;
    
    // Removed debug output
    
    // Calculate total calculations across all rounds
    total_calculations = 0;
    for (int round = 12; round >= 1; round--) {
        int categories_needed = round - 1;
        std::vector<uint32_t> states = getStatesWithCategoriesFilled(categories_needed);
        uint64_t round_calculations = (uint64_t)states.size() * 252 * 3;
        total_calculations += round_calculations;
    }
    
    print("Starting backward induction (" + formatNumber(total_calculations) + " calculations)...");
    
    // Show initial progress bar
    displayProgressBar();
}

// Update progress and display if enough time has passed
void EVCalculator::updateProgress(uint64_t calculations_completed) {
    completed_calculations += calculations_completed;
    
    auto now = std::chrono::steady_clock::now();
    auto time_since_update = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_time).count();
    
    // Update display every 1 second for more responsive feedback
    if (time_since_update >= 1000) {
        displayProgressBar();
        last_update_time = now;
    }
}

// Display comprehensive progress bar
void EVCalculator::displayProgressBar() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count() / 1000.0;
    
    if (total_calculations == 0 || elapsed <= 0) return; // Safety check
    
    double overall_percent = (double)completed_calculations / total_calculations * 100.0;
    double rate = completed_calculations / elapsed; // calculations per second
    
    // Calculate ETA with more reasonable bounds
    double eta = 0;
    if (rate > 0 && completed_calculations > 0) {
        eta = (total_calculations - completed_calculations) / rate;
        // Cap ETA at 24 hours to avoid ridiculous estimates
        if (eta > 86400) eta = 86400;
    }
    
    // Create visual progress bar
    int bar_width = 40;
    int filled = (int)(overall_percent / 100.0 * bar_width);
    std::string bar = "[" + std::string(filled, '=') + std::string(bar_width - filled, '-') + "]";
    
    // Round info
    std::string round_info = "Round " + std::to_string(current_round) + "/12";
    
    // Format output
    std::cout << "\r" << bar << " " 
              << std::fixed << std::setprecision(1) << overall_percent << "% "
              << round_info << " | "
              << "Elapsed: " << formatTime(elapsed) << " | "
              << "ETA: " << formatTime(eta) << " | "
              << formatNumber((uint64_t)rate) << " calc/s" 
              << std::flush;
}

// Format time in human readable format
std::string EVCalculator::formatTime(double seconds) {
    if (seconds < 60) {
        return std::to_string((int)seconds) + "s";
    } else if (seconds < 3600) {
        int mins = (int)(seconds / 60);
        int secs = (int)seconds % 60;
        return std::to_string(mins) + "m " + std::to_string(secs) + "s";
    } else {
        int hours = (int)(seconds / 3600);
        int mins = (int)(seconds / 60) % 60;
        return std::to_string(hours) + "h " + std::to_string(mins) + "m";
    }
}

// Format numbers with thousands separators
std::string EVCalculator::formatNumber(uint64_t number) {
    std::string str = std::to_string(number);
    std::string result;
    int count = 0;
    
    for (int i = str.length() - 1; i >= 0; i--) {
        if (count == 3) {
            result = "," + result;
            count = 0;
        }
        result = str[i] + result;
        count++;
    }
    
    return result;
}

// OPTIMIZATION: Memoization functions
uint64_t EVCalculator::getCacheKey(uint32_t state_index, uint8_t dice_index, int turn) {
    // Pack into 64-bit key: state_index(32) + dice_index(8) + turn(4) + round(4) + padding(16)
    return ((uint64_t)state_index << 32) | ((uint64_t)dice_index << 8) | ((uint64_t)turn << 4) | ((uint64_t)current_round);
}

void EVCalculator::clearCache() {
    // Clear static cache arrays
    for (size_t i = 0; i < CACHE_SIZE; i++) {
        cache_valid[i] = false;
    }
    cache_next_slot = 0;
    cache_hits = 0;
    cache_misses = 0;
}

// Static cache doesn't need size limiting (fixed size)

void EVCalculator::printCacheStats() {
    uint64_t total = cache_hits + cache_misses;
    double hit_rate = total > 0 ? (double)cache_hits / total * 100.0 : 0.0;
    
    // Count valid entries in static cache
    size_t valid_entries = 0;
    for (size_t i = 0; i < CACHE_SIZE; i++) {
        if (cache_valid[i]) valid_entries++;
    }
    
    double cache_mb = CACHE_SIZE * (sizeof(uint64_t) + sizeof(double) + sizeof(bool)) / (1024.0 * 1024.0);
    
    print("Cache stats: " + formatNumber(cache_hits) + " hits, " + formatNumber(cache_misses) + " misses (" + 
          std::to_string((int)hit_rate) + "% hit rate), " + formatNumber(valid_entries) + "/" + formatNumber(CACHE_SIZE) + " entries (~" + 
          std::to_string((int)cache_mb) + "MB static)");
}

// File output management
bool EVCalculator::openOutputFiles(const char* actions_filename, const char* ev_filename) {
    // Open actions file (binary, 1 byte per action)
    actions_file = new std::ofstream(actions_filename, std::ios::binary);
    if (!actions_file->is_open()) {
        delete actions_file;
        actions_file = nullptr;
        return false;
    }
    
    // Conditionally open EV file
    if (ev_filename) {
        ev_file = new std::ofstream(ev_filename, std::ios::binary);
        if (!ev_file->is_open()) {
            delete actions_file;
            delete ev_file;
            actions_file = nullptr;
            ev_file = nullptr;
            return false;
        }
        print("Opened files: " + std::string(actions_filename) + " and " + std::string(ev_filename));
    } else {
        ev_file = nullptr;
        print("Opened file: " + std::string(actions_filename));
    }
    
    return true;
}

void EVCalculator::closeOutputFiles() {
    if (actions_file) {
        actions_file->close();
        delete actions_file;
        actions_file = nullptr;
    }
    if (ev_file) {
        ev_file->close();
        delete ev_file;
        ev_file = nullptr;
    }
}

void EVCalculator::writeRoundData(int round, const std::vector<uint32_t>& states) {
    (void)round;
    if (!actions_file) return;
    
    uint64_t entries = states.size() * 252 * 3;
    print("  Writing " + formatNumber(entries) + " entries...");
    
    // Write data for each state in this round
    for (uint32_t state_index : states) {
        for (uint8_t dice_index = 0; dice_index < 252; dice_index++) {
            for (int turn = 0; turn < 3; turn++) {
                size_t idx = getEVIndex(state_index, dice_index, turn);
                
                // Always write action (1 byte)
                uint8_t action = current_action_table[idx];
                actions_file->write(reinterpret_cast<const char*>(&action), 1);
                
                // Conditionally write EV (4 bytes for float)
                if (ev_file) {
                    float ev = current_ev_table[idx];
                    ev_file->write(reinterpret_cast<const char*>(&ev), sizeof(float));
                }
            }
        }
    }
    
    // Flush to ensure data is written
    actions_file->flush();
    if (ev_file) ev_file->flush();
}


// Main EV calculation entry point
void EVCalculator::calculateOptimalActions(const char* output_filename, bool save_ev) {
    if (!loadRerollProbabilities("output/reroll_probabilities.dat")) {
        printError("Failed to load reroll probabilities");
        return;
    }
    
    // Open output files for incremental writing
    std::string actions_file = std::string(output_filename);
    std::string ev_file = save_ev ? actions_file.substr(0, actions_file.find_last_of('.')) + "_ev.dat" : "";
    
    if (!openOutputFiles(actions_file.c_str(), save_ev ? ev_file.c_str() : nullptr)) {
        printError("Failed to open output files");
        return;
    }
    
    // STATIC ALLOCATION: Allocate all tables upfront
    allocateAllTables(state_map->getNumStates());
    
    // Initialize progress tracking
    initializeProgress();
    
    // We'll work backwards from round 12 to round 1
    for (int round = 12; round >= 1; round--) {
        current_round = round;
        
        // OPTIMIZATION: Clear cache between rounds to save memory
        clearCache();
        
        // Set pointers for this round (no allocation, just pointer arithmetic)
        setCurrentRoundPointers(round, state_map->getNumStates());
        
        // Get all states with (round - 1) categories filled
        std::vector<uint32_t> states = getStatesWithCategoriesFilled(round - 1);
        
        // Process each state and dice combination (BACK TO ORIGINAL APPROACH)
        uint32_t calculations_in_batch = 0;
        const uint32_t batch_size = 100; // Update progress every 100 calculations
        
        for (size_t s = 0; s < states.size(); s++) {
            uint32_t state_index = states[s];
            
            for (uint8_t dice_index = 0; dice_index < 252; dice_index++) {
                // Calculate EV for each turn
                for (int turn = 1; turn <= 3; turn++) {
                    double ev = calculateEV(state_index, dice_index, turn);
                    current_ev_table[getEVIndex(state_index, dice_index, turn - 1)] = (float)ev;
                    
                    calculations_in_batch++;
                    if (calculations_in_batch >= batch_size) {
                        updateProgress(calculations_in_batch);
                        calculations_in_batch = 0;
                        
                        // Static cache is fixed size, no limiting needed
                    }
                }
            }
        }
        
        // Update with remaining calculations
        if (calculations_in_batch > 0) {
            updateProgress(calculations_in_batch);
        }
        
        saveCurrentRoundEV(round);
        writeRoundData(round, states);
        
        std::cout << std::endl;
        print("Round " + std::to_string(round) + " completed");
        printCacheStats();
    }
    
    std::cout << std::endl;
    auto total_time = std::chrono::steady_clock::now() - start_time;
    auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(total_time).count();
    
    closeOutputFiles();
    deallocateAllTables();
    print("Completed in " + formatTime(total_seconds) + " (" + formatNumber(completed_calculations) + " calculations)");
}

double EVCalculator::calculateEV(uint32_t state_index, uint8_t dice_index, int turn) {
    // Check static hash cache
    uint64_t cache_key = getCacheKey(state_index, dice_index, turn);
    size_t hash_index = cache_key % CACHE_SIZE;
    
    // Check if the slot contains our key (simple hash table with replacement)
    if (cache_valid[hash_index] && cache_keys[hash_index] == cache_key) {
        cache_hits++;
        return cache_values[hash_index];
    }
    
    // Cache miss - calculate result
    cache_misses++;
    double result = (turn == 3) ? calculateTurn3EV(state_index, dice_index) 
                                : calculateTurn12EV(state_index, dice_index, turn);
    
    // Store in static cache (direct hash, replace if collision)
    cache_keys[hash_index] = cache_key;
    cache_values[hash_index] = result;
    cache_valid[hash_index] = true;
    
    return result;
}

double EVCalculator::calculateTurn3EV(uint32_t state_index, uint8_t dice_index) {
    GameState state = state_map->getStateFromIndex(state_index);
    double best_ev = 0.0;
    uint8_t best_action = 0;
    
    for (int category = 0; category < 12; category++) {
        if (!isValidCategoryChoice(state, category, dice_index)) continue;
        
        int score = calculateCategoryScore(dice_index, category);
        double future_ev = 0.0;
        
        if (future_ev_table && state_index < state_map->getNumStates()) {
            future_ev = future_ev_table[state_index];
        }
        
        double ev = score + future_ev;
        if (ev > best_ev) {
            best_ev = ev;
            best_action = category;
        }
    }
    
    current_action_table[getEVIndex(state_index, dice_index, 2)] = best_action;
    return best_ev;
}

double EVCalculator::calculateTurn12EV(uint32_t state_index, uint8_t dice_index, int turn) {
    double best_ev = 0.0;
    uint8_t best_action = 31;
    
    // Try keep masks in smart order (keep all first, then fewer dice)
    static const uint8_t smart_keep_order[] = {31, 15, 23, 27, 29, 30, 7, 11, 13, 14, 19, 21, 22, 25, 26, 28, 3, 5, 6, 9, 10, 12, 17, 18, 20, 24, 1, 2, 4, 8, 16, 0};
    
    for (int k = 0; k < 32; k++) {
        uint8_t keep_mask = smart_keep_order[k];
        double ev = 0.0;
        
        for (uint8_t result_dice = 0; result_dice < 252; result_dice++) {
            double prob = getRerollProbability(dice_index, keep_mask, result_dice);
            if (prob > 1e-10) {
                double next_ev = calculateEV(state_index, result_dice, turn + 1);
                ev += prob * next_ev;
            }
        }
        
        if (ev > best_ev) {
            best_ev = ev;
            best_action = keep_mask;
        }
    }
    
    current_action_table[getEVIndex(state_index, dice_index, turn - 1)] = best_action;
    return best_ev;
}