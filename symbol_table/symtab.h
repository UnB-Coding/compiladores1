/******************************************************
 FGA0003 - Compiladores 1
 Curso de Engenharia de Software
 Universidade de Brasília (UnB)

 Arquivo: symtab.h
 Descrição: Tabela de símbolos (hash table com encadeamento)

 A tabela de símbolos armazena pares (nome, valor inteiro)
 para cada variável declarada no programa fonte. Utiliza uma
 hash table de tamanho fixo com encadeamento externo para
 tratar colisões.

 Utilização:
   1. Incluir este cabeçalho no módulo que precise acessar
      variáveis (#include "symtab.h").
   2. Chamar sym_set() para criar ou atualizar uma variável.
   3. Chamar sym_lookup() para consultar uma variável existente.
   4. Chamar sym_free() ao final da execução para liberar
      a memória alocada.
 ******************************************************/

#ifndef SYMTAB_H
#define SYMTAB_H

/*
 * Número de buckets da tabela hash.
 * Valor primo para distribuir melhor as chaves e
 * reduzir colisões.
 */
#define SYMTAB_SIZE 211

/*
 * Tipos suportados pelo compilador.
 * TYPE_NONE indica que a variável foi usada sem declaração
 * explícita (comportamento legado: assume inteiro).
 */
typedef enum {
    TYPE_NONE,   /* sem tipo explícito (legado) */
    TYPE_INT,    /* inteiro */
    TYPE_FLOAT,  /* ponto flutuante */
    TYPE_CHAR,   /* caractere */
    TYPE_BOOL    /* booleano */
} SymType;

/*
 * Estrutura de uma entrada na tabela de símbolos.
 * Cada entrada pertence a uma lista encadeada dentro
 * de um bucket da tabela hash.
 */
typedef struct SymEntry {
    char    *name;  /* nome da variável (alocado dinamicamente) */
    SymType  type;  /* tipo declarado da variável */
    int      value; /* valor inteiro associado à variável */
    struct SymEntry *next;  /* próxima entrada no mesmo bucket (ou NULL) */
} SymEntry;

/*
 * Busca uma variável pelo nome na tabela.
 *
 * Parâmetro:
 *   name  - nome da variável a buscar.
 *
 * Retorno:
 *   Ponteiro para a entrada correspondente, ou NULL
 *   se a variável não existir na tabela.
 *
 * Nota: não verifica tipo; apenas retorna a entrada.
 */
SymEntry *sym_lookup(const char *name);

/*
 * Insere uma nova variável ou atualiza o valor de uma
 * variável já existente na tabela.
 *
 * Parâmetros:
 *   name  - nome da variável.
 *   type  - tipo declarado (TYPE_NONE para legado).
 *   value - valor inteiro a ser atribuído.
 *
 * Retorno:
 *   Ponteiro para a entrada inserida ou atualizada.
 *
 * Nota: encerra o programa com EXIT_FAILURE em caso
 *       de falha na alocação de memória.
 */
SymEntry *sym_set(const char *name, SymType type, int value);

/*
 * Retorna o nome legível do tipo ("int", "float", etc.).
 * Destinada a mensagens de erro e depuração.
 *
 * Parâmetro:
 *   type - valor do enum SymType.
 *
 * Retorno:
 *   String constante descrevendo o tipo.
 */
const char *sym_type_name(SymType type);

/*
 * Imprime o conteúdo completo da tabela de símbolos
 * na saída padrão, incluindo o tipo declarado de cada
 * variável. Exibe "(vazia)" quando não há nenhuma
 * entrada. Destinada à depuração.
 */
void sym_print(void);

/*
 * Libera toda a memória alocada pela tabela de símbolos,
 * incluindo os nomes duplicados via strdup(). Deve ser
 * chamada ao final da execução para evitar vazamentos.
 */
void sym_free(void);

#endif /* SYMTAB_H */
