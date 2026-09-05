# Phase 9 release scope, version policy, and canonical inventory

Status: **Phase 9.A complete**. Phase 9 is in progress; Phase 9.B through 9.H have not started. This document defines release inputs and decisions, but does not freeze ABI/SPI, change versions, or create packages.

## Identity, purpose, and boundaries

The canonical project name is **PapinhoSecureTransport**, abbreviated **PST**. Public symbols retain the `pst_`/`PST_` naming already present.

PST is a generic secure-transport library. TLS 1.2 and TLS 1.3 are the secure-transport technologies implemented today through providers; PST does not implement cryptography, certificate validation, or a TLS state machine itself. The conceptual domain is not permanently limited to TLS, but SPI 2.4 does not support DTLS, QUIC-related transports, Noise, or other future families.

PST is application-protocol neutral. HTTP, SMTP, IMAP, enterprise protocols, and custom protocols may run above it, but PST neither parses nor implements them. It is useful across the public Internet, LANs, private/corporate networks, legacy-to-modern links, and modern client/server applications.

PapinhoBrowser and PapinhoLegacyMail are possible consumers. PapinhoAccelerator is specific to PapinhoBrowser; it is neither PST infrastructure nor a dependency of PST or PapinhoLegacyMail. SMTP, IMAP, OAuth, accounts, and provider-specific mail behavior remain outside PST.

## Release-content classification

| Repository content | Classification | Release decision input |
|---|---|---|
| `include/papinho_secure_transport.h` and target public adapter header | RELEASE SDK | Public consumer surface; final inclusion is subject to 9.B ABI audit |
| portable `src/pst_*.c` core | RELEASE RUNTIME / source release | Library implementation; not installed as headers |
| `src/pst_backend.h` and internal headers | DEVELOPMENT / TEST | Internal SPI/provider contract; not consumer SDK |
| `src/backends/*` and private platform adapters | RELEASE RUNTIME / source release | Included only in matching target/provider build |
| `Makefile.vc6`, `Makefile.msvc`, `Makefile.openssl.msvc`, `tools/*.bat` | DEVELOPMENT / TEST and source release | Canonical developer builds; packaging entry points remain 9.E work |
| `tests/` and generated test PKI | DEVELOPMENT / TEST | Not runtime material; selected validation utilities may move into 9.F packages |
| `docs/` and root README | DOCUMENTATION | Release documentation input; full EN/pt-BR restructuring belongs to 9.D |
| `third_party/*/licenses`, provenance, manifests, patches | THIRD-PARTY SOURCE / PROVENANCE | Preserve and distribute as required by the package/license review |
| preserved RetroZilla source snapshot and OpenSSL source archive | THIRD-PARTY SOURCE / PROVENANCE | Source/provenance package input, not duplicated automatically into every binary package |
| generated third-party SDK headers/import libraries | RELEASE SDK dependency | Include only for packages whose integration model requires them |
| NSS and OpenSSL runtime DLLs/check files | RELEASE RUNTIME dependency | Include only in their target packages with manifests/notices |
| Schannel/SSPI/CryptoAPI | NOT DISTRIBUTED | Supplied by Windows |
| `build/`, root object files, logs, generated certificates, temporary servers | NOT DISTRIBUTED | Reproducible/generated development output; release packages must not copy the dev tree |
| examples | DEVELOPMENT / TEST currently | No `examples/` directory exists; real release examples are required in 9.D |

No current Makefile creates a final distributable PST DLL/package. Runtime versus SDK binary layout, export artifact, debug-symbol policy, and source-package composition remain decisions for 9.E after ABI/SPI freeze.

## Providers and capabilities

Capability bits are TLS1.2 `0x001`, TLS1.3 `0x002`, CLIENT_AUTH `0x004`, ALPN `0x008`, CUSTOM_TRUST `0x010`, SYSTEM_TRUST `0x020`, HOSTNAME_VERIFY `0x040`, RESUMPTION `0x080`, EARLY_DATA `0x100`, PEER_INFO `0x200`, NONBLOCKING `0x400`, and BACKEND_WAIT `0x800`.

| Provider ID | Mask | Target and toolchain | Dependency/distribution | Current limitations and evidence |
|---|---:|---|---|---|
| `retrozilla-nss` | `0x00000e5f` | Win32 x86, VC6/C89; tested NT4 SP6 | Bundled RetroZilla NSS/NSPR DLL/CHK runtime and generated SDK | TLS1.2/1.3, custom trust, hostname, ALPN, client auth, peer info, nonblocking and backend wait; no SYSTEM_TRUST; one active provider state per process; extensive VC6, host, and real-NT4 evidence |
| `schannel` | `0x00000e7d` | modern Windows x64, modern MSVC; tested Windows 10 build 19045 | Windows Schannel/SSPI/CryptoAPI, not redistributed | TLS1.2, custom/system trust, hostname, ALPN, client auth, peer info, nonblocking and backend wait; TLS1.3 not advertised on the tested runtime only; OS-dependent capabilities |
| `openssl` | `0x00000e7f` | modern Windows x64, modern MSVC; tested Windows 10 build 19045 | Bundled OpenSSL 3.5.8 `libssl`/`libcrypto` DLLs and SDK | TLS1.2/1.3 plus both trust modes and all remaining common features; SYSTEM_TRUST is Windows-specific; exact OpenSSL 3.5.8 baseline; default provider only |

The verified three-provider intersection is `0x00000e5d`: TLS1.2, CLIENT_AUTH, ALPN, CUSTOM_TRUST, HOSTNAME_VERIFY, PEER_INFO, NONBLOCKING, and BACKEND_WAIT. SYSTEM_TRUST is common to Schannel and OpenSSL but absent from NSS. TLS1.3 is present in NSS and OpenSSL but absent from the tested Schannel capability set.

For a tested combined modern registry ordered `[schannel, openssl]`, automatic selection chooses Schannel for TLS1.2+SYSTEM and common TLS1.2 policies, and OpenSSL for TLS1.3+SYSTEM or TLS1.3+CUSTOM. Exact selection never substitutes; ordered selection follows consumer order. The combined arrangement is an official optional binary SDK candidate after the Phase 9.E decision. It is not the default or generally recommended package; separate Schannel and OpenSSL SDKs remain available. Its manifest must declare provider order and OpenSSL runtime obligations.

## Build-target matrix

| Output | OS/architecture | Toolchain/CRT | Provider set | Runtime dependencies | Validation status |
|---|---|---|---|---|---|
| `build/vc6` | legacy Win32 x86 | VC6, C89, `/W4`, legacy VC runtime model | portable core plus opt-in `retrozilla-nss` | canonical repository NSS/NSPR DLL/CHK set; Windows system DLLs | host build green; real NT4 SP6 tested |
| `build/win64-modern-msvc` | modern Windows x64 | MSVC 19.51, `/MD /W4`, SDK 10.0.26100 | Schannel | OS-provided Schannel/SSPI/CryptoAPI | Windows 10 22H2 build 19045 tested |
| `build/win64-modern-msvc-openssl` | modern Windows x64 | MSVC 19.51, `/MD /W4`, SDK 10.0.26100 | OpenSSL; Schannel linked only in coexistence tests | staged OpenSSL 3.5.8 DLLs plus Windows CryptoAPI for SYSTEM_TRUST | Windows 10 22H2 build 19045 tested |

Outputs remain separated by target. Other `build/*` directories are ignored test evidence, third-party construction, fixture, or package-preparation workspaces and are not canonical release targets.

## Third-party, provenance, and license inventory

| Component | Identity | Preserved material | License/notice evidence | Distribution status |
|---|---|---|---|---|
| RetroZilla lineage | revision record `2f274574d3c6ee8769914046920d649bbae9f81b` | exact post-patch source ZIP, ordered RNG patch, build instructions, SDK, runtime and SHA-256 manifests | original RetroZilla LICENSE/LEGAL, NSS MPL-2.0 text, NSPR header evidence, per-file source notices | NSS/NSPR runtime and relevant notices are expected in the legacy runtime package; source/notice obligations require final 9.E review |
| NSS | 3.42 Beta | source within RetroZilla snapshot, generated SDK, runtime | MPL-2.0 evidence preserved | bundled legacy dependency |
| NSPR | 4.7.7 | source within snapshot, generated SDK, runtime | license evidence header and source notices preserved | bundled legacy dependency |
| OpenSSL | 3.5.8 LTS | official archive, staged headers/import libs/runtime, source/runtime manifests, exact build record | upstream Apache License 2.0 preserved twice for package convenience | `libcrypto-3-x64.dll` and `libssl-3-x64.dll` expected in OpenSSL runtime package |
| Schannel/SSPI/CryptoAPI | operating-system components | no vendored binaries | Windows-supplied; no third-party file bundled | not distributed by PST |
| PST itself | current repository | source and generated library outputs | **project license not yet selected** | `REVIEW REQUIRED` before public redistribution |

All audited manifests matched: RetroZilla top-level 7/7, source 1/1, patch 1/1, runtime 11/11, SDK 191/191; OpenSSL source 1/1, staged top-level 4/4, runtime 2/2. No mismatch was found.

RetroZilla reproducibility remains **Level B**: exact source/delta/configuration/toolchain procedure and canonical binaries are retained, but a fresh byte-identical rebuild was not proved. `REQUIRED_UNIQUE_FILES_IN_PSTW=0` and active build references to `C:\PSTW` are absent. The production RNG patch remains the only local source patch; failure-injection artifacts are test-only and must not ship.

OpenSSL provenance records the official archive SHA-256 `a8f84a39918ec6415ce765d9b429d313ba97b8143169c172e734b9514464f5b2`, no local source patch, `VC-WIN64A shared no-legacy no-fips no-autoload-config`, and an upstream `nmake test` PASS of 4137 tests in 346 files. Only the built-in default provider is used; legacy/FIPS modules and ambient OpenSSL substitution are absent.

This is an inventory, not legal advice. Before release, 9.E must select the PST project license and review exact binary/source package obligations and attributions for RetroZilla, NSS, NSPR, and OpenSSL. Future providers must be evaluated individually for license compatibility, redistribution rights, notices, source/modification obligations, provenance, maintenance, and vendored versus consumer-supplied dependency models. Technical feasibility alone does not authorize vendoring.

## Versions and proposed policy

After the public-bootstrap addendum, current values are API **1.3.0**, library **0.3.0**, and SPI **2.4**. They evolve independently:

- The API version identifies the public consumer contract and accepted structure/version families. Patch means compatible clarification/fix with no ABI layout or behavior break; minor means compatible additions using guarded tails/new symbols; major means an intentionally incompatible public contract. 9.B must validate these proposed rules before calling the ABI frozen.
- The library version identifies the shipped implementation/release. Patch means compatible defect/documentation/build correction; minor means meaningful compatible implementation/provider capability growth; major means a maturity/compatibility commitment justified by the full release, not merely a phase number.
- The SPI version identifies the internal provider contract. Patch/minor terminology is project-defined: a compatible guarded extension may increment the SPI minor, while a required prefix/layout/semantic incompatibility requires a new SPI major. 9.C owns the precise freeze and compatibility rules.

Recommendation: **RECOMMEND_0_4_0_FOR_PHASE9**, but do not apply it before the Phase 9 release decision. Since 0.3.0, the implementation gained deterministic multi-provider selection, Schannel, OpenSSL 3.5.8, TLS1.3, Windows OpenSSL SYSTEM_TRUST, cross-provider validation, and substantial lifecycle/readiness/security hardening. This is meaningful compatible feature growth appropriate for a 0.4.0 candidate.

Do not choose 1.0.0 yet. Public ABI and SPI have not completed 9.B/9.C freeze audits; packaging, examples, bilingual documentation, release-validation kits, and clean-machine RC proof remain incomplete; platform certification is intentionally narrow. A 1.0 maturity promise would be premature.

## Input to the 9.B public ABI audit

The public surface consists of exported functions in `papinho_secure_transport.h` and the Win32 adapter header; opaque handles (`runtime`, `config`, `credentials`, `trust`, `connection`, `peer_info`, `transport`); fixed-width PST integers and platform `pst_size`; `PST_RESULT`; result/operation/interest/close/trust/feature/capability/logging constants; callbacks; calling/export macros; and public structures for versions, diagnostics, logging, credentials/trust/identity, runtime selection/info, TLS policy, ALPN, I/O, wait, and peer summary.

9.B must inventory every function signature and symbol, verify C/C++ linkage and `PST_CALL`/`PST_API`, measure x86/x64 size/offset/alignment, review `pst_size` width, validate `struct_size` and API-version guards, decide unknown-tail/prefix behavior, confirm ownership/lifetime and buffer-copy contracts, and identify whether the Win32 adapter header is part of the stable SDK. This document does not freeze them.

## Input to the 9.C SPI audit

The internal SPI surface includes `PST_BACKEND_DESCRIPTOR`, its legacy prefix and metadata tail, `PST_BACKEND_VTABLE`, minimum-size/offset guards, metadata/component versions, provider IDs, capability bits, registration and exact/ordered/automatic selection, provider/backend/runtime/connection lifecycle, `ownership_accepted`, native transport kind, readiness interests, backend wait, incremental handshake/read/write/shutdown, clean/truncated closure, identity/trust/ALPN configuration, peer-info creation, and diagnostic copying.

9.C must verify append-only compatibility and all optional hooks, ownership transitions for every failure category, provider-local singleton behavior, lifecycle balance, selection failure semantics, terminal-state rules, capability honesty, and descriptor/vtable/version validation. No redesign is proposed in 9.A.

## Known limitations

| Classification | Limitation |
|---|---|
| BY DESIGN | TLS versions below 1.2, plaintext fallback, hidden downgrade, hidden trust union, and automatic provider substitution after selection are not supported |
| BY DESIGN | NSS does not provide SYSTEM_TRUST; custom and system trust are distinct policies |
| BY DESIGN | OpenSSL SYSTEM_TRUST disables PST-driven AIA and root-auto-update retrieval and performs no online revocation check |
| CURRENT IMPLEMENTATION | RetroZilla NSS permits one active provider state per process and uses the historical NSS 3.42 Beta/NSPR 4.7.7 lineage |
| CURRENT IMPLEMENTATION | Schannel TLS1.3 was unavailable and is not advertised on the tested Windows 10 build; this is not a claim about all Windows versions |
| CURRENT IMPLEMENTATION | OpenSSL integration targets exactly 3.5.8, uses only its default provider, and implements SYSTEM_TRUST only through its Win32 adapter |
| CURRENT IMPLEMENTATION | Public peer info exposes the owned leaf certificate, not a full public chain API |
| CURRENT IMPLEMENTATION | Resumption and early data/0-RTT capability bits are not advertised |
| NOT TESTED | Windows 2000, XP, 95, 98, Win32s/3.11, other modern Windows builds, domain-joined enterprise SYSTEM_TRUST, broad ECDSA profiles, and non-Windows targets |
| NOT TESTED | Large/unbounded or multi-day stress beyond completed bounded soak matrices |
| FUTURE | POSIX and other platform adapters/providers, dynamic provider plugins, expanded algorithm/version matrices, revocation/AIA policies, full-chain API, resumption, 0-RTT, and future secure transports |

## Evidence-backed security claims

The release may claim: policy-enforced TLS 1.2 minimum; TLS1.3 where advertised; fail-closed TLS/authentication/policy behavior; no plaintext fallback or silent version/provider/trust widening; hostname validation; exclusive custom trust; system trust only where advertised; explicit DER/PKCS#8 mTLS; ALPN; owned peer snapshots; authenticated clean-close versus truncated EOF classification; bounded nonblocking progress; terminal no-resurrection; normalized/redacted diagnostics and logging; and ownership/lifecycle balance under the tested matrices.

It must not claim perfect or military-grade security, formal verification, FIPS certification, all-Windows support, arbitrary NSS/OpenSSL compatibility, independent cryptographic implementation, or security beyond the explicit tested/provider capability set.

## Tested-platform and NT4 evidence

| Platform | Architecture/toolchain | Status | Evidence boundary |
|---|---|---|---|
| Windows NT 4.0 SP6 | x86, VC6/C89 | TESTED | RetroZilla NSS provider and public PST runtime |
| Windows 10 Pro 22H2 build 19045.6456 | x64, MSVC 19.51; also VC6 host builds | TESTED | Schannel and OpenSSL providers; host regressions for legacy build |
| Windows 2000 | x86 | NOT TESTED | no release support claim |
| Windows XP | x86 | NOT TESTED | no release support claim |
| Windows 95 | x86 | NOT CURRENTLY VALIDATED | future investigation only |
| Windows 98 | x86 | NOT CURRENTLY VALIDATED | future investigation only |
| Win32s / Windows 3.11 | x86/16-bit environment | FUTURE | no implementation or support claim |
| Other Windows builds, architectures, POSIX/Linux/Raspberry Pi/embedded | varies | NOT TESTED / FUTURE | architecture permits future adapters; no certification |

Real NT4 evidence includes TLS1.2 and TLS1.3, server authentication, mTLS, required ALPN, custom trust, `PR_WOULD_BLOCK_ERROR` and `PR_Poll` readiness, exact bidirectional secure I/O, repeated lifecycle, bounded graceful shutdown, peer snapshot after connection destruction, wrong hostname, untrusted CA, missing credentials, no-downgrade policy, clean close, data-then-close, abrupt/truncated close, and the readiness-spin fix. Three repeated TLS1.3 mTLS/ALPN/25-byte lifecycle cycles completed in one process.

The minimum 9.F legacy release-validation subset is: smoke/version/provider identity; TLS1.3 NSS client to modern OpenSSL server; TLS1.2 NSS client to modern Schannel server; server-auth and mTLS/ALPN modes; exact echo/content; bounded shutdown; one negative trust/hostname gate; and clear DLL provenance. No NT4 rerun is part of 9.A.

## Phase 9 handoff requirements

9.D must produce small compilable SDK examples for basic TLS, custom trust, system trust, mTLS, selection, incremental readiness, read/write, shutdown, diagnostics, and logging. They must explain ownership, cleanup, and bounds and must not be test harnesses disguised as examples. A short Getting Started path must teach runtime, configuration, TLS policy, trust, hostname, native transport, connection, handshake, wait, I/O, shutdown, and release before internals.

The root README should remain concise and welcoming, link complete English and Portuguese (Brazil) documentation paths, and serve systems programmers, application developers, retrocomputing enthusiasts, non-experts, and new contributors. Level 1 documentation explains what/why/build/first connection/contributing; Level 2 covers API/SPI/providers/trust/readiness/lifecycle/security/provenance. The full rewrite and possible `docs/en`/`docs/pt-BR` layout belong to 9.D.

The community message should explain that old clients need not force modern servers to re-enable insecure protocols: PST aims to bring modern secure transport toward legacy clients where feasible, while remaining useful outside retrocomputing. Contributions are welcome in documentation, translations, old-machine testing, build reproduction, C/C89/Win32/VC6, modern Windows, TLS/security, NSS/NSPR, Schannel, OpenSSL, providers/adapters, examples, application integration, and provenance/license review. A valuable research direction is a reproducible modern-TLS/TLS1.3-capable NSS/NSPR lineage for old Windows; PST does not promise to own such a fork.

TLS is current technology. DTLS, QUIC-related transports, Noise, and other secure transports are research directions only. Any proposal must establish architectural fit, semantics, readiness, security, ownership, compatibility, testing, license, and provenance.

A possible future PapinhoTrustStore is separate ecosystem work: a versioned/signed, provenance-aware, rollback-resistant, offline-installable trust-anchor source for old systems consumed through CUSTOM_TRUST. SYSTEM_TRUST remains host policy; PST is not a root program.

9.E must evaluate distinct legacy NSS runtime/SDK, modern Schannel runtime/SDK, modern OpenSSL runtime/SDK, optional combined, and source/development packages. Runtime means PST binary, runtime dependencies, notices, and manifest. SDK adds public headers/libraries, examples, integration documentation, and selected validation utilities. Packages must exclude build trees, private keys/test PKI, Python/Perl/NASM, compiler installations, logs, caches, external archives, and ambient dependencies unless deliberately documented.

The 9.E machine-readable manifest must contain PST library/API/SPI versions, target, architecture, toolchain, provider IDs and versions, runtime dependency filenames and hashes, provenance references, and package identity. Debug-symbol policy and exact license/notice placement remain decisions.

9.F validation must produce obvious `PASS`, `FAIL`, and `ENVIRONMENT_FAILURE`. Required cross-machine paths are NT4/PST/NSS to a modern OpenSSL TLS1.3 server and to a modern Schannel TLS1.2 server, preferably through a simple `validate-pst.bat <server-ip>` workflow. Modern validation covers Schannel TLS1.2, OpenSSL TLS1.2/TLS1.3, OpenSSL SYSTEM_TRUST where available, and provider selection/coexistence. Public-network checks remain opt-in.

9.G must test packaged artifacts, not the development tree, on clean modern Windows and NT4. It must verify dependency discovery, missing-DLL failure, PATH contamination, OpenSSL/NSS substitution resistance, documentation accuracy, and examples built against the packaged SDK. Runtime use must not depend on `C:\Projetos\PapinhoSecureTransport`, `C:\PSTW`, developer PATH, test certificates, Python/Perl/NASM, or Visual Studio.

## Frozen Phase 9 scope

Phase 9 **will** define version policy; audit/freeze public ABI and SPI; create approachable English and pt-BR documentation, examples, and contribution guidance; decide package families; review licenses/notices/provenance; create manifests and release validation; validate NT4-to-modern paths; test clean-machine release candidates; and establish the final release baseline.

Phase 9 **will not** add a fourth provider; implement DTLS, QUIC, Noise, POSIX, Win95/98/Win32s, HTTP/SMTP/IMAP, dynamic plugins, PapinhoTrustStore, an NSS fork, or unrelated cryptographic features. A reproduced genuine release blocker may be corrected in its responsible layer.

Deferred inventory: Windows 2000/XP validation is OPTIONAL VALIDATION; Windows 95/98, Win32s, POSIX, new adapters/providers, modern NSS, BearSSL, broader OpenSSL/Schannel/algorithm support, resumption, 0-RTT, full-chain API, dynamic providers, future secure transports, PapinhoTrustStore, longer stress, domain-joined SYSTEM_TRUST validation, revocation policy, and AIA policy are FUTURE or DEFERRED FEATURES. They do not block release unless a current claim depends on them.

## Decision and next steps

Phase 9.A answers the release question: PST is preparing separate legacy NSS, modern Schannel, and modern OpenSSL target baselines plus source/SDK/documentation/validation inputs, all derived from the current three-provider code and evidence. A combined modern provider package is an official optional candidate, not a default or recommendation. No final package has been created.

Phase 9.A is complete with no API, ABI, SPI, provider, build, or version change. The next step, only when explicitly requested, is **9.B - Public API / ABI Freeze Audit**.
