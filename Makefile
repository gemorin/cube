CXX=g++
SB6DIR=/Users/guillaume/git/sb6code
GLFWDIR=/usr/local/Cellar/glfw/3.2.1
INCS=-I$(GLFWDIR)/include/GLFW -I$(SB6DIR)/include
CXXFLAGS=-Wall $(INCS) -std=c++11 -g -O2 -fno-strict-aliasing
LDFLAGS=-L$(GLFWDIR)/lib -framework Cocoa -framework OpenGL -lglfw

all: main
main: main.cpp

clean:
	rm -rf *.o main main.dSYM
