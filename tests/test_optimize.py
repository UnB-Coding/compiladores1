"""Testes da fase de otimização da AST (optimize.c) — ciclo TDD.

A otimização ocorre APÓS a análise semântica e ANTES da geração de IR.
Os efeitos são observados em dois pontos da saída de parser_exe:

  1. "=== AST Otimizada ===" — estrutura da árvore após constant folding/DCE.
  2. "=== Código Intermediário (TAC) ===" — IR gerada sobre a AST já otimizada.

Constant Folding:  nós BINOP/UNARYOP com filhos literais são dobrados em
                   um único literal antes de chegar ao gerador de IR.
Dead Code Elim.:   AST_IF com condição literal e AST_WHILE(0) são removidos
                   antes de gerar qualquer instrução de desvio.
"""

import pytest


def _opt_ast(stdout: str) -> str:
    """Extrai o bloco 'AST Otimizada' do stdout."""
    marker = "=== AST Otimizada ==="
    assert marker in stdout, "bloco 'AST Otimizada' ausente na saída"
    tail = stdout.split(marker, 1)[1]
    return tail.split("====", 1)[0]


def _ir(stdout: str) -> str:
    """Extrai o bloco TAC do stdout."""
    marker = "=== Código Intermediário (TAC) ==="
    assert marker in stdout, "bloco de IR ausente na saída"
    tail = stdout.split(marker, 1)[1]
    return tail.split("====", 1)[0]


# ---------------------------------------------------------------------------
# Presença da fase de otimização
# ---------------------------------------------------------------------------

class TestOptimizePresence:
    def test_cabecalho_presente(self, parse):
        r = parse("int x = 1;")
        assert r["returncode"] == 0
        assert "=== AST Otimizada ===" in r["stdout"]

    def test_programa_vazio_nao_quebra(self, parse):
        r = parse("")
        assert r["returncode"] == 0


# ---------------------------------------------------------------------------
# Constant Folding — via AST otimizada (estrutura da árvore)
# ---------------------------------------------------------------------------

class TestConstantFoldingAST:
    def test_soma_inteiros_dobrada(self, parse):
        r = parse("int x = 3 + 4;")
        ast = _opt_ast(r["stdout"])
        assert "NUM (7)" in ast
        assert "BINOP" not in ast

    def test_multiplicacao_dobrada(self, parse):
        r = parse("int x = 2 * 5;")
        ast = _opt_ast(r["stdout"])
        assert "NUM (10)" in ast
        assert "BINOP" not in ast

    def test_subtracao_dobrada(self, parse):
        r = parse("int x = 10 - 3;")
        ast = _opt_ast(r["stdout"])
        assert "NUM (7)" in ast
        assert "BINOP" not in ast

    def test_divisao_inteira_dobrada(self, parse):
        r = parse("int x = 9 / 3;")
        ast = _opt_ast(r["stdout"])
        assert "NUM (3)" in ast
        assert "BINOP" not in ast

    def test_multiplicacao_por_zero_dobrada(self, parse):
        r = parse("int x = 42 * 0;")
        ast = _opt_ast(r["stdout"])
        assert "NUM (0)" in ast
        assert "BINOP" not in ast

    def test_encadeamento_dobrado(self, parse):
        # 2 + 3 * 4 → 2 + 12 → 14
        r = parse("int x = 2 + 3 * 4;")
        ast = _opt_ast(r["stdout"])
        assert "NUM (14)" in ast
        assert "BINOP" not in ast

    def test_unario_negativo_dobrado(self, parse):
        r = parse("int x = -7;")
        ast = _opt_ast(r["stdout"])
        assert "NUM (-7)" in ast
        assert "UNARYOP" not in ast

    def test_not_literal_dobrado(self, parse):
        r = parse("int x = !0;")
        ast = _opt_ast(r["stdout"])
        assert "UNARYOP" not in ast

    def test_relacional_literal_dobrado(self, parse):
        r = parse("int x = 3 < 5;")
        ast = _opt_ast(r["stdout"])
        assert "BINOP" not in ast
        assert "NUM (1)" in ast

    def test_igualdade_literal_dobrada(self, parse):
        r = parse("int x = 4 == 4;")
        ast = _opt_ast(r["stdout"])
        assert "BINOP" not in ast
        assert "NUM (1)" in ast

    def test_float_literal_dobrado(self, parse):
        r = parse("float x = 1.5 + 2.5;")
        ast = _opt_ast(r["stdout"])
        assert "BINOP" not in ast


# ---------------------------------------------------------------------------
# Constant Folding — via IR (TAC simplificado)
# ---------------------------------------------------------------------------

class TestConstantFoldingIR:
    def test_soma_sem_temporario(self, parse):
        r = parse("int x = 3 + 4;")
        ir = _ir(r["stdout"])
        assert "t0" not in ir
        assert "x = 7" in ir

    def test_encadeamento_sem_temporarios(self, parse):
        r = parse("int x = 2 + 3 * 4;")
        ir = _ir(r["stdout"])
        assert "t0" not in ir
        assert "x = 14" in ir

    def test_unario_sem_temporario(self, parse):
        r = parse("int x = -5;")
        ir = _ir(r["stdout"])
        assert "t0" not in ir
        assert "x = -5" in ir

    def test_resultado_execucao_correto_apos_folding(self, parse):
        r = parse("int x = 2 + 3;")
        assert r["returncode"] == 0
        assert "Declarado: x : int = 5" in r["stdout"]

    def test_resultado_encadeamento_correto(self, parse):
        r = parse("int x = 2 + 3 * 4;")
        assert r["returncode"] == 0
        assert "Declarado: x : int = 14" in r["stdout"]

    def test_variavel_nao_dobrada(self, parse):
        # x + 1 não pode ser dobrado (x não é literal)
        r = parse("int x = 5; int y = x + 1;")
        ir = _ir(r["stdout"])
        assert "t0" in ir  # temporário necessário

    def test_expressao_livre_dobrada(self, parse):
        # expressão-statement com literais
        r = parse("3 + 4;")
        ir = _ir(r["stdout"])
        # Após folding, expr_stmt(NUM(7)) não gera instrução
        assert "t0" not in ir


# ---------------------------------------------------------------------------
# Dead Code Elimination — via IR
# ---------------------------------------------------------------------------

class TestDeadCodeEliminationIR:
    def test_if_false_eliminado(self, parse):
        r = parse("int x = 0; if (0) { x = 99; }")
        assert r["returncode"] == 0
        ir = _ir(r["stdout"])
        assert "ifFalse" not in ir
        assert "x = 99" not in ir

    def test_if_true_else_eliminado(self, parse):
        r = parse("int x = 0; if (1) { x = 1; } else { x = 99; }")
        assert r["returncode"] == 0
        ir = _ir(r["stdout"])
        assert "ifFalse" not in ir
        assert "x = 99" not in ir
        assert "x = 1" in ir

    def test_if_false_else_mantido(self, parse):
        r = parse("int x = 0; if (0) { x = 99; } else { x = 42; }")
        assert r["returncode"] == 0
        ir = _ir(r["stdout"])
        assert "ifFalse" not in ir
        assert "x = 99" not in ir
        assert "x = 42" in ir

    def test_while_false_eliminado(self, parse):
        r = parse("int x = 0; while (0) { x = x + 1; }")
        assert r["returncode"] == 0
        ir = _ir(r["stdout"])
        assert "ifFalse" not in ir
        assert "L0" not in ir

    def test_codigo_apos_while_false_executado(self, parse):
        # O código depois do while eliminado deve continuar executando
        r = parse("int x = 5; while (0) { x = 99; } int y = x + 1;")
        assert r["returncode"] == 0
        assert "Declarado: y : int = 6" in r["stdout"]

    def test_if_false_sem_else_eliminado_por_completo(self, parse):
        r = parse("int x = 0; if (0) { x = 1; }")
        assert r["returncode"] == 0
        ir = _ir(r["stdout"])
        assert "ifFalse" not in ir


# ---------------------------------------------------------------------------
# Dead Code Elimination — via AST otimizada
# ---------------------------------------------------------------------------

class TestDeadCodeEliminationAST:
    def test_if_false_removido_da_ast(self, parse):
        r = parse("int x = 0; if (0) { x = 99; }")
        ast = _opt_ast(r["stdout"])
        assert "IF" not in ast
        assert "x = 99" not in ast

    def test_while_false_removido_da_ast(self, parse):
        r = parse("int x = 0; while (0) { x = x + 1; }")
        ast = _opt_ast(r["stdout"])
        assert "WHILE" not in ast

    def test_if_true_preservado_sem_else(self, parse):
        r = parse("int x = 0; if (1) { x = 5; }")
        ast = _opt_ast(r["stdout"])
        assert "IF" not in ast
        assert "ASSIGN (x)" in ast


# ---------------------------------------------------------------------------
# Correção após otimização (regressão)
# ---------------------------------------------------------------------------

class TestCorrecaoAposOtimizacao:
    def test_execucao_correta_com_variaveis(self, parse):
        r = parse("int a = 3; int b = 4; int c = a + b;")
        assert r["returncode"] == 0
        assert "Declarado: c : int = 7" in r["stdout"]

    def test_loop_real_nao_eliminado(self, parse):
        r = parse("int i = 0; while (i < 3) { i = i + 1; }")
        assert r["returncode"] == 0
        ir = _ir(r["stdout"])
        assert "ifFalse" in ir  # while real deve manter desvio

    def test_if_com_variavel_nao_eliminado(self, parse):
        r = parse("int x = 1; if (x > 0) { x = 2; }")
        assert r["returncode"] == 0
        ir = _ir(r["stdout"])
        assert "ifFalse" in ir

    def test_folding_nao_afeta_tabela_simbolos(self, parse):
        r = parse("int x = 2 + 3;")
        assert r["returncode"] == 0
        assert "x : int = 5" in r["stdout"]

    def test_programa_complexo_correto(self, parse):
        code = """
        int n = 10;
        int sum = 0;
        int i = 0;
        while (i < n) {
            sum = sum + i;
            i = i + 1;
        }
        """
        r = parse(code)
        assert r["returncode"] == 0
        assert "sum : int = 45" in r["stdout"]
