#!/bin/bash

# Interactive demo for PhoenixC

clear
echo "========================================"
echo "  PhoenixC - Interactive Demo"
echo "========================================"
echo ""

COMPILER="./compiler/phoenixc"
OUTPUT_DIR="./demo_output"

rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

# Demo 1: Basic Compilation
echo "Demo 1: Compiling a Simple Program"
echo "----------------------------------------"
cat input/test.c
echo ""
echo "Press Enter to compile..."
read

$COMPILER input/test.c --output $OUTPUT_DIR --print-ast --print-tac

echo ""
echo "Generated TAC saved to: $OUTPUT_DIR/output.tac"
echo "Press Enter to continue..."
read
clear

# Demo 2: Error Detection
echo "Demo 2: Error Detection"
echo "----------------------------------------"
cat input/test_with_errors.c
echo ""
echo "Press Enter to compile (this will fail)..."
read

$COMPILER input/test_with_errors.c --output $OUTPUT_DIR

echo ""
echo "Errors saved to: $OUTPUT_DIR/errors.json"
cat $OUTPUT_DIR/errors.json
echo ""
echo "Press Enter to continue..."
read
clear

# Demo 3: Complex Program
echo "Demo 3: Complex Program with Loops"
echo "----------------------------------------"
cat input/test_while_loop.c
echo ""
echo "Press Enter to compile..."
read

$COMPILER input/test_while_loop.c --output $OUTPUT_DIR --print-tac

echo ""
echo "Press Enter to continue..."
read
clear

# Demo 4: Optimizer
echo "Demo 4: TAC Optimizer"
echo "----------------------------------------"
cat input/test_optimizer.c
echo ""
echo "Press Enter to compile with -O..."
read

$COMPILER input/test_optimizer.c -O --output $OUTPUT_DIR --print-tac

echo ""
echo "Press Enter to finish demo..."
read

echo ""
echo "========================================"
echo "  Demo Complete!"
echo "========================================"
echo "Check $OUTPUT_DIR/ for all outputs"
