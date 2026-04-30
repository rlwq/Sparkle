import os
import sys
import subprocess

RESET  = '\033[0m'
RED    = '\033[31m'
GREEN  = '\033[32m'
YELLOW = '\033[33m'

def find_tests(folder: str) -> list[str]:
    return sorted(
        os.path.join(folder, f)
        for f in os.listdir(folder)
        if f.endswith('.rkl')
    )

def read_expected(rkl_path: str) -> str | None:
    out_path = rkl_path[:-4] + '.out'
    if not os.path.exists(out_path):
        return None
    with open(out_path) as f:
        return f.read().strip()

def run_test(binary: str, rkl_path: str) -> tuple[str, int]:
    result = subprocess.run(
        [binary, rkl_path],
        capture_output=True,
        text=True
    )
    return result.stdout.strip(), result.returncode


def main():
    if len(sys.argv) not in (3, 4):
        print('Usage: tester.py <executable> <tests_folder> [--rewrite]')
        sys.exit(1)

    binary = sys.argv[1]
    tests_folder = sys.argv[2]
    do_rewrite = False

    if len(sys.argv) >= 4:
        do_rewrite = (sys.argv[3] == '--rewrite')

    tests = find_tests(tests_folder)
    
    if do_rewrite:
        for rkl_path in tests:
            name = os.path.relpath(rkl_path, tests_folder)
            output, code = run_test(binary, rkl_path)

            if code:
                print(f'{RED}CAN\'T RUN TEST {name} {RESET}')
                sys.exit(1) 

            with open(rkl_path[:-4] + '.out', 'w') as out_file:
                _ = out_file.write(output)

        return

 
    passed = failed = skipped = 0
 
    for rkl_path in tests:
        name = os.path.relpath(rkl_path, tests_folder)
 
        expected = read_expected(rkl_path)
 
        if expected is None:
            print(f'{YELLOW}MISSING{RESET} {name}.out')
            skipped += 1
            continue

        output, code = run_test(binary, rkl_path)

        if code:
            print(f'{YELLOW}CAN\'T RUN{RESET} {rkl_path}')
            skipped += 1
            continue

        output = output.split()
        expected = expected.split()

        if output == expected:
            print(f'{GREEN}PASS{RESET} {name}')
            passed += 1
        else:
            print(f'{RED}FAIL{RESET} {name}')
            failed += 1
            
            print(f'    EXPECTED: {" ".join(expected)}')
            print(f'         GOT: {" ".join(output)}')
 
    print('-----------------')
    print('Results:',
          f'    {passed} passed, ',
          f'    {failed} failed, ',
          f'    {skipped} skipped', sep='\n')
 
    sys.exit(1 if (failed or skipped) else 0)
 
if __name__ == '__main__':
    main()
