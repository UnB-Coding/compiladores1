"""Testes de funcionamento do lexer standalone (lexer/lexer.l).

Cada teste passa uma string para o binário build/lexer_test_exe via stdin
e verifica os tokens produzidos.
"""

import pytest


# ---------------------------------------------------------------------------
# Palavras-chave de tipo
# ---------------------------------------------------------------------------

class TestKeywords:
    def test_kw_int(self, lex):
        tokens = lex("int")
        assert len(tokens) == 1
        assert tokens[0]["type"] == "KW_INT"
        assert tokens[0]["value"] == "int"

    def test_kw_float(self, lex):
        tokens = lex("float")
        assert tokens[0]["type"] == "KW_FLOAT"

    def test_kw_char(self, lex):
        tokens = lex("char")
        assert tokens[0]["type"] == "KW_CHAR"

    def test_kw_long(self, lex):
        tokens = lex("long")
        assert tokens[0]["type"] == "KW_LONG"

    def test_kw_double(self, lex):
        tokens = lex("double")
        assert tokens[0]["type"] == "KW_DOUBLE"

    def test_kw_short(self, lex):
        tokens = lex("short")
        assert tokens[0]["type"] == "KW_SHORT"

    def test_all_type_keywords(self, lex):
        tokens = lex("int float char long double short signed unsigned")
        types = [t["type"] for t in tokens]
        assert types == [
            "KW_INT", "KW_FLOAT", "KW_CHAR", "KW_LONG",
            "KW_DOUBLE", "KW_SHORT", "KW_SIGNED", "KW_UNSIGNED",
        ]


# ---------------------------------------------------------------------------
# Palavras-chave de controle de fluxo
# ---------------------------------------------------------------------------

class TestControlFlowKeywords:
    def test_if_else(self, lex):
        tokens = lex("if else")
        assert tokens[0]["type"] == "KW_IF"
        assert tokens[1]["type"] == "KW_ELSE"

    def test_while_for_do(self, lex):
        tokens = lex("while for do")
        types = [t["type"] for t in tokens]
        assert types == ["KW_WHILE", "KW_FOR", "KW_DO"]

    def test_switch_case_break(self, lex):
        tokens = lex("switch case break continue return")
        types = [t["type"] for t in tokens]
        assert types == ["KW_SWITCH", "KW_CASE", "KW_BREAK", "KW_CONTINUE", "KW_RETURN"]


# ---------------------------------------------------------------------------
# Identificadores
# ---------------------------------------------------------------------------

class TestIdentifiers:
    def test_simple(self, lex):
        tokens = lex("foo")
        assert tokens[0]["type"] == "IDENTIFIER"
        assert tokens[0]["value"] == "foo"

    def test_with_underscore_prefix(self, lex):
        tokens = lex("_bar")
        assert tokens[0]["type"] == "IDENTIFIER"
        assert tokens[0]["value"] == "bar" or tokens[0]["value"] == "_bar"

    def test_underscore_prefix_value(self, lex):
        tokens = lex("_myVar")
        assert tokens[0]["type"] == "IDENTIFIER"
        assert tokens[0]["value"] == "_myVar"

    def test_mixed_case(self, lex):
        tokens = lex("camelCase")
        assert tokens[0]["type"] == "IDENTIFIER"
        assert tokens[0]["value"] == "camelCase"

    def test_with_digits(self, lex):
        tokens = lex("var1")
        assert tokens[0]["type"] == "IDENTIFIER"
        assert tokens[0]["value"] == "var1"

    def test_not_starting_with_digit(self, lex):
        # "1var" deve produzir LIT_INT seguido de IDENTIFIER
        tokens = lex("1var")
        assert tokens[0]["type"] == "LIT_INT"
        assert tokens[1]["type"] == "IDENTIFIER"

    def test_keyword_prefix_is_identifier(self, lex):
        # "iff" não é palavra-chave, deve ser IDENTIFIER
        tokens = lex("iff")
        assert tokens[0]["type"] == "IDENTIFIER"
        assert tokens[0]["value"] == "iff"


# ---------------------------------------------------------------------------
# Literais numéricos
# ---------------------------------------------------------------------------

class TestLiterals:
    def test_integer_literal(self, lex):
        tokens = lex("42")
        assert tokens[0]["type"] == "LIT_INT"
        assert tokens[0]["value"] == "42"

    def test_integer_zero(self, lex):
        tokens = lex("0")
        assert tokens[0]["type"] == "LIT_INT"

    def test_float_literal(self, lex):
        tokens = lex("3.14")
        assert tokens[0]["type"] == "LIT_FLOAT"
        assert tokens[0]["value"] == "3.14"

    def test_float_scientific(self, lex):
        tokens = lex("1e10")
        assert tokens[0]["type"] == "LIT_FLOAT"

    def test_float_scientific_with_decimal(self, lex):
        tokens = lex("1.5e-3")
        assert tokens[0]["type"] == "LIT_FLOAT"

    def test_string_literal(self, lex):
        tokens = lex('"hello"')
        assert tokens[0]["type"] == "LIT_STRING"
        assert tokens[0]["value"] == '"hello"'

    def test_char_literal(self, lex):
        tokens = lex("'a'")
        assert tokens[0]["type"] == "LIT_CHAR"
        assert tokens[0]["value"] == "'a'"


# ---------------------------------------------------------------------------
# Operadores
# ---------------------------------------------------------------------------

class TestOperators:
    def test_arithmetic(self, lex):
        tokens = lex("+ - * /")
        types = [t["type"] for t in tokens]
        assert types == ["OP_PLUS", "OP_MINUS", "OP_MULT", "OP_DIV"]

    def test_assign(self, lex):
        tokens = lex("=")
        assert tokens[0]["type"] == "OP_ASSIGN"

    def test_relational_single(self, lex):
        tokens = lex("< >")
        assert tokens[0]["type"] == "OP_LT"
        assert tokens[1]["type"] == "OP_GT"

    def test_relational_double(self, lex):
        tokens = lex("== != <= >=")
        types = [t["type"] for t in tokens]
        assert types == ["OP_EQ", "OP_NE", "OP_LE", "OP_GE"]

    def test_logical(self, lex):
        tokens = lex("&& || !")
        types = [t["type"] for t in tokens]
        assert types == ["OP_AND", "OP_OR", "OP_NOT"]

    def test_eq_not_confused_with_assign(self, lex):
        tokens = lex("==")
        assert len(tokens) == 1
        assert tokens[0]["type"] == "OP_EQ"


# ---------------------------------------------------------------------------
# Delimitadores
# ---------------------------------------------------------------------------

class TestDelimiters:
    def test_braces(self, lex):
        tokens = lex("{ }")
        assert tokens[0]["type"] == "LBRACE"
        assert tokens[1]["type"] == "RBRACE"

    def test_parens(self, lex):
        tokens = lex("( )")
        assert tokens[0]["type"] == "LPAREN"
        assert tokens[1]["type"] == "RPAREN"

    def test_brackets(self, lex):
        tokens = lex("[ ]")
        assert tokens[0]["type"] == "LBRACKET"
        assert tokens[1]["type"] == "RBRACKET"

    def test_semicolon_and_comma(self, lex):
        tokens = lex("; ,")
        assert tokens[0]["type"] == "SEMICOLON"
        assert tokens[1]["type"] == "COMMA"


# ---------------------------------------------------------------------------
# Whitespace e comentários
# ---------------------------------------------------------------------------

class TestWhitespaceAndComments:
    def test_spaces_ignored(self, lex):
        tokens = lex("int   float")
        assert len(tokens) == 2

    def test_tabs_ignored(self, lex):
        tokens = lex("int\tfloat")
        assert len(tokens) == 2

    def test_newlines_ignored(self, lex):
        tokens = lex("int\nfloat")
        assert len(tokens) == 2

    def test_line_comment_ignored(self, lex):
        tokens = lex("int // isso é um comentário\nfloat")
        types = [t["type"] for t in tokens]
        assert types == ["KW_INT", "KW_FLOAT"]

    def test_block_comment_ignored(self, lex):
        tokens = lex("int /* bloco */ float")
        types = [t["type"] for t in tokens]
        assert types == ["KW_INT", "KW_FLOAT"]

    def test_empty_input(self, lex):
        tokens = lex("")
        assert tokens == []


# ---------------------------------------------------------------------------
# Sequência realista (trecho de código C)
# ---------------------------------------------------------------------------

class TestRealisticInput:
    def test_variable_declaration(self, lex):
        tokens = lex("int x = 10;")
        types = [t["type"] for t in tokens]
        assert types == ["KW_INT", "IDENTIFIER", "OP_ASSIGN", "LIT_INT", "SEMICOLON"]
        assert tokens[1]["value"] == "x"
        assert tokens[3]["value"] == "10"

    def test_if_condition(self, lex):
        tokens = lex("if (x > 0)")
        types = [t["type"] for t in tokens]
        assert types == ["KW_IF", "LPAREN", "IDENTIFIER", "OP_GT", "LIT_INT", "RPAREN"]

    def test_function_signature(self, lex):
        tokens = lex("int main(void)")
        types = [t["type"] for t in tokens]
        assert types == ["KW_INT", "IDENTIFIER", "LPAREN", "IDENTIFIER", "RPAREN"]

    def test_return_statement(self, lex):
        tokens = lex("return 0;")
        types = [t["type"] for t in tokens]
        assert types == ["KW_RETURN", "LIT_INT", "SEMICOLON"]
