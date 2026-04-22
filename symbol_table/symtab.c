/******************************************************
 FGA0003 - Compiladores 1
 Curso de Engenharia de Software
 Universidade de Brasília (UnB)

 Arquivo: symtab.c
 Descrição: Implementação da tabela de símbolos

 Este módulo implementa uma tabela de símbolos utilizando
 uma hash table com encadeamento externo. A tabela é
 alocada estaticamente como um array global de ponteiros
 (cada posição é a cabeça de uma lista encadeada).
 ******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

/*
 * Tabela hash global (visível apenas neste módulo).
 * Cada posição armazena a cabeça de uma lista encadeada
 * de entradas que compartilham o mesmo índice de hash.
 * Inicializada com NULL pelo C (variável estática).
 */
static SymEntry *table[SYMTAB_SIZE];

/*
 * Calcula o índice de hash para uma string usando o
 * algoritmo djb2 de Dan Bernstein:
 *   h = 5381
 *   para cada caractere c: h = h * 33 + c
 *
 * O resultado é reduzido ao intervalo [0, SYMTAB_SIZE)
 * via operação módulo.
 *
 * Parâmetro:
 *   s - string terminada em '\0'.
 *
 * Retorno:
 *   Índice no intervalo [0, SYMTAB_SIZE - 1].
 */
static unsigned int hash(const char *s) {
    unsigned int h = 5381;
    while (*s)
        h = ((h << 5) + h) + (unsigned char)*s++;
    return h % SYMTAB_SIZE;
}

/*
 * Busca uma variável pelo nome na tabela.
 * Percorre a lista encadeada no bucket correspondente
 * ao hash do nome, comparando string a string.
 *
 * Retorno: ponteiro para a entrada, ou NULL se ausente.
 */
SymEntry *sym_lookup(const char *name) {
    unsigned int idx = hash(name);  /* determina o bucket */
    SymEntry *e = table[idx];       /* início da lista encadeada */
    while (e) {
        if (strcmp(e->name, name) == 0)
            return e;               /* encontrou: retorna a entrada */
        e = e->next;                /* avança para o próximo nó */
    }
    return NULL;                    /* não encontrou */
}

/*
 * Insere ou atualiza uma variável na tabela.
 *
 * Fluxo:
 *   1. Busca a variável com sym_lookup().
 *   2. Se já existir, apenas atualiza o campo value.
 *   3. Se não existir, aloca uma nova entrada e a insere
 *      no início da lista encadeada do bucket (inserção
 *      em O(1)).
 *
 * A string 'name' é duplicada internamente com strdup(),
 * portanto o chamador pode liberar o original após a chamada.
 *
 * Encerra o programa se malloc() falhar.
 */
SymEntry *sym_set(const char *name, SymType type, int value) {
    /* Verifica se o símbolo já existe na tabela */
    SymEntry *e = sym_lookup(name);
    if (e) {
        e->value = value;   /* atualiza o valor existente */
        return e;
    }

    /* Cria uma nova entrada */
    unsigned int idx = hash(name);
    e = (SymEntry *)malloc(sizeof(SymEntry));
    if (!e) {
        fprintf(stderr, "Erro: falha ao alocar memória para símbolo '%s'\n", name);
        exit(EXIT_FAILURE);
    }
    e->name  = strdup(name);   /* cópia independente do nome */
    e->type  = type;
    e->value = value;
    e->next  = table[idx];     /* encadeia no início do bucket */
    table[idx] = e;            /* atualiza a cabeça da lista */
    return e;
}

/*
 * Imprime todas as entradas da tabela de símbolos.
 * Percorre cada bucket e, dentro dele, cada nó da lista
 * encadeada. Exibe "(vazia)" se nenhuma entrada existir.
 * Destinada exclusivamente à depuração.
 */
void sym_print(void) {
    printf("\n--- Tabela de Símbolos ---\n");
    int count = 0;
    for (int i = 0; i < SYMTAB_SIZE; i++) {
        SymEntry *e = table[i];
        while (e) {
            printf("  %s : %s = %d\n", e->name, sym_type_name(e->type), e->value);
            count++;
            e = e->next;
        }
    }
    if (count == 0)
        printf("  (vazia)\n");
    printf("--------------------------\n\n");
}

/*
 * Libera toda a memória ocupada pela tabela de símbolos.
 * Para cada bucket, percorre a lista encadeada liberando:
 *   1. A string 'name' (alocada por strdup() em sym_set()).
 *   2. A própria estrutura SymEntry (alocada por malloc()).
 * Ao final, cada bucket é reinicializado para NULL,
 * permitindo reutilização segura da tabela.
 */
void sym_free(void) {
    for (int i = 0; i < SYMTAB_SIZE; i++) {
        SymEntry *e = table[i];
        while (e) {
            SymEntry *next = e->next;  /* salva o próximo antes de liberar */
            free(e->name);
            free(e);
            e = next;
        }
        table[i] = NULL;  /* reinicializa o bucket */
    }
}

/*
 * Retorna o nome legível do tipo para exibição.
 */
const char *sym_type_name(SymType type) {
    switch (type) {
        case TYPE_INT:   return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_CHAR:  return "char";
        case TYPE_BOOL:  return "bool";
        default:         return "(sem tipo)";
    }
}
