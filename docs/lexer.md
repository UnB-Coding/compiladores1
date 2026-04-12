# Analisador Léxico

Esta seção detalha a implementação do analisador léxico, localizado no diretório `lexer/`.

A ferramenta responsável pela geração do analisador léxico é o **Flex** (Fast Lexical Analyzer Generator). O objetivo do analisador léxico é ler o código-fonte de entrada caractere por caractere e agrupá-los em tokens (como palavras-chave, identificadores, números, operadores, etc.).

## Visão Geral

- **Arquivo fonte:** Normalmente `lexer/lexer.l` (ou similar).
- **Entrada:** Código-fonte da linguagem.
- **Saída:** Fluxo de tokens para o analisador sintático (Bison).

## Subconjunto de gramática C

- Tipos:
  - int
  - float
  - char
  - long
  - double
  - short
  - signed
  - unsigned
- Expressões de controle:
  - if
  - else
  - while
  - for
  - do while
  - switch
  - case
  - break
  - continue
  - return
- Operadores:
  - - - - /
  - =
  - ==
  - !=

## Formalização (EBNF)

Para a estruturação das regras no analisador sintático, o subconjunto acima foi mapeado na seguinte notação:

```ebnf
<tipo> ::= "int" | "float" | "char" | "long" | "double" | "short"
<modificador> ::= "signed" | "unsigned"

<comando_selecao> ::= "if" "(" <expressao> ")" <bloco> ["else" <bloco>]
                    | "switch" "(" <expressao> ")" "{" <lista_casos> "}"

<comando_repeticao> ::= "while" "(" <expressao> ")" <bloco>
                      | "do" <bloco> "while" "(" <expressao> ")" ";"
                      | "for" "(" [<expressao>] ";" [<expressao>] ";" [<expressao>] ")" <bloco>

<comando_salto> ::= "return" [<expressao>] ";" | "break" ";" | "continue" ";"
```

## Limitações e Escopo Não Implementado

Para delimitar o desenvolvimento do compilador, os seguintes recursos não serão suportados:

* **Ponteiros:** Não haverá suporte para operadores de endereço (`&`) e desreferência (`*`).
* **Estruturas Complexas:** `struct`, `union` e `enum` não fazem parte deste subconjunto.
* **Pré-processador:** Diretivas como `#include` e `#define` não serão processadas.
* **Qualificadores:** Não há suporte para `const`, `static` ou `volatile`.
* **Bibliotecas Externas:** Funções da biblioteca padrão (ex: `printf`) serão tratadas apenas como identificadores comuns.

Mais detalhes da implementação devem ser documentados aqui à medida que o código evolui.

