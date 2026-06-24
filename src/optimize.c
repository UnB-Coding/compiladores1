/******************************************************
 FGA0003 - Compiladores 1
 Curso de Engenharia de Software
 Universidade de Brasília (UnB)

 Arquivo: optimize.c
 Descrição: Implementação da Otimização da AST

 Estratégia:
   - Travessia recursiva em pós-ordem (filhos antes do pai),
     usando double-pointers para substituição in-place.
   - Laço de ponto fixo: repete até uma varredura completa
     não produzir nenhuma nova modificação.

 Gerenciamento de memória:
   - Ao substituir um nó, o ponteiro next do nó original é
     transferido para o substituto antes de free_ast().
   - free_ast() é chamado com next = NULL para não encadear
     a liberação da continuação da lista.
 ******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "optimize.h"
#include "symbol_table/symtab.h"

/* ====================================================
 * Flag de ponto fixo: setada em 1 sempre que uma
 * transformação é aplicada durante uma varredura.
 * ==================================================== */
static int g_changed;

/* ====================================================
 * Funções auxiliares para literais
 * ==================================================== */

static int is_literal(ASTNode *n) {
    return n && (n->kind == AST_NUM ||
                 n->kind == AST_FLOAT ||
                 n->kind == AST_BOOL);
}

static double literal_value(ASTNode *n) {
    switch (n->kind) {
    case AST_NUM:   return (double)n->data.num.value;
    case AST_FLOAT: return n->data.flt.value;
    case AST_BOOL:  return (double)n->data.bln.value;
    default:        return 0.0;
    }
}

static SymType literal_type(ASTNode *n) {
    switch (n->kind) {
    case AST_FLOAT: return TYPE_FLOAT;
    case AST_BOOL:  return TYPE_BOOL;
    default:        return TYPE_INT;
    }
}

/* Cria o nó literal adequado para um valor e tipo resultante */
static ASTNode *make_literal(SymType type, double val) {
    switch (type) {
    case TYPE_FLOAT: return new_float_node(val);
    default:         return new_num_node((int)val);
    }
}

/* Promoção de tipo — espelha ast.c e ir.c */
static SymType promote_type(SymType a, SymType b) {
    if (a == TYPE_FLOAT || b == TYPE_FLOAT) return TYPE_FLOAT;
    return TYPE_INT;
}

/* Tipo resultante de uma operação binária */
static SymType binop_result_type(int op, SymType l, SymType r) {
    switch (op) {
    case '+': case '-': case '*': case '/':
        return promote_type(l, r);
    default:
        return TYPE_INT;
    }
}

/* Calcula o resultado de uma operação binária entre literais */
static double compute_binop(int op, double lv, double rv, SymType lt, SymType rt) {
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

/* ====================================================
 * Declarações antecipadas (recursão mútua)
 * ==================================================== */
static void optimize_node(ASTNode **node_ptr);
static void optimize_list(ASTNode **head_ptr);

/* ====================================================
 * try_fold_binop: tenta dobrar BINOP com ambos os filhos
 * literais num único nó literal.
 * ==================================================== */
static void try_fold_binop(ASTNode **node_ptr) {
    ASTNode *n = *node_ptr;
    ASTNode *left  = n->data.binop.left;
    ASTNode *right = n->data.binop.right;

    if (!is_literal(left) || !is_literal(right)) return;

    int op = n->data.binop.op;

    /* Não dobra divisão por zero (já bloqueada pela análise semântica,
     * mas defensivo para divisão por variável zerada em tempo de compilação). */
    if (op == '/' && literal_value(right) == 0.0) return;

    SymType lt = literal_type(left);
    SymType rt = literal_type(right);
    SymType rtype = binop_result_type(op, lt, rt);
    double  result = compute_binop(op, literal_value(left), literal_value(right), lt, rt);

    ASTNode *new_node = make_literal(rtype, result);
    new_node->next = n->next;
    n->next = NULL;   /* evita que free_ast encadeie a continuação */
    free_ast(n);
    *node_ptr = new_node;
    g_changed = 1;
}

/* ====================================================
 * try_fold_unary: tenta dobrar UNARYOP com operando
 * literal num único nó literal.
 * ==================================================== */
static void try_fold_unary(ASTNode **node_ptr) {
    ASTNode *n = *node_ptr;
    ASTNode *operand = n->data.unaryop.operand;

    if (!is_literal(operand)) return;

    int op = n->data.unaryop.op;
    SymType otype  = literal_type(operand);
    double  oval   = literal_value(operand);
    double  result;
    SymType rtype;

    switch (op) {
    case '-':
        result = -oval;
        rtype  = otype;
        break;
    case '!':
        result = (double)(!oval);
        rtype  = TYPE_INT;
        break;
    default:
        return;
    }

    ASTNode *new_node = make_literal(rtype, result);
    new_node->next = n->next;
    n->next = NULL;
    free_ast(n);
    *node_ptr = new_node;
    g_changed = 1;
}

/* ====================================================
 * try_dce_if: elimina AST_IF quando a condição é um
 * literal — substitui pelo branch correto.
 * ==================================================== */
static void try_dce_if(ASTNode **node_ptr) {
    ASTNode *n = *node_ptr;
    ASTNode *cond = n->data.if_stmt.cond;

    if (!is_literal(cond)) return;

    double cond_val = literal_value(cond);

    ASTNode *keep;
    ASTNode *discard;

    if (cond_val != 0.0) {
        keep    = n->data.if_stmt.then_branch;
        discard = n->data.if_stmt.else_branch;
    } else {
        keep    = n->data.if_stmt.else_branch;
        discard = n->data.if_stmt.then_branch;
    }

    /* Costura o branch mantido com a continuação da lista */
    if (keep) {
        /* Avança até o último nó do branch (next é NULL nos branches diretos,
         * mas defensivo caso haja listas internas). */
        ASTNode *last = keep;
        while (last->next) last = last->next;
        last->next = n->next;
    } else {
        /* Branch inexistente: o if desaparece por completo; a continuação
         * torna-se o novo *node_ptr. */
        keep = n->next;
    }

    /* Desliga os ponteiros do if para liberar apenas o necessário */
    ASTNode *cond_saved = n->data.if_stmt.cond;
    n->data.if_stmt.cond        = NULL;
    n->data.if_stmt.then_branch = NULL;
    n->data.if_stmt.else_branch = NULL;
    n->next = NULL;

    free_ast(cond_saved);
    free_ast(discard);
    free(n);   /* libera só o struct do if — filhos já desligados */

    *node_ptr = keep;
    g_changed = 1;
}

/* ====================================================
 * try_dce_while: elimina AST_WHILE(0) — corpo nunca
 * executa, remove o nó da lista.
 * ==================================================== */
static void try_dce_while(ASTNode **node_ptr) {
    ASTNode *n = *node_ptr;
    ASTNode *cond = n->data.while_stmt.cond;

    if (!is_literal(cond) || literal_value(cond) != 0.0) return;

    ASTNode *next = n->next;
    n->next = NULL;   /* evita que free_ast encadeie a continuação */
    free_ast(n);      /* libera cond + body + struct */
    *node_ptr = next;
    g_changed = 1;
}

/* ====================================================
 * optimize_node: traversal pós-ordem sobre um único nó.
 * Usa double-pointer para permitir substituição in-place.
 * ==================================================== */
static void optimize_node(ASTNode **node_ptr) {
    ASTNode *n = *node_ptr;
    if (!n) return;

    switch (n->kind) {

    /* ---- Literais e identificadores: nada a fazer ---- */
    case AST_NUM:
    case AST_FLOAT:
    case AST_CHAR:
    case AST_BOOL:
    case AST_ID:
        break;

    /* ---- Operação binária: otimiza filhos, tenta dobrar ---- */
    case AST_BINOP:
        optimize_node(&n->data.binop.left);
        optimize_node(&n->data.binop.right);
        try_fold_binop(node_ptr);
        break;

    /* ---- Operação unária: otimiza operando, tenta dobrar ---- */
    case AST_UNARYOP:
        optimize_node(&n->data.unaryop.operand);
        try_fold_unary(node_ptr);
        break;

    /* ---- Declaração: otimiza expressão de inicialização ---- */
    case AST_DECL:
        if (n->data.decl.init)
            optimize_node(&n->data.decl.init);
        break;

    /* ---- Atribuição: otimiza lado direito ---- */
    case AST_ASSIGN:
        optimize_node(&n->data.assign.expr);
        break;

    /* ---- Expressão como comando: otimiza a expressão ---- */
    case AST_EXPR_STMT:
        optimize_node(&n->data.expr_stmt.expr);
        break;

    /* ---- If / If-Else: otimiza condição, tenta DCE ---- */
    case AST_IF:
        optimize_node(&n->data.if_stmt.cond);
        try_dce_if(node_ptr);
        if (*node_ptr == n) {
            /* Não foi eliminado: otimiza os branches */
            optimize_list(&n->data.if_stmt.then_branch);
            if (n->data.if_stmt.else_branch)
                optimize_list(&n->data.if_stmt.else_branch);
        }
        break;

    /* ---- While: otimiza condição, tenta DCE ---- */
    case AST_WHILE:
        optimize_node(&n->data.while_stmt.cond);
        try_dce_while(node_ptr);
        if (*node_ptr == n) {
            optimize_list(&n->data.while_stmt.body);
        }
        break;

    /* ---- For: otimiza cada componente e o corpo ---- */
    case AST_FOR:
        if (n->data.for_stmt.init)
            optimize_node(&n->data.for_stmt.init);
        if (n->data.for_stmt.cond)
            optimize_node(&n->data.for_stmt.cond);
        optimize_list(&n->data.for_stmt.body);
        if (n->data.for_stmt.step)
            optimize_node(&n->data.for_stmt.step);
        break;

    /* ---- Bloco: otimiza a lista de statements internos ---- */
    case AST_BLOCK:
        optimize_list(&n->data.block.stmts);
        break;
    }
}

/* ====================================================
 * optimize_list: itera uma lista encadeada via ->next,
 * chamando optimize_node para cada elemento.
 *
 * Usa double-pointer: optimize_node pode substituir ou
 * remover *ptr; se *ptr mudar, o próximo passo do loop
 * continuará a partir do novo valor.
 * ==================================================== */
static void optimize_list(ASTNode **head_ptr) {
    ASTNode **ptr = head_ptr;
    while (*ptr) {
        optimize_node(ptr);
        if (*ptr) ptr = &((*ptr)->next);
        /* Se *ptr tornou-se NULL (nó eliminado sem sucessor),
         * a condição do while termina o loop naturalmente. */
    }
}

/* ====================================================
 * optimize_ast: ponto de entrada público.
 *
 * Executa varreduras até atingir ponto fixo (nenhuma
 * nova transformação em uma passagem completa).
 * ==================================================== */
ASTNode *optimize_ast(ASTNode *root) {
    do {
        g_changed = 0;
        optimize_list(&root);
    } while (g_changed);
    return root;
}
