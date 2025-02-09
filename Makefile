CC            := gcc
BIN           := ./bin
SRC           := ./src
WEB           := ./web
SRCS          := $(wildcard $(SRC)/*.c)
PROGRAMS      := $(patsubst $(SRC)/%.c,%,$(SRCS))
CFLAGS        := -Wall -Wextra -pedantic
INC           := -isystem ./ext/raygui -I./ext/raylib/include -I./include
LDLIBS        := -lraylib
LDFLAGS       := 
EMSDK_VERSION  = 
DEBUG         ?= 0


ifeq ($(DEBUG),0)
	CFLAGS += -O2
else
	CFLAGS += -Og -g -DDEBUG
endif

ifeq ($(OS),Windows_NT)    
	LDLIBS  += -lopengl32 -lgdi32 -lwinmm
    LDFLAGS += -L./ext/raylib/lib_mingw 
	#TODO : check why -mwindows changes window viewpoint
    #if [ -z ${DEBUG} ]; then CFLAGS="${CFLAGS} -mwindows"; fi
else 
	platform := $(shell uname -s)
	ifeq ($(platform),Linux)
		LDLIBS  += -lGL -lm -lpthread -ldl -lrt
		LDFLAGS += -L./ext/raylib/lib_linux
	else
		$(info Unsupported platform $(platform).)
		exit -1
	endif
endif


.PHONY: $(PROGRAMS) default gen-web

default:
	$(info No default target exists as of right now.)
	$(info Please specify a target)

$(PROGRAMS): % : $(SRC)/%.c
	$(CC) $(CFLAGS) $(INC) -o ./bin/$@ $(SRC)/$@.c $(LDFLAGS) $(LDLIBS)

.SECONDEXPANSION:
$(WEB)/%: $(SRC)/$$**.c ./shell.html
	mkdir -p $@ 
	emcc -o $@/index.html $(SRC)/$*.c -Os $(INC) ./ext/raylib/lib_web/libraylib.a \
	-L. -L./ext/raylib/lib_web/libraylib.a -s \
	USE_GLFW=3 --shell-file ./shell.html -DPLATFORM_WEB

gen-web: $(patsubst $(SRC)/%.c,$(WEB)/%,$(SRCS))
	