/******************************************************
FGA0003 - Compiladores 1
Curso de Engenharia de Software
Universidade de Brasília (UnB)

Arquivo: symtab.h
Descrição: Tabela de símbolos (hash table com encadeamento)
******************************************************/

#ifndef SYMTAB_H
#define SYMTAB_H

/* Número de buckets da tabela hash */
#define SYMTAB_SIZE 211

/* Entrada na tabela de símbolos */
typedef struct SymEntry {
    char *name;             /* nome da variável */
    int   value;            /* valor inteiro associado */
    struct SymEntry *next;  /* próxima entrada no mesmo bucket */
} SymEntry;

/* Busca um símbolo pelo nome.
   Retorna ponteiro para a entrada, ou NULL se não encontrado. */
SymEntry *sym_lookup(const char *name);

/* Insere ou atualiza um símbolo com o valor dado.
   Retorna ponteiro para a entrada inserida/atualizada. */
SymEntry *sym_set(const char *name, int value);

/* Imprime toda a tabela de símbolos (para depuração). */
void sym_print(void);

/* Libera toda a memória alocada pela tabela. */
void sym_free(void);

#endif /* SYMTAB_H */
