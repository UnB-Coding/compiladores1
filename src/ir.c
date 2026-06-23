/******************************************************
 FGA0003 - Compiladores 1
 Curso de Engenharia de Software
 Universidade de Brasília (UnB)

 Arquivo: ir.c
 Descrição: Implementação da Geração de Código Intermediário

 Percorre a AST (após análise semântica) e emite Código de
 Três Endereços (TAC) numa lista linear de instruções.

 Estratégia:
   - gen_expr() avalia uma expressão, emitindo as instruções
     necessárias, e RETORNA o operando que contém o resultado
     (um temporário, uma variável ou uma constante embutida).
   - gen_stmt() emite as instruções de um comando, criando
     rótulos e desvios para o controle de fluxo.

 Não há execução: nenhum valor é calculado nem impresso.
 A IR resultante é independente de máquina e serve de base
 para as fases de otimização e geração de código final.
 ******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"
#include "symbol_table/symtab.h"

/* ====================================================
 * Ambiente de tipos (type environment)
 *
 * A geração de IR ocorre ANTES da execução, quando a tabela
 * de símbolos real (symtab.c) ainda está vazia. Para que os
 * operandos-variável carreguem o tipo declarado correto —
 * informação essencial para otimizações sensíveis a tipo —
 * mantemos aqui um mapa local (nome → tipo) preenchido a
 * partir das declarações (AST_DECL) à medida que as visitamos.
 *
 * É uma lista simples (programas pequenos, sem escopos
 * aninhados separados); declarações redeclaradas já foram
 * barradas pela análise semântica.
 * ==================================================== */
typedef struct TypeEntry {
    char    *name;
    SymType  type;
    struct TypeEntry *next;
} TypeEntry;

static TypeEntry *type_env = NULL;

static void type_env_set(const char *name, SymType type) {
    for (TypeEntry *e = type_env; e; e = e->next) {
        if (strcmp(e->name, name) == 0) { e->type = type; return; }
    }
    TypeEntry *e = (TypeEntry *)malloc(sizeof(TypeEntry));
    if (!e) {
        fprintf(stderr, "Erro: falha ao alocar entrada de tipo IR\n");
        exit(EXIT_FAILURE);
    }
    e->name = strdup(name);
    e->type = type;
    e->next = type_env;
    type_env = e;
}

static SymType type_env_get(const char *name) {
    for (TypeEntry *e = type_env; e; e = e->next) {
        if (strcmp(e->name, name) == 0) return e->type;
    }
    return TYPE_NONE;
}

static void type_env_free(void) {
    TypeEntry *e = type_env;
    while (e) {
        TypeEntry *next = e->next;
        free(e->name);
        free(e);
        e = next;
    }
    type_env = NULL;
}

/* ====================================================
 * Construtores de operandos
 * ==================================================== */

static IROperand opnd_none(void) {
    IROperand o;
    o.kind = OPND_NONE;
    o.type = TYPE_NONE;
    return o;
}

static IROperand opnd_temp(int id, SymType type) {
    IROperand o;
    o.kind = OPND_TEMP;
    o.type = type;
    o.u.temp = id;
    return o;
}

static IROperand opnd_var(const char *name, SymType type) {
    IROperand o;
    o.kind = OPND_VAR;
    o.type = type;
    o.u.name = strdup(name);
    return o;
}

static IROperand opnd_label(int id) {
    IROperand o;
    o.kind = OPND_LABEL;
    o.type = TYPE_NONE;
    o.u.label = id;
    return o;
}

static IROperand opnd_const_int(int v, SymType type) {
    IROperand o;
    o.kind = (type == TYPE_CHAR) ? OPND_CONST_CHAR
           : (type == TYPE_BOOL) ? OPND_CONST_BOOL
           : OPND_CONST_INT;
    o.type = type;
    o.u.iconst = v;
    return o;
}

static IROperand opnd_const_float(double v) {
    IROperand o;
    o.kind = OPND_CONST_FLOAT;
    o.type = TYPE_FLOAT;
    o.u.fconst = v;
    return o;
}

/* ====================================================
 * Alocação de temporários e rótulos
 * ==================================================== */

static IROperand new_temp(IRProgram *p, SymType type) {
    return opnd_temp(p->temp_count++, type);
}

static int new_label(IRProgram *p) {
    return p->label_count++;
}

/* ====================================================
 * Emissão de instruções (append O(1) na cauda)
 * ==================================================== */

static IRInstr *emit(IRProgram *p, IROpcode opcode, int op,
                     IROperand dest, IROperand arg1, IROperand arg2) {
    IRInstr *ins = (IRInstr *)calloc(1, sizeof(IRInstr));
    if (!ins) {
        fprintf(stderr, "Erro: falha ao alocar instrução IR\n");
        exit(EXIT_FAILURE);
    }
    ins->opcode = opcode;
    ins->op     = op;
    ins->dest   = dest;
    ins->arg1   = arg1;
    ins->arg2   = arg2;
    ins->next   = NULL;

    if (!p->head) {
        p->head = p->tail = ins;
    } else {
        p->tail->next = ins;
        p->tail = ins;
    }
    return ins;
}

/* Emite a definição de um rótulo: "Lk:" */
static void emit_label(IRProgram *p, int label) {
    emit(p, IR_LABEL, 0, opnd_label(label), opnd_none(), opnd_none());
}

/* Emite desvio incondicional: "goto Lk" */
static void emit_goto(IRProgram *p, int label) {
    emit(p, IR_GOTO, 0, opnd_none(), opnd_label(label), opnd_none());
}

/* Emite desvio condicional: "ifFalse cond goto Lk" */
static void emit_iffalse(IRProgram *p, IROperand cond, int label) {
    emit(p, IR_IFFALSE, 0, opnd_none(), cond, opnd_label(label));
}

/* ====================================================
 * Inferência de tipo do resultado de operações
 *
 * Espelha promote_type() do avaliador: float domina;
 * char/bool promovem a int. Operadores relacionais,
 * de igualdade e lógicos produzem int (0/1).
 * ==================================================== */
static SymType promote_type(SymType a, SymType b) {
    if (a == TYPE_FLOAT || b == TYPE_FLOAT) return TYPE_FLOAT;
    return TYPE_INT;
}

static SymType binop_result_type(int op, SymType l, SymType r) {
    switch (op) {
    case '+': case '-': case '*': case '/':
        return promote_type(l, r);
    default:
        /* &&, ||, ==, !=, <, >, <=, >= → resultado booleano/int */
        return TYPE_INT;
    }
}

/* ====================================================
 * Declarações antecipadas (recursão mútua)
 * ==================================================== */
static IROperand gen_expr(IRProgram *p, ASTNode *node);
static void gen_stmt(IRProgram *p, ASTNode *node);
static void gen_list(IRProgram *p, ASTNode *node);

/* ====================================================
 * gen_expr: gera código para uma expressão.
 * Retorna o operando que contém o resultado.
 * ==================================================== */
static IROperand gen_expr(IRProgram *p, ASTNode *node) {
    if (!node) return opnd_none();

    switch (node->kind) {

    /* ---- Literais: viram constantes embutidas (sem instrução) ---- */
    case AST_NUM:
        return opnd_const_int(node->data.num.value, TYPE_INT);
    case AST_FLOAT:
        return opnd_const_float(node->data.flt.value);
    case AST_CHAR:
        return opnd_const_int((int)node->data.chr.value, TYPE_CHAR);
    case AST_BOOL:
        return opnd_const_int(node->data.bln.value ? 1 : 0, TYPE_BOOL);

    /* ---- Identificador: vira um operando-variável ---- */
    case AST_ID:
        return opnd_var(node->data.id.name, type_env_get(node->data.id.name));

    /* ---- Operação binária: t = arg1 op arg2 ---- */
    case AST_BINOP: {
        IROperand l = gen_expr(p, node->data.binop.left);
        IROperand r = gen_expr(p, node->data.binop.right);
        SymType rt = binop_result_type(node->data.binop.op, l.type, r.type);
        IROperand dest = new_temp(p, rt);
        emit(p, IR_BINOP, node->data.binop.op, dest, l, r);
        return dest;
    }

    /* ---- Operação unária: t = op arg ---- */
    case AST_UNARYOP: {
        IROperand a = gen_expr(p, node->data.unaryop.operand);
        /* '!' produz int; '-' preserva o tipo do operando */
        SymType rt = (node->data.unaryop.op == '!') ? TYPE_INT : a.type;
        IROperand dest = new_temp(p, rt);
        emit(p, IR_UNARYOP, node->data.unaryop.op, dest, a, opnd_none());
        return dest;
    }

    default:
        /* Comandos não são expressões; não deveria ocorrer aqui. */
        return opnd_none();
    }
}

/* ====================================================
 * gen_stmt: gera código para um único comando.
 * ==================================================== */
static void gen_stmt(IRProgram *p, ASTNode *node) {
    if (!node) return;

    switch (node->kind) {

    /* ---- Declaração: registra o tipo e, se houver init, copia ---- */
    case AST_DECL:
        type_env_set(node->data.decl.name, node->data.decl.type);
        if (node->data.decl.init) {
            IROperand rhs = gen_expr(p, node->data.decl.init);
            IROperand dst = opnd_var(node->data.decl.name, node->data.decl.type);
            emit(p, IR_COPY, 0, dst, rhs, opnd_none());
        }
        /* Declaração sem inicialização não gera código. */
        break;

    /* ---- Atribuição: var = expr ---- */
    case AST_ASSIGN: {
        IROperand rhs = gen_expr(p, node->data.assign.expr);
        IROperand dst = opnd_var(node->data.assign.name,
                                 type_env_get(node->data.assign.name));
        emit(p, IR_COPY, 0, dst, rhs, opnd_none());
        break;
    }

    /* ---- Expressão como comando: avalia e descarta o resultado ---- */
    case AST_EXPR_STMT:
        (void)gen_expr(p, node->data.expr_stmt.expr);
        break;

    /* ---- If / If-Else ----
     *   t = cond
     *   ifFalse t goto Lfalse
     *   <then>
     *   goto Lend           (apenas se houver else)
     * Lfalse:
     *   <else>
     * Lend:
     */
    case AST_IF: {
        IROperand cond = gen_expr(p, node->data.if_stmt.cond);
        int l_false = new_label(p);
        emit_iffalse(p, cond, l_false);
        gen_list(p, node->data.if_stmt.then_branch);

        if (node->data.if_stmt.else_branch) {
            int l_end = new_label(p);
            emit_goto(p, l_end);
            emit_label(p, l_false);
            gen_list(p, node->data.if_stmt.else_branch);
            emit_label(p, l_end);
        } else {
            emit_label(p, l_false);
        }
        break;
    }

    /* ---- While ----
     * Lstart:
     *   t = cond
     *   ifFalse t goto Lend
     *   <body>
     *   goto Lstart
     * Lend:
     */
    case AST_WHILE: {
        int l_start = new_label(p);
        int l_end   = new_label(p);
        emit_label(p, l_start);
        IROperand cond = gen_expr(p, node->data.while_stmt.cond);
        emit_iffalse(p, cond, l_end);
        gen_list(p, node->data.while_stmt.body);
        emit_goto(p, l_start);
        emit_label(p, l_end);
        break;
    }

    /* ---- For ----
     *   <init>
     * Lstart:
     *   t = cond                (se houver condição)
     *   ifFalse t goto Lend
     *   <body>
     *   <step>
     *   goto Lstart
     * Lend:
     */
    case AST_FOR: {
        if (node->data.for_stmt.init)
            gen_stmt(p, node->data.for_stmt.init);

        int l_start = new_label(p);
        int l_end   = new_label(p);
        emit_label(p, l_start);

        if (node->data.for_stmt.cond) {
            IROperand cond = gen_expr(p, node->data.for_stmt.cond);
            emit_iffalse(p, cond, l_end);
        }
        gen_list(p, node->data.for_stmt.body);
        if (node->data.for_stmt.step)
            gen_stmt(p, node->data.for_stmt.step);
        emit_goto(p, l_start);
        emit_label(p, l_end);
        break;
    }

    /* ---- Bloco: gera a lista de comandos internos ---- */
    case AST_BLOCK:
        gen_list(p, node->data.block.stmts);
        break;

    default:
        /* Literais/operadores soltos não deveriam aparecer como stmt. */
        break;
    }
}

/* ====================================================
 * gen_list: percorre a lista encadeada (->next) de comandos.
 * ==================================================== */
static void gen_list(IRProgram *p, ASTNode *node) {
    while (node) {
        gen_stmt(p, node);
        node = node->next;
    }
}

/* ====================================================
 * gen_ir: ponto de entrada da fase de geração de IR.
 * ==================================================== */
IRProgram *gen_ir(ASTNode *root) {
    IRProgram *p = (IRProgram *)calloc(1, sizeof(IRProgram));
    if (!p) {
        fprintf(stderr, "Erro: falha ao alocar programa IR\n");
        exit(EXIT_FAILURE);
    }
    gen_list(p, root);
    type_env_free();
    return p;
}

/* ====================================================
 * Impressão da IR em formato TAC
 * ==================================================== */

/* Reaproveita a notação de operadores legível */
static const char *ir_op_to_str(int op) {
    switch (op) {
        case '+': return "+";
        case '-': return "-";
        case '*': return "*";
        case '/': return "/";
        case '&': return "&&";
        case '|': return "||";
        case '!': return "!";
        case 'E': return "==";
        case 'N': return "!=";
        case '<': return "<";
        case '>': return ">";
        case 'l': return "<=";
        case 'g': return ">=";
        default:  return "?";
    }
}

/* Imprime um operando isolado */
static void print_operand(const IROperand *o) {
    switch (o->kind) {
    case OPND_NONE:        printf("_");                       break;
    case OPND_TEMP:        printf("t%d", o->u.temp);          break;
    case OPND_VAR:         printf("%s", o->u.name);           break;
    case OPND_CONST_INT:   printf("%d", o->u.iconst);         break;
    case OPND_CONST_FLOAT: printf("%g", o->u.fconst);         break;
    case OPND_CONST_BOOL:  printf("%s", o->u.iconst ? "true" : "false"); break;
    case OPND_CONST_CHAR: {
        char c = (char)o->u.iconst;
        if (c >= 32 && c < 127) printf("'%c'", c);
        else                    printf("%d", o->u.iconst);
        break;
    }
    case OPND_LABEL:       printf("L%d", o->u.label);         break;
    }
}

void ir_print(const IRProgram *prog) {
    if (!prog || !prog->head) {
        printf("(IR vazia)\n");
        return;
    }

    for (IRInstr *ins = prog->head; ins; ins = ins->next) {
        switch (ins->opcode) {

        case IR_LABEL:
            print_operand(&ins->dest);
            printf(":\n");
            break;

        case IR_GOTO:
            printf("    goto ");
            print_operand(&ins->arg1);
            printf("\n");
            break;

        case IR_IFFALSE:
            printf("    ifFalse ");
            print_operand(&ins->arg1);
            printf(" goto ");
            print_operand(&ins->arg2);
            printf("\n");
            break;

        case IR_COPY:
            printf("    ");
            print_operand(&ins->dest);
            printf(" = ");
            print_operand(&ins->arg1);
            printf("\n");
            break;

        case IR_BINOP:
            printf("    ");
            print_operand(&ins->dest);
            printf(" = ");
            print_operand(&ins->arg1);
            printf(" %s ", ir_op_to_str(ins->op));
            print_operand(&ins->arg2);
            printf("\n");
            break;

        case IR_UNARYOP:
            printf("    ");
            print_operand(&ins->dest);
            printf(" = %s", ir_op_to_str(ins->op));
            print_operand(&ins->arg1);
            printf("\n");
            break;
        }
    }
}

/* ====================================================
 * Liberação de memória
 * ==================================================== */

/* Libera o nome duplicado de um operando-variável */
static void free_operand(IROperand *o) {
    if (o->kind == OPND_VAR && o->u.name) {
        free(o->u.name);
        o->u.name = NULL;
    }
}

void ir_free(IRProgram *prog) {
    if (!prog) return;
    IRInstr *ins = prog->head;
    while (ins) {
        IRInstr *next = ins->next;
        free_operand(&ins->dest);
        free_operand(&ins->arg1);
        free_operand(&ins->arg2);
        free(ins);
        ins = next;
    }
    free(prog);
}
