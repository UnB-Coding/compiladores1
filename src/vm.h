#ifndef VM_H
#define VM_H

#include "ast.h"
#include "symbol_table/symtab.h"

/* ====================================================
 * Conjunto de instruções da VM stack-based
 * ==================================================== */
typedef enum {
    /* Constantes: empilham valor */
    BC_PUSH_INT,       /* iarg = valor inteiro              */
    BC_PUSH_FLOAT,     /* farg = valor double               */
    BC_PUSH_BOOL,      /* iarg = 0 ou 1                     */
    BC_PUSH_CHAR,      /* iarg = char como int              */

    /* Variáveis nomeadas */
    BC_DECL_VAR,       /* sarg=nome, type=tipo, iarg=has_init */
    BC_LOAD_VAR,       /* sarg=nome — empilha valor         */
    BC_STORE_VAR,      /* sarg=nome — desempilha e armazena */

    /* Operações */
    BC_BINOP,          /* iarg=op — desempilha 2, empilha resultado */
    BC_UNARYOP,        /* iarg=op — desempilha 1, empilha resultado */

    /* Controle de fluxo */
    BC_JMP,            /* iarg=offset — salto incondicional */
    BC_JMPF,           /* iarg=offset — salto se topo == 0 */

    /* Saída */
    BC_PRINT_EXPR,     /* desempilha e imprime "Resultado: x" */

    /* Fim */
    BC_HALT
} BCOpcode;

/* ====================================================
 * Instrução de bytecode
 * ==================================================== */
typedef struct {
    BCOpcode opcode;
    int      iarg;   /* inteiro genérico: constante, offset, op char */
    double   farg;   /* constante float */
    char    *sarg;   /* nome de variável (strdup ou NULL) */
    SymType  type;   /* tipo da variável em DECL_VAR */
} BCInstr;

/* ====================================================
 * Programa de bytecode (array dinâmico de instruções)
 * ==================================================== */
typedef struct {
    BCInstr *code;   /* array alocado dinamicamente */
    int      count;  /* instruções emitidas */
    int      cap;    /* capacidade alocada */
} BCProgram;

/* ====================================================
 * API pública
 * ==================================================== */
BCProgram *bc_compile(ASTNode *root);
void       bc_print(const BCProgram *prog);
void       bc_free(BCProgram *prog);
void       vm_run(const BCProgram *prog);

#endif /* VM_H */
