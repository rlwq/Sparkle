import os
import sys
import subprocess
import time
import re

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


def read_supl(rkl_path: str, extension: str) -> list[str] | None:
    out_path = rkl_path[:-4] + extension
    if not os.path.exists(out_path):
        return None
    with open(out_path) as f:
        return format_output(f.read())


def format_output(s: str) -> list[str]:
    return re.sub(r'\033\[[0-9;]*m', '', s).split()


def run_test(binary: str, rkl_path: str) -> tuple[list[str], list[str], int, float]:
    begin = time.perf_counter()
    result = subprocess.run(
        [binary, rkl_path],
        capture_output=True,
        text=True
    )
    end = time.perf_counter()
    return (format_output(result.stdout),
            format_output(result.stderr),
            result.returncode,
            end - begin)


def fmt_time(time: float) -> str:
    return f'{time * 1000:.2f}ms' if time < 1 else f'{time:.4f}s'


def rewrite(binary: str, tests: list[str], tests_folder: str, negative: bool) -> None:
    for rkl_path in tests:
        name = os.path.relpath(rkl_path, tests_folder)
        stdout, stderr, code, _ = run_test(binary, rkl_path)
        if negative:
            if not code:
                print(f'{RED}EXPECTED FAILURE BUT PASSED: {name}{RESET}')
                sys.exit(1)
            with open(rkl_path[:-4] + '.err', 'w') as f:
                print(' '.join(stderr), file=f)
            if stdout:
                with open(rkl_path[:-4] + '.out', 'w') as f:
                    print(' '.join(stdout), file=f)
        else:
            if code:
                print(f'{RED}CAN\'T RUN TEST {name}{RESET}')
                sys.exit(1)
            with open(rkl_path[:-4] + '.out', 'w') as f:
                print(' '.join(stdout), file=f)


def run_positive(binary: str, tests: list[str], tests_folder: str) -> tuple[int, int, int, float]:
    passed = failed = skipped = 0
    total_time = 0.0
 
    for rkl_path in tests:
        name = os.path.relpath(rkl_path, tests_folder)
        expected_out = read_supl(rkl_path, '.out')
 
        if expected_out is None:
            print(f'{YELLOW}MISSING{RESET} {name}.out')
            skipped += 1
            continue
 
        stdout, _, code, timing = run_test(binary, rkl_path)
        total_time += timing
        t = fmt_time(timing)
 
        if code:
            print(f'{YELLOW}CAN\'T RUN{RESET} ({t}) {name}')
            skipped += 1
            continue
 
        if stdout == expected_out:
            print(f'{GREEN}PASS{RESET} ({t}) {name}')
            passed += 1
        else:
            print(f'{RED}FAIL{RESET} ({t}) {name}')
            failed += 1
            print(f'    EXPECTED: {" ".join(expected_out)}')
            print(f'         GOT: {" ".join(stdout)}')
 
    return passed, failed, skipped, total_time
 
 
def run_negative(binary: str, tests: list[str], tests_folder: str) -> tuple[int, int, int, float]:
    passed = failed = skipped = 0
    total_time = 0.0
 
    for rkl_path in tests:
        name = os.path.relpath(rkl_path, tests_folder)
        expected_err = read_supl(rkl_path, '.err')
 
        if expected_err is None:
            print(f'{YELLOW}MISSING{RESET} {name}.err')
            skipped += 1
            continue
 
        stdout, stderr, code, timing = run_test(binary, rkl_path)
        total_time += timing
        t = fmt_time(timing)
 
        if not code:
            print(f'{RED}FAIL{RESET} ({t}) {name}  (expected failure, but exited 0)')
            failed += 1
            continue
 
        err_ok = stderr == expected_err
 
        expected_out = read_supl(rkl_path, '.out')
        out_ok = (expected_out is None) or (stdout == expected_out)
 
        if err_ok and out_ok:
            print(f'{GREEN}PASS{RESET} ({t}) {name}')
            passed += 1
        else:
            print(f'{RED}FAIL{RESET} ({t}) {name}')
            failed += 1

            if not err_ok:
                print(f'    STDERR EXPECTED: {" ".join(expected_err)}')
                print(f'    STDERR      GOT: {" ".join(stderr)}')

            if not out_ok and expected_out is not None:
                print(f'    STDOUT EXPECTED: {" ".join(expected_out)}')
                print(f'    STDOUT      GOT: {" ".join(stdout)}')
 
    return passed, failed, skipped, total_time
 

def run_all(binary: str, pos_folder: str | None, neg_folder: str | None) -> None:
    passed = failed = skipped = 0
    total_time = 0.0
 
    if pos_folder and os.path.isdir(pos_folder):
        print(f'\n{GREEN}POSITIVE TEST CASES{RESET}')
        p, f, s, t = run_positive(binary, find_tests(pos_folder), pos_folder)
        passed += p
        failed += f
        skipped += s
        total_time += t
 
    if neg_folder and os.path.isdir(neg_folder):
        print(f'\n{GREEN}NEGATIVE TEST CASES{RESET}')
        p, f, s, t = run_negative(binary, find_tests(neg_folder), neg_folder)
        passed += p
        failed += f
        skipped += s
        total_time += t
 
    print('\n════════════════════════════════════')
    print(f'Results ({fmt_time(total_time)}):',
          f'    {GREEN}{passed} passed{RESET}',
          f'    {YELLOW}{skipped} skipped{RESET}',
          f'    {RED}{failed} failed{RESET}',
          sep='\n', end='\n\n')
 
    sys.exit(1 if (failed or skipped) else 0)
 

def main():
    if len(sys.argv) < 3:
        print('Usage: tester.py <executable> <tests_folder> [--rewrite]')
        sys.exit(1)
 
    binary = sys.argv[1]
    tests_folder = sys.argv[2]
    rewriting = len(sys.argv) == 4 and sys.argv[3] == '--rewrite'
 
    pos_folder = os.path.join(tests_folder, 'positive')
    neg_folder = os.path.join(tests_folder, 'negative')
 
    if rewriting:
        if os.path.isdir(pos_folder):
            rewrite(binary, find_tests(pos_folder), pos_folder, negative=False)
        if os.path.isdir(neg_folder):
            rewrite(binary, find_tests(neg_folder), neg_folder, negative=True)
    else:
        run_all(binary, pos_folder, neg_folder)


if __name__ == '__main__':
    main()

