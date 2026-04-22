import subprocess
import shutil
from pathlib import Path
import pytest

ROOT = Path(__file__).parent.parent
LEXER_TEST_BIN = ROOT / "build" / "lexer_test_exe"
LEXER_YY_C = ROOT / "build" / "lexer.yy.c"
LEXER_TEST_MAIN = ROOT / "tests" / "lexer_test_main.c"


def _find_tool(name: str) -> str:
    """Localiza um executável no PATH ou em locais conhecidos do MSYS2."""
    found = shutil.which(name)
    if found:
        return found
    msys2_candidates = [
        f"/c/msys64/usr/bin/{name}",
        f"/c/msys64/usr/bin/{name}.exe",
        f"/c/msys64/ucrt64/bin/{name}",
        f"/c/msys64/ucrt64/bin/{name}.exe",
    ]
    for path in msys2_candidates:
        if Path(path).exists():
            return path
    return name  # tenta mesmo assim e deixa o subprocess errar


def _build_lexer_yy_c():
    """Gera build/lexer.yy.c a partir de lexer/lexer.l se necessário."""
    if LEXER_YY_C.exists():
        return
    flex = _find_tool("flex")
    result = subprocess.run(
        [flex, "-o", str(LEXER_YY_C), str(ROOT / "lexer" / "lexer.l")],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"flex falhou:\n{result.stderr}")


def _build_lexer_test_bin():
    """Compila o binário de teste linkando lexer.yy.c + lexer_test_main.c."""
    gcc = _find_tool("gcc")
    result = subprocess.run(
        [
            gcc,
            f"-I{ROOT}",
            f"-I{ROOT / 'symbol_table'}",
            "-o", str(LEXER_TEST_BIN),
            str(LEXER_YY_C),
            str(LEXER_TEST_MAIN),
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"Falha ao compilar lexer_test_exe:\n{result.stderr}"
        )


@pytest.fixture(scope="session", autouse=True)
def build_lexer_test():
    _build_lexer_yy_c()
    _build_lexer_test_bin()


@pytest.fixture
def lex():
    """Retorna uma função que tokeniza a string dada e devolve lista de dicts.

    Cada dict tem as chaves 'type' (nome do token) e 'value' (lexema).
    """
    def run(source: str) -> list[dict]:
        proc = subprocess.run(
            [str(LEXER_TEST_BIN)],
            input=source,
            capture_output=True,
            text=True,
        )
        tokens = []
        for line in proc.stdout.splitlines():
            if line.startswith("TOKEN "):
                parts = line.split(" ", 2)
                tok_type = parts[1]
                tok_value = parts[2] if len(parts) > 2 else ""
                tokens.append({"type": tok_type, "value": tok_value})
        return tokens

    return run
