<!-- SPDX-License-Identifier: MPL-2.0 -->

# SS-8 package candidate validation

Status: complete. Same-host isolated package gates, separate clean-machine execution, package-only Combined real TLS, and the real Windows NT 4.0 SP6 x86 package retest pass. This record is excluded from the source package by the established `docs/codex` policy.

## Candidate set

Package/Library `0.5.0`, public API `2.0.0`, provider SPI `3.0`.

| Package | SHA-256 |
|---|---|
| `papinho-secure-transport-0.5.0-src.zip` | `4df9945b0a219c9a04bc9429faeeeb10fb449fcc945bfdcbd6b1fe605e53c673` |
| `papinho-secure-transport-0.5.0-win32-x86-vc6-retrozilla-nss.zip` | `91d004e113c662386bc947658d0a11d7c3022066b647a8d361ba2c862de39a9f` |
| `papinho-secure-transport-0.5.0-win32-x64-msvc-19.51-schannel.zip` | `6e9ed7467051c2946ebc146f8fec49724759ed102b9e733191f0345125ce8a63` |
| `papinho-secure-transport-0.5.0-win32-x64-msvc-19.51-openssl3.zip` | `625959d8fd73cd3a73ea0f6ee23761fb19a1b20899016e3cb74f5aa9388b6635` |
| `papinho-secure-transport-0.5.0-win32-x64-msvc-19.51-schannel-openssl3.zip` | `19c43680e981a01cbd48a486fd12c53d5c50167d42b9f2cebf9955fa7c823707` |

`SHA256SUMS-packages.txt` SHA-256: `900ba5d74d5c9c8a84a9fe7c2464891ef17000a0df3ded8384a2e34958f0cae1`.

These are the final SS-8D documentation/package-layout candidates. Their PST libraries and provider runtimes are byte-identical to the SS-8 clean-machine/NT4-tested inputs; only source documentation, public examples, SDK documentation/manifests and their internal checksum records changed.

## Development-host evidence

- Canonical build plus two independent reproduction outputs produced byte-identical ZIP SHA-256 values.
- All five archives passed duplicate/traversal checks, clean extraction, complete internal SHA-256 coverage, package boundary checks, licensing and exact corresponding-source references.
- Four extracted SDKs compiled, linked and ran separate API 2.0 CLIENT and SERVER consumer programs (`CONSUMER_COUNT=8`) using package include/library/runtime inputs.
- The same validation passed from `C:\PST-SS8-Isolated-0.5.0`, outside the repository. This is isolated same-host evidence, not clean-machine evidence.
- Package OpenSSL DLL hashes equal the retained 3.5.8 runtime hashes. All ten packaged NSS/NSPR runtime files equal their versioned runtime inputs. No unexpected object/debug/PST DLL artifact occurs in an SDK.
- VC6 portable and NSS unit suites passed with `/W4` and zero warnings. OpenSSL and Combined modern suites passed with `/W4` and zero warnings.
- A Schannel CLIENT was compiled outside the checkout with `/W4` and zero warnings using only the extracted SDK headers/library plus documented Windows libraries. A real TLS 1.2 CUSTOM_TRUST run completed incremental handshake, `WRITE=25`, `READ=25`, `CONTENT_MATCH=1`, ALPN `fixture/1`, authenticated Peer Info and reciprocal shutdown. The fixture source/PKI were explicit validation inputs; runtime/provider resolution came from the package-linked consumer and Windows system libraries.
- Prepared external bundles: `build/ss8-clean-machine-validation` contains the five frozen ZIPs, checksum manifest, hash-first runner and instructions; `build/ss8-nt4-package-validation` contains an executable linked from the extracted NSS SDK library, package runtime DLLs, fixtures, NT4-compatible runners and its own manifest.
- Real package-level TLS tests ran from `C:\PST-SS8-Isolated-0.5.0`, using extracted SDK headers, libraries and adjacent runtimes rather than development build outputs:
  - NSS CLIENT TLS 1.2 and TLS 1.3 passed with REQUIRED mTLS, CUSTOM_TRUST, ALPN `fixture/1`, incremental readiness, `WRITE=25`, `READ=25`, content match and reciprocal shutdown.
  - NSS SERVER TLS 1.2 and TLS 1.3 passed; TLS 1.2 and TLS 1.3 REQUIRED mTLS, bidirectional I/O and reciprocal shutdown are covered. The TLS 1.2 REQUIRED run executed from `C:\PST-SS8-Isolated-0.5.0\nss-server-real` and recorded every NSS/NSPR module under that directory.
  - OpenSSL CLIENT and SERVER TLS 1.2/TLS 1.3 passed with CUSTOM_TRUST, SYSTEM_TRUST where advertised, REQUIRED mTLS, SERVER-preference ALPN, bidirectional I/O and reciprocal shutdown.
  - Combined CLIENT passed with EXACT Schannel/TLS 1.2 and EXACT OpenSSL/TLS 1.3. Combined SERVER passed EXACT Schannel, EXACT OpenSSL, ORDERED in both provider orders, AUTOMATIC, ALPN-driven pre-binding filtering to OpenSSL, and no post-binding fallback after an OpenSSL selection.
- The live OpenSSL module proof resolved `libssl-3-x64.dll` and `libcrypto-3-x64.dll` from `C:\PST-SS8-Isolated-0.5.0\openssl-real`. Their SHA-256 values exactly matched the extracted OpenSSL SDK runtime (`3fb3cd7804dbe3216c801b470e14461d80214ece99c637ae42ea3d8caf75d7ed` and `09eec573c9adea156ba2073f8cd61720d0aabeb7562d8498b4ecd21b710a3044`). The Combined runtime copies matched the same package bytes.
- `dumpbin /dependents` confirmed that the Schannel consumer has no OpenSSL import; the OpenSSL and Combined executables import the expected package OpenSSL DLL names. The active runtime logs contain zero development-tree runtime paths and zero private-key disclosure hits. `ROOT_RESIDUE=0` and `CA_RESIDUE=0` after the tests.
- A final validation of the unchanged candidate ZIPs passed archive/layout/internal-hash/licensing/source checks plus compile, link and runtime execution for all eight extracted-package CLIENT/SERVER consumers (`CONSUMER_COUNT=8`).
- SS-8R1 corrected the clean-machine runner contract: VC6 availability is probed before consumer compilation. With VC6 unavailable, NSS still passes hash, layout, extraction, internal SHA-256, license, corresponding-source and archive checks, reports `NSS_CONSUMER_COMPILE_LINK=NOT_APPLICABLE_TOOLCHAIN_UNAVAILABLE`, and does not fail the modern clean-machine run. The six modern CLIENT/SERVER consumers then compile, link and execute (`CONSUMER_COUNT=6`, `RESULT=PASS`). The package-linked public Combined selection test covers AUTOMATIC, EXACT and ORDERED and reports `COMBINED_SELECTION=PASS`. A complementary host run with VC6 available still executes all eight consumers and reports `NSS_CONSUMER_COMPILE_LINK=PASS`.
- The runner resolves the active `cl.exe` after the documented MSVC bootstrap and reads its locale-independent `FileVersionInfo.FileVersion`, with a numeric-only output fallback. It reports `CLEAN_MACHINE_MSVC=<actual cl version>` and never relabels a compatible compiler as 19.51. It also compares CurrentUser Root/CA counts before and after execution and requires `TRUST_STORE_CLEANUP=PASS`. The corrected bundle is `build/ss8-clean-machine-validation`; its `TRANSFER-SHA256SUMS.txt` SHA-256 is `da6af1ea8ab5eaccaed1148cbee9a8f213d8e27ca7ff144229a3ba6c97a279b1`.
- Bundle `work` is generated output, not a transfer input. A stale sentinel was placed in it before validation and was absent after runner startup, proving that the directory is removed and recreated from the transferred ZIPs. The post-validation `work` directory was removed; the final bundle contains no temporary output and every immutable input is covered by `TRANSFER-SHA256SUMS.txt`.
- Owner clean-machine evidence on Windows 10 Pro 19045 x64 passed Schannel TLS 1.2 CUSTOM_TRUST; OpenSSL CLIENT TLS 1.2/TLS 1.3; OpenSSL SERVER TLS 1.2/TLS 1.3 with mTLS and ALPN; package-local OpenSSL DLL hash verification; and unchanged CurrentUser trust-store counts (`Root 47 -> 47`, `CA 10 -> 10`). `SS8_REAL_TLS_CLEAN_MACHINE_RESULT=PASS` for those gates.
- A dedicated package-only Combined runner was then added and validated locally against only the extracted Combined SDK plus source-package harness/fixture inputs. It passes CLIENT EXACT Schannel TLS 1.2, CLIENT EXACT OpenSSL TLS 1.2/TLS 1.3, SERVER EXACT Schannel/OpenSSL, both ORDERED provider orders, AUTOMATIC, ALPN pre-binding filtering to OpenSSL, and OpenSSL-bound truncation with no post-binding fallback. Every positive case reports 25-byte bidirectional I/O and reciprocal shutdown; package-local OpenSSL module paths/hashes and `COMBINED_LOG_SECRET_HITS=0` pass.

## Owner clean-machine evidence

- The owner executed the transferred validation bundle on a separate clean Windows 10 Pro 19045 x64 machine with MSVC `19.44.35228`. All five package hashes, layouts, clean extractions, internal checksums, licensing/source/archive checks passed. VC6 was unavailable as expected, so the NSS consumer compile/link gate was correctly reported as `NOT_APPLICABLE_TOOLCHAIN_UNAVAILABLE`; all six modern CLIENT/SERVER package consumers and public Combined selection passed.
- Schannel TLS 1.2 CUSTOM_TRUST, OpenSSL CLIENT TLS 1.2/TLS 1.3, and OpenSSL SERVER TLS 1.2/TLS 1.3 with mTLS and ALPN passed. The CurrentUser trust-store counts remained unchanged (`Root 47 -> 47`, `CA 10 -> 10`).
- The owner then executed the package-only Combined runner. Its eleven real-network cases passed: CLIENT EXACT Schannel TLS 1.2; CLIENT EXACT OpenSSL TLS 1.2/TLS 1.3; SERVER EXACT Schannel TLS 1.2 and OpenSSL TLS 1.2/TLS 1.3; ORDERED in both directions; AUTOMATIC; ALPN-driven pre-binding filtering to OpenSSL; and OpenSSL-bound truncation with no post-binding fallback. Positive cases proved 25-byte bidirectional encrypted I/O and reciprocal shutdown.
- Combined loaded `libssl-3-x64.dll` and `libcrypto-3-x64.dll` only from `C:\ss8-clean-machine-validation\combined-real-tls\bin`. Their SHA-256 values matched the extracted Combined SDK: `3fb3cd7804dbe3216c801b470e14461d80214ece99c637ae42ea3d8caf75d7ed` and `09eec573c9adea156ba2073f8cd61720d0aabeb7562d8498b4ecd21b710a3044`. `COMBINED_LOG_SECRET_HITS=0` and `COMBINED_REAL_TLS_CLEAN_MACHINE=PASS`.
- The returned clean-machine evidence archive `clean.zip` has SHA-256 `c7775accd113bcf05375ab24f118214814eab055b062b14c603386897ef1065d`.

## Owner Windows NT 4.0 evidence

- The owner executed the transferred package on real Windows NT 4.0 SP6 x86. NSS SERVER TLS 1.2 passed with `TLS=0x0303`, `READ=25`, `WRITE=25`, `CONTENT_MATCH=1`, incremental readiness and reciprocal TLS shutdown.
- NSS SERVER TLS 1.3 REQUIRED mTLS passed with `TLS=0x0304`, `READ=25`, `WRITE=25`, `CONTENT_MATCH=1`, incremental readiness and reciprocal TLS shutdown.
- The raw-abrupt case passed with `RESULT=TRUNCATED` and `CLOSE_KIND=TRUNCATED`; the backend trace records EOF without peer close_notify and no weakening to clean close.
- The screenshots identify the package runner results as `PAPINHOSECURETRANSPORT SS-8 NT4 PACKAGE TLS 1.2 PASS`, `PAPINHOSECURETRANSPORT SS-8 NT4 PACKAGE TLS 1.3 MTLS PASS`, and `PAPINHOSECURETRANSPORT SS-8 NT4 PACKAGE TRUNCATION PASS`. The returned backend logs independently record `PR_Poll` progress, reciprocal close_notify for the positive cases, and strict truncation for raw EOF.
- The returned NT4 evidence archive `Desktop.zip` has SHA-256 `469d2bfa273ecd3b563cbc1a1d43c64509f71f9c69dc27a77a7be05fbe9915e8`.

## Closure

- `CLEAN_MACHINE_SOURCE_PACKAGE=PASS`.
- `CLEAN_MACHINE_MODERN_SDKS=PASS`.
- `PACKAGE_LEVEL_REAL_TLS_SMOKES=PASS`.
- `ISOLATED_PACKAGE_RUNTIME_RESOLUTION=PASS`.
- `DEVELOPMENT_TREE_RUNTIME_DEPENDENCIES=0`.
- `COMBINED_REAL_TLS_CLEAN_MACHINE=PASS`.
- `CLEAN_MACHINE_VALIDATION=PASS`.
- `NT4_PACKAGE_RETEST_RESULT=PASS`.
- The restricted-sandbox Schannel failure is classified as `SCHANNEL_FAIL10_CLASSIFICATION=ENVIRONMENTAL_SANDBOX_RESTRICTION`. The official suite passed in the normal host user context; `SCHANNEL_X64_REGRESSION=PASS`, `PRODUCTION_FIX_REQUIRED=NO`, and `PRODUCTION_CODE_CHANGED=NO`.
- The candidate hashes above are the active set after the Combined runner addition. Because the runner is included in the official source archive, only the source ZIP and external checksum file changed. Two independent final package builds were byte-identical; all four binary SDK ZIPs retain their prior hashes. The ten-file clean-machine bundle `TRANSFER-SHA256SUMS.txt` SHA-256 is `42037d258577de714aa5f4ff6a33d6b3f2e47d81f4ef835ec5d555cdaed1a7de`.
The validated candidate ZIPs were not regenerated after owner execution. Updating `docs/roadmap.md` at this point would change the source-package input and invalidate the returned package evidence, so the SS-8 closure is recorded only in this deliberately package-excluded evidence document until the next package-affecting cycle.

No commit, push, tag, release, upload or master mutation was performed by this SS-8 closure registration.
