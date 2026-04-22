# Guia Completo da Gramática — Compiladores 1 (UnB)

> Referência exaustiva da linguagem aceita pelo parser/lexer implementado em `parser.y` e `scanner.l`.

---

## 1. Visão Geral

A linguagem é um subconjunto imperativo simplificado, com semântica baseada em C. Todas as expressões são avaliadas como **inteiros** (`int`). Valores booleanos são representados internamente como `0` (falso) e `1` (verdadeiro). O programa consiste em uma sequência de **instruções** (*statements*), cada uma terminada por `;`.

---

## 2. Estrutura Léxica (Tokens)

### 2.1 Palavras Reservadas

| Palavra  | Token Bison | Descrição                            |
|----------|-------------|--------------------------------------|
| `int`    | `T_INT`     | Tipo inteiro                         |
| `float`  | `T_FLOAT`   | Tipo ponto flutuante (declaração)    |
| `char`   | `T_CHAR`    | Tipo caractere (declaração)          |
| `bool`   | `T_BOOL`    | Tipo booleano                        |
| `true`   | `TRUE_LIT`  | Literal booleano verdadeiro (= `1`)  |
| `false`  | `FALSE_LIT` | Literal booleano falso (= `0`)       |

> [!IMPORTANT]
> `true` e `false` são **literais**, não palavras-chave de tipo. Eles produzem valor semântico `intValue` (`1` e `0` respectivamente).

> [!NOTE]
> Embora `float` e `char` sejam reconhecidos como tipos válidos para declaração, **toda aritmética e armazenamento interno opera sobre `int`**. A declaração de tipo serve para fins de tipagem na tabela de símbolos, mas o valor armazenado é sempre um inteiro.

### 2.2 Literais

| Padrão Léxico | Token   | Tipo Semântico | Descrição                          |
|---------------|---------|----------------|------------------------------------|
| `[0-9]+`      | `NUM`   | `intValue`     | Número inteiro decimal (sem sinal) |
| `true`        | `TRUE_LIT`  | `intValue` (= 1) | Literal booleano verdadeiro    |
| `false`       | `FALSE_LIT` | `intValue` (= 0) | Literal booleano falso         |

> [!WARNING]
> Números de ponto flutuante (`3.14`), negativos (`-5` como literal), hexadecimais (`0xFF`), octais (`077`) e strings (`"hello"`) **não são suportados** como literais. O sinal negativo é tratado como operador unário de subtração (e nesta gramática, **não há operador unário `-`**, então `-5` resulta em `0 - 5` se precedido de uma expressão, ou erro de sintaxe caso contrário).

### 2.3 Identificadores

| Padrão Léxico              | Token | Tipo Semântico | Descrição            |
|----------------------------|-------|----------------|----------------------|
| `[a-zA-Z_][a-zA-Z0-9_]*`  | `ID`  | `strValue`     | Nome de variável     |

Regras:
- Começa com **letra** (`a-z`, `A-Z`) ou **underscore** (`_`)
- Seguido de zero ou mais letras, dígitos ou underscores
- **Case-sensitive**: `Var`, `var` e `VAR` são identificadores distintos
- Palavras reservadas (`int`, `float`, `char`, `bool`, `true`, `false`) têm prioridade sobre identificadores por aparecerem antes na definição do lexer

#### Exemplos válidos
```
x    _temp    myVar    counter1    _a_b_c    MAX_VALUE
```

#### Exemplos inválidos
```
1abc     (começa com dígito → lexado como NUM seguido de ID)
int      (palavra reservada)
my-var   (hífen não é permitido)
```

### 2.4 Operadores e Pontuação

#### Operadores Aritméticos

| Símbolo | Token    | Descrição       |
|---------|----------|-----------------|
| `+`     | `PLUS`   | Adição          |
| `-`     | `MINUS`  | Subtração       |
| `*`     | `TIMES`  | Multiplicação   |
| `/`     | `DIVIDE` | Divisão inteira |

#### Operadores Relacionais

| Símbolo | Token | Descrição       |
|---------|-------|-----------------|
| `<`     | `LT`  | Menor que       |
| `>`     | `GT`  | Maior que       |
| `<=`    | `LE`  | Menor ou igual  |
| `>=`    | `GE`  | Maior ou igual  |

#### Operadores de Igualdade

| Símbolo | Token | Descrição    |
|---------|-------|--------------|
| `==`    | `EQ`  | Igual a      |
| `!=`    | `NE`  | Diferente de |

#### Operadores Lógicos

| Símbolo | Token | Descrição      | Aridade |
|---------|-------|----------------|---------|
| `&&`    | `AND` | E lógico       | Binário |
| `\|\|`  | `OR`  | OU lógico      | Binário |
| `!`     | `NOT` | Negação lógica | Unário  |

#### Outros Símbolos

| Símbolo | Token       | Descrição                  |
|---------|-------------|----------------------------|
| `=`     | `ASSIGN`    | Atribuição                 |
| `;`     | `SEMICOLON` | Terminador de instrução    |
| `(`     | `LPAREN`    | Parêntese esquerdo         |
| `)`     | `RPAREN`    | Parêntese direito          |

### 2.5 Caracteres Ignorados

| Padrão        | Descrição                                     |
|---------------|-----------------------------------------------|
| `[ \t\n]+`    | Espaços, tabulações e quebras de linha         |

Qualquer outro caractere não reconhecido gera a mensagem:
```
Caractere não reconhecido: <char>
```

---

## 3. Gramática Formal (BNF)

```
<input>     ::= ε
              | <input> <stmt>

<stmt>      ::= <expr> ';'
              | ID '=' <expr> ';'
              | <type_spec> ID ';'
              | <type_spec> ID '=' <expr> ';'

<type_spec> ::= 'int'
              | 'float'
              | 'char'
              | 'bool'

<expr>      ::= <expr> '+' <expr>
              | <expr> '-' <expr>
              | <expr> '*' <expr>
              | <expr> '/' <expr>
              | <expr> '&&' <expr>
              | <expr> '||' <expr>
              | '!' <expr>
              | <expr> '==' <expr>
              | <expr> '!=' <expr>
              | <expr> '<' <expr>
              | <expr> '>' <expr>
              | <expr> '<=' <expr>
              | <expr> '>=' <expr>
              | '(' <expr> ')'
              | NUM
              | 'true'
              | 'false'
              | ID
```

---

## 4. Precedência e Associatividade de Operadores

A tabela abaixo lista os operadores da **menor** para a **maior** precedência:

| Nível | Operador(es)      | Associatividade | Descrição               |
|-------|--------------------|-----------------|-------------------------|
| 1     | `\|\|`             | Esquerda        | OU lógico               |
| 2     | `&&`               | Esquerda        | E lógico                |
| 3     | `==`  `!=`         | Esquerda        | Igualdade / Desigualdade|
| 4     | `<`  `>`  `<=` `>=`| Esquerda        | Comparação relacional   |
| 5     | `+`  `-`           | Esquerda        | Adição / Subtração      |
| 6     | `*`  `/`           | Esquerda        | Multiplicação / Divisão |
| 7     | `!`                | **Direita**     | Negação lógica (unário) |

> [!TIP]
> Parênteses `( )` podem ser usados para sobrescrever a precedência padrão em qualquer expressão.

### Exemplos de resolução de precedência

```c
// Entrada:
3 + 4 * 2;
// Equivale a: 3 + (4 * 2) = 11   (* tem maior precedência que +)

// Entrada:
x > 5 && x < 10;
// Equivale a: (x > 5) && (x < 10)   (&& tem menor precedência que > e <)

// Entrada:
!x || y && z;
// Equivale a: (!x) || (y && z)   (! > && > ||)

// Entrada:
1 + 2 == 3;
// Equivale a: (1 + 2) == 3 = 1   (+ tem maior precedência que ==)
```

---

## 5. Instruções (Statements)

### 5.1 Expressão Simples

```
<expr> ;
```

Avalia a expressão e imprime o resultado.

```c
42;              // → Resultado: 42
3 + 4 * 2;       // → Resultado: 11
true;            // → Resultado: 1
5 > 3;           // → Resultado: 1
```

### 5.2 Declaração de Variável (sem inicialização)

```
<type_spec> ID ;
```

Declara uma variável com o tipo especificado e valor inicial `0`.

```c
int x;           // → Declarado: x : int
bool flag;       // → Declarado: flag : bool
float f;         // → Declarado: f : float
char c;          // → Declarado: c : char
```

> [!CAUTION]
> Tentar declarar uma variável que já existe resulta em erro fatal:
> ```
> Erro: variável 'x' já declarada
> ```

### 5.3 Declaração de Variável com Inicialização

```
<type_spec> ID = <expr> ;
```

Declara e inicializa a variável. Para variáveis do tipo `bool`, o valor é normalizado: qualquer valor diferente de zero se torna `1`.

```c
int x = 100;             // → Declarado: x : int = 100
bool done = 0;           // → Declarado: done : bool = 0
bool yes = 42;           // → Declarado: yes : bool = 1   (normalizado: !!42 = 1)
int sum = 3 + 4;         // → Declarado: sum : int = 7
bool cmp = 5 > 3;        // → Declarado: cmp : bool = 1
```

### 5.4 Atribuição

```
ID = <expr> ;
```

Atribui um novo valor a uma variável **já declarada**. Para variáveis `bool`, o valor é normalizado com `!!`.

```c
int x = 10;
x = 20;                  // → int x = 20
bool flag = true;
flag = 0;                // → bool flag = 0
flag = 999;              // → bool flag = 1   (normalizado)
```

> [!CAUTION]
> Tentar atribuir a uma variável não declarada resulta em erro fatal:
> ```
> Erro: variável 'y' não declarada
> ```

---

## 6. Expressões

### 6.1 Expressões Aritméticas

| Expressão    | Semântica        | Exemplo      | Resultado |
|-------------|------------------|--------------|-----------|
| `a + b`     | Adição           | `3 + 4`      | `7`       |
| `a - b`     | Subtração        | `10 - 3`     | `7`       |
| `a * b`     | Multiplicação    | `3 * 4`      | `12`      |
| `a / b`     | Divisão inteira  | `7 / 2`      | `3`       |

> [!CAUTION]
> Divisão por zero causa erro fatal:
> ```
> Erro sintático: divisão por zero
> ```

### 6.2 Expressões Relacionais

| Expressão    | Semântica         | Exemplo      | Resultado |
|-------------|-------------------|--------------|-----------|
| `a < b`     | Menor que         | `3 < 5`      | `1`       |
| `a > b`     | Maior que         | `3 > 5`      | `0`       |
| `a <= b`    | Menor ou igual    | `5 <= 5`     | `1`       |
| `a >= b`    | Maior ou igual    | `3 >= 5`     | `0`       |

### 6.3 Expressões de Igualdade

| Expressão    | Semântica        | Exemplo      | Resultado |
|-------------|------------------|--------------|-----------|
| `a == b`    | Igualdade        | `5 == 5`     | `1`       |
| `a != b`    | Desigualdade     | `5 != 3`     | `1`       |

### 6.4 Expressões Lógicas

| Expressão    | Semântica           | Exemplo             | Resultado |
|-------------|---------------------|----------------------|-----------|
| `a && b`    | E lógico            | `1 && 0`             | `0`       |
| `a \|\| b`  | OU lógico           | `0 \|\| 1`           | `1`       |
| `!a`        | Negação (unário)    | `!0`                 | `1`       |

> [!NOTE]
> Os operadores lógicos operam com semântica C: qualquer valor diferente de zero é "verdadeiro". **Não há avaliação em curto-circuito** (short-circuit evaluation) — ambos os operandos são sempre avaliados.

### 6.5 Agrupamento

```c
(expr)
```
Parênteses alteram a ordem de avaliação:
```c
(3 + 4) * 2;     // → Resultado: 14   (sem parênteses seria 11)
```

### 6.6 Referência a Variável

Usar um `ID` em uma expressão recupera o valor armazenado na tabela de símbolos:

```c
int x = 10;
int y = 20;
x + y;            // → Resultado: 30
x > y;            // → Resultado: 0
```

> [!CAUTION]
> Usar uma variável não declarada em uma expressão causa erro fatal:
> ```
> Erro: variável 'z' não declarada
> ```

---

## 7. Sistema de Tipos

### 7.1 Tipos Disponíveis

| Tipo    | Enum Interno  | Descrição             | Armazenamento |
|---------|---------------|----------------------|---------------|
| `int`   | `TYPE_INT`    | Inteiro              | `int` (C)     |
| `float` | `TYPE_FLOAT`  | Ponto flutuante      | `int` (C) *   |
| `char`  | `TYPE_CHAR`   | Caractere            | `int` (C) *   |
| `bool`  | `TYPE_BOOL`   | Booleano             | `int` (C) **  |
| (nenhum)| `TYPE_NONE`   | Sem tipo (legado)    | `int` (C)     |

> \* `float` e `char` são reconhecidos sintaticamente e armazenados na tabela de símbolos com seu tipo, mas o valor é internamente um `int`. Não há conversão de tipo real.

> \*\* Valores `bool` são **normalizados**: na declaração com inicialização e na atribuição, aplica-se `!!valor`, garantindo que o resultado seja `0` ou `1`.

### 7.2 Regras de Normalização Booleana

Quando o tipo da variável destino é `bool`:

| Valor de Entrada | Valor Armazenado |
|-------------------|-------------------|
| `0`               | `0`               |
| `1`               | `1`               |
| `42`              | `1`               |
| `-1`              | `1`               |
| `true`            | `1`               |
| `false`           | `0`               |

---

## 8. Tabela de Símbolos

### 8.1 Estrutura

- **Implementação**: Hash table com encadeamento externo (211 buckets, número primo)
- **Algoritmo de hash**: djb2 (Dan Bernstein)
- **Cada entrada contém**: nome (`char*`), tipo (`SymType`), valor (`int`), ponteiro para próximo

### 8.2 Operações

| Função             | Descrição                                               |
|--------------------|---------------------------------------------------------|
| `sym_lookup(name)` | Busca variável; retorna `SymEntry*` ou `NULL`           |
| `sym_set(name, type, value)` | Insere ou atualiza variável                  |
| `sym_type_name(type)` | Retorna nome legível do tipo (`"int"`, `"bool"`, ...) |
| `sym_print()`      | Imprime toda a tabela (depuração)                       |
| `sym_free()`       | Libera toda memória alocada                             |

### 8.3 Comportamento ao Final da Execução

Ao terminar o `yyparse()`, a tabela de símbolos é impressa automaticamente:

```
--- Tabela de Símbolos ---
  x : int = 100
  flag : bool = 1
--------------------------
```

---

## 9. Tratamento de Erros

### 9.1 Erros Léxicos

| Situação                        | Mensagem                                |
|----------------------------------|-----------------------------------------|
| Caractere não reconhecido        | `Caractere não reconhecido: <char>`     |

Caracteres não reconhecidos são reportados mas **não interrompem** a análise; o lexer simplesmente os ignora e continua.

### 9.2 Erros Semânticos (interrompem a execução com `YYABORT`)

| Situação                                | Mensagem                                  |
|------------------------------------------|--------------------------------------------|
| Variável não declarada (em expressão)    | `Erro: variável '<nome>' não declarada`    |
| Variável não declarada (em atribuição)   | `Erro: variável '<nome>' não declarada`    |
| Variável já declarada (redeclaração)     | `Erro: variável '<nome>' já declarada`     |
| Divisão por zero                         | `Erro sintático: divisão por zero`         |

### 9.3 Erros Sintáticos

Erros de sintaxe são capturados pelo Bison com a mensagem padrão:
```
Erro sintático: syntax error
```

> [!NOTE]
> Não há mecanismo de **recuperação de erros** (`error` token). Qualquer erro semântico ou sintático **aborta** a execução imediatamente.

---

## 10. Exemplos Completos

### 10.1 Programa com Aritmética e Variáveis

```c
int x = 10;
int y = 20;
x + y;
x * y - 5;
(x + y) / 3;
```

**Saída esperada:**
```
Declarado: x : int = 10
Declarado: y : int = 20
Resultado: 30
Resultado: 195
Resultado: 10
```

### 10.2 Programa com Booleanos e Lógica

```c
bool a = true;
bool b = false;
a && b;
a || b;
!b;
int x = 42;
x > 10 && x < 100;
```

**Saída esperada:**
```
Declarado: a : bool = 1
Declarado: b : bool = 0
Resultado: 0
Resultado: 1
Resultado: 1
Declarado: x : int = 42
Resultado: 1
```

### 10.3 Programa com Reatribuição

```c
int count = 0;
count = count + 1;
count = count + 1;
count;
```

**Saída esperada:**
```
Declarado: count : int = 0
int count = 1
int count = 2
Resultado: 2
```

### 10.4 Programa com Comparações Compostas

```c
int a = 5;
int b = 10;
a == b;
a != b;
a < b;
a >= b;
a + 5 == b;
(a == 5) && (b == 10);
```

**Saída esperada:**
```
Declarado: a : int = 5
Declarado: b : int = 10
Resultado: 0
Resultado: 1
Resultado: 1
Resultado: 0
Resultado: 1
Resultado: 1
```

---

## 11. Limitações Conhecidas

| Limitação                              | Detalhes                                                    |
|----------------------------------------|-------------------------------------------------------------|
| Sem suporte a ponto flutuante real     | `float` é apenas um rótulo de tipo; valores são `int`       |
| Sem literais negativos                 | Não existe operador unário `-`; use `0 - n`                 |
| Sem strings                            | Literais de string (`"..."`) não são reconhecidos            |
| Sem estruturas de controle             | `if`, `else`, `while`, `for`, `switch` não implementados    |
| Sem funções                            | Não há declaração/chamada de funções                        |
| Sem arrays                             | Não há suporte a arrays ou ponteiros                         |
| Sem escopo                             | Todas as variáveis são globais                               |
| Sem short-circuit                      | `&&` e `||` sempre avaliam ambos operandos                  |
| Sem recuperação de erro                | Qualquer erro aborta o programa                              |
| Sem literais `char`                    | `'a'` não é reconhecido como literal de caractere            |
| Sem coerção de tipos                   | Não há conversão implícita/explícita entre tipos             |
| Sem comentários                        | `//` e `/* */` não são reconhecidos na entrada               |
| `NUM` apenas inteiros positivos        | Padrão `[0-9]+` reconhece somente inteiros sem sinal        |

---

## 12. Referência Rápida — Cheat Sheet

```
┌─────────────────────────────────────────────────────────────────┐
│ TIPOS:   int   float   char   bool                              │
│ LITERAIS: 42 (NUM)   true   false                               │
│                                                                 │
│ DECLARAÇÃO:                                                     │
│   int x;              → declara sem inicializar (valor = 0)     │
│   int x = 42;         → declara e inicializa                    │
│   bool b = true;      → declara booleano                        │
│                                                                 │
│ ATRIBUIÇÃO:                                                     │
│   x = expr;           → variável deve estar declarada           │
│                                                                 │
│ EXPRESSÃO:                                                      │
│   expr;               → avalia e imprime o resultado            │
│                                                                 │
│ OPERADORES (menor → maior precedência):                         │
│   ||  →  &&  →  == !=  →  < > <= >=  →  + -  →  * /  →  !     │
│                                                                 │
│ AGRUPAMENTO: ( expr )                                           │
│                                                                 │
│ TERMINADOR: toda instrução termina com ;                        │
└─────────────────────────────────────────────────────────────────┘
```
