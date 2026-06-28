# Explicação do Projeto do Interpretador

Este documento detalha o propósito e o funcionamento interno de cada arquivo e seus respectivos blocos de código do seu projeto de interpretador (baseado em C, utilizando Flex e Bison).

---

## 1. `Makefile`
**Propósito:** Arquivo de configuração lido pela ferramenta `make`. Ele automatiza todo o processo de compilação, traduzindo as gramáticas do Bison e as regras do Flex para código C, e depois compilando tudo em um executável.

* **Configuração de Variáveis:** Define os caminhos de diretórios (`build`), os nomes dos arquivos fonte (`parser.y`, `scanner.l`), os arquivos gerados e as flags do compilador.
```makefile
# Nome do executável e diretórios
EXEC       = $(BUILD_DIR)/parser_exe
BUILD_DIR  = build

# Parâmetros de compilação
CC      = gcc
CFLAGS  = -I. -Isrc -Isymbol_table
```
* **Regras Padrão e de Diretório:** A regra `all` define que, por padrão, o interpretador deve ser construído. A regra `$(BUILD_DIR)` garante que a pasta `build` seja criada antes da compilação.
* **Parser Principal:** Define as regras para gerar o interpretador (`parser_exe`). Primeiro, roda o `bison` em `parser.y` (gerando `.tab.c` e `.tab.h`). Em seguida, roda o `flex` em `scanner.l`. Por fim, compila o código gerado pelo Bison, o código gerado pelo Flex, a AST, a análise semântica, a geração/execução de IR e a tabela de símbolos em um único binário.
```makefile
$(EXEC): $(BISON_C) $(FLEX_C) symbol_table/symtab.c src/ast.c src/semantic.c src/ir.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(BISON_C) $(FLEX_C) symbol_table/symtab.c src/ast.c src/semantic.c src/ir.c

$(BISON_C) $(BISON_H): $(BISON_FILE) | $(BUILD_DIR)
	bison $(BISON_FLAGS) --defines=$(BISON_H) -o $(BISON_C) $(BISON_FILE)

$(FLEX_C): $(FLEX_FILE) $(BISON_H) | $(BUILD_DIR)
	flex $(FLEX_FLAGS) -o $(FLEX_C) $(FLEX_FILE)
```
* **Regra de Limpeza:** A regra `clean` exclui rapidamente todos os binários e códigos C gerados (`rm -f`).

---

## 2. `parser.y`
**Propósito:** Este é o coração do parser (Analisador Sintático). Escrito usando a sintaxe do Bison, ele define as regras gramaticais da linguagem, a precedência de operadores e as ações semânticas que constroem a Árvore Sintática Abstrata (AST).

* **Prólogo C (`%{ ... %}`):** Código C que é copiado diretamente para o topo do arquivo C gerado pelo Bison. Inclui os cabeçalhos (`ast.h`, `semantic.h`, `ir.h`, `symtab.h`), declara a variável global `ast_root` (raiz da AST) e implementa a função auxiliar `append_node()` para encadear nós em lista.
* **Definições Bison (`%union`, `%token`, `%type`, `%left`):** 
    * `%union`: Define os tipos de dados que os tokens e não-terminais podem carregar (inteiros, doubles, caracteres, strings e ponteiros para nós AST).
    * `%token`: Declara todos os terminais (palavras-chave, operadores, literais).
    * `%left` e `%right`: Resolve ambiguidades ditando a precedência e associatividade matemática (ex: `*` tem maior precedência que `+`).
```yacc
%union {
    int    intValue;           /* NUM, literais booleanos, type_spec */
    double floatValue;         /* FLOAT_LIT */
    char   charValue;          /* CHAR_LIT */
    char  *strValue;           /* identificadores */
    struct ASTNode *node;      /* nós da AST */
}

%token <intValue>   NUM
%token <strValue>   ID
%type <node>        expr stmt line line_list stmt_list
```
* **Regras de Gramática:** Define que um `program` é uma lista de linhas (`line_list`). Descreve como tratar a declaração de variáveis (`type_spec ID`), a atribuição, controle de fluxo (`if`, `while`, `for`) e blocos. As ações semânticas **apenas constroem nós da AST** — nenhum cálculo é executado durante o parsing.
```yacc
| type_spec ID ASSIGN expr SEMICOLON
    { $$ = new_decl_node((SymType)$1, $2, $4); free($2); }

| KW_IF LPAREN expr RPAREN stmt KW_ELSE stmt
    { $$ = new_if_node($3, $5, $7); }
```
* **Epílogo C (`%% ...`):** Contém a função `main()` do projeto. O pipeline de execução é:
  1. Chama `yyparse()` para construir a AST.
  2. Verifica erros léxicos (se houver, aborta).
  3. Imprime a AST para depuração (`print_ast`).
  4. Executa a análise semântica (`analyze_ast`). Se houver erros, aborta.
  5. Gera o código intermediário TAC (`gen_ir`).
  6. Imprime o TAC original (`ir_print`).
  7. Otimiza o IR in-place (`ir_optimize`).
  8. Imprime o IR otimizado.
  9. Executa o programa via IR (`ir_exec`).
  10. Imprime a tabela de símbolos final e libera toda a memória.

---

## 3. `scanner.l`
**Propósito:** Especificação do Analisador Léxico, escrito para a ferramenta Flex. Sua função é ler os caracteres, reconhecer padrões e agrupar esses caracteres em `tokens` compreensíveis para o `parser.y`.

* **Prólogo C (`%{ ... %}`):** Inclui bibliotecas necessárias e o arquivo de cabeçalho `parser.tab.h` (gerado pelo Bison), que contém os IDs numéricos dos tokens (`NUM`, `PLUS`, `KW_IF`, etc.). Declara a variável global `lexical_errors` para contar caracteres não reconhecidos.
* **Regras Léxicas: Literais:** Expressões regulares para reconhecer números de ponto flutuante, inteiros e caracteres únicos (incluindo tratamento para caracteres de escape padrão do C, como `\n` ou `\t`). Ao reconhecer, o lexer converte a string (`yytext`) para o valor equivalente em C via `strtol()` (inteiros) ou `strtod()` (floats) e o salva em `yylval`.
```lex
[0-9]+\.[0-9]*|\.[0-9]+  {
    errno = 0;
    double v = strtod(yytext, NULL);
    if (errno == ERANGE) {
        fprintf(stderr, "Aviso: literal de ponto flutuante fora de faixa: %s\n", yytext);
    }
    yylval.floatValue = v;
    return FLOAT_LIT;
}
```
* **Regras Léxicas: Operadores e Pontuação:** Reconhece símbolos isolados (`+`, `-`, `;`, etc.) e símbolos compostos (`==`, `>=`, `&&`). Retorna o token exato correspondente para o parser.
* **Regras Léxicas: Palavras-chave e Variáveis:** Lê termos como `int`, `if`, `true`, além de nomes de variáveis (identificadores definidos por `[a-zA-Z_][a-zA-Z0-9_]*`). Para identificadores, o lexer clona a string (usando `strdup`) para passar o nome da variável ao parser.
```lex
[a-zA-Z_][a-zA-Z0-9_]*  {
    yylval.strValue = strdup(yytext);
    return ID;
}
```
* **Espaços em Branco e Tratamento de Erros:** Ignora espaços em branco e quebras de linha de forma silenciosa. Qualquer caractere não reconhecido gera uma mensagem de erro em `stderr` e incrementa `lexical_errors`.
* **Epílogo C:** Implementa `yywrap`, retornando 1 para indicar que existe apenas um arquivo de fluxo de entrada.

---

## 4. `symbol_table/symtab.h`
**Propósito:** Define a estrutura e as assinaturas de funções para a **Tabela de Símbolos**, essencial para qualquer interpretador rastrear variáveis, seus tipos e seus valores em memória.

* **Definições e Enums:** Define o tamanho fixo da tabela Hash (`SYMTAB_SIZE 211`, um número primo para minimizar colisões) e o enum `SymType`, que representa os tipos internos aceitos (`INT`, `FLOAT`, `CHAR`, `BOOL`).
* **Unions e Structs:**
  * `SymValue`: Uma struct especial de C (Union) que compartilha o mesmo espaço de memória, podendo armazenar um `int`, `float` ou `char` dependendo do uso.
  * `SymEntry`: A estrutura que atua como o nó na tabela hash, armazenando o nome (`char*`), tipo, valor (`SymValue`) e um ponteiro `next` (para lidar com colisões da hash usando encadeamento simples).
```c
typedef union {
    int   iVal;   /* TYPE_INT, TYPE_BOOL */
    float fVal;   /* TYPE_FLOAT */
    char  cVal;   /* TYPE_CHAR */
} SymValue;

typedef struct SymEntry {
    char       *name;   /* nome da variável */
    SymType     type;   /* tipo declarado */
    SymValue    value;  /* valor associado */
    struct SymEntry *next;  /* próxima entrada em caso de colisão */
} SymEntry;
```
* **Assinaturas das Funções:** Declara de forma pública as funções para buscar (`sym_lookup`), inserir/atualizar (`sym_set`), imprimir (`sym_print`) e liberar a tabela de memória (`sym_free`).

---

## 5. `symbol_table/symtab.c`
**Propósito:** Implementa o motor de armazenamento das variáveis definido no `.h`, utilizando uma arquitetura de Tabela Hash com encadeamento externo.

* **Estado Global:** Inicializa `static SymEntry *table[SYMTAB_SIZE];`, o array que será as cabeças (buckets) das nossas listas encadeadas. Ele é estático, o que significa que é privado a este arquivo `.c`.
* **Função `hash`:** Implementação privada do algoritmo de Hash "djb2" de Dan Bernstein. Ele gera um número indexador único para cada string de texto reduzindo o índice usando `h % SYMTAB_SIZE`.
```c
static unsigned int hash(const char *s) {
    unsigned int h = 5381;
    while (*s)
        h = ((h << 5) + h) + (unsigned char)*s++;
    return h % SYMTAB_SIZE;
}
```
* **Função `sym_lookup`:** Dado um nome, ela gera o hash, navega até o "balde" (bucket) correto da tabela e varre a lista encadeada usando a função `strcmp`. Se encontrar a variável com aquele exato nome, ela retorna o nó; caso contrário, retorna Nulo (`NULL`).
* **Função `sym_set`:** Função para guardar os dados. Primeiro chama `sym_lookup`. Se a variável já existe, altera o valor antigo. Se a variável é nova, aloca a struct `SymEntry` via `malloc()`, duplica o nome dinamicamente com `strdup()`, assinala o valor, e o insere no início (head) da lista encadeada do bucket apropriado.
* **Funções de Impressão e Liberação de Memória:** 
  * `sym_print`: Percorre todos os buckets da Hash Table apenas para imprimir uma lista formatada no terminal (útil para debugar).
  * `sym_free`: Como os elementos são gerados usando `malloc`, isso varre todos os arrays apagando recursivamente com o método `free()` o nó alocado e a cópia da string alocada por `strdup()`, evitando Memory Leaks.

---

## 6. `ast.h` e `ast.c`
**Propósito:** Definem a **Árvore Sintática Abstrata (AST)**. Essa estrutura permite a separação entre a etapa de análise sintática (parsing) e as etapas subsequentes (análise semântica, geração de IR, otimização e execução). O parser constrói a árvore e, após sua conclusão, as demais fases a processam.

### Estrutura do Nó da Árvore (`ast.h`)
O cabeçalho define um enumerador (`NodeType`) com **14 tipos de nós** suportados, incluindo literais (`AST_NUM`, `AST_FLOAT`, `AST_CHAR`, `AST_BOOL`), identificadores (`AST_ID`), operações (`AST_BINOP`, `AST_UNARYOP`), comandos (`AST_ASSIGN`, `AST_DECL`, `AST_EXPR_STMT`), controle de fluxo (`AST_IF`, `AST_WHILE`, `AST_FOR`) e estrutural (`AST_BLOCK`). A `struct ASTNode` implementa uma `union` para otimização de memória, armazenando os dados específicos estritamente necessários de acordo com o tipo de nó atual:
```c
typedef struct ASTNode {
    NodeType kind;              /* tipo do nó                 */
    struct ASTNode *next;       /* encadeamento para listas de comandos */

    union {
        struct { int value; } num;           /* AST_NUM */
        struct { double value; } flt;        /* AST_FLOAT */
        struct { char value; } chr;          /* AST_CHAR */
        struct { int value; } bln;           /* AST_BOOL */
        struct { char *name; } id;           /* AST_ID */
        struct { int op; ASTNode *left, *right; } binop;    /* AST_BINOP */
        struct { int op; ASTNode *operand; } unaryop;       /* AST_UNARYOP */
        struct { char *name; ASTNode *expr; } assign;       /* AST_ASSIGN */
        struct { SymType type; char *name; ASTNode *init; } decl;  /* AST_DECL */
        struct { ASTNode *expr; } expr_stmt;                /* AST_EXPR_STMT */
        struct { ASTNode *cond, *then_branch, *else_branch; } if_stmt;   /* AST_IF */
        struct { ASTNode *cond, *body; } while_stmt;        /* AST_WHILE */
        struct { ASTNode *init, *cond, *step, *body; } for_stmt;  /* AST_FOR */
        struct { ASTNode *stmts; } block;                   /* AST_BLOCK */
    } data;
} ASTNode;
```

### Construtores de Nós (`ast.c`)
O arquivo `ast.c` implementa as funções construtoras que alocam e inicializam cada tipo de nó da AST (ex: `new_num_node()`, `new_binop_node()`, `new_if_node()`, etc.). Todos os nós são alocados com `calloc()`, que zera a memória e garante que ponteiros não inicializados sejam `NULL`.

### Impressão Visual da AST (`print_ast` em `ast.c`)
A função `print_ast(ASTNode *node, int level)` percorre a árvore recursivamente, imprimindo cada nó com indentação proporcional ao nível de profundidade. Ao final de cada nó, percorre a lista encadeada (`->next`) para imprimir o próximo comando no mesmo nível. Facilita a depuração visual da estrutura sintática.

### Limpeza de Memória (`free_ast` em `ast.c`)
Como os nós da árvore são instanciados dinamicamente com `calloc()`, a função `free_ast` é responsável por liberar adequadamente toda a estrutura alocada. O método utiliza uma abordagem recursiva pós-ordem (de baixo para cima), liberando os nós filhos e strings duplicadas antes de liberar o nó pai, prevenindo vazamentos de memória (memory leaks).
```c
void free_ast(ASTNode *node) {
    if (!node) return;
    free_ast(node->next); // Libera a lista encadeada

    switch (node->kind) {
    case AST_BINOP:
        free_ast(node->data.binop.left);
        free_ast(node->data.binop.right);
        break;
    // ...
    }
    free(node);
}
```

---

## 7. `semantic.h` e `semantic.c`
**Propósito:** Implementam a **Análise Semântica** da AST. Essa fase é executada **após** o parsing e **antes** da geração de IR e execução. Percorre a AST inteira — incluindo todos os branches de `if/else` e corpos de loops — para detectar erros estáticos.

### Tabela Shadow (`semantic.c`)
A análise semântica utiliza uma tabela auxiliar independente chamada "tabela shadow", implementada como lista encadeada simples de entradas `(nome, tipo)`. Essa tabela rastreia quais variáveis foram declaradas e seus tipos, sem alterar a tabela de símbolos real (`symtab.c`) que será usada na execução.

### Verificações Realizadas
* **Variável não declarada:** Uso de identificador sem declaração prévia → erro.
* **Redeclaração:** Tentativa de declarar variável com nome já existente → erro.
* **Divisão por zero:** Divisão com divisor literal zero → erro.
* **Conversão implícita:** Conversões com possível perda de precisão (ex: `float → int`, `int → char`, numérico → `bool`) → aviso (não impede execução).

### Inferência de Tipos
A função `infer_expr_type()` determina o tipo resultante de uma expressão sem calcular seu valor, utilizando a mesma lógica de promoção: float domina, operadores lógicos/relacionais retornam int.

### Interface
```c
int analyze_ast(ASTNode *root);
```
Retorna `0` se nenhum erro semântico foi encontrado, ou o número de erros. Avisos são impressos em `stderr` mas não contam como erros.

---

## 8. `ir.h` e `ir.c`
**Propósito:** Implementam a **Geração de Código Intermediário**, a **Otimização** e a **Execução** do programa. Este é o módulo mais extenso do projeto (~1100 linhas).

### Geração de IR (`gen_ir`)
Percorre a AST e produz uma **Representação Intermediária (IR)** na forma de **Código de Três Endereços (TAC)** — uma lista linear de instruções (quádruplas). Cada instrução possui um opcode, um operador opcional e até três operandos (`dest`, `arg1`, `arg2`).

Os opcodes implementados são:

| Opcode | Forma impressa | Significado |
|:---|:---|:---|
| `IR_COPY` | `x = y` | cópia/atribuição |
| `IR_BINOP` | `x = y <op> z` | operação binária |
| `IR_UNARYOP` | `x = <op>y` | operação unária |
| `IR_LABEL` | `Lk:` | define um rótulo |
| `IR_GOTO` | `goto Lk` | desvio incondicional |
| `IR_IFFALSE` | `ifFalse x goto Lk` | desvia se `x` for falso |
| `IR_DECL` | `decl x : T` | declaração de variável |
| `IR_PRINT` | `print x` | imprime resultado de `expr_stmt` |

### Otimização (`ir_optimize`)
Executa **6 passes de otimização** em loop de ponto fixo (repete enquanto houver mudanças):

1. **Constant Folding:** Resolve operações `BINOP`/`UNARYOP` com operandos constantes em tempo de compilação (ex: `t0 = 10 + 20` → `t0 = 30`).
2. **Constant Propagation:** Substitui usos de temporários que receberam constantes pelo valor constante direto.
3. **Dead Code Elimination (DCE):** Remove código após `goto` incondicional e resolve `ifFalse` com condição constante (ex: `ifFalse false goto L0` → `goto L0`).
4. **Dead Temp Elimination:** Remove definições de temporários que nunca são lidos (órfãos da propagação).
5. **Redundant Goto Elimination:** Remove `goto Lk` imediatamente seguido de `Lk:` (desvio para a instrução seguinte).
6. **Dead Label Elimination:** Remove rótulos `Lk:` que não são alvo de nenhum desvio.

### Execução (`ir_exec`)
Lineariza a lista encadeada de instruções em um array, constrói um mapa de rótulos (`label_map`) e interpreta as instruções sequencialmente com um ponteiro de instrução (`ip`). Desvios (`goto`, `ifFalse`) alteram o `ip` para o índice do rótulo alvo.

### Ambiente de Tipos
Como a geração de IR ocorre antes da execução (quando a tabela de símbolos ainda está vazia), o módulo mantém seu próprio mapa local `(nome → tipo)` chamado *type environment*, preenchido a partir das declarações à medida que são visitadas.
