#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import subprocess
import sys

def main():
    decaf_jar = os.path.join('..', '..', 'result', 'decaf.jar')
    names = sys.argv[1:]
    for name in names:
        bname,ext = os.path.splitext(name)
        if ext != '.decaf':
            continue
        print("Gen data: " + name);
        # Run the test case, redirecting stdout/stderr to output/bname.result
        subprocess.call(['java', '-jar', decaf_jar, '-l', '1', name],
                stdout=open(os.path.join('result', bname + '.result'), 'w'),
                stderr=subprocess.STDOUT)

if __name__ == '__main__':
    main()
