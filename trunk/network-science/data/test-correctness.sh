#!/bin/bash
./data-gen 5000 100000 0.5 > data.in
./algo -i data.in -con -deg -vio -info
