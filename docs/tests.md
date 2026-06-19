# Qualidade, Testes e Automação

Para garantir a confiabilidade técnica do front-end e do motor semântico, o projeto conta com uma suíte de testes automatizados e regras de automação de compilação.

---

## 1. O que CONSEGUIMOS Fazer (Estrutura de Testes e Automação)

### 1.1 Suíte de Testes com Pytest
A equipe desenvolveu **mais de 150 casos de teste** usando a biblioteca `pytest` do Python. Estes testes estão divididos de forma modular na pasta [tests](../tests):

* [test_lexer.py](../tests/test_lexer.py): validação isolada do analisador léxico standalone, testando reconhecimento de keywords de tipo, literais inteiros/decimais, e o mapeamento de sequências de escape.
* [test_scanner.py](../tests/test_scanner.py): validação da integração do scanner.l fornecendo tokens para a gramática.
* [test_parser.py](../tests/test_parser.py): o teste mais complexo da suíte. Alimenta o executável compilado (`parser_exe`) via fluxo de entrada padrão (`stdin`) com fragmentos de código C e assevera:
  - **Validade Sintática:** Se expressões complexas e controle de fluxo compilam com código de retorno zero.
  - **Erros Sintáticos:** Valida se a falta de delimitadores (ex: ponto e vírgula, parênteses desbalanceados) reporta erros esperados no fluxo de erro padrão (`stderr`).
  - **Comportamento da AST e Semântica:** Verifica se as saídas impressas de declarações, atribuições e expressões correspondem aos valores calculados pelo *tree-walker*.
  - **Erros Semânticos:** Valida a falha controlada do processo e as mensagens de erro em caso de divisões por zero ou uso de variáveis não declaradas.
  - **Regras de Coerção:** Testa exaustivamente a coerção implícita (truncamentos, promoções aritméticas de booleanos e caracteres).

### 1.2 Fixture de Compilação Automatizada
O arquivo [conftest.py](../tests/conftest.py) gerencia as fixtures do pytest. Ele é responsável por:
* Invocar a compilação automática dos binários (`make`) na pasta `build/` antes de disparar os testes, garantindo que o pytest sempre execute a versão mais recente do código-fonte em C.
* Capturar a entrada (`stdin`), saída (`stdout`), erros (`stderr`) e códigos de retorno dos processos executados, permitindo asserções programáticas flexíveis.

### 1.3 Cobertura com Pytest-Cov
* A cobertura de testes do código C e Python é medida e reportada graficamente na pasta `build/coverage_html/` através do módulo `pytest-cov`, permitindo auditoria visual dos caminhos de execução validados.

### 1.4 Automação de Compilação (Makefile)
O arquivo [Makefile](../Makefile) centraliza o fluxo de geração de arquivos pelo Flex e Bison e a compilação final dos executáveis.
* **Executável Principal:** `build/parser_exe` (compilado a partir do Bison `parser.tab.c`, Flex `lex.yy.c`, `src/ast.c` e `symbol_table/symtab.c`).
* **Regras Auxiliares:** Suporte para compilar o lexer standalone e limpar artefatos (`make clean`).

---

## 2. O que NÃO CONSEGUIMOS Fazer (Limitações de Qualidade e Ambiente)

* **Dependência do Ambiente Local:** A execução automatizada do pytest depende da presença das ferramentas GCC, Flex, Bison e Make instaladas e configuradas no PATH do sistema. Os testes falham de imediato se executados em ambientes Windows nativos que não possuam um subsistema como MSYS2/MinGW habilitado.
* **Testes de Transpilação e Otimização:** A suíte de testes valida a geração correta das instruções lineares do código intermediário (TAC) e a fidelidade do código Python final gerado pelo transpilador, executando o arquivo Python de saída e validando seus resultados.
* **Ausência de Análise Dinâmica de Memória Automatizada:** Embora tenhamos projetado o desalocador da AST (`free_ast()`) e da tabela de símbolos (`sym_free()`) para evitar vazamentos de memória (memory leaks), a suíte de testes do pytest não executa ferramentas de análise dinâmica de memória (como o *Valgrind*) de forma automatizada no pipeline de testes.
