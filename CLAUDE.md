# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

An educational **interpreter** for a small subset of C, built for the UnB course *FGA0003 — Compiladores 1*. It uses **Flex** (scanner) + **Bison** (parser) to build an AST, runs a semantic pass over it, lowers the AST to a three-address-code (TAC) IR, optimizes that IR, and interprets it. Documentation and code comments are in Portuguese; keep that convention. Commits follow Conventional Commits (`feat`, `fix`, `docs`, `refactor`, `test`, `chore`, `build`) with scopes like `lexer`, `parser`, `tests`, `build`, `docs`.

## Build & run

Requires `flex` ≥ 2.6, `bison` ≥ 3.0, `gcc`, `make`. All generated artifacts (`*.tab.c/h`, `*.yy.c`, executables) land in `build/`.

```bash
make                 # build all executables into build/
make clean           # remove generated sources and executables
./build/parser_exe   # the interpreter; reads source from stdin
echo 'int x = 1+2;' | ./build/parser_exe
```

`parser_exe` (the real interpreter, and the only executable) is built from `src/parser.y` + `src/scanner.l` + `symtab.c` + `ast.c` + `semantic.c` + `ir.c`. It does **not** link `-lfl` because `parser.y` supplies `main()` and `scanner.l` supplies `yywrap()`. (A legacy standalone lexer — `examples/lexer.l` plus its `lexer_exe`/`lex` test machinery — was retired; don't reintroduce references to it.)

## Tests

Tests are **pytest** drivers that compile the C executables (via `tests/conftest.py` fixtures) and feed them source over stdin, asserting on stdout/stderr. `conftest.py` regenerates and compiles the needed binaries on first use into `build/`.

```bash
python3 -m pytest                          # full suite (config in pytest.ini)
python3 -m pytest tests/test_parser.py     # one file
python3 -m pytest tests/test_ir.py::TestX::test_y   # one test
pip install -r requirements-test.txt       # pytest + pytest-cov + pytest-html
```

`pytest.ini` enables coverage by default (HTML report to `build/coverage_html`). Test backends and their fixtures:
- `scan` fixture → `src/scanner.l` tokens (Bison token names: `T_INT`, `ID`, `PLUS`, …).
- `parse` fixture → full `parser_exe`, returns `{stdout, stderr, returncode}`.

The C test main `tests/scanner_test_main.c` prints one `TOKEN <type> <value>` line per token; the `scan` fixture parses that format.

## Architecture

Pipeline, each phase consuming the previous one's output:

```
scanner.l (Flex) → parser.y (Bison) → AST → semantic.c → ir.c (TAC) → ir_optimize → ir_exec
                                              symtab.c (symbol table)
```

`main()` lives in `src/parser.y`: it calls `yyparse()` (which builds the AST), then prints the AST, runs semantic analysis, lowers it to TAC (printed before and after optimization), executes the IR via `ir_exec()`, prints the symbol table, and frees memory.

**Strict separation of parse and execute.** Bison semantic actions *only* build AST nodes — no computation, I/O, or symbol-table access happens during parsing. This is the key design invariant; preserve it when adding phases.

- **`src/ast.h` / `ast.c`** — The AST is a tagged union (`ASTNode.kind` selects the active `data` member). `ast.c` holds only node constructors, `print_ast()`, and `free_ast()` — **no execution** (the old AST-walking evaluator `eval_ast` was retired; the IR is the sole execution path, so don't reintroduce a second engine). Statements are chained via an **intrusive `next` pointer** on the struct base, so any node can be a list element (`line_list`, block `stmt_list`). Traversal helpers process a *single* node and do **not** follow `->next`; list iteration is the caller's job (`gen_list()` in `ir.c`, `analyze_list()` in `semantic.c`). List building uses `append_node()` which is O(n²). Operators are encoded as `int` char codes with a mixed convention: ASCII for `+ - * / < > ! & |`, and mnemonics for the rest — `'E'`==`==`, `'N'`==`!=`, `'l'`==`<=`, `'g'`==`>=`.

- **Execution semantics (`ir_exec` in `ir.c`)** — Values are carried as a `double` (the universal internal type) plus a `SymType` tag. Note these deliberate divergences from C: single global scope (blocks don't create lexical scope), **no short-circuit** evaluation of `&&`/`||`, division-by-zero (int *and* float) is a fatal runtime error, and declarations/assignments/free expressions auto-print results (educational convenience). Because `ir_exec` runs the *optimized* IR, this auto-printed output already reflects constant folding and dead-code elimination.

- **`src/semantic.h` / `semantic.c`** — `analyze_ast()` walks the whole tree (including all branches and loop bodies) before execution, returning error count (0 = ok). Detects undeclared use, redeclaration, literal division-by-zero; emits warnings (e.g. lossy conversions) to stderr without counting them as errors.

- **`src/ir.h` / `ir.c`** — Lowering of the AST to **three-address code** as a flat linked list of quadruples (`IRInstr`); this is the **sole execution path**. `gen_ir()` builds it, `ir_optimize()` runs constant folding + constant propagation + dead-code elimination to a fixed point in-place, `ir_exec()` interprets it, `ir_print()` dumps TAC. Operands (`IROperand`) embed constants directly and carry `SymType`, deliberately to keep optimizations simple.

- **`symbol_table/symtab.h` / `symtab.c`** — Fixed-size hash table, 211 buckets, djb2 hash, external chaining, single global scope. Values stored in a `SymValue` union (`iVal` for int/bool, `fVal` for float, `cVal` for char), discriminated by the entry's `type`.

**Supported language subset:** types `int float char bool`; `if/else`, `while`, `for`, blocks; arithmetic/relational/logical operators. **Not supported:** functions, arrays, pointers, strings, comments, preprocessor, `switch`/`do-while`, `break`/`continue`/`return`, `++`/`--`, compound assignment, bitwise ops, multi-variable declarations.

## Docs

Detailed technical docs live in `docs/` (MkDocs site, `mkdocs.yml`); `docs/arquitetura_interpretador.md` is the authoritative deep-dive. The GitHub Actions workflow only deploys the MkDocs site to GitHub Pages — there is no CI for building or testing the interpreter.
