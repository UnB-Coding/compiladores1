/******************************************************
 FGA0003 - Compiladores 1
 Curso de Engenharia de Software
 Universidade de Brasília (UnB)

 Arquivo: ast.c
 Descrição: Implementação da Árvore Sintática Abstrata

 Contém:
   - Construtores de nós (alocação + inicialização)
   - print_ast(): impressão indentada da árvore
   - free_ast(): liberação recursiva de memória

 A execução do programa NÃO ocorre aqui: após a análise
 semântica, a AST é traduzida para código intermediário
 (ir.c) e executada por ir_exec().
 ******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symbol_table/symtab.h"

/* ====================================================
 * Macro auxiliar: aloca um nó com tipo definido
 * ==================================================== */
static ASTNode *alloc_node(NodeType kind) {
    ASTNode *n = (ASTNode *)calloc(1, sizeof(ASTNode));
    if (!n) {
        fprintf(stderr, "Erro: falha ao alocar nó AST\n");
        exit(EXIT_FAILURE);
    }
    n->kind = kind;
    n->next = NULL;
    return n;
}

/* ====================================================
 * Construtores de nós
 * ==================================================== */

ASTNode *new_num_node(int value) {
    ASTNode *n = alloc_node(AST_NUM);
    n->data.num.value = value;
    return n;
}

ASTNode *new_float_node(double value) {
    ASTNode *n = alloc_node(AST_FLOAT);
    n->data.flt.value = value;
    return n;
}

ASTNode *new_char_node(char value) {
    ASTNode *n = alloc_node(AST_CHAR);
    n->data.chr.value = value;
    return n;
}

ASTNode *new_bool_node(int value) {
    ASTNode *n = alloc_node(AST_BOOL);
    n->data.bln.value = value;
    return n;
}

ASTNode *new_id_node(const char *name) {
    ASTNode *n = alloc_node(AST_ID);
    n->data.id.name = strdup(name);
    return n;
}

ASTNode *new_binop_node(int op, ASTNode *left, ASTNode *right) {
    ASTNode *n = alloc_node(AST_BINOP);
    n->data.binop.op    = op;
    n->data.binop.left  = left;
    n->data.binop.right = right;
    return n;
}

ASTNode *new_unaryop_node(int op, ASTNode *operand) {
    ASTNode *n = alloc_node(AST_UNARYOP);
    n->data.unaryop.op      = op;
    n->data.unaryop.operand = operand;
    return n;
}

ASTNode *new_assign_node(const char *name, ASTNode *expr) {
    ASTNode *n = alloc_node(AST_ASSIGN);
    n->data.assign.name = strdup(name);
    n->data.assign.expr = expr;
    return n;
}

ASTNode *new_decl_node(SymType type, const char *name, ASTNode *init) {
    ASTNode *n = alloc_node(AST_DECL);
    n->data.decl.type = type;
    n->data.decl.name = strdup(name);
    n->data.decl.init = init;
    return n;
}

ASTNode *new_expr_stmt_node(ASTNode *expr) {
    ASTNode *n = alloc_node(AST_EXPR_STMT);
    n->data.expr_stmt.expr = expr;
    return n;
}

ASTNode *new_if_node(ASTNode *cond, ASTNode *then_b, ASTNode *else_b) {
    ASTNode *n = alloc_node(AST_IF);
    n->data.if_stmt.cond        = cond;
    n->data.if_stmt.then_branch = then_b;
    n->data.if_stmt.else_branch = else_b;
    return n;
}

ASTNode *new_while_node(ASTNode *cond, ASTNode *body) {
    ASTNode *n = alloc_node(AST_WHILE);
    n->data.while_stmt.cond = cond;
    n->data.while_stmt.body = body;
    return n;
}

ASTNode *new_for_node(ASTNode *init, ASTNode *cond, ASTNode *step,
                      ASTNode *body) {
    ASTNode *n = alloc_node(AST_FOR);
    n->data.for_stmt.init = init;
    n->data.for_stmt.cond = cond;
    n->data.for_stmt.step = step;
    n->data.for_stmt.body = body;
    return n;
}

ASTNode *new_block_node(ASTNode *stmts) {
    ASTNode *n = alloc_node(AST_BLOCK);
    n->data.block.stmts = stmts;
    return n;
}

/* ====================================================
 * Impressão visual da AST (para demonstração)
 *
 * Percorre a árvore recursivamente, imprimindo cada nó
 * com indentação proporcional ao nível de profundidade.
 * Ao final de cada nó, percorre a lista encadeada (->next).
 * ==================================================== */

/* Converte o código interno do operador para string legível */
static const char *op_to_str(int op) {
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

void print_ast(ASTNode *node, int level) {
    if (!node) return;

    /* Indentação: 2 espaços por nível */
    for (int i = 0; i < level; i++) printf("  ");

    switch (node->kind) {

    case AST_NUM:
        printf("NUM (%d)\n", node->data.num.value);
        break;

    case AST_FLOAT:
        printf("FLOAT (%g)\n", node->data.flt.value);
        break;

    case AST_CHAR:
        printf("CHAR ('%c')\n", node->data.chr.value);
        break;

    case AST_BOOL:
        printf("BOOL (%s)\n", node->data.bln.value ? "true" : "false");
        break;

    case AST_ID:
        printf("ID (%s)\n", node->data.id.name);
        break;

    case AST_BINOP:
        printf("BINOP (%s)\n", op_to_str(node->data.binop.op));
        print_ast(node->data.binop.left,  level + 1);
        print_ast(node->data.binop.right, level + 1);
        break;

    case AST_UNARYOP:
        printf("UNARYOP (%s)\n", op_to_str(node->data.unaryop.op));
        print_ast(node->data.unaryop.operand, level + 1);
        break;

    case AST_ASSIGN:
        printf("ASSIGN (%s)\n", node->data.assign.name);
        print_ast(node->data.assign.expr, level + 1);
        break;

    case AST_DECL:
        printf("DECL (%s : %s)\n",
               node->data.decl.name,
               sym_type_name(node->data.decl.type));
        if (node->data.decl.init) {
            for (int i = 0; i < level + 1; i++) printf("  ");
            printf("init:\n");
            print_ast(node->data.decl.init, level + 2);
        }
        break;

    case AST_EXPR_STMT:
        printf("EXPR_STMT\n");
        print_ast(node->data.expr_stmt.expr, level + 1);
        break;

    case AST_IF:
        printf("IF\n");
        for (int i = 0; i < level + 1; i++) printf("  ");
        printf("cond:\n");
        print_ast(node->data.if_stmt.cond, level + 2);
        for (int i = 0; i < level + 1; i++) printf("  ");
        printf("then:\n");
        print_ast(node->data.if_stmt.then_branch, level + 2);
        if (node->data.if_stmt.else_branch) {
            for (int i = 0; i < level + 1; i++) printf("  ");
            printf("else:\n");
            print_ast(node->data.if_stmt.else_branch, level + 2);
        }
        break;

    case AST_WHILE:
        printf("WHILE\n");
        for (int i = 0; i < level + 1; i++) printf("  ");
        printf("cond:\n");
        print_ast(node->data.while_stmt.cond, level + 2);
        for (int i = 0; i < level + 1; i++) printf("  ");
        printf("body:\n");
        print_ast(node->data.while_stmt.body, level + 2);
        break;

    case AST_FOR:
        printf("FOR\n");
        if (node->data.for_stmt.init) {
            for (int i = 0; i < level + 1; i++) printf("  ");
            printf("init:\n");
            print_ast(node->data.for_stmt.init, level + 2);
        }
        if (node->data.for_stmt.cond) {
            for (int i = 0; i < level + 1; i++) printf("  ");
            printf("cond:\n");
            print_ast(node->data.for_stmt.cond, level + 2);
        }
        if (node->data.for_stmt.step) {
            for (int i = 0; i < level + 1; i++) printf("  ");
            printf("step:\n");
            print_ast(node->data.for_stmt.step, level + 2);
        }
        for (int i = 0; i < level + 1; i++) printf("  ");
        printf("body:\n");
        print_ast(node->data.for_stmt.body, level + 2);
        break;

    case AST_BLOCK:
        printf("BLOCK\n");
        print_ast(node->data.block.stmts, level + 1);
        break;
    }

    /* Percorre a lista encadeada (próximo comando no mesmo nível) */
    print_ast(node->next, level);
}

/* ====================================================
 * free_ast: libera recursivamente toda a memória da AST
 * ==================================================== */
void free_ast(ASTNode *node) {
    if (!node) return;

    /* Libera o próximo na lista encadeada */
    free_ast(node->next);

    switch (node->kind) {
    case AST_NUM:
    case AST_FLOAT:
    case AST_CHAR:
    case AST_BOOL:
        /* Nenhuma alocação interna */
        break;

    case AST_ID:
        free(node->data.id.name);
        break;

    case AST_BINOP:
        free_ast(node->data.binop.left);
        free_ast(node->data.binop.right);
        break;

    case AST_UNARYOP:
        free_ast(node->data.unaryop.operand);
        break;

    case AST_ASSIGN:
        free(node->data.assign.name);
        free_ast(node->data.assign.expr);
        break;

    case AST_DECL:
        free(node->data.decl.name);
        free_ast(node->data.decl.init);
        break;

    case AST_EXPR_STMT:
        free_ast(node->data.expr_stmt.expr);
        break;

    case AST_IF:
        free_ast(node->data.if_stmt.cond);
        free_ast(node->data.if_stmt.then_branch);
        free_ast(node->data.if_stmt.else_branch);
        break;

    case AST_WHILE:
        free_ast(node->data.while_stmt.cond);
        free_ast(node->data.while_stmt.body);
        break;

    case AST_FOR:
        free_ast(node->data.for_stmt.init);
        free_ast(node->data.for_stmt.cond);
        free_ast(node->data.for_stmt.step);
        free_ast(node->data.for_stmt.body);
        break;

    case AST_BLOCK:
        free_ast(node->data.block.stmts);
        break;
    }

    free(node);
}
