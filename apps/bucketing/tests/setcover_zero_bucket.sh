#!/bin/sh
# Run from anywhere with the SetCover executable as argument.
# Build it with the normal OpenMP flags, including -march=native.
set -eu
exe=$(realpath "$1")
work=$(mktemp -d)
trap 'rm -f "$work/edge.adj"; rmdir "$work"' EXIT
printf 'AdjacencyGraph\n2\n2\n0\n1\n1\n0\n' > "$work/edge.adj"
OMP_NUM_THREADS=1 timeout 10 "$exe" -s -rounds 1 "$work/edge.adj"
