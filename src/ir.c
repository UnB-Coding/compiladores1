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
#include <limits.h>
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

    /* ---- Declaração: emite IR_DECL e, se houver init, IR_COPY ---- */
    case AST_DECL: {
        type_env_set(node->data.decl.name, node->data.decl.type);
        int has_init = node->data.decl.init ? 1 : 0;
        emit(p, IR_DECL, has_init,
             opnd_var(node->data.decl.name, node->data.decl.type),
             opnd_none(), opnd_none());
        if (has_init) {
            IROperand rhs = gen_expr(p, node->data.decl.init);
            IROperand dst = opnd_var(node->data.decl.name, node->data.decl.type);
            emit(p, IR_COPY, 0, dst, rhs, opnd_none());
        }
        break;
    }

    /* ---- Atribuição: var = expr ---- */
    case AST_ASSIGN: {
        IROperand rhs = gen_expr(p, node->data.assign.expr);
        IROperand dst = opnd_var(node->data.assign.name,
                                 type_env_get(node->data.assign.name));
        emit(p, IR_COPY, 0, dst, rhs, opnd_none());
        break;
    }

    /* ---- Expressão como comando: avalia e emite IR_PRINT ---- */
    case AST_EXPR_STMT: {
        IROperand res = gen_expr(p, node->data.expr_stmt.expr);
        emit(p, IR_PRINT, 0, opnd_none(), res, opnd_none());
        break;
    }

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

        case IR_DECL:
            printf("    decl %s : %s%s\n",
                   ins->dest.u.name, sym_type_name(ins->dest.type),
                   ins->op ? " (init)" : "");
            break;

        case IR_PRINT:
            printf("    print ");
            print_operand(&ins->arg1);
            printf("\n");
            break;
        }
    }
}

/* ====================================================
 * Otimização do IR
 * ==================================================== */

static void free_operand(IROperand *o);  /* definida na seção de liberação */

static int is_const_opnd(IROperand o) {
    return o.kind == OPND_CONST_INT  || o.kind == OPND_CONST_FLOAT ||
           o.kind == OPND_CONST_CHAR || o.kind == OPND_CONST_BOOL;
}

static double const_opnd_val(IROperand o) {
    return o.kind == OPND_CONST_FLOAT ? o.u.fconst : (double)o.u.iconst;
}

static IROperand make_const_opnd(SymType type, double val) {
    if (type == TYPE_FLOAT) return opnd_const_float(val);
    return opnd_const_int((int)val, type);
}

static double ir_compute_binop(int op, double lv, double rv,
                               SymType lt, SymType rt) {
    int use_float = (lt == TYPE_FLOAT || rt == TYPE_FLOAT);
    switch (op) {
    case '+': return use_float ? lv + rv : (double)((int)lv + (int)rv);
    case '-': return use_float ? lv - rv : (double)((int)lv - (int)rv);
    case '*': return use_float ? lv * rv : (double)((int)lv * (int)rv);
    case '/': return use_float ? lv / rv : (double)((int)lv / (int)rv);
    case '&': return (double)(lv != 0.0 && rv != 0.0);
    case '|': return (double)(lv != 0.0 || rv != 0.0);
    case 'E': return (double)(lv == rv);
    case 'N': return (double)(lv != rv);
    case '<': return (double)(lv <  rv);
    case '>': return (double)(lv >  rv);
    case 'l': return (double)(lv <= rv);
    case 'g': return (double)(lv >= rv);
    default:  return 0.0;
    }
}

/* INT_MIN / -1 estoura o intervalo de int (UB em C: SIGFPE em x86).
 * Detecta esse caso para a divisão inteira, tratando-o como erro fatal
 * de execução — análogo à divisão por zero. */
static int is_int_div_overflow(int op, double lv, double rv,
                               SymType lt, SymType rt) {
    if (op != '/') return 0;
    if (lt == TYPE_FLOAT || rt == TYPE_FLOAT) return 0;
    return (int)lv == INT_MIN && (int)rv == -1;
}

/* Pass 1: Constant Folding — BINOP/UNARYOP com operandos constantes → COPY */
static int ir_pass_const_fold(IRProgram *prog) {
    int changed = 0;
    for (IRInstr *ins = prog->head; ins; ins = ins->next) {
        if (ins->opcode == IR_BINOP &&
            is_const_opnd(ins->arg1) && is_const_opnd(ins->arg2)) {
            if (ins->op == '/' && const_opnd_val(ins->arg2) == 0.0) continue;
            /* Não dobra INT_MIN / -1: deixa o erro surgir na execução. */
            if (is_int_div_overflow(ins->op,
                                    const_opnd_val(ins->arg1),
                                    const_opnd_val(ins->arg2),
                                    ins->arg1.type, ins->arg2.type)) continue;
            SymType lt    = ins->arg1.type;
            SymType rt    = ins->arg2.type;
            SymType rtype = binop_result_type(ins->op, lt, rt);
            double  res   = ir_compute_binop(ins->op,
                                             const_opnd_val(ins->arg1),
                                             const_opnd_val(ins->arg2),
                                             lt, rt);
            /* arg1/arg2 são OPND_CONST_* — sem heap alocado; sobrescreve direto */
            ins->opcode = IR_COPY;
            ins->op     = 0;
            ins->arg1   = make_const_opnd(rtype, res);
            ins->arg2   = opnd_none();
            changed = 1;
        } else if (ins->opcode == IR_UNARYOP && is_const_opnd(ins->arg1)) {
            double av = const_opnd_val(ins->arg1);
            double res;
            SymType rtype;
            switch (ins->op) {
            case '-': res = -av;               rtype = ins->arg1.type; break;
            case '!': res = (double)(av==0.0); rtype = TYPE_INT;       break;
            default:  continue;
            }
            ins->opcode = IR_COPY;
            ins->op     = 0;
            ins->arg1   = make_const_opnd(rtype, res);
            changed = 1;
        }
    }
    return changed;
}

/* Pass 2: Propagação de Constantes — t = const → substitui usos de t */
static int ir_pass_const_propagate(IRProgram *prog) {
    if (prog->temp_count == 0) return 0;
    IROperand *known = (IROperand *)calloc(prog->temp_count, sizeof(IROperand));
    if (!known) return 0;

    for (IRInstr *ins = prog->head; ins; ins = ins->next) {
        if (ins->opcode == IR_COPY &&
            ins->dest.kind == OPND_TEMP &&
            is_const_opnd(ins->arg1)) {
            known[ins->dest.u.temp] = ins->arg1;
        }
    }

    int changed = 0;
    for (IRInstr *ins = prog->head; ins; ins = ins->next) {
        if (ins->arg1.kind == OPND_TEMP &&
            ins->arg1.u.temp < prog->temp_count &&
            known[ins->arg1.u.temp].kind != OPND_NONE) {
            ins->arg1 = known[ins->arg1.u.temp];
            changed = 1;
        }
        if (ins->arg2.kind == OPND_TEMP &&
            ins->arg2.u.temp < prog->temp_count &&
            known[ins->arg2.u.temp].kind != OPND_NONE) {
            ins->arg2 = known[ins->arg2.u.temp];
            changed = 1;
        }
    }

    free(known);
    return changed;
}

/* Pass 3: Dead Code Elimination — remove código após IR_GOTO e IFFALSE(const) */
static int ir_pass_dce(IRProgram *prog) {
    int changed = 0;
    int in_dead = 0;
    IRInstr *prev = NULL;
    IRInstr *cur  = prog->head;

    while (cur) {
        if (in_dead) {
            if (cur->opcode == IR_LABEL) {
                in_dead = 0;
            } else {
                IRInstr *next = cur->next;
                if (prev) prev->next = next;
                else      prog->head = next;
                if (cur == prog->tail) prog->tail = prev;
                if (cur->dest.kind == OPND_VAR && cur->dest.u.name)
                    { free(cur->dest.u.name); cur->dest.u.name = NULL; }
                if (cur->arg1.kind == OPND_VAR && cur->arg1.u.name)
                    { free(cur->arg1.u.name); cur->arg1.u.name = NULL; }
                if (cur->arg2.kind == OPND_VAR && cur->arg2.u.name)
                    { free(cur->arg2.u.name); cur->arg2.u.name = NULL; }
                free(cur);
                cur = next;
                changed = 1;
                continue;
            }
        }

        if (cur->opcode == IR_IFFALSE && is_const_opnd(cur->arg1)) {
            double cv = const_opnd_val(cur->arg1);
            if (cv == 0.0) {
                cur->opcode = IR_GOTO;
                cur->arg1   = cur->arg2;
                cur->arg2   = opnd_none();
                in_dead = 1;
                changed = 1;
            } else {
                IRInstr *next = cur->next;
                if (prev) prev->next = next;
                else      prog->head = next;
                if (cur == prog->tail) prog->tail = prev;
                free(cur);
                cur = next;
                changed = 1;
                continue;
            }
        } else if (cur->opcode == IR_GOTO) {
            in_dead = 1;
        }

        prev = cur;
        cur  = cur->next;
    }
    return changed;
}

/* Pass 4: Dead Temp Elimination — remove a definição de um temporário que
 * nunca é lido. Surge tipicamente após a propagação de constantes, que
 * substitui os usos de "tN" por uma constante e deixa "tN = ..." órfão.
 *
 * Só remove instruções sem efeito colateral observável (IR_COPY, IR_UNARYOP
 * e IR_BINOP que não seja divisão): a divisão pode abortar a execução
 * (divisão por zero / INT_MIN / -1), erro que deve ser preservado mesmo que
 * o resultado seja descartado. O ponto-fixo de ir_optimize reexecuta o passe,
 * de modo que cadeias de temporários mortos (t1 depende de t0) somem em
 * iterações sucessivas. */
static int ir_pass_dead_temp(IRProgram *prog) {
    if (prog->temp_count == 0) return 0;
    char *used = (char *)calloc(prog->temp_count, 1);
    if (!used) return 0;

    for (IRInstr *ins = prog->head; ins; ins = ins->next) {
        if (ins->arg1.kind == OPND_TEMP && ins->arg1.u.temp < prog->temp_count)
            used[ins->arg1.u.temp] = 1;
        if (ins->arg2.kind == OPND_TEMP && ins->arg2.u.temp < prog->temp_count)
            used[ins->arg2.u.temp] = 1;
    }

    int changed = 0;
    IRInstr *prev = NULL;
    IRInstr *cur  = prog->head;
    while (cur) {
        int removable = cur->opcode == IR_COPY    ||
                        cur->opcode == IR_UNARYOP ||
                        (cur->opcode == IR_BINOP && cur->op != '/');
        if (removable &&
            cur->dest.kind == OPND_TEMP &&
            cur->dest.u.temp < prog->temp_count &&
            !used[cur->dest.u.temp]) {
            IRInstr *next = cur->next;
            if (prev) prev->next = next;
            else      prog->head = next;
            if (cur == prog->tail) prog->tail = prev;
            free_operand(&cur->dest);
            free_operand(&cur->arg1);
            free_operand(&cur->arg2);
            free(cur);
            cur = next;
            changed = 1;
            continue;
        }
        prev = cur;
        cur  = cur->next;
    }

    free(used);
    return changed;
}

/* Pass 5: Redundant Jump Elimination — remove "goto Lk" imediatamente seguido
 * de "Lk:". Esse desvio inútil sobra, por exemplo, de um if(false) sem else
 * depois que o DCE descarta o corpo morto, deixando o goto colado ao próprio
 * rótulo de destino. */
static int ir_pass_redundant_goto(IRProgram *prog) {
    int changed = 0;
    IRInstr *prev = NULL;
    IRInstr *cur  = prog->head;
    while (cur) {
        if (cur->opcode == IR_GOTO &&
            cur->arg1.kind == OPND_LABEL &&
            cur->next &&
            cur->next->opcode == IR_LABEL &&
            cur->next->dest.u.label == cur->arg1.u.label) {
            IRInstr *next = cur->next;          /* o rótulo permanece */
            if (prev) prev->next = next;
            else      prog->head = next;
            /* cur->next existe, logo cur nunca é a cauda aqui */
            free(cur);                          /* goto não tem operando-variável */
            cur = next;
            changed = 1;
            continue;
        }
        prev = cur;
        cur  = cur->next;
    }
    return changed;
}

/* Pass 6: Dead Label Elimination — remove a definição de um rótulo "Lk:" que
 * não é alvo de nenhum desvio (goto/ifFalse). Esses rótulos órfãos sobram, por
 * exemplo, depois que o DCE e a eliminação de desvio redundante esvaziam o
 * corpo de um if(false)/while(false), deixando "Lk:" sem nenhuma referência. */
static int ir_pass_dead_label(IRProgram *prog) {
    if (prog->label_count == 0) return 0;
    char *used = (char *)calloc(prog->label_count, 1);
    if (!used) return 0;

    for (IRInstr *ins = prog->head; ins; ins = ins->next) {
        if (ins->opcode == IR_GOTO &&
            ins->arg1.kind == OPND_LABEL &&
            ins->arg1.u.label < prog->label_count)
            used[ins->arg1.u.label] = 1;
        if (ins->opcode == IR_IFFALSE &&
            ins->arg2.kind == OPND_LABEL &&
            ins->arg2.u.label < prog->label_count)
            used[ins->arg2.u.label] = 1;
    }

    int changed = 0;
    IRInstr *prev = NULL;
    IRInstr *cur  = prog->head;
    while (cur) {
        if (cur->opcode == IR_LABEL &&
            cur->dest.kind == OPND_LABEL &&
            cur->dest.u.label < prog->label_count &&
            !used[cur->dest.u.label]) {
            IRInstr *next = cur->next;
            if (prev) prev->next = next;
            else      prog->head = next;
            if (cur == prog->tail) prog->tail = prev;
            free(cur);                          /* rótulo não tem operando-variável */
            cur = next;
            changed = 1;
            continue;
        }
        prev = cur;
        cur  = cur->next;
    }

    free(used);
    return changed;
}

void ir_optimize(IRProgram *prog) {
    if (!prog) return;
    int changed;
    do {
        changed  = ir_pass_const_fold(prog);
        changed |= ir_pass_const_propagate(prog);
        changed |= ir_pass_dce(prog);
        changed |= ir_pass_dead_temp(prog);
        changed |= ir_pass_redundant_goto(prog);
        changed |= ir_pass_dead_label(prog);
    } while (changed);
}

/* ====================================================
 * Execução do IR
 * ==================================================== */

typedef struct { double val; SymType type; } IRValue;

static SymValue ir_to_sym_value(SymType type, double val) {
    SymValue v;
    switch (type) {
        case TYPE_FLOAT: v.fVal = (float)val;     break;
        case TYPE_CHAR:  v.cVal = (char)(int)val; break;
        case TYPE_BOOL:  v.iVal = !!((int)val);   break;
        default:         v.iVal = (int)val;        break;
    }
    return v;
}

static double ir_sym_val_as_double(const SymEntry *e) {
    switch (e->type) {
        case TYPE_FLOAT: return (double)e->value.fVal;
        case TYPE_CHAR:  return (double)e->value.cVal;
        default:         return (double)e->value.iVal;
    }
}

static void ir_print_value(SymType type, double val) {
    switch (type) {
        case TYPE_FLOAT: printf("%g", val); break;
        case TYPE_CHAR: {
            char c = (char)(int)val;
            if (c >= 32 && c < 127) printf("'%c'", c);
            else                    printf("%d", (int)c);
            break;
        }
        case TYPE_BOOL: printf("%s", ((int)val) ? "true" : "false"); break;
        default:        printf("%d", (int)val); break;
    }
}

static IRValue ir_read_operand(IROperand op, IRValue *temps) {
    IRValue v = {0.0, TYPE_INT};
    switch (op.kind) {
    case OPND_CONST_INT:
    case OPND_CONST_CHAR:
    case OPND_CONST_BOOL:
        v.val  = (double)op.u.iconst;
        v.type = op.type;
        break;
    case OPND_CONST_FLOAT:
        v.val  = op.u.fconst;
        v.type = TYPE_FLOAT;
        break;
    case OPND_TEMP:
        if (temps) v = temps[op.u.temp];
        break;
    case OPND_VAR: {
        SymEntry *e = sym_lookup(op.u.name);
        if (e) { v.val = ir_sym_val_as_double(e); v.type = e->type; }
        break;
    }
    default:
        break;
    }
    return v;
}

static IRValue ir_exec_binop(int op, IRValue l, IRValue r) {
    IRValue res;
    int use_float = (l.type == TYPE_FLOAT || r.type == TYPE_FLOAT);
    switch (op) {
    case '+': res.val = use_float ? l.val+r.val : (double)((int)l.val+(int)r.val); break;
    case '-': res.val = use_float ? l.val-r.val : (double)((int)l.val-(int)r.val); break;
    case '*': res.val = use_float ? l.val*r.val : (double)((int)l.val*(int)r.val); break;
    case '/':
        if (r.val == 0.0) { fprintf(stderr, "Erro: divisão por zero\n"); exit(EXIT_FAILURE); }
        if (is_int_div_overflow(op, l.val, r.val, l.type, r.type)) {
            fprintf(stderr, "Erro: overflow em divisão de inteiros (INT_MIN / -1)\n");
            exit(EXIT_FAILURE);
        }
        res.val = use_float ? l.val/r.val : (double)((int)l.val/(int)r.val); break;
    case '&': res.val = (double)(l.val != 0.0 && r.val != 0.0); break;
    case '|': res.val = (double)(l.val != 0.0 || r.val != 0.0); break;
    case 'E': res.val = (double)(l.val == r.val); break;
    case 'N': res.val = (double)(l.val != r.val); break;
    case '<': res.val = (double)(l.val <  r.val); break;
    case '>': res.val = (double)(l.val >  r.val); break;
    case 'l': res.val = (double)(l.val <= r.val); break;
    case 'g': res.val = (double)(l.val >= r.val); break;
    default:  res.val = 0.0; break;
    }
    res.type = binop_result_type(op, l.type, r.type);
    return res;
}

static IRValue ir_exec_unaryop(int op, IRValue a) {
    IRValue res;
    switch (op) {
    case '-': res.val = -a.val;             res.type = a.type;   break;
    case '!': res.val = (double)(!a.val);   res.type = TYPE_INT; break;
    default:  res.val = 0.0;                res.type = TYPE_INT; break;
    }
    return res;
}

void ir_exec(IRProgram *prog) {
    if (!prog || !prog->head) return;

    int n = 0;
    for (IRInstr *ins = prog->head; ins; ins = ins->next) n++;

    IRInstr **instrs = (IRInstr **)malloc(n * sizeof(IRInstr *));
    if (!instrs) { fprintf(stderr, "Erro: ir_exec malloc\n"); exit(EXIT_FAILURE); }
    { int i = 0; for (IRInstr *ins = prog->head; ins; ins = ins->next) instrs[i++] = ins; }

    int *label_map = NULL;
    if (prog->label_count > 0) {
        label_map = (int *)calloc(prog->label_count, sizeof(int));
        if (!label_map) { fprintf(stderr, "Erro: ir_exec calloc\n"); exit(EXIT_FAILURE); }
        for (int i = 0; i < n; i++)
            if (instrs[i]->opcode == IR_LABEL)
                label_map[instrs[i]->dest.u.label] = i;
    }

    IRValue *temps = NULL;
    if (prog->temp_count > 0) {
        temps = (IRValue *)calloc(prog->temp_count, sizeof(IRValue));
        if (!temps) { fprintf(stderr, "Erro: ir_exec calloc\n"); exit(EXIT_FAILURE); }
    }

    const char *pending_decl = NULL;

    for (int ip = 0; ip < n; ) {
        IRInstr *ins = instrs[ip];
        switch (ins->opcode) {

        case IR_LABEL:
            break;

        case IR_GOTO:
            ip = label_map[ins->arg1.u.label];
            continue;

        case IR_IFFALSE: {
            IRValue cond = ir_read_operand(ins->arg1, temps);
            if (cond.val == 0.0) {
                ip = label_map[ins->arg2.u.label];
                continue;
            }
            break;
        }

        case IR_DECL: {
            SymValue zero; zero.iVal = 0;
            sym_set(ins->dest.u.name, ins->dest.type, zero);
            if (!ins->op) {
                printf("Declarado: %s : %s\n",
                       ins->dest.u.name, sym_type_name(ins->dest.type));
            } else {
                pending_decl = ins->dest.u.name;
            }
            break;
        }

        case IR_COPY: {
            IRValue val = ir_read_operand(ins->arg1, temps);
            if (ins->dest.kind == OPND_TEMP) {
                if (temps) temps[ins->dest.u.temp] = val;
            } else {
                SymValue sv = ir_to_sym_value(ins->dest.type, val.val);
                SymEntry *e = sym_set(ins->dest.u.name, ins->dest.type, sv);
                double stored = ir_sym_val_as_double(e);
                if (pending_decl &&
                    strcmp(pending_decl, ins->dest.u.name) == 0) {
                    printf("Declarado: %s : %s = ",
                           ins->dest.u.name, sym_type_name(ins->dest.type));
                    ir_print_value(ins->dest.type, stored);
                    printf("\n");
                    pending_decl = NULL;
                } else {
                    printf("%s %s = ",
                           sym_type_name(ins->dest.type), ins->dest.u.name);
                    ir_print_value(ins->dest.type, stored);
                    printf("\n");
                }
            }
            break;
        }

        case IR_BINOP: {
            IRValue l = ir_read_operand(ins->arg1, temps);
            IRValue r = ir_read_operand(ins->arg2, temps);
            if (temps) temps[ins->dest.u.temp] = ir_exec_binop(ins->op, l, r);
            break;
        }

        case IR_UNARYOP: {
            IRValue a = ir_read_operand(ins->arg1, temps);
            if (temps) temps[ins->dest.u.temp] = ir_exec_unaryop(ins->op, a);
            break;
        }

        case IR_PRINT: {
            IRValue v = ir_read_operand(ins->arg1, temps);
            printf("Resultado: ");
            ir_print_value(v.type, v.val);
            printf("\n");
            break;
        }
        }
        ip++;
    }

    free(instrs);
    if (label_map) free(label_map);
    if (temps) free(temps);
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
