CXX=g++
SB6DIR=/Users/guillaume/git/sb6code
INCS=-I$(SB6DIR)/include
CXXFLAGS=-Wall $(INCS) -std=c++11 -g -O2
LDFLAGS=-L$(SB6DIR)/lib -L/usr/local/lib -framework Cocoa -framework IOKit -framework OpenGL -lsb6 -lglfw

all: main
main: main.cpp

clean:
	rm -f *.o main
