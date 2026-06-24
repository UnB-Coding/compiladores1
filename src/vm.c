/******************************************************
 FGA0003 - Compiladores 1
 Curso de Engenharia de Software
 Universidade de Brasília (UnB)

 Arquivo: vm.c
 Descrição: Gerador de bytecode (AST → bytecode) e
            máquina virtual stack-based que executa
            o bytecode gerado.

 Fluxo:
   bc_compile(root)  — percorre a AST otimizada e
                        emite instruções BCInstr
   bc_print(prog)    — imprime bytecode legível
   vm_run(prog)      — executa o bytecode na VM
   bc_free(prog)     — libera memória
 ******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "symbol_table/symtab.h"

/* ====================================================
 * Constantes internas
 * ==================================================== */
#define VM_STACK_MAX 1024
#define INIT_CAP     64
#define MAX_LABELS   512

/* ====================================================
 * VMValue — valor em tempo de execução na pilha da VM
 * ==================================================== */
typedef struct { double val; SymType type; } VMValue;

/* ====================================================
 * Compiler — estado interno do gerador de bytecode
 * ==================================================== */
typedef struct {
    BCProgram *prog;

    /* Rótulos: mapeiam id → índice da instrução destino */
    int label_count;
    int label_targets[MAX_LABELS];

    /* Patchpoints: instruções JMP/JMPF que precisam de resolução */
    struct { int instr_idx; int label_id; } patches[MAX_LABELS * 2];
    int patch_count;
} Compiler;

/* ====================================================
 * Funções auxiliares do compilador
 * ==================================================== */

static void ensure_cap(BCProgram *p) {
    if (p->count < p->cap) return;
    p->cap = (p->cap == 0) ? INIT_CAP : p->cap * 2;
    p->code = realloc(p->code, (size_t)p->cap * sizeof(BCInstr));
    if (!p->code) { fprintf(stderr, "bc_compile: sem memória\n"); exit(1); }
}

static int emit_instr(Compiler *c, BCOpcode op, int iarg, double farg,
                      const char *sarg, SymType type) {
    ensure_cap(c->prog);
    int idx = c->prog->count++;
    BCInstr *ins = &c->prog->code[idx];
    ins->opcode = op;
    ins->iarg   = iarg;
    ins->farg   = farg;
    ins->sarg   = sarg ? strdup(sarg) : NULL;
    ins->type   = type;
    return idx;
}

static int new_label(Compiler *c) {
    int id = c->label_count++;
    c->label_targets[id] = -1;
    return id;
}

static void define_label(Compiler *c, int label_id) {
    c->label_targets[label_id] = c->prog->count;
}

static void emit_jmp(Compiler *c, BCOpcode op, int label_id) {
    int idx = emit_instr(c, op, -1, 0.0, NULL, TYPE_NONE);
    c->patches[c->patch_count].instr_idx = idx;
    c->patches[c->patch_count].label_id  = label_id;
    c->patch_count++;
}

static void patch_all(Compiler *c) {
    for (int i = 0; i < c->patch_count; i++) {
        int instr = c->patches[i].instr_idx;
        int lbl   = c->patches[i].label_id;
        c->prog->code[instr].iarg = c->label_targets[lbl];
    }
}

/* ====================================================
 * Gerador de bytecode — forward declarations
 * ==================================================== */
static void bc_compile_expr(Compiler *c, ASTNode *node);
static void bc_compile_stmt(Compiler *c, ASTNode *node);
static void bc_compile_list(Compiler *c, ASTNode *node);

/* ====================================================
 * Gerador de bytecode — expressões
 * Percorre a AST em pós-ordem e emite instruções de pilha.
 * ==================================================== */
static void bc_compile_expr(Compiler *c, ASTNode *node) {
    if (!node) return;

    switch (node->kind) {
    case AST_NUM:
        emit_instr(c, BC_PUSH_INT, node->data.num.value, 0.0, NULL, TYPE_INT);
        break;
    case AST_FLOAT:
        emit_instr(c, BC_PUSH_FLOAT, 0, node->data.flt.value, NULL, TYPE_FLOAT);
        break;
    case AST_BOOL:
        emit_instr(c, BC_PUSH_BOOL, node->data.bln.value, 0.0, NULL, TYPE_BOOL);
        break;
    case AST_CHAR:
        emit_instr(c, BC_PUSH_CHAR, (int)(unsigned char)node->data.chr.value,
                   0.0, NULL, TYPE_CHAR);
        break;
    case AST_ID:
        emit_instr(c, BC_LOAD_VAR, 0, 0.0, node->data.id.name, TYPE_NONE);
        break;
    case AST_BINOP:
        bc_compile_expr(c, node->data.binop.left);
        bc_compile_expr(c, node->data.binop.right);
        emit_instr(c, BC_BINOP, node->data.binop.op, 0.0, NULL, TYPE_NONE);
        break;
    case AST_UNARYOP:
        bc_compile_expr(c, node->data.unaryop.operand);
        emit_instr(c, BC_UNARYOP, node->data.unaryop.op, 0.0, NULL, TYPE_NONE);
        break;
    default:
        break;
    }
}

/* ====================================================
 * Gerador de bytecode — lista encadeada de comandos
 * ==================================================== */
static void bc_compile_list(Compiler *c, ASTNode *node) {
    while (node) {
        bc_compile_stmt(c, node);
        node = node->next;
    }
}

/* ====================================================
 * Gerador de bytecode — comandos
 * ==================================================== */
static void bc_compile_stmt(Compiler *c, ASTNode *node) {
    if (!node) return;

    switch (node->kind) {

    case AST_DECL: {
        int has_init = (node->data.decl.init != NULL) ? 1 : 0;
        emit_instr(c, BC_DECL_VAR, has_init, 0.0,
                   node->data.decl.name, node->data.decl.type);
        if (has_init) {
            bc_compile_expr(c, node->data.decl.init);
            emit_instr(c, BC_STORE_VAR, 0, 0.0,
                       node->data.decl.name, node->data.decl.type);
        }
        break;
    }

    case AST_ASSIGN:
        bc_compile_expr(c, node->data.assign.expr);
        emit_instr(c, BC_STORE_VAR, 0, 0.0,
                   node->data.assign.name, TYPE_NONE);
        break;

    case AST_EXPR_STMT:
        bc_compile_expr(c, node->data.expr_stmt.expr);
        emit_instr(c, BC_PRINT_EXPR, 0, 0.0, NULL, TYPE_NONE);
        break;

    case AST_IF: {
        int L_else = new_label(c);

        bc_compile_expr(c, node->data.if_stmt.cond);
        emit_jmp(c, BC_JMPF, L_else);

        bc_compile_list(c, node->data.if_stmt.then_branch);

        if (node->data.if_stmt.else_branch) {
            int L_end = new_label(c);
            emit_jmp(c, BC_JMP, L_end);
            define_label(c, L_else);
            bc_compile_list(c, node->data.if_stmt.else_branch);
            define_label(c, L_end);
        } else {
            define_label(c, L_else);
        }
        break;
    }

    case AST_WHILE: {
        int L_start = new_label(c);
        int L_end   = new_label(c);

        define_label(c, L_start);
        bc_compile_expr(c, node->data.while_stmt.cond);
        emit_jmp(c, BC_JMPF, L_end);

        bc_compile_list(c, node->data.while_stmt.body);

        emit_jmp(c, BC_JMP, L_start);
        define_label(c, L_end);
        break;
    }

    case AST_FOR: {
        int L_start = new_label(c);
        int L_end   = new_label(c);

        if (node->data.for_stmt.init)
            bc_compile_stmt(c, node->data.for_stmt.init);

        define_label(c, L_start);
        if (node->data.for_stmt.cond) {
            bc_compile_expr(c, node->data.for_stmt.cond);
            emit_jmp(c, BC_JMPF, L_end);
        }

        bc_compile_list(c, node->data.for_stmt.body);

        if (node->data.for_stmt.step)
            bc_compile_stmt(c, node->data.for_stmt.step);

        emit_jmp(c, BC_JMP, L_start);
        define_label(c, L_end);
        break;
    }

    case AST_BLOCK:
        bc_compile_list(c, node->data.block.stmts);
        break;

    default:
        bc_compile_expr(c, node);
        break;
    }
}

/* ====================================================
 * bc_compile: API pública — compila AST para bytecode
 * ==================================================== */
BCProgram *bc_compile(ASTNode *root) {
    BCProgram *prog = calloc(1, sizeof(BCProgram));
    if (!prog) { fprintf(stderr, "bc_compile: sem memória\n"); exit(1); }

    Compiler c;
    memset(&c, 0, sizeof(c));
    c.prog = prog;

    bc_compile_list(&c, root);
    emit_instr(&c, BC_HALT, 0, 0.0, NULL, TYPE_NONE);

    patch_all(&c);
    return prog;
}

/* ====================================================
 * bc_print: imprime bytecode legível
 * ==================================================== */
static const char *opcode_name(BCOpcode op) {
    switch (op) {
    case BC_PUSH_INT:   return "PUSH_INT";
    case BC_PUSH_FLOAT: return "PUSH_FLOAT";
    case BC_PUSH_BOOL:  return "PUSH_BOOL";
    case BC_PUSH_CHAR:  return "PUSH_CHAR";
    case BC_DECL_VAR:   return "DECL_VAR";
    case BC_LOAD_VAR:   return "LOAD_VAR";
    case BC_STORE_VAR:  return "STORE_VAR";
    case BC_BINOP:      return "BINOP";
    case BC_UNARYOP:    return "UNARYOP";
    case BC_JMP:        return "JMP";
    case BC_JMPF:       return "JMPF";
    case BC_PRINT_EXPR: return "PRINT_EXPR";
    case BC_HALT:       return "HALT";
    default:            return "UNKNOWN";
    }
}

static const char *op_str(int op) {
    switch (op) {
    case '+': return "+";  case '-': return "-";
    case '*': return "*";  case '/': return "/";
    case '&': return "&&"; case '|': return "||";
    case '!': return "!";  case 'E': return "==";
    case 'N': return "!="; case '<': return "<";
    case '>': return ">";  case 'l': return "<=";
    case 'g': return ">="; default:  return "?";
    }
}

void bc_print(const BCProgram *prog) {
    for (int i = 0; i < prog->count; i++) {
        const BCInstr *ins = &prog->code[i];
        printf("  %3d: %s", i, opcode_name(ins->opcode));
        switch (ins->opcode) {
        case BC_PUSH_INT:
            printf(" %d", ins->iarg);
            break;
        case BC_PUSH_FLOAT:
            printf(" %g", ins->farg);
            break;
        case BC_PUSH_BOOL:
            printf(" %s", ins->iarg ? "true" : "false");
            break;
        case BC_PUSH_CHAR:
            if (ins->iarg >= 32 && ins->iarg < 127)
                printf(" '%c'", (char)ins->iarg);
            else
                printf(" %d", ins->iarg);
            break;
        case BC_DECL_VAR:
            printf(" %s %s%s", ins->sarg, sym_type_name(ins->type),
                   ins->iarg ? " (init)" : "");
            break;
        case BC_LOAD_VAR:
        case BC_STORE_VAR:
            printf(" %s", ins->sarg);
            break;
        case BC_BINOP:
        case BC_UNARYOP:
            printf(" %s", op_str(ins->iarg));
            break;
        case BC_JMP:
        case BC_JMPF:
            printf(" %d", ins->iarg);
            break;
        default:
            break;
        }
        printf("\n");
    }
}

/* ====================================================
 * bc_free: libera memória do programa de bytecode
 * ==================================================== */
void bc_free(BCProgram *prog) {
    if (!prog) return;
    for (int i = 0; i < prog->count; i++)
        free(prog->code[i].sarg);
    free(prog->code);
    free(prog);
}

/* ====================================================
 * Funções auxiliares da VM — espelham ast.c
 * ==================================================== */

static SymType promote_type(SymType a, SymType b) {
    if (a == TYPE_FLOAT || b == TYPE_FLOAT) return TYPE_FLOAT;
    return TYPE_INT;
}

static SymValue to_sym_value(SymType type, double val) {
    SymValue v;
    switch (type) {
    case TYPE_FLOAT: v.fVal = (float)val;        break;
    case TYPE_CHAR:  v.cVal = (char)(int)val;    break;
    case TYPE_BOOL:  v.iVal = !!((int)val);      break;
    default:         v.iVal = (int)val;           break;
    }
    return v;
}

static double sym_val_as_double(const SymEntry *e) {
    switch (e->type) {
    case TYPE_FLOAT: return (double)e->value.fVal;
    case TYPE_CHAR:  return (double)e->value.cVal;
    default:         return (double)e->value.iVal;
    }
}

static void print_value(SymType type, double val) {
    switch (type) {
    case TYPE_FLOAT: printf("%g", val); break;
    case TYPE_CHAR: {
        char ch = (char)(int)val;
        if (ch >= 32 && ch < 127) printf("'%c'", ch);
        else printf("%d", (int)ch);
        break;
    }
    case TYPE_BOOL: printf("%s", ((int)val) ? "true" : "false"); break;
    default:        printf("%d", (int)val); break;
    }
}

static double compute_binop(int op, double lv, double rv,
                            SymType lt, SymType rt) {
    SymType rtype = promote_type(lt, rt);
    switch (op) {
    case '+':
        return rtype==TYPE_FLOAT ? lv+rv : (double)((int)lv+(int)rv);
    case '-':
        return rtype==TYPE_FLOAT ? lv-rv : (double)((int)lv-(int)rv);
    case '*':
        return rtype==TYPE_FLOAT ? lv*rv : (double)((int)lv*(int)rv);
    case '/':
        if (rv == 0.0) { fprintf(stderr,"Erro: divisão por zero\n"); exit(1); }
        return rtype==TYPE_FLOAT ? lv/rv : (double)((int)lv/(int)rv);
    case '&': return (double)(lv && rv);
    case '|': return (double)(lv || rv);
    case 'E': return (double)(lv == rv);
    case 'N': return (double)(lv != rv);
    case '<': return (double)(lv <  rv);
    case '>': return (double)(lv >  rv);
    case 'l': return (double)(lv <= rv);
    case 'g': return (double)(lv >= rv);
    default:
        fprintf(stderr, "Erro VM: operador desconhecido '%c'\n", op);
        exit(1);
    }
}

static SymType binop_result_type(int op, SymType lt, SymType rt) {
    switch (op) {
    case '+': case '-': case '*': case '/':
        return promote_type(lt, rt);
    default:
        return TYPE_INT;
    }
}

/* ====================================================
 * vm_run: executa o bytecode na VM stack-based.
 *
 * Saída idêntica a eval_ast():
 *   declaração com init  → "Declarado: x : int = 5"
 *   declaração sem init  → "Declarado: x : int"
 *   atribuição normal    → "int x = 7"
 *   expressão stmt       → "Resultado: 7"
 * ==================================================== */
void vm_run(const BCProgram *prog) {
    VMValue stack[VM_STACK_MAX];
    int sp = -1;

    /* Rastreia se o próximo STORE_VAR é de uma declaração com init */
    int     last_decl_pending = 0;
    char    last_decl_name[256] = "";
    SymType last_decl_type = TYPE_NONE;

    int ip = 0;
    while (ip < prog->count) {
        const BCInstr *ins = &prog->code[ip];

        switch (ins->opcode) {

        case BC_PUSH_INT:
            stack[++sp] = (VMValue){ (double)ins->iarg, TYPE_INT };
            break;

        case BC_PUSH_FLOAT:
            stack[++sp] = (VMValue){ ins->farg, TYPE_FLOAT };
            break;

        case BC_PUSH_BOOL:
            stack[++sp] = (VMValue){ (double)ins->iarg, TYPE_BOOL };
            break;

        case BC_PUSH_CHAR:
            stack[++sp] = (VMValue){ (double)ins->iarg, TYPE_CHAR };
            break;

        case BC_LOAD_VAR: {
            SymEntry *e = sym_lookup(ins->sarg);
            stack[++sp] = (VMValue){ sym_val_as_double(e), e->type };
            break;
        }

        case BC_DECL_VAR: {
            /* Registra variável na tabela de símbolos com valor zero */
            SymValue zero; zero.iVal = 0;
            sym_set(ins->sarg, ins->type, zero);

            if (ins->iarg) {
                /* has_init=1: aguarda o próximo STORE_VAR para imprimir */
                last_decl_pending = 1;
                strncpy(last_decl_name, ins->sarg,
                        sizeof(last_decl_name) - 1);
                last_decl_name[sizeof(last_decl_name) - 1] = '\0';
                last_decl_type = ins->type;
            } else {
                /* has_init=0: imprime "Declarado: x : int" agora */
                printf("Declarado: %s : %s\n",
                       ins->sarg, sym_type_name(ins->type));
                last_decl_pending = 0;
            }
            break;
        }

        case BC_STORE_VAR: {
            VMValue v = stack[sp--];
            SymEntry *e = sym_lookup(ins->sarg);
            if (!e) {
                fprintf(stderr, "Erro VM: variável '%s' não declarada\n",
                        ins->sarg);
                exit(1);
            }
            e->value = to_sym_value(e->type, v.val);
            double stored = sym_val_as_double(e);

            if (last_decl_pending &&
                strcmp(last_decl_name, ins->sarg) == 0) {
                /* Inicialização de declaração → "Declarado: x : int = 5" */
                printf("Declarado: %s : %s = ",
                       ins->sarg, sym_type_name(e->type));
                print_value(e->type, stored);
                printf("\n");
                last_decl_pending = 0;
            } else {
                /* Atribuição normal → "int x = 7" */
                printf("%s %s = ", sym_type_name(e->type), ins->sarg);
                print_value(e->type, stored);
                printf("\n");
            }
            break;
        }

        case BC_BINOP: {
            VMValue r = stack[sp--];
            VMValue l = stack[sp--];
            double res = compute_binop(ins->iarg, l.val, r.val,
                                       l.type, r.type);
            SymType rt = binop_result_type(ins->iarg, l.type, r.type);
            stack[++sp] = (VMValue){ res, rt };
            break;
        }

        case BC_UNARYOP: {
            VMValue v = stack[sp--];
            double res;
            SymType rt;
            if (ins->iarg == '-') {
                res = -v.val;
                rt  = v.type;
            } else { /* '!' */
                res = (double)(!v.val);
                rt  = TYPE_INT;
            }
            stack[++sp] = (VMValue){ res, rt };
            break;
        }

        case BC_JMP:
            ip = ins->iarg;
            continue;

        case BC_JMPF: {
            double top = stack[sp--].val;
            if (top == 0.0) {
                ip = ins->iarg;
                continue;
            }
            break;
        }

        case BC_PRINT_EXPR: {
            VMValue v = stack[sp--];
            printf("Resultado: ");
            print_value(v.type, v.val);
            printf("\n");
            break;
        }

        case BC_HALT:
            return;
        }

        ip++;
    }
}
