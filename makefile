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
Compiler =g++ -std=c++11 -Wall
LDLIBS =-lGLEW -lGL -lX11 -lglfw -lpthread -ldl
Remove =rm
Objects =main.cpp imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_impl_glfw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/imgui_impl_opengl3.cpp
Name =ComputeShader

project:
	$(Compiler) $(Objects) $(LDLIBS)

run:
	./$(Name)

compile-run:
	$(Compiler) $(Objects) $(LDLIBS)
	./$(Name)
