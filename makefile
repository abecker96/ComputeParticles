###########################################################
#
#	Makefile for Computer Graphics Final Project
#	@file main.cpp
#	@author Aidan Becker
#	@islandID ABecker2
#	@professor Dr. Scott King
#	@class COSC 4328-001 Computer Graphics
#	@version 0.0
#
###########################################################
Compiler =g++  -std=c++11 -Wall -Ofast
LDLIBS =-lGLEW -lGL -lX11 -lglfw
Remove =rm
Object =main.cpp -o
Name =main.out
	
assignment5:
	$(Compiler) $(Object) $(Name) $(LDLIBS)
	
clean:
	$(Remove) $(Name)

run:
	$(Compiler) $(Object) $(Name) $(LDLIBS)
	./$(Name)

remake:
	$(Remove) $(Name)
	$(Compiler) $(Object) $(Name) $(LDLIBS)
	./$(Name)