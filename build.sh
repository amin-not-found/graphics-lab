#/usr/bin/bash
set -xe

if [ -z "$1" ]
  then
    echo "No input file supplied as argument."
    exit -1
fi

CFLAGS="-Wall -Wextra -pedantic"
CFLAGS+=" -isystem ./ext/raygui -I./ext/raylib/include -I./include" 
LDFLAGS="-lraylib"
FILE=$(basename $1)
FILE=${FILE%%.*}

if [ -z ${DEBUG} ]; 
    then CFLAGS="${CFLAGS} -O2";
    else CFLAGS="${CFLAGS} -Og -g"; 
fi


# check if using windows(mingw)
if [[ $(grep Microsoft /proc/version) ]]; then
    CC="cc.exe"
    OUTPUT="${FILE}.exe"
    CFLAGS="${CFLAGS}"
    LDFLAGS+=" -L./ext/raylib/lib_mingw -lopengl32 -lgdi32 -lwinmm"
    # TODO : check why -mwindows changes window viewpoint
    #if [ -z ${DEBUG} ]; then CFLAGS="${CFLAGS} -mwindows"; fi
else # Assuming linux(not actually tested)
    CC="cc"
    OUTPUT="${FILE}.exe"
    LDFLAGS+=" -L./ext/raylib/lib_linux -lGL -lm -lpthread -ldl -lrt"
fi

${CC} ${CFLAGS} -o ./bin/${FILE} $1 ${LDFLAGS}
#${CC} ${CFLAGS} -E $1 ${LDFLAGS}



