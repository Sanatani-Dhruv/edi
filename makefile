O=build/edi

BUILD=build

PREFIX=

SRC=$(shell find -name "*.c")
HEADER=$(shell find -name "*.h")

all: ${O}

${O}: ${BUILD} ${SRC} ${HEADER}
	cc ${CFLAG} ${SRC} -o ${O}

${BUILD}:
	mkdir -p build

run: ${O}
	${RPFLAG} ./${O} ${RFLAG}

install: ${O}
	cp ${O} ${PREFIX}/usr/local/bin/
