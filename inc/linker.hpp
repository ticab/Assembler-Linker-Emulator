#ifndef _LINKER_H_
#define _LINKER_H_
#include <stdio.h>
#include <cmath>
#include <iostream>
#include <list>
#include <stdlib.h>
#include <string.h>
#include "structs.hpp"
#include <unordered_map>

using namespace std;

typedef enum{
  HEX, RELOC, DEF
}File_type;

File_type type = DEF;

list<Symbol*> symbols_table;
list<Symbol*> undefined_symbols;
list<Section*> sections;
list<Section*> pl;
list<Reloc*> relocation_table;
int max_adr;
unordered_map<string, int> place;
unordered_map<string, list<int>> file_sections;
list<Section*>::iterator it;

void passHex(FILE* in, int id);

void passRel(FILE* in, int id);

void print_into_file(FILE* out);

void add_st_adr();

void resolve_reloc();

void resolve_reloc_r();

void add_place(char* name);

void set_place(FILE* in);

void make_symbol_table(FILE* in, int id);

bool in_symbol_table(const char* name);

void clear_undefined();

bool is_undefined(const char* name);

bool has_symbol(const char* name, Section* s);

void print_hex(FILE* out);


#endif