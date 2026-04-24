# Explicação do Projeto do Compilador

Este documento detalha o propósito e o funcionamento interno de cada arquivo e seus respectivos blocos de código do seu projeto de compilador (baseado em C, utilizando Flex e Bison).

---

## 1. `Makefile`
**Propósito:** Arquivo de configuração lido pela ferramenta `make`. Ele automatiza todo o processo de compilação, traduzindo as gramáticas do Bison e as regras do Flex para código C, e depois compilando tudo em executáveis.

* **Configuração de Variáveis:** Define os caminhos de diretórios (`build`), os nomes dos arquivos fonte (`parser.y`, `scanner.l`, `lexer/lexer.l`), os arquivos gerados, as flags do compilador (`CC = gcc`, `CFLAGS`) e as bibliotecas necessárias para o Flex (`LDFLAGS = -lfl`).
```makefile
# Nome dos executáveis e diretórios
EXEC       = $(BUILD_DIR)/parser_exe
LEXER_EXEC = $(BUILD_DIR)/lexer_exe
BUILD_DIR  = build

# Parâmetros de compilação
CC      = gcc
CFLAGS  = -I. -Isymbol_table
LDFLAGS = -lfl     # biblioteca do Flex (em algumas distros, pode ser -ll)
```
* **Regras Padrão e de Diretório:** A regra `all` define que, por padrão, o compilador e o lexer standalone devem ser construídos. A regra `$(BUILD_DIR)` garante que a pasta `build` seja criada antes da compilação.
* **Lexer Standalone:** Regras para compilar `lexer/lexer.l`. Chama o `flex` para gerar `lexer.yy.c` e depois usa o `gcc` para gerar o executável `lexer_exe`.
* **Parser Principal:** Define as regras para gerar o interpretador/compilador (`parser_exe`). Primeiro, roda o `bison` em `parser.y` (gerando `.tab.c` e `.tab.h`). Em seguida, roda o `flex` em `scanner.l`. Por fim, compila o código gerado pelo Bison, o código gerado pelo Flex e a tabela de símbolos (`symtab.c`) em um único binário.
```makefile
$(EXEC): $(BISON_C) $(FLEX_C) symbol_table/symtab.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(BISON_C) $(FLEX_C) symbol_table/symtab.c $(LDFLAGS)

$(BISON_C) $(BISON_H): $(BISON_FILE) | $(BUILD_DIR)
	bison $(BISON_FLAGS) --defines=$(BISON_H) -o $(BISON_C) $(BISON_FILE)

$(FLEX_C): $(FLEX_FILE) $(BISON_H) | $(BUILD_DIR)
	flex $(FLEX_FLAGS) -o $(FLEX_C) $(FLEX_FILE)
```
* **Regras de Teste e Limpeza:** Inclui uma regra (`lexer_test`) para compilar uma versão de teste do lexer e uma regra `clean` para excluir rapidamente todos os binários e códigos C gerados (`rm -f`).

---

## 2. `parser.y`
**Propósito:** Este é o coração do parser (Analisador Sintático). Escrito usando a sintaxe do Bison, ele define as regras gramaticais da linguagem, a precedência de operadores e o comportamento (ações semânticas) quando uma expressão é reconhecida.

* **Prólogo C (`%{ ... %}`):** Código C que é copiado diretamente para o topo do arquivo C gerado pelo Bison. Inclui os cabeçalhos (como `symtab.h`), funções utilitárias para conversão e promoção de tipos (`promote_type`, `to_sym_value`), e implementa uma pilha de execução condicional (`g_exec`, `g_exec_stack`) para controlar blocos de `if/else`.
* **Definições Bison (`%union`, `%token`, `%type`, `%left`):** 
    * `%union`: Define os tipos de dados que os tokens podem carregar (inteiros, decimais, strings ou valores de expressão avaliados).
    * `%token`: Declara todos os terminais (palavras-chave, operadores, literais).
    * `%left` e `%right`: Resolve ambiguidades ditando a precedência e associatividade matemática (ex: `*` tem maior precedência que `+`).
```yacc
%union {
    int    intValue;    /* NUM, literais booleanos, type_spec */
    double floatValue;  /* FLOAT_LIT */
    char   charValue;   /* CHAR_LIT */
    char  *strValue;    /* identificadores */
    struct {
        double val;     /* valor numérico (double para uniformidade) */
        int    type;    /* SymType do resultado */
    } exprVal;          /* expressões tipadas */
}

%token <intValue>   NUM
%token <strValue>   ID
%type <exprVal>     expr
```
* **Regras de Fluxo e Declaração:** Define que um `program` é uma lista de linhas (`line_list`). Descreve como tratar a declaração de variáveis (`type_spec ID`), a atribuição e blocos condicionais. Durante a análise, se o fluxo do `if` permitir (`g_exec` verdadeiro), os valores são avaliados, inseridos na tabela de símbolos e impressos.
* **Regras de Expressão (`expr`):** Contém a lógica de expressões matemáticas (`+`, `-`, `*`, `/`), lógicas (`AND`, `OR`, `NOT`) e relacionais (`==`, `<`). Aqui ocorre o comportamento de um *interpretador*: em vez de apenas montar uma árvore (AST), o código no Bison já executa os cálculos aritméticos imediatamente baseados na promoção de tipos (inteiros virando floats, se necessário).
```yacc
expr PLUS expr    {
    $$.type = promote_type($1.type, $3.type);
    if ($$.type == TYPE_FLOAT)
        $$.val = $1.val + $3.val;
    else
        $$.val = (double)((int)$1.val + (int)$3.val);
}
```
* **Epílogo C (`%% ...`):** Contém a função `main()` do projeto. Ela chama `yyparse()` para iniciar o parsing do texto de entrada. Ao terminar, ela imprime o estado final da tabela de símbolos e libera a memória alocada. Também implementa `yyerror` para reportar falhas sintáticas.

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

## 4. `lexer/lexer.l`
**Propósito:** Este arquivo é apenas um **modelo/mockup** para testes. Durante o desenvolvimento, foi necessário rodar e testar o analisador léxico de forma isolada, antes do parser estar pronto. Por isso, este arquivo foi criado de forma "standalone", utilizando uma estrutura própria de `enum` para definir os tokens no lugar de receber as definições reais do Bison.

!!! note "Atenção"
    O analisador léxico oficial e funcional que o projeto compila e utiliza de verdade é o **`scanner.l`**, pois ele foi implementado em conjunto com a análise sintática do **`parser.y`**. O Makefile inclui `lexer.l` apenas para permitir sua execução autônoma (rodar o lexer sozinho).

* **Prólogo C:** Declara a variável global `line_number` para rastrear as linhas e gerar boas mensagens de erro. Como foi feito para rodar sem o Bison, ele mesmo define o bloco `enum` temporário com todos os tokens possíveis (`KW_INT`, `OP_PLUS`, `LBRACE`, etc.).
```c
enum {
    KW_INT = 256, KW_FLOAT, KW_CHAR, KW_LONG,
    OP_PLUS, OP_MINUS, OP_MULT, OP_DIV,
    IDENTIFIER, LBRACE, RBRACE
    // ...
};
```
* **Regras de Comentários:** Identifica e descarta os blocos de comentários de linha única (`//`) e de múltiplas linhas (`/* ... */`). Para comentários multi-linhas, ele tem o cuidado de contar as quebras de linha internas para não dessincronizar o contador `line_number`.
```lex
"/*"([^*]|\*+[^*/])*\*+"/"     {
    for (int i = 0; yytext[i]; i++) {
        if (yytext[i] == '\n') line_number++;
    }
}
```
* **Palavras-Chave de C:** Detecta os tipos primitivos avançados (long, double, unsigned) e controle de fluxo complexo (switch, do, break).
* **Operadores, Delimitadores e Literais:** Regex similares, porém ampliadas para suportar strings entre aspas duplas e floats em notação científica (ex: `1.2e-4`). Preocupa-se com o "Maximal Munch" (analisar o maior tamanho possível): verifica operadores duplos (como `==`) antes dos simples (como `=`).
* **Tratamento de Nova Linha e Erros:** Incrementa o `line_number` sempre que a regex encontra um `\n`. Se o código fornecer um caractere inválido, exibe uma notificação de erro informando a linha precisa.

---

## 5. `symbol_table/symtab.h`
**Propósito:** Define a estrutura e as assinaturas de funções para a **Tabela de Símbolos**, essencial para qualquer compilador rastrear variáveis, seus tipos e seus valores em memória.

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
**Propósito:** Definem a **Árvore Sintática Abstrata (AST)**. Essa estrutura permite a separação entre a etapa de análise sintática (parsing) e a etapa de execução (avaliação). O parser constrói a árvore e, após sua conclusão, o compilador a percorre para executar o programa.

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

### O Avaliador da Árvore (`eval_ast` em `ast.c`)
A execução do programa é realizada através da função `eval_ast`. Essa função recursiva avalia a árvore nó por nó executando a ação correspondente ao `NodeType`. No exemplo de um laço de repetição `AST_WHILE`, a função avalia a condição do nó filho e executa a ramificação do corpo recursivamente enquanto a condição for verdadeira:
```c
    case AST_WHILE: {
        while (1) {
            EvalResult cond = eval_ast(node->data.while_stmt.cond);
            if (cond.val == 0.0) break;
            exec_list(node->data.while_stmt.body); // Executa o corpo do laço
        }
        break;
    }
```

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
