# C library tests

Host GoogleTest suites cover character handling, I/O, allocation, strings and
runtime helpers. Requires Make, native C11/C++17 compilers and
`third_party/googletest`.

```sh
bash libc/tests/run.sh
LIBC_TEST_FILTER='string/*' bash libc/tests/run.sh
```

Pass GoogleTest arguments directly. ASan/UBSan are enabled by default;
`SANITIZE=0` disables them. `CC`/`CXX` select compilers. `LIBC_TEST_TIMEOUT`
limits each executable; `LIBC_TEST_BUILD_DIR` selects an absolute output path.
Production symbols are renamed to keep the host runtime separate from Ghost's
implementation.
