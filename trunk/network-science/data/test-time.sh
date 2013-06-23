#!/bin/bash
./data-gen 1000000 3000000 0.5 > data.in
time ./algo -i data.in -con -deg
