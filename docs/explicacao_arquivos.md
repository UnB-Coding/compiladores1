# Explicação do Projeto do Interpretador

Este documento detalha o propósito e o funcionamento interno de cada arquivo e seus respectivos blocos de código do seu projeto de interpretador (baseado em C, utilizando Flex e Bison).

---

## 1. `Makefile`
**Propósito:** Arquivo de configuração lido pela ferramenta `make`. Ele automatiza todo o processo de compilação, traduzindo a gramática do Bison e as regras do Flex para código C, e depois compilando tudo em **um único executável** (`parser_exe`, o interpretador).

* **Configuração de Variáveis:** Define o diretório de saída (`build`), os arquivos-fonte (`src/parser.y`, `src/scanner.l`), os arquivos gerados e as flags do compilador (`CC = gcc`, `CFLAGS = -I. -Isrc -Isymbol_table`). Note que `LDFLAGS` é **vazio**: como `parser.y` fornece `main()` e `scanner.l` fornece `yywrap()`, **não** é necessário linkar a biblioteca do Flex (`-lfl`).
```makefile
# Nome do executável e diretório
EXEC       = $(BUILD_DIR)/parser_exe
BUILD_DIR  = build

# Parâmetros de compilação
CC      = gcc
CFLAGS  = -I. -Isrc -Isymbol_table
LDFLAGS =          # sem -lfl: parser.y tem main(), scanner.l tem yywrap()
```
* **Regras Padrão e de Diretório:** A regra `all` constrói apenas o interpretador (`parser_exe`). A regra `$(BUILD_DIR)` garante que a pasta `build` exista antes da compilação.
* **Parser Principal:** Primeiro roda o `bison` em `parser.y` (gerando `.tab.c` e `.tab.h`); depois roda o `flex` em `scanner.l` (que depende do `.tab.h`). Por fim, compila o código gerado pelo Bison e pelo Flex junto com a tabela de símbolos (`symtab.c`), a AST (`ast.c`), a análise semântica (`semantic.c`) e a geração/execução de IR (`ir.c`) em um único binário.
```makefile
$(EXEC): $(BISON_C) $(FLEX_C) symbol_table/symtab.c src/ast.c src/semantic.c src/ir.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(BISON_C) $(FLEX_C) symbol_table/symtab.c src/ast.c src/semantic.c src/ir.c $(LDFLAGS)

$(BISON_C) $(BISON_H): $(BISON_FILE) | $(BUILD_DIR)
	bison $(BISON_FLAGS) --defines=$(BISON_H) -o $(BISON_C) $(BISON_FILE)

$(FLEX_C): $(FLEX_FILE) $(BISON_H) | $(BUILD_DIR)
	flex $(FLEX_FLAGS) -o $(FLEX_C) $(FLEX_FILE)
```
* **Limpeza:** A regra `clean` remove rapidamente o binário e os códigos C gerados pelo Flex/Bison (`rm -f`). (Os testes em `pytest` compilam seus próprios binários de teste via `conftest.py`, fora do `Makefile`.)

---

## 2. `parser.y`
**Propósito:** Este é o coração do parser (Analisador Sintático). Escrito usando a sintaxe do Bison, ele define as regras gramaticais da linguagem, a precedência de operadores e o comportamento (ações semânticas) quando uma expressão é reconhecida.

* **Prólogo C (`%{ ... %}`):** Código C copiado para o topo do arquivo gerado pelo Bison. Inclui os cabeçalhos das fases seguintes (`ast.h`, `semantic.h`, `ir.h`, `symtab.h`) e declara a raiz global da AST (`ast_root`). **Nenhuma** lógica de avaliação ou de tabela de símbolos vive aqui — isso é responsabilidade das fases posteriores.
* **Definições Bison (`%union`, `%token`, `%type`, `%left`):**
    * `%union`: Define os tipos que os símbolos podem carregar. Crucialmente, expressões e comandos carregam um **ponteiro para nó da AST** (`struct ASTNode *node`), e não um valor já calculado.
    * `%token`: Declara todos os terminais (palavras-chave, operadores, literais).
    * `%left` e `%right`: Resolve ambiguidades ditando a precedência e associatividade matemática (ex: `*` tem maior precedência que `+`).
```yacc
%union {
    int    intValue;       /* NUM, literais booleanos, type_spec */
    double floatValue;     /* FLOAT_LIT */
    char   charValue;      /* CHAR_LIT */
    char  *strValue;       /* identificadores */
    struct ASTNode *node;  /* nós da AST (expressões e comandos) */
}

%token <intValue>   NUM
%token <strValue>   ID
%type <node>        expr
```
* **Invariante de projeto — apenas constrói AST:** As ações semânticas do parser **somente montam nós da AST**; nenhum cálculo, I/O ou acesso à tabela de símbolos ocorre durante o parsing. Essa separação rigorosa entre *analisar* e *executar* é o invariante central do projeto: ela permite inserir as fases de análise semântica, geração e otimização de IR sem tocar no parser.
* **Regras de Expressão (`expr`):** Cada produção apenas invoca um construtor de nó (`new_binop_node`, `new_unaryop_node`, `new_num_node`, …) e devolve o ponteiro resultante. A aritmética em si só acontece muito depois, em `ir_exec()`.
```yacc
expr PLUS expr    { $$ = new_binop_node('+', $1, $3); }
```
* **Epílogo C (`%% ...`):** Contém a função `main()` do projeto. Ela chama `yyparse()` (que constrói a AST) e então orquestra as fases seguintes: impressão da AST, análise semântica (`analyze_ast`), geração de IR (`gen_ir`), otimização (`ir_optimize`), execução (`ir_exec`), impressão da tabela de símbolos e liberação de memória. Também implementa `yyerror` para reportar falhas sintáticas.

---

## 3. `scanner.l`
**Propósito:** Especificação do Analisador Léxico (para o parser de expressões aritméticas), escrito para a ferramenta Flex. Sua função é ler os caracteres, reconhecer padrões e agrupar esses caracteres em `tokens` compreensíveis para o `parser.y`.

* **Prólogo C (`%{ ... %}`):** Inclui bibliotecas necessárias e o arquivo de cabeçalho `parser.tab.h` (gerado pelo Bison), que contém os IDs numéricos dos tokens (`NUM`, `PLUS`, `KW_IF`, etc.).
* **Regras Léxicas: Literais:** Expressões regulares para reconhecer números de ponto flutuante, inteiros e caracteres únicos (incluindo tratamento para caracteres de escape padrão do C, como `\n` ou `\t`). Ao reconhecer, o lexer converte a string (`yytext`) para o valor equivalente em C e o salva em `yylval`.
```lex
[0-9]+\.[0-9]*|\.[0-9]+  {
    yylval.floatValue = atof(yytext);
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
* **Espaços em Branco e Tratamento de Erros:** Ignora espaços em branco e quebras de linha de forma silenciosa. Qualquer caractere não reconhecido gera uma mensagem de erro simples.
* **Epílogo C:** Implementa `yywrap`, retornando 1 para indicar que existe apenas um arquivo de fluxo de entrada.

---

## 4. Lexer standalone (`examples/lexer.l`) — **aposentado**

!!! warning "Componente removido"
    Versões antigas do projeto incluíam um analisador léxico *standalone* (`examples/lexer.l`, com seu próprio `lexer_exe` e a máquina de testes `test_lexer.py`). Ele usava um `enum` próprio de tokens, em vez de receber as definições do Bison, e reconhecia um subconjunto maior do C (comentários, `long`, `double`, `switch`, strings, notação científica).

    Esse lexer foi **retirado**. O único analisador léxico do projeto é hoje o **`scanner.l`** (Seção 3), que opera integrado ao `parser.y`. Não reintroduza referências a `lexer.l`, `lexer_exe` ou `test_lexer.py`. Os testes léxicos vivem agora em `tests/test_scanner.py`.

---

## 5. `symbol_table/symtab.h`
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
* **Assinaturas das Funções:** Declara de forma pública as funções para buscar (`sym_lookup`), inserir/atualizar (`sym_set`), e liberar a tabela de memória (`sym_free`), criando uma "interface" para uso no parser.

---

## 6. `symbol_table/symtab.c`
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
```c
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
```
* **Função `sym_set`:** Função para guardar os dados. Primeiro chama `sym_lookup`. Se a variável já existe, altera o valor antigo. Se a variável é nova, aloca a struct `SymEntry` via `malloc()`, duplica o nome dinamicamente com `strdup()`, assinala o valor, e o insere no início (head) da lista encadeada do bucket apropriado.
* **Funções de Impressão e Liberação de Memória:** 
  * `sym_print`: Percorre todos os buckets da Hash Table apenas para imprimir uma lista formatada no terminal (útil para debugar).
  * `sym_free`: Como os elementos são gerados usando `malloc`, isso varre todos os arrays apagando recursivamente com o método `free()` o nó alocado e a cópia da string alocada por `strdup()`, evitando Memory Leaks.

---

## 7. `ast.h` e `ast.c`
**Propósito:** Definem a **Árvore Sintática Abstrata (AST)**. Essa estrutura permite a separação entre a etapa de análise sintática (parsing) e as fases posteriores. O parser constrói a árvore; depois, a análise semântica a valida e a geração de IR a traduz para TAC. O `ast.c` contém **apenas** construtores de nós, `print_ast()` e `free_ast()` — **nenhuma execução**.

### Estrutura do Nó da Árvore (`ast.h`)
O cabeçalho define um enumerador (`NodeType`) para os tipos de nós suportados, como comandos condicionais (`IF`), laços de repetição (`WHILE`), literais e operações binárias. A `struct ASTNode` implementa uma `union` para otimização de memória, armazenando os dados específicos estritamente necessários de acordo com o tipo de nó atual:
```c
typedef enum {
    AST_NUM, AST_ID, AST_BINOP, AST_IF, AST_WHILE, AST_FOR // ...
} NodeType;

typedef struct ASTNode {
    NodeType kind;              /* tipo do nó                 */
    struct ASTNode *next;       /* encadeamento para listas de comandos */

    union {
        /* AST_BINOP: left OP right */
        struct {
            int op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binop;
        
        /* Outros tipos como while, for, block... */
    } data;
} ASTNode;
```

!!! note "A AST não é mais executada diretamente"
    Versões antigas do projeto executavam o programa percorrendo a AST com um avaliador *tree-walking* (`eval_ast`, com o tipo `EvalResult`). Esse avaliador foi **aposentado**: a AST é hoje traduzida para uma representação intermediária (TAC) por `gen_ir()`, e a execução acontece exclusivamente em `ir_exec()` (ver Seções 8 e 9). Não reintroduza um segundo motor de execução sobre a AST.

### Codificação de operadores
Os operadores binários e unários são guardados no campo `int op` do nó com uma convenção mista: caracteres ASCII para `+ - * / < > ! & |`, e mnemônicos para o restante — `'E'` (`==`), `'N'` (`!=`), `'l'` (`<=`), `'g'` (`>=`). Essa mesma codificação é reaproveitada nos operandos da IR.

### Limpeza de Memória (`free_ast` em `ast.c`)
Como os nós da árvore são instanciados dinamicamente com `calloc()`, a função `free_ast` é responsável por liberar adequadamente toda a estrutura alocada. O método utiliza uma abordagem recursiva post-order (de baixo para cima), liberando os nós filhos antes de liberar o nó pai, prevenindo vazamentos de memória (memory leaks).
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

## 8. `src/semantic.h` e `src/semantic.c`
**Propósito:** Implementam a **análise semântica estática**, executada *após* o parsing e *antes* da geração de IR. A função pública `analyze_ast(ASTNode *root)` percorre a árvore inteira — inclusive ramos de `if/else` e corpos de laço que talvez nunca executem — e retorna a quantidade de erros encontrados (`0` significa programa válido).

* **Erros detectados (fatais):**
  * **Uso de variável não declarada** — referência a um identificador que nunca foi declarado.
  * **Redeclaração** — declarar duas vezes a mesma variável (há apenas o escopo global único).
  * **Divisão por zero literal** — expressões como `x / 0`, detectáveis estaticamente.
* **Avisos (não-fatais):** Conversões com perda de precisão (ex.: atribuir `float` a `int`) são reportadas em `stderr`, mas **não** contam como erro.
* **Iteração de listas:** assim como na geração de IR, o percorredor trata um único nó; a iteração sobre listas encadeadas (`->next`) fica a cargo do chamador (`analyze_list()`).

Se `analyze_ast()` retorna um valor positivo, o `main()` aborta **sem** gerar IR nem executar.

---

## 9. `src/ir.h` e `src/ir.c`
**Propósito:** Implementam a **geração, otimização e execução** do Código Intermediário (TAC). Esta é a **única via de execução** do interpretador. Uma descrição aprofundada do formato está em [Código Intermediário](codigo_intermediario.md).

* **`gen_ir(root)`** — Percorre a AST e produz um `IRProgram`: uma **lista linear de quádruplas** (`IRInstr`). Cada instrução tem um opcode (`IR_COPY`, `IR_BINOP`, `IR_IFFALSE`, `IR_GOTO`, `IR_LABEL`, `IR_DECL`, `IR_PRINT`, …), um operador opcional e até três operandos. Constantes ficam **embutidas** nos operandos e cada operando carrega seu `SymType`.
* **`ir_optimize(prog)`** — Otimiza a IR *in-place*, aplicando em **ponto fixo** quatro passes: dobramento de constantes, propagação de constantes, eliminação de temporários mortos e remoção de rótulos órfãos / `goto` redundantes.
* **`ir_exec(prog)`** — Interpreta a IR otimizada: lineariza a lista, constrói um mapa de rótulos e executa cada instrução com um ponteiro de instrução, manipulando a tabela de símbolos. É aqui que a aritmética, as conversões de tipo, a divisão por zero (fatal) e a auto-impressão de resultados realmente acontecem.
* **`ir_print(prog)`** / **`ir_free(prog)`** — Imprimem o TAC em formato legível e liberam toda a memória do programa IR, respectivamente.
