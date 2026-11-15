# Mini C Compiler - Project Report

## Project Overview

A complete Mini C compiler implementation using traditional compiler design techniques including lexical analysis (Flex), syntax analysis (Bison), semantic analysis, and intermediate code generation (Three-Address Code).

## Team Information

- **Project Name**: Mini C Compiler with Self-Healing
- **Course**: Compiler Design
- **Implementation Language**: C (Compiler), Python (Self-Healing Module)

## Features Implemented

### ✅ Phase 1: Lexical Analysis (Flex)
- Tokenization of C source code
- Recognition of keywords, identifiers, literals, operators
- Comment handling (single-line and multi-line)
- Error reporting for invalid tokens

### ✅ Phase 2: Syntax Analysis (Bison)
- LALR(1) parser implementation
- Context-free grammar for Mini C
- Abstract Syntax Tree (AST) construction
- Syntax error detection and reporting

### ✅ Phase 3: Semantic Analysis
- Symbol table management with scope handling
- Type checking for expressions
- Variable declaration/usage validation
- Undeclared variable detection
- Redeclaration detection
- Type mismatch warnings

### ✅ Phase 4: Intermediate Code Generation
- Three-Address Code (TAC) generation
- Support for:
  - Arithmetic operations (+, -, *, /)
  - Comparison operations (<, >, <=, >=, ==, !=)
  - Assignments
  - Control flow (if/else, while)
  - Function calls
  - Temporary variable generation
  - Label generation for control flow

### ✅ Phase 5: Error Reporting
- Comprehensive JSON error output
- Error categorization (lexical, syntax, semantic, type)
- Line and column number reporting
- Integration with self-healing module

### ✅ Bonus: Self-Healing Module
- Python-based automatic error correction
- OpenAI GPT integration
- Iterative healing with retry logic
- Automatic test case generation

## Supported C Language Features

### Data Types
- `int` - Integer type
- `float` - Floating-point type
- `void` - Void type (for functions)

### Operators
- **Arithmetic**: `+`, `-`, `*`, `/`
- **Comparison**: `==`, `!=`, `<`, `>`, `<=`, `>=`
- **Assignment**: `=`

### Control Structures
- `if` statements
- `if-else` statements
- `while` loops

### Functions
- Function declarations
- Function calls with arguments
- Return statements

## Project Structure
```
version-2/
├── compiler/
│   ├── src/
│   │   ├── lexer/lexer.l          # Flex lexical analyzer
│   │   ├── parser/parser.y        # Bison parser grammar
│   │   ├── ast/                   # AST implementation
│   │   ├── semantic/              # Semantic analyzer
│   │   ├── icg/                   # Intermediate code generator
│   │   ├── error/                 # Error reporting system
│   │   └── main.c                 # Compiler driver
│   ├── build/                     # Compiled objects
│   └── Makefile                   # Build configuration
├── healer/
│   ├── healer.py                  # Self-healing module
│   └── requirements.txt           # Python dependencies
├── input/                         # Test C programs
├── output/                        # Compilation output
├── test_results/                  # Test suite results
└── run_all_tests.sh              # Automated test runner
```

## Build Instructions

### Prerequisites
```bash
sudo apt-get install build-essential flex bison libfl-dev
pip3 install openai
```

### Build
```bash
cd compiler
make clean
make
```

### Usage
```bash
# Compile a C file
./minic input_file.c --output output_dir

# With debugging output
./minic input_file.c --output output_dir --print-ast --print-tac

# Self-healing mode
export OPENAI_API_KEY="your-key"
python3 healer/healer.py input_file.c --output output_dir
```

## Test Results

### Test Cases Executed

1. **Basic Arithmetic** - PASS ✓
2. **Nested Conditionals** - PASS ✓
3. **While Loops** - PASS ✓
4. **Complex Expressions** - PASS ✓
5. **Comparison Operators** - PASS ✓
6. **Function Calls** - PASS ✓
7. **Error Detection (Missing Semicolon)** - PASS ✓
8. **Error Detection (Undeclared Variable)** - PASS ✓
9. **Error Detection (Type Mismatch)** - PASS ✓

### Sample Output

**Input (test.c)**:
```c
int main() {
    int a = 5;
    int b = 10;
    int c = a + b;
    return c;
}
```

**Generated TAC**:
```
  a = 5
  b = 10
  t0 = a + b
  c = t0
  return c
```

## Technical Implementation Details

### Symbol Table
- Implemented as linked list structure
- Supports nested scopes
- Tracks variable types and initialization status

### TAC Generation
- Expression evaluation with temporary variables
- Control flow using labels and conditional jumps
- Function call handling with parameter passing

### Error Recovery
- Continues parsing after syntax errors
- Collects all errors before reporting
- Provides detailed error context

## Challenges and Solutions

### Challenge 1: Include Path Resolution
**Problem**: Header files not found during compilation  
**Solution**: Updated Makefile with correct include paths (-Isrc/*)

### Challenge 2: Symbol Name Conflicts
**Problem**: Variable names conflicting with function names  
**Solution**: Renamed variables with `flag_` prefix

### Challenge 3: Scope Management
**Problem**: Variables visible outside their scope  
**Solution**: Implemented hierarchical symbol table with parent pointers

## Future Enhancements

1. Array support
2. Pointer arithmetic
3. Structure/union types
4. For loops
5. Switch statements
6. Code optimization passes
7. Assembly code generation
8. Better error recovery strategies

## References

1. Aho, Lam, Sethi, Ullman - "Compilers: Principles, Techniques, and Tools" (Dragon Book)
2. Flex Manual - https://westes.github.io/flex/manual/
3. Bison Manual - https://www.gnu.org/software/bison/manual/
4. OpenAI API Documentation

## Conclusion

Successfully implemented a functional Mini C compiler demonstrating all phases of compilation from lexical analysis to intermediate code generation. The addition of a self-healing module using AI showcases modern approaches to compiler error correction.

## License

Educational project for Compiler Design course.