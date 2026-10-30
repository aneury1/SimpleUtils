# Validation results

Validated 2026-10-10 with GNU C++ 13.3.0, CMake 3.28.3, OpenSSL 3.0.13 and
GoogleTest 1.14.0 on Linux.

- Debug CMake build: static archive, example, and GoogleTest executable built successfully.
- GoogleTest/CTest: 155 registered tests; 154 passed, 1 skipped, 0 failed.
- Skipped test: `IP.Interfaces`. The host denies `getifaddrs` interface enumeration
  with an OS permission error. The test skips only permission denial; unexpected
  enumeration errors still fail. Actual enumeration success remains to be verified
  on a host that permits it. IP address validation tests passed.
- IPv6 loopback, TCP/UDP binary exchanges, TCP timeout/refused connect/EOF,
  HTTP/HEAD/chunked loopback and malformed-input tests passed.
- AddressSanitizer + UndefinedBehaviorSanitizer: same 154 passed, 1 skipped;
  no address or undefined-behavior diagnostics. LeakSanitizer cannot operate in
  this traced/restricted environment, so this run used `ASAN_OPTIONS=detect_leaks=0`.
  No leak-clean claim is made.
- All 14 canonical public headers compiled independently, with repeated inclusion,
  under `-std=c++17 -Wall -Wextra -Werror`.
- Release install/relocation check passed: installed the archive, headers and
  CMake package, moved the installation prefix, then built and ran a separate
  two-translation-unit consumer using `find_package(SimpleUtils 1 CONFIG REQUIRED)`.
- Normal library build had no compiler warnings with its configured warning flags.

The Windows/POSIX branches are supplied, but Windows and macOS builds/runs were
not performed here. Windows formatter invocation is explicitly unsupported;
POSIX formatting is tested with a local shell-free executable invocation.
Tests exercise broad API behavior and negative cases; they are not a proof of
all inputs, OS configurations, concurrency behavior or cryptographic certification.

To reproduce with GoogleTest installed:

```sh
cmake -S . -B build -DSIMPLEUTILS_BUILD_TESTS=ON -DSIMPLEUTILS_BUILD_EXAMPLES=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake -DSOURCE_DIR="$PWD" -DBUILD_ROOT="$PWD/install-check" -P test/verify_install.cmake
```

For address/UB checks on GCC/Clang:

```sh
cmake -S . -B build-sanitized -DSIMPLEUTILS_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-sanitized --parallel
./build-sanitized/test/simpleutils_tests
```

If your host cannot run LeakSanitizer, set `ASAN_OPTIONS=detect_leaks=0` during
both the build (GoogleTest discovery runs then) and executable invocation.
