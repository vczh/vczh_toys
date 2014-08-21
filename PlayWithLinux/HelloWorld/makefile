CPP = g++ -std=c++11 -g
OBJ = ./Obj/
BIN = ./Bin/

all : main.o
	$(CPP) -o $(BIN)main $(OBJ)main.o $(OBJ)calc.o

main.o : main.cpp calc.o
	$(CPP) -o $(OBJ)main.o -c main.cpp

calc.o : calc.cpp calc.h
	$(CPP) -o $(OBJ)calc.o -c calc.cpp

clean:
	rm $(OBJ)* -rf
	rm $(BIN)* -rf

