/******************************************************
 FGA0003 - Compiladores 1
 Curso de Engenharia de Software
 Universidade de Brasília (UnB)

 Arquivo: optimize.h
 Descrição: Otimização da AST (Constant Folding + Dead Code Elimination)

 A otimização é executada APÓS a análise semântica e ANTES
 da geração de código intermediário (IR). Percorre a AST em
 pós-ordem e aplica transformações até atingir um ponto fixo
 (nenhuma nova modificação em uma varredura completa).

 Otimizações implementadas:
   - Constant Folding: BINOP/UNARYOP com operandos literais
     são substituídos por um único nó literal.
   - Dead Code Elimination:
       AST_IF com condição literal conhecida → substitui pelo
       branch correto e descarta o outro.
       AST_WHILE(0) → removido por completo da lista.
 ******************************************************/

#ifndef OPTIMIZE_H
#define OPTIMIZE_H

#include "ast.h"

/* ====================================================
 * optimize_ast: ponto de entrada da fase de otimização.
 *
 * Parâmetro:
 *   root — raiz da AST (lista encadeada de statements).
 *          Pode ser NULL (programa vazio).
 *
 * Retorno:
 *   Nova raiz da AST (pode diferir da original se o
 *   primeiro statement for eliminado ou substituído).
 *
 * Nota: opera in-place, modificando nós existentes e
 *       liberando nós descartados com free_ast().
 * ==================================================== */
ASTNode *optimize_ast(ASTNode *root);

#endif /* OPTIMIZE_H */
