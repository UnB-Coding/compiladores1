# test_integração.py — Testes de integração com expressões completas
# ===================================================================
#
# Estes testes verificam que o Lexer tokeniza corretamente expressões
# aritméticas completas, como as que o Parser vai receber.
# Aqui testamos a sequência INTEIRA de tokens, não apenas um tipo.

import pytest


class TestExpressoesCompletas:
    """Expressões aritméticas reais com a sequência completa de tokens."""

    def test_soma_simples(self, lexer):
        """1 + 2"""
        tokens = lexer.tokenize("1 + 2")
        resultado = [(t.type, t.lexeme) for t in tokens]
        assert resultado == [
            ("NUM", "1"), ("PLUS", "+"), ("NUM", "2"),
        ]

    def test_expressao_com_4_operacoes(self, lexer):
        """1 + 2 - 3 * 4 / 5"""
        tokens = lexer.tokenize("1 + 2 - 3 * 4 / 5")
        resultado = [(t.type, t.lexeme) for t in tokens]
        assert resultado == [
            ("NUM", "1"),  ("PLUS", "+"),
            ("NUM", "2"),  ("MINUS", "-"),
            ("NUM", "3"),  ("TIMES", "*"),
            ("NUM", "4"),  ("DIVIDE", "/"),
            ("NUM", "5"),
        ]

    def test_parenteses_simples(self, lexer):
        """(1 + 2) * 3"""
        tokens = lexer.tokenize("(1 + 2) * 3")
        resultado = [(t.type, t.lexeme) for t in tokens]
        assert resultado == [
            ("LPAREN", "("),
            ("NUM", "1"),  ("PLUS", "+"),  ("NUM", "2"),
            ("RPAREN", ")"),
            ("TIMES", "*"),
            ("NUM", "3"),
        ]

    def test_parenteses_aninhados(self, lexer):
        """((1 + 2) * (3 - 4))"""
        tokens = lexer.tokenize("((1 + 2) * (3 - 4))")
        resultado = [(t.type, t.lexeme) for t in tokens]
        assert resultado == [
            ("LPAREN", "("),
            ("LPAREN", "("),
            ("NUM", "1"), ("PLUS", "+"), ("NUM", "2"),
            ("RPAREN", ")"),
            ("TIMES", "*"),
            ("LPAREN", "("),
            ("NUM", "3"), ("MINUS", "-"), ("NUM", "4"),
            ("RPAREN", ")"),
            ("RPAREN", ")"),
        ]

    def test_expressao_longa(self, lexer):
        """10 + 20 * 30 - 40 / 5"""
        tokens = lexer.tokenize("10 + 20 * 30 - 40 / 5")
        tipos = [t.type for t in tokens]
        assert tipos == [
            "NUM", "PLUS", "NUM", "TIMES", "NUM",
            "MINUS", "NUM", "DIVIDE", "NUM",
        ]
        # Verifica os lexemas numéricos
        nums = [t.lexeme for t in tokens if t.type == "NUM"]
        assert nums == ["10", "20", "30", "40", "5"]

    def test_expressao_multiline(self, lexer):
        """Expressão distribuída em múltiplas linhas."""
        source = """
            100
            + 200
            * 3
        """
        tokens = lexer.tokenize(source)
        tipos = [t.type for t in tokens]
        assert tipos == ["NUM", "PLUS", "NUM", "TIMES", "NUM"]
        nums = [t.lexeme for t in tokens if t.type == "NUM"]
        assert nums == ["100", "200", "3"]


class TestContagemDeTokens:
    """Verifica a contagem total de tokens para entradas conhecidas."""

    @pytest.mark.parametrize("entrada, total_esperado", [
        ("42",              1),     # apenas NUM
        ("1+2",             3),     # NUM PLUS NUM
        ("(1)",             3),     # LPAREN NUM RPAREN
        ("(1+2)*3",         7),     # ( NUM + NUM ) * NUM
        ("",                0),     # vazio
        ("   ",             0),     # só espaços
        ("\n\t\n",          0),     # só whitespace
    ], ids=[
        "um_numero", "soma", "parenteses", "expressao",
        "vazio", "espacos", "whitespace",
    ])
    def test_contagem(self, lexer, entrada, total_esperado):
        """Número total de tokens deve bater."""
        tokens = lexer.tokenize(entrada)
        assert len(tokens) == total_esperado, (
            f"Para '{entrada!r}': esperava {total_esperado} tokens, "
            f"obteve {len(tokens)}: {[(t.type, t.lexeme) for t in tokens]}"
        )


class TestOrdemDosTokens:
    """Garante que a ordem dos tokens reflete a ordem da entrada."""

    def test_ordem_preservada(self, lexer):
        """Os tokens devem aparecer na mesma ordem que na entrada."""
        tokens = lexer.tokenize("9 - 8 + 7 * 6 / 5")
        lexemas = [t.lexeme for t in tokens]
        assert lexemas == ["9", "-", "8", "+", "7", "*", "6", "/", "5"]

    def test_parenteses_na_posicao_correta(self, lexer):
        """Parênteses devem aparecer nas posições corretas."""
        tokens = lexer.tokenize("(1) + (2)")
        tipos = [t.type for t in tokens]
        assert tipos == [
            "LPAREN", "NUM", "RPAREN",
            "PLUS",
            "LPAREN", "NUM", "RPAREN",
        ]
