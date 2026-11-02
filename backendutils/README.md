# SimpleUtils / BackendUtils 1.0.0

A C++17 static utility library. All public classes, enums, templates and functions live
under `simpleutils` (`simpleutils::sslHelper` for certificate compatibility). No
wxWidgets dependency and no additions to `namespace std`.

## Requirements

- CMake 3.16+, a C++17 compiler, OpenSSL development headers/libraries (1.1.1+).
- GoogleTest only for tests (installed package, local source tree, or optional fetch).
- Linux/POSIX and Windows socket branches. See VALIDATION.md for platforms actually tested.

`simpleutils` itself is always a static archive. This does not force OpenSSL or the
C++ runtime to be statically linked. To request static OpenSSL, configure with
`-DOPENSSL_USE_STATIC_LIBS=TRUE` and supply its static development libraries.

## Build, test, install

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DSIMPLEUTILS_BUILD_TESTS=ON -DSIMPLEUTILS_BUILD_EXAMPLES=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$HOME/.local"
```

If GoogleTest is not installed, pass `-DSIMPLEUTILS_FETCH_GTEST=ON` to opt in to
fetching v1.14.0 from GitHub, or use
`-DSIMPLEUTILS_GTEST_SOURCE_DIR=/absolute/path/to/googletest` for an offline build.
Tests default OFF for dependency/subproject use. No downloads occur by default.
For Visual Studio add `--config Release` to build/install and `-C Release` to CTest.

Installation produces `lib/libsimpleutils.a` (or `simpleutils.lib`), public headers
and versioned, relocatable CMake package metadata. GNUInstallDirs may choose `lib64`.

Consumer CMake:

```cmake
find_package(SimpleUtils 1 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE simpleutils::simpleutils)
```

Set `CMAKE_PREFIX_PATH` to your installation prefix. OpenSSL, Threads and Windows
system socket libraries propagate automatically. In a source-tree build use
`add_subdirectory(BackendUtils)` and the same target.

Verify installation and relocation independently:

```sh
cmake -DSOURCE_DIR="$PWD" -DBUILD_ROOT="$PWD/build-install-check" \
  -P test/verify_install.cmake
```

## Usage

```cpp
#include <simpleutils/simpleutils.h>
using namespace simpleutils;
auto words = split(trim(" one,two,three "), ",");
auto result = mapToVector(std::vector<int>{1, 2, 3}, [](int x) { return x*x; });
auto key = generateEncryptionKey();
auto ciphertext = encryptText("private text", key, "record-id");
auto plaintext = decryptText(ciphertext, key, "record-id");
auto client = Socket::connectTcp("127.0.0.1", 9000);
client.sendAll("hello");
auto reply = client.receiveExact(5);
auto response = httpRequest("http://127.0.0.1:8080/status");
```

For selective includes, use `<simpleutils/stringUtils.h>`, etc. Original short
includes such as `<stringUtils.h>` also forward to the canonical headers.
Original root files forward for source-tree users; all names now require `simpleutils::`.

## Modules and contracts

| Header | Public API |
|---|---|
| `stringUtils.h` | `toString`, `toBytes`, `ReadFile`, `ReadBinaryFile`, `WriteFile`, `ExtractPathOnly`, `ExtractFileOnly`, `trim`, `toLower`, `toUpper`, `startsWith`, `endsWith`, `replaceAll`, `split`, `join`, `splitCSV`, `parseCSV`, `parseCSVText`, `toHex`, `fromHex`, `find` |
| `bufferUtils.h` | `Buffer` data/set/append/insert/remove/slice/pad/iteration, `ByteUtils::toBuffer`, `toHexString`, `reverseByte` |
| `containerUtils.h` | `contains`, `mapToVector`, `filterToVector`, `findOptional`, `uniqueStable`, `groupBy`, `chunk`, `zip`, `keys`, `values`, `getOr` |
| `functionUtils.h` | `ScopeExit`, `scopeExit`, `compose`, `pipe`, `retry`, `Overloaded`, `Memoized`, `memoize` |
| `encryptionUtils.h` | `EncryptionAlgorithm::AES256GCM`, `EncryptionKey`, `generateEncryptionKey`, `randomBytes`, `deriveEncryptionKey`, `encrypt`, `decrypt`, `encryptText`, `decryptText`, `encryptFile`, `decryptFile` |
| `socketUtils.h` | Move-only `Socket`: `connectTcp`, `listenTcp`, `accept`, `bindUdp`, `sendTo`, `receiveFrom`, `sendAll`, `receive`, `receiveExact`, `setTimeout`, `localPort`, `valid`, `close`, `release`, `nativeHandle` |
| `httpUtils.h` | `urlEncode`, `urlDecode`, `buildQuery`, `HttpUrl`, `parseHttpUrl`, `HttpHeaders`, `HttpResponse`, `makeHttpRequest`, `parseHttpResponse`, `httpRequest` |
| `ipUtils.h` | `IPInfo`, `GetLocalIPs2`, `GetAllLocalIPs`, `isIPv4`, `isIPv6` |
| `regexUtils.h` | `validateExpression` (full match, invalid regex returns false) |
| `threadWrapper.h` | `ThreadWrapper` (joins instead of silently detaching), `join`, `joinable` |
| `linkList.h` | Owning/copyable/movable `link_list`, non-owning compatibility `link_node` |
| `sslHelper.h` | `sslHelper::generateX509`, `RSA_KEY_LENGTH` |
| `cmdUtils.h` | `TTYTerminal`, `IsInstalled`, `GetTerminal`, `getLatestClangFormat`, `formatWithClangFormat` |

### Strings, files and buffers

- Strings are binary safe; case/trim helpers deliberately use ASCII rules, not Unicode normalization.
- `ReadFile`/`WriteFile` throw on I/O errors. Writes truncate and are not atomic.
- Paths accept both `/` and `\`. No-parent `ExtractPathOnly` returns empty.
- `find` follows `string_view::find`, including empty needle = 0 and no match = npos.
- `split` retains empty entries by default, including trailing ones; empty separator is invalid.
- CSV accepts escaped quotes, embedded CR/LF inside quotes, CRLF/LF records, empty
  fields and a configurable separator. Malformed quoting throws. `splitCSV` requires
  one record; `parseCSV` reads a file; `parseCSVText` parses content.
- Hex decoding is strict: even length, no spaces or `0x` prefix.
- Buffer `Mid(first,last)` uses an exclusive end and appends to the destination.
  All ranges are checked. `removeFrom` clamps count at the end, rejects negatives.
  `pad` updates data and returns it. Iteration visits an initial snapshot so callbacks
  may resize the buffer; indexes refer to that snapshot. Buffer is not synchronized.

### Containers and functions

`mapToVector`/`filterToVector` always return vectors. `findOptional` returns a value
copy. `uniqueStable` preserves the first occurrence (quadratic equality search).
`groupBy` and dictionary keys use map order. `zip` stops at the shorter input.
`chunk` rejects zero size. `getOr` never inserts.
`compose(f,g)(args...)` calls f(g(args...)); `pipe` reverses that order.
Scope-exit callbacks must not throw; guards can be moved or released.
`retry<Exception>(attempts, callable)` retries only the chosen exception type and
rethrows the final failure. Side effects must be safe to repeat.
`memoize<int(int,int)>(callable)` owns a per-object ordered cache. Arguments must be
copyable/orderable, return values copyable, and cached functions should be pure.
Exceptions are not cached. `clear()` and `cacheSize()` manage the cache. No locking.

### Encryption

AES-256-GCM uses random 12-byte nonces and 16-byte authentication tags through
OpenSSL EVP. Binary output is `SU01 | nonce[12] | tag[16] | ciphertext`.
Text output is a hex representation of that envelope. AAD is authenticated but not
stored: pass the identical AAD to decrypt. Any altered payload/nonce/tag, wrong key
or wrong AAD fails authentication. The version header is checked before decrypting.

Store generated 32-byte keys separately from ciphertext. Password derivation uses
PBKDF2-HMAC-SHA256, a caller-stored random salt of at least 16 bytes, and configurable
iterations (default 600000, minimum 100000); store the salt and iterations with your
application metadata. No hardcoded password or key. File APIs read entire files into
memory, authenticate before writing decrypted output, and throw for unsupported
algorithms. Output writes are not atomic. Temporary plaintext is cleared in the
binary decrypt error path, but caller-owned strings and key storage are not secure
memory containers; the caller controls their lifetime.

### Sockets and HTTP

"Raw socket" here means direct OS TCP/UDP socket utilities, not privileged
`SOCK_RAW` packet crafting. IPv4 and IPv6 are supported. Owners must not be shared
across concurrent close/move/accept operations. Adopted native handles are owned;
`release` transfers ownership. `close` is idempotent. TCP receive empty = EOF;
UDP empty datagrams are valid. Oversize UDP datagrams are consumed and rejected,
not silently truncated. `receiveExact` throws on early EOF. Defaults do not impose
timeouts on listeners/datagrams until `setTimeout` is called. Accepted sockets
should have their timeout set explicitly.

Connect timeout is per resolved address; DNS resolution is synchronous and not
bounded by it. Send/receive timeouts apply per OS operation, not as a total request
deadline. HTTP requests ask for connection-close and read until EOF with a response
size cap. HTTP supports binary bodies, length/close/chunked framing and trailers;
invalid framing, duplicate length/transfer headers and header injection are rejected.
Parsed headers are lowercase and repeated non-framing headers are comma-joined;
this is not a cookie-management API. `parseHttpResponse(..., ..., true)` handles
HEAD responses without expecting the advertised body. URL parsing recognizes HTTP
and HTTPS, but the network helper supports **HTTP only**; HTTPS throws before I/O.
No TLS client, redirects, decompression, cookies, informational-response chains,
HTTP/2 or persistent connections. Certificate generation is separate from transport.

### Other utilities and migration

- Removed accidental wxWidgets coupling: `GetAllLocalIPs` returns `vector<string>`.
  Interface enumeration throws on OS errors. `IPInfo::hostName` is empty to avoid
  blocking reverse-DNS lookups.
- `ThreadWrapper` forwards callables/arguments and joins on destruction. Do not
  destroy it from its own worker thread. It no longer detaches by default.
- The original list leaked nodes and linked incorrectly; the replacement owns
  values via `std::list` and consistently inserts at the front.
- `GetTerminal` only detects; it no longer launches a terminal. `IsInstalled` never
  interpolates names into a shell. Formatter calls use isolated temporary files
  and shell-free process launch on POSIX; formatting currently throws on Windows.
- `sslHelper::generateX509` returns bool, generates a self-signed RSA-4096 certificate
  with matching DNS/IP SAN and randomized serial number. POSIX private-key mode is
  0600; Windows permissions follow directory ACLs. Development self-signed
  certificates do not establish public trust. Failed file writes may leave partial files.

## Tests

GoogleTest covers every module's main API, negative inputs, binary/empty data,
all byte values for reverse/hex operations, each envelope byte's tamper detection,
crypto interoperability, local TCP/UDP/HTTP exchanges, timeout/EOF behavior,
certificate/key matching and SAN, safe command execution and multi-translation-unit
header linking. IPv6 loopback and interface enumeration may explicitly skip when
those OS capabilities are unavailable. Installation/relocation is checked by a
separate real consumer. This is broad regression coverage, not a claim to enumerate
all possible inputs or validate every operating system. See VALIDATION.md.
