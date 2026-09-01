# Phase 7.B connection failure matrix

Status: **in progress**. This audit does not start Phase 7.C.

## Architectural findings

TCP connection establishment belongs to consumer-side transport setup. The public Win32 adapter wraps an already-connected socket; a refused connection therefore occurs before a PST transport exists and cannot naturally produce a PST connection diagnostic. It remains bounded in the consumer and leaves socket cleanup with that consumer.

After pst_connection_attach reports ownership_accepted=1, PST is the sole owner even if import subsequently fails. Before acceptance, the caller remains owner. Release destroys an accepted transport exactly once. No repeated-release guarantee is added.

Internal states are CREATED, ATTACHED, HANDSHAKING, ESTABLISHED, SHUTTING, CLOSED and FAILED. Handshake and I/O failures are terminal. The audit found that a backend wait failure was logged but did not change core state. It also found that the NSS adapter returned CLOSED for every PR_POLL_HUP, preventing PR_Read from distinguishing clean TLS close from truncation and potentially hiding buffered plaintext.

The minimal correction makes non-OK backend wait terminal: CLOSED only for PST_RESULT_CLOSED, otherwise FAILED. NSS poll ERR and NVAL remain TRANSPORT_FAILURE; HUP supplies READ readiness so the subsequent SSL read classifies clean EOF versus truncation. READ|HUP preserves readable plaintext. No public API, result value, SPI, ownership rule, TLS policy or logging ABI changed.

## Matrix

| Scenario / stage | Result | Final state / owner | Diagnostic / logging | Evidence | Status |
|---|---|---|---|---|---|
| Runtime/connection creation failure | existing normalized constructor result | no usable object | copied 7.A constructor diagnostic | test_diagnostic_creation, test_logging | proved |
| TCP connect refused / no listener | consumer transport error; no PST result yet | no PST connection; consumer closes | no PST native disclosure | transport boundary audit | outside PST |
| Attach before ownership acceptance | attach error | CREATED/retryable; caller owns | TRANSPORT operation | test_backend_spi and code audit | proved |
| Attach after acceptance/import failure | normalized attach failure | FAILED; PST closes once | TRANSPORT operation | SPI/NSS ownership tests | proved |
| Peer closes before TLS | never ESTABLISHED; native-normalized failure | FAILED; PST owns | HANDSHAKE | dedicated fixture required | pending |
| Peer sends non-TLS bytes | normally PROTOCOL_FAILURE, fail closed | FAILED | HANDSHAKE | mapping audit | pending execution |
| Abrupt close/reset during handshake | bounded failure | FAILED | HANDSHAKE | dedicated fixture required | pending |
| Wrong hostname | HOSTNAME_MISMATCH | FAILED | AUTHENTICATION | Phase 4/5 and real NT4 Phase 6 | reused PASS |
| Untrusted CA | AUTH_FAILURE | FAILED | AUTHENTICATION | Phase 4/5 and real NT4 Phase 6 | reused PASS |
| Missing required client credential | justified protocol/transport failure | FAILED | HANDSHAKE/AUTHENTICATION | real NT4 Phase 6 | reused PASS |
| TLS 1.3 required vs TLS 1.2 only | PROTOCOL_FAILURE | FAILED; no downgrade | HANDSHAKE | Phase 5 and real NT4 Phase 6 | reused PASS |
| Required ALPN mismatch | policy/protocol failure | FAILED; never operational | CONFIGURATION/HANDSHAKE | Phase 5 | reused PASS |
| Normal established TLS I/O | OK | ESTABLISHED until shutdown | success clears stale failure | fresh TLS 1.2/1.3 echo | PASS |
| Clean TLS close after read | CLOSED/CLEAN | CLOSED | normal close, not ERROR | backend audit; fixture required | pending execution |
| Abrupt post-establish close/read | TRUNCATED | FAILED | READ | mapping test; fixture required | pending execution |
| Close around write | reported partial bytes then failure | FAILED | WRITE | fixture required | pending |
| PR_Poll READ/WRITE | matching interest | operational | progress only | Phase 6 readiness tests | proved |
| PR_Poll HUP or READ|HUP | OK + READ, then SSL read classifies | unchanged until read | progress | new NSS unit cases | host PASS; NT4 pending |
| PR_Poll ERR/NVAL | TRANSPORT_FAILURE | FAILED in core | WAIT | new NSS/core tests | PASS |
| Backend wait failure | TRANSPORT_FAILURE | FAILED; no resurrection | WAIT snapshot | new test_backend_spi case | PASS |
| Shutdown peer disappearance | bounded normalized failure | FAILED | SHUTDOWN | fixture required | pending |
| Release after failure | bounded and single close | destroyed | copied snapshot survives | SPI/diagnostic/lifecycle tests | proved |

PR_CONNECT_RESET_ERROR and PR_END_OF_FILE_ERROR normalize to TRUNCATED. Selected connectivity errors normalize to TRANSPORT_FAILURE; SSL errors to PROTOCOL_FAILURE; certificate/trust errors to AUTH_FAILURE; bad certificate domain to HOSTNAME_MISMATCH; unknown SEC/native errors to BACKEND_FAILURE. Several native causes may correctly share one portable result.

## Boundedness, security and observability

Operations remain incremental and wait has an explicit timeout. Existing integration loops and lifecycle shutdown use fixed step limits. WOULD_BLOCK and timeout remain progress, not WARN/ERROR. Logging stays observational and structurally redacted. Provider fallback, plaintext fallback, policy bypass and downgrade remain absent.

Fresh host runs restricted PATH to third_party/retrozilla-nss/prebuilt/win32-x86-vc6/runtime:

- TLS 1.2: TLS=0x0303 WRITE=25 READ=25 CONTENT_MATCH=1 ALPN=9 AUTH=2.
- TLS 1.3: TLS=0x0304 WRITE=25 READ=25 CONTENT_MATCH=1 ALPN=9 AUTH=2.
- Both servers: AUTH=True, ALPN=fixture/1, RECV=25 SEND=25 CONTENT_MATCH=True.
- OFF logging: zero events.
- Clean VC6 C89 /W4 suite and NSS unit test: PASS, zero warnings.

## Remaining closure gates

Phase 7.B cannot close yet. Deterministic executions remain required for pre-TLS close, non-TLS bytes, abrupt handshake close/reset, clean and abrupt post-establish close/read, close around write, and shutdown failure. Because the correction changes NSS HUP/readiness behavior, a targeted real NT4 HUP/read classification regression is required. Historical Phase 6 success is not claimed as validation of this new behavior.
## Phase 7.B2 functional fixture results

The single local fixture tests/connection_failure_server.py and VC6 public-API client tests/test_connection_failures.c use 80 operation steps, 125 ms waits and 10/12 second fixture/runner bounds.

| Mode | Modern observed result | Diagnostic / log at ERROR | Status |
|---|---|---|---|
| pre_tls_close | FINAL=TRANSPORT_FAILURE, never established | HANDSHAKE, one ERROR | PASS |
| non_tls | FINAL=TRANSPORT_FAILURE, never established | HANDSHAKE, one ERROR; fixture bytes absent | PASS; provider mapping justified |
| handshake_close | FINAL=TRUNCATED after ClientHello | HANDSHAKE, one ERROR | PASS |
| handshake_reset | explicit SO_LINGER RST; FINAL=TRUNCATED | HANDSHAKE, one ERROR | PASS and deterministic on host |
| clean_close / read_clean | FINAL=CLOSED, CLOSE_KIND=CLEAN | no diagnostic, no ERROR | PASS |
| data_then_close | READ=29, CONTENT_MATCH=1, then CLOSED/CLEAN | no stale diagnostic/error | PASS; primary HUP/read regression |
| abrupt_close / read_abrupt | FIN without close_notify observed as CLOSED/CLEAN | no diagnostic/error | FAIL: real NSS/backend limitation |
| close_around_write | WRITE=24, server RECV=24/MATCH, then TRUNCATED on read | READ diagnostic, one ERROR | PASS; local write acceptance is not delivery proof |
| shutdown_abort | local shutdown completed before peer abort became observable | no diagnostic/error | bounded/release-safe, not proof of shutdown-step failure |

OFF produced zero events for pre_tls_close. TRACE produced 17 progress events, zero ERROR/WARN, for data_then_close and did not alter result or bytes.

### Newly exposed truncation blocker

The abrupt fixture is reliable: TLS 1.3, mTLS and ALPN complete, then the underlying socket sends FIN without TLS close_notify. In this preserved NSS, PR_Read returns zero for both raw EOF and received close_notify. Public NSS headers expose no reliable close-notify-received state. Treating every zero read as truncated would break clean close; treating HUP as proof of truncation would misclassify close_notify followed by FIN. No speculative heuristic was added.

The modern failure matrix is therefore not fully ready and Phase 7.B remains in progress independently of the NT4 execution requirement.

### Targeted NT4 package

Package: build/nt4-validation/client

New files are test_connection_failures.exe and run_failure_regression.bat. The BAT is ASCII/CRLF, uses no PowerShell, percent-tilde expansion, exit or exit /b, and preserves ERRORLEVEL using the established ver/verify convention.

Modern server, once per mode:

    python tests\connection_failure_server.py 0.0.0.0 PORT MODE build\nt4-validation\server-fixture\server.pem build\nt4-validation\server-fixture\server.key build\nt4-validation\server-fixture\ca.pem 13 fixture/1 required

On NT4:

    run_tls13.bat HOST PORT localhost
    run_failure_regression.bat HOST PORT localhost data_then_close
    run_failure_regression.bat HOST PORT localhost clean_close
    run_failure_regression.bat HOST PORT localhost abrupt_close

Package preparation is READY; no NT4 PASS is claimed.
