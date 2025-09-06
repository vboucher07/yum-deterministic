#include "state_index_map.h"
#include "state_generation.h"
#include <iostream>
#include <fstream>
#include <algorithm>

StateIndexMap::StateIndexMap() {}

bool StateIndexMap::generate() {
    std::cout << "Generating states..." << std::endl;
    
    const uint32_t MAX_STATES = 1599264;
    std::vector<GameState> allStates(MAX_STATES);
    uint32_t totalStates = GenerateAllStates(allStates.data());
    
    std::vector<uint32_t> keys;
    keys.reserve(totalStates);
    for (uint32_t i = 0; i < totalStates; i++) {
        keys.push_back(allStates[i].value);
    }
    
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    
    key_to_index.clear();
    index_to_key.clear();
    index_to_key.reserve(keys.size());
    
    for (uint32_t i = 0; i < keys.size(); i++) {
        key_to_index[keys[i]] = i;
        index_to_key.push_back(keys[i]);
    }
    
    std::cout << "Built mapping for " << keys.size() << " states" << std::endl;
    return true;
}

uint32_t StateIndexMap::getIndex(const GameState& state) const {
    auto it = key_to_index.find(state.value);
    if (it == key_to_index.end()) {
        return UINT32_MAX;
    }
    return it->second;
}

bool StateIndexMap::saveToFile(const char* filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Save number of keys
    uint32_t numKeys = index_to_key.size();
    file.write(reinterpret_cast<const char*>(&numKeys), sizeof(numKeys));
    
    // Save index_to_key array
    file.write(reinterpret_cast<const char*>(index_to_key.data()), 
               numKeys * sizeof(uint32_t));
    
    return file.good();
}

bool StateIndexMap::loadFromFile(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Load number of keys
    uint32_t numKeys;
    file.read(reinterpret_cast<char*>(&numKeys), sizeof(numKeys));
    
    // Load index_to_key array
    index_to_key.resize(numKeys);
    file.read(reinterpret_cast<char*>(index_to_key.data()), 
              numKeys * sizeof(uint32_t));
    
    // Rebuild key_to_index map
    key_to_index.clear();
    for (uint32_t i = 0; i < numKeys; i++) {
        key_to_index[index_to_key[i]] = i;
    }
    
    std::cout << "Loaded " << numKeys << " state mappings" << std::endl;
    return file.good();
}
