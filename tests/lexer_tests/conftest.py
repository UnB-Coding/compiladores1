# conftest.py — Fixtures para testar o Lexer (Flex) de expressões aritméticas
# =============================================================================
#
# O driver lexer_test imprime tokens no formato:
#     <TIPO, "lexema">
#
# Este conftest parseia essa saída e fornece o LexerRunner via fixture.

import subprocess
import re
import os
import pytest
from dataclasses import dataclass
from pathlib import Path


# ---------------------------------------------------------------------------
# 1. ESTRUTURA DE DADOS
# ---------------------------------------------------------------------------
@dataclass(frozen=True)
class Token:
    """Representação de um token retornado pelo Lexer."""
    type: str       # Ex: "NUM", "PLUS", "MINUS"
    lexeme: str     # Ex: "42", "+", "-"


# ---------------------------------------------------------------------------
# 2. PARSER DA SAÍDA DO DRIVER
# ---------------------------------------------------------------------------
# Captura: <TIPO, "lexema">
TOKEN_PATTERN = re.compile(r'<\s*(\w+)\s*,\s*"([^"]*)"\s*>')


def parse_lexer_output(stdout: str) -> list[Token]:
    """Converte a saída do driver em lista de Tokens."""
    return [
        Token(type=m.group(1), lexeme=m.group(2))
        for m in TOKEN_PATTERN.finditer(stdout)
    ]


# ---------------------------------------------------------------------------
# 3. EXECUTOR DO LEXER
# ---------------------------------------------------------------------------
class LexerRunner:
    """Executa o binário lexer_test via subprocess e parseia a saída."""

    def __init__(self, binary_path: str, timeout: float = 5.0):
        self.binary_path = binary_path
        self.timeout = timeout

    def run_raw(self, source_code: str) -> subprocess.CompletedProcess:
        """Executa o lexer e retorna o resultado cru (stdout + stderr + returncode)."""
        return subprocess.run(
            [self.binary_path],
            input=source_code,
            capture_output=True,
            text=True,
            timeout=self.timeout,
        )

    def tokenize(self, source_code: str) -> list[Token]:
        """Executa o lexer e retorna a lista de Tokens parseados."""
        result = self.run_raw(source_code)
        # returncode 0 = sucesso
        assert result.returncode == 0, (
            f"Lexer retornou código {result.returncode}.\n"
            f"stderr: {result.stderr}\n"
            f"entrada: {source_code!r}"
        )
        return parse_lexer_output(result.stdout)

    def tokenize_with_stderr(self, source_code: str) -> tuple[list[Token], str]:
        """Retorna tokens E o conteúdo de stderr (para verificar mensagens de erro)."""
        result = self.run_raw(source_code)
        tokens = parse_lexer_output(result.stdout)
        # stderr + stdout combinados para capturar mensagens do printf de erro
        all_output = result.stdout + result.stderr
        return tokens, all_output


# ---------------------------------------------------------------------------
# 4. FIXTURES
# ---------------------------------------------------------------------------
LEXER_BINARY_ENV = "LEXER_BIN"
LEXER_BINARY_DEFAULT = "lexer/lexer_test"  # relativo à raiz do projeto


@pytest.fixture(scope="session")
def lexer_binary_path() -> str:
    """Resolve o caminho do binário do Lexer de teste."""
    path = os.environ.get(LEXER_BINARY_ENV, LEXER_BINARY_DEFAULT)
    resolved = str(Path(path).resolve())
    if not os.path.isfile(resolved):
        pytest.skip(
            f"Binário não encontrado em '{resolved}'.\n"
            f"Compile com: cd lexer && make test-driver\n"
            f"Ou defina LEXER_BIN=/caminho/para/lexer_test"
        )
    if not os.access(resolved, os.X_OK):
        pytest.skip(f"Binário '{resolved}' sem permissão de execução.")
    return resolved


@pytest.fixture(scope="session")
def lexer(lexer_binary_path: str) -> LexerRunner:
    """Fornece o LexerRunner pronto para uso em todos os testes."""
    return LexerRunner(lexer_binary_path)
