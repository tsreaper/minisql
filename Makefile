SRC := $(subst $(shell cd)\,,$(shell for /r . %%s in (*.cpp) do @echo %%s))
OBJ := $(patsubst src%.cpp,obj%.o,$(SRC))
FLAGS := -Wall -std=c++11 -I src -O2

minisql: $(OBJ)
	@echo Linking object files...
	@g++ $(OBJ) $(FLAGS) -o minisql
	@echo Creating data folders...
	@-mkdir data\catalog
	@-mkdir data\index
	@-mkdir data\record

$(OBJ): obj\\%.o: src\\%.cpp
	@echo Compiling $^
	@if not exist $(dir $@) mkdir $(dir $@)
	@g++ -c $^ $(FLAGS) -o $@

clean:
	@echo Cleaning object files...
	@rd /s /q obj
