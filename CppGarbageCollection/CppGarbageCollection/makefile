CPP = g++ -std=c++11

BIN = ./Bin/

all:	
	mkdir -p $(BIN)
	$(CPP)		-o $(BIN)Main.o		-c Main.cpp
	$(CPP)		-o $(BIN)gc_ptr.o	-c gc_ptr.cpp
	$(CPP)		-o $(BIN)UnitTest $(BIN)Main.o $(BIN)gc_ptr.o

clean:
	rm $(BIN)*
