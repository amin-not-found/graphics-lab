#/usr/bin/bash
set -xe

if [ -z "$1" ]
  then
    echo "No input file supplied as argument."
    exit -1
fi

if [[ $(grep Microsoft /proc/version) ]]; then
    MAKE=make.exe
else 
    MAKE=make
fi

FILE=$(basename $1)
FILE=${FILE%%.*}

${MAKE} DEBUG=1 ${FILE} 
./bin/${FILE}.exe