/******************************************************
FGA0003 - Compiladores 1
Curso de Engenharia de Software
Universidade de Brasília (UnB)

Arquivo: lexer.l
Descrição: Exemplo de analisador léxico (tokens aritméticos)
******************************************************/

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"  /* contém definições geradas pelo Bison (NUM, PLUS, etc.) */
%}

%%

[0-9]+  {
    yylval.intValue = atoi(yytext);
    return NUM;
}

"+"     { return PLUS; }
"-"     { return MINUS; }
"*"     { return TIMES; }
"/"     { return DIVIDE; }
"("     { return LPAREN; }
")"     { return RPAREN; }
";"     { return SEMICOLON; }

 /* Operadores relacionais e de igualdade (multi-caractere antes dos simples) */
"=="    { return EQ; }
"!="    { return NE; }
"<="    { return LE; }
">="    { return GE; }
"<"     { return LT; }
">"     { return GT; }

 /* Operadores lógicos */
"&&"    { return AND; }
"||"    { return OR; }
"!"     { return NOT; }

 /* Atribuição (depois de == e !=) */
"="     { return ASSIGN; }

 /* Palavras-chave de tipo */
"int"       { return T_INT; }
"float"     { return T_FLOAT; }
"char"      { return T_CHAR; }
"bool"      { return T_BOOL; }

 /* Literais booleanos (antes da regra de identificador) */
"true"      { yylval.intValue = 1; return TRUE_LIT; }
"false"     { yylval.intValue = 0; return FALSE_LIT; }

[a-zA-Z_][a-zA-Z0-9_]*  {
    yylval.strValue = strdup(yytext);
    return ID;
}

[ \t\n]+  { /* ignora espaços, tabs e quebras de linha */ }

.        { printf("Caractere não reconhecido: %s\n", yytext); }

%%

int yywrap(void) {
    return 1;
}