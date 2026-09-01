# Consumer-controlled logging and event sink design

Status: Phase 7.A7 design complete. This document defines a future logging boundary; it does not add a logger, callback, event ABI, worker, file, console output, PAL dependency, or wire-visible behavior. Phase 7.A remains in progress.

## Decision

PST should eventually offer a limited public, consumer-owned event sink backed by a compact structured event ABI. A richer internal TRACE representation may exist behind that boundary while its taxonomy is still changing. Only reviewed, backend-neutral events cross the public boundary. This is option C: limited public sink plus a richer internal TRACE model.

Preformatted text is not the primary ABI. Structured events let each consumer format, localize, route, or store observations without making PST depend on a console, filesystem, Event Log, syslog, GUI, PAL, or another Papinho project. PST creates no log history and never selects a destination.

## Cross-Papinho level convention

PST adopts the Papinho semantic level set exactly:

| PST level | PapinhoAccelerator semantic level | Events admitted by the threshold |
|---|---|---|
| OFF | OFF | none |
| ERROR | ERROR | ERROR |
| WARN | WARN | WARN and ERROR |
| INFO | INFO | INFO, WARN, and ERROR |
| DEBUG | DEBUG | DEBUG, INFO, WARN, and ERROR |
| TRACE | TRACE | TRACE, DEBUG, INFO, WARN, and ERROR |

The meanings are shared; implementation code and types are not. PST must not include Accelerator headers, link its logger, call its PAL, or claim numeric compatibility until the actual Accelerator numeric constants have been reviewed. The implementation subtask must verify those constants before freezing PST values. If numeric equality is unsuitable, PST still freezes explicit local values and preserves the semantic mapping above.

### 7.A8 numeric compatibility audit

The authoritative PapinhoAccelerator files inspected for 7.A8 are `src/runtime/log.h`, `src/runtime/log.c`, and `tests/log_test.c` in the local PapinhoAccelerator source tree. They currently define `DEBUG=0`, `INFO=1`, `WARNING=2`, `ERROR=3`, and `OFF=4`; delivery rejects OFF and otherwise emits when `event_level >= minimum_level`. No TRACE level exists.

This is not a six-level numeric convention that PST can adopt unchanged. Assigning a PST TRACE number, renumbering the existing meanings, or claiming numeric identity would invent an unverified cross-project ABI. Phase 7.A8 therefore stops before public logging constants or sink implementation. The required next decision is a separately reviewed cross-Papinho convention that adds TRACE while explicitly deciding whether the established Accelerator values remain frozen or are versioned. Semantic compatibility remains unchanged in the meantime.

One effective threshold is sufficient initially. There is no separate enable flag and no per-category threshold. OFF means that the sink receives no normal logger events. It does not suppress return values, pst_result_string, explicitly requested diagnostic snapshots, state queries, application UI, or any other functional output.

## Severity policy

- ERROR means that an intended operation failed: runtime/backend initialization failure, fatal transport failure, handshake or authentication failure, hostname mismatch, or an unexpected truncated secure stream. Emit one event at the most useful public layer rather than repeating the same failure at every layer.
- WARN means execution continues in an explicitly degraded or recoverable mode. It is intentionally rare. WOULD_BLOCK, partial I/O, ordinary timeout, an unsuccessful candidate followed by successful automatic selection, and normal closure are not warnings.
- INFO is a small operational lifecycle set: runtime ready with selected backend, secure connection established with negotiated TLS version when available, and clean connection shutdown/close. Reads, writes, polls, waits, handshake iterations, and WOULD_BLOCK never belong at INFO.
- DEBUG records major state transitions and backend-neutral decisions: backend selection/capability decisions, effective TLS policy, authentication classification, and normalized operation/result context. It must not mirror every internal branch.
- TRACE records fine-grained progress: handshake/shutdown steps, readiness interest and result, WOULD_BLOCK progression, partial byte counts, and progress-guard decisions. Byte counts are TRACE metadata; buffer contents are never events.

## Configuration and ownership

Logging policy belongs to the runtime. Connections inherit the runtime's sink and threshold; per-connection configuration is not justified initially. With no sink, effective logging is OFF. A configured sink must include an explicit level: PST must not silently choose INFO merely because a callback pointer is present. A null sink is accepted as OFF regardless of the accompanying level.

The first implementation should take an immutable, versioned logging configuration during runtime creation. Existing runtime constructors must retain their current behavior and should be wrapped by a distinctly named logging-aware constructor rather than having parameters appended. The exact exported names and record layout require a focused ABI review because the current runtime-options minimum-size rules must remain compatible.

The configuration conceptually contains a level, callback, and caller-owned context. The consumer owns the callback and context and must keep both valid until runtime release returns. Initial immutability avoids a new synchronization promise. A later controlled runtime-level setter may enable GUI-driven INFO/DEBUG/TRACE changes without recreating connections, but only after atomicity, callback replacement, in-flight callback, and context-lifetime rules are designed and tested.

An application's CLI, GUI, or configuration file is only a source:

    application CLI / GUI / config file
                 -> application configuration model
                 -> effective PST runtime sink and level

PST itself has no CLI or GUI.

## Callback contract

The signature direction is a C89-compatible void callback conceptually equivalent to:

    void PST_CALL sink(void *user_context, const PST_LOG_EVENT *event);

Names are not frozen by this design. Delivery is synchronous on the application thread currently executing the PST call. There is no worker thread, queue, implicit lock, or asynchronous lifetime. The event pointer is valid only for the callback. A sink may block, but doing so blocks the PST operation and is discouraged.

The callback has no status result. Ignoring an event or malfunctioning in consumer code cannot change a PST_RESULT, TLS state, ownership transition, or fail-closed decision. Language exceptions or non-local exits across the C ABI are outside the contract and are consumer bugs.

The callback must not reenter, mutate, or release the same runtime or connection that caused the event. It may use an unrelated PST object only within that object's existing one-thread-at-a-time and lifetime rules. Such a call can synchronously produce a nested callback, so the consumer remains responsible for recursion in its sink. PST does not add locks to promise arbitrary reentrancy.

Runtime release produces no callback after it returns. The initial contract should avoid events during destruction where their ownership or source association would be ambiguous.

## Event boundary

The public event is a fixed, caller-copyable value with no borrowed data. The implementation subtask should evaluate this compact prefix:

- struct_size and api_version;
- level;
- compact stable event ID;
- coarse category;
- coarse source kind (RUNTIME or CONNECTION), without an object pointer;
- normalized PST_RESULT and public PST_DIAGNOSTIC_OPERATION when relevant;
- copied fixed-capacity backend ID;
- validity flags for optional, non-secret numeric metadata;
- negotiated TLS version, readiness interest/result, timeout/progress indication, or byte count only where the event ID defines their meaning;
- an inline copy of the redacted public diagnostic subset for failure events, if the ABI review finds that preferable to duplicating its individual fields.

The exact structure, offsets, constants, optional-field mask, and callback/config names are not frozen in 7.A7. Public records must use explicit PST integer/size types, explicit numeric constants, struct_size, api_version, same-major known-prefix writes, append-only growth, VC6/C89 layout tests, and caller-owned context. Consumers retaining an event copy only the supported known prefix and must not assume an unknown tail.

No runtime or connection pointer is exposed as association metadata. Source kind, category, operation, and synchronous call context provide safe coarse association initially. If multi-connection correlation later proves insufficient, add a separately reviewed opaque numeric correlation token supplied by the consumer; do not expose addresses, native handles, global sequence IDs, SOCKET, or NSS objects.

Internal events may carry finer phase and progress detail required to diagnose issues such as NT4 readiness spinning. The public sink receives only reviewed projections. Internal phases, NSS branches, native handles, native error domains/codes, backend state, and arbitrary messages do not become public ABI merely because TRACE is enabled.

## Event IDs and categories

Public IDs should remain compact and semantic rather than enumerate implementation calls. A candidate first taxonomy for the ABI review is:

- runtime ready;
- connection secure;
- connection closed cleanly;
- operation failed;
- degraded/recoverable condition;
- backend or policy decision;
- state transition;
- operation progress step;
- readiness/wait result;
- I/O progress.

The implementation review may merge IDs, but must not create an ID for every PR_Read, PR_Poll, NSS error, or state-machine edge. ERROR deduplication is part of each emission site's contract.

Coarse categories are RUNTIME, BACKEND, CONNECTION, TLS, AUTHENTICATION, IO, READINESS, and SHUTDOWN. They support routing and display; they do not imply separate thresholds and are never NSS-specific.

## Diagnostic integration

Logging is presentation, not diagnostic storage:

    PST_RESULT / progress
            -> structured diagnostic snapshot
            -> optional event projection

PST_DIAGNOSTIC_INFO remains queryable and unchanged with logging OFF or no sink. For ERROR and diagnostic-bearing DEBUG events, the event should carry an inline redacted snapshot or an equivalent value copy captured at emission time. Requiring the callback to query the connection would create staleness and forbidden same-object reentrancy. The event must project from the existing diagnostic source rather than independently recapture native state.

The public diagnostic layout, operation values, constructor semantics, and generation rules remain unchanged. Native error domains/codes and detailed internal phases remain private at TRACE. Backend implementation version remains authoritative provider metadata for a future structural query, not a log-only fact. The backend ID remains retrozilla-nss; logging design does not reopen backend naming.

## Security and privacy boundary

Redaction is absolute at every level, including TRACE. An event can never contain passwords, private keys or key bytes, OAuth tokens, credential material, certificate private material, certificate DER, trust-store contents, environment secrets, application/protocol payload, email or HTTP bodies, native pointers/handles, native error text/codes, filesystem or DLL paths, or arbitrary backend strings.

Foundational events contain no hostname, peer-controlled text, subject names, ALPN bytes, or arbitrary display string. Those require a later field-specific privacy, escaping, control-character, and log-injection review. Complete certificate DER is never logging data.

PST does not add endpoint fields for logging. Remote/local address logging remains the consumer or transport adapter's responsibility unless a future transport-neutral metadata API is designed. Generic structures never acquire SOCKET or sockaddr for observability.

The sink affects no TLS alert, packet, trust decision, credential handling, hostname verification, readiness behavior, secure I/O, shutdown, or remote disclosure.

## Performance and failure behavior

The hot path first checks for a non-null sink and whether the level passes the cumulative threshold. Filtered events require no formatting, allocation, diagnostic conversion, or expensive metadata lookup. Enabled basic events are fixed stack/value records; there is no dynamic message formatting requirement.

Reporting OOM must not allocate. Event delivery cannot recurse into allocation-based error reporting, and sink behavior never weakens fail-closed handling. PST keeps no history, ring buffer, file, or GUI list; retained events are consumer storage.

## Implementation test plan

The implementation gate must include deterministic VC6 /W4 tests for:

- OFF and null-sink delivery of zero events, while diagnostics and functional results remain available;
- exact cumulative filtering at ERROR, WARN, INFO, DEBUG, and TRACE;
- explicit-level validation and the absence of a separate enabled boolean;
- callback context, synchronous calling thread, pointer lifetime, safe known-prefix copying, and no history;
- runtime and connection source association across multiple connections;
- documented same-object reentrancy rejection/undefined-use boundary and unrelated-object nested delivery;
- void callback behavior that cannot alter a mocked failure or successful TLS operation;
- no allocation for a basic event and an OOM event path that does not allocate;
- one meaningful ERROR per failure and no INFO flood from read/write/poll loops;
- WOULD_BLOCK as TRACE only, ordinary timeout as TRACE (not WARN), and partial byte counts as TRACE;
- runtime-ready, secure-connection, negotiated-version, and clean-close INFO policy;
- redaction scans proving no secrets, payload bytes, native error codes/text, paths, hostname/peer text, certificate/DER, native handles, or endpoint types cross the event ABI;
- diagnostic availability and generation semantics unchanged at every logging level;
- no worker thread, asynchronous queue, automatic file, console output, PAL, or Accelerator linkage;
- host TLS 1.2/TLS 1.3 regression and NT4 compatibility after actual code is introduced.

Before numeric levels or event IDs are frozen, the review must verify PapinhoAccelerator's actual level constants. Before a public record is frozen, tests must assert VC6 size, offsets, version negotiation, unknown-tail preservation, and append-only compatibility.

## Scope disposition and next step

Phase 7 and 7.A remain in progress. Phase 7.A7 changes documentation only. It does not change API 1.1.0, library 0.2.0, SPI 2.3, PST_DIAGNOSTIC_INFO, TLS, trust, credentials, hostname validation, readiness, wire behavior, or remote disclosure. It starts neither 7.B nor Phase 8/9.

The exact next step within 7.A is to resolve the six-level numeric convention identified by the 7.A8 audit. Only after that decision may 7.A8 freeze the minimal level/config/callback/event prefix and compact IDs, resolve the logging-aware runtime-constructor compatibility shape, and prove VC6 layout before adding emission sites.
