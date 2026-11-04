# Matrix multiplication

## Build and run tests

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Or compile directly with GCC/Clang and run from the repository root:

```sh
g++ -std=c++17 -O3 -pthread -fno-fast-math -ffp-contract=off -Wall -Wextra \
    tests.cpp generate.cpp multiply.cpp -o matrix_tests
./matrix_tests
```

An optional argument selects the fixture directory when running from another
working directory: `matrix_tests /path/to/testdata/2048`. CMake builds already
embed the absolute fixture directory.

## Frozen 2048 x 2048 regression data

`testdata/2048/a.bin`, `b.bin`, and `expected.bin` contain the complete fixed
inputs A and B and the expected product A x B. Each file is exactly 33,554,432
bytes: 4,194,304 IEEE-754 binary64 values in row-major, little-endian order,
without a header. Total fixture size is 96 MiB. Git stores the files directly;
Git LFS and runtime generation are not required. `SHA256SUMS` records their hashes.

The test loads these bytes and checks every result element's 64-bit
representation, with no tolerance and no checksum-only comparison. It runs with
1, 3, and 8 threads, including uneven row partitions, and reports the first
mismatching zero-based row/column and expected/actual hexadecimal bits.
Missing, truncated, and oversized fixtures fail the test.

The data contains dense signed fractional values. The numerical contract is
IEEE-754 binary64 with round-to-nearest, ties-to-even, increasing `k` accumulation,
and separately rounded multiplication and addition. Fused multiply-add and
reassociation can change the last bits, so compile **without fast-math and with
FP contraction disabled** (`-fno-fast-math -ffp-contract=off`, or `/fp:strict` on
MSVC). The CMake targets apply these options.

### Fixture provenance

`tools/generate_2048.cpp` is an offline maintenance tool. Its fixed integer PRNG
seeds (`0x6a09e667f3bcc909` for A, `0xbb67ae8584caa73b` for B), xorshift64
steps (13, 7, 17), and conversion `(int(state % 2001) - 1000) / 10.0` define
the inputs. Its independent scalar dot-product
oracle transposes B for locality, accumulates in increasing `k`, and forces each
product and addition to round to binary64. It does not call or link
`multiply_matrices`. Test execution never runs this tool or regenerates the
expected result.

To reproduce into a **new** directory for review:

```sh
cmake --build build --config Release --target generate_2048_fixture
./build/generate_2048_fixture /tmp/matrix-fixture-review
```

For multi-configuration builds the tool is in `build/Release/`. Compare all three
files against the committed hashes before considering any fixture change.
