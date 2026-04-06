# Analisador Sintático

Esta seção detalha a implementação do analisador sintático, localizado no diretório `parser/`.

A ferramenta responsável pela geração do analisador sintático é o **Bison** (GNU parser generator). Ele recebe a sequência de tokens produzida pelo analisador léxico e tenta encontrar uma estrutura sintática de acordo com as regras da gramática livre de contexto da linguagem.

## Visão Geral

- **Arquivo fonte:** Normalmente `parser/parser.y` (ou similar).
- **Entrada:** Fluxo de tokens advindo do Flex.
- **Saída:** Árvore de Sintaxe Abstrata (AST) ou código intermediário.

Mais detalhes e regras gramaticais deverão ser documentados aqui durante o desenvolvimento.
