# Testes

## Como funciona

Os testes utilizam **pytest** para verificar o comportamento do lexer de forma automatizada. A arquitetura tem três camadas:

```
tests/
├── conftest.py          # fixtures de build e execução
├── lexer_test_main.c    # driver C com main() para o binário de teste
└── test_lexer.py        # casos de teste organizados por categoria
```

### Fluxo de build (automático ao rodar os testes)

1. O `conftest.py` verifica se `build/lexer.yy.c` já existe; se não existir, invoca o `flex` para gerá-lo a partir de `lexer/lexer.l`.
2. Em seguida compila `build/lexer.yy.c` + `tests/lexer_test_main.c` com `gcc` (sem `-lfl`, porque `lexer_test_main.c` fornece o próprio `main`), gerando o binário `build/lexer_test_exe`.
3. O `gcc` e o `flex` são localizados via `PATH` ou, no Windows, nos caminhos conhecidos do MSYS2 (`/c/msys64/usr/bin` e `/c/msys64/ucrt64/bin`).

Esse build acontece uma única vez por sessão de testes (`scope="session"`), antes de qualquer caso de teste ser executado.

### Fixture `lex`

A fixture `lex` exposta pelo `conftest.py` retorna uma função auxiliar `run(source: str) -> list[dict]` que:

1. Passa a string `source` via **stdin** para `build/lexer_test_exe`.
2. Lê a saída linha a linha, extraindo as linhas com prefixo `TOKEN`.
3. Devolve uma lista de dicionários `{"type": ..., "value": ...}`.

Formato de saída do binário:

```
TOKEN KW_INT int
TOKEN IDENTIFIER x
TOKEN OP_ASSIGN =
TOKEN LIT_INT 42
TOKEN SEMICOLON ;
```

### Categorias de teste (`test_lexer.py`)

| Classe | O que testa |
|---|---|
| `TestKeywords` | Palavras-chave de tipo (`int`, `float`, `char`, ...) |
| `TestControlFlowKeywords` | Palavras-chave de controle (`if`, `while`, `return`, ...) |
| `TestIdentifiers` | Identificadores simples, com `_`, mistos e com dígitos |
| `TestLiterals` | Literais inteiros, float, string e char |
| `TestOperators` | Aritméticos, relacionais, lógicos e atribuição |
| `TestDelimiters` | Chaves, parênteses, colchetes, `;` e `,` |
| `TestWhitespaceAndComments` | Espaços, tabs, quebras de linha e comentários `//` e `/* */` |
| `TestRealisticInput` | Trechos reais de código C (declaração, `if`, função, `return`) |

---

## Dependências

- Python 3.x
- pytest >= 7.0 (declarado em `requirements-test.txt`)
- gcc e flex instalados (via MSYS2 no Windows)

Instalar o pytest:

```bash
pip install -r requirements-test.txt
```

---

## Como rodar

```bash
# Todos os testes
python -m pytest

# Com saída detalhada (nome de cada teste)
python -m pytest -v

# Apenas uma categoria
python -m pytest -v -k TestKeywords

# Apenas um teste específico
python -m pytest -v tests/test_lexer.py::TestLiterals::test_float_literal

# Parar no primeiro erro
python -m pytest -x
```

> O `pytest.ini` já aponta `testpaths = tests`, então não é necessário indicar o caminho da pasta.
