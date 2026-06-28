# Tabela de Símbolos, Gramática e Sistema de Tipos

Esta seção detalha o funcionamento da **Tabela de Símbolos** e as regras que regem a semântica de **tipagem e coerção** no interpretador, implementadas nos arquivos [symtab.h](../symbol_table/symtab.h) e [symtab.c](../symbol_table/symtab.c).

---

## 1. O que CONSEGUIMOS Fazer (Tabela de Símbolos e Tipagem)

### 1.1 Estrutura de Dados da Tabela de Símbolos
Para persistir o estado das variáveis, mapear seus tipos e gerenciar leituras e escritas durante a execução, construímos do zero uma tabela hash baseada em buckets e listas encadeadas:
* **Capacidade Fixa:** A tabela de símbolos é alocada como um vetor global de ponteiros `table` com tamanho definido de **211 buckets** (número primo ideal para evitar padrões de agrupamento).
* **Algoritmo djb2:** A função de dispersão para as chaves string (identificadores) utiliza o algoritmo clássico e eficiente `djb2` de Dan Bernstein, o qual distribui as chaves homogeneamente:
  ```c
  static unsigned int hash(const char *s) {
      unsigned int h = 5381;
      while (*s)
          h = ((h << 5) + h) + (unsigned char)*s++;
      return h % SYMTAB_SIZE;
  }
  ```
* **Resolução de Colisões:** Conflitos de chaves cujo hash resulta no mesmo índice são resolvidos por meio de encadeamento externo (*chaining*) usando uma lista encadeada simples iniciada em cada bucket.
* **Inserção e Busca:** 
  - `sym_lookup(const char *name)`: Varre o bucket de hash da string em tempo $O(1)$ médio, retornando a entrada `SymEntry`.
  - `sym_set(const char *name, SymType type, SymValue value)`: Insere uma nova variável criando uma cópia do nome via `strdup()` ou atualiza os dados se a variável já existir na tabela.

### 1.2 O Sistema de Tipos
A união tipada `SymValue` e o enum `SymType` suportam quatro tipos primitivos da linguagem:
```c
typedef enum {
    TYPE_NONE,   /* sem tipo explícito (comportamento de compatibilidade legada) */
    TYPE_INT,    /* números inteiros */
    TYPE_FLOAT,  /* números de ponto flutuante */
    TYPE_CHAR,   /* caractere único */
    TYPE_BOOL    /* tipo lógico */
} SymType;

typedef union {
    int   iVal;   /* Usado por TYPE_INT e TYPE_BOOL */
    float fVal;   /* Usado por TYPE_FLOAT */
    char  cVal;   /* Usado por TYPE_CHAR */
} SymValue;
```

### 1.3 Coerções e Promoções Implícitas
Durante a execução do programa na fase de interpretação do IR (`ir_exec()`), o interpretador resolve as incompatibilidades de tipos de dados de acordo com as seguintes regras semânticas:
1. **Dominância do Float:** Se qualquer operando em uma expressão binária for do tipo `float` (`TYPE_FLOAT`), o outro operando é convertido implicitamente para ponto flutuante, e a operação gera um resultado do tipo `float`. Exemplo: `1 + 2.5` resulta no valor `3.5` (float).
2. **Promoção de Char e Bool:** Valores `char` e `bool` são promovidos automaticamente para `int` em operações aritméticas. 
   - Exemplo com Char: `'A' + 1` resulta no inteiro `66`, correspondente ao código ASCII de 'A' (65) acrescido de 1.
   - Exemplo com Bool: `true + 5` é promovido e avaliado como `1 + 5`, resultando no inteiro `6`.
3. **Truncamento na Atribuição:** A atribuição de um valor real (`float`) para um identificador declarado como inteiro (`int`) causa o truncamento automático do valor fracionário através da função `ir_to_sym_value()`. Exemplo: `int x = 3.99;` resulta no armazenamento do valor inteiro `3`.
4. **Tratamento de Booleano em Expressões:** Valores relacionais e lógicos retornam inteiro `1` para verdadeiro e `0` para falso na avaliação de comandos condicionais e iterativos.

### 1.4 Verificação Semântica Estática e Escopo
A análise semântica é implementada em [semantic.c](../src/semantic.c) e executada **antes** da geração de IR e da execução. Ela percorre a AST inteira — incluindo todos os branches de `if/else` e corpos de loops — para detectar erros estáticos:

* **Redeclaração:** O interpretador impede a criação de variáveis com o mesmo identificador. Tentativas de redeclaração (ex: `int a; int a;`) são detectadas estaticamente como erro semântico.
* **Variável não declarada:** O uso de qualquer variável em expressões ou atribuições sem uma declaração prévia é detectado estaticamente como erro semântico.
* **Divisão por zero com literais:** Expressões como `x / 0` com divisor literal zero são detectadas estaticamente.
* **Conversões com perda de precisão:** Conversões implícitas potencialmente problemáticas (ex: `float → int`, `int → char`, `int → bool`) são reportadas como **avisos** (`warnings`) em `stderr`, mas não impedem a execução.
* **Tabela Shadow:** A análise semântica utiliza uma tabela auxiliar independente (`ShadowEntry`) que armazena apenas `(nome, tipo)` das variáveis declaradas, sem alterar a tabela de símbolos real usada na execução.
* **Exibição e Desalocação:** Ao término da execução, a função `sym_print()` exibe formatadamente o valor de cada identificador ativo de acordo com seu tipo, e `sym_free()` limpa recursivamente todos os nós e nomes alocados dinamicamente na tabela.

---

## 2. O que NÃO CONSEGUIMOS Fazer (Limitações de Tipagem e Tabela)

Devido às restrições planejadas para o escopo do interpretador, o sistema possui as seguintes limitações:

* **Escopo Global Único:** A Tabela de Símbolos é estruturada em um escopo plano e único. Não há pilha de tabelas de símbolos ou suporte para tabelas locais por bloco (escopo de bloco `{ ... }`). Variáveis declaradas dentro de blocos de controle compartilham o mesmo espaço que as variáveis globais, impossibilitando sombreamento (*shadowing*) de nomes no mesmo programa.
* **Tipos de Dados Complexos:** Não há validação sintática ou semântica para declaração de vetores (arrays), ponteiros ou estruturas definidas pelo usuário (`struct`, `union`).
* **Short-circuit Evaluation:** Os operadores lógicos `&&` e `||` avaliam ambos os operandos (não implementam avaliação de curto-circuito como no C padrão).
