CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O0 -g -pthread \
            -Iinclude -I../Shared/include

PC_SRC_DIR := src
SHARED_SRC_DIR := ../Shared/src
BUILD_DIR := build

SRC := $(wildcard $(PC_SRC_DIR)/*.cpp) $(wildcard $(SHARED_SRC_DIR)/*.cpp)

OBJ := $(patsubst $(PC_SRC_DIR)/%.cpp,$(BUILD_DIR)/pc_%.o,$(wildcard $(PC_SRC_DIR)/*.cpp)) \
       $(patsubst $(SHARED_SRC_DIR)/%.cpp,$(BUILD_DIR)/shared_%.o,$(wildcard $(SHARED_SRC_DIR)/*.cpp))

TARGET := $(BUILD_DIR)/program

all: $(TARGET)

# Compile PC sources
$(BUILD_DIR)/pc_%.o: $(PC_SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile Shared sources
$(BUILD_DIR)/shared_%.o: $(SHARED_SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)/*

.PHONY: all clean