<!-- SPDX-License-Identifier: MPL-2.0 -->

# OpenSSL failure, close, diagnostics and lifecycle hardening

Status: OSSL-E complete. Phase 9 and OSSL-F have not started.

The canonical OpenSSL 3.5.8 provider remains capability mask `0x00000e5f`; API 1.2.0, library 0.3.0 and SPI 2.4 are unchanged. OSSL-E added test-fixture coverage only. No production source or public ABI changed.

## Canonical result matrix

| Case | Canonical PST outcome | Evidence |
|---|---|---|
| TLS 1.2 / TLS 1.3 success | OK, ESTABLISHED | Fresh 10-message exact-version secure echoes |
| Plaintext peer | PROTOCOL_FAILURE, FAILED | Fresh deterministic plaintext server |
| 1.3 client vs 1.2 server; inverse | PROTOCOL_FAILURE, FAILED | Fresh exact-bound mismatch runs |
| Wrong root / missing intermediate | AUTH_FAILURE, FAILED | OSSL-D/D2 real matrix |
| Wrong hostname | HOSTNAME_MISMATCH, FAILED | OSSL-D real matrix |
| Required ALPN absent or unoffered | POLICY_VIOLATION, FAILED before application I/O | OSSL-D plus deterministic policy matrix |
| Missing/untrusted client credential | AUTH_FAILURE when the TLS alert survives; otherwise honest generic provider/transport result | OSSL-D real mTLS matrix; no result is invented from peer intent |
| Local certificate/key mismatch | AUTH_FAILURE during connection configuration; no operational connection | OSSL-D local negative |
| `close_notify` | CLOSED/CLEAN | Fresh peer-clean run |
| Data then `close_notify` | exact data, then CLOSED/CLEAN | Fresh data-then-close run |
| EOF without `close_notify` | TRUNCATED, FAILED, READ | Fresh peer-abrupt run |
| RST after established secure echo | TRUNCATED, FAILED | Fresh deterministic `SO_LINGER(1,0)` fixture |
| Fatal provider wait | TRANSPORT_FAILURE, FAILED, WAIT | Test-only invalidated transferred descriptor |
| Provider-internal/configuration failure | BACKEND_FAILURE unless stronger reliable evidence exists | Unit/configuration paths |

## Mapping and precedence

`SSL_ERROR_ZERO_RETURN` alone establishes authenticated clean closure. Unexpected EOF and `SSL_ERROR_SYSCALL` with zero return establish truncation; other syscall failures are transport failures. `SSL_ERROR_SSL` first considers reliable TLS-alert and X509 verification evidence: hostname mismatch remains distinct, certificate verification and supported certificate alerts are authentication failures, and protocol-stack failures are protocol failures. Generic failures are not promoted to authentication or policy failures. The provider drains its private ERR queue after classification and exposes no ERR code or text.

The first specific TLS, policy, authentication or hostname result is terminal. Later socket or cleanup outcomes cannot overwrite it. Required ALPN is checked after cryptographic handshake but before PST reports ESTABLISHED. No plaintext fallback, version widening, implicit credentials, trust-all behavior, `SSL_VERIFY_NONE`, or `SSL_OP_IGNORE_UNEXPECTED_EOF` exists.

## Lifecycle, diagnostics and logging

Fatal core states reject handshake, read, write, wait, shutdown and peer-info through the public state contract. The wait-fatal SPI reproducer additionally verifies provider failure, WAIT diagnostic phase and no handshake/read/write/shutdown resurrection. Public diagnostics contain only normalized result, operation and `backend_id=openssl`; snapshots are value copies and remain valid after connection/runtime destruction. The D2 B1-C-A-B2 sequence proves a prior failure snapshot remains immutable and a subsequent success has no stale diagnostic.

Logging reuses the provider-neutral hardened core: OFF has no callbacks; ERROR emits one logical terminal event; INFO/TRACE remain bounded structured events with no hostname, ALPN text, DER, key, trust bytes, payload, ERR/X509/socket details, endpoint, pointer or handle. Functional peer information remains available only through the explicit peer-info API. The deterministic logging/lifecycle suites cover callback lifetime and release in CREATED, ATTACHED, HANDSHAKING, ESTABLISHED, SHUTTING, CLOSED and FAILED states.

Connection destruction is local and bounded: `SSL_free`, `SSL_CTX_free`, then the single provider-owned `closesocket`; it does not perform an implicit network shutdown. Failed attach retains caller ownership unless `ownership_accepted` is set. Unit counters cover 100 provider init/shutdown cycles, multiple runtime isolation, accepted/unaccepted transport cleanup and empty ERR queue. D2 covers same-runtime success/failure/success and dual-runtime failure/release isolation.

## Fresh OSSL-E functional evidence

- TLS 1.2: 10/10 25-byte echoes, TLS `0x0303`, shutdown in 2 steps.
- TLS 1.3: 10/10 25-byte echoes, TLS `0x0304`, shutdown in 2 steps.
- Both exact version mismatches: PROTOCOL_FAILURE with ERR queue empty.
- Clean close, data then clean close, abrupt EOF and established-session RST: PASS.
- Fatal wait: TRANSPORT_FAILURE, diagnostic WAIT, no resurrection.
- 4 MiB backpressure: exact 4,194,304-byte echo with WANT_WRITE observed.
- Mini-stability: 100 consecutive 64-byte TLS 1.3 echoes on one connection; exact content and bounded shutdown.

The 4 MiB server deliberately emits three-byte fragments, so its high wait count tracks real byte progress rather than polling spin. Hard step bounds remained healthy for handshake (maximum 3) and shutdown (maximum 2); close tests completed well below 40 steps. Runtime DLL hashes matched the staged 3.5.8 manifest. Only the default provider was present; legacy and FIPS providers were absent and missing ambient configuration/module paths were deliberately supplied.

Prior OSSL-D/D2 evidence is retained for the complete identity, ALPN, B1-C-A-B2, dual-runtime, Schannel-independent-server, coexistence and selection matrices. OSSL-E does not replace those proofs. NT4 retest is unnecessary because no shared, NSS, core or production transport semantics changed.
