CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TARGET = my_allocator

SRC = demo.cpp

all: $(TARGET) run

install: $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

distclean:
	rm -f $(TARGET)