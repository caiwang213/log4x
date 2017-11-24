CC = gcc -Wall -ansi

all:
	$(CXX) -g -std=c++11 -D_DEBUG -pthread log4x.cpp main.cpp -o bin/log4x

clean:
	@echo Cleaning up...
	@rm   bin/log4x 
	@echo Done.
