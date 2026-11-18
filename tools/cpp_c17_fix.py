#!/usr/bin/env python3
"""Token-aware C++ fixes for C++17 conformance (no regular expressions over raw text).

Scans C/C++ sources and changes only tokens outside comments and literals:
  --literal-suffix  a string or character literal directly followed by an identifier gets a space between them
                    (C++11 reads "text"NAME as a user-defined literal suffix; the macro is meant to be a token);
  --nullptr         the NULL macro becomes nullptr.

Usage: python3 cpp_c17_fix.py [--literal-suffix] [--nullptr] [--check] FILE...
--check counts the changes without writing the files.
Encoding and line endings (CRLF or LF) are kept.
"""
import re
import sys
from pathlib import Path

TOKEN = re.compile(r'''
    (?P<line_comment>//[^\n]*)
  | (?P<block_comment>/\*.*?\*/)
  | (?P<string>(?:u8|u|U|L)?"(?:\\.|[^"\\\n])*")
  | (?P<char>(?:u8|u|U|L)?'(?:\\.|[^'\\\n])*')
  | (?P<ident>[A-Za-z_][A-Za-z_0-9]*)
  | (?P<other>.)
''', re.VERBOSE | re.DOTALL)


def fix(text, literal_suffix, nullptr):
    out = []
    changes = 0
    previous_kind = None
    previous_end = -1
    for match in TOKEN.finditer(text):
        kind = match.lastgroup
        value = match.group(0)
        if kind == 'ident' and nullptr and value == 'NULL':
            value = 'nullptr'
            changes += 1
        if literal_suffix and kind == 'ident' and previous_kind in ('string', 'char') \
                and match.start() == previous_end:
            out.append(' ')
            changes += 1
        out.append(value)
        previous_kind = kind
        previous_end = match.end()
    return ''.join(out), changes


def main(argv):
    literal_suffix = '--literal-suffix' in argv
    nullptr = '--nullptr' in argv
    check = '--check' in argv
    files = [a for a in argv if not a.startswith('--')]
    total = 0
    for name in files:
        path = Path(name)
        raw = path.read_bytes().decode('utf-8')
        new, changes = fix(raw, literal_suffix, nullptr)
        if changes and not check:
            path.write_bytes(new.encode('utf-8'))
        if changes:
            total += changes
            print(f'{name}: {changes}')
    print(f'total changes: {total}{" (check only)" if check else ""}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
