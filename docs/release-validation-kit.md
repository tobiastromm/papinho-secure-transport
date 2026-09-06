<!-- SPDX-License-Identifier: MPL-2.0 -->

# Release validation kit

Status: Phase 9.F complete. Offline package validation, isolated host consumers, real provider TLS, public selection, online system trust, and final NT4 package validation passed.

## Source of truth

The validator reads only the five immutable 0.4.0 ZIPs under dist/packages/0.4.0. It recreates dist/validation/0.4.0, extracts each package, verifies its external and internal SHA-256 records, checks package boundaries and corresponding source, and compiles minimal consumers against extracted public headers and static libraries. It never reads dist/staging.

Run from an ordinary PowerShell prompt:

    powershell -NoProfile -ExecutionPolicy Bypass -File tools/validate-release-packages.ps1 -CompileConsumers

The consumer includes only papinho_secure_transport.h and papinho_secure_transport_win32.h. VC6 and modern MSVC environments come from the documented repository bootstraps; all PST headers, libraries and provider runtime files come from the extracted SDK.

## Frozen package hashes

| Package | SHA-256 |
|---|---|
| source | 8d20b8975c06029fc828a42cdb6c75dc96c74ee1e0dedab867077fbf21cc223d |
| NSS | 55e38a19d743849317cab919dc7ba682ee823137666ee60004cf562e1788bed4 |
| Schannel | 044f22dc2eac6ef82a53a096f0a27d1ce6719bb7d7798d71b6ebaa648f53eda8 |
| OpenSSL | a59847425895dad2b3b8bb96ae40b70e2698f3a02d1812c0f8de40404953741f |
| Combined | 8e78dd36f6f9486c7eea50df093a74c3cd2838b0a1823b8f9c406f3d496a0010 |

## Validation matrix

| Gate | Current result |
|---|---|
| five external hashes and extraction | PASS |
| internal SHA256SUMS coverage | PASS |
| source package and legacy corresponding source | PASS |
| four SDK structural/license/source boundaries | PASS |
| extracted-package compile/link/run on host | PASS |
| clean-machine Windows x64 | PASS - separate physical Windows 10 Pro 22H2 x64, build 19045.6332, no PST checkout |
| isolated package runtime | PASS |
| NSS package host TLS 1.2 | PASS - TLS=0x0303, WRITE=25, READ=25, CONTENT_MATCH=1 |
| NSS package host TLS 1.3 | PASS - TLS=0x0304, WRITE=25, READ=25, CONTENT_MATCH=1 |
| Schannel package functional TLS 1.2 | PASS |
| OpenSSL package functional TLS 1.2/1.3 | PASS |
| OpenSSL TLS 1.3 SYSTEM_TRUST online | PASS - www.cloudflare.com |
| Combined package AUTOMATIC/EXACT/ORDERED functional matrix | PASS |
| real NT4 TLS 1.2 and TLS 1.3 from release ZIP | PASS - Windows NT 4.0 SP6 x86 |

## Prepare the NT4 transfer directory

After running the main validator, build the public TLS fixture and copy runtime files exclusively from the extracted ZIPs:

    powershell -NoProfile -ExecutionPolicy Bypass -File tools/prepare-nt4-release-validation.ps1

The output is dist/validation/0.4.0/nt4-transfer. Add only freshly generated canonical test credentials before transfer; they are intentionally absent from the versioned kit.

## Real NT4 gate

Transfer the unmodified NSS ZIP and its externally recorded SHA-256 to Windows NT 4.0 SP6 x86. Extract it into a new directory using an NT4-compatible extractor. Do not copy files from the checkout or staging tree into that directory. Deploy the contents of runtime/win32-x86-vc6-retrozilla-nss beside the validation executable.

The real package-derived run was performed on Windows NT 4.0 SP6 x86. The modern fixture was 172.16.0.1 and the NT4 client was 172.16.0.11. TLS 1.2 negotiated 0x0303 and TLS 1.3 negotiated 0x0304. Both runs reported WRITE=25, READ=25, CONTENT_MATCH=1, ALPN=9, AUTH=2, and the release-specific PASS marker. Both servers reported AUTH=True, ALPN=fixture/1, RECV=25, SEND=25, and CONTENT_MATCH=True. The supplied captures also showed incremental readiness and provider shutdown. NSS/NSPR DLLs remained beside the executable; none was installed in SYSTEM32. Evidence files produced on NT4 were tls12-client.log, tls13-client.log, tls12-modules.log, and tls13-modules.log.

The host-side isolated run is not a clean-machine result and is not a substitute for this NT4 gate.

## Current closure decision

The package hashes, extraction, internal hashes, licensing/source boundaries, four minimal public consumers, NSS host and real NT4 TLS 1.2/TLS 1.3, Schannel TLS 1.2, OpenSSL TLS 1.2/TLS 1.3, online OpenSSL TLS 1.3 SYSTEM_TRUST, Combined public selection, DLL provenance, canonical package reproduction, and clean-machine x64 execution are PASS. No package or production source was modified. Phase 9.G and Phase 9 are complete and ready for owner release approval; publication has not occurred.
## X64 package execution evidence

All functional consumers were compiled with /MD /W4 from test sources in the extracted source ZIP, using public headers and static libraries only from the applicable extracted SDK. The initial commands produced C4996 warnings for legacy CRT calls; recompilation used the build policy _CRT_SECURE_NO_WARNINGS and completed with zero warnings. No build output or dist/staging path was an input.

Schannel TLS 1.2 passed with backend schannel, TLS 0x0303, custom trust, hostname and peer authentication, ALPN fixture/1, WRITE=25, READ=25, CONTENT_MATCH=1, and clean shutdown.

OpenSSL TLS 1.2 and TLS 1.3 passed with backend openssl, TLS 0x0303 and 0x0304 respectively, custom trust, hostname and peer authentication, ALPN fixture/1, WRITE=25, READ=25, CONTENT_MATCH=1, and clean shutdown.

OpenSSL TLS 1.3 SYSTEM_TRUST passed online against www.cloudflare.com at 104.16.124.96. The result reported TLS 0x0304, CHAIN=1, HOSTNAME=1, AUTH=1, and no application bytes. The bounded shutdown ended with the already-modeled remote shutdown result after the authenticated handshake; the identity/system-trust gate passed.

The Combined public matrix passed: TLS 1.2 SYSTEM_TRUST AUTOMATIC selected schannel; TLS 1.3 SYSTEM_TRUST AUTOMATIC selected openssl; EXACT openssl TLS 1.3 passed; EXACT schannel TLS 1.3 returned UNSUPPORTED; ORDERED openssl then schannel for TLS 1.2 selected openssl.

With PATH restricted to the executable directory and Windows system directories, the loaded paths were dist/validation/0.4.0/x64-openssl/libssl-3-x64.dll and libcrypto-3-x64.dll. Their SHA-256 values matched the extracted SDK files: libssl 3fb3cd7804dbe3216c801b470e14461d80214ece99c637ae42ea3d8caf75d7ed and libcrypto 09eec573c9adea156ba2073f8cd61720d0aabeb7562d8498b4ecd21b710a3044. Dumpbin showed no OpenSSL dependency for Schannel and the expected OpenSSL dependencies for OpenSSL and Combined.
## Phase 9.G clean-machine status

Phase 9.G first preserved the distinction between isolated-package and clean-machine evidence. External execution then passed on a separate physical Windows 10 Pro 22H2 x64 machine, build 19045.6332, without a PST checkout. Consumers compiled and linked from extracted SDKs; Schannel TLS 1.2, OpenSSL TLS 1.2/TLS 1.3/SYSTEM_TRUST graceful shutdown, Combined AUTOMATIC/EXACT/ORDERED, module provenance, dependency inspection, restricted PATH, and absence of checkout, external staging/build, and global OpenSSL dependencies passed. Internal audit evidence is preserved under `docs/codex/release-evidence/0.4.0/clean-machine-x64/` and remains excluded from the public source package.

OpenSSL EOF without reciprocal `close_notify` remains FAILED/TRUNCATED and is classified expected peer behavior, not success. The final online gate used Cloudflare with HTTP/1.1 `Connection: close`, TLS 1.3, SYSTEM_TRUST, and required graceful shutdown; reciprocal alerts were observed and shutdown completed without truncation.
