# This file is generated from "./makefile.debug.makegen" by Vczh Makefile Generator

.PHONY : all clean

bin_TARGET = ././Bin/
obj_TARGET = ././Obj/

DATABASE_BASIC_DIR = ./../Source/./
DATABASE_BASIC_h = $(DATABASE_BASIC_DIR)DatabaseVlppReference.h
DATABASE_UTILITY_DIR = ./../Source/./Utility/
DATABASE_UTILITY_cpp = $(wildcard $(DATABASE_UTILITY_DIR)*.cpp)
DATABASE_UTILITY_h = $(wildcard $(DATABASE_UTILITY_DIR)*.h)
UNITTEST_DIR = ././
UNITTEST_cpp = $(wildcard $(UNITTEST_DIR)*.cpp)
UNITTEST_h = $(wildcard $(UNITTEST_DIR)*.h)
VLPP_DIR = ./../Source/./
VLPP_cpp = $(VLPP_DIR)Vlpp.cpp
VLPP_h = $(VLPP_DIR)Vlpp.h

DATABASE_UTILITY_o = $(patsubst $(DATABASE_UTILITY_DIR)%.cpp, $(obj_TARGET)%.o, $(DATABASE_UTILITY_cpp))
UNITTEST_o = $(patsubst $(UNITTEST_DIR)%.cpp, $(obj_TARGET)%.o, $(UNITTEST_cpp))
VLPP_o = $(patsubst $(VLPP_DIR)%.cpp, $(obj_TARGET)%.o, $(VLPP_cpp))

ALL_o = $(DATABASE_UTILITY_o) $(VLPP_o) $(UNITTEST_o)
all : $(ALL_o)
	g++ -std=c++11 -g -o $(bin_TARGET)UnitTest  $(ALL_o)

$(DATABASE_BASIC_h) : $(VLPP_h)
$(DATABASE_UTILITY_h) : $(DATABASE_BASIC_h)
$(DATABASE_UTILITY_cpp) : $(DATABASE_UTILITY_h)
$(VLPP_cpp) : $(VLPP_h)
$(UNITTEST_h) : $(DATABASE_BASIC_h) $(DATABASE_UTILITY_h)
$(UNITTEST_cpp) : $(UNITTEST_h)

$(DATABASE_UTILITY_o) : $(obj_TARGET)%.o : $(DATABASE_UTILITY_DIR)%.cpp
	g++ -std=c++11 -g -o $@ -c $<
$(VLPP_o) : $(obj_TARGET)%.o : $(VLPP_DIR)%.cpp
	g++ -std=c++11 -g -o $@ -c $<
$(UNITTEST_o) : $(obj_TARGET)%.o : $(UNITTEST_DIR)%.cpp
	g++ -std=c++11 -g -o $@ -c $<

clean:
	rm $(bin_TARGET)* -rf
	rm $(obj_TARGET)* -rf
