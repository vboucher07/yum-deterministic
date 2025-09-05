# C++ Project

A basic C++ project structure with Makefile build system.

## Project Structure

```
.
├── src/           # Source files (.cpp)
├── include/       # Header files (.h/.hpp)
├── bin/           # Compiled executables (created during build)
├── obj/           # Object files (created during build)
├── build/         # Build artifacts (created during build)
├── Makefile       # Build configuration
├── .gitignore     # Git ignore rules
└── README.md      # This file
```

## Building the Project

### Basic Commands

```bash
# Build the project
make

# Build and run
make run

# Clean build artifacts
make clean

# Rebuild from scratch
make rebuild

# Build debug version
make debug

# Build release version
make release

# Show help
make help
```

### Requirements

- g++ compiler with C++17 support
- make utility

## Getting Started

1. Clone or download this project
2. Navigate to the project directory
3. Run `make` to build the project
4. Run `./bin/main` or `make run` to execute

## Adding New Files

- Add source files (`.cpp`) to the `src/` directory
- Add header files (`.h` or `.hpp`) to the `include/` directory
- The Makefile will automatically detect and compile new `.cpp` files

## Customization

You can modify the `Makefile` to:
- Change compiler flags (`CXXFLAGS`)
- Add additional include directories
- Link external libraries
- Modify the target executable name
