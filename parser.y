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
%token PLUS MINUS TIMES DIVIDE LPAREN RPAREN ASSIGN

/* Declara precedência:
   - PLUS e MINUS têm menor precedência
   - TIMES e DIVIDE têm maior precedência */
%left PLUS MINUS
%left TIMES DIVIDE

/* Associa o não terminal expr ao tipo intValue */
%type <intValue> expr

%%

input:
      %empty
    | input line
    ;
line:
      '\n'                  { /* linha vazia */ }
    | expr '\n'             { printf("Resultado: %d\n", $1); }
    | ID ASSIGN expr '\n'   { sym_set($1, $3); printf("%s = %d\n", $1, $3); free($1); }
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