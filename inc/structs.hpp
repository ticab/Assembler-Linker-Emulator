#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include <stdio.h>
#include <list>
using namespace std;

class Symbol{
	public:
    int num;
    int value;
    const char* type;
    const char* bind;
    const char* ndx;
		const char* name;
    int size=0;
    Symbol(int n, int v, const char* t, const char* b, const char* nd, const char* na):
      num(n),value(v),type(t),bind(b),name(na),ndx(nd) 
      {}
    Symbol():type(NULL), bind(NULL), name(NULL), ndx(0), num(0), value(0){}
};

class Section{
  public:
    list<char> section_mem;
    int counter=0;
    const char* name;
    int table_ndx;
    int size=0;
    int start_adr=0;
    list<const char*> symbols;
    Section(const char* n, int table):name(n), table_ndx(table){}

    void push_mem(int val){
      section_mem.push_back(val>>24 & 0xff);
      section_mem.push_back(val>>16 & 0xff);
      section_mem.push_back(val>>8 & 0xff);
      section_mem.push_back(val & 0xff);
    }
};

class Reloc{
  public:
    int offset;
    const char* type;
    const char* symbol_name;
    int addend;
    int addend2;
    Reloc(int of, const char* t, const char* s_n, int a, int a2):
      offset(of), type(t), symbol_name(s_n), addend(a), addend2(a2){}
};

typedef enum{
  SCTN, GLOB, EXT, DIR, INSTR, LABEL
}Type;

#endif