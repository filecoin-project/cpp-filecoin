#!/usr/bin/env python3

import sys
import json
import argparse
from pathlib import Path
import re


def remove_non_whitelist(wl):
    def f(x):
        for w in wl:
            if w in x['file']:
                print("selected for analysis: {}".format(w))
                return True
        return False

    return f


def do(args) -> None:
    builddir = args.p

    if not builddir.exists():
        raise Exception("build dir {} does not exist".format(builddir))

    if not builddir.is_dir():
        raise Exception("build dir {} is not dir".format(builddir))

    p = Path(builddir, "compile_commands.json")
    if not p.exists():
        raise Exception("build dir {} does not contain compile_commands.json".format(builddir))

    with p.open("r") as f:
        data = f.read()
        j = json.loads(data)

        wl = read_whitelist()
        j = filter(remove_non_whitelist(wl), j)
        j = list(j)
        s = json.dumps(j, indent=4)

    p2 = builddir / "filter_compile_commands"
    p2.mkdir(parents=True, exist_ok=True)
    with (p2 / "compile_commands.json").open("w") as w:
        w.write(s)
        print("success")


def read_whitelist():
    whitelist = []
    print("provide whitelisted files, one path on a single line")

    for line in sys.stdin:
        lines = re.split("[\s\n]+", line)
        for l in lines:
            if len(l) > 0:
                whitelist.append(l)

    return whitelist


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog="filter "
    )
    parser.add_argument("-p", help="path to build dir",
                        type=Path)

    args = parser.parse_args()
    do(args)
