#!/bin/sh
set -exu

g++ -c zdrojak1.cpp -o zdrojak1.o -O2
g++ -c zdrojak2.cpp -o zdrojak2.o -O2
g++ -o binarka zdrojak1.o zdrojak2.o -O2
