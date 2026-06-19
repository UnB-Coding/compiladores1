# Histórico de Desenvolvimento e Sprints (Equipa 15)

## Visão Geral do Projeto
O projeto visa a construção de um transpilador educativo da linguagem C para Python. O desenvolvimento foi estruturado sob o princípio arquitetural de um "pipeline de fases desacopladas". Até ao momento, a equipa concluiu com sucesso 6 Sprints geridas com a metodologia ágil Scrum.

---

## Detalhamento das Sprints

### Sprint 1: Fundação e Analisador Léxico
* **Objetivo:** Estabelecer a infraestrutura do projeto e concluir a análise léxica completa.
* **Implementação:** Configuração do repositório, `Makefile` e desenvolvimento do analisador léxico com a ferramenta `Flex` (`scanner.l`).
* **Progresso:** Tokenização bem-sucedida de palavras-chave, operadores, literais e identificadores. Início do framework de testes usando TDD com a biblioteca `pytest`.

### Sprint 2: Analisador Sintático (Parser) e AST
* **Objetivo:** Análise sintática completa e demonstração funcional (PC1).
* **Implementação:** Analisador sintático desenvolvido com `Bison`, implementando regras para declarações, expressões e validação de precedência.
* **Árvore Sintática:** Construção da AST com 14 tipos de nós diferentes, incluindo um avaliador interativo (*tree-walker*).
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

### Sprint 5: Otimização e Refinamento
* **Objetivo da Fase:** Atuar sobre a Representação Intermediária para produzir código mais eficiente e limpo.
* **Foco do Otimizador:** Planeamento da implementação de otimizações independentes da máquina diretamente na IR, utilizando técnicas como *constant folding* (resolução de expressões constantes em tempo de compilação), eliminação de código morto (*dead code elimination*) e propagação de constantes.
* **Status Atual:** A finalização do otimizador foi oficialmente definida pela equipa como a prioridade principal e o que falta implementar para o fecho da próxima entrega.

### Sprint 6: Testes, QA e Code Freeze
* **Objetivo:** Expansão de cobertura de testes e encerramento do desenvolvimento focado na documentação (*Code Freeze*).
* **Qualidade de Software:** Atingimos a estabilidade do software comprovada por uma impressionante suíte automatizada de aproximadamente 150 testes executados via `pytest`.
* **Cobertura:** Implementação do `pytest-cov` com geração de relatórios em formato HTML.
* **Superação de Desafios:** As dificuldades iniciais na comunicação e falta de experiência em desenvolvimento de compiladores foram mitigadas