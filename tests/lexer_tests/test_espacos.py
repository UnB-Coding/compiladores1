# test_espacos.py — Testes para tratamento de espaços em branco
# ==============================================================
#
# O scanner.l define:
#   [ \t\n]+  → { /* não faz nada */ }
#
# Ou seja: espaços, tabs e newlines devem ser IGNORADOS.
# Os tokens resultantes devem ser os mesmos independente de
# quanto whitespace existe entre eles.

import pytest


class TestEspacosBasicos:
    """Whitespace não deve gerar tokens."""

    def test_apenas_espacos(self, lexer):
        """Entrada só com espaços não gera nenhum token."""
        tokens = lexer.tokenize("   ")
        assert len(tokens) == 0

    def test_apenas_tabs(self, lexer):
        """Entrada só com tabs não gera nenhum token."""
        tokens = lexer.tokenize("\t\t\t")
        assert len(tokens) == 0

    def test_apenas_newlines(self, lexer):
        """Entrada só com quebras de linha não gera nenhum token."""
        tokens = lexer.tokenize("\n\n\n")
        assert len(tokens) == 0

    def test_mistura_de_whitespace(self, lexer):
        """Mistura de espaço + tab + newline não gera tokens."""
        tokens = lexer.tokenize("  \t\n  \t\n  ")
        assert len(tokens) == 0

    def test_entrada_vazia(self, lexer):
        """String vazia não gera tokens."""
        tokens = lexer.tokenize("")
        assert len(tokens) == 0


class TestEspacosEntreTokens:
    """Whitespace variado entre tokens não muda o resultado."""

    def test_um_espaco(self, lexer):
        """'1 + 2' com um espaço entre cada."""
        tokens = lexer.tokenize("1 + 2")
        tipos = [t.type for t in tokens]
        assert tipos == ["NUM", "PLUS", "NUM"]

    def test_muitos_espacos(self, lexer):
        """'1    +    2' com muitos espaços."""
        tokens = lexer.tokenize("1    +    2")
        tipos = [t.type for t in tokens]
        assert tipos == ["NUM", "PLUS", "NUM"]

    def test_tabs_como_separador(self, lexer):
        """Tabs entre tokens."""
        tokens = lexer.tokenize("1\t+\t2")
        tipos = [t.type for t in tokens]
        assert tipos == ["NUM", "PLUS", "NUM"]

    def test_newlines_como_separador(self, lexer):
        """Newlines entre tokens."""
        tokens = lexer.tokenize("1\n+\n2")
        tipos = [t.type for t in tokens]
        assert tipos == ["NUM", "PLUS", "NUM"]

    def test_sem_espacos(self, lexer):
        """'1+2' colado — deve funcionar igual."""
        tokens = lexer.tokenize("1+2")
        tipos = [t.type for t in tokens]
        assert tipos == ["NUM", "PLUS", "NUM"]

    def test_whitespace_no_inicio_e_fim(self, lexer):
        """Espaços antes e depois da expressão são ignorados."""
        tokens = lexer.tokenize("  \t 42 \n ")
        assert len(tokens) == 1
        assert tokens[0].type == "NUM"
        assert tokens[0].lexeme == "42"


class TestEquivalencia:
    """Expressões iguais com whitespace diferente devem gerar os mesmos tokens."""

    @pytest.mark.parametrize("expressao", [
        "1+2*3",
        "1 + 2 * 3",
        "1  +  2  *  3",
        "1\t+\t2\t*\t3",
        "1\n+\n2\n*\n3",
        "  1 + 2 * 3  ",
    ], ids=["colado", "1espaco", "2espacos", "tabs", "newlines", "bordas"])
    def test_mesma_expressao_whitespace_variado(self, lexer, expressao):
        """Todas as variações devem gerar a mesma sequência de tokens."""
        tokens = lexer.tokenize(expressao)
        tipos_lexemas = [(t.type, t.lexeme) for t in tokens]
        esperado = [
            ("NUM", "1"), ("PLUS", "+"), ("NUM", "2"),
            ("TIMES", "*"), ("NUM", "3"),
        ]
        assert tipos_lexemas == esperado
