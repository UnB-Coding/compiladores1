"""Testes da geração de bytecode e execução via VM (vm.c).

Alimenta o build/parser_exe com código-fonte C via stdin e verifica:
  1. A seção "=== Bytecode ===" está presente e bem formada.
  2. As instruções geradas são as corretas para cada construct.
  3. A VM produz a mesma saída de execução que o antigo eval_ast().
"""

import pytest


def _bc(stdout: str) -> str:
    """Extrai o bloco de bytecode do stdout."""
    marker = "=== Bytecode ==="
    assert marker in stdout, "bloco de bytecode ausente na saída"
    tail = stdout.split(marker, 1)[1]
    return tail.split("====", 1)[0]


# ---------------------------------------------------------------------------
# Presença e forma geral
# ---------------------------------------------------------------------------

class TestBytecodePresence:
    def test_cabecalho_presente(self, parse):
        r = parse("int x = 1;")
        assert "=== Bytecode ===" in r["stdout"]

    def test_programa_vazio_nao_quebra(self, parse):
        r = parse("")
        assert r["returncode"] == 0

    def test_halt_presente(self, parse):
        bc = _bc(parse("int x = 1;")["stdout"])
        assert "HALT" in bc


# ---------------------------------------------------------------------------
# Geração de expressões
# ---------------------------------------------------------------------------

class TestBytecodeGenExprs:
    def test_push_int(self, parse):
        bc = _bc(parse("42;")["stdout"])
        assert "PUSH_INT" in bc

    def test_push_float(self, parse):
        bc = _bc(parse("3.14;")["stdout"])
        assert "PUSH_FLOAT" in bc

    def test_push_bool(self, parse):
        bc = _bc(parse("true;")["stdout"])
        assert "PUSH_BOOL" in bc

    def test_push_char(self, parse):
        bc = _bc(parse("'a';")["stdout"])
        assert "PUSH_CHAR" in bc

    def test_binop(self, parse):
        bc = _bc(parse("int a = 1; int b = 2; a + b;")["stdout"])
        assert "BINOP +" in bc

    def test_unaryop(self, parse):
        bc = _bc(parse("int x = 1; !x;")["stdout"])
        assert "UNARYOP !" in bc

    def test_load_var(self, parse):
        bc = _bc(parse("int x = 5; x;")["stdout"])
        assert "LOAD_VAR x" in bc

    def test_expr_stmt_gera_print_expr(self, parse):
        bc = _bc(parse("42;")["stdout"])
        assert "PRINT_EXPR" in bc


# ---------------------------------------------------------------------------
# Declarações e atribuições
# ---------------------------------------------------------------------------

class TestBytecodeDeclaracoes:
    def test_decl_sem_init_gera_decl_var(self, parse):
        bc = _bc(parse("int x;")["stdout"])
        assert "DECL_VAR x" in bc

    def test_decl_com_init_gera_decl_e_push_e_store(self, parse):
        bc = _bc(parse("int x = 5;")["stdout"])
        assert "DECL_VAR x" in bc
        assert "PUSH_INT 5" in bc
        assert "STORE_VAR x" in bc

    def test_atribuicao_gera_store_var(self, parse):
        bc = _bc(parse("int x = 0; x = 7;")["stdout"])
        assert "STORE_VAR x" in bc


# ---------------------------------------------------------------------------
# Constant folding visível no bytecode (optimizer age antes do compilador)
# ---------------------------------------------------------------------------

class TestBytecodeConstantFolding:
    def test_soma_literal_foldada(self, parse):
        bc = _bc(parse("int x = 3 + 4;")["stdout"])
        assert "PUSH_INT 7" in bc
        assert "BINOP" not in bc

    def test_expressao_complexa_foldada(self, parse):
        bc = _bc(parse("int x = 2 * 3 + 4;")["stdout"])
        assert "PUSH_INT 10" in bc
        assert "BINOP" not in bc


# ---------------------------------------------------------------------------
# Controle de fluxo — instruções de salto
# ---------------------------------------------------------------------------

class TestBytecodeControlFlow:
    def test_if_sem_else_gera_jmpf(self, parse):
        bc = _bc(parse("int x = 1; if (x > 0) { x = 2; }")["stdout"])
        assert "JMPF" in bc

    def test_if_else_gera_jmp_e_jmpf(self, parse):
        bc = _bc(parse("int x = 1; if (x) { x = 1; } else { x = 2; }")["stdout"])
        assert "JMPF" in bc
        assert "JMP" in bc

    def test_while_gera_jmp_e_jmpf(self, parse):
        bc = _bc(parse("int i = 0; while (i < 3) { i = i + 1; }")["stdout"])
        assert "JMPF" in bc
        assert "JMP" in bc

    def test_for_gera_jmp_e_jmpf(self, parse):
        bc = _bc(parse("int n=0; for (int i=0; i<3; i=i+1) { n=n+1; }")["stdout"])
        assert "JMPF" in bc
        assert "JMP" in bc

    def test_jmp_offset_resolvido(self, parse):
        bc = _bc(parse("int x = 1; if (x) { x = 0; } else { x = 1; }")["stdout"])
        for line in bc.splitlines():
            line = line.strip()
            if line.startswith("JMP ") or line.startswith("JMPF "):
                offset = int(line.split()[-1])
                assert offset >= 0, f"offset não resolvido: {line}"


# ---------------------------------------------------------------------------
# Execução: saída da VM idêntica à antiga eval_ast()
# ---------------------------------------------------------------------------

class TestVMExecution:
    def test_decl_com_init_imprime_declarado(self, parse):
        r = parse("int x = 10;")
        assert "Declarado: x : int = 10" in r["stdout"]

    def test_decl_sem_init_imprime_declarado_sem_valor(self, parse):
        r = parse("int x;")
        assert "Declarado: x : int" in r["stdout"]

    def test_atribuicao_imprime_tipo_var_valor(self, parse):
        r = parse("int x = 0; x = 7;")
        assert "int x = 7" in r["stdout"]

    def test_expr_stmt_imprime_resultado(self, parse):
        r = parse("3 + 4;")
        assert "Resultado: 7" in r["stdout"]

    def test_expr_float_resultado(self, parse):
        r = parse("1.5 + 1.0;")
        assert "Resultado: 2.5" in r["stdout"]

    def test_while_executa_loop(self, parse):
        code = "int sum = 0; int i = 1; while (i < 4) { sum = sum + i; i = i + 1; }"
        r = parse(code)
        assert r["returncode"] == 0
        assert "Declarado: sum : int = 0" in r["stdout"]
        assert "int sum = 6" in r["stdout"]

    def test_if_else_ramo_verdadeiro(self, parse):
        r = parse("int x = 1; if (x > 0) { x = 99; } else { x = 0; }")
        assert "int x = 99" in r["stdout"]

    def test_if_else_ramo_falso(self, parse):
        r = parse("int x = 0; if (x > 0) { x = 99; } else { x = 42; }")
        assert "int x = 42" in r["stdout"]

    def test_for_executa_iteracoes(self, parse):
        code = "int n = 0; for (int i = 0; i < 3; i = i + 1) { n = n + 1; }"
        r = parse(code)
        assert r["returncode"] == 0
        assert "int n = 3" in r["stdout"]

    def test_bool_declarado(self, parse):
        r = parse("bool b = true;")
        assert "Declarado: b : bool = true" in r["stdout"]

    def test_char_declarado(self, parse):
        r = parse("char c = 'A';")
        assert "Declarado: c : char = 'A'" in r["stdout"]

    def test_float_declarado(self, parse):
        r = parse("float f = 3.14;")
        assert "Declarado: f : float = 3.14" in r["stdout"]
