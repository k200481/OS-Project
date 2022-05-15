# module directories
MDirs = ./Main ./FSP ./Disk ./BlockManager $(foreach D, ./Elements, $(wildcard $(D)/*))
Binaries = FSProc
Libs = pthread
# compiler
CC = g++
# files
CFiles = $(foreach D, $(MDirs), $(wildcard $(D)/*.cpp))
OFiles = $(patsubst %.cpp, %.o, $(CFiles))
DFiles = $(patsubst %.cpp, %.d, $(CFiles))
# flags
OPT = -O0
DEP = -MP -MD # magic flags
CFlags = $(foreach D, $(MDirs),-I$(D)) $(OPT) $(DEP) -std=c++17
Libflags = $(addprefix -l,$(Libs))

all: $(Binaries)

$(Binaries): $(OFiles)
	$(CC) -o $@ -g $^ $(Libflags)

%.o: %.cpp
	$(CC) -c -o $@ -g $< $(CFlags)

list:
	 $(info $(CFiles))
	 $(info $(DFiles))
	 $(info $(OFiles))

.PHONY: clean

clean:
	rm -rf $(Binaries) $(OFiles) $(DFiles) null.d bin

# magic
-include $(DFiles)
