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
| Independent TLS server | Windows 10 19045 host, VC6 PST client | RetroZilla NSS | 1.2 | PowerShell 5.1/.NET Framework `SslStream` over Schannel | server-auth, no ALPN, temporary self-signed RSA custom anchor | loopback Win32 socket | deterministic handshake and exact echo | `schannel_tls_server.ps1`, runtime integration | real, host | TESTED/PASS | `CIPHER=0xc030`, 25/25 match; identity removed by exact thumbprint; Schannel TLS 1.3 not executed on this host fixture |
| Intermediate chain | modern Windows, VC6 x86 | RetroZilla NSS | 1.2 and 1.3 | Python 3.14.7/OpenSSL 3.5.7 | root to intermediate to RSA leaf, custom root, localhost | Win32 socket adapter | chain validates, hostname authenticates, echo succeeds | generated PKI plus runtime integration | real, host | TESTED/PASS | TLS 1.2 `0xc030`, TLS 1.3 `0x1302`; missing-intermediate control returned `AUTH_FAILURE` |
| Negotiated cipher | modern Windows, VC6 x86 | RetroZilla NSS | 1.2 and 1.3 | Schannel and OpenSSL-backed fixtures | public peer summary | Win32 socket adapter | `PST_PEER_INFO_SUMMARY.cipher_suite != 0` | `test_tls_runtime_integration` public assertion/output | real, host | TESTED/PASS | TLS 1.2 `0xc030`; TLS 1.3 `0x1302`; IDs are observations, not frozen policy |

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

Python 3.14.7 reports OpenSSL 3.5.7 for the existing server fixtures. Running `openssl s_server` would therefore not provide an independent engine. The bounded independent fixture uses PowerShell 5.1/.NET Framework `SslStream`, whose Windows provider is Schannel. It passed TLS 1.2 server-auth and exact echo. The Windows 10/.NET Framework fixture does not expose a readily usable deterministic TLS 1.3 server mode, so independent TLS 1.3 was not executed and does not block 7.F; TLS 1.3 remains proven against OpenSSL on the host and real NT4.

## Phase 7.F2 functional evidence

`tests/generate_interop_pki.ps1` locates only the OpenSSL distributed with the existing Git installation and generates RSA-2048/SHA-256 test-only assets under `build/fixtures/interoperability-pki`. The positive chain is root, intermediate and `localhost` leaf; the server presents leaf plus intermediate while PST receives only root DER. Structural verification succeeds with the intermediate and fails without it.

`tests/schannel_tls_server.ps1` creates a temporary test identity through `New-SelfSignedCertificate`, exports only its public DER for PST custom trust, listens with a 120-second bounded accept, performs Schannel TLS 1.2, validates and echoes exactly 25 bytes, and removes only its exact thumbprint from `CurrentUser/My` and any Windows-created `CurrentUser/CA` copy in `finally`. It installs no persistent trust anchor and logs no private key. The real result was:

```text
PST:      TLS=0x0303 CIPHER=0xc030 WRITE=25 READ=25 CONTENT_MATCH=1 AUTH=2
Schannel: TLS=Tls12 CIPHER=Aes256 STRENGTH=256 RECV=25 SEND=25 CONTENT_MATCH=TRUE PASS
Cleanup:  STORE_REMAINING=0
```

The real intermediate-chain results were:

```text
TLS 1.2: TLS=0x0303 CIPHER=0xc030 WRITE=25 READ=25 CONTENT_MATCH=1 AUTH=2
TLS 1.3: TLS=0x0304 CIPHER=0x1302 WRITE=25 READ=25 CONTENT_MATCH=1 AUTH=2
Negative control without intermediate: HANDSHAKE_FAIL=9 DIAG_RESULT=9 NO_RESURRECTION=1
```

The normal mTLS/required-ALPN OpenSSL baselines also passed after the public cipher assertion: TLS 1.2 reported `0xc030`, TLS 1.3 reported `0x1302`, both with `WRITE=25 READ=25 CONTENT_MATCH=1`, `AUTH=True`, and `ALPN=fixture/1`.
Generate the intermediate-chain assets from the repository root with:

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File tests\generate_interop_pki.ps1
```

Start the independent TLS 1.2 fixture in one shell:

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File tests\schannel_tls_server.ps1 -BindAddress 127.0.0.1 -Port 8462 -CertificateOutputPath build\fixtures\schannel\server.der -AcceptTimeoutSeconds 120
```

After `READY`, run the public PST client from a bootstrapped shell with the canonical runtime-only `PATH`:

```bat
build\vc6\test_tls_runtime_integration.exe 127.0.0.1 8462 localhost build\fixtures\schannel\server.der - - 12 12 - disabled
```

For the intermediate-chain gate, use `tests\nt4_tls_server.py` with `server-chain.pem`, `server.key`, and `root.pem`; pass only `root.der` to the PST client. For the negative control, replace `server-chain.pem` with `server.pem` while retaining root-only PST trust.

## Mandatory closure set

The existing evidence already passes VC6 public-header consumption, host and NT4 TLS 1.2/1.3, server authentication, mTLS, ALPN, custom CA, hostname verification, nonblocking progress, clean/truncated closure, repeated lifecycle and representative fail-closed peers.

All three bounded functional gaps are closed:

1. deterministic Schannel TLS 1.2 server-auth and echo: PASS;
2. positive root/intermediate/leaf chain in TLS 1.2 and TLS 1.3 plus negative omission control: PASS;
3. public `cipher_suite != 0` assertion/reporting in TLS 1.2 and TLS 1.3: PASS.

Phase 7.F remains in progress only for its separate closure audit. Multi-chunk real I/O is optional because partial I/O is already deterministic and large/long traffic belongs to Phase 7.H. Public Internet servers, Windows 2000/XP, a second PST backend, a second transport adapter, ECDSA coverage, exhaustive ciphers and long-run testing do not block 7.F.

## Deferred housekeeping

Preserving the exact RetroZilla NSS source lineage, revision, patchset, VC6 build process, SDK/dist and prebuilt runtime provenance so `C:\PSTW` becomes disposable remains separate and does not block Phase 7.F.