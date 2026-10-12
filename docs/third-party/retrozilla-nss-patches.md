<!-- SPDX-License-Identifier: MPL-2.0 -->

# RetroZilla NSS/NSPR local patch history

## Governed upstream identity

PST uses RetroZilla revision `2f274574d3c6ee8769914046920d649bbae9f81b`,
NSS 3.42 Beta and NSPR 4.7.7. The governed source archive is
`third_party/retrozilla-nss/source/retrozilla-2f274574d3c6ee8769914046920d649bbae9f81b-patched.zip`,
SHA-256 `5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388`.
It preserves the original RetroZilla notices, NSS MPL-2.0 files and NSPR
license evidence. Distribution of the runtime is accompanied by this source,
the ordered patches and the rebuilding documentation.

## Patch inventory

### 0001 — Win32 secure RNG fail closed

`0001-win32-secure-rng-fail-closed-nt4.patch` changes
`security/nss/lib/freebl/win_rand.c`. It preserves SystemFunction036 and
CryptoAPI as entropy sources but fails closed when neither source succeeds.
Its runtime output is `freebl3.dll` plus its integrity `.chk`. This patch is
already present in the governed source ZIP and must not be applied twice.

### 0002 — reciprocal shutdown and `pendingBuf`

`0002-close-notify-pendingbuf-shutdown.patch` changes `sslimpl.h` and
`sslsecur.c`. The historical `ssl_SecureShutdown` discarded the result of
`SSL3_SendAlert`; in nonblocking operation the alert plaintext could be
accepted while encrypted output remained in `pendingBuf`, after which native
SEND shutdown could occur too early.

The correction records that `close_notify` was generated, drains pending
ciphertext incrementally, propagates `PR_WOULD_BLOCK_ERROR`, prevents duplicate
alerts and marks SEND shutdown only after the lower shutdown succeeds. The PST
adapter maps this factual would-block result to `NEED_WRITE` and WRITE interest.
It does not fabricate completion and does not change Model E ownership.

The qualification patch with its original audit paths is preserved under
`docs/release-evidence/0.6.5/nss-close-notify-pendingbuf/qualified-original.patch`
with SHA-256 `ee6a127d06e59351cefdb63ce8b85312ff99cc773ac39910739ecae1c097e65f`.
The repository patch has portable paths and SHA-256
`fab7d1e7ea696339ecd75b9f5265017e0248728183281eb0f76cf9319b03451f`;
the code hunks are equivalent.

## Build and binary identity

The qualified build used Microsoft Visual C++ 6.0 SP6 (`cl 12.00.8804`),
linker `6.00.8447`, Processor Pack/i586, Win32 subsystem 4.0 and the historical
MozillaBuild/MSYS make environment. NSS uses its established `/MD` runtime;
PST consumer libraries remain separately built as `/ML` and `/MD`.

The previous `ssl3.dll` SHA-256 was
`a45adb3ca8abfab4716315acba515f0de1ad4c56b095c89f32042dfc120277da`.
The qualified candidate is
`68f20ede3c71cbaa132ef9e1da9f54ccf8fa5d96da761e29892070511fed0fce`.
It remains i386, retains 107 exports and the expected imports, section layout,
CRT dependency and NT4-compatible subsystem. Rebuilds can differ in PE/COFF
timestamps and embedded source-path capitalization; functional equivalence is
not described as byte identity.

## Qualification evidence

Real TLS 1.2 and TLS 1.3, mTLS, encrypted I/O, partial output, repeated factual
`NEED_WRITE`, WRITE interest, output drain, exactly one local alert, reciprocal
shutdown, cancel while pending, clean close, reset/truncation and data before
abrupt EOF passed. API 3/SPI 4 Model E and layered-close contracts also passed.
The real Windows NT 4.0 SP6 x86 matrix passed with PST `/ML` and `/MD`.

The returned NT4 archives did not repeat loaded-module hashes inside NT4.
Identity is derived from the frozen R3 kits whose candidate runtime was hashed
before transfer. This limitation is part of the evidence and is not elided.

## Apply, rebuild, test and revert

1. Verify the source ZIP and `patches/MANIFEST.sha256`.
2. Extract the ZIP. Do not reapply patch 0001.
3. Apply patch 0002 from the extraction root with `patch -p1`.
4. Verify the resulting `sslimpl.h` and `sslsecur.c` hashes from the manifest.
5. Use the VC6/MozillaBuild recipe in [build-retrozilla-nss.md](../build-retrozilla-nss.md).
6. Verify architecture, imports, all 107 exports and the runtime SHA-256.
7. Run NSS unit/integration, backpressure, shutdown, truncation and Model E
   gates before replacing the repository runtime.

Reversion means restoring the prior governed runtime and removing patch 0002
from the declared patch order in one reviewed change. Never edit a distributed
source ZIP or runtime DLL in place. A future patch update receives a new file,
hash, output hashes and qualification evidence.

Related documents: [ADR-0008](../adr/ADR-0008-lifecycle-failure-aware-de-fechamento-nativo.md),
[NSS backend](../backend-nss.md), [release packaging](../release-packaging.md),
[release licensing](../release-licensing.md), and [target matrix](../target-matrix.md).
