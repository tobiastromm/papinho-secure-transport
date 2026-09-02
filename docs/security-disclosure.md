# Diagnostics and security disclosure matrix

Status: Phase 7.G audit complete; Phase 7.G remains in progress pending focused functional logging/wire-equivalence revalidation and closure audit.

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

Before closure-audit recommendation, run existing TLS 1.2 and 1.3 integrations at OFF, INFO, TRACE plus one negative policy fixture at OFF/ERROR/TRACE, confirming identical results and server-observed behavior. No new runner or wire semantics is needed.

API remains 1.2.0, library 0.3.0, SPI 2.3. Tests/docs-only changes require no NT4 retest.
