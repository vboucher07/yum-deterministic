#include <iostream>
#include <string>
#include <cstring>
#include "utils.h"
#include "state_generation.h"
#include "state_index_map.h"
#include "ev_calculator.h"
#include "types.h"

void printUsage(const char* programName) {
    print("Usage: " + std::string(programName) + " [command] [options]");
    print("Commands:");
    print("  generate         Generate state index mapping");
    print("  generate-reroll  Generate reroll probabilities table");
    print("  calculate-ev     Calculate optimal actions via backward induction");
    print("    --save-ev      Also save EV values (creates large file)");
    print("  hello            Default hello world");
}

void generateMapping() {
    StateIndexMap mapping;
    if (!mapping.generate() || !mapping.saveToFile("output/state_mapping.dat")) {
        printError("Failed to generate state mapping!");
        return;
    }
    print("Generated " + std::to_string(mapping.getNumStates()) + " state mappings");
}

void generateRerollProbabilities() {
    if (!GenerateRerollProbabilities("output/reroll_probabilities.dat")) {
        printError("Failed to generate reroll probabilities!");
    }
}

void calculateEV(bool save_ev) {
    print("Loading state mapping...");
    StateIndexMap mapping;
    if (!mapping.loadFromFile("output/state_mapping.dat")) {
        printError("Failed to load state mapping! Run 'generate' first.");
        return;
    }
    print("Loaded " + std::to_string(mapping.getNumStates()) + " states");
    
    EVCalculator calculator(&mapping);
    calculator.calculateOptimalActions("output/optimal_actions.dat", save_ev);
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
    } else if (command == "calculate-ev") {
        bool save_ev = (argc > 2 && std::string(argv[2]) == "--save-ev");
        calculateEV(save_ev);
    } else if (command == "hello") {
        print("Hello, World!");
        greet("Yum Deterministic AI");
    } else {
        printError("Unknown command: " + command);
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}