# Geração de Código Intermediário, Otimização e Execução (TAC)

Esta seção descreve as fases de **geração de código intermediário**, **otimização** e **execução**, implementadas em `src/ir.c` e `src/ir.h`.

Após a [análise semântica](guia_gramatica.md) validar a AST, o interpretador a percorre uma última vez para produzir uma **Representação Intermediária (IR)** na forma de **Código de Três Endereços** (*Three-Address Code*, TAC). Essa representação linear é independente de máquina e serve de base para as otimizações e para a execução final.

## Por que código intermediário?

A AST é uma estrutura em árvore, conveniente para análise mas pouco prática para otimizar. A IR a "achata" em uma **lista linear de instruções**, cada uma com no máximo três endereços (um destino e até dois operandos):

```
t0 = 3 * 4
t1 = 2 + t0
a  = t1
```

Esse formato é exatamente sobre o qual algoritmos clássicos de otimização operam.

## Posição no pipeline

```
Léxico (Flex) → Sintático (Bison) → AST
      → Análise Semântica (analyze_ast)
      → Geração de IR (gen_ir)        ← esta fase
      → Otimização (ir_optimize)      ← esta fase
      → Execução (ir_exec)            ← esta fase
```

## Modelo de instruções

Cada instrução é uma **quádrupla** (`IRInstr`) com um código de operação, um operador opcional e até três operandos:

| Opcode      | Forma impressa            | Significado                         |
|-------------|---------------------------|-------------------------------------|
| `IR_COPY`   | `x = y`                   | cópia/atribuição                    |
| `IR_BINOP`  | `x = y <op> z`            | operação binária                    |
| `IR_UNARYOP`| `x = <op>y`               | operação unária                     |
| `IR_LABEL`  | `Lk:`                     | define um rótulo                    |
| `IR_GOTO`   | `goto Lk`                 | desvio incondicional                |
| `IR_IFFALSE`| `ifFalse x goto Lk`       | desvia se `x` for falso (zero)      |
| `IR_DECL`   | `decl x : T`              | declaração de variável              |
| `IR_PRINT`  | `print x`                 | imprime resultado de `expr_stmt`    |

### Operandos

Um `IROperand` pode ser um **temporário** (`t0`, `t1`, …), uma **variável** nomeada, uma **constante embutida** (inteira, float, char ou bool) ou um **rótulo** (`L0`, `L1`, …).

Decisões de projeto que favorecem otimizações:

- **Constantes ficam embutidas** nos operandos em vez de materializadas em temporários, tornando o dobramento de constantes trivial.
- Cada operando **carrega seu `SymType`**, habilitando análises sensíveis a tipo. Como a geração ocorre antes da execução, o módulo mantém seu próprio *ambiente de tipos* (`name → SymType`), preenchido a partir das declarações.
- Cada temporário recebe **um único valor** (estilo *SSA-friendly*), simplificando a análise de fluxo de dados.

## Tradução do controle de fluxo

Comandos estruturados são traduzidos para rótulos e desvios.

### `if (cond) ... else ...`

```
    t0 = cond
    ifFalse t0 goto L0
    <then>
    goto L1
L0:
    <else>
L1:
```

(Sem `else`, o `goto L1` é omitido e o ramo falso cai direto em `L0`.)

### `while (cond) ...`

```
L0:
    t0 = cond
    ifFalse t0 goto L1
    <corpo>
    goto L0
L1:
```

### `for (init; cond; step) ...`

```
    <init>
L0:
    t0 = cond
    ifFalse t0 goto L1
    <corpo>
    <step>
    goto L0
L1:
```

Cada um dos três componentes do `for` é opcional; se ausente, sua parte correspondente é simplesmente omitida.

## Otimização do IR (`ir_optimize`)

O otimizador executa **6 passes** em loop de ponto fixo (repete enquanto algum passe produzir mudanças):

### Pass 1: Constant Folding
Resolve operações `BINOP`/`UNARYOP` cujos operandos são todos constantes, convertendo-as em `COPY` com o resultado pré-calculado.
```
    t0 = 10 + 20     →     t0 = 30
    t1 = !false      →     t1 = 1
```
Não dobra divisão por zero nem `INT_MIN / -1` (preserva o erro para a execução).

### Pass 2: Constant Propagation
Identifica temporários que receberam constantes (`tN = const`) e substitui todos os usos de `tN` pela constante direta.

### Pass 3: Dead Code Elimination (DCE)
- `ifFalse <const_false> goto Lk` → converte em `goto Lk` e marca código seguinte como morto.
- `ifFalse <const_true> goto Lk` → remove a instrução (condição sempre verdadeira, nunca desvia).
- Código após `goto` é removido até o próximo rótulo.

### Pass 4: Dead Temp Elimination
Remove definições de temporários (`tN = ...`) que nunca são lidos por nenhuma instrução. Surge tipicamente após a propagação de constantes deixar definições órfãs.

### Pass 5: Redundant Goto Elimination
Remove `goto Lk` imediatamente seguido de `Lk:` — um desvio para a instrução seguinte é desnecessário.

### Pass 6: Dead Label Elimination
Remove rótulos `Lk:` que não são referenciados por nenhum `goto` ou `ifFalse`. Surgem depois que o DCE e outras passes esvaziam blocos.

## Execução do IR (`ir_exec`)

O interpretador de IR executa o programa TAC (possivelmente otimizado):

1. **Linearização:** Converte a lista encadeada de instruções em um array indexável.
2. **Mapa de rótulos:** Constrói `label_map[Lk] = índice` para resolver desvios em O(1).
3. **Interpretação:** Percorre as instruções com um ponteiro de instrução (`ip`). Desvios alteram o `ip` para o índice do rótulo alvo.
4. **Valores:** Utiliza um array de `IRValue` (`{double val, SymType type}`) para temporários e a tabela de símbolos para variáveis nomeadas.

## Exemplo completo

Entrada:

```c
int n = 0;
for (int i = 0; i < 3; i = i + 1) {
    n = n + i;
}
```

TAC gerado:

```
    decl n : int (init)
    n = 0
    decl i : int (init)
    i = 0
L0:
    t0 = i < 3
    ifFalse t0 goto L1
    t1 = n + i
    n = t1
    t2 = i + 1
    i = t2
    goto L0
L1:
```

## Testes

Os testes em `tests/test_ir.py` alimentam o `parser_exe` com código-fonte e verificam o bloco TAC emitido — cobrindo expressões, precedência, cópias, declarações sem inicialização e os três tipos de controle de fluxo (incluindo aninhamento).

Os testes em `tests/test_ir_optimize.py` verificam as otimizações — cobrindo dobramento de constantes, propagação de constantes e eliminação de código morto, assegurando que o IR otimizado produz resultados corretos.
