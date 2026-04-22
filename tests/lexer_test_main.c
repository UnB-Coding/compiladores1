/*
 * Driver de teste para o lexer standalone.
 * Compila junto com build/lexer.yy.c (sem -lfl) e imprime um token por linha:
 *   TOKEN <TIPO> <LEXEMA>
 * Usado pelos testes pytest via subprocess.
 */

#include <stdio.h>

/* Mesmo enum definido em lexer/lexer.l */
enum {
    KW_INT = 256, KW_FLOAT, KW_CHAR, KW_LONG,
    KW_DOUBLE, KW_SHORT, KW_SIGNED, KW_UNSIGNED,

    KW_IF, KW_ELSE, KW_WHILE, KW_FOR, KW_DO,
    KW_SWITCH, KW_CASE, KW_BREAK, KW_CONTINUE,
    KW_RETURN,

    LIT_INT, LIT_FLOAT, LIT_STRING, LIT_CHAR,

    IDENTIFIER,

    OP_PLUS, OP_MINUS, OP_MULT, OP_DIV,

    OP_ASSIGN,

    OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE,

    OP_AND, OP_OR, OP_NOT,

    LBRACE, RBRACE, LPAREN, RPAREN,
    LBRACKET, RBRACKET, SEMICOLON, COMMA
};

extern int yylex(void);
extern char *yytext;

static const char *token_name(int tok) {
    switch (tok) {
        case KW_INT:      return "KW_INT";
        case KW_FLOAT:    return "KW_FLOAT";
        case KW_CHAR:     return "KW_CHAR";
        case KW_LONG:     return "KW_LONG";
        case KW_DOUBLE:   return "KW_DOUBLE";
        case KW_SHORT:    return "KW_SHORT";
        case KW_SIGNED:   return "KW_SIGNED";
        case KW_UNSIGNED: return "KW_UNSIGNED";
        case KW_IF:       return "KW_IF";
        case KW_ELSE:     return "KW_ELSE";
        case KW_WHILE:    return "KW_WHILE";
        case KW_FOR:      return "KW_FOR";
        case KW_DO:       return "KW_DO";
        case KW_SWITCH:   return "KW_SWITCH";
        case KW_CASE:     return "KW_CASE";
        case KW_BREAK:    return "KW_BREAK";
        case KW_CONTINUE: return "KW_CONTINUE";
        case KW_RETURN:   return "KW_RETURN";
        case LIT_INT:     return "LIT_INT";
        case LIT_FLOAT:   return "LIT_FLOAT";
        case LIT_STRING:  return "LIT_STRING";
        case LIT_CHAR:    return "LIT_CHAR";
        case IDENTIFIER:  return "IDENTIFIER";
        case OP_PLUS:     return "OP_PLUS";
        case OP_MINUS:    return "OP_MINUS";
        case OP_MULT:     return "OP_MULT";
        case OP_DIV:      return "OP_DIV";
        case OP_ASSIGN:   return "OP_ASSIGN";
        case OP_EQ:       return "OP_EQ";
        case OP_NE:       return "OP_NE";
        case OP_LT:       return "OP_LT";
        case OP_GT:       return "OP_GT";
        case OP_LE:       return "OP_LE";
        case OP_GE:       return "OP_GE";
        case OP_AND:      return "OP_AND";
        case OP_OR:       return "OP_OR";
        case OP_NOT:      return "OP_NOT";
        case LBRACE:      return "LBRACE";
        case RBRACE:      return "RBRACE";
        case LPAREN:      return "LPAREN";
        case RPAREN:      return "RPAREN";
        case LBRACKET:    return "LBRACKET";
        case RBRACKET:    return "RBRACKET";
        case SEMICOLON:   return "SEMICOLON";
        case COMMA:       return "COMMA";
        default:          return "UNKNOWN";
    }
}

int main(void) {
    int tok;
    while ((tok = yylex()) != 0) {
        printf("TOKEN %s %s\n", token_name(tok), yytext);
    }
    return 0;
}
