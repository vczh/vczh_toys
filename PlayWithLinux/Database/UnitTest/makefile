CPP = g++ -std=c++11 -g
OBJ = ./Obj/
BIN = ./Bin/

UNITTEST_CPP = $(wildcard *.cpp)
UNITTEST_OBJ = $(patsubst %.cpp, $(OBJ)%.o, $(UNITTEST_CPP))

DATABASE_DIR = ../Source/
DATABASE_CPP = $(wildcard $(DATABASE_DIR)*.cpp)
DATABASE_H   = $(wildcard $(DATABASE_DIR)*.h)
DATABASE_OBJ = $(patsubst $(DATABASE_DIR)%.cpp, $(OBJ)$.o, $(DATABASE_CPP))

ALL_OBJ      = $(UNITTEST_OBJ) $(DATABASE_OBJ)

.PHONY : all clean

all : $(ALL_OBJ)
	$(CPP) -o $(BIN)UnitTest $(ALL_OBJ)

$(DATABASE_CPP) : $(DATABASE_H)
$(DATABASE_OBJ) : $(OBJ)%.o : $(DATABASE_DIR)%.cpp
	$(CPP) -o $@ -c $<

$(UNITTEST_CPP) : $(DATABASE_OBJ)
$(UNITTEST_OBJ) : $(OBJ)%.o : $(UNITTEST_DIR)%.cpp
	$(CPP) -o $@ -c $<

clean:
	rm $(OBJ)* -rf
	rm $(BIN)* -rf
