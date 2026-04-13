/******************************************************
 * lexer_driver.c — Driver para testar o Lexer isoladamente
 *
 * Em vez de chamar yyparse() (que aciona o parser),
 * este programa chama yylex() em loop e imprime cada
 * token no formato:
 *
 *     <TIPO, "lexema">
 *
 * Isso permite que o pytest capture a saída e valide.
 ******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "build/parser.tab.h"   /* NUM, PLUS, MINUS, etc. */

/* Variáveis externas do Flex */
extern int yylex(void);
extern char *yytext;
extern FILE *yyin;

/* yylval é a união definida no parser.y */
YYSTYPE yylval;

/* Mapeia o ID numérico do token para o nome em string */
static const char *token_name(int tok) {
    switch (tok) {
        case NUM:     return "NUM";
        case PLUS:    return "PLUS";
        case MINUS:   return "MINUS";
        case TIMES:   return "TIMES";
        case DIVIDE:  return "DIVIDE";
        case LPAREN:  return "LPAREN";
        case RPAREN:  return "RPAREN";
        default:      return "UNKNOWN";
    }
}

int main(int argc, char *argv[]) {
    /* Se receber um arquivo como argumento, usa ele; senão, usa stdin */
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fprintf(stderr, "Erro: não foi possível abrir '%s'\n", argv[1]);
            return 1;
        }
    }

    int tok;
    while ((tok = yylex()) != 0) {
        printf("<%s, \"%s\">\n", token_name(tok), yytext);
    }

    if (yyin && yyin != stdin) {
        fclose(yyin);
    }

    return 0;
}