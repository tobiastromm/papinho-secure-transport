<!-- SPDX-License-Identifier: MPL-2.0 -->

# OpenSSL on Windows system-trust architecture

Status: OSSL-ST-A through OSSL-ST-D and Phase 9 are complete. The OpenSSL provider advertises `SYSTEM_TRUST` with capability mask `0x00000e7f`; the release remains unpublished.

## Contract

On Windows, PST system trust means that the Windows certificate chain engine is the authority for whether the peer chain terminates in trust effective for the calling user/process. It is not a copy of whichever certificates happen to be enumerable in one ROOT store. OpenSSL remains the TLS/record engine and performs an independent provider-local hostname check. Custom trust remains an exclusive connection-local OpenSSL `X509_STORE` containing only caller-supplied anchors; it is never unioned with system trust.

The initial implementation should use the current-user chain engine (`HCCE_CURRENT_USER`). Windows logical CurrentUser stores include inherited LocalMachine physical stores and applicable policy stores. This matches an interactive consumer's effective identity more closely than separately merging CurrentUser and LocalMachine ROOT. A service naturally evaluates under its service identity; a future explicit machine-only policy would require a separately designed public semantic, not a hidden switch.

Windows documents logical ROOT, CA and Trust stores across CurrentUser, LocalMachine, CurrentUser Group Policy, LocalMachine Group Policy and LocalMachine Enterprise locations. The chain engine, rather than manual enumeration, is responsible for combining effective stores and recognizing explicit distrust. A Windows `CERT_TRUST_IS_EXPLICIT_DISTRUST` or failing SSL chain policy is terminal authentication failure.

## Preferred validation pipeline

1. OpenSSL performs TLS 1.2/1.3 negotiation and supplies the peer leaf and server-provided intermediates.
2. A Windows-only OpenSSL trust adapter converts those certificates to temporary `CERT_CONTEXT` objects and places intermediates in a connection-local memory store.
3. `CertGetCertificateChain(HCCE_CURRENT_USER, ...)` builds a server-auth chain with `szOID_PKIX_KP_SERVER_AUTH`, the memory store as the additional untrusted store, and current time.
4. `CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_BASE, ...)` checks the resulting Windows chain policy without duplicating hostname semantics. Its return value means the call executed; both chain trust status and `CERT_CHAIN_POLICY_STATUS.dwError` must be zero.
5. Independently, OpenSSL checks the configured hostname using its supported hostname API against the leaf certificate. Windows trust success never bypasses hostname validation.
6. All `CERT_CONTEXT`, chain-context and memory-store objects are released on every path. Native status remains provider-private and is normalized once.

This is a hybrid architecture: Windows is authoritative for chain construction, effective anchors, distrust, time, constraints and server-auth usage; OpenSSL is authoritative for TLS and hostname. It does not route traffic through Schannel and does not require OpenSSL to recreate Windows root policy.

## Deterministic network and revocation policy

The implementation does not introduce hidden network traffic inside an incremental handshake. It sets `CERT_CHAIN_DISABLE_AIA`, `CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL`, and `CERT_CHAIN_DISABLE_AUTH_ROOT_AUTO_UPDATE`. Normal servers must send sufficient intermediates; locally cached/system CA certificates may participate. Missing intermediates fail authentication rather than triggering AIA.

PST currently does not promise revocation checking. ST-B must therefore request no online CRL/OCSP revocation flag and must not use policy flags that ignore otherwise detected chain errors. Cached Windows explicit distrust remains authoritative. Online revocation is a separate future policy feature because it needs public semantics, timeout/cancellation behavior and failure-mode decisions. This is an intentionally documented bounded subset of Windows online chain behavior, while retaining effective installed roots, enterprise policy and explicit distrust.

Trust is evaluated live per connection, not copied at runtime creation and not cached process-wide. Store and distrust updates affect newly validated connections; an established connection is not retroactively revalidated. This preserves multiple `OSSL_LIB_CTX` runtimes and avoids cross-connection contamination.

## ST-B implementation evidence

The private adapter is `src/backends/openssl/platform/win32/pst_openssl_system_trust_win32.c`; its header contains only copied DER inputs and normalized private outcomes. For each OpenSSL verification callback, the provider copies the leaf and each non-leaf peer certificate to owned DER, creates a temporary memory store for intermediates, and calls the adapter. The adapter builds with `HCCE_CURRENT_USER`, current time, server-auth EKU, and the three no-network flags above. It requests no revocation flag (`REVOCATION_POLICY=NOT_PERFORMED`) and releases the leaf context, chain context, memory store, and all temporary DER on every exit.

`SSL_CTX_set_cert_verify_callback` is installed only for SYSTEM trust while `SSL_VERIFY_PEER` remains enabled. The callback is synchronous inside `SSL_do_handshake`; it requires Windows trust and then `X509_check_host` before returning success. Therefore PST cannot report handshake completion or expose application I/O before both checks pass. CUSTOM trust continues through its exclusive connection-local OpenSSL `X509_STORE`; the two paths are never unioned or used as fallbacks.

The focused test proves malformed DER rejection and the private-root negative without installing a root. In that negative, `CertGetCertificateChain` and `CertVerifyCertificateChainPolicy` both execute successfully, but chain/policy errors remain present, so the adapter rejects. This specifically proves that a true policy-call BOOL cannot override nonzero `dwError`. No persistent store is opened for writes; the only certificate addition targets the temporary memory store. Trust refresh is `PER_CONNECTION_WINDOWS_CHAIN_EVALUATION`, with no runtime-global CryptoAPI object or cache.

ST-B kept `PST_CAP_SYSTEM_TRUST` absent at `0x00000e5f` until the initial ST-C gates passed. ST-C then added only that capability, producing `0x00000e7f`.

## Options scorecard

Scores are relative: 5 is strongest/best except complexity and security risk, where 1 is lowest.

| Option | Windows fidelity | Complexity | Security risk | Revocation | Enterprise/distrust | Performance | Testability | POSIX reuse |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Import ROOT into OpenSSL store | 2 | 2 | 5 | 1 | 1 | 5 | 4 | 4 |
| Windows chain engine for all certificate policy | 5 | 4 | 2 | 5 | 5 | 3 | 3 | 1 |
| Hybrid Windows chain/trust plus OpenSSL hostname | 5 | 4 | 2 | 4 | 5 | 3 | 4 | 2 |

Root import is rejected: it can miss explicit distrust, enterprise/Group Policy semantics, effective-store precedence, automatic updates and Windows path-building decisions. A pure Windows SSL-policy route could also check hostname, but would duplicate/replace the provider's established OpenSSL hostname behavior. The hybrid is preferred because it retains a clean TLS/trust boundary and permits precise `HOSTNAME_MISMATCH` normalization.

## Error and disclosure policy

Untrusted root, partial chain, explicit distrust, time invalidity, invalid signature, constraints, wrong EKU/usage and revoked status normalize to `AUTH_FAILURE`. Only the independent OpenSSL hostname check maps to `HOSTNAME_MISMATCH`. CryptoAPI allocation/call/structure failures with no certificate-policy conclusion map to `BACKEND_FAILURE`. A specific authentication or hostname result precedes later OpenSSL/socket/cleanup failure.

Public diagnostics and logs continue to expose only normalized result, operation and `backend_id=openssl`. They never expose Windows status/HRESULT, store names derived from environment, certificate subject/issuer, thumbprint, DER, hostname, ALPN, endpoint, handle or pointer. Windows trust state is separate from the OpenSSL ERR queue and must not leave either domain stale.

## Isolation and tests for ST-B/ST-C

Mandatory functional gates are OpenSSL TLS 1.2 and TLS 1.3 to environment-qualified Windows-trusted public endpoints, with peer info and bounded shutdown; a Schannel TLS 1.2 comparison; wrong hostname; and a private PST root that passes CUSTOM_TRUST but fails SYSTEM_TRUST. The same runtime must run system success, custom wrong-root failure, then system success with no state crossover.

Tests requiring store mutation need separate explicit authorization. If used, they may modify CurrentUser only, record before/after state, add an exact generated thumbprint, remove it in `finally`, and prove cleanup. A controlled root placed in CurrentUser ROOT and then CurrentUser Disallowed can test explicit distrust; no LocalMachine mutation is permitted. Enterprise-root behavior is established structurally from Windows chain-engine/store semantics unless a domain-joined fixture is available; it must not be claimed as runtime proof otherwise.

Public-site tests are environment-dependent integration gates rather than a permanent dependency on one company. At least two configurable endpoints should be supported. Expired/invalid-chain cases should use deterministic local certificates plus an isolated/restricted test chain engine when possible, without weakening the production system-trust path.

## Source boundary and work estimate

Proposed layout:

```text
src/backends/openssl/pst_backend_openssl.c
src/backends/openssl/platform/pst_openssl_system_trust.h
src/backends/openssl/platform/win32/pst_openssl_system_trust_win32.c
tests/test_openssl_system_trust.c
tests/run_openssl_system_trust.ps1
```

The generic OpenSSL file owns TLS integration and normalized state; the Win32 adapter alone includes CryptoAPI types and links `crypt32.lib`. A future POSIX adapter may implement its own system-trust semantics. No public API or SPI change is expected: `PST_TRUST_SOURCE_SYSTEM` and `PST_CAP_SYSTEM_TRUST` already exist. The capability mask remained `0x00000e5f` through ST-B; after the positive, negative, isolation, disclosure and cleanup gates passed in ST-C, only `PST_CAP_SYSTEM_TRUST` was added, producing `0x00000e7f`.

Historical plan: ST-A policy/architecture (this document); ST-B bounded provider-local Win32 implementation; ST-C real TLS 1.2/1.3, negative, isolation and store-safety matrix; ST-D hardening and closure. That work used API 1.2.0/library 0.3.0; the frozen release baseline is API 1.3.0, library 0.4.0 and SPI 2.4. CryptoAPI is OS-provided, so there is no new third-party redistribution license.

The consumer motivation includes PapinhoBrowser through PapinhoAccelerator to arbitrary public TLS 1.3 sites, but PST remains generic. The same policy is useful for enterprise/LAN applications whose corporate roots are installed through effective Windows policy.

## ST-C functional and isolation evidence

On Windows 10 build 19045, the production OpenSSL descriptor completed SYSTEM-trust-only handshakes with no custom CA, credentials, ALPN requirement, HTTP request, or application payload. `www.cloudflare.com` passed exact TLS 1.2 (`0x0303`, cipher `0xcca9`) and TLS 1.3 (`0x0304`, cipher `0x1302`); `www.google.com` independently passed both versions with the same normalized trust/hostname result. Each snapshot reported certificate present, chain validated, hostname validated, peer authenticated, nonzero cipher, SHA-256 length 32, owned leaf DER, and validity after connection destruction. Shutdown was bounded; these public peers closed transport after the client alert without a reciprocal authenticated close, which is recorded separately from the already successful handshake.

The same Google peer with configured `wrong.invalid` retained Windows trust success but failed specifically as `HOSTNAME_MISMATCH`, with logging OFF, immutable diagnostic, and no resurrection. The repository private root was not installed: SYSTEM mode returned `AUTH_FAILURE`, while the same root/intermediate/leaf immediately passed CUSTOM mode with TLS 1.3 and exact 25-byte echo. `SYSTEM_STORE_MUTATION=NONE`, `AIA_NETWORK_FETCH=DISABLED`, `AUTH_ROOT_AUTO_UPDATE_NETWORK=DISABLED`, `REVOCATION_POLICY=NOT_PERFORMED`, and `TRUST_REFRESH=PER_CONNECTION_WINDOWS_CHAIN_EVALUATION` remain true.

One runtime completed `S1 public SYSTEM success -> C1 private CUSTOM success -> S2 private SYSTEM AUTH_FAILURE -> C2 private CUSTOM success -> S3 public SYSTEM success`. The copied S2 diagnostic remained valid after destruction; C2/S3 had no diagnostic and the OpenSSL ERR queue was empty. Two simultaneous runtimes then proved R1 public SYSTEM success, R2 private CUSTOM success, R1 SYSTEM recovery, R1 release, and continued R2 CUSTOM success. Nine created connections and three runtimes were destroyed exactly once.

With registration order Schannel then OpenSSL, TLS 1.2 plus SYSTEM trust selects Schannel automatically; TLS 1.3 plus SYSTEM trust selects OpenSSL; exact OpenSSL succeeds; exact Schannel TLS 1.3 remains `UNSUPPORTED`; ordered `[openssl, schannel]` with TLS 1.2 selects OpenSSL. Schannel and NSS masks are unchanged. Enterprise and Group Policy trust are `STRUCTURALLY_SUPPORTED_BY_WINDOWS_CHAIN_ENGINE`; `RUNTIME_DOMAIN_TEST=NOT_PERFORMED`.

The public runner accepts endpoint overrides and is opt-in because it depends on DNS/network state. Defaults are test conveniences, not production dependencies. The PapinhoBrowser-specific PapinhoAccelerator may use PST/OpenSSL for public TLS 1.3 with Windows SYSTEM trust; this is an example consumer path, not generic PST infrastructure. The same trust mode also recognizes enterprise/private CAs installed through effective Windows policy.

## ST-D hardening and formal closure

The canonical result matrix is frozen as follows:

| Case | Expected PST result | Final evidence |
|---|---|---|
| Public TLS 1.2 under SYSTEM | `OK` | PASS; TLS `0x0303`, chain/hostname/authenticated true, no application bytes |
| Public TLS 1.3 under SYSTEM | `OK` | PASS; TLS `0x0304`, chain/hostname/authenticated true, no application bytes |
| Trusted chain, wrong hostname | `HOSTNAME_MISMATCH` | PASS; terminal, immutable diagnostic, no resurrection |
| Private PST PKI under SYSTEM | `AUTH_FAILURE` | PASS; no CUSTOM anchor leakage |
| Same private PKI under CUSTOM | `OK` | PASS; TLS 1.3 and exact 25-byte echo |
| Same-runtime S1/C1/S2/C2/S3 | mixed as specified | PASS; recovery, immutable failure snapshot, empty ERR queue |
| Two-runtime isolation | `OK` | PASS; R2 remains functional after R1 release |
| Adapter policy-error path | certificate rejected / `AUTH_FAILURE` at provider boundary | PASS; API BOOL true does not override chain or policy error |
| Public endpoint unavailable | `ENVIRONMENT_FAILURE` | Runner distinguishes DNS, no IPv4, and bounded TCP-unreachable failures from `PST_SYSTEM_TRUST_FAILURE` |
| Clean connection destruction | `OK` | PASS; peer snapshot remains owned after connection destruction |
| Failure destruction | preserved root result | PASS; failure snapshot remains valid and cleanup does not overwrite it |

The final public revalidation on Windows 10 build 19045 passed `www.cloudflare.com` and `www.google.com` at TLS 1.2 and TLS 1.3. Every connection reported capability mask `0x00000e7f`, `CERT=1`, `CHAIN=1`, `HOSTNAME=1`, `AUTH=1`, nonzero cipher, SHA-256 length 32, owned leaf DER, and zero application bytes. DNS and TCP reachability are bounded environment preflights; there is no automatic endpoint retry. A remote endpoint that is reachable but then produces a PST handshake or policy failure is reported separately as `PST_SYSTEM_TRUST_FAILURE`.

Failure precedence is frozen: a Windows certificate-policy rejection maps to `AUTH_FAILURE`; after Windows trust succeeds, OpenSSL hostname evidence maps to `HOSTNAME_MISMATCH`; an adapter operational failure without a certificate-policy conclusion maps to `BACKEND_FAILURE`. Terminal connection state preserves that first reliable cause. Malformed DER is certificate rejection. Allocation, memory-store, chain-call, and policy-call operational failures are reviewed paths but are not forced by production test hooks; no synthetic internal-failure claim is made.

Adapter acceptance requires all four conditions: `CertGetCertificateChain` succeeds, `TrustStatus.dwErrorStatus == 0`, `CertVerifyCertificateChainPolicy` returns true, and `CERT_CHAIN_POLICY_STATUS.dwError == 0`. The memory store contains only server-supplied untrusted intermediates. It is never an anchor store and no intermediate is promoted to root. Production opens no system ROOT, CA, or Disallowed store for mutation: `SYSTEM_STORE_MUTATION=NONE`. Explicit distrust is delegated to the effective Windows chain engine.

Network and refresh statements are deliberately bounded: `AIA_NETWORK_FETCH=DISABLED`, automatic root-update retrieval is disabled for this chain operation, and `REVOCATION_POLICY=NOT_PERFORMED`. PST does not freeze or administer Windows roots globally. `TRUST_REFRESH=PER_CONNECTION_WINDOWS_CHAIN_EVALUATION`; current effective policy is evaluated for each new connection. Enterprise and Group Policy behavior is `STRUCTURALLY_SUPPORTED_BY_WINDOWS_CHAIN_ENGINE`, while `RUNTIME_DOMAIN_TEST=NOT_PERFORMED`. `HCCE_CURRENT_USER` means the effective chain-engine view for the caller, including applicable inherited machine and policy stores; it does not mean manual enumeration of CurrentUser ROOT.

OpenSSL remains the sole hostname authority and the adapter requests server-auth EKU with `USAGE_MATCH_TYPE_AND`; there is no ANY_EKU broadening. The synchronous verify callback runs inside `SSL_do_handshake`, creates no worker and cannot outlive connection state. Temporary DER copies, `CERT_CONTEXT`, memory store, chain context, OpenSSL `X509`, `SSL`, `SSL_CTX`, and socket ownership follow exactly-once cleanup. Release without explicit shutdown is bounded local destruction and performs no new trust evaluation.

Logging and disclosure gates passed. OFF produced zero events for public success and private failure. A representative private trust failure at ERROR produced exactly one logical ERROR and no WARN. TRACE failure produced six bounded events, including one ERROR and two TRACE events. A public authenticated SYSTEM success followed by the endpoint's known non-reciprocal shutdown produced nine bounded events, including four TRACE events and one terminal shutdown ERROR; trust/hostname success remained unchanged. Public diagnostics/logs expose normalized result, operation, and `backend_id=openssl`, never Windows/OpenSSL native codes or strings, certificate identity/material, hostname/endpoint text, handles, pointers, or socket values. Peer certificate information remains available only through the explicit owned peer-info API. The OpenSSL ERR queue was empty after failure/recovery; Windows native status is consumed immediately within the synchronous adapter and never persisted publicly.

The final capability mask decodes to TLS 1.2 (`0x1`), TLS 1.3 (`0x2`), client auth (`0x4`), ALPN (`0x8`), custom trust (`0x10`), system trust (`0x20`), hostname verification (`0x40`), peer info (`0x200`), nonblocking (`0x400`), and backend wait (`0x800`): `0x00000e7f`. SYSTEM_TRUST is the sole delta from `0x00000e5f`. Registration order Schannel then OpenSSL selects Schannel for TLS 1.2 plus SYSTEM, OpenSSL for TLS 1.3 plus SYSTEM, and Schannel for the common TLS 1.2 custom policy. Exact OpenSSL TLS 1.3 SYSTEM succeeds; exact Schannel rejects it as `UNSUPPORTED`; ordered `[openssl, schannel]` TLS 1.2 SYSTEM selects OpenSSL.

Known limitations are explicit: no online revocation check, no PST-driven AIA retrieval, no PST-driven automatic root-update retrieval, no domain-joined runtime certification, OpenSSL hostname checking, and effective Windows trust evaluated at connection time. SYSTEM_TRUST remains host-OS policy. A possible future portable/updatable Papinho trust store would be a CUSTOM_TRUST source and is a separate project.

PapinhoAccelerator on Windows 10 using PST/OpenSSL for TLS 1.3 plus Windows SYSTEM trust is an example consumer path specific to PapinhoBrowser, not part of PST architecture. The same feature also serves enterprise, LAN, and private services whose corporate CA is trusted by effective Windows policy.

OSSL-ST-A through ST-D are coherent and complete: architecture, provider-local implementation, real functional/isolation proof, selection, hardening, and documentation agree. That work did not change the public API or SPI; the frozen release baseline is API 1.3.0, library 0.4.0 and SPI 2.4. Phase 9 is complete.
