# module directories
MDirs = . ./FSLib
Binaries = Test
Libs = pthread
# compiler
CC = gcc
# files
CFiles = $(foreach D, $(MDirs), $(wildcard $(D)/*.c))
OFiles = $(patsubst %.c, %.o, $(CFiles))
DFiles = $(patsubst %.c, %.d, $(CFiles))
# flags
OPT = -O0
DEP = -MP -MD # magic flags
CFlags = $(foreach D, $(MDirs),-I$(D)) $(OPT) $(DEP)
Libflags = $(addprefix -l,$(Libs))

all: $(Binaries)

$(Binaries): $(OFiles)
	$(CC) -o $@ -g $^ $(Libflags)

%.o: %.c
	$(CC) -c -o $@ -g $< $(CFlags)

list:
	 $(info $(CFiles))
	 $(info $(DFiles))
	 $(info $(OFiles))

.PHONY: clean

clean:
	rm -rf $(Binaries) $(OFiles) $(DFiles) null.d

# magic
-include $(DFiles)
