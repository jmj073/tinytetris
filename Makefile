MAKEFLAGS += -rR
LDFLAGS=-lcurses

TARGET := tinytetris

all: tinytetris

tinytetris: tinytetris-commented.cpp
	g++ -std=c++20 -o $(TARGET) $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
