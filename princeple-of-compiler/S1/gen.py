#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import subprocess
import sys

def read_txt_file(filename):
    with open(filename,'r') as f:
        txt = f.read().strip()
    # Python should be able to do it automatically, but just in case...
    txt = txt.replace('\r','')
    return txt

def main():
    decaf_jar = os.path.join('..', '..', 'result', 'decaf.jar')
    names = sys.argv[1:]
    for name in names:
        bname,ext = os.path.splitext(name)
        if ext != '.decaf':
            continue
        print("Gen data: " + name);
        # Run the test case, redirecting stdout/stderr to output/bname.result
        subprocess.call(['java', '-jar', decaf_jar, '-l', '0', name],
                stdout=open(os.path.join('result', bname + '.result'), 'w'),
                stderr=subprocess.STDOUT)

if __name__ == '__main__':
    main()
