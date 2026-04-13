# test_numeros.py — Testes para o token NUM (literais inteiros)
# ==============================================================
#
# O scanner.l define a regra: [0-9]+  → retorna NUM
# Isso significa que qualquer sequência de dígitos deve virar NUM.
#
# Edge cases:
#   - Zero: "0"
#   - Número grande: "999999999"
#   - Múltiplos números separados por operadores: "1+2"
#   - Números colados em parênteses: "(42)"

import pytest


# -----------------------------------------------------------------------
# NÚMEROS SIMPLES
# -----------------------------------------------------------------------
NUMEROS_VALIDOS = [
    ("0",            "0",            "zero"),
    ("1",            "1",            "dígito único"),
    ("42",           "42",           "dois dígitos"),
    ("123",          "123",          "três dígitos"),
    ("999999",       "999999",       "número grande"),
    ("007",          "007",          "zeros à esquerda"),
    ("1234567890",   "1234567890",   "todos os dígitos"),
]


@pytest.mark.parametrize(
    "entrada, lexema_esperado, descricao",
    NUMEROS_VALIDOS,
    ids=[c[2] for c in NUMEROS_VALIDOS],
)
def test_numero_simples(lexer, entrada, lexema_esperado, descricao):
    """Cada sequência de dígitos deve gerar exatamente um token NUM."""
    tokens = lexer.tokenize(entrada)
    assert len(tokens) == 1, (
        f"[{descricao}] Esperava 1 token, obteve {len(tokens)}: {tokens}"
    )
    assert tokens[0].type == "NUM"
    assert tokens[0].lexeme == lexema_esperado


# -----------------------------------------------------------------------
# MÚLTIPLOS NÚMEROS NA MESMA ENTRADA
# -----------------------------------------------------------------------
def test_dois_numeros_separados_por_espaco(lexer):
    """'10 20' deve gerar dois tokens NUM."""
    tokens = lexer.tokenize("10 20")
    nums = [t for t in tokens if t.type == "NUM"]
    assert len(nums) == 2
    assert nums[0].lexeme == "10"
    assert nums[1].lexeme == "20"


def test_numeros_separados_por_operador(lexer):
    """'3+4' deve gerar NUM PLUS NUM."""
    tokens = lexer.tokenize("3+4")
    assert len(tokens) == 3
    assert tokens[0] == tokens[0].__class__(type="NUM", lexeme="3")
    assert tokens[1].type == "PLUS"
    assert tokens[2] == tokens[2].__class__(type="NUM", lexeme="4")


def test_numero_entre_parenteses(lexer):
    """'(42)' deve gerar LPAREN NUM RPAREN."""
    tokens = lexer.tokenize("(42)")
    assert len(tokens) == 3
    assert tokens[0].type == "LPAREN"
    assert tokens[1].type == "NUM"
    assert tokens[1].lexeme == "42"
    assert tokens[2].type == "RPAREN"
