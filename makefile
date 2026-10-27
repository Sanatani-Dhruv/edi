O=edi

SRC=$(shell find -name "*.c")
HEADER=$(shell find -name "*.h")

all: ${O} run

${O}: ${SRC} ${HEADER}
	cc ${CFLAG} ${SRC} -o ${O}

run: ${O}
	${CFLAG} ./${O}
