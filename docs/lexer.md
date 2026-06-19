# Analisador Léxico (Scanner / Flex)

O analisador léxico é o primeiro estágio do nosso front-end. Ele lê o código-fonte C caractere por caractere e agrupa sequências de caracteres (lexemas) em unidades significativas denominadas **tokens**, descartando elementos insignificantes como espaços em branco.

O analisador léxico é gerado automaticamente pelo **Flex** a partir das regras definidas no arquivo [scanner.l](../src/scanner.l).

---

## 1. O que CONSEGUIMOS Fazer (Funcionalidades Implementadas)

Nosso analisador léxico está totalmente funcional e suporta os seguintes elementos da linguagem de entrada:

### 1.1 Palavras-chave de Tipo
* `int` (retorna `T_INT`)
* `float` (retorna `T_FLOAT`)
* `char` (retorna `T_CHAR`)
* `bool` (retorna `T_BOOL`) — Tipo estendido adicionado para modernizar o compilador.

### 1.2 Palavras-chave de Controle de Fluxo
* `if` (retorna `KW_IF`)
* `else` (retorna `KW_ELSE`)
* `while` (retorna `KW_WHILE`)
* `for` (retorna `KW_FOR`)

### 1.3 Literais e Valores Especiais
* **Literais Inteiros:** Cadeias de dígitos numéricos (ex: `123`), capturados pela regex `[0-9]+` e convertidos via `atoi()`. Retorna o valor numérico em `yylval.intValue`.
* **Literais de Ponto Flutuante:** Números decimais (ex: `3.14`, `.5`, `2.`), capturados pela regex `[0-9]+\.[0-9]*|\.[0-9]+` e convertidos via `atof()`. Retorna o valor em `yylval.floatValue`.
* **Literais Booleans:** Reconhece `true` (retorna `TRUE_LIT` com `yylval.intValue = 1`) e `false` (retorna `FALSE_LIT` com `yylval.intValue = 0`).
* **Literais de Caractere (Simples):** Caractere único delimitado por aspas simples (ex: `'A'`). A regex `'[^\\']'` captura o caractere na posição `yytext[1]` e retorna em `yylval.charValue`.
* **Sequências de Escape em Caracteres:** Trata escapes clássicos da linguagem C através da regra `'\\.'` e mapeamento interno via `switch-case`:
  - `\n` (quebra de linha)
  - `\t` (tabulação)
  - `\r` (retorno de carro)
  - `\0` (caractere nulo)
  - `\\` (barra invertida)
  - `\'` (aspas simples)
  - Outros padrões como `\a`, `\b`, `\f`, `\v`, `\"`, `\?`.
  - Retorna o caractere decodificado em `yylval.charValue` sob o token `CHAR_LIT`.

### 1.4 Identificadores
* Nomes de variáveis iniciados por letra ou sublinhado, seguidos de letras, dígitos ou sublinhados (ex: `idade`, `_contador_1`). Capturado por `[a-zA-Z_][a-zA-Z0-9_]*`.
* Realiza a duplicação dinâmica do lexema usando `strdup(yytext)` e passa para o Bison através de `yylval.strValue`.

### 1.5 Operadores e Delimitadores
* **Aritméticos:** `+` (`PLUS`), `-` (`MINUS`), `*` (`TIMES`), `/` (`DIVIDE`).
* **Atribuição:** `=` (`ASSIGN`).
* **Relacionais:** `==` (`EQ`), `!=` (`NE`), `<` (`LT`), `>` (`GT`), `<=` (`LE`), `>=` (`GE`).
* **Lógicos:** `&&` (`AND`), `||` (`OR`), `!` (`NOT`).
* **Pontuação/Estruturais:** `(` (`LPAREN`), `)` (`RPAREN`), `{` (`LBRACE`), `}` (`RBRACE`), `;` (`SEMICOLON`).

---

## 2. O que NÃO CONSEGUIMOS Fazer (Limitações do Lexer)

Para simplificar o escopo da disciplina e focar no motor do compilador, o analisador léxico **não reconhece nem processa** os seguintes recursos:

* **Tipos de Dados Adicionais:** Não há reconhecimento de keywords como `double`, `long`, `short`, `signed`, `unsigned` ou modificadores como `const`, `static`, `volatile`.
* **Estruturas e Uniões:** Palavras-chave como `struct`, `union`, `enum` e `typedef` não são tokenizadas.
* **Comandos de Salto e Desvio:** Palavras-chave `break`, `continue`, `return`, `switch`, `case`, `default`, `do` não são mapeadas em tokens.
* **Literais de Cadeias de Caracteres (Strings):** Não há suporte para strings entre aspas duplas (ex: `"texto"`). Apenas literais de caractere único (`'c'`) são analisados.
* **Comentários:** O analisador léxico atual não possui regras para filtrar ou ignorar comentários de linha (`//`) ou de bloco (`/* ... */`).
* **Diretivas do Pré-processador:** Diretivas iniciadas com `#` (ex: `#include`, `#define`) não são suportadas.
* **Operadores Avançados:** Operadores de ponteiro (`&`, `*`), atribuição composta (`+=`, `-=`, `*=`, `/=`), incremento/decremento (`++`, `--`), deslocamento (`<<`, `>>`) ou acesso a membros (`.`, `->`) não são reconhecidos.
* **Erros Léxicos:** Caracteres que não casam com nenhuma das regras (ex: `@`, `$`, `?`) são reportados na saída padrão (`printf("Caractere não reconhecido...")`), mas não interrompem o analisador de imediato, repassando um token desconhecido.

---

## 3. Interface Léxico-Sintática (Flex para Bison)

A comunicação do Lexer com o Parser é feita através da variável global `yylval`. A união semântica (`%union` do Bison) define quais tipos de valores podem ser enviados:

```c
%union {
    int    intValue;    /* Usado por NUM, TRUE_LIT, FALSE_LIT */
    double floatValue;  /* Usado por FLOAT_LIT */
    char   charValue;   /* Usado por CHAR_LIT */
    char  *strValue;    /* Usado por ID */
    struct ASTNode *node;
}
```

Cada correspondência léxica de literal ou identificador preenche a variável apropriada no membro `yylval` antes de dar o `return` para o Bison. Por exemplo, ao encontrar um identificador:
```lex
[a-zA-Z_][a-zA-Z0-9_]*  {
    yylval.strValue = strdup(yytext);
    return ID;
}
```
O Bison consome o token `ID` e tem acesso imediato à string alocada por `yylval.strValue`. A memória desta string deve ser desalocada posteriormente.
