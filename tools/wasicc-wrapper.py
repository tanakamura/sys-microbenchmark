#!/usr/bin/env python3

import sys
import subprocess
import os

ignore_list = [
    '--start-group',
    '--end-group',
    '--as-needed',
    '--no-undefined',
]

def main(args):
    new_args = []
    prog = args[0]
    args = args[1:]

    for a in args:
        if a[:4] == '-Wl,':
            linker_command = a[4:]
            if linker_command in ignore_list:
                pass
            elif linker_command[:5] == '-rpath':
                pass
            else:
                new_args.append(a)
        else:
            new_args.append(a)

    if os.path.basename(prog) == 'wasic++-wrapper.py':
        com = 'wasic++'
    else:
        com = 'wasicc'

    new_args.insert(0, com)

    subprocess.call(new_args)

if __name__ == '__main__':
    main(sys.argv)
