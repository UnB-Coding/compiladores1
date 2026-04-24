/******************************************************
FGA0003 - Compiladores 1
Curso de Engenharia de Software
Universidade de Brasília (UnB)

Arquivo: parser.y
Descrição: Gramática LALR(1) que constrói uma AST

As ações semânticas APENAS constroem nós da AST.
Nenhuma execução (cálculos, Tabela de Símbolos, I/O)
ocorre durante o parsing. A execução acontece depois,
em eval_ast().
******************************************************/

%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "symbol_table/symtab.h"

/* Declarações para evitar avisos de função implícita */
int yylex(void);
void yyerror(const char *s);

/* Raiz da AST — preenchida ao final do parsing */
ASTNode *ast_root = NULL;

/* Função auxiliar: anexa nó ao final de uma lista encadeada.
 * Retorna a cabeça da lista resultante. */
static ASTNode *append_node(ASTNode *list, ASTNode *node) {
    if (!list) return node;
    ASTNode *p = list;
    while (p->next) p = p->next;
    p->next = node;
    return list;
}
%}

/* ====================================================
 * Valor semântico dos tokens e não-terminais
 * ==================================================== */
%union {
    int    intValue;           /* NUM, literais booleanos, type_spec */
    double floatValue;         /* FLOAT_LIT */
    char   charValue;          /* CHAR_LIT */
    char  *strValue;           /* identificadores */
    struct ASTNode *node;      /* nós da AST */
}

/* ====================================================
 * Tokens que carregam valor semântico
 * ==================================================== */
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

/* Tokens para controle de fluxo e blocos */
%token KW_IF KW_ELSE KW_WHILE KW_FOR LBRACE RBRACE

/* Operadores relacionais, de igualdade e lógicos */
%token AND OR NOT EQ NE LT GT LE GE

/* ====================================================
 * Precedência (menor → maior):
 *   OR < AND < EQ NE < LT GT LE GE < PLUS MINUS
 *   < TIMES DIVIDE < NOT UMINUS
 * ==================================================== */
%left OR
%left AND
%left EQ NE
%left LT GT LE GE
%left PLUS MINUS
%left TIMES DIVIDE
%right NOT UMINUS

/* ====================================================
 * Tipos dos não-terminais (todos produzem nós AST)
 * ==================================================== */
%type <node> expr stmt line line_list stmt_list
%type <intValue> type_spec

/* Conflito shift/reduce do dangling-else: resolvido por shift (semântica C) */
%expect 1

%%

/* ====================================================
 * Gramática
 * ==================================================== */

program:
      line_list                  { ast_root = $1; }
    ;

line_list:
      %empty                     { $$ = NULL; }
    | line_list line             { $$ = append_node($1, $2); }
    ;

line:
      stmt                       { $$ = $1; }
    ;

stmt:
      expr SEMICOLON
        { $$ = new_expr_stmt_node($1); }

    | ID ASSIGN expr SEMICOLON
        { $$ = new_assign_node($1, $3); free($1); }

    | type_spec ID SEMICOLON
        { $$ = new_decl_node((SymType)$1, $2, NULL); free($2); }

    | type_spec ID ASSIGN expr SEMICOLON
        { $$ = new_decl_node((SymType)$1, $2, $4); free($2); }

    | KW_IF LPAREN expr RPAREN stmt
        { $$ = new_if_node($3, $5, NULL); }

    | KW_IF LPAREN expr RPAREN stmt KW_ELSE stmt
        { $$ = new_if_node($3, $5, $7); }

    | KW_WHILE LPAREN expr RPAREN stmt
        { $$ = new_while_node($3, $5); }

    | KW_FOR LPAREN for_init SEMICOLON for_cond SEMICOLON for_step RPAREN stmt
        { $$ = new_for_node($3, $5, $7, $9); }

    | LBRACE stmt_list RBRACE
        { $$ = new_block_node($2); }
    ;

/* ====================================================
 * Componentes do FOR
 *
 * Cada componente é opcional (pode ser vazio).
 * O for_init pode ser uma declaração com inicialização,
 * uma atribuição, ou vazio.
 * ==================================================== */
%type <node> for_init for_cond for_step;

for_init:
      %empty                          { $$ = NULL; }
    | type_spec ID ASSIGN expr        { $$ = new_decl_node((SymType)$1, $2, $4); free($2); }
    | ID ASSIGN expr                  { $$ = new_assign_node($1, $3); free($1); }
    ;

for_cond:
      %empty                          { $$ = NULL; }
    | expr                            { $$ = $1; }
    ;

for_step:
      %empty                          { $$ = NULL; }
    | ID ASSIGN expr                  { $$ = new_assign_node($1, $3); free($1); }
    ;

/* Lista de comandos dentro de um bloco */
stmt_list:
      %empty                     { $$ = NULL; }
    | stmt_list stmt             { $$ = append_node($1, $2); }
    ;

/* ====================================================
 * Tipo para declarações
 * ==================================================== */
type_spec:
      T_INT     { $$ = TYPE_INT; }
    | T_FLOAT   { $$ = TYPE_FLOAT; }
    | T_CHAR    { $$ = TYPE_CHAR; }
    | T_BOOL    { $$ = TYPE_BOOL; }
    ;

/* ====================================================
 * Expressões — constroem nós AST, sem calcular nada
 * ==================================================== */
expr:
    /* Operadores aritméticos */
      expr PLUS expr           { $$ = new_binop_node('+', $1, $3); }
    | expr MINUS expr          { $$ = new_binop_node('-', $1, $3); }
    | expr TIMES expr          { $$ = new_binop_node('*', $1, $3); }
    | expr DIVIDE expr         { $$ = new_binop_node('/', $1, $3); }

    /* Menos unário */
    | MINUS expr %prec UMINUS  { $$ = new_unaryop_node('-', $2); }

    /* Operadores lógicos */
    | expr AND expr            { $$ = new_binop_node('&', $1, $3); }
    | expr OR expr             { $$ = new_binop_node('|', $1, $3); }
    | NOT expr                 { $$ = new_unaryop_node('!', $2); }

    /* Operadores de igualdade */
    | expr EQ expr             { $$ = new_binop_node('E', $1, $3); }
    | expr NE expr             { $$ = new_binop_node('N', $1, $3); }

    /* Operadores relacionais */
    | expr LT expr             { $$ = new_binop_node('<', $1, $3); }
    | expr GT expr             { $$ = new_binop_node('>', $1, $3); }
    | expr LE expr             { $$ = new_binop_node('l', $1, $3); }
    | expr GE expr             { $$ = new_binop_node('g', $1, $3); }

    /* Agrupamento */
    | LPAREN expr RPAREN       { $$ = $2; }

    /* Literais */
    | NUM                      { $$ = new_num_node($1); }
    | FLOAT_LIT                { $$ = new_float_node($1); }
    | CHAR_LIT                 { $$ = new_char_node($1); }
    | TRUE_LIT                 { $$ = new_bool_node($1); }
    | FALSE_LIT                { $$ = new_bool_node($1); }

    /* Variáveis */
    | ID                       { $$ = new_id_node($1); free($1); }
    ;

%%

/* ====================================================
 * main: parsing → execução → limpeza
 * ==================================================== */
int main(void) {
    if (yyparse() == 0 && ast_root) {
        /* Fase 1 concluída: AST construída com sucesso.
         * Fase 2: percorrer a AST e executar o programa. */
        EvalResult last = { 0.0, TYPE_INT };
        ASTNode *cur = ast_root;
        while (cur) {
            last = eval_ast(cur);
            cur = cur->next;
        }
        (void)last;  /* suprime aviso de variável não usada */

        /* Imprime a tabela de símbolos final */
        sym_print();

        /* Libera toda a memória */
        free_ast(ast_root);
        sym_free();
    }
    return 0;
}

void yyerror(const char *s) {
    fprintf(stderr, "Erro sintático: %s\n", s);
}