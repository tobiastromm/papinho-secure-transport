<!-- SPDX-License-Identifier: MPL-2.0 -->

# OpenSSL provider extension

Status: **OSSL-F and the post-Phase-8 OpenSSL provider extension complete**. This is a deliberate post-Phase-8 provider extension; Phase 8 remains complete and Phase 9 has not started.

Follow-up status: **OSSL-ST-A through OSSL-ST-D Windows SYSTEM_TRUST complete**. The capability is advertised at `0x00000e7f`; Phase 9 has not started.

## Frozen baseline and provenance

The initial baseline is **OpenSSL 3.5.8 LTS**, released 2026-08-25. The OpenSSL 3.5 line is supported through 2030-04-08. Official release source: `https://github.com/openssl/openssl/releases/download/openssl-3.5.8/openssl-3.5.8.tar.gz`; published SHA-256: `a8f84a39918ec6415ce765d9b429d313ba97b8143169c172e734b9514464f5b2`. The official source/signature and published digest must be verified before import.

OpenSSL 3.5.8 uses Apache License 2.0. When source or binaries are bundled, retain the upstream `LICENSE.txt`, copyright and applicable notice material, identify modifications, and satisfy the license's redistribution conditions. Static versus dynamic linking does not change which upstream license applies; packaging obligations must be reviewed from the exact release. This document records the inventory and is not legal advice.

The canonical policy is source-based preparation, not an arbitrary installed OpenSSL or third-party binary. Future canonical paths:

```text
third_party/openssl/source/openssl-3.5.8.tar.gz
third_party/openssl/licenses/LICENSE-APACHE-2.0.txt
third_party/openssl/patches/MANIFEST.sha256
third_party/openssl/prebuilt/win64-msvc-3.5.8/{include,lib,runtime}
third_party/openssl/prebuilt/win64-msvc-3.5.8/MANIFEST.sha256
build/win64-modern-msvc-openssl
build/win64-modern-msvc-combined
```

Prefer zero source patches. Any necessary patch must be ordered and hashed under `third_party/openssl/patches`; silent vendored edits are forbidden. The source archive, signature/digest, license, configure command, toolchain identity, runtime/import-library hashes and source-to-binary record must exist from the first build.

The initial support contract is the exact OpenSSL 3.5.8 baseline and its generated ABI, not arbitrary OpenSSL 3.x. A future compatible-patch policy requires explicit binary and functional validation. A shared build with normal import-library linking is preferred. Expected x64 runtime names from the canonical build are `libssl-3-x64.dll` and `libcrypto-3-x64.dll`; loaders must not wildcard-match other ABI families. Normal linking avoids a large `LoadLibrary/GetProcAddress` shim, while separate package directories prevent accidental ambient DLL selection and allow security updates to be audited. Phase 9 retains final redistribution/package policy.

## Targets and provider identity

The stable PST backend ID is `openssl`; the version belongs in internal SPI metadata. Initial implementation target: Windows 10 x64 or newer, using the repository's newest-supported modern MSVC bootstrap. Earlier Windows versions are not claimed until tested. OpenSSL is excluded from VC6/NT4 packages.

Target manifests are intentionally distinct:

- legacy: `[retrozilla-nss]`, `build/vc6`;
- modern Schannel: `[schannel]`, `build/win64-modern-msvc`;
- modern OpenSSL: `[openssl]`, `build/win64-modern-msvc-openssl`;
- future combined modern: `[schannel, openssl]`, `build/win64-modern-msvc-combined`.

Only OpenSSL-only development output is required initially. The combined package is a later functional gate. Its recommended automatic order is Schannel then OpenSSL, subject to validation: capability filtering keeps Schannel for an acceptable TLS 1.2 policy and skips it for TLS-1.3-required policy so OpenSSL may be selected. Exact and ordered selection remain consumer-controlled. No post-selection fallback or silent TLS downgrade is added.

## SPI and transport mapping

SPI 2.4 already expresses initialization/shutdown, runtime and connection lifecycle, neutral transport attachment/ownership, frozen identity/TLS policy, incremental handshake/read/write/wait/shutdown, close kind, peer snapshots, diagnostics and logging. No missing hook or layout requirement was found. API 1.2.0, library 0.3.0 and SPI 2.4 remain unchanged.

The provider consumes `PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET` through the existing internal envelope. It validates size/version/kind, makes the socket nonblocking privately, and accepts ownership only after successful attachment. No generic `if backend == openssl` routing is permitted. Future POSIX work should add a provider-neutral POSIX native transport adapter while reusing most OpenSSL TLS code; no POSIX implementation is part of OSSL-A.

OpenSSL types (`SSL_CTX`, `SSL`, `BIO`, `X509`, `EVP_PKEY`, `OSSL_LIB_CTX`) remain under `src/backends/openssl/`, never in the public header or generic SPI structures. Recommended source files are `pst_backend_openssl.c` and `pst_backend_openssl.h`, with platform socket mechanics isolated behind the native transport boundary.

## Incremental TLS design

Each connection uses a nonblocking socket and private `SSL`. `SSL_do_handshake` maps success to COMPLETE, `SSL_ERROR_WANT_READ` to NEED_READ, and `SSL_ERROR_WANT_WRITE` to NEED_WRITE. ZERO_RETURN is authenticated clean closure only when the TLS API reports close-notify. SYSCALL/unexpected EOF and protocol-stack failures are mapped from actual context without treating socket EOF as clean. Cases that can progress on either direction use the existing READ_WRITE interest where justified.

`SSL_get_error` must be called immediately on the same operation/thread with its return value. `SSL_pending`/`SSL_has_pending` can permit immediate read progress without waiting for socket readability, but must obey existing anti-spin and bounded-progress rules. Provider wait maps the current normalized interest to bounded socket readiness; buffered TLS work remains provider-private.

TLS policy uses `SSL_CTX_set_min_proto_version` and `SSL_CTX_set_max_proto_version` for exact TLS 1.2, exact TLS 1.3, or the 1.2-1.3 range. SSLv2/v3 and TLS 1.0/1.1 are never enabled, and negotiated protocol is verified against the frozen range. OpenSSL TLS 1.3 and TLS 1.2 are functionally validated on Windows 10 x64.

Reads/writes use the `_ex` APIs where appropriate and preserve partial-I/O semantics. No plaintext fallback exists. Secure defaults are retained; PST does not invent a restrictive cipher list or pin fixture ciphers. Session resumption and early data/0-RTT remain unsupported and unadvertised.

`SSL_shutdown` is incremental: success completes, return 0 means close-notify was sent but reciprocal completion is not assumed mandatory indefinitely, and WANT_READ/WANT_WRITE map to existing shutdown progress. PST keeps bounded shutdown and may finish after sending its alert according to the established provider contract. Raw EOF without authenticated close-notify is `FAILED/TRUNCATED`.

## Trust, identity, ALPN and peer information

Initial OpenSSL support advertises **custom trust only**. A private `X509_STORE` receives the copied PST trust anchors and verifies the server chain; system defaults are not composed into it. Windows SYSTEM_TRUST needs a deliberate adapter that imports Windows roots into an isolated OpenSSL store and is deferred because Schannel already supplies native system trust. Merely loading a file/default path cannot justify the capability.

Hostname validation uses official verification parameters, such as `X509_VERIFY_PARAM_set1_host`/`SSL_set1_host`, with peer verification enabled. No custom SAN/CN parser and no bypass are allowed. ALPN uses the official client protocol-list API and selected-protocol query, preserving offered wire format, required/optional behavior, multiple protocols and normalized copy-out.

Certificate DER is decoded into provider-owned `X509`; unencrypted PKCS#8 DER is decoded into provider-owned `EVP_PKEY`; the matching pair is installed in the private context and checked. There is no filesystem, persistent key store or automatic identity source. All objects are released deterministically.

Peer info obtains negotiated protocol, `SSL_CIPHER_get_protocol_id`/standard numeric suite ID, selected ALPN, verified authentication flags and an owned peer leaf certificate. The provider DER-encodes/copies the leaf and computes SHA-256 through OpenSSL, then creates the existing pointer-free PST snapshot. Native OpenSSL pointers never escape.

## Lifecycle, configuration and error discipline

OpenSSL 3.x performs thread-safe automatic library initialization, but the PST provider should use an `OSSL_LIB_CTX` per runtime, explicitly load only the OpenSSL **default cryptographic provider**, and construct contexts with that library context. PST provider/backend terminology is separate from OpenSSL cryptographic-provider modules. The `legacy` and `fips` modules are not required, bundled or loaded; there is no FIPS claim.

Disable ambient configuration loading for provider-owned initialization where the supported API permits, and do not call default trust-path APIs implicitly. The canonical package must not depend on machine-global `openssl.cnf`, `OPENSSL_CONF`, `OPENSSL_MODULES`, ambient DLL search, or arbitrary installed provider modules. If an unavoidable OpenSSL configuration interaction is discovered during implementation, stop and document it before advertising capabilities.

OpenSSL's thread-local error queue is provider-private. Clear it immediately before an operation that will be classified; on failure, drain/capture the complete relevant queue into bounded private state; drain stale entries after classification. Never expose numeric codes or OpenSSL error strings through public diagnostics/logging. Failure on connection A must not contaminate connection B. Multiple OpenSSL runtimes and simultaneous Schannel/OpenSSL runtimes are mandatory future gates; no artificial singleton is designed.

Public diagnostics contain only normalized result, operation and `backend_id=openssl`. Logging uses Papinho Logging Levels v1 and provider-neutral structured events; TRACE may report normalized progress only. No error-stack text, hostname, ALPN text, certificate/key/trust bytes, endpoint, socket, pointer, handle or payload is public.

## Build preparation

Use the existing canonical modern MSVC x64 environment. Official OpenSSL Windows preparation requires a suitable Perl, NASM for the assembler-enabled x64 build, MSVC tools and NMAKE. Do not install these automatically. The planned canonical sequence from an ordinary bootstrapped modern shell is:

```bat
perl Configure VC-WIN64A shared --prefix=<staging> --openssldir=<private-staging-config>
nmake
nmake test
nmake install_sw
```

Freeze the exact Perl/NASM/MSVC/SDK versions, complete Configure output/options and hashes during the source-preparation task. OpenSSL third-party outputs stay outside PST build outputs. The PST OpenSSL target consumes only the explicit staged include/lib/runtime paths.

## Security baseline and future validation

The provider must fail closed with TLS >= 1.2, exact version bounds, peer verification, official hostname checks, required ALPN enforcement, explicit credentials, no plaintext fallback, no trust-all mode, no legacy cryptographic provider, no insecure compatibility widening and no secret logging. FIPS, resumption and early data are future optional work.

Initial same-family Python/OpenSSL fixtures may accelerate development, but cannot be the only interoperability proof. Final closure requires an independent engine, preferably PST OpenSSL client against a Schannel server, plus separate processes when Python brings its own incompatible OpenSSL family. It must also prove same-process Schannel and OpenSSL runtimes, two OpenSSL runtimes, error-queue isolation, lifecycle balance and cross-provider selection.

## Frozen roadmap

1. **OSSL-A - Architecture / Version / Build / Provenance:** complete.
2. **OSSL-B - Backend Skeleton / Registration / Modern Build Integration:** lifecycle, exact metadata, explicit target and transport ownership; no TLS claims.
3. **OSSL-C - TLS 1.2/TLS 1.3 / Readiness / Secure I/O:** real protocols, incremental handshake, partial I/O and bounded wait/shutdown.
4. **OSSL-D - Trust / Hostname / ALPN / mTLS / Peer Info:** custom trust and complete common semantic surface; system trust remains optional/deferred.
5. **OSSL-E - Failure / Close / Diagnostics / Logging / Lifecycle Hardening:** truncation, error precedence/queue isolation, multiple runtimes and security disclosure.
6. **OSSL-F - NSS/Schannel/OpenSSL Cross-Provider Validation and Extension Closure:** combined target, capability-driven selection and independent-engine proof.

No NSS, Schannel, public API, generic SPI, or portable core behavior changed in OSSL-C. OSSL-C is complete; OSSL-D is next and has not started.

## OSSL-B implementation result

The exact official 3.5.8 archive is retained and hash-verified. The shared `VC-WIN64A` build used `no-legacy no-fips no-autoload-config`; `nmake`, all 4137 mandatory upstream tests, and `nmake install_sw` passed. Canonical generated headers, import libraries, and the two required DLLs are retained under `third_party/openssl/prebuilt/win64-msvc-3.5.8`, with SHA-256 manifests. No external OpenSSL from PATH is consumed.

At OSSL-B, the `openssl` backend supplied an SPI 2.4 skeleton with adapter metadata 0.1.0 and OpenSSL component metadata 3.5.8 LTS. Its only advertised capability is `NONBLOCKING`, which is implemented by its private Win32 socket attachment. Each PST runtime owns an independent `OSSL_LIB_CTX` and explicitly loaded built-in default provider. Tests prove 100 initialize/shutdown cycles, two simultaneous runtimes with independent release, exact and automatic selection, error-queue cleanup, invalid/valid transport behavior, and exactly-one socket close.

At that milestone, handshake, secure read/write and shutdown were unsupported; OSSL-C subsequently implemented them. Peer identity, complete trust/hostname semantics, ALPN and client authentication remain OSSL-D. The OpenSSL connection destructor remains the single native-socket close root.

The isolated test sets `OPENSSL_CONF` and `OPENSSL_MODULES` to nonexistent locations. Runtime creation still loads `default`; an explicit audit finds `legacy` and `fips` unavailable. No OpenSSL error code or string enters PST public diagnostics.

## OSSL-C implementation and functional evidence

Each connection owns an isolated `SSL_CTX` because TLS min/max and the narrow fixture trust store are frozen connection policy. It also owns exactly one client-mode `SSL`. `SSL_set_fd` uses the socket BIO without transferring the native close root to OpenSSL; `SSL_free`/`SSL_CTX_free` run before the PST OpenSSL connection performs the single `closesocket`.

The provider maps PST 12/13 to exact OpenSSL TLS 1.2/1.3 bounds, clears `SSL_MODE_AUTO_RETRY`, partial-write and moving-buffer modes, and reports application consumption only when `SSL_write_ex` succeeds. `SSL_get_error` is called immediately after each failed handshake/read/write/shutdown operation. WANT_READ and WANT_WRITE become PST readiness, provider-local `select` is bounded, and `SSL_pending`/`SSL_has_pending` prevent unnecessary socket waits when OpenSSL has buffered work. `SSL_OP_IGNORE_UNEXPECTED_EOF` is explicitly absent so authenticated `close_notify` is CLOSED/CLEAN and raw EOF is FAILED/TRUNCATED.

OSSL-C intentionally uses only a provider-private DER root for its deterministic fixtures with `SSL_VERIFY_PEER`. It does not advertise CUSTOM_TRUST, HOSTNAME_VERIFY, ALPN, CLIENT_AUTH, SYSTEM_TRUST or PEER_INFO; those complete public semantics remain OSSL-D. Advertised capabilities are TLS1.2, TLS1.3, NONBLOCKING and BACKEND_WAIT (`0x00000c03`).

Real gates passed with canonical OpenSSL 3.5.8 on Microsoft Windows 10 Pro x64 10.0.19045: exact TLS 1.2 and TLS 1.3, 1.2-1.3 range selecting both endpoints, both exact-version mismatch failures without widening/downgrade, 10 x 25-byte TLS 1.2 and TLS 1.3 exchanges, fragmented input/output, a 4 MiB backpressure case with observed WANT_WRITE and exact content, incremental local shutdown, clean peer close, data then close, abrupt EOF as truncation, and a plaintext peer failing closed. The independent-engine gate used the repository Schannel TLS 1.2 server and passed handshake plus 25-byte secure echo. The same machine reports Schannel TLS 1.3 unavailable/not advertised while OpenSSL TLS 1.3 passes.

The canonical copied DLL hashes match the staged runtime manifest: `libcrypto-3-x64.dll` `09eec573c9adea156ba2073f8cd61720d0aabeb7562d8498b4ecd21b710a3044`; `libssl-3-x64.dll` `3fb3cd7804dbe3216c801b470e14461d80214ece99c637ae42ea3d8caf75d7ed`. Functional artifacts are under ignored `build/phase-ossl-c`.

## OSSL-D implementation progress

The OpenSSL connection now uses an exclusive connection-local `X509_STORE`, `SSL_VERIFY_PEER`, `SSL_set1_host` for certificate verification, and an independent SNI value from the same frozen DNS hostname. It never loads system/default trust paths, `SSL_CERT_FILE`, or `SSL_CERT_DIR`. ALPN consumes the core-validated PST wire encoding through `SSL_set_alpn_protos`, preserves offer order, verifies the selected value belongs to the offer, and enforces required versus optional absence.

Explicit client identity is decoded entirely in memory: exact single-certificate DER through `d2i_X509`, exact PKCS#8/private-key DER through the OpenSSL EVP decoder, followed by `SSL_CTX_check_private_key`. No certificate/key temp file, Windows store, callback, or implicit identity lookup exists. Peer snapshots copy the exact leaf DER, SHA-256, normalized TLS version, standard cipher-suite ID and authentication flags before the connection is destroyed.

Real TLS 1.2/TLS 1.3 gates pass for root-intermediate-leaf, correct hostname, required/optional/multiple ALPN, and explicit mTLS. Wrong root and missing intermediate map to AUTH_FAILURE; wrong hostname maps to HOSTNAME_MISMATCH; required ALPN absence maps to POLICY_VIOLATION; missing/wrong client identity maps to AUTH_FAILURE; local cert/key mismatch fails during connection creation. TLS 1.3 mTLS server fingerprint comparison proves the exact configured client identity. OSSL-C readiness, 4 MiB WANT_WRITE, clean/data close, truncation and shutdown regressions pass, as does the independent Schannel TLS 1.2 server gate.

The implemented capability mask is `0x00000e5f`: TLS1.2, TLS1.3, CUSTOM_TRUST, HOSTNAME_VERIFY, ALPN, CLIENT_AUTH, PEER_INFO, NONBLOCKING and BACKEND_WAIT. SYSTEM_TRUST remains absent. OSSL-D is not yet marked complete because the required real B-C-A-B sequence within one runtime and functional two-runtime isolation runner have not yet been executed.

## OSSL-D2 isolation closure

The earlier OSSL-D blocked state was closed by `test_openssl_runtime_isolation.exe`, a single-process, real-TLS harness. Process 20108 executed B1-C-A-B2 in one OpenSSL runtime: TLS 1.3/custom trust/hostname/required ALPN/mTLS, wrong-root AUTH_FAILURE with immutable diagnostic and empty ERR queue, distinct TLS 1.2 server-auth without ALPN/client identity, then the original strong TLS 1.3 configuration again. All eight connections were created and destroyed exactly once; B1 peer state survived the full sequence and B2 had no stale diagnostic. Server-side evidence confirmed the exact client fingerprint for B1 and B2.

The same process then kept two OpenSSL runtimes alive. R1 completed TLS 1.3, R2 produced a deliberate trust failure, R1 completed another TLS 1.3 connection, R1 was released, and R2 still completed TLS 1.3 before its own release. Both runtime contexts were destroyed exactly once and the OpenSSL error queue was empty after the cross-runtime failure.

The combined selection model registered `schannel` then `openssl` using the real masks `0x00000e7d` and `0x00000e5f`. Automatic common TLS 1.2 selected Schannel; TLS 1.3 plus custom trust/hostname selected OpenSSL; system trust selected Schannel; exact OpenSSL succeeded; exact Schannel TLS 1.3 was unsupported; ordered `[openssl, schannel]` selected OpenSSL; and an all-incompatible early-data requirement returned UNSUPPORTED with the expected diagnostic behavior. OSSL-D is complete.

## OSSL-E hardening closure

The canonical failure/result matrix is recorded in `docs/openssl-failure-hardening.md`. Fresh real tests cover plaintext and exact-version protocol failures, clean and truncated closure, established-session RST, fatal wait, incremental shutdown, 4 MiB backpressure and 100 repeated TLS 1.3 echoes. Existing OSSL-D/D2 identity, logging, lifecycle, same-runtime recovery, dual-runtime isolation, independent Schannel server, coexistence and selection proofs remain applicable. No production or public ABI change was required. OSSL-E is complete; OSSL-F remains unstarted.
## OSSL-F extension closure

The formal NSS/Schannel/OpenSSL comparison and SYSTEM_TRUST follow-up analysis are recorded in `docs/openssl-extension-closure.md`. The three providers remain first-class on their deliberate targets, common semantics agree across the exact capability intersection, and capability differences remain explicit. OSSL-F and the OpenSSL extension are complete. Phase 9 remains unstarted.

The selected SYSTEM_TRUST design is documented in `docs/openssl-system-trust.md`: Windows chain-engine trust and usage policy, OpenSSL TLS and hostname verification, live per-connection evaluation, no CUSTOM_TRUST union and no hidden AIA/revocation network access in the initial implementation. Capability advertisement remains gated on functional proof.

## OSSL-ST-B private system-trust adapter

The Windows-only adapter now builds an effective `HCCE_CURRENT_USER` server-auth chain from an owned leaf plus peer-supplied intermediates in a temporary memory store. It disables AIA, URL retrieval and automatic root update, requests no online revocation, requires both successful policy execution and a zero policy error, and never imports Windows roots into OpenSSL. The synchronous OpenSSL certificate-verification callback blocks handshake completion until Windows trust and OpenSSL hostname validation both succeed. CUSTOM trust remains unchanged and exclusive.

The adapter unit gate rejects the repository private chain with `CHAIN_API=1`, `CHAIN_ERROR=1`, `POLICY_API=1`, and `POLICY_ERROR=1`, proving the BOOL-versus-`dwError` rule without system-store mutation. Fresh CUSTOM trust TLS 1.2 and TLS 1.3 echoes passed with 25-byte bidirectional content matches. The capability remains hidden at `0x00000e5f`; public SYSTEM trust and ST-C functional/isolation claims have not begun.

## OSSL-ST-C capability advertisement

Real Windows 10 build 19045 handshakes proved exact TLS 1.2 and TLS 1.3 with Windows SYSTEM trust against two configurable public endpoints, without custom CA or application payload. Private-root rejection, same-root CUSTOM success, specific hostname mismatch, peer snapshot lifetime, same-runtime SYSTEM/CUSTOM alternation, two-runtime release isolation, diagnostics, ERR queue discipline and combined selection all passed. SYSTEM_TRUST is the only new capability bit (`0x00000e5f` to `0x00000e7f`). The opt-in runner is `tests/run_openssl_system_trust_integration.ps1`; the default fast suite remains offline.

## OSSL-ST-D formal closure

The hardening matrix passed without a production-code change. Public TLS 1.2/1.3, wrong hostname, private SYSTEM rejection, private CUSTOM success, same-runtime and two-runtime recovery, adapter BOOL/status checks, diagnostics, logging, cleanup, selection, legacy separation, and all canonical builds are green. The public runner now performs a bounded TCP preflight so endpoint unavailability is classified as ENVIRONMENT_FAILURE; the default OpenSSL suite remains offline. Full policy and limitations are frozen in docs/openssl-system-trust.md. Phase 9 remains not started.
