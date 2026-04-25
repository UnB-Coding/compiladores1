/******************************************************
FGA0003 - Compiladores 1
Curso de Engenharia de Software
Universidade de Brasília (UnB)

Arquivo: lexer/lexer.l
Descrição: Analisador léxico para subconjunto da linguagem C
           (transpilador C → Python)
******************************************************/

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Contador de linhas para mensagens de erro */
int line_number = 1;

/* ===== Definição dos tokens =====
   Quando integrado com o Bison, remover este enum
   e descomentar: #include "parser.tab.h"          */

/* #include "parser.tab.h" */

enum {
    /* Palavras-chave: tipos */
    KW_INT = 256, KW_FLOAT, KW_CHAR, KW_LONG,
    KW_DOUBLE, KW_SHORT, KW_SIGNED, KW_UNSIGNED,

    /* Palavras-chave: controle de fluxo */
    KW_IF, KW_ELSE, KW_WHILE, KW_FOR, KW_DO,
    KW_SWITCH, KW_CASE, KW_BREAK, KW_CONTINUE,
    KW_RETURN,

    /* Literais */
    LIT_INT, LIT_FLOAT, LIT_STRING, LIT_CHAR,

    /* Identificador */
    IDENTIFIER,

    /* Operadores aritméticos */
    OP_PLUS, OP_MINUS, OP_MULT, OP_DIV,

    /* Operadores de atribuição */
    OP_ASSIGN,

    /* Operadores relacionais */
    OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE,

    /* Operadores lógicos */
    OP_AND, OP_OR, OP_NOT,

    /* Delimitadores */
    LBRACE, RBRACE, LPAREN, RPAREN,
    LBRACKET, RBRACKET, SEMICOLON, COMMA
};
%}

%option noyywrap

%%

 /* ===== COMENTÁRIOS (ignorar) ===== */
"//".*                          { /* comentário de linha */ }
"/*"([^*]|\*+[^*/])*\*+"/"     {
                                    for (int i = 0; yytext[i]; i++) {
                                        if (yytext[i] == '\n') line_number++;
                                    }
                                }

 /* ===== PALAVRAS-CHAVE: Tipos ===== */
"int"           { return KW_INT; }
"float"         { return KW_FLOAT; }
"char"          { return KW_CHAR; }
"long"          { return KW_LONG; }
"double"        { return KW_DOUBLE; }
"short"         { return KW_SHORT; }
"signed"        { return KW_SIGNED; }
"unsigned"      { return KW_UNSIGNED; }

 /* ===== PALAVRAS-CHAVE: Controle de fluxo ===== */
"if"            { return KW_IF; }
"else"          { return KW_ELSE; }
"while"         { return KW_WHILE; }
"for"           { return KW_FOR; }
"do"            { return KW_DO; }
"switch"        { return KW_SWITCH; }
"case"          { return KW_CASE; }
"break"         { return KW_BREAK; }
"continue"      { return KW_CONTINUE; }
"return"        { return KW_RETURN; }

 /* ===== OPERADORES MULTI-CHAR (antes dos single-char) ===== */
"=="            { return OP_EQ; }
"!="            { return OP_NE; }
"<="            { return OP_LE; }
">="            { return OP_GE; }
"&&"            { return OP_AND; }
"||"            { return OP_OR; }

 /* ===== OPERADORES SINGLE-CHAR ===== */
"+"             { return OP_PLUS; }
"-"             { return OP_MINUS; }
"*"             { return OP_MULT; }
"/"             { return OP_DIV; }
"="             { return OP_ASSIGN; }
"<"             { return OP_LT; }
">"             { return OP_GT; }
"!"             { return OP_NOT; }

 /* ===== DELIMITADORES ===== */
"{"             { return LBRACE; }
"}"             { return RBRACE; }
"("             { return LPAREN; }
")"             { return RPAREN; }
"["             { return LBRACKET; }
"]"             { return RBRACKET; }
";"             { return SEMICOLON; }
","             { return COMMA; }

 /* ===== LITERAIS ===== */
[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?   { return LIT_FLOAT; }
[0-9]+[eE][+-]?[0-9]+              { return LIT_FLOAT; }
[0-9]+                              { return LIT_INT; }
'(\\.|[^'\\])'                      { return LIT_CHAR; }
\"(\\.|[^"\\])*\"                   { return LIT_STRING; }

 /* ===== IDENTIFICADORES ===== */
[a-zA-Z_][a-zA-Z0-9_]*             { return IDENTIFIER; }

 /* ===== WHITESPACE ===== */
\n              { line_number++; }
[ \t\r]+        { /* ignora */ }

 /* ===== CARACTERE NÃO RECONHECIDO ===== */
.               { fprintf(stderr, "Erro léxico na linha %d: caractere não reconhecido '%s'\n", line_number, yytext); }

%%
