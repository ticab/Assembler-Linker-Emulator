#ifndef _ASSEMBLER_H_
#define _ASSEMBLER_H_

#include "../misc/flexb.hpp"
#include "../misc/lexer.h"
#include "../misc/parser.h"
#include "../inc/structs.hpp"

struct Tns{
	char* name;
	Arg* arg;
  Tns(char* n, Arg* a):name(n),arg(a){}
};

Section* section = NULL;

list<Symbol*> symbols_table;
list<Section*> sections;
list<Reloc*> relocation_table;
list<Tns*> tns_table;

void add_to_table(Type t, const char* name);

void add_to_reloc(const char* symbol_name, int offset, const char* type, int add);

bool is_directive(const Instr* i);

int reg_to_int(const char* reg);

bool is_instruction1(const Instr* i);

bool is_directive2(const Instr* i);

bool is_instruction2(const Instr* i);

bool is_label(const char* i);

void pass1();

void middle_pass();

void pass2();

void print_into_file(FILE* out);

#endif