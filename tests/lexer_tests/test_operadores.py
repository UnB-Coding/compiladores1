# test_operadores.py — Testes para operadores (+, -, *, /) e parênteses
# ======================================================================
#
# O scanner.l define:
#   "+"  → PLUS
#   "-"  → MINUS
#   "*"  → TIMES
#   "/"  → DIVIDE
#   "("  → LPAREN
#   ")"  → RPAREN
#
# Cada operador é um único caractere, então não há ambiguidade de
# maximal munch neste lexer (diferente de um lexer C completo).
# Mas precisamos garantir que cada um é reconhecido corretamente.

import pytest


# -----------------------------------------------------------------------
# OPERADORES INDIVIDUAIS
# -----------------------------------------------------------------------
OPERADORES = [
    ("+",  "PLUS",    "soma"),
    ("-",  "MINUS",   "subtração"),
    ("*",  "TIMES",   "multiplicação"),
    ("/",  "DIVIDE",  "divisão"),
]


@pytest.mark.parametrize(
    "simbolo, tipo_esperado, descricao",
    OPERADORES,
    ids=[c[2] for c in OPERADORES],
)
def test_operador_individual(lexer, simbolo, tipo_esperado, descricao):
    """Cada operador isolado deve gerar um token do tipo correto."""
    tokens = lexer.tokenize(simbolo)
    assert len(tokens) == 1, (
        f"[{descricao}] Esperava 1 token para '{simbolo}', obteve {len(tokens)}"
    )
    assert tokens[0].type == tipo_esperado
    assert tokens[0].lexeme == simbolo


# -----------------------------------------------------------------------
# PARÊNTESES
# -----------------------------------------------------------------------
PARENTESES = [
    ("(",  "LPAREN",  "abre parênteses"),
    (")",  "RPAREN",  "fecha parênteses"),
]


@pytest.mark.parametrize(
    "simbolo, tipo_esperado, descricao",
    PARENTESES,
    ids=[c[2] for c in PARENTESES],
)
def test_parenteses(lexer, simbolo, tipo_esperado, descricao):
    """Parênteses devem ser reconhecidos como LPAREN/RPAREN."""
    tokens = lexer.tokenize(simbolo)
    assert len(tokens) == 1
    assert tokens[0].type == tipo_esperado
    assert tokens[0].lexeme == simbolo


# -----------------------------------------------------------------------
# OPERADORES EM SEQUÊNCIA (sem espaço)
# -----------------------------------------------------------------------
def test_operadores_consecutivos(lexer):
    """'+-*/' deve gerar 4 tokens, um para cada operador."""
    tokens = lexer.tokenize("+-*/")
    assert len(tokens) == 4
    tipos = [t.type for t in tokens]
    assert tipos == ["PLUS", "MINUS", "TIMES", "DIVIDE"]


def test_parenteses_aninhados(lexer):
    """'(())' deve gerar LPAREN LPAREN RPAREN RPAREN."""
    tokens = lexer.tokenize("(())")
    assert len(tokens) == 4
    tipos = [t.type for t in tokens]
    assert tipos == ["LPAREN", "LPAREN", "RPAREN", "RPAREN"]


# -----------------------------------------------------------------------
# OPERADORES ENTRE NÚMEROS (contexto real)
# -----------------------------------------------------------------------
@pytest.mark.parametrize("expr, tipos_esperados", [
    ("1+2",     ["NUM", "PLUS", "NUM"]),
    ("10-3",    ["NUM", "MINUS", "NUM"]),
    ("4*5",     ["NUM", "TIMES", "NUM"]),
    ("8/2",     ["NUM", "DIVIDE", "NUM"]),
], ids=["soma", "subtracao", "multiplicacao", "divisao"])
def test_operador_entre_numeros(lexer, expr, tipos_esperados):
    """Expressão binária deve gerar a sequência NUM OP NUM."""
    tokens = lexer.tokenize(expr)
    tipos = [t.type for t in tokens]
    assert tipos == tipos_esperados
