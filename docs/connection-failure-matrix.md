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

Package preparation is READY. Real NT4 now proves clean_close and data_then_close PASS; abrupt_close remains the next target.

## Phase 7.B NT4 failure timeline diagnostic

An early data_then_close attempt was invalid fixture/operator evidence: the server printed READY, its former 10-second initial accept window expired before the operator launched the NT4 client, and the later client connect ended with native error 10060. This timeout occurred before PST could connect and must not be attributed to PST. A prompt repeat reached ACCEPT, negotiated TLS 1.3 with authenticated mTLS and ALPN fixture/1, sent the expected 29 bytes, completed TLS close_notify, and the NT4 client reported TOTAL_READ=29, clean close and PASS. clean_close also passed on real NT4.

The diagnostic client and backend now share one GetTickCount epoch. failure-client.log records configured bounds, every handshake/read/wait/shutdown boundary, operation/result/bytes/read total/close kind, loop elapsed time, terminal check, every release boundary and total process elapsed time. failure-backend.log records operational NSS/NSPR events. failure-modules.log contains only module paths.

Modern clean_close diagnostic evidence:

- total elapsed: 234 ms;
- attach began handshake at 46 ms;
- handshake completed at 218 ms after one 109 ms poll;
- PR_Read began at 218 ms and returned zero/CLOSED clean at 234 ms;
- no HUP, ERR or NVAL was reported because no PR_Poll was needed after handshake;
- connection, config, credentials, trust and runtime release all completed at 234 ms.

The next real NT4 run is abrupt_close. Phase 7.B remains in progress because the existing abrupt-close classification blocker remains unresolved.

### NT4 diagnostic log creation correction

The initial NT4 diagnostic console output still had perceptible client-side delays after the server exited, and none of the three diagnostic files was created. Console timing cannot substitute for the missing files. The client now opens failure-client.log directly with C89 fopen mode w, reports numeric errno on failure and keeps sparse timestamped console markers. Backend and module traces use separate relative files, create them with mode w on first event and report open failures. The BAT sets all three expected environment variables and no longer redirects stdout. The 79 ms modern clean_close validation proves log creation only; it is not evidence that the NT4 delay disappeared.

### NT4 pre-handshake timeline package

The real NT4 evidence stopped after BOUNDS, so the diagnostic now covers every setup boundary through HANDSHAKE_BEGIN and routes every setup failure through timestamped conditional cleanup. It records fixture loading, trust, credentials, config, identity, ALPN/TLS policy, backend registration, runtime creation, Winsock startup, socket creation, blocking connect, transport creation, connection creation and attach. Environment reporting is limited to presence flags and equality with the three test-controlled relative filenames.

Trace creation is demand-driven: failure-modules.log first opens while RUNTIME_CREATE loads and inventories NSS/NSPR modules; failure-backend.log first opens during CONNECTION_CREATE when the backend creates NSS connection state. Their absence before those boundaries is therefore not evidence of fopen failure.

Modern clean_close passed with all three logs: runtime creation ended at 32 ms, blocking connect ended successfully at 32 ms, connection creation and attach ended at 32 ms, handshake completed at 79 ms, clean EOF was read at 79 ms, and cleanup/total also ended at 79 ms. Real NT4 subsequently passed clean_close and a valid data_then_close execution. The next real NT4 run is abrupt_close. Phase 7.B remains in progress.

### Manual fixture accept window

The fixture now uses an explicit 120-second timeout only for the initial listener accept and prints ACCEPT_TIMEOUT_SECONDS=120 on READY. If no client arrives it reports FIXTURE_TIMEOUT STAGE=ACCEPT REASON=NO_CLIENT before exiting. Once ACCEPT succeeds, all existing 10-second per-operation socket/TLS bounds remain unchanged. The modern matrix was rerun: every previously passing mode still passed, while abrupt_close/read_abrupt retain the already documented NSS classification blocker.

## RetroZilla NSS close_notify observability audit

The exact examined lineage is RetroZilla source record 2f274574d3c6ee8769914046920d649bbae9f81b, NSS 3.42 Beta and NSPR 4.7.7, built as Win32 x86 with VC6. The examined build-tree ssl3.dll has the same SHA-256 as the versioned runtime (a45adb3ca8abfab4716315acba515f0de1ad4c56b095c89f32042dfc120277da).

PR_Read dispatches to the SSL layer's ssl_SecureRead/ssl_SecureRecv, then DoRecv, ssl3_GatherAppDataRecord, ssl3_GatherCompleteHandshake, ssl3_GatherData, and finally ssl_DefRecv on the lower NSPR descriptor. Raw transport EOF makes ssl3_GatherData return zero without setting a distinct public error. A valid alert record instead reaches ssl3_HandleAlert; for alert description zero (close_notify) it sets private ss->recvdCloseNotify, invokes the registered received-alert callback synchronously while processing the record, and the gather/read path ultimately also returns zero. Thus the ordinary PR_Read return value intentionally collapses the two cases, but the alert event remains observable.

The reliable supported mechanism is the public SSL_AlertReceivedCallback API. It is declared in ssl.h, implemented in sslsecur.c, listed in ssl.def since the NSS 3.30 symbol set, present in the VC6 link map, and exported by the exact versioned ssl3.dll. Upstream tests in this same source tree register it, receive close_notify, then verify that PR_Read returns zero. SSL_GetChannelInfo, SSL_SecurityStatus, SSL_OptionGet, SSL_DataPending, NSPR EOF/error state, and poll/HUP expose no equivalent closure distinction.

The recommended correction is provider-local: register an alert-received callback on the private SSL descriptor, retain only a private boolean when alert description zero is observed, and classify a later PR_Read == 0 as CLEAN only when that flag is set; otherwise use the existing TRUNCATED result/close-kind path. Callback registration failure must fail closed during provider attach. No TLS record parsing, HUP heuristic, NSS source patch, public PST API change, generic core change, or SPI change is required. This audit makes no behavior change; implementation and modern plus real-NT4 clean/data/abrupt regressions are the next step.

Cross-platform evidence before the provider correction was definitive: real NT4 clean_close PASS; real NT4 data_then_close PASS with all 29 bytes before close_notify; real NT4 abrupt_close FAIL because TCP FIN without close_notify became CLOSED/CLEAN. Windows 10 reproduced those three original provider outcomes.

## Phase 7.B3 provider-local close_notify implementation

The RetroZilla NSS provider now resolves and requires `SSL_AlertReceivedCallback`, registers it on each private SSL descriptor immediately after `SSL_ImportFD`, and retains a per-connection boolean only when alert description zero is observed. Registration failure fails attach closed as `BACKEND_FAILURE`. A later `PR_Read == 0` maps to `CLOSED/CLEAN` only when that connection observed close_notify; otherwise it maps to `FAILED/TRUNCATED`. Application data returned before the alert remains observable and is not discarded.

This change is confined to the private NSS provider. It does not parse TLS records, infer closure from HUP, patch NSS, or change the public API, generic core, SPI, readiness, ownership, timeout, or shutdown contracts.

Modern-host validation passed: `clean_close` ended `FINAL_STATE=CLOSED CLOSE_KIND=1` without a diagnostic; `data_then_close` delivered `READ=29 CONTENT_MATCH=1` before the same clean close; and `abrupt_close` plus `read_abrupt` ended `FINAL_STATE=FAILED CLOSE_KIND=2 DIAG_RESULT=TRUNCATED DIAG_OPERATION=READ`. The pre-handshake close/reset fixtures retained their existing failure mappings. TLS 1.2 and TLS 1.3 bidirectional regressions both reported `WRITE=25 READ=25 CONTENT_MATCH=1`.

Targeted real-NT4 revalidation subsequently passed for clean_close, data_then_close and abrupt_close. Phase 7.B then remained in progress for the shutdown-abort proof and closure audit.

## Phase 7.B4 shutdown-abort contract proof

The existing PST contract delegates shutdown completion to the provider's `shutdown_step`. The RetroZilla NSS provider calls `PR_Shutdown(ssl_fd, PR_SHUTDOWN_BOTH)`: `ssl_SecureShutdown` attempts `SSL3_SendAlert(close_notify)` without retrying or using its return value, immediately invokes shutdown on the lower NSPR descriptor, records the local send/receive shutdown bits, and returns that lower shutdown result. It does not wait for peer close_notify. On success PST reports `COMPLETE`, moves directly from `ESTABLISHED` through `SHUTTING` to `CLOSED`, clears diagnostics, and rejects all later connection operations.

Consequently, peer disappearance *during a pending shutdown step* is not observable for this provider in the tested contract. The synchronized fixture does not fabricate such a state. Instead it uses a separate bounded control connection: after authenticated TLS and the expected 24-byte payload, the server announces readiness; the client calls PST shutdown; the server observes the resulting transport shutdown, explicitly aborts/closes its side, and only then confirms the abort over the control connection. The client performs terminal-state checks and release only after that confirmation.

On Windows 10 the final fully bounded clean-build proof completed shutdown in one call and 0 ms (`SHUTDOWN_MAX_STEPS=80`, `SHUTDOWN_WAIT_MS=125`), with each client control wait bounded to 2000 ms and the second server accept bounded to 10 seconds; both final control waits completed in 0 ms. A preceding repeat of the same binary measured 16 ms for shutdown and 15 ms for post-abort confirmation, still within the same bounds. The result was `PST_RESULT_CLOSED`, `CLOSED/CLEAN`, no diagnostic, no ERROR/WARN event, and zero events at logging OFF. Handshake, read, write, wait and repeated shutdown all returned `INVALID_STATE` after the server-side abort; release completed without hang or duplicate ownership action. Ownership remained accepted exactly once and the provider descriptor/native socket remained connection-owned until release.

No PST source, API, SPI, backend behavior, timeout, readiness, ownership or shutdown semantics changed. The three-cycle clean lifecycle retained `SHUTDOWN_STEPS=1 SHUTDOWN_COMPLETE=1`, and clean_close, data_then_close and abrupt_close retained their expected classifications. Because only test orchestration and documentation changed, another NT4 execution is not required; the already completed real NT4 lifecycle and targeted close/data/abrupt evidence remain applicable. Phase 7.B stays in progress only for its closure audit.
