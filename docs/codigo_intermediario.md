# Geração de Código Intermediário (TAC)

Esta seção descreve a fase de **geração de código intermediário**, implementada em `src/ir.c` e `src/ir.h`.

Após a [análise semântica](explicacao_arquivos.md) validar a AST, o compilador a percorre uma última vez para produzir uma **Representação Intermediária (IR)** na forma de **Código de Três Endereços** (*Three-Address Code*, TAC). Essa representação linear é independente de máquina e serve de **base para futuras otimizações** e para a posterior geração de código final.

## Por que código intermediário?

A AST é uma estrutura em árvore, conveniente para análise mas pouco prática para otimizar. A IR a "achata" em uma **lista linear de instruções**, cada uma com no máximo três endereços (um destino e até dois operandos):

```
t0 = 3 * 4
t1 = 2 + t0
a  = t1
```

Esse formato é exatamente sobre o qual algoritmos clássicos de otimização operam:

- **Dobramento de constantes** (*constant folding*) — `t0 = 3 * 4` vira `t0 = 12`.
- **Propagação de cópias** (*copy propagation*) — eliminar atribuições redundantes.
- **Eliminação de código morto** (*dead-code elimination*).
- **Análise de fluxo de dados** e **alocação de registradores**.

## Posição no pipeline

```
Léxico (Flex) → Sintático (Bison) → AST
      → Análise Semântica
      → Geração de IR (TAC)   ← esta fase
      → Execução (eval_ast)
```

A geração ocorre **antes** da execução e é totalmente independente dela: nenhum valor é calculado e nenhuma I/O acontece. A função `gen_ir()` retorna um `IRProgram*` que pode ser impresso (`ir_print()`) e liberado (`ir_free()`).

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

### Operandos

Um `IROperand` pode ser um **temporário** (`t0`, `t1`, …), uma **variável** nomeada, uma **constante embutida** (inteira, float, char ou bool) ou um **rótulo** (`L0`, `L1`, …).

Decisões de projeto que favorecem otimizações futuras:

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
    n = 0
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
