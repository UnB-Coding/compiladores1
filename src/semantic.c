/******************************************************
 FGA0003 - Compiladores 1
 Curso de Engenharia de Software
 Universidade de Brasília (UnB)

 Arquivo: semantic.c
 Descrição: Implementação da Análise Semântica

 Percorre a AST antes da execução para detectar erros
 estáticos. Usa uma "tabela shadow" (lista simples de
 declarações) para rastrear quais variáveis foram
 declaradas e seus tipos, sem alterar a tabela de
 símbolos real (symtab.c).

 Diferença fundamental vs a execução (ir_exec):
   - Analisa TODOS os branches (then + else, corpo de loops)
   - NÃO executa: não calcula valores, não faz I/O
   - Detecta erros em dead code
 ******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "semantic.h"
#include "symbol_table/symtab.h"

/* ====================================================
 * Tabela shadow: lista encadeada simples de declarações
 *
 * Armazena apenas (nome, tipo) — não armazena valores,
 * pois a análise semântica não executa o programa.
 * ==================================================== */
typedef struct ShadowEntry {
    char    *name;
    SymType  type;
    struct ShadowEntry *next;
} ShadowEntry;

/* Estado global da análise */
static ShadowEntry *shadow_head = NULL;
static int error_count   = 0;
static int warning_count = 0;

/* ====================================================
 * Funções auxiliares da tabela shadow
 * ==================================================== */

/* Busca uma variável na tabela shadow */
static ShadowEntry *shadow_lookup(const char *name) {
    ShadowEntry *cur = shadow_head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

/* Insere uma variável na tabela shadow (no início da lista) */
static void shadow_insert(const char *name, SymType type) {
    ShadowEntry *e = (ShadowEntry *)malloc(sizeof(ShadowEntry));
    if (!e) {
        fprintf(stderr, "Erro: falha ao alocar entrada shadow\n");
        exit(EXIT_FAILURE);
    }
    e->name = strdup(name);
    e->type = type;
    e->next = shadow_head;
    shadow_head = e;
}

/* Libera toda a tabela shadow */
static void shadow_free(void) {
    ShadowEntry *cur = shadow_head;
    while (cur) {
        ShadowEntry *next = cur->next;
        free(cur->name);
        free(cur);
        cur = next;
    }
    shadow_head = NULL;
}

/* ====================================================
 * Funções de reporte
 * ==================================================== */

static void sem_error(const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "Erro semântico: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    error_count++;
}

static void sem_warning(const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "Aviso semântico: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    warning_count++;
}

/* ====================================================
 * Determinação de tipo resultante de uma expressão
 *
 * Retorna o tipo que uma expressão produziria, sem
 * calcular seu valor. Usado para verificar conversões.
 * ==================================================== */
static SymType infer_expr_type(ASTNode *node);

/* Promoção de tipo (mesma lógica de ast.c) */
static SymType promote_type(SymType a, SymType b) {
    if (a == TYPE_FLOAT || b == TYPE_FLOAT) return TYPE_FLOAT;
    return TYPE_INT;
}

static SymType infer_expr_type(ASTNode *node) {
    if (!node) return TYPE_INT;

    switch (node->kind) {
    case AST_NUM:   return TYPE_INT;
    case AST_FLOAT: return TYPE_FLOAT;
    case AST_CHAR:  return TYPE_CHAR;
    case AST_BOOL:  return TYPE_BOOL;

    case AST_ID: {
        ShadowEntry *e = shadow_lookup(node->data.id.name);
        if (e) return e->type;
        return TYPE_INT; /* fallback se não encontrada (erro já reportado) */
    }

    case AST_BINOP: {
        int op = node->data.binop.op;
        /* Operadores lógicos e relacionais sempre retornam int */
        if (op == '&' || op == '|' || op == 'E' || op == 'N' ||
            op == '<' || op == '>' || op == 'l' || op == 'g') {
            return TYPE_INT;
        }
        /* Operadores aritméticos: promoção */
        SymType lt = infer_expr_type(node->data.binop.left);
        SymType rt = infer_expr_type(node->data.binop.right);
        return promote_type(lt, rt);
    }

    case AST_UNARYOP:
        if (node->data.unaryop.op == '!')
            return TYPE_INT;
        return infer_expr_type(node->data.unaryop.operand);

    default:
        return TYPE_INT;
    }
}

/* ====================================================
 * Verifica se um nó é um literal zero (para detecção
 * estática de divisão por zero)
 * ==================================================== */
static int is_literal_zero(ASTNode *node) {
    if (!node) return 0;
    switch (node->kind) {
    case AST_NUM:   return node->data.num.value == 0;
    case AST_FLOAT: return node->data.flt.value == 0.0;
    case AST_BOOL:  return node->data.bln.value == 0;
    default:        return 0;
    }
}

/* ====================================================
 * Verificação de conversão implícita com perda de dados
 * ==================================================== */
static void check_implicit_conversion(SymType target, SymType source,
                                       const char *context) {
    /* float → int: perda de parte fracionária */
    if (target == TYPE_INT && source == TYPE_FLOAT) {
        sem_warning("conversão implícita de float para int em %s "
                    "(possível perda de precisão)", context);
    }
    /* float → char: perda significativa */
    if (target == TYPE_CHAR && source == TYPE_FLOAT) {
        sem_warning("conversão implícita de float para char em %s "
                    "(possível perda de precisão)", context);
    }
    /* int → char: possível truncamento */
    if (target == TYPE_CHAR && source == TYPE_INT) {
        sem_warning("conversão implícita de int para char em %s "
                    "(possível truncamento)", context);
    }
    /* qualquer numérico → bool: normalização para 0/1 (todo valor != 0 vira true) */
    if (target == TYPE_BOOL &&
        (source == TYPE_INT || source == TYPE_FLOAT || source == TYPE_CHAR)) {
        sem_warning("conversão implícita de %s para bool em %s "
                    "(normalização para 0/1)", sym_type_name(source), context);
    }
}

/* Forward declaration: analyze_list é chamada por analyze_node */
static void analyze_list(ASTNode *node);

/* ====================================================
 * analyze_node: análise semântica recursiva de um nó
 * ==================================================== */
static void analyze_node(ASTNode *node) {
    if (!node) return;

    switch (node->kind) {

    /* ---- Literais: nada a verificar ---- */
    case AST_NUM:
    case AST_FLOAT:
    case AST_CHAR:
    case AST_BOOL:
        break;

    /* ---- Identificador: verifica declaração ---- */
    case AST_ID: {
        ShadowEntry *e = shadow_lookup(node->data.id.name);
        if (!e) {
            sem_error("variável '%s' não declarada", node->data.id.name);
        }
        break;
    }

    /* ---- Operação binária ---- */
    case AST_BINOP:
        analyze_node(node->data.binop.left);
        analyze_node(node->data.binop.right);

        /* Divisão por zero com literal */
        if (node->data.binop.op == '/' &&
            is_literal_zero(node->data.binop.right)) {
            sem_error("divisão por zero");
        }
        break;

    /* ---- Operação unária ---- */
    case AST_UNARYOP:
        analyze_node(node->data.unaryop.operand);
        break;

    /* ---- Atribuição ---- */
    case AST_ASSIGN: {
        /* Analisa a expressão do lado direito primeiro */
        analyze_node(node->data.assign.expr);

        /* Verifica que a variável foi declarada */
        ShadowEntry *e = shadow_lookup(node->data.assign.name);
        if (!e) {
            sem_error("variável '%s' não declarada", node->data.assign.name);
        } else {
            /* Verifica conversão implícita */
            SymType rhs_type = infer_expr_type(node->data.assign.expr);
            char context[128];
            snprintf(context, sizeof(context), "atribuição a '%s'",
                     node->data.assign.name);
            check_implicit_conversion(e->type, rhs_type, context);
        }
        break;
    }

    /* ---- Declaração ---- */
    case AST_DECL: {
        /* Verifica redeclaração */
        ShadowEntry *e = shadow_lookup(node->data.decl.name);
        if (e) {
            sem_error("variável '%s' já declarada", node->data.decl.name);
        } else {
            /* Registra na tabela shadow */
            shadow_insert(node->data.decl.name, node->data.decl.type);
        }

        /* Analisa a expressão de inicialização, se existir */
        if (node->data.decl.init) {
            analyze_node(node->data.decl.init);

            /* Verifica conversão implícita na inicialização */
            SymType init_type = infer_expr_type(node->data.decl.init);
            char context[128];
            snprintf(context, sizeof(context),
                     "inicialização de '%s'", node->data.decl.name);
            check_implicit_conversion(node->data.decl.type, init_type,
                                       context);
        }
        break;
    }

    /* ---- Expressão como comando ---- */
    case AST_EXPR_STMT:
        analyze_node(node->data.expr_stmt.expr);
        break;

    /* ---- If / If-Else: analisa TODOS os branches ---- */
    case AST_IF:
        analyze_node(node->data.if_stmt.cond);
        /* Analisa then_branch como lista (pode ser bloco ou stmt único) */
        analyze_list(node->data.if_stmt.then_branch);
        /* Analisa else_branch se existir — mesmo que a condição seja
         * sempre true, queremos verificar erros no else */
        if (node->data.if_stmt.else_branch) {
            analyze_list(node->data.if_stmt.else_branch);
        }
        break;

    /* ---- While: analisa condição + corpo ---- */
    case AST_WHILE:
        analyze_node(node->data.while_stmt.cond);
        analyze_list(node->data.while_stmt.body);
        break;

    /* ---- For: analisa todos os componentes ---- */
    case AST_FOR:
        if (node->data.for_stmt.init)
            analyze_node(node->data.for_stmt.init);
        if (node->data.for_stmt.cond)
            analyze_node(node->data.for_stmt.cond);
        if (node->data.for_stmt.step)
            analyze_node(node->data.for_stmt.step);
        analyze_list(node->data.for_stmt.body);
        break;

    /* ---- Bloco: analisa a lista de statements ---- */
    case AST_BLOCK:
        analyze_list(node->data.block.stmts);
        break;
    }
}

/* ====================================================
 * analyze_list: percorre uma lista encadeada de nós
 * (equivalente a exec_list, mas para análise)
 * ==================================================== */
static void analyze_list(ASTNode *node) {
    while (node) {
        analyze_node(node);
        node = node->next;
    }
}

/* ====================================================
 * analyze_ast: ponto de entrada público
 * ==================================================== */
int analyze_ast(ASTNode *root) {
    /* Reinicia estado */
    shadow_free();
    error_count   = 0;
    warning_count = 0;

    /* Percorre a AST inteira */
    analyze_list(root);

    /* Libera tabela shadow */
    shadow_free();

    /* Reporta sumário */
    if (error_count > 0) {
        fprintf(stderr, "Análise semântica: %d erro(s)", error_count);
        if (warning_count > 0)
            fprintf(stderr, ", %d aviso(s)", warning_count);
        fprintf(stderr, "\n");
    } else if (warning_count > 0) {
        fprintf(stderr, "Análise semântica: %d aviso(s)\n", warning_count);
    }

    return error_count;
}
