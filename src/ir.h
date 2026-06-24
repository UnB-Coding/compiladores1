/******************************************************
 FGA0003 - Compiladores 1
 Curso de Engenharia de Software
 Universidade de Brasília (UnB)

 Arquivo: ir.h
 Descrição: Geração de Código Intermediário (TAC)

 Esta fase é executada APÓS a análise semântica e produz
 a representação que será executada por ir_exec(). Percorre
 a AST e produz uma Representação Intermediária (IR) na forma de
 Código de Três Endereços (Three-Address Code, TAC):

     t0 = a + b
     x  = t0
     ifFalse x goto L1
     ...
     L1:

 A IR é uma LISTA LINEAR de instruções (quádruplas), em
 contraste com a estrutura em árvore da AST. Essa forma
 plana é a base sobre a qual futuras otimizações operam
 (constant folding, propagação de cópias, eliminação de
 código morto, alocação de registradores, etc.).

 Características de projeto que favorecem otimizações futuras:
   - Constantes ficam embutidas nos operandos (não são
     materializadas em temporários), permitindo dobramento
     de constantes trivial.
   - Cada operando carrega seu SymType, habilitando análises
     sensíveis a tipo.
   - Temporários são numerados (t0, t1, ...) e cada um recebe
     um único valor (estilo SSA-friendly), facilitando a
     análise de fluxo de dados.
 ******************************************************/

#ifndef IR_H
#define IR_H

#include "ast.h"
#include "symbol_table/symtab.h"

/* ====================================================
 * Tipo de um operando de instrução
 * ==================================================== */
typedef enum {
    OPND_NONE,        /* operando ausente (ex.: arg2 de uma cópia)  */
    OPND_TEMP,        /* temporário: t0, t1, ...                    */
    OPND_VAR,         /* variável nomeada do programa               */
    OPND_CONST_INT,   /* constante inteira                          */
    OPND_CONST_FLOAT, /* constante ponto flutuante                  */
    OPND_CONST_CHAR,  /* constante caractere                        */
    OPND_CONST_BOOL,  /* constante booleana                         */
    OPND_LABEL        /* rótulo (alvo de desvio)                    */
} IROperandKind;

/* ====================================================
 * Operando de uma instrução de três endereços
 * ==================================================== */
typedef struct {
    IROperandKind kind;
    SymType       type;     /* tipo do valor (para otimizações)     */
    union {
        int    temp;        /* OPND_TEMP  → índice do temporário    */
        char  *name;        /* OPND_VAR   → nome (strdup)           */
        int    iconst;      /* OPND_CONST_INT/CHAR/BOOL             */
        double fconst;      /* OPND_CONST_FLOAT                    */
        int    label;       /* OPND_LABEL → índice do rótulo        */
    } u;
} IROperand;

/* ====================================================
 * Código de operação de uma instrução
 * ==================================================== */
typedef enum {
    IR_LABEL,    /* Lk:                  — define um rótulo         */
    IR_GOTO,     /* goto Lk              — desvio incondicional     */
    IR_IFFALSE,  /* ifFalse x goto Lk    — desvia se x == 0         */
    IR_COPY,     /* x = y                — cópia/atribuição         */
    IR_BINOP,    /* x = y <op> z         — operação binária         */
    IR_UNARYOP,  /* x = <op> y           — operação unária          */
    IR_DECL,     /* decl x : T           — declaração; dest=var, op=has_init */
    IR_PRINT     /* print x              — imprime resultado de expr_stmt    */
} IROpcode;

/* ====================================================
 * Instrução (quádrupla) — nó de uma lista encadeada
 *
 * O campo 'op' guarda o operador (mesmo código char usado
 * na AST: '+', '-', 'E', '<', ...) para IR_BINOP/IR_UNARYOP.
 * ==================================================== */
typedef struct IRInstr {
    IROpcode  opcode;
    int       op;        /* operador, quando aplicável               */
    IROperand dest;      /* destino (temp/var) ou rótulo definido    */
    IROperand arg1;      /* primeiro operando / condição / alvo goto */
    IROperand arg2;      /* segundo operando (BINOP)                 */
    struct IRInstr *next;
} IRInstr;

/* ====================================================
 * Programa em IR: lista de instruções + contadores
 * ==================================================== */
typedef struct {
    IRInstr *head;        /* primeira instrução                     */
    IRInstr *tail;        /* última instrução (append O(1))         */
    int      temp_count;  /* próximo temporário a alocar            */
    int      label_count; /* próximo rótulo a alocar                */
} IRProgram;

/* ====================================================
 * gen_ir: percorre a AST e produz a IR completa.
 *
 * Parâmetro:
 *   root — raiz da AST (lista encadeada de statements).
 *
 * Retorno:
 *   Ponteiro para um IRProgram alocado dinamicamente.
 *   O chamador deve liberá-lo com ir_free().
 * ==================================================== */
IRProgram *gen_ir(ASTNode *root);

/* ====================================================
 * ir_print: imprime a IR em formato TAC legível.
 * ==================================================== */
void ir_print(const IRProgram *prog);

/* ====================================================
 * ir_free: libera toda a memória do programa IR.
 * ==================================================== */
void ir_free(IRProgram *prog);

/* ====================================================
 * ir_optimize: otimiza a IR in-place.
 * Executa constant folding, propagação de constantes e
 * eliminação de código morto em loop de ponto fixo.
 * ==================================================== */
void ir_optimize(IRProgram *prog);

/* ====================================================
 * ir_exec: executa o programa IR diretamente.
 * Lineariza a lista, constrói mapa de rótulos e
 * interpreta cada instrução com um ponteiro de instrução.
 * ==================================================== */
void ir_exec(IRProgram *prog);

#endif /* IR_H */
