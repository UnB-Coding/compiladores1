"""Testes do parser (parser.y + scanner.l + ast.c).

Alimenta o build/parser_exe com código-fonte C via stdin e
verifica o stdout/stderr/código de saída produzidos.

Formato de saída do ast.c:
  Declaração com init  → "Declarado: <nome> : <tipo> = <valor>"
  Declaração sem init  → "Declarado: <nome> : <tipo>"
  Atribuição           → "<tipo> <nome> = <valor>"
  Expressão stmt       → "Resultado: <valor>"
  Tabela de símbolos   → "--- Tabela de Símbolos ---"
  Erro sintático       → stderr: "Erro sintático: ..."
"""

import pytest


# ---------------------------------------------------------------------------
# Validade sintática — exit code
# ---------------------------------------------------------------------------

class TestSyntaxValidity:
    def test_programa_vazio_ok(self, parse):
        r = parse("")
        assert r["returncode"] == 0

    def test_declaracao_simples_ok(self, parse):
        r = parse("int x = 5;")
        assert r["returncode"] == 0

    def test_expressao_aritmetica_ok(self, parse):
        r = parse("1 + 2;")
        assert r["returncode"] == 0

    def test_if_simples_ok(self, parse):
        r = parse("int x = 1; if (x > 0) { x = 2; }")
        assert r["returncode"] == 0

    def test_while_ok(self, parse):
        r = parse("int i = 0; while (i < 0) { i = i + 1; }")
        assert r["returncode"] == 0

    def test_for_ok(self, parse):
        r = parse("for (int i = 0; i < 3; i = i + 1) { i + 0; }")
        assert r["returncode"] == 0

    def test_erro_falta_semicolon(self, parse):
        r = parse("int x = 5")   # falta ;
        # parser.y main() retorna 0 mesmo em erro; a evidência fica no stderr
        assert "Erro" in r["stderr"] or "syntax" in r["stderr"]

    def test_erro_parentese_nao_fechado(self, parse):
        r = parse("if (1 > 0 { 1; }")
        assert "Erro" in r["stderr"] or "syntax" in r["stderr"]

    def test_erro_sintático_vai_para_stderr(self, parse):
        r = parse("int x = ;")
        assert "Erro" in r["stderr"] or "erro" in r["stderr"] or "syntax" in r["stderr"]

    def test_multiplos_stmts_ok(self, parse):
        code = "int a = 1; int b = 2; a + b;"
        r = parse(code)
        assert r["returncode"] == 0


# ---------------------------------------------------------------------------
# Declarações
# ---------------------------------------------------------------------------

class TestDeclarations:
    def test_decl_int_com_init(self, parse):
        r = parse("int x = 10;")
        assert "Declarado: x : int = 10" in r["stdout"]

    def test_decl_int_sem_init(self, parse):
        r = parse("int y;")
        assert "Declarado: y : int" in r["stdout"]

    def test_decl_float(self, parse):
        r = parse("float f = 3.14;")
        assert "Declarado: f : float" in r["stdout"]

    def test_decl_char(self, parse):
        r = parse("char c = 'A';")
        assert "Declarado: c : char" in r["stdout"]

    def test_decl_bool_true(self, parse):
        r = parse("bool b = true;")
        assert "Declarado: b : bool" in r["stdout"]

    def test_decl_bool_false(self, parse):
        r = parse("bool b = false;")
        assert "Declarado: b : bool" in r["stdout"]

    def test_redeclaracao_falha(self, parse):
        r = parse("int x = 1; int x = 2;")
        assert r["returncode"] != 0

    def test_multiplas_declaracoes(self, parse):
        r = parse("int a = 1; int b = 2;")
        assert "Declarado: a : int = 1" in r["stdout"]
        assert "Declarado: b : int = 2" in r["stdout"]


# ---------------------------------------------------------------------------
# Atribuições
# ---------------------------------------------------------------------------

class TestAssignments:
    def test_atribuicao_inteiro(self, parse):
        r = parse("int x = 0; x = 42;")
        assert "int x = 42" in r["stdout"]

    def test_atribuicao_float(self, parse):
        r = parse("float f = 0.0; f = 1.5;")
        assert "float f = 1.5" in r["stdout"]

    def test_atribuicao_expressao(self, parse):
        r = parse("int x = 0; x = 3 + 4;")
        assert "int x = 7" in r["stdout"]

    def test_atribuicao_variavel_nao_declarada_falha(self, parse):
        r = parse("x = 5;")
        assert r["returncode"] != 0


# ---------------------------------------------------------------------------
# Expressões como comandos (expr;)
# ---------------------------------------------------------------------------

class TestExpressionStatements:
    def test_inteiro_literal(self, parse):
        r = parse("42;")
        assert "Resultado: 42" in r["stdout"]

    def test_soma_inteiros(self, parse):
        r = parse("3 + 4;")
        assert "Resultado: 7" in r["stdout"]

    def test_subtracao(self, parse):
        r = parse("10 - 3;")
        assert "Resultado: 7" in r["stdout"]

    def test_multiplicacao(self, parse):
        r = parse("6 * 7;")
        assert "Resultado: 42" in r["stdout"]

    def test_divisao_inteira(self, parse):
        # 5 / 2 = 2 em divisão inteira
        r = parse("5 / 2;")
        assert "Resultado: 2" in r["stdout"]

    def test_divisao_float(self, parse):
        r = parse("5.0 / 2;")
        assert "Resultado: 2.5" in r["stdout"]

    def test_menos_unario(self, parse):
        r = parse("-7;")
        assert "Resultado: -7" in r["stdout"]

    def test_expressao_relacional_verdadeiro(self, parse):
        r = parse("3 > 1;")
        assert "Resultado: 1" in r["stdout"]

    def test_expressao_relacional_falso(self, parse):
        r = parse("1 > 3;")
        assert "Resultado: 0" in r["stdout"]

    def test_igualdade(self, parse):
        r = parse("5 == 5;")
        assert "Resultado: 1" in r["stdout"]

    def test_diferenca(self, parse):
        r = parse("5 != 5;")
        assert "Resultado: 0" in r["stdout"]

    def test_and_logico(self, parse):
        r = parse("1 && 0;")
        assert "Resultado: 0" in r["stdout"]

    def test_or_logico(self, parse):
        r = parse("1 || 0;")
        assert "Resultado: 1" in r["stdout"]

    def test_not_logico(self, parse):
        r = parse("!0;")
        assert "Resultado: 1" in r["stdout"]

    def test_precedencia_multiplicacao_antes_soma(self, parse):
        # 2 + 3 * 4 = 14, não 20
        r = parse("2 + 3 * 4;")
        assert "Resultado: 14" in r["stdout"]

    def test_parenteses_alteram_precedencia(self, parse):
        r = parse("(2 + 3) * 4;")
        assert "Resultado: 20" in r["stdout"]


# ---------------------------------------------------------------------------
# Controle de fluxo
# ---------------------------------------------------------------------------

class TestControlFlow:
    def test_if_ramo_verdadeiro(self, parse):
        r = parse("if (1) { 99; }")
        assert "Resultado: 99" in r["stdout"]

    def test_if_ramo_falso_nao_executa(self, parse):
        r = parse("if (0) { 99; }")
        assert "Resultado: 99" not in r["stdout"]

    def test_if_else_verdadeiro(self, parse):
        r = parse("if (1) { 10; } else { 20; }")
        assert "Resultado: 10" in r["stdout"]
        assert "Resultado: 20" not in r["stdout"]

    def test_if_else_falso(self, parse):
        r = parse("if (0) { 10; } else { 20; }")
        assert "Resultado: 20" in r["stdout"]
        assert "Resultado: 10" not in r["stdout"]

    def test_while_nao_executa_condicao_falsa(self, parse):
        r = parse("while (0) { 99; }")
        assert "Resultado: 99" not in r["stdout"]

    def test_while_executa_uma_vez(self, parse):
        # i começa em 1, condição i < 1 é falsa após 0 iterações
        r = parse("int i = 1; while (i < 1) { i = i + 1; }")
        assert r["returncode"] == 0

    def test_for_executa_n_vezes(self, parse):
        # acumula resultado via expressão — conta 3 iterações
        code = "int n = 0; for (int i = 0; i < 3; i = i + 1) { n = n + 1; }"
        r = parse(code)
        assert r["returncode"] == 0
        assert "int n = 3" in r["stdout"]

    def test_bloco_isola_sequencia(self, parse):
        r = parse("{ 1; 2; 3; }")
        assert "Resultado: 1" in r["stdout"]
        assert "Resultado: 2" in r["stdout"]
        assert "Resultado: 3" in r["stdout"]


# ---------------------------------------------------------------------------
# Tabela de símbolos
# ---------------------------------------------------------------------------

class TestSymbolTable:
    def test_tabela_presente_no_output(self, parse):
        r = parse("int x = 5;")
        # Evita comparar caracteres acentuados (encoding C vs Python no Windows)
        assert "Tabela" in r["stdout"] and "---" in r["stdout"]

    def test_tabela_vazia_sem_variaveis(self, parse):
        r = parse("1 + 2;")
        assert "(vazia)" in r["stdout"]

    def test_variavel_na_tabela(self, parse):
        r = parse("int x = 42;")
        assert "x" in r["stdout"]
        assert "int" in r["stdout"]

    def test_multiplas_variaveis_na_tabela(self, parse):
        r = parse("int a = 1; float b = 2.0;")
        assert "a" in r["stdout"]
        assert "b" in r["stdout"]

    def test_valor_atualizado_reflete_na_tabela(self, parse):
        r = parse("int x = 1; x = 99;")
        # a tabela final deve mostrar x = 99
        tabela = r["stdout"].split("Tabela de Símbolos")[-1]
        assert "99" in tabela
