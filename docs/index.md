# Interpretador C (Equipe 15)

Documentação técnica do projeto da **Equipe 15** desenvolvido para a disciplina de **Compiladores 1** da Universidade de Brasília (UnB). 

Desenvolvemos um interpretador completo de um subconjunto estruturado da linguagem C. O interpretador realiza análise léxica e sintática (com construção de AST), validação semântica com verificação e coerção de tipos, geração de código intermediário linear (TAC), otimização desse código intermediário (dobramento e propagação de constantes, eliminação de código morto) e execução direta da IR otimizada.

---

## Membros da Equipe 15
* Vitor Feijó Leonardo
* Caio Pacheco Santos
* Hauedy Wegener Soares
* Gabriel Henrique Castelo Costa
* Pedro Henrique Ferreira Xavier

---

## Como a Documentação está Organizada

* **[Evolução do Projeto e Sprints](sprints.md):** Histórico de desenvolvimento (Sprints 1 a 8) utilizando Scrum e a experiência com a prática de Pair Programming.
* **[Analisador Léxico (Flex)](lexer.md):** Regras de expressões regulares, mapeamento de keywords, tratamento de sequências de escape em caracteres e limitações léxicas.
* **[Analisador Sintático e AST](parser.md):** Integração Flex-Bison, resolução de precedências de operadores, estrutura de 14 nós da AST, construção da árvore (somente parsing, sem execução) e liberação de memória.
* **[Tabela de Símbolos e Tipagem](guia_gramatica.md):** Funcionamento do algoritmo de hash `djb2`, buckets para tratamento de colisões e regras de coerção e promoção implícitas.
* **[Qualidade, Testes e Automação](tests.md):** Estruturação da suíte com mais de 150 testes em `pytest`, fixture de build e relatórios de cobertura com `pytest-cov`.
* **[Como Executar](como_executar.md):** Instruções e pré-requisitos para compilar e testar o interpretador localmente.
* **[Explicação dos Arquivos](explicacao_arquivos.md):** Arquitetura interna e mapeamento de diretórios do repositório.
* **[Contribuição](contributing.md):** Diretrizes para desenvolvimento colaborativo no repositório.

---

## Visão Geral do Escopo do Interpretador

### O que CONSEGUIMOS Fazer (Funcionalidades Ativas)
O interpretador aceita e processa com sucesso um código C que contenha:
* Declarações de variáveis de tipo `int`, `float`, `char` e `bool` com inicializações opcionais (ex: `int x = 10;`).
* Atribuições simples a variáveis previamente declaradas (`x = x + 5;`).
* Expressões complexas relacionais, lógicas e aritméticas com precedência matemática correta (ex: `2 + 3 * 4` resulta em `14`).
* Coerções implícitas e promoções de tipos conforme o padrão ANSI C (como promover `char` para `int` em operações aritméticas e truncar `float` para `int` em atribuições).
* Controle de fluxo estruturado: desvios condicionais `if` e `if/else`, e laços iterativos `while` e `for`.
* Blocos aninhados de instruções contidos entre chaves `{ ... }`.
* Geração de código intermediário linear no formato de Código de Três Endereços (TAC).
* Otimizações sobre a IR (dobramento e propagação de constantes, eliminação de código morto), aplicadas em ponto fixo.
* Execução do código por meio de interpretação direta da IR otimizada (`ir_exec`).
* Exibição da árvore sintática e do TAC (antes e depois da otimização) para fins didáticos.
* Desalocação completa de memória pós-ordem (`free_ast()`), garantindo execução livre de vazamentos de memória.

### O que NÃO CONSEGUIMOS Fazer (Limitações Atuais)
* **Sem Funções ou Sub-rotinas:** A linguagem executa apenas linearmente. Não há sintaxe para declaração de funções ou desvio `return`.
* **Sem Pointers, Arrays ou Structs:** Tipos avançados (ponteiros, vetores e estruturas) não são reconhecidos.
* **Sem Desvios Secundários:** O interpretador não suporta palavras-chave de escape ou controle fino como `break`, `continue`, `switch-case` ou `do-while`.
* **Sem Pré-processador:** Diretivas como `#include` ou `#define` não são processadas.
