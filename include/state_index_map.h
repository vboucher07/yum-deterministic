#ifndef STATE_INDEX_MAP_H
#define STATE_INDEX_MAP_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include "types.h"

class StateIndexMap {
private:
    std::unordered_map<uint32_t, uint32_t> key_to_index;
    std::vector<uint32_t> index_to_key;
    
public:
    StateIndexMap();
    ~StateIndexMap();
    
    // Generate index mapping from all possible GameStates
    bool generate();
    
    // Get index for a GameState (returns UINT32_MAX if not found)
    uint32_t getIndex(const GameState& state) const;
    
    // Get GameState from index
    GameState getStateFromIndex(uint32_t index) const;
    
    // Get total number of states
    uint32_t getNumStates() const { return index_to_key.size(); }
    
    // Save/load mapping to/from file
    bool saveToFile(const char* filename) const;
    bool loadFromFile(const char* filename);
};

#endif // STATE_INDEX_MAP_H
