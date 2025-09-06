#include <iostream>
#include <string>
#include <cstring>
#include "utils.h"
#include "state_generation.h"
#include "state_index_map.h"
#include "types.h"

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [command]" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  generate       Generate state index mapping" << std::endl;
    std::cout << "  generate-reroll Generate reroll probabilities table" << std::endl;
    std::cout << "  hello          Default hello world" << std::endl;
}

void generateMapping() {
    StateIndexMap mapping;
    if (!mapping.generate() || !mapping.saveToFile("output/state_mapping.dat")) {
        std::cerr << "Failed to generate state mapping!" << std::endl;
        return;
    }
    std::cout << "Generated " << mapping.getNumStates() << " state mappings" << std::endl;
}

void generateRerollProbabilities() {
    if (!GenerateRerollProbabilities("output/reroll_probabilities.dat")) {
        std::cerr << "Failed to generate reroll probabilities!" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        greet("Yum Deterministic AI");
        printUsage(argv[0]);
        return 0;
    }
    
    std::string command = argv[1];
    
    if (command == "generate") {
        generateMapping();
    } else if (command == "generate-reroll") {
        generateRerollProbabilities();
    } else if (command == "hello") {
        std::cout << "Hello, World!" << std::endl;
        greet("Yum Deterministic AI");
    } else {
        std::cout << "Unknown command: " << command << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}