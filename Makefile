CC = g++

CXXFLAGS=-std=c++11
#LDFLAGS= `sdl2-config --libs`
#CXXFLAGS=`sdl2-config --cflags`

main: main.o

main.o: main.cpp
