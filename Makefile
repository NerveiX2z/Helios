CXX ?= clang++
CXXFLAGS += -std=c++17 -Wall -Wextra -O2 -Iinclude $(shell sdl2-config --cflags)
LDFLAGS += $(shell sdl2-config --libs) -lGLESv2

SRC = src/core/main.cpp src/core/Engine.cpp src/world/Player.cpp src/renderer/GLRenderer.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = helios

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: all
	./$(TARGET)

test: all
	./$(TARGET) --test

.PHONY: all clean run test
