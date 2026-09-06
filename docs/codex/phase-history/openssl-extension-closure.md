<!-- SPDX-License-Identifier: MPL-2.0 -->

# OpenSSL extension three-provider closure audit

Status: OSSL-F and the post-Phase-8 OpenSSL provider extension are complete. Phase 9 has not started.

## Provider inventory and capability intersection

| Capability | RetroZilla NSS | Schannel | OpenSSL |
|---|---:|---:|---:|
| TLS 1.2 | yes | yes | yes |
| TLS 1.3 | yes | no | yes |
| custom trust | yes | yes | yes |
| system trust | no | yes | yes |
| hostname verification | yes | yes | yes |
| ALPN | yes | yes | yes |
| client authentication | yes | yes | yes |
| peer information | yes | yes | yes |
| nonblocking | yes | yes | yes |
| backend wait | yes | yes | yes |

The masks verified from descriptors and tests are NSS `0x00000e5f`, Schannel `0x00000e7d`, and OpenSSL `0x00000e7f`. Their exact three-provider intersection remains `0x00000e5d`: TLS 1.2, custom trust, hostname verification, ALPN, client authentication, peer information, nonblocking operation and backend wait. TLS 1.3 and system trust remain deliberate three-provider differences; SYSTEM_TRUST is now shared by the two modern Windows providers.

Canonical targets remain separate: VC6/NT4 packages contain RetroZilla NSS; the modern Schannel target uses the operating-system provider; the modern OpenSSL target stages only OpenSSL 3.5.8; the Schannel plus OpenSSL build is a combined model/test target unless explicitly packaged later. No package is required to contain every provider.

## Three-provider semantic comparison

Across the common intersection, qualified real evidence establishes TLS 1.2 with custom root/intermediate/localhost, required `fixture/1` ALPN, explicit DER/PKCS#8 mTLS identity, exact 25-byte echo, authenticated peer information and bounded shutdown. The same server leaf produces a 32-byte SHA-256 certificate fingerprint and normalized certificate-present, chain-validated, hostname-validated and peer-authenticated flags. Cipher suites are nonzero standard numeric identifiers but need not match.

All three providers fail closed for wrong custom root, missing intermediate, wrong hostname, required ALPN absence, missing credentials, abrupt EOF and non-TLS peers. Optional ALPN absence establishes the connection and returns UNAVAILABLE; multiple ALPN preserves the selected offered value. A provider reports AUTH_FAILURE only when reliable verification or alert evidence survives; an alert/close race may honestly yield a generic protocol, transport or backend category. No provider infers authentication from peer intent.

Authenticated `close_notify` is CLOSED/CLEAN; application data preceding it is delivered exactly once. EOF without it is FAILED/TRUNCATED. Established-session RST is explicitly TRUNCATED for OpenSSL; comparable NSS/Schannel transport evidence remains provider/stage dependent. Fatal wait is terminal and bounded. Specific TLS/authentication/hostname/policy evidence precedes later cleanup/socket failure.

NSS and OpenSSL both pass TLS 1.3 custom-trust, hostname, ALPN, mTLS and secure-I/O gates. On the validated Windows 10 build 19045, Schannel does not advertise TLS 1.3; this is an honest capability difference and a primary reason for OpenSSL. Exact and ordered selection never substitutes a provider. Registration order `schannel`, then `openssl`, yields Schannel for common TLS 1.2 and system trust, OpenSSL for TLS 1.3 plus custom trust/hostname, and OpenSSL for TLS 1.3 plus system trust.

## Readiness, lifecycle and disclosure

The provider-private mechanisms differ: NSS uses `PR_Poll`, Schannel uses WinSock `select`, and OpenSSL uses `select` plus WANT_READ/WANT_WRITE and `SSL_pending`. The public contract is the same: bounded nonblocking progress, partial-I/O safety, terminal fatal wait and no polling spin. OpenSSL's fresh 4 MiB backpressure proof observed WANT_WRITE with exact content; the Schannel/NSS deterministic readiness matrices prove their corresponding partial paths.

Ownership is identical externally: before `ownership_accepted`, the caller/core owns the transport; afterward the provider owns it and closes it exactly once. NSS's process-global active-state limitation remains provider-local. Schannel imposes no generic singleton, and multiple independent OpenSSL `OSSL_LIB_CTX` runtimes have been proven. Same-runtime success/failure/success, diagnostic clearing, immutable snapshots, release isolation and balanced lifecycle evidence exist for every provider where its lifecycle permits.

Diagnostics expose only normalized result, operation and backend ID. Logging OFF emits no event, representative ERROR emits one logical event, and INFO/TRACE progress is bounded. Neither surface exposes keys, certificate/trust DER, payload, hostname/ALPN text, endpoints, native errors, handles, pointers, Schannel key containers, or OpenSSL object identity. Peer certificate data remains available solely through the explicit peer-info API.

## Provenance and distribution

OpenSSL is the official 3.5.8 LTS source archive with published SHA-256 `a8f84a39918ec6415ce765d9b429d313ba97b8143169c172e734b9514464f5b2`, Apache-2.0 notices, retained canonical build procedure, manifested staged DLLs, default provider only, and legacy/FIPS absent. The target does not resolve a random PATH OpenSSL.

RetroZilla revision `2f274574d3c6ee8769914046920d649bbae9f81b`, NSS 3.42 Beta, NSPR 4.7.7, the NT4 RNG patch, source/SDK/runtime manifests and licenses remain repository-contained. `REQUIRED_UNIQUE_FILES_IN_PSTW=0`. Schannel is supplied by Windows; no third-party Schannel runtime is bundled.

## Historical OpenSSL system-trust decision at OSSL-F

At OSSL-F closure, OpenSSL did not advertise SYSTEM_TRUST and the enhancement was deliberately deferred. OSSL-ST-A through ST-D subsequently implemented and formally closed it. OpenSSL now advertises the capability at `0x00000e7f`, and TLS 1.3 plus Windows system trust selects OpenSSL on the tested Windows 10 runtime.

A secure implementation is not merely importing the CurrentUser ROOT store. It must define CurrentUser versus LocalMachine merge and precedence, enterprise/group-policy roots, the Disallowed store, update/snapshot lifetime, intermediate-store use, AIA/network-fetch policy, revocation policy, and the semantic difference between Windows chain-engine evaluation and OpenSSL X509 validation. Simply copying trusted roots would not reproduce Windows distrust or revocation behavior.

The completed follow-up remained provider-local: the OpenSSL target links CryptoAPI, uses a private Win32 chain-engine adapter, and adds focused selection/integration tests and security documentation. It required no public API or SPI change. The final design is documented in `docs/openssl-system-trust.md`.

Phase 9 should consider library version `0.4.0` for the fully validated third provider, but owns the actual release/version decision. Current versions remain API 1.2.0, library 0.3.0 and SPI 2.4.
