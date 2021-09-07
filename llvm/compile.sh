#!/bin/bash

./llamac --printIRCode < $1 > a.ll
clang -o a.out a.ll lib.a