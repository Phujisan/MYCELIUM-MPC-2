#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
for name in test_v097 test_v098; do
  gcc -std=c11 -O2 -Idsp "tests/${name}.c" -lm -o "/tmp/mycelium_official_${name}"
  "/tmp/mycelium_official_${name}"
done
