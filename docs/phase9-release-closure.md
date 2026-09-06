<!-- SPDX-License-Identifier: MPL-2.0 -->

# Phase 9 release closure

Status: COMPLETE and ready for owner release approval. No release has been published.

## Frozen release candidate

- Package and library: 0.4.0
- Public API: 1.3.0
- Provider SPI: 2.4
- License: MPL-2.0
- Distribution: static SDKs only
- Providers: RetroZilla NSS, Schannel, and OpenSSL 3.5.8
- Combined Schannel/OpenSSL SDK: official optional package, not the default recommendation

The five ZIPs in `dist/packages/0.4.0` are the immutable candidate artifacts. `SHA256SUMS-packages.txt` is the publication checksum candidate; it is a checksum record, not a signature.

## Validation result

Package integrity, internal hashes, corresponding source, licensing boundaries, isolated package consumers, Windows NT 4.0 SP6 x86 TLS 1.2 and TLS 1.3, Windows 10 build 19045 x64 Schannel TLS 1.2, OpenSSL TLS 1.2/TLS 1.3/SYSTEM_TRUST, Combined provider selection, and runtime DLL provenance passed.

The NT4 evidence is `REAL_NT4_PACKAGE_RUNTIME=PASS`. It does not claim a fresh NT4 installation. No additional fresh-install NT4 gate is required by the frozen release policy.

The isolated-package tests were followed by clean-machine validation on a separate physical Windows 10 Pro 22H2 x64 machine, build 19045.6332, without a PST checkout. The preserved evidence supports:

```text
CLEAN_MACHINE_X64=PASS
STAGING_REPRODUCIBILITY=PASS
CANONICAL_PACKAGE_REPRODUCTION=PASS
ARCHIVE_BYTE_REPRODUCIBILITY=PASS
SIGNED_RELEASE=NO
RELEASE_STATUS=READY_FOR_OWNER_RELEASE_APPROVAL
```

## Deployment prerequisites

- NT4/NSS: Windows NT 4.0 SP6 x86; deploy the SDK-provided NSS/NSPR runtime beside the executable. No NSS DLL installation in SYSTEM32 is required.
- Schannel: a compatible Windows x64 system providing Schannel, CryptoAPI/CNG, and Winsock; consumers built with `/MD` require the applicable Microsoft Visual C++ runtime. OpenSSL is not required.
- OpenSSL: the package-provided `libssl-3-x64.dll` and `libcrypto-3-x64.dll` beside the executable, compatible Windows system APIs, and the applicable Microsoft Visual C++ runtime.
- Combined: the two package-provided OpenSSL DLLs plus the Windows components required by Schannel and the applicable Microsoft Visual C++ runtime.

Microsoft system and runtime DLLs are not redistributed by PST.

## Licensing and security

PST-authored files use MPL-2.0. NSS/NSPR historical notices and corresponding source are preserved. OpenSSL is accompanied by its Apache-2.0 license and provenance. Windows components are not redistributed. The final archive scan found no private keys, tokens, passwords, dumps, logs, IDE-user files, or absolute personal paths. Public test strings and third-party API constants matched by broad secret patterns are fixtures, not credentials.

## Validated claims and limitations

The candidate evidence supports TLS 1.2 with all three providers in the tested environments, TLS 1.3 with RetroZilla NSS on NT4 SP6 x86, TLS 1.3 with OpenSSL on Windows 10 build 19045 x64, the documented trust modes, and the validated Combined selection matrix. It does not claim Windows 11 or Windows Server validation, FIPS status, formal verification, universal Windows compatibility, DLL ABI distribution, bit-for-bit reproducibility, or release signing.

## Draft release plan

- Proposed tag: `v0.4.0` (the repository currently has no existing tag convention).
- Proposed title: `PapinhoSecureTransport 0.4.0`.
- Release status before owner approval: candidate; keep artifact filenames at version 0.4.0.
- Assets: the source ZIP, four binary SDK ZIPs, and `SHA256SUMS-packages.txt`.
- Publication must explain static-only distribution, API 1.3.0, SPI 2.4, MPL-2.0 corresponding source, validated targets, prerequisites, Combined's optional status, lack of signing, and the limitations above.

Draft notes: PapinhoSecureTransport 0.4.0 is the first stabilized static-SDK candidate. It provides provider-neutral secure transport through RetroZilla NSS for Windows NT 4.0 x86, Schannel for modern Windows x64, OpenSSL 3.5.8 for modern Windows x64, and an optional Combined SDK. The release includes the MPL-2.0 source package and provider licensing/provenance records. Validation covers the precise environments and protocol combinations listed above; other Windows versions remain unclaimed until tested.

## Clean-machine closure

The clean machine compiled and linked consumers only against the extracted Schannel, OpenSSL, and Combined SDKs. Schannel TLS 1.2, OpenSSL TLS 1.2/TLS 1.3, OpenSSL TLS 1.3 SYSTEM_TRUST with graceful shutdown, all required Combined selection cases, package hashes, dependency inspection, OpenSSL DLL provenance, restricted runtime PATH, and absence of checkout/staging/build/global-OpenSSL dependencies passed.

The online shutdown investigation remains classified `EXPECTED_PEER_BEHAVIOR`: TCP EOF without reciprocal `close_notify` remains `PST_RESULT_TRUNCATED` and was not converted to success. The final Cloudflare HTTP/1.1 `Connection: close` TLS 1.3 SYSTEM_TRUST gate observed both alerts and `SHUTDOWN=COMPLETE` without truncation. Detailed internal evidence is under `docs/codex/release-evidence/0.4.0/clean-machine-x64/`.

Phase 9.G-R1 completed the canonical package builder gate. The immutable historical candidates and their frozen hashes remain selected. No technical, documentation, licensing, provenance, packaging, security, or validation blocker remains for owner release approval.

ADR-0002 target naming compliance remains a future controlled migration. It does not rename or invalidate the frozen 0.4.0 targets.
No commit, tag, signing, push, GitHub release, upload, or publication was performed.
