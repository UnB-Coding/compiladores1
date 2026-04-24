/******************************************************
 FGA0003 - Compiladores 1
 Curso de Engenharia de Software
 Universidade de Brasília (UnB)

 Arquivo: ast.h
 Descrição: Definição da Árvore Sintática Abstrata (AST)

 A AST separa completamente a fase de análise (parsing)
 da fase de execução. O Bison constrói a árvore durante
 o parsing; após o parsing, eval_ast() percorre a árvore
 e executa o programa.
 ******************************************************/

#ifndef AST_H
#define AST_H

#include "symbol_table/symtab.h"

/* ====================================================
 * Tipos de nó da AST
 * ==================================================== */
typedef enum {
    /* Literais */
    AST_NUM,        /* literal inteiro                    */
    AST_FLOAT,      /* literal ponto flutuante            */
    AST_CHAR,       /* literal caractere                  */
    AST_BOOL,       /* literal booleano (true/false)      */

    /* Identificador */
    AST_ID,         /* referência a variável              */

    /* Operadores */
    AST_BINOP,      /* operação binária (+, -, *, /, etc) */
    AST_UNARYOP,    /* operação unária (-, !)             */

    /* Comandos */
    AST_ASSIGN,     /* atribuição: id = expr              */
    AST_DECL,       /* declaração: tipo id [= expr]       */
    AST_EXPR_STMT,  /* expressão como comando (expr;)     */

    /* Controle de fluxo */
    AST_IF,         /* if (cond) then [else]              */
    AST_WHILE,      /* while (cond) body                  */
    AST_FOR,        /* for (init; cond; step) body        */

    /* Estrutural */
    AST_BLOCK       /* bloco { stmt_list }                */
} NodeType;

/* ====================================================
 * Resultado tipado da avaliação de uma expressão
 * ==================================================== */
typedef struct {
    double  val;    /* valor numérico (double para uniformidade) */
    SymType type;   /* tipo semântico do resultado              */
} EvalResult;

/* ====================================================
 * Nó da AST
 *
 * CRÍTICO: o ponteiro 'next' na base da struct permite
 * encadear comandos em lista (line_list, blocos).
 * ==================================================== */
typedef struct ASTNode {
    NodeType kind;              /* tipo do nó                 */
    struct ASTNode *next;       /* próximo comando na lista   */

    union {
        /* AST_NUM */
        struct { int value; } num;

        /* AST_FLOAT */
        struct { double value; } flt;

        /* AST_CHAR */
        struct { char value; } chr;

        /* AST_BOOL */
        struct { int value; } bln;

        /* AST_ID */
        struct { char *name; } id;

        /* AST_BINOP: left OP right */
        struct {
            int op;                     /* operador ('+', '-', etc.) */
            struct ASTNode *left;
            struct ASTNode *right;
        } binop;

        /* AST_UNARYOP: OP operand */
        struct {
            int op;                     /* operador ('-', '!')       */
            struct ASTNode *operand;
        } unaryop;

        /* AST_ASSIGN: id = expr */
        struct {
            char *name;
            struct ASTNode *expr;
        } assign;

        /* AST_DECL: type_spec id [= expr] */
        struct {
            SymType type;
            char *name;
            struct ASTNode *init;       /* NULL se sem inicialização */
        } decl;

        /* AST_EXPR_STMT: expr; */
        struct {
            struct ASTNode *expr;
        } expr_stmt;

        /* AST_IF: if (cond) then_branch [else else_branch] */
        struct {
            struct ASTNode *cond;
            struct ASTNode *then_branch;
            struct ASTNode *else_branch; /* NULL se sem else */
        } if_stmt;

        /* AST_WHILE: while (cond) body */
        struct {
            struct ASTNode *cond;
            struct ASTNode *body;
        } while_stmt;

        /* AST_FOR: for (init; cond; step) body */
        struct {
            struct ASTNode *init;
            struct ASTNode *cond;
            struct ASTNode *step;
            struct ASTNode *body;
        } for_stmt;

        /* AST_BLOCK: { stmt_list } */
        struct {
            struct ASTNode *stmts;      /* cabeça da lista encadeada */
        } block;
    } data;
} ASTNode;

/* ====================================================
 * Construtores de nós (alocam memória)
 * ==================================================== */

/* Literais */
ASTNode *new_num_node(int value);
ASTNode *new_float_node(double value);
ASTNode *new_char_node(char value);
ASTNode *new_bool_node(int value);

/* Identificador */
ASTNode *new_id_node(const char *name);

/* Operadores */
ASTNode *new_binop_node(int op, ASTNode *left, ASTNode *right);
ASTNode *new_unaryop_node(int op, ASTNode *operand);

/* Comandos */
ASTNode *new_assign_node(const char *name, ASTNode *expr);
ASTNode *new_decl_node(SymType type, const char *name, ASTNode *init);
ASTNode *new_expr_stmt_node(ASTNode *expr);

/* Controle de fluxo */
ASTNode *new_if_node(ASTNode *cond, ASTNode *then_b, ASTNode *else_b);
ASTNode *new_while_node(ASTNode *cond, ASTNode *body);
ASTNode *new_for_node(ASTNode *init, ASTNode *cond, ASTNode *step,
                      ASTNode *body);

/* Estrutural */
ASTNode *new_block_node(ASTNode *stmts);

/* ====================================================
 * Avaliador: percorre a AST e executa o programa
 * ==================================================== */
EvalResult eval_ast(ASTNode *node);

/* ====================================================
 * Liberação de memória: percorre a árvore recursivamente
 * ==================================================== */
void free_ast(ASTNode *node);

#endif /* AST_H */
