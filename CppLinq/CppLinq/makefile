CPP = g++ -std=c++11

BIN = ./Bin/

all:	
	mkdir -p $(BIN)
	$(CPP)		-o $(BIN)Main.o		-c Main.cpp
	$(CPP)		-o $(BIN)UnitTest $(BIN)Main.o

clean:
	rm $(BIN)*
