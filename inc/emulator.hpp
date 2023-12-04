#ifndef _EMULATOR_H_
#define _EMULATOR_H_
#include <stdio.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <map>

using namespace std;

vector<int> gpr;
vector<int> csr;
map<int, char> memory;
bool end_e;
int term_out;

char a, b, c, modif, instr;
int d, t=0;
clock_t start_time;
double time_cfg=0.5;
int pre=0;

void read_hex(FILE* in);
void set_time_cfg(int x);

void load();
void store();
void shift();
void logic();
void arith();
void aswap();
void jump();
void call();
void inti();
void halt();

void execute();

void print_regs();


#endif