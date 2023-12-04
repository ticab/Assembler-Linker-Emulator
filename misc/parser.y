%{
	#include "flexb.hpp"
	#include <stdio.h>
  extern FILE* yyin;
	int yylex(void);
	void yyerror(const char*);
%}

%output "parser.c"
%defines "parser.h"

%union {
	int         num;
	char       *ident;
	struct Arg  *arg;
}

%token PLUS
%token MINUS
%token PAR1
%token PAR2
%token COMMA
%token NL

%token <num>   NUM
%token <ident> IDENT

%type <arg> arg;
%type <arg> args;

%%

prog:
  | instr prog
  ;

instr: 
  IDENT args NL
    { make_instruction($1); }
  | NL {}

args:  
  { $$ = NULL; }
  | arg args
    { add($1); }
  | arg COMMA args
    { add($1); }
  ;

arg
  : PAR1 IDENT PLUS NUM[num] PAR2
    { if($num==0) { $$ = make_argument1($2); }
      else { $$ = make_argument($2, $num); }}
  | PAR1 IDENT PLUS IDENT PAR2
    { $$ = make_argument2($2, $4); }  
  | PAR1 IDENT PAR2
    { $$ = make_argument1($2); }
  | IDENT
    { $$ = make_argument($1, 0); }
  | NUM[num]
    { $$ = make_argument(NULL, $num);}
  | IDENT MINUS IDENT
     { $$ = make_argument3($1, $3, 0); }  
  | IDENT PLUS IDENT
     { $$ = make_argument3($1, $3, 1); }  
  ;

%%
