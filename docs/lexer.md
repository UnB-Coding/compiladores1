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
-

Mais detalhes da implementação devem ser documentados aqui à medida que o código evolui.
