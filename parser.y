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

/* Imprime um valor formatado de acordo com o tipo */
static void print_value(int type, double val) {
    switch ((SymType)type) {
        case TYPE_FLOAT: printf("%g", val); break;
        case TYPE_CHAR: {
            char c = (char)(int)val;
            if (c >= 32 && c < 127)
                printf("'%c'", c);
            else
                printf("%d", (int)c);
            break;
        }
        case TYPE_BOOL:  printf("%s", ((int)val) ? "true" : "false"); break;
        default:         printf("%d", (int)val); break;
    }
}

/* Promoção de tipo C-standard: float domina; char/bool promovem a int */
static int promote_type(int a, int b) {
    if (a == TYPE_FLOAT || b == TYPE_FLOAT) return TYPE_FLOAT;
    return TYPE_INT;
}

/* Converte valor double para SymValue de acordo com o tipo destino */
static SymValue to_sym_value(SymType type, double val) {
    SymValue v;
    switch (type) {
        case TYPE_FLOAT: v.fVal = (float)val; break;
        case TYPE_CHAR:  v.cVal = (char)(int)val; break;
        case TYPE_BOOL:  v.iVal = !!((int)val); break;
        default:         v.iVal = (int)val; break;
    }
    return v;
}

/* Extrai valor double de SymEntry */
static double sym_val_as_double(const SymEntry *e) {
    switch (e->type) {
        case TYPE_FLOAT: return (double)e->value.fVal;
        case TYPE_CHAR:  return (double)e->value.cVal;
        default:         return (double)e->value.iVal;
    }
}
%}

/* Define valor semântico */
%union {
    int    intValue;    /* NUM, literais booleanos, type_spec */
    double floatValue;  /* FLOAT_LIT */
    char   charValue;   /* CHAR_LIT */
    char  *strValue;    /* identificadores */
    struct {
        double val;     /* valor numérico (double para uniformidade) */
        int    type;    /* SymType do resultado */
    } exprVal;          /* expressões tipadas */
}

/* Tokens que carregam valor semântico */
%token <intValue>   NUM
%token <floatValue> FLOAT_LIT
%token <charValue>  CHAR_LIT
%token <strValue>   ID

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
   - NOT/UMINUS tem maior precedência (unários) */
%left OR
%left AND
%left EQ NE
%left LT GT LE GE
%left PLUS MINUS
%left TIMES DIVIDE
%right NOT UMINUS

/* Associa não terminais aos tipos da %union */
%type <exprVal>  expr
%type <intValue> type_spec

%%

program:
      line_list
    ;

line_list:
      %empty
    | line_list line
    ;

line:
      stmt
    ;

stmt:
      expr SEMICOLON             {
                                    printf("Resultado: ");
                                    print_value($1.type, $1.val);
                                    printf("\n");
                                 }
    | ID ASSIGN expr SEMICOLON   {
                                    /* Atribuição: exige que a variável já tenha sido declarada */
                                    SymEntry *e = sym_lookup($1);
                                    if (!e) {
                                        fprintf(stderr, "Erro: variável '%s' não declarada\n", $1);
                                        free($1);
                                        YYABORT;
                                    }
                                    /* Converte o valor para o tipo da variável */
                                    e->value = to_sym_value(e->type, $3.val);
                                    printf("%s %s = ", sym_type_name(e->type), $1);
                                    print_value(e->type, sym_val_as_double(e));
                                    printf("\n");
                                    free($1);
                                 }
    | type_spec ID SEMICOLON     {
                                    /* Declaração de variável sem inicialização */
                                    SymEntry *e = sym_lookup($2);
                                    if (e) {
                                        fprintf(stderr, "Erro: variável '%s' já declarada\n", $2);
                                        free($2);
                                        YYABORT;
                                    }
                                    SymValue v = to_sym_value((SymType)$1, 0.0);
                                    sym_set($2, (SymType)$1, v);
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
                                    SymValue v = to_sym_value((SymType)$1, $4.val);
                                    sym_set($2, (SymType)$1, v);
                                    printf("Declarado: %s : %s = ", $2, sym_type_name((SymType)$1));
                                    print_value($1, sym_val_as_double(sym_lookup($2)));
                                    printf("\n");
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
    /* Operadores aritméticos (aritmética inteira quando ambos não-float) */
      expr PLUS expr    {
                            $$.type = promote_type($1.type, $3.type);
                            if ($$.type == TYPE_FLOAT)
                                $$.val = $1.val + $3.val;
                            else
                                $$.val = (double)((int)$1.val + (int)$3.val);
                        }
    | expr MINUS expr   {
                            $$.type = promote_type($1.type, $3.type);
                            if ($$.type == TYPE_FLOAT)
                                $$.val = $1.val - $3.val;
                            else
                                $$.val = (double)((int)$1.val - (int)$3.val);
                        }
    | expr TIMES expr   {
                            $$.type = promote_type($1.type, $3.type);
                            if ($$.type == TYPE_FLOAT)
                                $$.val = $1.val * $3.val;
                            else
                                $$.val = (double)((int)$1.val * (int)$3.val);
                        }
    | expr DIVIDE expr  {
                            if ($3.val == 0.0) {
                                yyerror("divisão por zero");
                                YYABORT;
                            }
                            $$.type = promote_type($1.type, $3.type);
                            if ($$.type == TYPE_FLOAT)
                                $$.val = $1.val / $3.val;
                            else
                                $$.val = (double)((int)$1.val / (int)$3.val);
                        }
    /* Menos unário */
    | MINUS expr %prec UMINUS {
                            $$.val = -$2.val;
                            $$.type = $2.type;
                        }
    /* Operadores lógicos (resultado sempre int 0 ou 1) */
    | expr AND expr     { $$.val = (double)($1.val && $3.val); $$.type = TYPE_INT; }
    | expr OR expr      { $$.val = (double)($1.val || $3.val); $$.type = TYPE_INT; }
    | NOT expr          { $$.val = (double)(!$2.val); $$.type = TYPE_INT; }
    /* Operadores de igualdade */
    | expr EQ expr      { $$.val = (double)($1.val == $3.val); $$.type = TYPE_INT; }
    | expr NE expr      { $$.val = (double)($1.val != $3.val); $$.type = TYPE_INT; }
    /* Operadores relacionais */
    | expr LT expr      { $$.val = (double)($1.val < $3.val); $$.type = TYPE_INT; }
    | expr GT expr      { $$.val = (double)($1.val > $3.val); $$.type = TYPE_INT; }
    | expr LE expr      { $$.val = (double)($1.val <= $3.val); $$.type = TYPE_INT; }
    | expr GE expr      { $$.val = (double)($1.val >= $3.val); $$.type = TYPE_INT; }
    /* Agrupamento */
    | LPAREN expr RPAREN{ $$ = $2; }
    /* Literais */
    | NUM               { $$.val = (double)$1; $$.type = TYPE_INT; }
    | FLOAT_LIT         { $$.val = $1; $$.type = TYPE_FLOAT; }
    | CHAR_LIT          { $$.val = (double)$1; $$.type = TYPE_CHAR; }
    | TRUE_LIT          { $$.val = (double)$1; $$.type = TYPE_BOOL; }
    | FALSE_LIT         { $$.val = (double)$1; $$.type = TYPE_BOOL; }
    /* Variáveis */
    | ID                {
                            SymEntry *e = sym_lookup($1);
                            if (!e) {
                                fprintf(stderr, "Erro: variável '%s' não declarada\n", $1);
                                free($1);
                                YYABORT;
                            }
                            $$.val = sym_val_as_double(e);
                            $$.type = e->type;
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