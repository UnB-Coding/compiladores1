import subprocess
import shutil
import sys
from pathlib import Path
import pytest

ROOT = Path(__file__).parent.parent
BUILD_DIR = ROOT / "build"

# No Windows o gcc gera executáveis com .exe; no Linux sem extensão.
# Usar o sufixo correto evita pegar um ELF Linux quando rodando no Windows.
_EXE = ".exe" if sys.platform == "win32" else ""

# ---------------------------------------------------------------------------
# Caminhos — lexer standalone
# ---------------------------------------------------------------------------
LEXER_TEST_BIN = BUILD_DIR / f"lexer_test_exe{_EXE}"
LEXER_YY_C = BUILD_DIR / "lexer.yy.c"
LEXER_TEST_MAIN = ROOT / "tests" / "lexer_test_main.c"

# ---------------------------------------------------------------------------
# Caminhos — scanner + parser
# ---------------------------------------------------------------------------
SCANNER_TEST_BIN = BUILD_DIR / f"scanner_test_exe{_EXE}"
SCANNER_YY_C = BUILD_DIR / "lex.yy.c"
PARSER_TAB_H = BUILD_DIR / "parser.tab.h"
PARSER_TAB_C = BUILD_DIR / "parser.tab.c"
SCANNER_TEST_MAIN = ROOT / "tests" / "scanner_test_main.c"
PARSER_EXE = BUILD_DIR / f"parser_exe{_EXE}"


# ---------------------------------------------------------------------------
# Utilitário: localiza ferramentas no PATH ou no MSYS2
# ---------------------------------------------------------------------------
def _find_tool(name: str) -> str:
    found = shutil.which(name)
    if found:
        return found
    for candidate in [
        f"/c/msys64/usr/bin/{name}",
        f"/c/msys64/usr/bin/{name}.exe",
        f"/c/msys64/ucrt64/bin/{name}",
        f"/c/msys64/ucrt64/bin/{name}.exe",
    ]:
        if Path(candidate).exists():
            return candidate
    return name  # deixa o subprocess falhar com mensagem clara


# ---------------------------------------------------------------------------
# Build — lexer standalone
# ---------------------------------------------------------------------------

def _build_lexer_yy_c():
    if LEXER_YY_C.exists():
        return
    flex = _find_tool("flex")
    r = subprocess.run(
        [flex, "-o", str(LEXER_YY_C), str(ROOT / "examples" / "lexer.l")],
        cwd=ROOT, capture_output=True, text=True,
    )
    if r.returncode != 0:
        raise RuntimeError(f"flex (lexer.l) falhou:\n{r.stderr}")


def _build_lexer_test_bin():
    gcc = _find_tool("gcc")
    r = subprocess.run(
        [
            gcc,
            f"-I{ROOT}", f"-I{ROOT / 'src'}", f"-I{ROOT / 'symbol_table'}",
            "-o", str(LEXER_TEST_BIN),
            str(LEXER_YY_C),
            str(LEXER_TEST_MAIN),
        ],
        cwd=ROOT, capture_output=True, text=True,
    )
    if r.returncode != 0:
        raise RuntimeError(f"Falha ao compilar lexer_test_exe:\n{r.stderr}")


# ---------------------------------------------------------------------------
# Build — scanner (scanner.l) e parser_exe
# ---------------------------------------------------------------------------

def _build_parser_tab():
    """Gera build/parser.tab.c e build/parser.tab.h a partir de parser.y."""
    if PARSER_TAB_H.exists() and PARSER_TAB_C.exists():
        return
    bison = _find_tool("bison")
    r = subprocess.run(
        [bison, f"--defines={PARSER_TAB_H}", "-o", str(PARSER_TAB_C),
         str(ROOT / "src" / "parser.y")],
        cwd=ROOT, capture_output=True, text=True,
    )
    if r.returncode != 0:
        raise RuntimeError(f"bison falhou:\n{r.stderr}")


def _build_scanner_yy_c():
    """Gera build/lex.yy.c a partir de scanner.l."""
    if SCANNER_YY_C.exists():
        return
    flex = _find_tool("flex")
    r = subprocess.run(
        [flex, "-o", str(SCANNER_YY_C), str(ROOT / "src" / "scanner.l")],
        cwd=ROOT, capture_output=True, text=True,
    )
    if r.returncode != 0:
        raise RuntimeError(f"flex (scanner.l) falhou:\n{r.stderr}")


def _build_scanner_test_bin():
    """Compila scanner_test_exe: lex.yy.c + scanner_test_main.c."""
    gcc = _find_tool("gcc")
    r = subprocess.run(
        [
            gcc,
            f"-I{ROOT}", f"-I{BUILD_DIR}", f"-I{ROOT / 'src'}", f"-I{ROOT / 'symbol_table'}",
            "-o", str(SCANNER_TEST_BIN),
            str(SCANNER_YY_C),
            str(SCANNER_TEST_MAIN),
        ],
        cwd=ROOT, capture_output=True, text=True,
    )
    if r.returncode != 0:
        raise RuntimeError(f"Falha ao compilar scanner_test_exe:\n{r.stderr}")


def _build_parser_exe():
    """Compila parser_exe completo: parser.tab.c + lex.yy.c + symtab.c + ast.c."""
    gcc = _find_tool("gcc")
    r = subprocess.run(
        [
            gcc,
            f"-I{ROOT}", f"-I{BUILD_DIR}", f"-I{ROOT / 'src'}", f"-I{ROOT / 'symbol_table'}",
            "-o", str(PARSER_EXE),
            str(PARSER_TAB_C),
            str(SCANNER_YY_C),
            str(ROOT / "symbol_table" / "symtab.c"),
            str(ROOT / "src" / "ast.c"),
        ],
        cwd=ROOT, capture_output=True, text=True,
    )
    if r.returncode != 0:
        raise RuntimeError(f"Falha ao compilar parser_exe:\n{r.stderr}")


# ---------------------------------------------------------------------------
# Fixtures de sessão — build
# ---------------------------------------------------------------------------

@pytest.fixture(scope="session", autouse=True)
def build_lexer_test():
    """Compila lexer_test_exe (usado pelos testes do lexer standalone)."""
    _build_lexer_yy_c()
    _build_lexer_test_bin()


@pytest.fixture(scope="session")
def build_scanner_test():
    """Compila scanner_test_exe e parser_exe (usados pelos testes de scanner/parser)."""
    _build_parser_tab()
    _build_scanner_yy_c()
    _build_scanner_test_bin()
    _build_parser_exe()


# ---------------------------------------------------------------------------
# Fixtures de execução
# ---------------------------------------------------------------------------

@pytest.fixture
def lex():
    """Tokeniza via lexer standalone (tokens: KW_INT, IDENTIFIER, OP_PLUS…)."""
    def run(source: str) -> list[dict]:
        proc = subprocess.run(
            [str(LEXER_TEST_BIN)],
            input=source, capture_output=True, text=True,
        )
        tokens = []
        for line in proc.stdout.splitlines():
            if line.startswith("TOKEN "):
                parts = line.split(" ", 2)
                tokens.append({
                    "type":  parts[1],
                    "value": parts[2] if len(parts) > 2 else "",
                })
        return tokens
    return run


@pytest.fixture
def scan(build_scanner_test):
    """Tokeniza via scanner.l (tokens Bison: T_INT, ID, PLUS, EQ…)."""
    def run(source: str) -> list[dict]:
        proc = subprocess.run(
            [str(SCANNER_TEST_BIN)],
            input=source, capture_output=True, text=True,
        )
        tokens = []
        for line in proc.stdout.splitlines():
            if line.startswith("TOKEN "):
                parts = line.split(" ", 2)
                tokens.append({
                    "type":  parts[1],
                    "value": parts[2] if len(parts) > 2 else "",
                })
        return tokens
    return run


@pytest.fixture
def parse(build_scanner_test):
    """Executa parser_exe com o código C fornecido via stdin.

    Retorna dict com chaves:
      stdout     — saída padrão (declarações, resultados, tabela de símbolos)
      stderr     — saída de erro (mensagens de erro sintático/semântico)
      returncode — código de saída (0 = sucesso)
    """
    def run(source: str) -> dict:
        proc = subprocess.run(
            [str(PARSER_EXE)],
            input=source, capture_output=True, text=True,
        )
        return {
            "stdout":     proc.stdout,
            "stderr":     proc.stderr,
            "returncode": proc.returncode,
        }
    return run
