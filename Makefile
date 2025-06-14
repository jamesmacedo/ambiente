CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -MMD -MP
LDFLAGS = -lxcb -lxcb-ewmh -lxcb-keysyms -lcairo -lxcb-damage -lxcb-xfixes -lxcb-composite -lxcb-render -lxcb-render-util -lcairo

SRC_DIR = app
BUILD_DIR = build

SRCS = $(shell find $(SRC_DIR) -name '*.cpp') 
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
TARGET = $(BUILD_DIR)/ambiente

SRC = ambiente.cpp

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) $(SRC) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(OBJS:.o=.d)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

