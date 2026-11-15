# Compiler Design Project Submission

## Student Information
- Name: [Your Name]
- Roll Number: [Your Roll Number]
- Course: Compiler Design
- Project: Mini C Compiler

## How to Run

### Build the Compiler
```bash
cd compiler
make
```

### Test with Sample Programs
```bash
# Test valid program
./compiler/phoenixc input/test.c --output output --print-tac

# Test error detection
./compiler/phoenixc input/test_with_errors.c --output output
```

### Run All Tests
```bash
./run_all_tests.sh
```

### Interactive Demo
```bash
./demo.sh
```

## Project Highlights

✅ Complete lexical analysis with Flex  
✅ LALR parser with Bison  
✅ Full AST construction  
✅ Semantic analysis with symbol tables  
✅ TAC intermediate code generation  
✅ Comprehensive error reporting  
✅ 10+ test cases with validation  
✅ Self-healing capability (bonus)  

## Files Submitted

- Source code: `compiler/src/`
- Test cases: `input/`
- Test results: `test_results/`
- Documentation: `docs/PROJECT_REPORT.md`
- Demo script: `demo.sh`
- Test runner: `run_all_tests.sh`

## Output Files

All compilation outputs are in `output/` and `test_results/` directories.