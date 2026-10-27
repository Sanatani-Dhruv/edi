O=edi

PREFIX=

SRC=$(shell find -name "*.c")
HEADER=$(shell find -name "*.h")

all: ${O}

${O}: ${SRC} ${HEADER}
	cc ${CFLAG} ${SRC} -o ${O}

run: ${O}
	${CFLAG} ./${O}

install: ${O}
	cp ${O} ${PREFIX}/usr/local/bin/
