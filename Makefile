CC = gcc
BIN := ./bin
SRC := ./src
SRCS := $(wildcard $(SRC)/*.c)
PROGRAMS := $(patsubst $(SRC)/%.c,%,$(SRCS))
CFLAGS := -Wall -Wextra -pedantic
CFLAGS += -isystem ./ext/raygui -I./ext/raylib/include -I./include
LDLIBS := -lraylib
LDFLAGS :=
DEBUG ?= 0

ifeq ($(DEBUG),0)
	CFLAGS += -O2
else
	CFLAGS += -Og -g
endif

platform != uname -s
ifeq ($(platform),Linux)
    LDLIBS  += -lGL -lm -lpthread -ldl -lrt
	LDFLAGS += -L./ext/raylib/lib_linux
else ifeq ($(platform),Windows_NT)
    LDLIBS  += -lopengl32 -lgdi32 -lwinmm
    LDFLAGS += -L./ext/raylib/lib_mingw 
	#TODO : check why -mwindows changes window viewpoint
    #if [ -z ${DEBUG} ]; then CFLAGS="${CFLAGS} -mwindows"; fi
else
	@echo Unsupported platform $(platform).
	exit -1
endif


.PHONY: $(PROGRAMS)

default:
	@echo No default target exists as of right now.
	@echo Please specify a target

$(PROGRAMS): % : $(SRC)/%.c
	$(CC) $(CFLAGS) -o ./bin/$@ $(SRC)/$@.c $(LDFLAGS) $(LDLIBS)