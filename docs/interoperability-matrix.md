# Interoperability Matrix

## Status and scope

Phase 7.F is in progress. This document is the canonical bounded interoperability matrix for the current release. `TESTED` means an execution is recorded, `SUPPORTED` is an explicit implemented contract, `EXPECTED` is architectural expectation without execution, `NOT TESTED` has no evidence, and `UNSUPPORTED` is explicitly unavailable.

The current tested target is Win32 x86 with a VC6/C89 consumer, the RetroZilla NSS 3.42 Beta and NSPR 4.7.7 provider, the private Win32 socket adapter, and the versioned VC6 runtime. Tests cover a modern Windows development host and real Windows NT 4.0 SP6. Windows 2000, XP, 95/98, and Win32s are not inferred from NT4 evidence.

## Dimension classification

| Dimension | Current mandatory release scope | Future or non-blocking |
|---|---|---|
| Consumer/compiler ABI | VC6 C89 `/W4`, Win32 x86, public-header-only consumer | other compilers and architectures |
| Operating system | modern Windows host and real NT4 SP6 | Windows 2000/XP validation; Windows 95/98; Win32s |
| TLS versions | TLS 1.2 and TLS 1.3, fixed/range policy and no downgrade | older TLS and future versions |
| Server implementation | OpenSSL-backed deterministic fixture plus one bounded independent implementation | broad TLS-stack or Internet-server campaign |
| Authentication | server authentication and mTLS using in-memory DER | PKCS#12, passwords, hardware and callbacks |
| ALPN | required, optional, disabled, ordered multiple offer and mismatch | HTTP protocol semantics |
| Certificate/trust | custom CA DER, localhost verification, current RSA profile, one intermediate-chain proof | system trust, algorithm matrix, IDNA and IP-literal claims |
| Close behavior | close_notify, data then close_notify, FIN without close_notify | exhaustive peer shutdown variants |
| Backend/provider | `retrozilla-nss`, exact versioned runtime lineage | additional PST backends (Phase 8), arbitrary NSS versions |
| Transport | consumer-provided connected Win32 socket through the private adapter | POSIX and other adapters |
| Lifecycle | bounded repeated lifecycle and multiple connections | long-run/stress (Phase 7.H) |
| Failure behavior | representative incompatible peers fail closed with 7.A diagnostics | exhaustive native error taxonomy |

## Canonical evidence matrix

| Dimension | Client/platform and toolchain | PST backend | TLS | Server implementation | Authentication / ALPN / trust | Transport | Expected behavior | Evidence | Kind/location | Status | Scope and limitations |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Public consumer ABI | modern Windows, VC6 C89 `/W4` | neutral | n/a | n/a | n/a | opaque `pst_transport` | public header compiles alone | `test_public_header` | real compile, host | TESTED/PASS | mandatory; not Phase 9 ABI freeze |
| Host TLS 1.2 | modern Windows, VC6 x86 | RetroZilla NSS | 1.2 | Python `ssl`, OpenSSL 3.5.7 | mTLS, required `fixture/1`, custom CA | Win32 socket/NSPR import | authenticated 25-byte echo | runtime integration, Phases 5/7.C/7.D/7.E | real, host | TESTED/PASS | mandatory |
| Host TLS 1.3 | modern Windows, VC6 x86 | RetroZilla NSS | 1.3 | Python `ssl`, OpenSSL 3.5.7 | mTLS, required `fixture/1`, custom CA | Win32 socket/NSPR import | authenticated 25-byte echo | runtime integration, Phases 5/7.C/7.D/7.E | real, host | TESTED/PASS | mandatory |
| NT4 TLS 1.2 | Windows NT 4.0 SP6, VC6 x86 | RetroZilla NSS | 1.2 | Python `ssl`, OpenSSL-backed host fixture | mTLS, required `fixture/1`, custom CA | NT4 Winsock/NSPR import | authenticated bidirectional I/O | Phase 6 package/evidence | real, NT4 | TESTED/PASS | mandatory |
| NT4 TLS 1.3 | Windows NT 4.0 SP6, VC6 x86 | RetroZilla NSS | 1.3 | Python `ssl`, OpenSSL-backed host fixture | mTLS, required `fixture/1`, custom CA | NT4 Winsock/NSPR import | authenticated bidirectional I/O | Phase 6 package/evidence | real, NT4 | TESTED/PASS | mandatory |
| Server-auth only | host and NT4 | RetroZilla NSS | 1.2 | Python `ssl` | no client credential, custom CA, localhost | Win32 socket adapter | authenticated server and echo | Phase 5/6 server-auth fixture | real | TESTED/PASS | TLS 1.3 server-auth is not separately claimed |
| mTLS | host and NT4 | RetroZilla NSS | 1.2/1.3 | Python `ssl` | client DER/PKCS#8, custom CA | Win32 socket adapter | mutual authentication succeeds | Phase 5/6/7 functional evidence | real | TESTED/PASS | current DER source only |
| ALPN | host and NT4 | RetroZilla NSS | 1.2/1.3 | Python `ssl` | required/optional/disabled/multiple/mismatch | Win32 socket adapter | selected protocol copied; required mismatch fails | `test_tls_policy`, Phase 6/7.D | mock + real | TESTED/PASS | no HTTP semantics |
| Custom trust and hostname | host and NT4 | RetroZilla NSS | 1.2/1.3 | Python `ssl` | custom CA DER, `localhost` | Win32 socket adapter | valid peer passes; wrong hostname/untrusted CA fail | Phase 4/5/6/7.D | real | TESTED/PASS | IP literal and IDNA not claimed |
| System trust | all | RetroZilla NSS | n/a | n/a | system store | n/a | unavailable in current legacy provider | capability/configuration evidence | deterministic | UNSUPPORTED | not a 7.F failure |
| Nonblocking readiness | host and NT4 | RetroZilla NSS/NSPR | 1.2/1.3 | Python `ssl` | authenticated fixtures | Win32 socket, `PR_Poll` | WOULD_BLOCK plus bounded progress | Phase 6/7.C | mock + real | TESTED/PASS | exact network mask sequence is not promised |
| Close semantics | host and NT4 | RetroZilla NSS | 1.3 representative | Python `ssl` fixture | authenticated | Win32 socket adapter | clean close, data+clean close, truncated FIN classification | Phase 7.B | real | TESTED/PASS | provider-local close-notify adaptation |
| Repeated lifecycle | host and NT4 | RetroZilla NSS | 1.3 | Python `ssl` | mTLS, required ALPN | fresh Win32 socket per cycle | three create/I/O/shutdown/destroy cycles | lifecycle integration, Phase 6/7.E | real | TESTED/PASS | not stress/concurrency |
| Representative failures | host and NT4 where recorded | RetroZilla NSS | mixed | Python `ssl` plus non-TLS fixture | non-TLS, mismatch, wrong host, bad CA, missing client credential, ALPN mismatch, truncation | Win32 socket adapter | terminal normalized failure, no resurrection | Phase 6/7.B/7.D | mock + real | TESTED/PASS | full native taxonomy not duplicated |
| Independent TLS server | modern Windows | RetroZilla NSS | at least 1.2; 1.3 if fixture supports it | non-OpenSSL engine, preferred Schannel | server-auth; echo/content validation | loopback Win32 socket | successful deterministic handshake and echo | not yet implemented | real, host | NOT TESTED | mandatory bounded 7.F gap; one implementation only |
| Intermediate chain | modern Windows | RetroZilla NSS | 1.2 or 1.3 | deterministic local fixture | root to intermediate to RSA leaf; custom root trust | Win32 socket adapter | chain validates, hostname authenticates, echo succeeds | no existing chain-length greater than one fixture found | real, host | NOT TESTED | mandatory bounded 7.F gap |
| Negotiated cipher | modern Windows | RetroZilla NSS | 1.2 and 1.3 | deterministic fixture | normal positive cases | Win32 socket adapter | `PST_PEER_INFO_SUMMARY.cipher_suite != 0` | backend populates from `SSLChannelInfo`; current functional runner does not assert it | implementation plus pending assertion | SUPPORTED/NOT TESTED | mandatory cheap functional assertion; no cipher-suite matrix |

## Platform matrix

| Platform | Classification | Evidence/limitation |
|---|---|---|
| Modern Windows development host | TESTED | deterministic suite and real NSS TLS/failure/lifecycle fixtures |
| Windows NT 4.0 SP6 | TESTED | real Phase 6 and Phase 7.B execution |
| Windows 2000 | NOT TESTED | optional future validation; not inferred from NT4 |
| Windows XP | NOT TESTED | optional future validation; not inferred from NT4 |
| Windows 95/98 | NOT CURRENTLY VALIDATED | outside current mandatory release proof |
| Win32s/Windows 3.11 | FUTURE | no current claim |

## ABI, provider and certificate audit

`papinho_secure_transport.h` uses explicit PST integer types, `pst_size`, opaque handles, `PST_CALL` and `PST_API`. It contains no `SOCKET`, `HANDLE`, NSS/NSPR type, provider-native structure or native descriptor. The separate Win32 convenience header accepts a socket value as `pst_size` without exporting the Winsock type. This is an interoperability boundary audit, not an ABI freeze.

The only implemented PST backend is `retrozilla-nss`. The tested lineage is RetroZilla NSS 3.42 Beta, NSPR 4.7.7, VC6 Win32 x86 with the repository's versioned runtime. Compatibility with arbitrary NSS versions is not claimed. Additional PST backends belong to Phase 8; no second transport adapter is created in 7.F.

The current server fixture is a SHA-256-with-RSA certificate with a 2048-bit RSA public key, DNS SAN `localhost`, issued directly by the test root. It proves only this certificate/key profile and a root-to-leaf chain. ECDSA and an algorithm matrix are not mandatory for the current release. One deterministic intermediate-chain case is mandatory because chain validation is part of the current authentication contract and direct issuance does not exercise chain building.

Python 3.14.7 reports OpenSSL 3.5.7 for the existing server fixtures. Running `openssl s_server` would therefore not provide an independent engine. One bounded non-OpenSSL server is mandatory if practical; a minimal local Schannel server is the preferred audit result. It should prove server-auth and echo on TLS 1.2, plus TLS 1.3 only if the host Schannel configuration supports it, without turning the fixture into a reusable TLS framework.

## Mandatory closure set and gaps

The existing evidence already passes VC6 public-header consumption, host and NT4 TLS 1.2/1.3, server authentication, mTLS, ALPN, custom CA, hostname verification, nonblocking progress, clean/truncated closure, repeated lifecycle and representative fail-closed peers.

Phase 7.F remains open for exactly these bounded functional gaps:

1. one deterministic non-OpenSSL server implementation;
2. one positive root/intermediate/leaf chain fixture;
3. functional assertion and reporting of a nonzero negotiated cipher for TLS 1.2 and TLS 1.3.

A separate TLS 1.3 server-auth-only row may be collected while exercising those fixtures, but it is not a new credential-format or platform requirement. Multi-chunk real I/O is optional because partial I/O is already deterministic and large/long traffic belongs to Phase 7.H. Public Internet servers, Windows 2000/XP, a second PST backend, a second transport adapter, ECDSA coverage, exhaustive ciphers and long-run testing do not block 7.F.

## Deferred housekeeping

Preserving the exact RetroZilla NSS source lineage, revision, patchset, VC6 build process, SDK/dist and prebuilt runtime provenance so `C:\PSTW` becomes disposable remains separate and does not block Phase 7.F.