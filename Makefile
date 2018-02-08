CXX=g++
GLFWDIR=/usr/local/Cellar/glfw/3.2.1
INCS=-I$(GLFWDIR)/include/GLFW
CXXFLAGS=-Wall $(INCS) -std=c++11 -g -O2
LDFLAGS=-L$(GLFWDIR)/lib -framework Cocoa -framework OpenGL -lglfw

all: main
main: main.cpp

clean:
	rm -rf *.o main main.dSYM
