SRCS = ../src/assembler.cpp flexb.cpp parser.c lexer.c
SRCS1 = ../src/linker.cpp
SRCS2 = ../src/emulator.cpp
CC = g++

test: all
	$(CC) $(SRCS) -o ../testsA/assembler
	$(CC) $(SRCS1) -o ../testsA/linker
	$(CC) $(SRCS2) -o ../testsA/emulator

all: $(SRCS) flexb.hpp

lexer.c: lexer.l flexb.hpp
	flex lexer.l

parser.c: parser.y lexer.l flexb.hpp
	bison parser.y

clean:
	rm -rf *.o lexer.c lexer.h parser.c parser.h assembler
