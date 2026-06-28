# Qualidade, Testes e Automação

Para garantir a confiabilidade técnica do front-end, do motor semântico e do back-end de IR, o projeto conta com uma suíte de testes automatizados e regras de automação de compilação.

---

## 1. O que CONSEGUIMOS Fazer (Estrutura de Testes e Automação)

### 1.1 Suíte de Testes com Pytest
A equipe desenvolveu **mais de 150 casos de teste** usando a biblioteca `pytest` do Python. Estes testes estão divididos de forma modular na pasta [tests](../tests):

* [test_scanner.py](../tests/test_scanner.py): validação da integração do `scanner.l`, testando reconhecimento de keywords de tipo, literais inteiros/decimais, sequências de escape e o mapeamento correto de tokens para a gramática do Bison.
* [test_parser.py](../tests/test_parser.py): o teste mais complexo da suíte. Alimenta o executável compilado (`parser_exe`) via fluxo de entrada padrão (`stdin`) com fragmentos de código C e assevera:
  - **Validade Sintática:** Se expressões complexas e controle de fluxo compilam com código de retorno zero.
  - **Erros Sintáticos:** Valida se a falta de delimitadores (ex: ponto e vírgula, parênteses desbalanceados) reporta erros esperados no fluxo de erro padrão (`stderr`).
  - **Comportamento da AST e Semântica:** Verifica se as saídas impressas de declarações, atribuições e expressões correspondem aos valores esperados.
  - **Erros Semânticos:** Valida a falha controlada do processo e as mensagens de erro em caso de divisões por zero ou uso de variáveis não declaradas.
  - **Regras de Coerção:** Testa exaustivamente a coerção implícita (truncamentos, promoções aritméticas de booleanos e caracteres).
* [test_ir.py](../tests/test_ir.py): validação da geração de código intermediário (TAC). Alimenta o `parser_exe` com código-fonte e verifica o bloco TAC emitido — cobrindo expressões, precedência, cópias, declarações sem inicialização e os três tipos de controle de fluxo (incluindo aninhamento).
* [test_ir_optimize.py](../tests/test_ir_optimize.py): validação das otimizações do IR. Testa o dobramento de constantes (*constant folding*), a propagação de constantes (*constant propagation*) e a eliminação de código morto (*dead code elimination*), verificando que o IR otimizado produz resultados corretos e elimina as instruções redundantes.

### 1.2 Fixture de Compilação Automatizada
O arquivo [conftest.py](../tests/conftest.py) gerencia as fixtures do pytest. Ele é responsável por:
* Invocar a compilação automática dos binários (`make`) na pasta `build/` antes de disparar os testes, garantindo que o pytest sempre execute a versão mais recente do código-fonte em C.
* Capturar a entrada (`stdin`), saída (`stdout`), erros (`stderr`) e códigos de retorno dos processos executados, permitindo asserções programáticas flexíveis.

### 1.3 Cobertura com Pytest-Cov
* A cobertura de testes do código Python é medida e reportada graficamente através do módulo `pytest-cov`, permitindo auditoria visual dos caminhos de execução validados.

### 1.4 Automação de Compilação (Makefile)
O arquivo [Makefile](../Makefile) centraliza o fluxo de geração de arquivos pelo Flex e Bison e a compilação final do executável.
* **Executável Principal:** `build/parser_exe` (compilado a partir do Bison `parser.tab.c`, Flex `lex.yy.c`, `src/ast.c`, `src/semantic.c`, `src/ir.c` e `symbol_table/symtab.c`).
* **Regra de Limpeza:** `make clean` para remover artefatos gerados.

---

## 2. O que NÃO CONSEGUIMOS Fazer (Limitações de Qualidade e Ambiente)

* **Dependência do Ambiente Local:** A execução automatizada do pytest depende da presença das ferramentas GCC, Flex, Bison e Make instaladas e configuradas no PATH do sistema. Os testes falham de imediato se executados em ambientes Windows nativos que não possuam um subsistema como MSYS2/MinGW habilitado.
* **Ausência de Análise Dinâmica de Memória Automatizada:** Embora tenhamos projetado o desalocador da AST (`free_ast()`), do IR (`ir_free()`) e da tabela de símbolos (`sym_free()`) para evitar vazamentos de memória (memory leaks), a suíte de testes do pytest não executa ferramentas de análise dinâmica de memória (como o *Valgrind*) de forma automatizada no pipeline de testes.
