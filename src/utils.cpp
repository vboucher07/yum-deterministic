#include "utils.h"
#include <iostream>

void greet(const std::string& name) {
    std::cout << "Hello from " << name << "!" << std::endl;
}

void print(const std::string& message) {
    std::cout << message << std::endl;
}

void printError(const std::string& message) {
    // add error prefix and call print
    print("Error: " + message);
}

void printWarning(const std::string& message) {
    // add warning prefix and call print
    print("Warning: " + message);
}

void printInfo(const std::string& message) {
    // add info prefix and call print
    print("Info: " + message);
}