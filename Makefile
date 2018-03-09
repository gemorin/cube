CXX=g++
#GLFWDIR=/usr/local/Cellar/glfw/3.2.1
GLFWDIR=/Users/guillaume/dev/glfw
INCS=-I$(GLFWDIR)/include
CXXFLAGS=-Wall $(INCS) -std=c++14 -g -O2
LDFLAGS=-L$(GLFWDIR)/src -framework Cocoa -framework OpenGL -lglfw

all: main
main: main.cpp cube.o
cube.o: cube.cpp cube.h

clean:
	rm -rf *.o main main.dSYM
