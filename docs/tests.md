# Testes

## Estrutura

```
tests/
├── conftest.py           # build automático e fixtures
├── lexer_test_main.c     # driver C do lexer standalone
├── scanner_test_main.c   # driver C do scanner (tokens Bison)
├── test_lexer.py         # testes do lexer/lexer.l
├── test_scanner.py       # testes do scanner.l
└── test_parser.py        # testes do parser.y + ast.c
```

---

## Instalação

```bash
pip install -r requirements-test.txt
```

Dependências de sistema: `gcc`, `flex`, `bison`.

- **Linux:** `sudo apt install gcc flex bison`
- **Windows:** MSYS2 com `pacman -S mingw-w64-ucrt-x86_64-gcc flex bison`

---

## Como rodar

```bash
# Todos os testes (com relatório de cobertura)
python -m pytest

# Saída detalhada por teste
python -m pytest -v

# Filtrar por suíte ou categoria
python -m pytest tests/test_parser.py
python -m pytest -k TestDeclarations

# Parar no primeiro erro
python -m pytest -x
```

O `pytest.ini` define `testpaths = tests`, então não é necessário passar o caminho.

---

## Relatório de cobertura

Ao rodar `python -m pytest`, o terminal exibe automaticamente:

```
Name                    Stmts   Miss  Cover   Missing
-----------------------------------------------------
tests\conftest.py          99     19    81%   39-47, 57-63 ...
tests\test_lexer.py       175      0   100%
tests\test_parser.py      171      0   100%
tests\test_scanner.py     205      0   100%
-----------------------------------------------------
TOTAL                     650     19    97%
```

- **`Stmts`** — linhas executáveis
- **`Miss`** — linhas não executadas por nenhum teste
- **`Missing`** — números das linhas não cobertas

O relatório HTML completo é gerado em `build/coverage_html/index.html`.

> O `conftest.py` tem 81% porque as linhas faltantes são blocos de tratamento de erro dos builds (ex: `raise RuntimeError` quando `flex` falha). Esses caminhos só executam em ambientes quebrados — não vale testá-los.

> A cobertura mede apenas o código **Python** (`conftest.py` e arquivos `test_*.py`). O código C (`ast.c`, `scanner.l`, `parser.y`) roda como processo externo via `subprocess` e não é medido aqui.

---

## Fixtures

O `conftest.py` compila os binários automaticamente antes dos testes, sem precisar rodar `make`.

| Fixture | Binário | Tokens |
|---|---|---|
| `lex` | `build/lexer_test_exe` | `KW_INT`, `IDENTIFIER`, `OP_PLUS` |
| `scan` | `build/scanner_test_exe` | `T_INT`, `ID`, `PLUS` |
| `parse` | `build/parser_exe` | recebe código C, retorna `stdout`/`stderr`/`returncode` |

### `lex` e `scan`

Ambas recebem uma string de código e retornam uma lista de dicionários `{"type": ..., "value": ...}`:

```python
tokens = lex("int x = 42;")
# [{"type": "KW_INT", "value": "int"}, {"type": "IDENTIFIER", "value": "x"}, ...]

tokens = scan("int x = 42;")
# [{"type": "T_INT", "value": "int"}, {"type": "ID", "value": "x"}, ...]
```

### `parse`

Executa o parser completo com o código recebido via stdin:

```python
r = parse("int x = 5; x + 1;")
r["returncode"]  # 0 = sem erro de compilação
r["stdout"]      # "Declarado: x : int = 5\nResultado: 6\n..."
r["stderr"]      # mensagens de erro sintático/semântico
```

Formato do stdout:

| Comando C | Saída |
|---|---|
| `int x = 5;` | `Declarado: x : int = 5` |
| `int x;` | `Declarado: x : int` |
| `x = 10;` | `int x = 10` |
| `3 + 4;` | `Resultado: 7` |
| fim do programa | `--- Tabela de Símbolos ---` |

> `parser.y` sempre termina com `exit 0`, mesmo em erro sintático. Para verificar erros, cheque `r["stderr"]`.

---

## Casos de teste

### `test_lexer.py` — `lexer/lexer.l`

| Classe | Cobre |
|---|---|
| `TestKeywords` | `int`, `float`, `char`, `long`, `double`, `short`, `signed`, `unsigned` |
| `TestControlFlowKeywords` | `if`, `else`, `while`, `for`, `do`, `switch`, `case`, `break`, `continue`, `return` |
| `TestIdentifiers` | simples, `_prefixo`, camelCase, com dígitos, não começa com dígito |
| `TestLiterals` | inteiro, float, notação científica, string, char |
| `TestOperators` | aritméticos, relacionais, lógicos, atribuição |
| `TestDelimiters` | `{}`, `()`, `[]`, `;`, `,` |
| `TestWhitespaceAndComments` | espaços, tabs, `\n`, comentários `//` e `/* */` |
| `TestRealisticInput` | trechos reais de código C |

### `test_scanner.py` — `scanner.l`

| Classe | Cobre |
|---|---|
| `TestTypeKeywords` | `T_INT`, `T_FLOAT`, `T_CHAR`, `T_BOOL` |
| `TestControlFlowKeywords` | `KW_IF/ELSE/WHILE/FOR`; prefixo de keyword vira `ID` |
| `TestBoolLiterals` | `TRUE_LIT`, `FALSE_LIT`; `truex` → `ID` |
| `TestIdentifiers` | simples, underscore, camelCase, com dígitos |
| `TestNumericLiterals` | `NUM`, `FLOAT_LIT`; float reconhecido antes de inteiro |
| `TestCharLiterals` | `'a'`, `'\n'`, `'\t'`, `'\\'` |
| `TestArithmeticOperators` | `PLUS`, `MINUS`, `TIMES`, `DIVIDE` |
| `TestRelationalOperators` | `EQ/NE/LE/GE/LT/GT`; `=` não confunde com `==` |
| `TestLogicalOperators` | `AND`, `OR`, `NOT`; `!` não confunde com `!=` |
| `TestAssignAndDelimiters` | `ASSIGN`, `SEMICOLON`, `LPAREN/RPAREN`, `LBRACE/RBRACE` |
| `TestWhitespace` | espaços, tabs, newlines ignorados; entrada vazia |
| `TestRealisticSequences` | declaração, `if`, `while`, `for`, bloco, bool |

### `test_parser.py` — `parser.y` + `ast.c`

| Classe | Cobre |
|---|---|
| `TestSyntaxValidity` | programas válidos (exit 0); erros sintáticos no stderr |
| `TestDeclarations` | `int`, `float`, `char`, `bool` com e sem init; redeclaração falha |
| `TestAssignments` | saída `tipo nome = valor`; variável não declarada falha |
| `TestExpressionStatements` | aritmética, divisão inteira vs float, precedência, lógica |
| `TestControlFlow` | `if`/`else`, `while`, `for` iterando, blocos `{}` |
| `TestSymbolTable` | tabela presente, `(vazia)` sem variáveis, valor final correto |

---

## Makefile

```bash
make        # compila parser_exe e lexer_exe
make clean  # remove artefatos de build
```

Os testes **não dependem do `make`** — o `conftest.py` compila tudo automaticamente.
