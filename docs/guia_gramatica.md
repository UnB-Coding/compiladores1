# Guia Completo da Gramática — Compiladores 1 (UnB)

> **FGA0003 — Compiladores 1**
> Curso de Engenharia de Software — Universidade de Brasília (UnB)
>
> Referência definitiva da linguagem reconhecida pelo compilador.
> Gerado a partir dos arquivos-fonte `scanner.l`, `parser.y` e `symbol_table/symtab.h`.

---

## Índice

1. [Visão Geral](#1-visão-geral)
2. [Estrutura de um Programa](#2-estrutura-de-um-programa)
3. [Análise Léxica (Scanner)](#3-análise-léxica-scanner)
   - 3.1 [Palavras Reservadas](#31-palavras-reservadas)
   - 3.2 [Literais](#32-literais)
   - 3.3 [Identificadores](#33-identificadores)
   - 3.4 [Operadores e Pontuação](#34-operadores-e-pontuação)
   - 3.5 [Espaços em Branco e Comentários](#35-espaços-em-branco-e-comentários)
4. [Gramática Formal (BNF)](#4-gramática-formal-bnf)
5. [Sistema de Tipos](#5-sistema-de-tipos)
   - 5.1 [Tipos Primitivos](#51-tipos-primitivos)
   - 5.2 [Promoção e Conversão Implícita](#52-promoção-e-conversão-implícita)
   - 5.3 [Armazenamento Interno (`SymValue`)](#53-armazenamento-interno-symvalue)
6. [Declaração de Variáveis](#6-declaração-de-variáveis)
7. [Atribuição](#7-atribuição)
8. [Expressões](#8-expressões)
   - 8.1 [Operadores Aritméticos](#81-operadores-aritméticos)
   - 8.2 [Operadores Relacionais](#82-operadores-relacionais)
   - 8.3 [Operadores de Igualdade](#83-operadores-de-igualdade)
   - 8.4 [Operadores Lógicos](#84-operadores-lógicos)
   - 8.5 [Menos Unário](#85-menos-unário)
   - 8.6 [Agrupamento com Parênteses](#86-agrupamento-com-parênteses)
9. [Precedência e Associatividade](#9-precedência-e-associatividade)
10. [Tabela de Símbolos](#10-tabela-de-símbolos)
11. [Tratamento de Erros](#11-tratamento-de-erros)
12. [Sequências de Escape](#12-sequências-de-escape)
13. [Exemplos Completos](#13-exemplos-completos)
14. [Limitações Conhecidas](#14-limitações-conhecidas)

---

## 1. Visão Geral

O compilador implementa um **subconjunto da linguagem C** com foco em:

- Declaração e manipulação de variáveis com **tipagem estática** (`int`, `float`, `char`, `bool`).
- Avaliação de **expressões aritméticas, relacionais e lógicas**.
- **Conversão implícita de tipos** seguindo regras inspiradas no padrão C.
- **Divisão inteira** quando ambos operandos são inteiros (semântica C-standard).
- Suporte a **literais booleanos** (`true` / `false`) como extensão.

O programa é **interpretado linha a linha**: cada declaração ou expressão é avaliada e seu resultado impresso imediatamente.

---

## 2. Estrutura de um Programa

Um programa é uma sequência de zero ou mais **instruções** (*statements*). Cada instrução deve ser terminada com ponto-e-vírgula (`;`). Não há necessidade de uma função `main()` — o código é executado sequencialmente, de cima para baixo.

```
program → line_list
line_list → ε | line_list line
line → stmt
```

### Tipos de Instrução

| Forma                              | Descrição                                 |
| ---------------------------------- | ----------------------------------------- |
| `expr ;`                           | Avalia a expressão e imprime o resultado  |
| `id = expr ;`                      | Atribui o valor da expressão à variável   |
| `type id ;`                        | Declara variável sem inicialização        |
| `type id = expr ;`                 | Declara variável com inicialização        |

---

## 3. Análise Léxica (Scanner)

O analisador léxico (`scanner.l`) reconhece os seguintes tokens:

### 3.1 Palavras Reservadas

| Palavra     | Token     | Descrição                    |
| ----------- | --------- | ---------------------------- |
| `int`       | `T_INT`   | Tipo inteiro                 |
| `float`     | `T_FLOAT` | Tipo ponto flutuante         |
| `char`      | `T_CHAR`  | Tipo caractere               |
| `bool`      | `T_BOOL`  | Tipo booleano (extensão)     |
| `true`      | `TRUE_LIT`| Literal booleano verdadeiro  |
| `false`     | `FALSE_LIT`| Literal booleano falso      |

> **Nota:** `true` e `false` são tratados como **literais** com valor semântico (`1` e `0`, respectivamente), não como identificadores.

### 3.2 Literais

#### 3.2.1 Literais Inteiros (`NUM`)

Sequência de um ou mais dígitos decimais (`0-9`).

| Exemplo | Valor |
| ------- | ----- |
| `0`     | 0     |
| `42`    | 42    |
| `1000`  | 1000  |

> Não há suporte a literais hexadecimais (`0x`), octais (`0`) ou binários (`0b`).

#### 3.2.2 Literais de Ponto Flutuante (`FLOAT_LIT`)

Números com ponto decimal. A regra utiliza **maximal munch** — a correspondência mais longa é preferida sobre inteiros.

| Formato        | Exemplos          |
| -------------- | ----------------- |
| `dígitos.dígitos` | `3.14`, `0.5`, `100.0` |
| `dígitos.`     | `3.`, `100.`      |
| `.dígitos`     | `.5`, `.001`      |

> Não há suporte a notação científica (`1e10`, `3.14e-2`).

#### 3.2.3 Literais de Caractere (`CHAR_LIT`)

Caractere único entre aspas simples, ou uma sequência de escape.

| Formato      | Exemplos                  |
| ------------ | ------------------------- |
| Simples      | `'A'`, `'z'`, `'0'`      |
| Escape       | `'\n'`, `'\t'`, `'\\'`    |

Veja a [Seção 12](#12-sequências-de-escape) para a lista completa de escapes suportados.

#### 3.2.4 Literais Booleanos (`TRUE_LIT`, `FALSE_LIT`)

| Literal | Valor interno |
| ------- | ------------- |
| `true`  | `1`           |
| `false` | `0`           |

### 3.3 Identificadores

Um identificador começa com uma letra (`a-z`, `A-Z`) ou underscore (`_`), seguido de zero ou mais letras, dígitos ou underscores.

**Regex:** `[a-zA-Z_][a-zA-Z0-9_]*`

Exemplos válidos: `x`, `contagem`, `_temp`, `valor2`, `minha_var`

> **Importante:** As palavras reservadas (`int`, `float`, `char`, `bool`, `true`, `false`) são reconhecidas **antes** da regra de identificador, portanto nunca podem ser usadas como nome de variável.

### 3.4 Operadores e Pontuação

#### Operadores Aritméticos

| Símbolo | Token    | Operação          |
| ------- | -------- | ------------------ |
| `+`     | `PLUS`   | Adição             |
| `-`     | `MINUS`  | Subtração / Negação|
| `*`     | `TIMES`  | Multiplicação      |
| `/`     | `DIVIDE` | Divisão            |

#### Operadores Relacionais

| Símbolo | Token | Operação       |
| ------- | ----- | -------------- |
| `<`     | `LT`  | Menor que      |
| `>`     | `GT`  | Maior que      |
| `<=`    | `LE`  | Menor ou igual |
| `>=`    | `GE`  | Maior ou igual |

#### Operadores de Igualdade

| Símbolo | Token | Operação       |
| ------- | ----- | -------------- |
| `==`    | `EQ`  | Igual a        |
| `!=`    | `NE`  | Diferente de   |

#### Operadores Lógicos

| Símbolo | Token | Operação      |
| ------- | ----- | ------------- |
| `&&`    | `AND` | E lógico      |
| `\|\|`  | `OR`  | Ou lógico     |
| `!`     | `NOT` | Negação lógica|

#### Pontuação

| Símbolo | Token       | Uso                            |
| ------- | ----------- | ------------------------------ |
| `=`     | `ASSIGN`    | Atribuição                     |
| `;`     | `SEMICOLON` | Terminador de instrução        |
| `(`     | `LPAREN`    | Abre agrupamento               |
| `)`     | `RPAREN`    | Fecha agrupamento              |

> **Nota:** O scanner reconhece `==` e `!=` **antes** de `=`, garantindo que a atribuição não seja confundida com igualdade.

### 3.5 Espaços em Branco e Comentários

- **Espaços, tabulações e quebras de linha** (`[ \t\n]+`) são **ignorados** silenciosamente.
- **Comentários** não são suportados na implementação atual. Caracteres `//` ou `/* */` causarão erros léxicos.
- Qualquer **caractere não reconhecido** gera a mensagem: `Caractere não reconhecido: <char>`.

---

## 4. Gramática Formal (BNF)

```bnf
<program>    ::= <line_list>

<line_list>  ::= ε
              |  <line_list> <line>

<line>       ::= <stmt>

<stmt>       ::= <expr> ";"
              |  ID "=" <expr> ";"
              |  <type_spec> ID ";"
              |  <type_spec> ID "=" <expr> ";"

<type_spec>  ::= "int" | "float" | "char" | "bool"

<expr>       ::= <expr> "+" <expr>
              |  <expr> "-" <expr>
              |  <expr> "*" <expr>
              |  <expr> "/" <expr>
              |  "-" <expr>                    /* menos unário */
              |  <expr> "&&" <expr>
              |  <expr> "||" <expr>
              |  "!" <expr>
              |  <expr> "==" <expr>
              |  <expr> "!=" <expr>
              |  <expr> "<"  <expr>
              |  <expr> ">"  <expr>
              |  <expr> "<=" <expr>
              |  <expr> ">=" <expr>
              |  "(" <expr> ")"
              |  NUM
              |  FLOAT_LIT
              |  CHAR_LIT
              |  TRUE_LIT
              |  FALSE_LIT
              |  ID
```

---

## 5. Sistema de Tipos

### 5.1 Tipos Primitivos

| Tipo    | `SymType`    | Tamanho interno | Faixa de valores           |
| ------- | ------------ | --------------- | -------------------------- |
| `int`   | `TYPE_INT`   | `int` (4 bytes) | −2³¹ a 2³¹−1              |
| `float` | `TYPE_FLOAT` | `float` (4 bytes)| ±3.4×10³⁸ (aprox.)       |
| `char`  | `TYPE_CHAR`  | `char` (1 byte) | −128 a 127 (signed)        |
| `bool`  | `TYPE_BOOL`  | `int` (4 bytes) | `0` (false) ou `1` (true)  |

Existe também `TYPE_NONE` na enumeração interna, indicando variável sem tipo explícito (comportamento legado), mas não é acessível pela gramática.

### 5.2 Promoção e Conversão Implícita

O sistema de tipos segue regras inspiradas no C padrão:

#### Promoção em Operações Aritméticas

```
Se qualquer operando é float → resultado é float (aritmética de ponto flutuante)
Caso contrário              → resultado é int    (aritmética inteira)
```

> **Atenção:** `char` e `bool` são **promovidos a `int`** em operações aritméticas. Isso significa que `'A' + 1` resulta em `int`, não `char`.

#### Divisão Inteira

Quando ambos os operandos são inteiros (incluindo `char` e `bool`):
- `5 / 2` → `2` (truncamento para zero, como em C)
- `5.0 / 2` → `2.5` (divisão de ponto flutuante por promoção)

#### Conversão na Atribuição

Ao atribuir um valor a uma variável, o valor é **convertido** para o tipo declarado da variável:

| Tipo Destino | Conversão aplicada              | Exemplo                         |
| ------------ | ------------------------------- | ------------------------------- |
| `int`        | Trunca para inteiro             | `int j = 3.7;` → `j = 3`      |
| `float`      | Converte para float             | `float f = 5;` → `f = 5`      |
| `char`       | Trunca para inteiro, depois char | `char d = 65;` → `d = 'A'`   |
| `bool`       | `0` → `false`, não-zero → `true`| `bool b = 42;` → `b = true`  |

### 5.3 Armazenamento Interno (`SymValue`)

Os valores são armazenados na tabela de símbolos usando uma **union discriminada**:

```c
typedef union {
    int   iVal;   // TYPE_INT, TYPE_BOOL
    float fVal;   // TYPE_FLOAT
    char  cVal;   // TYPE_CHAR
} SymValue;
```

O campo ativo é determinado pelo `SymType` da entrada. Internamente, todas as expressões são avaliadas usando `double` para uniformidade, e o valor é convertido para o tipo destino somente no momento da atribuição.

---

## 6. Declaração de Variáveis

### Declaração sem inicialização

```c
type id;
```

A variável é criada com o valor padrão `0` (convertido para o tipo declarado).

| Tipo    | Valor inicial |
| ------- | ------------- |
| `int`   | `0`           |
| `float` | `0.0`         |
| `char`  | `'\0'` (0)    |
| `bool`  | `false` (0)   |

**Exemplo:**
```c
int x;       // Declarado: x : int
float pi;    // Declarado: pi : float
```

### Declaração com inicialização

```c
type id = expr;
```

A expressão é avaliada e convertida para o tipo declarado.

**Exemplos:**
```c
int x = 42;           // Declarado: x : int = 42
float pi = 3.14;      // Declarado: pi : float = 3.14
char c = 'A';         // Declarado: c : char = 'A'
bool flag = true;     // Declarado: flag : bool = true
int j = 3.7;          // Declarado: j : int = 3  (truncado)
char d = 65;          // Declarado: d : char = 'A' (código ASCII)
```

### Regras

- **Declaração duplicada é proibida.** Declarar uma variável já existente causa erro fatal:
  ```
  Erro: variável 'x' já declarada
  ```
- **Variáveis devem ser declaradas antes do uso.** Usar uma variável não declarada causa erro:
  ```
  Erro: variável 'x' não declarada
  ```

---

## 7. Atribuição

```c
id = expr;
```

Atribui o resultado da expressão à variável já declarada. O valor é **convertido implicitamente** para o tipo da variável.

**Exemplos:**
```c
int x = 10;
x = 20;          // int x = 20
x = 3.7;         // int x = 3 (truncado para int)

float f = 1.0;
f = 5;            // float f = 5 (promovido para float)
```

### Regras

- A variável **deve ter sido declarada previamente** com `type id;` ou `type id = expr;`.
- Não é possível mudar o tipo de uma variável após a declaração.
- O tipo da variável determina como o valor será armazenado, independentemente do tipo da expressão.

---

## 8. Expressões

Todas as expressões produzem um par `(valor, tipo)`. O tipo do resultado depende dos operandos e do operador utilizado.

### 8.1 Operadores Aritméticos

| Operador | Sintaxe      | Tipo do Resultado                     |
| -------- | ------------ | ------------------------------------- |
| `+`      | `a + b`      | `float` se algum é float; senão `int` |
| `-`      | `a - b`      | `float` se algum é float; senão `int` |
| `*`      | `a * b`      | `float` se algum é float; senão `int` |
| `/`      | `a / b`      | `float` se algum é float; senão `int` |

**Semântica de divisão:**
- Divisão **inteira** quando ambos operandos são não-float: `7 / 2` → `3`
- Divisão de **ponto flutuante** quando pelo menos um é float: `7.0 / 2` → `3.5`
- **Divisão por zero** causa erro fatal: `Erro sintático: divisão por zero`

### 8.2 Operadores Relacionais

| Operador | Sintaxe     | Descrição       | Tipo do Resultado |
| -------- | ----------- | --------------- | ----------------- |
| `<`      | `a < b`     | Menor que       | `int` (0 ou 1)    |
| `>`      | `a > b`     | Maior que       | `int` (0 ou 1)    |
| `<=`     | `a <= b`    | Menor ou igual  | `int` (0 ou 1)    |
| `>=`     | `a >= b`    | Maior ou igual  | `int` (0 ou 1)    |

A comparação é realizada em `double`, permitindo comparações mistas entre tipos.

### 8.3 Operadores de Igualdade

| Operador | Sintaxe     | Descrição     | Tipo do Resultado |
| -------- | ----------- | ------------- | ----------------- |
| `==`     | `a == b`    | Igual a       | `int` (0 ou 1)    |
| `!=`     | `a != b`    | Diferente de  | `int` (0 ou 1)    |

**Exemplo:** `'a' == 97;` → `Resultado: 1` (pois `'a'` = 97 em ASCII)

### 8.4 Operadores Lógicos

| Operador | Sintaxe     | Descrição        | Tipo do Resultado |
| -------- | ----------- | ---------------- | ----------------- |
| `&&`     | `a && b`    | E lógico (AND)   | `int` (0 ou 1)    |
| `\|\|`   | `a \|\| b`  | Ou lógico (OR)   | `int` (0 ou 1)    |
| `!`      | `!a`        | Negação (NOT)    | `int` (0 ou 1)    |

- Operandos são avaliados como **truthy/falsy**: zero é falso, qualquer não-zero é verdadeiro.
- O resultado é sempre `int` com valor `0` ou `1`.

> **Nota:** Os operadores lógicos **não** fazem avaliação de curto-circuito (*short-circuit*) — ambos os operandos são sempre avaliados.

### 8.5 Menos Unário

```c
-expr
```

Nega o valor da expressão. O tipo do resultado é o mesmo da expressão.

**Exemplos:**
```c
-42;      // Resultado: -42         (int)
-3.14;    // Resultado: -3.14       (float)
```

### 8.6 Agrupamento com Parênteses

```c
(expr)
```

Parênteses podem ser usados para alterar a ordem de avaliação.

**Exemplo:**
```c
(2 + 3) * 4;    // Resultado: 20
2 + 3 * 4;      // Resultado: 14
```

---

## 9. Precedência e Associatividade

Da **menor** para a **maior** precedência:

| Nível | Operador(es)      | Associatividade | Descrição             |
| ----- | ----------------- | --------------- | --------------------- |
| 1     | `\|\|`            | Esquerda        | Ou lógico             |
| 2     | `&&`              | Esquerda        | E lógico              |
| 3     | `==` `!=`         | Esquerda        | Igualdade             |
| 4     | `<` `>` `<=` `>=` | Esquerda        | Relacional            |
| 5     | `+` `-`           | Esquerda        | Adição / Subtração    |
| 6     | `*` `/`           | Esquerda        | Multiplicação / Divisão |
| 7     | `!` `-` (unário)  | **Direita**     | Negação / Menos unário |

> **Exemplo de precedência:**
> ```c
> !a && b || c > 3 + 1 * 2
> ```
> É interpretado como:
> ```c
> ((!a) && b) || (c > (3 + (1 * 2)))
> ```

---

## 10. Tabela de Símbolos

A tabela de símbolos utiliza uma **hash table com encadeamento externo** (separate chaining).

### Especificações

| Propriedade            | Valor                                  |
| ---------------------- | -------------------------------------- |
| Tamanho                | 211 buckets (número primo)             |
| Algoritmo de hash      | djb2 (Dan Bernstein)                   |
| Resolução de colisões  | Listas encadeadas                      |
| Inserção               | O(1) — início da lista                 |
| Busca                  | O(n) no pior caso por bucket           |

### Estrutura de uma Entrada

```c
typedef struct SymEntry {
    char       *name;    // nome da variável (cópia via strdup)
    SymType     type;    // tipo declarado (TYPE_INT, TYPE_FLOAT, etc.)
    SymValue    value;   // valor armazenado (union discriminada)
    struct SymEntry *next;  // próximo no bucket (ou NULL)
} SymEntry;
```

### Funções Disponíveis

| Função            | Descrição                                              |
| ----------------- | ------------------------------------------------------ |
| `sym_lookup(name)` | Busca variável; retorna `SymEntry*` ou `NULL`         |
| `sym_set(name, type, value)` | Insere ou atualiza variável              |
| `sym_type_name(type)` | Retorna string legível do tipo (`"int"`, `"float"`, etc.) |
| `sym_print()`     | Imprime toda a tabela (depuração)                      |
| `sym_free()`      | Libera toda a memória alocada                          |

### Saída ao Final do Programa

Ao terminar a execução, a tabela de símbolos é automaticamente impressa:

```
--- Tabela de Símbolos ---
  x : float = 3.14
  c : char = 'A' (65)
  flag : bool = true
  i : int = 10
--------------------------
```

---

## 11. Tratamento de Erros

O compilador detecta os seguintes erros em tempo de execução:

### Erros Semânticos (Fatais — abortam a execução)

| Erro                                         | Mensagem                                        |
| -------------------------------------------- | ----------------------------------------------- |
| Uso de variável não declarada                | `Erro: variável 'x' não declarada`              |
| Declaração duplicada de variável             | `Erro: variável 'x' já declarada`               |
| Divisão por zero                             | `Erro sintático: divisão por zero`               |

### Erros Léxicos (Não fatais)

| Erro                                         | Mensagem                                        |
| -------------------------------------------- | ----------------------------------------------- |
| Caractere não reconhecido                    | `Caractere não reconhecido: <char>`              |
| Sequência de escape inválida em char literal | `Sequência de escape inválida: <seq>`            |

### Erros Sintáticos (Fatais)

| Erro                                         | Mensagem                                        |
| -------------------------------------------- | ----------------------------------------------- |
| Token inesperado / estrutura inválida        | `Erro sintático: syntax error`                   |

> Todos os erros fatais (semânticos e sintáticos) causam **`YYABORT`**, encerrando a execução do parser.

---

## 12. Sequências de Escape

As seguintes sequências de escape são reconhecidas dentro de literais de caractere:

| Sequência | Caractere             | Código ASCII |
| --------- | --------------------- | ------------ |
| `\a`      | Alerta (bell)         | 7            |
| `\b`      | Backspace             | 8            |
| `\f`      | Form feed             | 12           |
| `\n`      | Nova linha (newline)  | 10           |
| `\r`      | Retorno de carro      | 13           |
| `\t`      | Tabulação horizontal  | 9            |
| `\v`      | Tabulação vertical    | 11           |
| `\\`      | Barra invertida       | 92           |
| `\'`      | Aspas simples         | 39           |
| `\"`      | Aspas duplas          | 34           |
| `\?`      | Interrogação          | 63           |
| `\0`      | Caractere nulo        | 0            |

> Sequências de escape não listadas acima geram um aviso e usam o caractere literal após a barra.

---

## 13. Exemplos Completos

### Exemplo 1 — Tipos e operações básicas

```c
int x = 10;
float y = 3.14;
x + y;
5 / 2;
5.0 / 2;
```

**Saída esperada:**
```
Declarado: x : int = 10
Declarado: y : float = 3.14
Resultado: 13.14
Resultado: 2
Resultado: 2.5
```

### Exemplo 2 — Caracteres e ASCII

```c
char c = 'A';
c;
c + 1;
char d = 65;
'a' == 97;
```

**Saída esperada:**
```
Declarado: c : char = 'A'
Resultado: 'A'
Resultado: 66
Declarado: d : char = 'A'
Resultado: 1
```

### Exemplo 3 — Booleanos e lógica

```c
bool flag = true;
bool other = false;
flag && other;
flag || other;
!flag;
```

**Saída esperada:**
```
Declarado: flag : bool = true
Declarado: other : bool = false
Resultado: 0
Resultado: 1
Resultado: 0
```

### Exemplo 4 — Conversão implícita na atribuição

```c
int j = 3.7;
char d = 65;
float f = 5;
bool b = 42;
```

**Saída esperada:**
```
Declarado: j : int = 3
Declarado: d : char = 'A'
Declarado: f : float = 5
Declarado: b : bool = true
```

### Exemplo 5 — Sequências de escape

```c
char newline = '\n';
char tab = '\t';
char backslash = '\\';
char quote = '\'';
```

**Saída esperada:**
```
Declarado: newline : char = 10
Declarado: tab : char = 9
Declarado: backslash : char = '\' (92)
Declarado: quote : char = ''' (39)
```

### Exemplo 6 — Expressões mistas com variáveis

```c
float x = 3.14;
int i = 10;
i + x;
bool b = true;
b && (x > 2.0);
```

**Saída esperada:**
```
Declarado: x : float = 3.14
Declarado: i : int = 10
Resultado: 13.14
Declarado: b : bool = true
Resultado: 1
```

---

## 14. Limitações Conhecidas

| Limitação | Descrição |
| --------- | --------- |
| Sem controle de fluxo | Não há `if`, `else`, `while`, `for`, `switch` |
| Sem funções | Não há declaração ou chamada de funções |
| Sem arrays/ponteiros | Não há suporte a arrays, strings ou ponteiros |
| Sem strings | Não há tipo string nem literais `"..."` |
| Sem comentários | `//` e `/* */` não são reconhecidos |
| Sem escopo | Todas as variáveis são globais |
| Sem notação científica | `1e10`, `3.14e-2` não são reconhecidos |
| Sem literais octais/hex | `0xFF`, `077` não são reconhecidos |
| Sem operadores bit-a-bit | `&`, `|`, `^`, `~`, `<<`, `>>` não existem |
| Sem operadores compostos | `+=`, `-=`, `*=`, `/=` não existem |
| Sem incremento/decremento | `++`, `--` não existem |
| Sem operador ternário | `? :` não existe |
| Sem `sizeof` | O operador `sizeof` não é suportado |
| Sem `typedef` | Definição de tipos personalizados não é suportada |
| Sem `struct`/`union`/`enum` | Tipos compostos não são suportados |
| Sem pré-processador | `#include`, `#define`, `#ifdef` etc. não são reconhecidos |
| Sem curto-circuito | `&&` e `||` avaliam sempre ambos os operandos |
| Sem declaração múltipla | `int a, b;` não é suportado |
| Sem cast explícito | `(int)3.14` não é suportado |
