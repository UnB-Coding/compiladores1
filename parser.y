/******************************************************
FGA0003 - Compiladores 1
Curso de Engenharia de Software
Universidade de Brasília (UnB)

Arquivo: parser.y
Descrição: Exemplo de gramática para expressão aritmética
******************************************************/


%{
#include <stdio.h>
#include <stdlib.h>
#include "symtab.h"
/* Declarações para evitar avisos de função implícita */
int yylex(void);
void yyerror(const char *s);
%}

/* Define valor semântico (intValue) */
%union {
    int intValue;
    char *strValue;
}

/* Token que carrega valor semântico */
%token <intValue> NUM
%token <strValue> ID

/* Tokens sem valor semântico, mas com precedência */
%token PLUS MINUS TIMES DIVIDE LPAREN RPAREN ASSIGN SEMICOLON

/* Tokens para palavras-chave de tipo */
%token T_INT T_FLOAT T_CHAR T_BOOL

/* Declara precedência:
   - PLUS e MINUS têm menor precedência
   - TIMES e DIVIDE têm maior precedência */
%left PLUS MINUS
%left TIMES DIVIDE

/* Associa não terminais aos tipos da %union */
%type <intValue> expr
%type <intValue> type_spec

%%

input:
      %empty
    | input stmt
    ;
stmt:
      expr SEMICOLON             { printf("Resultado: %d\n", $1); }
    | ID ASSIGN expr SEMICOLON   {
                                /* Atribuição: exige que a variável já tenha sido declarada */
                                SymEntry *e = sym_lookup($1);
                                if (e) {
                                    e->value = $3;
                                    printf("%s = %d\n", $1, $3);
                                } else {
                                    fprintf(stderr, "Erro: variável '%s' não declarada\n", $1);
                                }
                                free($1);
                            }
    | type_spec ID SEMICOLON {
                                /* Declaração de variável sem inicialização */
                                SymEntry *e = sym_lookup($2);
                                if (e) {
                                    fprintf(stderr, "Erro: variável '%s' já declarada\n", $2);
                                } else {
                                    sym_set($2, (SymType)$1, 0);
                                    printf("Declarado: %s : %s\n", $2, sym_type_name((SymType)$1));
                                }
                                free($2);
                            }
    | type_spec ID ASSIGN expr SEMICOLON {
                                /* Declaração de variável com inicialização */
                                SymEntry *e = sym_lookup($2);
                                if (e) {
                                    fprintf(stderr, "Erro: variável '%s' já declarada\n", $2);
                                } else {
                                    sym_set($2, (SymType)$1, $4);
                                    printf("Declarado: %s : %s = %d\n", $2, sym_type_name((SymType)$1), $4);
                                }
                                free($2);
                            }
    ;

type_spec:
      T_INT     { $$ = TYPE_INT; }
    | T_FLOAT   { $$ = TYPE_FLOAT; }
    | T_CHAR    { $$ = TYPE_CHAR; }
    | T_BOOL    { $$ = TYPE_BOOL; }
    ;

expr:
      expr PLUS expr    { $$ = $1 + $3; }
    | expr MINUS expr   { $$ = $1 - $3; }
    | expr TIMES expr   { $$ = $1 * $3; }
    | expr DIVIDE expr  { $$ = $1 / $3; }
    | LPAREN expr RPAREN{ $$ = $2; }
    | NUM               { $$ = $1; }
    | ID                {
                            SymEntry *e = sym_lookup($1);
                            if (e) {
                                $$ = e->value;
                            } else {
                                printf("Aviso: variável '%s' não definida\n", $1);
                                $$ = 0;
                            }
                            free($1);
                          }
    ;

%%

int main(void) {
    int result = yyparse();
    sym_print();
    sym_free();
    return result;
}

void yyerror(const char *s) {
    fprintf(stderr, "Erro sintático: %s\n", s);
}