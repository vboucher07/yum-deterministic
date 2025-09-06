#include <iostream>
#include <string>
#include <cstring>
#include "utils.h"
#include "state_generation.h"
#include "state_index_map.h"

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [command]" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  generate       Generate state index mapping" << std::endl;
    std::cout << "  hello          Default hello world" << std::endl;
}

void generateMapping() {
    std::cout << "=== Generating State Index Mapping ===" << std::endl;
    
    StateIndexMap mapping;
    if (!mapping.generate()) {
        std::cerr << "Failed to generate state mapping!" << std::endl;
        return;
    }
    
    if (!mapping.saveToFile("output/state_mapping.dat")) {
        std::cerr << "Failed to save state mapping!" << std::endl;
        return;
    }
    
    std::cout << "State mapping generated and saved successfully!" << std::endl;
    std::cout << "Total states: " << mapping.getNumStates() << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        greet("C++ Project");
        printUsage(argv[0]);
        return 0;
    }
    
    std::string command = argv[1];
    
    if (command == "generate") {
        generateMapping();
    } else if (command == "hello") {
        std::cout << "Hello, World!" << std::endl;
        greet("C++ Project");
    } else {
        std::cout << "Unknown command: " << command << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}