#!/bin/bash
./data-gen 500 500 0.5 > data.in
./algo -i data.in -con -deg -act -vio -info
