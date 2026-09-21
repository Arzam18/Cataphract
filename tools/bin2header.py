#!/usr/bin/env python3
"""
Generate a C++ header containing the raw bytes of an input file as a
comma-separated initializer list. Used to replace Clang's #embed, which
is not available in the NDK r28 toolchain.

Usage: bin2header.py <input.bin> <output.hpp>
"""

import sys


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <input.bin> <output.hpp>", file=sys.stderr)
        return 1

    src, dst = sys.argv[1], sys.argv[2]

    with open(src, "rb") as f:
        data = f.read()

    with open(dst, "w") as f:
        f.write("// Auto-generated from ")
        f.write(src)
        f.write(" — do not edit.\n")

        # 20 bytes per line keeps the file readable and avoids one
        # gigantic token line, which some editors/compilers dislike.
        for i in range(0, len(data), 20):
            f.write("    ")
            f.write(",".join(str(b) for b in data[i:i + 20]))
            f.write(",\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
