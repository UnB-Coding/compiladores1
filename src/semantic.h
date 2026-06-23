/******************************************************
 FGA0003 - Compiladores 1
 Curso de Engenharia de Software
 Universidade de Brasília (UnB)

 Arquivo: semantic.h
 Descrição: Análise Semântica da AST

 A análise semântica é executada APÓS o parsing e ANTES
 da execução (eval_ast). Percorre a AST inteira — incluindo
 todos os branches de if/else e corpos de loops — para
 detectar erros estáticos como:
   - Uso de variável não declarada
   - Redeclaração de variável
   - Divisão por zero com literais
   - Conversões implícitas com perda de precisão (warnings)
 ******************************************************/

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

/* ====================================================
 * analyze_ast: executa a análise semântica sobre a AST
 *
 * Parâmetro:
 *   root — raiz da AST (lista encadeada de statements)
 *
 * Retorno:
 *   0 se nenhum erro semântico foi encontrado.
 *   Número de erros se houve problemas.
 *
 * Nota: warnings (ex: conversão com perda de precisão)
 * são impressos em stderr mas NÃO contam como erros —
 * não impedem a execução.
 * ==================================================== */
int analyze_ast(ASTNode *root);

#endif /* SEMANTIC_H */
