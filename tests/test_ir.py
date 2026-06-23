"""Testes da geração de código intermediário (ir.c).

Alimenta o build/parser_exe com código-fonte C via stdin e
verifica o trecho de Código de Três Endereços (TAC) emitido
pela fase de geração de IR.

Formato da saída (ir.c):
  Cabeçalho      → "=== Código Intermediário (TAC) ==="
  Binária        → "<dest> = <a> <op> <b>"
  Unária         → "<dest> = <op><a>"
  Cópia/atrib.   → "<dest> = <a>"
  Rótulo         → "Lk:"
  Desvio incond. → "goto Lk"
  Desvio cond.   → "ifFalse <x> goto Lk"

Temporários são nomeados t0, t1, ...; rótulos L0, L1, ...
"""

import pytest


def _ir(stdout: str) -> str:
    """Extrai apenas o bloco TAC do stdout do parser_exe."""
    marker = "=== Código Intermediário (TAC) ==="
    assert marker in stdout, "bloco de IR ausente na saída"
    tail = stdout.split(marker, 1)[1]
    # O bloco termina na linha de '=' que fecha a seção.
    return tail.split("====", 1)[0]


# ---------------------------------------------------------------------------
# Presença e forma geral
# ---------------------------------------------------------------------------

class TestIRPresence:
    def test_cabecalho_presente(self, parse):
        r = parse("int x = 1;")
        assert "=== Código Intermediário (TAC) ===" in r["stdout"]

    def test_programa_vazio_nao_quebra(self, parse):
        # Programa vazio não tem AST → nenhuma seção de IR é impressa.
        r = parse("")
        assert r["returncode"] == 0


# ---------------------------------------------------------------------------
# Expressões → temporários
# ---------------------------------------------------------------------------

class TestIRExpressions:
    def test_literal_nao_gera_temporario(self, parse):
        # Uma expressão-comando com literal puro não emite instrução.
        ir = _ir(parse("42;")["stdout"])
        assert "t0" not in ir

    def test_binop_gera_temporario(self, parse):
        ir = _ir(parse("int x = 3 + 4;")["stdout"])
        assert "t0 = 3 + 4" in ir
        assert "x = t0" in ir

    def test_precedencia_ordena_temporarios(self, parse):
        # 2 + 3 * 4 → multiplicação primeiro
        ir = _ir(parse("2 + 3 * 4;")["stdout"])
        assert "t0 = 3 * 4" in ir
        assert "t1 = 2 + t0" in ir

    def test_constante_embutida_no_operando(self, parse):
        # Constantes não são materializadas em temporário próprio.
        ir = _ir(parse("int x = 5 * 2;")["stdout"])
        assert "t0 = 5 * 2" in ir

    def test_unario_negativo(self, parse):
        ir = _ir(parse("int x = 1; int y = -x;")["stdout"])
        assert "t0 = -x" in ir

    def test_unario_not(self, parse):
        ir = _ir(parse("!0;")["stdout"])
        assert "t0 = !0" in ir

    def test_operadores_relacionais_e_logicos(self, parse):
        ir = _ir(parse("(1 < 2) && (3 == 3);")["stdout"])
        assert "<" in ir and "==" in ir and "&&" in ir


# ---------------------------------------------------------------------------
# Atribuições e declarações → cópias
# ---------------------------------------------------------------------------

class TestIRCopies:
    def test_atribuicao_gera_copia(self, parse):
        ir = _ir(parse("int x = 0; x = 7;")["stdout"])
        assert "x = 7" in ir

    def test_decl_sem_init_nao_gera_codigo(self, parse):
        # Declaração sem inicialização não produz instrução de cópia.
        ir = _ir(parse("int x;")["stdout"])
        assert "x =" not in ir


# ---------------------------------------------------------------------------
# Controle de fluxo → rótulos e desvios
# ---------------------------------------------------------------------------

class TestIRControlFlow:
    def test_if_sem_else(self, parse):
        ir = _ir(parse("int x = 1; if (x > 0) { x = 2; }")["stdout"])
        assert "ifFalse" in ir
        assert "L0:" in ir

    def test_if_else_tem_goto(self, parse):
        ir = _ir(parse("if (1) { 1; } else { 2; }")["stdout"])
        assert "ifFalse" in ir
        assert "goto L" in ir

    def test_while_tem_rotulo_de_volta(self, parse):
        ir = _ir(parse("int i = 0; while (i < 3) { i = i + 1; }")["stdout"])
        # Deve haver um rótulo de início, um ifFalse e um goto de retorno.
        assert "ifFalse" in ir
        assert "goto L0" in ir
        assert "L0:" in ir
        assert "L1:" in ir

    def test_for_traduz_para_init_cond_step(self, parse):
        code = "int n = 0; for (int i = 0; i < 3; i = i + 1) { n = n + i; }"
        ir = _ir(parse(code)["stdout"])
        # init antes do rótulo de início
        assert "i = 0" in ir
        # condição testada com ifFalse
        assert "ifFalse" in ir
        # step e retorno ao topo
        assert "goto L0" in ir

    def test_for_partes_vazias(self, parse):
        code = "int x = 0; int i = 0; for (; i < 3;) { x = x + 1; i = i + 1; }"
        r = parse(code)
        assert r["returncode"] == 0
        ir = _ir(r["stdout"])
        assert "ifFalse" in ir

    def test_aninhamento_gera_rotulos_distintos(self, parse):
        code = "int i = 0; while (i < 2) { int j = 0; while (j < 2) { j = j + 1; } i = i + 1; }"
        ir = _ir(parse(code)["stdout"])
        # while externo (L0/L1) e interno (L2/L3)
        assert "L0:" in ir and "L1:" in ir
        assert "L2:" in ir and "L3:" in ir
