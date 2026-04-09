#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Robin Jarry

"""
Generate a C file that references all function and variable declarations from
project headers. This ensures the linker will complain if any declared symbol
is missing a definition.
"""

import argparse
import json
import subprocess


def is_project_header(path: str, excludes: list[str]) -> bool:
    if not path:
        return False
    if path.startswith("/usr"):
        return False
    if path.endswith(".c"):
        return False
    for exc in excludes:
        if path.endswith(exc):
            return False
    return True


def get_effective_file(node: dict, current_file: str) -> str:
    loc = node.get("loc", {})

    range_file = node.get("range", {}).get("begin", {}).get("file", "")
    if range_file:
        return range_file

    f = loc.get("file", "")
    if f:
        return f

    f = loc.get("expansionLoc", {}).get("file", "")
    if f:
        return f

    return current_file


def update_current_file(node: dict, current_file: str) -> str:
    loc = node.get("loc", {})
    f = loc.get("file", "")
    if f:
        return f
    f = node.get("range", {}).get("begin", {}).get("file", "")
    if f:
        return f
    return current_file


def extract_symbols(ast: dict, excludes: list[str]) -> tuple[list[str], list[str]]:
    funcs = []
    variables = []
    current_file = ""

    for node in ast.get("inner", []):
        effective_file = get_effective_file(node, current_file)
        current_file = update_current_file(node, current_file)

        kind = node.get("kind")
        name = node.get("name", "")

        if not is_project_header(effective_file, excludes):
            continue
        if name.startswith("_"):
            continue

        if kind == "FunctionDecl":
            if node.get("storageClass", "") == "static":
                continue
            funcs.append(name)

        elif kind == "VarDecl":
            if node.get("storageClass", "") == "extern":
                variables.append(name)

    return sorted(set(funcs)), sorted(set(variables))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("include_dir")
    parser.add_argument("--exclude", action="append", default=[])
    args = parser.parse_args()

    # Dump the AST in JSON format.
    cmd = (
        "clang",
        "-Xclang",
        "-ast-dump=json",
        "-fsyntax-only",
        "-I" + args.include_dir,
        "-include",
        "ecoli.h",
        "-x",
        "c",
        "/dev/null",
    )
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        parser.error(result.stderr)

    # Extract all ecoli symbols from the AST
    ast = json.loads(result.stdout)
    funcs, variables = extract_symbols(ast, args.exclude)

    # Generate a dummy C source file that references all declared symbols in
    # ecoli headers.
    print("/* Auto-generated - do not edit. */")
    print()
    print("#include <ecoli.h>")
    print("#include <stdio.h>")
    print()
    print("static void *syms[] = {")
    for f in funcs:
        print(f"\t(void *){f},")
    for v in variables:
        print(f"\t(void *)&{v},")
    print("};")
    print()
    print("int main(void)")
    print("{")
    print("\tfor (unsigned i = 0; i < sizeof(syms) / sizeof(syms[0]); i++)")
    print('\t\tprintf("%p\\n", syms[i]);')
    print("\treturn 0;")
    print("}")


if __name__ == "__main__":
    main()
