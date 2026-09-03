# OpenSSL on Windows system-trust architecture

Status: OSSL-ST-B private Win32 adapter implemented and structurally tested. `SYSTEM_TRUST` remains capability-hidden until ST-C functional/isolation gates pass; Phase 9 has not started.

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

ST-B keeps `PST_CAP_SYSTEM_TRUST` absent (`0x00000e5f`) and selection behavior unchanged. ST-C owns real system-trusted TLS positives, wrong-hostname and isolation sequences; its runner may reuse the existing identity integration interface after capability advertisement. No full ST-C matrix was executed here.
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

The generic OpenSSL file owns TLS integration and normalized state; the Win32 adapter alone includes CryptoAPI types and links `crypt32.lib`. A future POSIX adapter may implement its own system-trust semantics. No public API or SPI change is expected: `PST_TRUST_SOURCE_SYSTEM` and `PST_CAP_SYSTEM_TRUST` already exist. The capability mask remains `0x00000e5f` until positive, negative, isolation, disclosure and cleanup gates pass.

Refined plan: ST-A policy/architecture (this document); ST-B bounded provider-local Win32 implementation; ST-C real TLS 1.2/1.3, negative, isolation and store-safety matrix; ST-D hardening and closure. Current versions remain API 1.2.0, library 0.3.0 and SPI 2.4. CryptoAPI is OS-provided, so there is no new third-party redistribution license.

The consumer motivation includes PapinhoBrowser through PapinhoAccelerator to arbitrary public TLS 1.3 sites, but PST remains generic. The same policy is useful for enterprise/LAN applications whose corporate roots are installed through effective Windows policy.
