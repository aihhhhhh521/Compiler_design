%{
#include <string>
#include "node.h"
#include "parser.h"

#define YY_DECL int yylex(void)
%}

/* Flex options */
%option noyywrap
%option yylineno

/* Regex Definitions */
ID    [A-Za-z_][A-Za-z0-9_]*
DEC   [1-9][0-9]*|0

%%

"int"               { return INT; }
"return"            { return RETURN; }
"void"              { return VOID; }
"if"                { return IF; }
"else"              { return ELSE; }
"while"             { return WHILE; }
"continue"          { return CONTINUE; }
"break"             { return BREAK; }

{DEC}               { yylval.integer = atoi(yytext); return INTEGER_LITERAL; }
{ID}                { yylval.str = new std::string(yytext); return IDENTIFIER; }

"+"                 { return PLUS; }
"-"                 { return MINUS; }
"*"                 { return STAR; }
"/"                 { return SLASH; }
"%"                 { return PERCENT; }
"<"                 { return LT_OP; }
"<="                { return LE_OP; }
">"                 { return GT_OP; }
">="                { return GE_OP; }
"=="                { return EQ_OP; }
"!="                { return NE_OP; }
"&&"                { return AND_OP; }
"||"                { return OR_OP; }
"!"                 { return LOG_NOT; }
"~"                 { return BIT_NOT; }
"&"                 { return BIT_AND; }
"|"                 { return BIT_OR; }
"^"                 { return BIT_XOR; }
"="                 { return ASSIGN; }
";"                 { return SEMI; }
","                 { return COMMA; }
"("                 { return LPAREN; }
")"                 { return RPAREN; }
"{"                 { return LBRACE; }
"}"                 { return RBRACE; }

[ \t\n]+            { /* 忽略空白字符 */ }
"//".* { /* 忽略单行注释 */ }

.                   { fprintf(stderr, "Lexer error on line %d: Unexpected character '%s'\n", yylineno, yytext); }

%%