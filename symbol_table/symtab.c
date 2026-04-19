/******************************************************
FGA0003 - Compiladores 1
Curso de Engenharia de Software
Universidade de Brasília (UnB)

Arquivo: symtab.c
Descrição: Implementação da tabela de símbolos
******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

/* Tabela hash global */
static SymEntry *table[SYMTAB_SIZE];

/* Função hash djb2 (Dan Bernstein) */
static unsigned int hash(const char *s) {
    unsigned int h = 5381;
    while (*s)
        h = ((h << 5) + h) + (unsigned char)*s++;
    return h % SYMTAB_SIZE;
}

SymEntry *sym_lookup(const char *name) {
    unsigned int idx = hash(name);
    SymEntry *e = table[idx];
    while (e) {
        if (strcmp(e->name, name) == 0)
            return e;
        e = e->next;
    }
    return NULL;
}

SymEntry *sym_set(const char *name, int value) {
    /* Se já existe, atualiza o valor */
    SymEntry *e = sym_lookup(name);
    if (e) {
        e->value = value;
        return e;
    }

    /* Caso contrário, cria uma nova entrada */
    unsigned int idx = hash(name);
    e = (SymEntry *)malloc(sizeof(SymEntry));
    if (!e) {
        fprintf(stderr, "Erro: falha ao alocar memória para símbolo '%s'\n", name);
        exit(EXIT_FAILURE);
    }
    e->name  = strdup(name);
    e->value = value;
    e->next  = table[idx];
    table[idx] = e;
    return e;
}

void sym_print(void) {
    printf("\n--- Tabela de Símbolos ---\n");
    int count = 0;
    for (int i = 0; i < SYMTAB_SIZE; i++) {
        SymEntry *e = table[i];
        while (e) {
            printf("  %s = %d\n", e->name, e->value);
            count++;
            e = e->next;
        }
    }
    if (count == 0)
        printf("  (vazia)\n");
    printf("--------------------------\n\n");
}

void sym_free(void) {
    for (int i = 0; i < SYMTAB_SIZE; i++) {
        SymEntry *e = table[i];
        while (e) {
            SymEntry *next = e->next;
            free(e->name);
            free(e);
            e = next;
        }
        table[i] = NULL;
    }
}
