"""Testes de otimização do IR (ir_optimize em ir.c).

Verifica que constant folding, propagação de constantes e
eliminação de código morto operam corretamente sobre a IR.

Seções da saída do parser_exe:
  "=== Código Intermediário (TAC) ===" — IR bruto (não otimizado)
  "=== IR Otimizado ==="               — IR após ir_optimize()
"""

import pytest


def _ir_opt(stdout: str) -> str:
    """Extrai o bloco de IR otimizado do stdout do parser_exe."""
    marker = "=== IR Otimizado ==="
    assert marker in stdout, "seção 'IR Otimizado' ausente na saída"
    tail = stdout.split(marker, 1)[1]
    return tail.split("====", 1)[0]


def _ir_raw(stdout: str) -> str:
    """Extrai o bloco de IR bruto (não otimizado)."""
    marker = "=== Código Intermediário (TAC) ==="
    assert marker in stdout, "seção TAC ausente na saída"
    tail = stdout.split(marker, 1)[1]
    return tail.split("====", 1)[0]


# ---------------------------------------------------------------------------
# Presença da seção otimizada
# ---------------------------------------------------------------------------

class TestIROtimizacaoPresenca:
    def test_secao_presente(self, parse):
        r = parse("int x = 1;")
        assert "=== IR Otimizado ===" in r["stdout"]

    def test_programa_vazio_nao_quebra(self, parse):
        r = parse("")
        assert r["returncode"] == 0

    def test_programa_com_variaveis_ok(self, parse):
        r = parse("int a = 1; int b = 2;")
        assert r["returncode"] == 0
        assert "=== IR Otimizado ===" in r["stdout"]


# ---------------------------------------------------------------------------
# Constant Folding no IR
# ---------------------------------------------------------------------------

class TestConstantFoldingIR:
    def test_binop_inteiros_foldado(self, parse):
        # 3 + 4 → 7 no IR otimizado; sem BINOP
        r = parse("int x = 3 + 4;")
        opt = _ir_opt(r["stdout"])
        assert "x = 7" in opt
        assert "+" not in opt

    def test_binop_float_foldado(self, parse):
        r = parse("float f = 1.5 + 2.5;")
        opt = _ir_opt(r["stdout"])
        assert "4" in opt
        assert "+" not in opt

    def test_subtracao_foldada(self, parse):
        r = parse("int x = 10 - 3;")
        opt = _ir_opt(r["stdout"])
        assert "x = 7" in opt

    def test_multiplicacao_foldada(self, parse):
        r = parse("int x = 6 * 7;")
        opt = _ir_opt(r["stdout"])
        assert "x = 42" in opt

    def test_divisao_inteira_foldada(self, parse):
        r = parse("int x = 10 / 3;")
        opt = _ir_opt(r["stdout"])
        assert "x = 3" in opt

    def test_unario_negativo_foldado(self, parse):
        r = parse("int x = -5;")
        opt = _ir_opt(r["stdout"])
        assert "x = -5" in opt

    def test_unario_not_foldado(self, parse):
        r = parse("int x = 0; !0;")
        opt = _ir_opt(r["stdout"])
        assert "print 1" in opt

    def test_expr_stmt_binop_foldado(self, parse):
        # expr stmt com constantes — IR bruto tem BINOP, otimizado não
        r = parse("3 + 4;")
        raw = _ir_raw(r["stdout"])
        opt = _ir_opt(r["stdout"])
        assert "+" in raw       # IR bruto tem a operação
        assert "print 7" in opt  # IR otimizado imprime 7 diretamente

    def test_binop_com_variavel_nao_foldado(self, parse):
        # a + 4: a é variável → não pode foldar
        r = parse("int a = 3; int x = a + 4;")
        opt = _ir_opt(r["stdout"])
        assert "a + 4" in opt   # operação permanece no IR otimizado

    def test_relacional_foldado(self, parse):
        r = parse("int x = 3 > 1;")
        opt = _ir_opt(r["stdout"])
        assert "x = 1" in opt


# ---------------------------------------------------------------------------
# Propagação de Constantes no IR
# ---------------------------------------------------------------------------

class TestPropagacaoConstantesIR:
    def test_temp_propagado_para_copia(self, parse):
        # int x = 3 + 4: t0 = 7, x = t0 → x = 7 (t0 propagado)
        r = parse("int x = 3 + 4;")
        opt = _ir_opt(r["stdout"])
        assert "x = 7" in opt
        assert "x = t0" not in opt

    def test_temp_propagado_para_print(self, parse):
        # 3 + 4; → t0 = 7, print t0 → print 7
        r = parse("3 + 4;")
        opt = _ir_opt(r["stdout"])
        assert "print 7" in opt
        assert "print t0" not in opt

    def test_cadeia_de_propagacao(self, parse):
        # 2 + 3 * 4 → t0=12, t1=2+12=14; tudo propagado
        r = parse("2 + 3 * 4;")
        opt = _ir_opt(r["stdout"])
        assert "print 14" in opt


# ---------------------------------------------------------------------------
# Dead Code Elimination no IR
# ---------------------------------------------------------------------------

class TestDCEIR:
    def test_if_falso_vira_goto(self, parse):
        # if(0) → IR_IFFALSE(0) → IR_GOTO
        r = parse("if (0) { 99; }")
        opt = _ir_opt(r["stdout"])
        assert "goto" in opt
        assert "ifFalse" not in opt

    def test_if_falso_corpo_removido(self, parse):
        # corpo do if(0) é código morto após DCE
        r = parse("if (0) { 99; }")
        opt = _ir_opt(r["stdout"])
        assert "print 99" not in opt

    def test_if_falso_else_executado(self, parse):
        # if(0) else branch → else permanece, then removido
        r = parse("if (0) { 10; } else { 20; }")
        opt = _ir_opt(r["stdout"])
        assert "print 20" in opt
        assert "print 10" not in opt

    def test_while_falso_corpo_removido(self, parse):
        # while(0) → corpo é código morto
        r = parse("while (0) { 99; }")
        opt = _ir_opt(r["stdout"])
        assert "print 99" not in opt
        assert "goto" in opt


# ---------------------------------------------------------------------------
# Corretude de Execução após Otimização
# ---------------------------------------------------------------------------

class TestExecucaoAposOtimizacao:
    def test_decl_com_fold(self, parse):
        r = parse("int x = 3 + 4;")
        assert "Declarado: x : int = 7" in r["stdout"]

    def test_expr_stmt_resultado(self, parse):
        r = parse("3 + 4;")
        assert "Resultado: 7" in r["stdout"]

    def test_if_falso_nao_executa_corpo(self, parse):
        r = parse("if (0) { 99; }")
        assert "Resultado: 99" not in r["stdout"]

    def test_while_falso_nao_executa(self, parse):
        r = parse("while (0) { 1; }")
        assert "Resultado: 1" not in r["stdout"]

    def test_if_verdadeiro_fold_executa_then(self, parse):
        # if(1) → IFFALSE(1) removido, then executa
        r = parse("if (1) { 42; } else { 0; }")
        assert "Resultado: 42" in r["stdout"]
        assert "Resultado: 0" not in r["stdout"]

    def test_operacoes_encadeadas_foldadas(self, parse):
        r = parse("2 + 3 * 4;")
        assert "Resultado: 14" in r["stdout"]

    def test_fold_negativo(self, parse):
        r = parse("-7;")
        assert "Resultado: -7" in r["stdout"]

    def test_fold_not(self, parse):
        r = parse("!0;")
        assert "Resultado: 1" in r["stdout"]

    def test_decl_sem_init_nao_quebra(self, parse):
        r = parse("int x;")
        assert r["returncode"] == 0
        assert "Declarado: x : int" in r["stdout"]
