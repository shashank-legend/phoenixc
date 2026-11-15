#!/bin/bash

# Automated test runner for PhoenixC
# Tests all sample programs and generates report

COMPILER="./compiler/phoenixc"
OUTPUT_DIR="./test_results"
INPUT_DIR="./input"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "  PhoenixC - Test Suite"
echo "========================================"
echo ""

# Clean and create output directory
rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

# Counter for statistics
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Test valid programs
echo "Testing Valid Programs:"
echo "----------------------------------------"

VALID_TESTS=(
    "test.c"
    "test_arithmetic.c"
    "test_nested_if.c"
    "test_while_loop.c"
    "test_expressions.c"
    "test_comparisons.c"
    "test_functions.c"
    "test_for_loop.c"
    "test_optimizer.c"
)

for test_file in "${VALID_TESTS[@]}"; do
    if [ ! -f "$INPUT_DIR/$test_file" ]; then
        continue
    fi
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -n "Testing $test_file ... "
    
    # Create output subdirectory
    TEST_NAME=$(basename $test_file .c)
    mkdir -p "$OUTPUT_DIR/$TEST_NAME"
    
    # Run compiler
    $COMPILER "$INPUT_DIR/$test_file" -O --output "$OUTPUT_DIR/$TEST_NAME" > "$OUTPUT_DIR/$TEST_NAME/log.txt" 2>&1
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}✗ FAIL${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
done

echo ""
echo "Testing Error Detection:"
echo "----------------------------------------"

ERROR_TESTS=(
    "test_with_errors.c"
    "error_missing_semicolon.c"
    "error_undeclared.c"
    "error_type_mismatch.c"
    "error_function_calls.c"
)

for test_file in "${ERROR_TESTS[@]}"; do
    if [ ! -f "$INPUT_DIR/$test_file" ]; then
        continue
    fi
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -n "Testing $test_file ... "
    
    # Create output subdirectory
    TEST_NAME=$(basename $test_file .c)
    mkdir -p "$OUTPUT_DIR/$TEST_NAME"
    
    # Run compiler (expecting failure)
    $COMPILER "$INPUT_DIR/$test_file" --output "$OUTPUT_DIR/$TEST_NAME" > "$OUTPUT_DIR/$TEST_NAME/log.txt" 2>&1
    
    if [ $? -ne 0 ]; then
        echo -e "${GREEN}✓ PASS (Error Detected)${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}✗ FAIL (Should have failed)${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
done

echo ""
echo "========================================"
echo "  Test Results Summary"
echo "========================================"
echo "Total Tests:  $TOTAL_TESTS"
echo -e "Passed:       ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed:       ${RED}$FAILED_TESTS${NC}"
echo ""
echo "Detailed results saved in: $OUTPUT_DIR/"
echo "========================================"

[ "$FAILED_TESTS" -eq 0 ]
