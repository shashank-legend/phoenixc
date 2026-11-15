#!/usr/bin/env python3
"""
PhoenixC Healer
Automatically repairs code that fails to compile, using a local LLM via Ollama.
"""

import os
import sys
import json
import subprocess
import argparse
from pathlib import Path
from datetime import datetime
import requests

COMPILER_PATH = os.environ.get("PHOENIXC", "./compiler/phoenixc")

class CompilerHealer:
    def __init__(self, model="codellama", max_iterations=3, ollama_url="http://127.0.0.1:11434"):
        """Initialize the healer with Ollama"""
        self.model = model
        self.max_iterations = max_iterations
        self.ollama_url = ollama_url
        self.compiler_path = COMPILER_PATH
        
        # Test Ollama connection
        try:
            response = requests.get(f"{self.ollama_url}/api/tags", timeout=5)
            if response.status_code != 200:
                print("Warning: Could not connect to Ollama. Make sure it's running.")
                print("Start Ollama with: ollama serve")
        except:
            print("Warning: Ollama not responding. Make sure it's running.")
            print("Start Ollama with: ollama serve")
    
    def compile_code(self, input_file, output_dir="output"):
        """Compile C code and return results"""
        os.makedirs(output_dir, exist_ok=True)
        
        cmd = [self.compiler_path, input_file, "--output", output_dir]
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True
        )
        
        return {
            'success': result.returncode == 0,
            'stdout': result.stdout,
            'stderr': result.stderr,
            'returncode': result.returncode
        }
    
    def read_errors(self, error_file="output/errors.json"):
        """Read compilation errors from JSON"""
        if not os.path.exists(error_file):
            return None
        
        try:
            with open(error_file, 'r') as f:
                return json.load(f)
        except:
            return None
    
    def read_source(self, file_path):
        """Read source code from file"""
        try:
            with open(file_path, 'r') as f:
                return f.read()
        except Exception as e:
            print(f"Error reading file: {e}")
            return None
    
    def write_source(self, file_path, content):
        """Write corrected source code to file"""
        try:
            os.makedirs(os.path.dirname(file_path), exist_ok=True)
            with open(file_path, 'w') as f:
                f.write(content)
            return True
        except Exception as e:
            print(f"Error writing file: {e}")
            return False
    
    def format_errors_for_prompt(self, errors):
        """Format errors into readable string"""
        if not errors or errors.get('error_count', 0) == 0:
            return "No errors found."
        
        formatted = f"The compiler found {errors['error_count']} error(s):\n\n"
        
        for i, error in enumerate(errors.get('errors', []), 1):
            formatted += f"Error {i}:\n"
            formatted += f"  Type: {error['type']}\n"
            formatted += f"  Location: Line {error['line']}, Column {error['column']}\n"
            formatted += f"  Message: {error['message']}\n"
            formatted += "\n"
        
        return formatted
    
    def generate_fix_prompt(self, source_code, errors):
        """Generate detailed prompt for Ollama"""
        error_description = self.format_errors_for_prompt(errors)
        
        prompt = f"""Fix the compilation errors in this C code.

ERRORS:
{error_description}

ORIGINAL CODE:
{source_code}

Instructions:
1. Fix ALL compilation errors
2. Keep the original logic
3. Add missing semicolons, declarations, etc.
4. Return ONLY the corrected C code
5. Do NOT add explanations or comments

CORRECTED CODE:"""

        return prompt
    
    def ask_ollama_for_fix(self, prompt):
        """Call Ollama API to get corrected code"""
        try:
            print("  Requesting fix from Ollama (local AI)...")
            
            response = requests.post(
                f"{self.ollama_url}/api/generate",
                json={
                    "model": self.model,
                    "prompt": prompt,
                    "stream": False,
                    "options": {
                        "temperature": 0.2,
                        "num_predict": 1000
                    }
                },
                timeout=120
            )
            
            if response.status_code != 200:
                print(f"  ✗ Ollama error: {response.status_code}")
                return None
            
            result = response.json()
            fixed_code = result.get('response', '').strip()
            
            if not fixed_code:
                print("  ✗ Ollama returned empty response")
                return None
            
            # Clean up the response
            # Remove markdown code blocks if present
            if "```c" in fixed_code:
                start = fixed_code.find("```c") + 4
                end = fixed_code.rfind("```")
                if end > start:
                    fixed_code = fixed_code[start:end].strip()
            elif "```" in fixed_code:
                start = fixed_code.find("```") + 3
                end = fixed_code.rfind("```")
                if end > start:
                    fixed_code = fixed_code[start:end].strip()
            
            # Try to extract just the code (remove explanations)
            lines = fixed_code.split('\n')
            code_lines = []
            in_code = False
            
            for line in lines:
                # Skip prose lines the model sometimes adds (but never C code lines)
                stripped = line.strip().lower()
                looks_like_code = stripped.endswith((';', '{', '}', ')')) or stripped.startswith(('//', '#'))
                if not looks_like_code and any(k in stripped for k in ['here is', 'here\'s', 'i fixed', 'corrected', 'the error']):
                    continue
                # Start collecting when we see code-like patterns
                if any(keyword in line for keyword in ['int ', 'float ', 'void ', 'return', '#include', '//']):
                    in_code = True
                if in_code:
                    code_lines.append(line)
            
            if code_lines:
                fixed_code = '\n'.join(code_lines)
            
            return fixed_code
            
        except requests.exceptions.Timeout:
            print("  ✗ Ollama request timed out")
            return None
        except requests.exceptions.ConnectionError:
            print("  ✗ Could not connect to Ollama. Is it running?")
            print("  Start it with: ollama serve")
            return None
        except Exception as e:
            print(f"  ✗ Error calling Ollama: {e}")
            return None
    
    def save_healing_log(self, output_dir, log_data):
        """Save healing process log"""
        log_file = os.path.join(output_dir, "healing_log.json")
        with open(log_file, 'w') as f:
            json.dump(log_data, f, indent=2)
    
    def heal(self, input_file, output_dir="healed_output"):
        """Main healing process"""
        print("=" * 70)
        print("  PHOENIXC HEALER - rising from compile errors (powered by Ollama)")
        print("=" * 70)
        print(f"Input file: {input_file}")
        print(f"Output directory: {output_dir}")
        print(f"AI Model: {self.model}")
        print()
        
        # Create output directory
        os.makedirs(output_dir, exist_ok=True)
        
        # Get base filename
        base_name = os.path.splitext(os.path.basename(input_file))[0]
        
        # Log data
        log_data = {
            'original_file': input_file,
            'model': self.model,
            'start_time': datetime.now().isoformat(),
            'iterations': [],
            'success': False
        }
        
        current_file = input_file
        iteration = 0
        
        # One compile pass for the original file plus one per healing attempt,
        # so the final attempt is always verified.
        total_passes = self.max_iterations + 1
        while iteration < total_passes:
            iteration += 1
            print(f"\n{'─' * 70}")
            print(f"  PASS {iteration}/{total_passes}")
            print('─' * 70)
            
            iteration_data = {
                'iteration': iteration,
                'input_file': current_file
            }
            
            # Compile
            print("→ Compiling code...")
            compile_result = self.compile_code(current_file, output_dir)
            
            if compile_result['success']:
                print("  ✓ Compilation SUCCESSFUL!")
                print()
                print(compile_result['stdout'])
                
                # If this was a healed version, save final corrected file
                if iteration > 1:
                    final_file = os.path.join(output_dir, f"{base_name}_corrected.c")
                    source = self.read_source(current_file)
                    self.write_source(final_file, source)
                    print(f"\n✓ HEALED CODE SAVED: {final_file}")
                else:
                    print("\n✓ Original code compiled successfully (no healing needed)")
                
                iteration_data['status'] = 'success'
                log_data['iterations'].append(iteration_data)
                log_data['success'] = True
                log_data['final_file'] = final_file if iteration > 1 else input_file
                log_data['end_time'] = datetime.now().isoformat()
                
                self.save_healing_log(output_dir, log_data)
                return True
            
            print("  ✗ Compilation FAILED")
            
            # Read errors
            error_file = os.path.join(output_dir, "errors.json")
            errors = self.read_errors(error_file)
            
            if not errors:
                print("  ✗ Could not read error information")
                iteration_data['status'] = 'error_read_failed'
                log_data['iterations'].append(iteration_data)
                self.save_healing_log(output_dir, log_data)
                return False
            
            print(f"  Found {errors['error_count']} error(s):")
            for err in errors.get('errors', []):
                print(f"    - Line {err['line']}: {err['message']}")
            
            iteration_data['errors'] = errors

            if iteration == total_passes:
                iteration_data['status'] = 'attempts_exhausted'
                log_data['iterations'].append(iteration_data)
                break
            
            # Read source
            source_code = self.read_source(current_file)
            if not source_code:
                print("  ✗ Could not read source code")
                iteration_data['status'] = 'source_read_failed'
                log_data['iterations'].append(iteration_data)
                self.save_healing_log(output_dir, log_data)
                return False
            
            # Generate fix prompt
            prompt = self.generate_fix_prompt(source_code, errors)
            
            # Get fixed code from Ollama
            fixed_code = self.ask_ollama_for_fix(prompt)
            
            if not fixed_code:
                print("  ✗ Could not get fix from Ollama")
                iteration_data['status'] = 'ollama_failed'
                log_data['iterations'].append(iteration_data)
                self.save_healing_log(output_dir, log_data)
                return False
            
            print("  ✓ Received fixed code from Ollama")
            
            # Save fixed code
            attempt_file = os.path.join(output_dir, f"{base_name}_attempt_{iteration}.c")
            if not self.write_source(attempt_file, fixed_code):
                print("  ✗ Could not save fixed code")
                iteration_data['status'] = 'write_failed'
                log_data['iterations'].append(iteration_data)
                self.save_healing_log(output_dir, log_data)
                return False
            
            print(f"  ✓ Saved to: {attempt_file}")
            
            iteration_data['status'] = 'healing_attempted'
            iteration_data['attempt_file'] = attempt_file
            log_data['iterations'].append(iteration_data)
            
            # Use this for next iteration
            current_file = attempt_file
        
        print(f"\n{'=' * 70}")
        print(f"  ✗ Failed to heal code after {self.max_iterations} iterations")
        print('=' * 70)
        
        log_data['success'] = False
        log_data['end_time'] = datetime.now().isoformat()
        self.save_healing_log(output_dir, log_data)
        
        return False

def main():
    parser = argparse.ArgumentParser(
        description="PhoenixC Healer - automatically fix compilation errors using a local LLM (Ollama)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Heal a buggy C file
  python3 healer.py input/buggy_code.c
  
  # Specify output directory
  python3 healer.py input/buggy_code.c --output my_output
  
  # Use different model
  python3 healer.py input/buggy_code.c --model deepseek-coder
  
  # Set max healing attempts
  python3 healer.py input/buggy_code.c --max-iterations 5

Available Models (install with 'ollama pull <model>'):
  - codellama (default, good for C code)
  - deepseek-coder (alternative)
  - llama3 (general purpose)
        """
    )
    
    parser.add_argument("input_file", help="Input C source file to heal")
    parser.add_argument("--model", default="codellama", 
                       help="Ollama model to use (default: codellama)")
    parser.add_argument("--max-iterations", type=int, default=3, 
                       help="Maximum healing attempts (default: 3)")
    parser.add_argument("--output", default="healed_output", 
                       help="Output directory (default: healed_output)")
    parser.add_argument("--ollama-url", default="http://127.0.0.1:11434",
                       help="Ollama server URL (default: http://127.0.0.1:11434)")
    
    args = parser.parse_args()
    
    # Check input file
    if not os.path.exists(args.input_file):
        print(f"Error: Input file '{args.input_file}' not found")
        sys.exit(1)
    
    # Check compiler
    if not os.path.exists(COMPILER_PATH):
        print(f"Error: Compiler '{COMPILER_PATH}' not found")
        print("Build it first with: cd compiler && make")
        sys.exit(1)
    
    # Check Ollama is running
    try:
        response = requests.get(f"{args.ollama_url}/api/tags", timeout=5)
        if response.status_code != 200:
            print("Error: Ollama is not responding")
            print("Make sure Ollama is running. It should have started automatically.")
            print("If not, start it with: ollama serve")
            sys.exit(1)
    except:
        print("Error: Cannot connect to Ollama")
        print("Ollama should be running at", args.ollama_url)
        print("Check if it's running with: ps aux | grep ollama")
        sys.exit(1)
    
    print("✓ Ollama is running")
    print(f"✓ Using model: {args.model}")
    print()
    
    # Create healer and run
    healer = CompilerHealer(
        model=args.model,
        max_iterations=args.max_iterations,
        ollama_url=args.ollama_url
    )
    
    success = healer.heal(args.input_file, args.output)
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()