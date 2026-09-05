<!-- SPDX-License-Identifier: MPL-2.0 -->

# Release validation kit

Status: Phase 9.F is in progress. Offline package validation and isolated host consumers are reproducible; final execution of the extracted NSS SDK on a real Windows NT 4.0 SP6 x86 system remains mandatory before closure.

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
| clean-machine Windows x64 | NOT_PERFORMED |
| isolated package runtime | PASS |
| NSS package host TLS 1.2 | PASS - TLS=0x0303, WRITE=25, READ=25, CONTENT_MATCH=1 |
| NSS package host TLS 1.3 | PASS - TLS=0x0304, WRITE=25, READ=25, CONTENT_MATCH=1 |
| Schannel package functional TLS 1.2 | PENDING |
| OpenSSL package functional TLS 1.2/1.3 | PENDING |
| Combined package AUTOMATIC/EXACT/ORDERED functional matrix | PENDING |
| real NT4 TLS 1.2 and TLS 1.3 from release ZIP | PASS - Windows NT 4.0 SP6 x86 |

## Prepare the NT4 transfer directory

After running the main validator, build the public TLS fixture and copy runtime files exclusively from the extracted ZIPs:

    powershell -NoProfile -ExecutionPolicy Bypass -File tools/prepare-nt4-release-validation.ps1

The output is dist/validation/0.4.0/nt4-transfer. Add only freshly generated canonical test credentials before transfer; they are intentionally absent from the versioned kit.

## Real NT4 gate

Transfer the unmodified NSS ZIP and its externally recorded SHA-256 to Windows NT 4.0 SP6 x86. Extract it into a new directory using an NT4-compatible extractor. Do not copy files from the checkout or staging tree into that directory. Deploy the contents of runtime/windows-nt4-x86-vc6-retrozilla-nss beside the validation executable.

The real package-derived run was performed on Windows NT 4.0 SP6 x86. The modern fixture was 172.16.0.1 and the NT4 client was 172.16.0.11. TLS 1.2 negotiated 0x0303 and TLS 1.3 negotiated 0x0304. Both runs reported WRITE=25, READ=25, CONTENT_MATCH=1, ALPN=9, AUTH=2, and the release-specific PASS marker. Both servers reported AUTH=True, ALPN=fixture/1, RECV=25, SEND=25, and CONTENT_MATCH=True. The supplied captures also showed incremental readiness and provider shutdown. NSS/NSPR DLLs remained beside the executable; none was installed in SYSTEM32. Evidence files produced on NT4 were tls12-client.log, tls13-client.log, tls12-modules.log, and tls13-modules.log.

The host-side isolated run is not a clean-machine result and is not a substitute for this NT4 gate. Phase 9.G has not started.

## Current closure decision

The package hashes, extraction, internal hashes, licensing/source boundaries, four minimal public consumers, NSS host TLS 1.2/TLS 1.3, and real NT4 package TLS 1.2/TLS 1.3 are PASS. Phase 9.F remains BLOCKED only on the remaining x64 provider-functional package matrix. No package or production source was modified, and Phase 9.G remains NOT STARTED.
