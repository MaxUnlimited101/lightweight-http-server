CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -pthread -O2
LDFLAGS := -pthread

SRC := main.cpp threadpool.cpp
OBJ := $(SRC:.cpp=.o)
DEPS := threadpool.hpp

TARGET := server.out

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)