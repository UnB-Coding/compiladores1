/*
 * Driver de teste para o scanner (scanner.l + parser.tab.h).
 * Compila junto com build/lex.yy.c (sem parser.tab.c) e imprime
 * um token por linha no formato:
 *   TOKEN <TIPO> <LEXEMA>
 * Usado pelos testes pytest via subprocess.
 */

#include <stdio.h>
#include "parser.tab.h"   /* -Ibuild/  durante a compilação */

/* yylval é declarado extern em parser.tab.h; definimos aqui
 * já que não linkamos com parser.tab.c.                     */
YYSTYPE yylval;

extern int   yylex(void);
extern char *yytext;

static const char *token_name(int tok) {
    switch (tok) {
        /* Literais */
        case NUM:       return "NUM";
        case FLOAT_LIT: return "FLOAT_LIT";
        case CHAR_LIT:  return "CHAR_LIT";
        case TRUE_LIT:  return "TRUE_LIT";
        case FALSE_LIT: return "FALSE_LIT";

        /* Identificador */
        case ID:        return "ID";

        /* Tipos */
        case T_INT:     return "T_INT";
        case T_FLOAT:   return "T_FLOAT";
        case T_CHAR:    return "T_CHAR";
        case T_BOOL:    return "T_BOOL";

        /* Controle de fluxo */
        case KW_IF:     return "KW_IF";
        case KW_ELSE:   return "KW_ELSE";
        case KW_WHILE:  return "KW_WHILE";
        case KW_FOR:    return "KW_FOR";

        /* Operadores aritméticos */
        case PLUS:      return "PLUS";
        case MINUS:     return "MINUS";
        case TIMES:     return "TIMES";
        case DIVIDE:    return "DIVIDE";

        /* Atribuição e separador */
        case ASSIGN:    return "ASSIGN";
        case SEMICOLON: return "SEMICOLON";

        /* Parênteses e blocos */
        case LPAREN:    return "LPAREN";
        case RPAREN:    return "RPAREN";
        case LBRACE:    return "LBRACE";
        case RBRACE:    return "RBRACE";

        /* Relacionais */
        case EQ:        return "EQ";
        case NE:        return "NE";
        case LT:        return "LT";
        case GT:        return "GT";
        case LE:        return "LE";
        case GE:        return "GE";

        /* Lógicos */
        case AND:       return "AND";
        case OR:        return "OR";
        case NOT:       return "NOT";

        default:        return "UNKNOWN";
    }
}

int main(void) {
    int tok;
    while ((tok = yylex()) != 0)
        printf("TOKEN %s %s\n", token_name(tok), yytext);
    return 0;
}
