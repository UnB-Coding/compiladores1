/******************************************************
FGA0003 - Compiladores 1
Curso de Engenharia de Software
Universidade de Brasília (UnB)

Arquivo: scanner.l
Descrição: Analisador léxico (Flex) para o compilador

Retorna tokens para literais, operadores, palavras-chave
de tipo, controle de fluxo (if/else/while/for) e
delimitadores de bloco.
******************************************************/

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"  /* contém definições geradas pelo Bison (NUM, PLUS, etc.) */
%}

%%

 /* Literais de ponto flutuante (antes de inteiros para maximal munch) */
[0-9]+\.[0-9]*|\.[0-9]+  {
    yylval.floatValue = atof(yytext);
    return FLOAT_LIT;
}

[0-9]+  {
    yylval.intValue = atoi(yytext);
    return NUM;
}

 /* Literais de caractere: caractere simples entre aspas simples */
'[^\\']'  {
    yylval.charValue = yytext[1];
    return CHAR_LIT;
}

 /* Literais de caractere: sequências de escape padrão C */
'\\.'     {
    switch (yytext[2]) {
        case 'a':  yylval.charValue = '\a'; break;
        case 'b':  yylval.charValue = '\b'; break;
        case 'f':  yylval.charValue = '\f'; break;
        case 'n':  yylval.charValue = '\n'; break;
        case 'r':  yylval.charValue = '\r'; break;
        case 't':  yylval.charValue = '\t'; break;
        case 'v':  yylval.charValue = '\v'; break;
        case '\\': yylval.charValue = '\\'; break;
        case '\'': yylval.charValue = '\''; break;
        case '"':  yylval.charValue = '"';  break;
        case '?':  yylval.charValue = '?';  break;
        case '0':  yylval.charValue = '\0'; break;
        default:
            fprintf(stderr, "Sequência de escape inválida: %s\n", yytext);
            yylval.charValue = yytext[2];
            break;
    }
    return CHAR_LIT;
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

 /* Palavras-chave de controle de fluxo (antes de identificadores) */
"if"        { return KW_IF; }
"else"      { return KW_ELSE; }
"while"     { return KW_WHILE; }
"for"       { return KW_FOR; }

 /* Literais booleanos (antes da regra de identificador) */
"true"      { yylval.intValue = 1; return TRUE_LIT; }
"false"     { yylval.intValue = 0; return FALSE_LIT; }

 /* Delimitadores de bloco */
"{"         { return LBRACE; }
"}"         { return RBRACE; }

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