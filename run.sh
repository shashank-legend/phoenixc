#!/bin/bash
# PhoenixC - build & run a single file (optionally self-heal it)
set -e
cd "$(dirname "$0")"

for tool in make flex bison; do
    command -v "$tool" >/dev/null || { echo "Error: $tool is not installed"; exit 1; }
done

make -C compiler >/dev/null
echo "✓ phoenixc built"

if [ -z "$1" ]; then
    echo "Usage: $0 <file.c> [--heal] [extra phoenixc flags]"
    echo "  $0 input/test_for_loop.c -O"
    echo "  $0 input/test_with_errors.c --heal"
    exit 1
fi

INPUT_FILE="$1"; shift
[ -f "$INPUT_FILE" ] || { echo "Error: '$INPUT_FILE' not found"; exit 1; }

if [ "$1" == "--heal" ]; then
    shift
    python3 -c "import requests" 2>/dev/null || pip3 install -r healer/requirements.txt
    python3 healer/healer.py "$INPUT_FILE" --output healed_output "$@"
else
    mkdir -p output
    ./compiler/phoenixc "$INPUT_FILE" --output output --print-ast --print-tac "$@"
fi
