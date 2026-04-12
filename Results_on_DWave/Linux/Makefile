# Makefile for A5_GMR_2 Cryptanalysis

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -O3 -DNDEBUG -march=native -Wall -Wextra
LDFLAGS = 

# Source and target
SRCDIR = .
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
TARGET = a5_gmr2_attack

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Clean and rebuild
rebuild: clean all

# Run the program
run: $(TARGET)
	./$(TARGET)

# Declare phony targets
.PHONY: all clean rebuild run