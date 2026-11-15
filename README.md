# 🔥 PhoenixC

**A self-healing Mini C compiler that rises from its own errors.**

PhoenixC is a from-scratch compiler for a subset of C, built with Flex and Bison. It runs every classic compiler phase (lexing, parsing, semantic analysis, three-address-code generation, optimization) and emits machine-readable diagnostics. When a program fails to compile, the **Phoenix Healer** passes those diagnostics to a local LLM (via [Ollama](https://ollama.com)), applies the suggested fix, and recompiles until the code builds or the attempts run out.

```
 source.c ──▶ Lexer ──▶ Parser ──▶ AST ──▶ Semantic ──▶ TAC ──▶ Optimizer ──▶ output.tac
   (Flex)      (Bison LALR(1))        analysis      gen        (-O)
                                         │
                                  errors.json ──▶ 🔥 Phoenix Healer (Ollama) ──┐
                                         ▲                                     │
                                         └────────── recompile fixed code ◀────┘
```

## Features

| Phase | What it does |
|---|---|
| **Lexical analysis** | Flex scanner: keywords, identifiers, int/float literals, operators, `//` and `/* */` comments, line and column tracking |
| **Syntax analysis** | Bison LALR(1) grammar with **zero conflicts** (dangling `else` resolved through precedence) that builds a full AST |
| **Semantic analysis** | Scoped symbol tables, a function table with **forward references**, parameter binding, **arity and argument-type checks**, return-type checks, implicit-narrowing (`float → int`) errors, use-before-init warnings, `/ 0` warnings |
| **Code generation** | Three-address code with `func … / endfunc` frames, labels, conditional jumps, `param` and `call` |
| **Optimizer (`-O`)** | Constant folding, algebraic simplification (`x+0`, `x*1`, `x*0`), constant and copy propagation, dead-temporary elimination |
| **Diagnostics** | `errors.json` lists each error with its type, line, column and message, for tools to read |
| **Self-healing** | Iterative LLM repair loop that runs locally through Ollama (no API keys) and keeps a full `healing_log.json` |

### Supported language

- Types: `int`, `float`, `void`
- Operators: `+ - * / %`, unary `-`, `== != < > <= >=`, `=`
- Control flow: `if` / `else`, `while`, `for`
- Functions with parameters, calls, and `return`

## Quick start

**Requirements:** a C compiler, `make`, `flex`, `bison` (plus Python 3 and Ollama for healing).

```bash
# macOS:  xcode-select --install && brew install flex bison
# Ubuntu: sudo apt-get install build-essential flex bison

make -C compiler          # builds compiler/phoenixc
./run_all_tests.sh        # 14 tests: 9 valid programs, 5 error cases
```

### Compile a program

```bash
./compiler/phoenixc input/test_for_loop.c --print-tac --output output
./compiler/phoenixc input/test_optimizer.c -O --print-tac
./compiler/phoenixc --help
```

### Heal a broken program

```bash
ollama pull codellama && ollama serve &
pip install -r healer/requirements.txt
python3 healer/healer.py input/test_with_errors.c --max-iterations 3
```

The healer writes each attempt (`*_attempt_N.c`), the final `*_corrected.c`, and `healing_log.json` to `healed_output/`. To use another model, pass `--model deepseek-coder`. To point at a different compiler binary, set `PHOENIXC=/path/to/phoenixc`.

## Example

**Input** (`input/test_optimizer.c`):

```c
int main() {
    int seconds = 60 * 60 * 24;
    int offset = -5 + 2 * 3;
    int x = seconds * 1 + 0;
    int y = (x - offset) * 0;
    ...
}
```

**`phoenixc -O --print-tac`**:

```
func main:
  seconds = 86400
  offset = 1
  x = seconds
  y = 0
  ...
endfunc
```

**Diagnostics** (`input/error_function_calls.c`):

```
[TYPE_ERROR]       Line 6:  void function 'log_value' should not return a value
[SEMANTIC_ERROR]   Line 10: Function 'add' expects 2 argument(s) but 1 given
[UNDECLARED_ERROR] Line 11: Call to undeclared function 'subtract'
```

## Project layout

```
compiler/
  src/lexer/lexer.l        Flex scanner
  src/parser/parser.y      Bison grammar → AST
  src/ast/                 AST nodes and pretty-printer
  src/semantic/            symbol tables, function table, type checker
  src/icg/icg.c, tac.c     three-address code generation
  src/icg/optimize.c       TAC optimizer
  src/error/               error collection and JSON output
  src/main.c               compiler driver / CLI
healer/healer.py           LLM-powered self-healing loop
input/                     sample programs (valid and intentionally broken)
run_all_tests.sh           test suite (exit code reflects pass/fail)
demo.sh                    interactive walkthrough
docs/                      original course report and submission notes
```

## About

Built as a Compiler Design course project and extended into PhoenixC. See [`docs/PROJECT_REPORT.md`](docs/PROJECT_REPORT.md) for the original report.
