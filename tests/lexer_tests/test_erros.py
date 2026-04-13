# test_erros.py — Testes para caracteres não reconhecidos
# ========================================================
#
# O scanner.l define a regra catch-all:
#   .  → printf("Caractere não reconhecido: %s\n", yytext);
#
# Ou seja, qualquer caractere que não seja dígito, operador,
# parêntese ou whitespace vai disparar essa mensagem.
# O lexer NÃO para — ele imprime o aviso e continua.
#
# Nos testes, verificamos:
#   1. Que a mensagem de erro aparece na saída
#   2. Que os tokens válidos ao redor ainda são reconhecidos

import pytest


# -----------------------------------------------------------------------
# CARACTERES ILEGAIS ISOLADOS
# -----------------------------------------------------------------------
CARACTERES_ILEGAIS = [
    ("@",  "arroba"),
    ("#",  "cerquilha"),
    ("$",  "cifrão"),
    ("!",  "exclamação"),
    ("&",  "e-comercial"),
    ("=",  "igual"),
    ("%",  "porcentagem"),
    ("^",  "circunflexo"),
    ("?",  "interrogação"),
    ("a",  "letra minúscula"),
    ("Z",  "letra maiúscula"),
    ("_",  "underscore"),
    (";",  "ponto e vírgula"),
    (",",  "vírgula"),
    (".",  "ponto"),
]


@pytest.mark.parametrize(
    "char, descricao",
    CARACTERES_ILEGAIS,
    ids=[c[1] for c in CARACTERES_ILEGAIS],
)
def test_caractere_nao_reconhecido(lexer, char, descricao):
    """Caractere ilegal deve gerar mensagem 'Caractere não reconhecido'."""
    tokens, saida = lexer.tokenize_with_stderr(char)
    assert "não reconhecido" in saida or "nao reconhecido" in saida, (
        f"[{descricao}] Esperava mensagem de erro para '{char}', "
        f"mas a saída foi: {saida!r}"
    )


# -----------------------------------------------------------------------
# CARACTERES ILEGAIS MISTURADOS COM VÁLIDOS
# -----------------------------------------------------------------------
def test_erro_nao_interrompe_lexer(lexer):
    """O lexer deve continuar após encontrar caractere ilegal.

    Entrada: '1 @ 2' → deve gerar NUM para 1 e 2,
    e reportar erro para @.
    """
    tokens, saida = lexer.tokenize_with_stderr("1 @ 2")
    nums = [t for t in tokens if t.type == "NUM"]
    assert len(nums) == 2, "Lexer parou após erro — deveria continuar"
    assert nums[0].lexeme == "1"
    assert nums[1].lexeme == "2"
    assert "não reconhecido" in saida or "nao reconhecido" in saida


def test_multiplos_erros(lexer):
    """Múltiplos caracteres ilegais geram múltiplas mensagens."""
    tokens, saida = lexer.tokenize_with_stderr("@#$")
    count = saida.lower().count("não reconhecido") + saida.lower().count("nao reconhecido")
    assert count >= 3, (
        f"Esperava pelo menos 3 mensagens de erro, encontrou {count}. "
        f"Saída: {saida!r}"
    )


def test_erro_entre_expressao_valida(lexer):
    """'1+a+2' → NUM PLUS [erro] PLUS NUM."""
    tokens, saida = lexer.tokenize_with_stderr("1+a+2")
    tipos = [t.type for t in tokens]
    # O 'a' gera erro mas os tokens válidos continuam
    assert "NUM" in tipos
    assert "PLUS" in tipos
    assert "não reconhecido" in saida or "nao reconhecido" in saida


# -----------------------------------------------------------------------
# ROBUSTEZ — O lexer não deve crashar
# -----------------------------------------------------------------------
class TestRobustez:

    def test_entrada_muito_longa(self, lexer):
        """Entrada com muitos tokens não deve causar crash."""
        # 1000 somas: 1+1+1+...+1
        expr = "+".join(["1"] * 1000)
        tokens = lexer.tokenize(expr)
        nums = [t for t in tokens if t.type == "NUM"]
        assert len(nums) == 1000

    def test_numero_muito_grande(self, lexer):
        """Número com muitos dígitos não deve causar crash."""
        big = "9" * 100
        tokens = lexer.tokenize(big)
        assert len(tokens) == 1
        assert tokens[0].type == "NUM"
        assert tokens[0].lexeme == big

    def test_muitos_parenteses(self, lexer):
        """Muitos parênteses aninhados."""
        expr = "(" * 100 + "1" + ")" * 100
        tokens = lexer.tokenize(expr)
        lparens = [t for t in tokens if t.type == "LPAREN"]
        rparens = [t for t in tokens if t.type == "RPAREN"]
        nums = [t for t in tokens if t.type == "NUM"]
        assert len(lparens) == 100
        assert len(rparens) == 100
        assert len(nums) == 1
