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

/* Literais booleanos (carregam intValue: 1 para true, 0 para false) */
%token <intValue> TRUE_LIT FALSE_LIT

/* Tokens sem valor semântico, mas com precedência */
%token PLUS MINUS TIMES DIVIDE LPAREN RPAREN ASSIGN SEMICOLON

/* Tokens para palavras-chave de tipo */
%token T_INT T_FLOAT T_CHAR T_BOOL

/* Operadores relacionais, de igualdade e lógicos */
%token AND OR NOT EQ NE LT GT LE GE

/* Declara precedência (menor → maior):
   - OR  tem menor precedência
   - NOT tem maior precedência (unário) */
%left OR
%left AND
%left EQ NE
%left LT GT LE GE
%left PLUS MINUS
%left TIMES DIVIDE
%right NOT

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
                                if (!e) {
                                    fprintf(stderr, "Erro: variável '%s' não declarada\n", $1);
                                    free($1);
                                    YYABORT;
                                }
                                /* Normaliza valor para bool (C-style) */
                                if (e->type == TYPE_BOOL) {
                                    e->value = !!$3;
                                } else {
                                    e->value = $3;
                                }
                                printf("%s %s = %d\n", sym_type_name(e->type), $1, e->value);
                                free($1);
                            }
    | type_spec ID SEMICOLON {
                                /* Declaração de variável sem inicialização */
                                SymEntry *e = sym_lookup($2);
                                if (e) {
                                    fprintf(stderr, "Erro: variável '%s' já declarada\n", $2);
                                    free($2);
                                    YYABORT;
                                }
                                sym_set($2, (SymType)$1, 0);
                                printf("Declarado: %s : %s\n", $2, sym_type_name((SymType)$1));
                                free($2);
                            }
    | type_spec ID ASSIGN expr SEMICOLON {
                                /* Declaração de variável com inicialização */
                                SymEntry *e = sym_lookup($2);
                                if (e) {
                                    fprintf(stderr, "Erro: variável '%s' já declarada\n", $2);
                                    free($2);
                                    YYABORT;
                                }
                                /* Normaliza valor para bool (C-style) */
                                int val = ($1 == TYPE_BOOL) ? !!$4 : $4;
                                sym_set($2, (SymType)$1, val);
                                printf("Declarado: %s : %s = %d\n", $2, sym_type_name((SymType)$1), val);
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
    | expr DIVIDE expr  {
                            if ($3 == 0) {
                                yyerror("divisão por zero");
                                YYABORT;
                            }
                            $$ = $1 / $3;
                        }
    | expr AND expr     { $$ = $1 && $3; }
    | expr OR expr      { $$ = $1 || $3; }
    | NOT expr          { $$ = !$2; }
    | expr EQ expr      { $$ = ($1 == $3); }
    | expr NE expr      { $$ = ($1 != $3); }
    | expr LT expr      { $$ = ($1 < $3); }
    | expr GT expr      { $$ = ($1 > $3); }
    | expr LE expr      { $$ = ($1 <= $3); }
    | expr GE expr      { $$ = ($1 >= $3); }
    | LPAREN expr RPAREN{ $$ = $2; }
    | NUM               { $$ = $1; }
    | TRUE_LIT          { $$ = $1; }
    | FALSE_LIT         { $$ = $1; }
    | ID                {
                            SymEntry *e = sym_lookup($1);
                            if (!e) {
                                fprintf(stderr, "Erro: variável '%s' não declarada\n", $1);
                                free($1);
                                YYABORT;
                            }
                            $$ = e->value;
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