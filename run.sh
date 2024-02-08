#/usr/bin/bash
set -xe

if [ -z "$1" ]
  then
    echo "No input file supplied as argument."
    exit -1
fi

FILE=$(basename $1)
FILE=${FILE%%.*}

make DEBUG=1 ${FILE} && ./bin/${FILE}.exe