#include "flexb.hpp"
#include <iostream>
#include <list>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
using namespace std;

list<Arg*> arg_list;
list<Instr*> instr_list;

char* copy_str(const char *in){
	size_t len = strnlen(in, 32);
	char* nov = new char[len + 1];
	strncpy(nov, in, len);
	nov[len] = '\0';
	return nov;
}

Arg* make_argument(char *reg, int off){
	Arg* a = new Arg();
	a->name  = reg;
	a->offset  = off;
	return a;
}
Arg* make_argument1(char *reg){
	Arg* a = new Arg();
	string n = reg;
	char* nreg = new char[n.size()+2];
	nreg[0]='[';
	for(int i=1; i<n.size()+1; i++){
		nreg[i]=reg[i-1];
	}
	nreg[n.size()+1]=']';
	a->name=nreg;
	a->offset = 0;
	return a;
}

Arg* make_argument2(char* reg, char* sim){
	Arg* a = new Arg();
	a->name  = reg;
	a->name2 = sim;
	return a;
}
Arg* make_argument3(char* reg, char* sim, int op){
	Arg* a = new Arg();
	a->name=reg;
	a->name2=sim;
	a->op=op;
	return a;
}
void add(Arg* a){
	arg_list.push_front(a);
}

Instr* make_instruction(char *name){
	Instr* i = new Instr();
	i->name = name;
	i->arg = arg_list;
	arg_list.clear();
	instr_list.push_back(i);
	return i;
}

