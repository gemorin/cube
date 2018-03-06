CXX=g++
GLFWDIR=/usr/local/Cellar/glfw/3.2.1
INCS=-I$(GLFWDIR)/include/GLFW
CXXFLAGS=-Wall $(INCS) -std=c++14 -g -O2
LDFLAGS=-framework Cocoa -framework OpenGL -lglfw

all: main
main: main.cpp cube.o

clean:
	rm -rf *.o main main.dSYM
