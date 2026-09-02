# Diagnostics and security disclosure matrix

Status: Phase 7.G complete. The disclosure audit, focused abuse matrix, functional logging equivalence, remote semantic equivalence, and formal closure audit all passed without production behavior or public ABI changes.

## Boundary

The canonical rule is: local diagnostics may be specific, while remote disclosure remains minimal. Portable control flow uses normalized PST_RESULT and typed progress records. Native detail is private and must not change TLS alerts or wire data. Functional peer information is returned only when explicitly requested; passive diagnostics and logging expose a smaller reviewed subset.

| Surface | Field/data | Source | Local/remote | Peer-controlled? | Secret? | Public? | Logged? | Redaction rule and evidence |
|---|---|---|---|---|---|---|---|---|
| PST_RESULT | normalized category | core/provider mapping | local | no | no | yes | yes | Stable enum only; deterministic failure matrices. |
| pst_result_string | generic English text | static table | local | no | no | yes | no | Constant strings; no interpolation. |
| PST_DIAGNOSTIC_INFO | size/version, valid, generation, result, coarse operation, copied backend ID | explicit projection | local | no | no | yes | n/a | Fixed 56-byte record; no native codes, phase, pointer, hostname, ALPN, certificate, credential, path, environment, or payload fields. |
| PST_LOG_EVENT | size/version, level, event/category IDs, result, operation, copied backend ID | fixed-value emitter | local callback | no | no | yes | yes | Fixed 60-byte record; no message/native/detail pointer. |
| peer-info | TLS version, cipher, flags, fingerprint, requested leaf DER | authenticated provider state | local | certificate is peer-originated | identity data | explicit query | no | Functional data, not passive diagnostics. |
| negotiated ALPN | copied length-delimited bytes | negotiated TLS state | local | jointly negotiated | no | explicit query | no | Functional result only; never copied into diagnostic/log events. |
| Phase 6/7.B trace files | timing, module paths, private native trace | test/debug instrumentation | local file | no | may contain paths/codes | no | outside public sink | Opt-in instrumentation, not stable ABI; no automatic log file. |
| TLS wire behavior | provider-required alerts/closure | NSS protocol engine | remote | n/a | no PST diagnostics | protocol only | no | No plaintext diagnostic protocol and no snapshot/callback data on wire. |

## Audit findings

PST_DIAGNOSTIC_INFO contains exactly the approved fields. pst_diagnostic_export_public validates the caller prefix, zeroes only the known record, restores size/version, assigns semantic fields explicitly, copies only the fixed backend ID, and terminates it. It never copies the richer internal structure; a larger caller tail remains untouched. Generation is a context-local modular counter, not a timestamp, nonce, global ID, or cross-object correlator.

PST_LOG_EVENT contains only reviewed numeric fields and the fixed backend ID. The emitter zero-initializes the event, uses no formatting or allocation, copies at most 31 identifier bytes, terminates the buffer, and invokes the synchronous callback. Event/category IDs are the small stable sets 1 through 8. The callback is observational and void; context lifetime and same-object reentrancy restrictions remain consumer obligations. Lifecycle tests retain late_callback=0.

Backend ID comes from the registered descriptor, not failed selector reflection or peer input. Unknown exact selection produces an empty ID. It is copied inline and never borrowed. No production logging call accepts a printf-style arbitrary message.

Levels remain OFF=0, ERROR=1, WARN=2, INFO=3, DEBUG=4, TRACE=5. A non-OFF event emits when its level is at or below the configured threshold. OFF suppresses callbacks only. No production WARN emission exists; WOULD_BLOCK, timeout, optional ALPN absence, clean close, and partial I/O are not WARN.

INFO is limited to runtime ready, secure connection, and clean shutdown. Handshake/read/write/wait/readiness progress remains TRACE; connection creation/attach transitions remain DEBUG. Terminal operations emit through the centralized one-ERROR helper, with representative counts covered by existing matrices.

Internal snapshots may retain native domains/codes, flags, and fine phases. These fields do not enter either public record or portable control flow. pst_result_string is static generic text. Diagnostics/logging do not select or enrich TLS alerts. PST sends no plaintext pre-handshake diagnostic and has no post-handshake diagnostic wire protocol.

Hostname, peer ALPN text, endpoints, certificate contents, credentials, keys, trust stores, paths, environment values, tokens, and payload are absent from diagnostics/log events even at TRACE. Wrong hostname exposes HOSTNAME_MISMATCH; trust failures expose AUTH_FAILURE; truncation exposes TRUNCATED and coarse READ context. Explicit functional ALPN/peer-certificate APIs remain available and are not over-redacted.

Historical failure-client.log, failure-backend.log, failure-modules.log and their environment switches are opt-in test/debug instrumentation. They are not PST_LOG_EVENT and may contain local native/path detail unsuitable for public or remote disclosure.

## Evidence and remaining gate

Existing evidence covers every result string, both ABI layouts, explicit projection, larger-tail preservation, generation wrap/copy, backend-ID boundaries, the complete 6x6 threshold matrix, OFF diagnostic independence, pre-runtime failure, isolation, bounded INFO, one-ERROR failures, and absence of WARN for progress. Phase 7.B covers clean/truncated closure, 7.D covers protocol/ALPN/hostname/trust/backend/pre-runtime failures, and 7.E covers callback lifetime.

Focused 7.G tests place password, token, path, hostname, ALPN, certificate-like bytes, payload, format tokens, termination boundaries, and native codes adjacent to richer inputs and prove that only approved fields cross. No production defect or behavior change was required.

## Phase 7.G2 functional equivalence

The existing real RetroZilla NSS fixture was run with identical non-logging inputs at every level. TLS 1.2 reported cipher 0xc030, WRITE=25, READ=25, CONTENT_MATCH=1, authenticated peer state, and ALPN fixture/1 at OFF, INFO, and TRACE. Event totals were respectively 0, 3, and 15; INFO emitted exactly three INFO events, while TRACE emitted three INFO, two DEBUG, and ten TRACE events. The server observed TLS 1.2, AUTH=True, ALPN fixture/1, RECV=25, SEND=25, and CONTENT_MATCH=True in every run.

TLS 1.3 reported cipher 0x1302 and the same successful I/O, authentication, and ALPN values at all three levels. Event totals were 0, 3, and 17; INFO again emitted exactly three INFO events, while TRACE emitted three INFO, two DEBUG, and twelve TRACE events. The server observed TLS 1.3, AUTH=True, ALPN fixture/1, RECV=25, SEND=25, and CONTENT_MATCH=True in every run.

The negative case was required ALPN fixture/1 against a TLS 1.3 server offering no ALPN. OFF, ERROR, and TRACE all returned POLICY_VIOLATION in terminal FAILED state with valid diagnostic result POLICY_VIOLATION, public operation CONFIGURATION, backend ID retrozilla-nss, and no resurrection. Event totals were 0, 1, and 6. ERROR and TRACE each contained exactly one ERROR; TRACE additionally contained one INFO, two DEBUG, and two safe structured progress events, with zero WARN. The server observed the same TLS 1.3/no-ALPN connection and the same local-abort Win32 10053 category in all three runs. No application payload was sent.

Logging therefore changed only local callback delivery. OFF preserved results, diagnostics, peer summary, ALPN, cipher and secure I/O. INFO remained a bounded three-event lifecycle set with no per-I/O/readiness flood. TRACE exposed only fields present in the fixed public event ABI, so it could not contain payload, hostname, ALPN text, DER, keys, trust material, native detail, paths, endpoints or handles. No raw TLS records were compared; semantic wire behavior was equivalent.

The functional gate is complete.

## Phase 7.G closure

The formal closure audit found every mandatory diagnostics/security-disclosure guarantee satisfied. Public diagnostics and log events remain fixed, structured, local, allocation-free value projections with absolute redaction. Native detail remains internal; functional peer data remains available only through explicit APIs; logging levels affect only callback delivery; INFO is bounded; TRACE carries only safe structured progress; normal progress is not WARN; and representative failures emit one logical ERROR. TLS 1.2, TLS 1.3, and required-ALPN failure runs proved semantic wire equivalence across logging levels. No production disclosure defect, ABI expansion, security-policy change, or mandatory untested surface remains.

Phase 7.G is complete. Phase 7 remains in progress; Phase 7.H is next but is not started.

API remains 1.2.0, library 0.3.0, SPI 2.3. Tests/docs-only changes require no NT4 retest.
