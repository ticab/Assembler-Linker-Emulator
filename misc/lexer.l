%{
#include "parser.h"
#include "flexb.hpp"
%}

%option outfile="lexer.c" header-file="lexer.h"

%option noyywrap

PLUS  "+"
MINUS "-"
COMMA ","
PAR1  "["
PAR2  "]"

NUM   [0-9]*
HEX   0[Xx][0-9A-Fa-f]+
STR   ["][a-zA-Z0-9 ]*["]
IDENT [A-Za-z_%$.][A-Za-z0-9_:]*
COM   "#"[^\n\r]*
WS    [ \r\t]*
NEWL  [\n]

%%

{WS}    { }
{COM}   { }
{NEWL}	{ return NL; }
{NUM}		{ sscanf(yytext, "%d", &yylval.num);
			  	return NUM;}
{HEX}		{ sscanf(yytext, "%x", &yylval.num);
			  	return NUM;}
{STR}    { yylval.ident = copy_str(yytext);
					return IDENT;
					}
{IDENT}     {
	          yylval.ident = copy_str(yytext);
			  return IDENT;
            }
{PLUS}      { return PLUS; }
{MINUS}     { return MINUS; }
{PAR1}      { return PAR1; }
{PAR2}      { return PAR2; }
{COMMA}     { return COMMA; }
%%

int yyerror(const char* msg)
{
    fprintf(stderr, "lexer error: %s\n", msg);
    return 0;
}
