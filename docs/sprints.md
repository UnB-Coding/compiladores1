# Histórico de Desenvolvimento e Sprints (Equipa 15)

## Visão Geral do Projeto
O projeto visa a construção de um interpretador educativo da linguagem C. O desenvolvimento foi estruturado sob o princípio arquitetural de um "pipeline de fases desacopladas". Até ao momento, a equipa concluiu com sucesso 8 Sprints geridas com a metodologia ágil Scrum.

---

## Detalhamento das Sprints

### Sprint 1: Fundação e Analisador Léxico
* **Objetivo:** Estabelecer a infraestrutura do projeto e concluir a análise léxica completa.
* **Implementação:** Configuração do repositório, `Makefile` e desenvolvimento do analisador léxico com a ferramenta `Flex` (`scanner.l`).
* **Progresso:** Tokenização bem-sucedida de palavras-chave, operadores, literais e identificadores. Início do framework de testes usando TDD com a biblioteca `pytest`.

### Sprint 2: Analisador Sintático (Parser) e AST
* **Objetivo:** Análise sintática completa e demonstração funcional (PC1).
* **Implementação:** Analisador sintático desenvolvido com `Bison`, implementando regras para declarações, expressões e validação de precedência.
* **Árvore Sintática:** Construção da AST com 14 tipos de nós diferentes.
* **Demo Visual:** Criação de um *pretty-printer* para impressão hierárquica da AST no terminal para facilitação de debug.

### Sprint 3: Semântica e Definição da IR
* **Objetivo:** Validação de tipos, Tabela de Símbolos e início da modelagem da Representação Intermediária (IR).
* **Tabela de Símbolos:** Construída via *hash table* (utilizando algoritmo `djb2`) para lidar com a gestão em memória das variáveis declaradas.
* **Motor Semântico:** Implementação de regras de escopo, promoção de tipos e coerção (ex: suporte nativo aos tipos `int`, `float`, `char` e `bool`).
* **Definição da IR:** Mapeamento teórico e definição do formato estrutural do Código de Três Endereços (*Three-Address Code*).

### Sprint 4: Integração de Controle de Fluxo e Gerador de IR
* **Controlo de Fluxo:** Expansão massiva da AST para integração de blocos lógicos como `if`/`else`, e laços de repetição como `while` e `for`.
* **Geração de Código Intermediário:** Avanço decisivo no projeto com a implementação real do gerador de código intermediário integrado ao pipeline. A lógica da árvore passou a poder ser traduzida para as instruções simplificadas de três endereços.
* **Segurança e Memória:** Aplicação do sistema de gestão de libertação de memória da AST e da tabela de símbolos, resolvendo *memory leaks*.

### Sprint 5: Otimização do IR
* **Objetivo:** Atuar sobre a Representação Intermediária para produzir código mais eficiente e limpo.
* **Implementação:** Desenvolvimento de 6 passes de otimização executadas em loop de ponto fixo:
  - *Constant folding* (resolução de expressões constantes em tempo de compilação).
  - *Constant propagation* (substituição de usos de temporários constantes pelo valor).
  - *Dead code elimination* (remoção de código inalcançável após desvios constantes).
  - *Dead temp elimination* (remoção de definições de temporários nunca lidos).
  - *Redundant goto elimination* (remoção de desvios para a instrução seguinte).
  - *Dead label elimination* (remoção de rótulos sem referências).
* **Status:** Otimizador completo e funcional.

### Sprint 6: Análise Semântica Estática
* **Objetivo:** Implementar verificação semântica estática independente da execução.
* **Implementação:** Módulo `semantic.c` com tabela shadow independente que percorre toda a AST (incluindo dead code) para detectar erros estáticos antes da geração de IR.
* **Verificações:** Variáveis não declaradas, redeclarações, divisão por zero com literais e conversões implícitas com perda de precisão (avisos).

### Sprint 7: Execução via IR e Interpretador de TAC
* **Objetivo:** Substituir o modelo de execução por interpretação direta do código intermediário.
* **Implementação:** Função `ir_exec()` que lineariza as instruções TAC em array, constrói mapa de rótulos e interpreta com ponteiro de instrução. Saída automática de declarações, atribuições e expressões.
* **Resultado:** Pipeline completo de 5 fases (léxico → sintático → semântica → IR/otimização → execução).

### Sprint 8: Testes, QA e Code Freeze
* **Objetivo:** Expansão de cobertura de testes e encerramento do desenvolvimento focado na documentação (*Code Freeze*).
* **Qualidade de Software:** Atingimos a estabilidade do software comprovada por uma impressionante suíte automatizada de aproximadamente 150 testes executados via `pytest`, incluindo testes específicos para geração de IR (`test_ir.py`) e otimizações (`test_ir_optimize.py`).
* **Cobertura:** Implementação do `pytest-cov` com geração de relatórios em formato HTML.
* **Superação de Desafios:** As dificuldades iniciais na comunicação e falta de experiência em desenvolvimento de compiladores foram mitigadas pela prática contínua de Pair Programming e revisões de código em equipe.