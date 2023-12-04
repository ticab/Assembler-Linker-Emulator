#ifndef _FLEXB_H_
#define _FLEXB_H_
#include <iostream>
#include <list>
using namespace std;

struct Arg{
	char* name;
	int offset;
	char* name2;
	int op=-1;
};
class Instr{
	public:
		char* name;
		list<Arg*> arg;
};

char* copy_str(const char*);

Arg* make_argument(char*, int);
Arg* make_argument1(char*);
Arg* make_argument2(char*, char*);
Arg* make_argument3(char*, char*, int);
Instr* make_instruction(char*);
void add(Arg*);

#endif

