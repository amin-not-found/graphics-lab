#/usr/bin/bash
set -xe

CFLAGS="-Wall -Wextra -pedantic -g"
CFLAGS+=" -isystem ./ext/raygui -I./ext/raylib/include" 
LDFLAGS="-lraylib"
FILE="game_of_life"

# check if using windows(wsl)
if [[ $(grep Microsoft /proc/version) ]]; then
    CC="cc.exe"
    OUTPUT="${FILE}.exe"
    LDFLAGS+=" -L./ext/raylib/lib_mingw -lopengl32 -lgdi32 -lwinmm"
else # Assuming linux(not actually tested)
    CC="cc"
    OUTPUT="${FILE}.exe"
    LDFLAGS+=" -L./ext/raylib/lib_linux -lGL -lm -lpthread -ldl -lrt"
fi



${CC} ${CFLAGS} -o ${FILE} ./${FILE}.c ${LDFLAGS}

./${OUTPUT}


