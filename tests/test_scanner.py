"""Testes do scanner (scanner.l) — tokens usados pelo parser Bison.

Os nomes de token diferem do lexer standalone:
  scanner.l  →  T_INT, ID, PLUS, EQ, ...
  lexer.l    →  KW_INT, IDENTIFIER, OP_PLUS, OP_EQ, ...
"""

import pytest


# ---------------------------------------------------------------------------
# Palavras-chave de tipo
# ---------------------------------------------------------------------------

class TestTypeKeywords:
    def test_int(self, scan):
        tokens = scan("int")
        assert tokens[0]["type"] == "T_INT"

    def test_float(self, scan):
        tokens = scan("float")
        assert tokens[0]["type"] == "T_FLOAT"

    def test_char(self, scan):
        tokens = scan("char")
        assert tokens[0]["type"] == "T_CHAR"

    def test_bool(self, scan):
        tokens = scan("bool")
        assert tokens[0]["type"] == "T_BOOL"

    def test_todos_os_tipos(self, scan):
        tokens = scan("int float char bool")
        tipos = [t["type"] for t in tokens]
        assert tipos == ["T_INT", "T_FLOAT", "T_CHAR", "T_BOOL"]


# ---------------------------------------------------------------------------
# Palavras-chave de controle de fluxo
# ---------------------------------------------------------------------------

class TestControlFlowKeywords:
    def test_if_else(self, scan):
        tokens = scan("if else")
        assert tokens[0]["type"] == "KW_IF"
        assert tokens[1]["type"] == "KW_ELSE"

    def test_while(self, scan):
        tokens = scan("while")
        assert tokens[0]["type"] == "KW_WHILE"

    def test_for(self, scan):
        tokens = scan("for")
        assert tokens[0]["type"] == "KW_FOR"

    def test_keyword_prefix_vira_id(self, scan):
        # "iff" não é palavra-chave → deve ser ID
        tokens = scan("iff")
        assert tokens[0]["type"] == "ID"
        assert tokens[0]["value"] == "iff"

    def test_integer_nao_e_int(self, scan):
        # "integer" começa com "int" mas é identificador
        tokens = scan("integer")
        assert tokens[0]["type"] == "ID"


# ---------------------------------------------------------------------------
# Literais booleanos
# ---------------------------------------------------------------------------

class TestBoolLiterals:
    def test_true(self, scan):
        tokens = scan("true")
        assert tokens[0]["type"] == "TRUE_LIT"
        assert tokens[0]["value"] == "true"

    def test_false(self, scan):
        tokens = scan("false")
        assert tokens[0]["type"] == "FALSE_LIT"
        assert tokens[0]["value"] == "false"

    def test_truex_e_id(self, scan):
        tokens = scan("truex")
        assert tokens[0]["type"] == "ID"


# ---------------------------------------------------------------------------
# Identificadores
# ---------------------------------------------------------------------------

class TestIdentifiers:
    def test_simples(self, scan):
        tokens = scan("x")
        assert tokens[0]["type"] == "ID"
        assert tokens[0]["value"] == "x"

    def test_com_underscore(self, scan):
        tokens = scan("_var")
        assert tokens[0]["type"] == "ID"
        assert tokens[0]["value"] == "_var"

    def test_camel_case(self, scan):
        tokens = scan("myVar")
        assert tokens[0]["type"] == "ID"
        assert tokens[0]["value"] == "myVar"

    def test_com_digitos(self, scan):
        tokens = scan("var1")
        assert tokens[0]["type"] == "ID"
        assert tokens[0]["value"] == "var1"

    def test_nao_comeca_com_digito(self, scan):
        # "1x" → NUM seguido de ID
        tokens = scan("1x")
        assert tokens[0]["type"] == "NUM"
        assert tokens[1]["type"] == "ID"


# ---------------------------------------------------------------------------
# Literais numéricos
# ---------------------------------------------------------------------------

class TestNumericLiterals:
    def test_inteiro(self, scan):
        tokens = scan("42")
        assert tokens[0]["type"] == "NUM"
        assert tokens[0]["value"] == "42"

    def test_zero(self, scan):
        tokens = scan("0")
        assert tokens[0]["type"] == "NUM"
        assert tokens[0]["value"] == "0"

    def test_float_com_ponto(self, scan):
        tokens = scan("3.14")
        assert tokens[0]["type"] == "FLOAT_LIT"
        assert tokens[0]["value"] == "3.14"

    def test_float_so_decimal(self, scan):
        tokens = scan(".5")
        assert tokens[0]["type"] == "FLOAT_LIT"

    def test_float_sem_parte_decimal(self, scan):
        tokens = scan("2.")
        assert tokens[0]["type"] == "FLOAT_LIT"

    def test_float_antes_de_int(self, scan):
        # "3.14" deve ser FLOAT_LIT, não NUM + ID
        tokens = scan("3.14")
        assert len(tokens) == 1
        assert tokens[0]["type"] == "FLOAT_LIT"


# ---------------------------------------------------------------------------
# Literais de caractere
# ---------------------------------------------------------------------------

class TestCharLiterals:
    def test_char_simples(self, scan):
        tokens = scan("'a'")
        assert tokens[0]["type"] == "CHAR_LIT"

    def test_char_escape_newline(self, scan):
        tokens = scan(r"'\n'")
        assert tokens[0]["type"] == "CHAR_LIT"

    def test_char_escape_tab(self, scan):
        tokens = scan(r"'\t'")
        assert tokens[0]["type"] == "CHAR_LIT"

    def test_char_escape_backslash(self, scan):
        tokens = scan(r"'\\'")
        assert tokens[0]["type"] == "CHAR_LIT"


# ---------------------------------------------------------------------------
# Operadores aritméticos
# ---------------------------------------------------------------------------

class TestArithmeticOperators:
    def test_mais(self, scan):
        assert scan("+")[0]["type"] == "PLUS"

    def test_menos(self, scan):
        assert scan("-")[0]["type"] == "MINUS"

    def test_vezes(self, scan):
        assert scan("*")[0]["type"] == "TIMES"

    def test_divisao(self, scan):
        assert scan("/")[0]["type"] == "DIVIDE"

    def test_todos(self, scan):
        tokens = scan("+ - * /")
        assert [t["type"] for t in tokens] == ["PLUS", "MINUS", "TIMES", "DIVIDE"]


# ---------------------------------------------------------------------------
# Operadores relacionais e de igualdade
# ---------------------------------------------------------------------------

class TestRelationalOperators:
    def test_eq(self, scan):
        tokens = scan("==")
        assert len(tokens) == 1
        assert tokens[0]["type"] == "EQ"

    def test_ne(self, scan):
        assert scan("!=")[0]["type"] == "NE"

    def test_le(self, scan):
        assert scan("<=")[0]["type"] == "LE"

    def test_ge(self, scan):
        assert scan(">=")[0]["type"] == "GE"

    def test_lt(self, scan):
        assert scan("<")[0]["type"] == "LT"

    def test_gt(self, scan):
        assert scan(">")[0]["type"] == "GT"

    def test_eq_nao_confunde_com_assign(self, scan):
        tokens = scan("= ==")
        assert tokens[0]["type"] == "ASSIGN"
        assert tokens[1]["type"] == "EQ"

    def test_todos_relacionais(self, scan):
        tokens = scan("== != <= >= < >")
        assert [t["type"] for t in tokens] == ["EQ", "NE", "LE", "GE", "LT", "GT"]


# ---------------------------------------------------------------------------
# Operadores lógicos
# ---------------------------------------------------------------------------

class TestLogicalOperators:
    def test_and(self, scan):
        assert scan("&&")[0]["type"] == "AND"

    def test_or(self, scan):
        assert scan("||")[0]["type"] == "OR"

    def test_not(self, scan):
        assert scan("!")[0]["type"] == "NOT"

    def test_not_ne_distintos(self, scan):
        # "!" é NOT; "!=" é NE — não devem ser confundidos
        tokens = scan("! !=")
        assert tokens[0]["type"] == "NOT"
        assert tokens[1]["type"] == "NE"


# ---------------------------------------------------------------------------
# Atribuição e separadores
# ---------------------------------------------------------------------------

class TestAssignAndDelimiters:
    def test_assign(self, scan):
        assert scan("=")[0]["type"] == "ASSIGN"

    def test_semicolon(self, scan):
        assert scan(";")[0]["type"] == "SEMICOLON"

    def test_lparen_rparen(self, scan):
        tokens = scan("()")
        assert tokens[0]["type"] == "LPAREN"
        assert tokens[1]["type"] == "RPAREN"

    def test_lbrace_rbrace(self, scan):
        tokens = scan("{}")
        assert tokens[0]["type"] == "LBRACE"
        assert tokens[1]["type"] == "RBRACE"


# ---------------------------------------------------------------------------
# Whitespace ignorado
# ---------------------------------------------------------------------------

class TestWhitespace:
    def test_espacos_ignorados(self, scan):
        tokens = scan("int   float")
        assert len(tokens) == 2

    def test_tabs_ignorados(self, scan):
        assert len(scan("int\tfloat")) == 2

    def test_newlines_ignorados(self, scan):
        assert len(scan("int\nfloat")) == 2

    def test_entrada_vazia(self, scan):
        assert scan("") == []


# ---------------------------------------------------------------------------
# Sequências realistas
# ---------------------------------------------------------------------------

class TestRealisticSequences:
    def test_declaracao_inteira(self, scan):
        tokens = scan("int x = 5;")
        tipos = [t["type"] for t in tokens]
        assert tipos == ["T_INT", "ID", "ASSIGN", "NUM", "SEMICOLON"]
        assert tokens[1]["value"] == "x"
        assert tokens[3]["value"] == "5"

    def test_declaracao_float(self, scan):
        tokens = scan("float y = 3.14;")
        tipos = [t["type"] for t in tokens]
        assert tipos == ["T_FLOAT", "ID", "ASSIGN", "FLOAT_LIT", "SEMICOLON"]

    def test_condicional(self, scan):
        tokens = scan("if (x > 0)")
        tipos = [t["type"] for t in tokens]
        assert tipos == ["KW_IF", "LPAREN", "ID", "GT", "NUM", "RPAREN"]

    def test_while(self, scan):
        tokens = scan("while (i < 10)")
        tipos = [t["type"] for t in tokens]
        assert tipos == ["KW_WHILE", "LPAREN", "ID", "LT", "NUM", "RPAREN"]

    def test_for_cabecalho(self, scan):
        tokens = scan("for (i = 0; i < 3; i = i + 1)")
        tipos = [t["type"] for t in tokens]
        assert tipos[0] == "KW_FOR"
        assert "SEMICOLON" in tipos

    def test_bloco(self, scan):
        tokens = scan("{ int x = 1; }")
        assert tokens[0]["type"] == "LBRACE"
        assert tokens[-1]["type"] == "RBRACE"

    def test_bool_literal_em_expressao(self, scan):
        tokens = scan("bool b = true;")
        tipos = [t["type"] for t in tokens]
        assert tipos == ["T_BOOL", "ID", "ASSIGN", "TRUE_LIT", "SEMICOLON"]
