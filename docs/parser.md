# Front-end: Analisador Sintático (Parser) e AST

O analisador sintático processa o fluxo de tokens fornecido pelo scanner, valida se o código-fonte obedece às regras gramaticais e constrói a **Árvore Sintática Abstrata (AST)**, que serve de base para o motor semântico e a execução.

A sintaxe é processada por um analisador **LALR(1)** gerado pelo **Bison** a partir do arquivo [parser.y](../src/parser.y). A estrutura da AST e as rotinas de manipulação de nós estão em [ast.h](../src/ast.h) e [ast.c](../src/ast.c).

---

## 1. O que CONSEGUIMOS Fazer (Funcionalidades Sintáticas e AST)

O analisador sintático e o interpretador AST oferecem suporte completo às seguintes regras estruturais:

### 1.1 Regras de Gramática Suportadas
* **Declaração de Variáveis:** Aceita declarações explícitas de tipos (`int`, `float`, `char`, `bool`) seguidas do identificador e finalizadas por ponto e vírgula. Aceita inicialização opcional na própria declaração (ex: `float pi = 3.14;`).
* **Atribuição Simples:** Comandos que atualizam o valor de variáveis existentes (ex: `x = 10;`).
* **Expressões Gerais:** Expressões lógicas, relacionais, aritméticas, parênteses e negação/menos unário.
* **Estruturas Condicionais:** Condicionais simples `if (cond) stmt` e compostas `if (cond) stmt else stmt`.
* **Laços de Repetição:**
  - `while (cond) stmt`
  - `for (init; cond; step) stmt` — Onde qualquer uma das partes (`init`, `cond` ou `step`) pode ser omitida (ex: `for (; i < 10 ;) ...`).
* **Blocos de Código:** Agrupamento de comandos delimitados por chaves `{ stmt_list }`.

### 1.2 Resolução de Precedência e Associatividade
Para evitar ambiguidades sem inflar as regras de produção da gramática Bison, a precedência dos operadores foi explicitada no arquivo [parser.y](../src/parser.y) por meio das diretivas `%left` e `%right`.

A ordem de avaliação definida (da menor para a maior precedência) é:
1. `OR` (`||`) — Associativo à esquerda.
2. `AND` (`&&`) — Associativo à esquerda.
3. `EQ` (`==`), `NE` (`!=`) — Associativos à esquerda.
4. `LT` (`<`), `GT` (`>`), `LE` (`<=`), `GE` (`>=`) — Associativos à esquerda.
5. `PLUS` (`+`), `MINUS` (`-`) — Associativos à esquerda.
6. `TIMES` (`*`), `DIVIDE` (`/`) — Associativos à esquerda.
7. `NOT` (`!`), `UMINUS` (menos unário) — Associativos à direita.

O clássico conflito sintático *dangling-else* foi resolvido nativamente no Bison através de uma precedência implícita de emparelhamento que favorece a ação de *shift* (associando o `else` ao `if` mais interno), o que resulta em apenas 1 conflito shift/reduce previsto (`%expect 1`).

### 1.3 Estrutura de Nós da AST
A AST separa a fase de análise da execução. O arquivo [ast.h](../src/ast.h) define **14 tipos de nós** que cobrem todo o subconjunto gramatical implementado:

| Tipo de Nó | Enumeração (`NodeType`) | Informações Armazenadas |
| :--- | :--- | :--- |
| **Inteiro** | `AST_NUM` | Valor numérico inteiro (`int value`). |
| **Float** | `AST_FLOAT` | Valor numérico real (`double value`). |
| **Char** | `AST_CHAR` | Valor do caractere simples (`char value`). |
| **Booleano** | `AST_BOOL` | Valor booleano (`int value` sendo 0 ou 1). |
| **Identificador** | `AST_ID` | String com o nome da variável (`char *name`). |
| **Op. Binária** | `AST_BINOP` | Operador (`op`) e ponteiros para os nós `left` e `right`. |
| **Op. Unária** | `AST_UNARYOP` | Operador (`op`) e ponteiro para o nó de operando (`operand`). |
| **Atribuição** | `AST_ASSIGN` | Nome do identificador (`name`) e nó com a expressão (`expr`). |
| **Declaração** | `AST_DECL` | Tipo declared (`type`), nome (`name`) e nó de inicialização (`init`). |
| **Comando Expr** | `AST_EXPR_STMT` | Nó contendo a expressão correspondente (`expr`). |
| **Condicional** | `AST_IF` | Nós para a condição (`cond`), ramo verdadeiro (`then_branch`) e falso (`else_branch`). |
| **Repetição While** | `AST_WHILE` | Nós para a condição (`cond`) e o corpo do laço (`body`). |
| **Repetição For** | `AST_FOR` | Nós para inicialização (`init`), condição (`cond`), incremento (`step`) e corpo (`body`). |
| **Bloco** | `AST_BLOCK` | Ponteiro para a cabeça da lista encadeada de comandos (`stmts`). |

* **Estrutura Encadeada:** A struct `ASTNode` possui o campo `struct ASTNode *next`, que permite encadear sequencialmente comandos no mesmo escopo (como listas de instruções de um programa ou corpo de blocos) de forma simples e direta, sem exigir nós coletores adicionais na AST.

### 1.4 Rotinas de Execução e Gerenciamento
* **Tree-Walker Evaluator:** A execução do programa é feita percorrendo a AST de maneira recursiva através da função `eval_ast(ASTNode *node)`. Expressões retornam um tipo estruturado `EvalResult` contendo o valor numérico unificado em um campo `double` e o tipo semântico correspondente (`SymType`), de modo a detectar inconsistências em tempo de execução.
* **Impressão Visual da AST:** A função `print_ast(ASTNode *node, int level)` percorre a árvore e a exibe no terminal de forma recuada, facilitando a depuração sintática estrutural de qualquer programa fonte C de entrada.
* **Liberação de Memória:** O compilador faz o gerenciamento dinâmico estrito de memória. A função `free_ast(ASTNode *node)` percorre recursivamente a árvore em pós-ordem, liberando strings duplicadas via `strdup()` em identificadores e declarações, e depois desaloca o próprio nó, eliminando vazamentos de memória.

---

## 2. O que NÃO CONSEGUIMOS Fazer (Limitações do Interpretador)

O parser e a AST do projeto atual impõem as seguintes restrições:

* **Sem Geração de Código Python (Transpilação):** O compilador funciona temporariamente como um **interpretador direto de AST**. Ele não emite código Python a partir da AST (embora este seja o objetivo do transpilador final). A execução ocorre de forma embutida em C durante a caminhada na árvore (`eval_ast()`).
* **Sem Escopos Aninhados Dinâmicos (Locais):** Embora suporte blocos com chaves (`{ ... }`), a tabela de símbolos armazena variáveis de forma global. Não há gerenciamento de escopo dinâmico ou variáveis locais reais para os blocos; redeclarar uma variável dentro de um bloco com chaves que já existe fora dele causa colisão semântica.
* **Sem Suporte a Funções e Retornos:** Não há regras gramaticais para reconhecer declaração de funções, listas de parâmetros ou instruções de retorno (`return`). O fluxo é executado sequencialmente de cima para baixo.
* **Sem Controle de Fluxo Avançado:** Comandos de controle alternativos como `break`, `continue`, `switch-case` ou `do-while` não possuem regras de derivação sintática ou nós correspondentes na AST.
* **Sem Vetores e Matrizes (Arrays):** Não há suporte para declarar ou indexar arrays (ex: `int arr[10];` ou `arr[i]`).
* **Recuperação de Erros Simples:** Diante de erros sintáticos, o Bison reporta via `yyerror()` a mensagem genérica `Erro sintático: syntax error`, mas não realiza recuperação sofisticada de pânico com descarte inteligente de tokens (o parser encerra a análise de imediato).
