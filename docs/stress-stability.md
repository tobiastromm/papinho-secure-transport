# Stress and long-run stability

Status: Phase 7.H1 audit and bounded soak plan complete. Phase 7.H remains in progress; implementation and execution are next. Phase 8 is not started.

## Scope and existing evidence

Phase 7.H repeats already-correct behavior; it adds no feature, public stress API, production hook, throughput target, unsupported threading, or unbounded endurance run. Existing evidence is reused as baseline: three complete TLS 1.3 lifecycles in one process on Windows 10 and NT4; runtime recreation; NSS A/B/C active-global rejection and recovery; exact lifecycle/ownership counters; clean/truncated failure classification; anti-spin readiness matrices; real TLS 1.2/1.3 secure echo; and OFF/INFO/TRACE logging equivalence.

Those proofs establish correctness but not sustained volume. Remaining gaps are many sequential real connections on one runtime, many deterministic messages on one connection, repeated process-global runtime recreation, repeated clean/truncated/policy outcomes, mixed recovery, batch resource trends, and event-count stability.

## Mandatory closure matrix

| Gate | Count | Runtime pattern | Primary invariants | Bound |
|---|---:|---|---|---|
| TLS 1.2 sequential soak | 100 connections | one runtime/config | version 0x0303, authenticated fixture/1, exact echo, clean release | 10 s/client, 20 min overall |
| TLS 1.3 sequential soak | 100 connections | one runtime/config | version 0x0304, authenticated fixture/1, exact echo, clean release | 10 s/client, 20 min overall |
| Single-connection I/O soak | 1000 exchanges of 64 bytes | one TLS 1.3 connection | sequence payload, no loss/duplication/corruption, exact 64000 bytes each direction | 10 s progress, 10 min overall |
| Runtime lifecycle soak | 50 cycles | create runtime, one TLS 1.3 echo, release | initialize/shutdown and create/destroy balance; final recreation succeeds | 10 s/client, 15 min overall |
| NSS A/B/C pattern | 20 cycles | A success, B active rejected, release A, C success/release | deterministic UNSUPPORTED for B, no global poisoning | deterministic runner, 2 min overall |
| Required-ALPN failure soak | 100 connections | reusable runtime/config where valid | POLICY_VIOLATION, FAILED, CONFIGURATION diagnostic, no resurrection, later success | 10 s/client, 20 min overall |
| Abrupt truncation soak | 50 connections | one runtime/config | FAILED/TRUNCATED/READ; safe release; no stale close_notify | 10 s/client, 10 min overall |
| Clean-close soak | 50 connections | one runtime/config | CLOSED/CLEAN; no stale truncation | 10 s/client, 10 min overall |
| Mixed sequence | 10 repetitions (80 connections) | one runtime where permitted | success, success, policy failure, success, truncation, success, clean close, success | 10 s/client, 15 min overall |
| Logging OFF soak | TLS 1.3 100-connection run | same positive soak | zero events; identical network results | same positive bound |
| INFO soak | 50 successful TLS 1.3 connections | one runtime | exactly 3 INFO/session, 150 total; no DEBUG/TRACE | 10 min overall |
| TRACE mini-soak | 20 connections plus 200 I/O exchanges | one runtime/connection as applicable | bounded coherent counts, no ERROR/WARN, no corruption or secret fields | 10 min overall |
| Counter-balance mock soak | 500 mixed lifecycle cycles | mock backend | initialize=shutdown, runtime create=destroy, connection create=destroy, ownership accept=provider close | 2 min overall |
| Final regression chain | TLS 1.2 success, TLS 1.3 success, truncation, final TLS 1.3 success | fresh final state | canonical 25/25 matches, correct truncation, reusable provider | 5 min overall |

Counts are retained because operations are local loopback and bounded. They are sufficient to expose per-cycle drift without becoming an endurance lab. TRACE is smaller because event volume is higher. Partial-I/O stability uses exact mocks plus real I/O soak; no network shaper is justified.

## Focused runner and server

Prefer one VC6 C89 executable, tests/test_stress_stability.c, with bounded modes/defaults. Reuse public PST setup from test_tls_runtime_integration and test_nt4_lifecycle_integration. Keep credentials/trust/config alive where one runtime is required, fail fast, and print a machine-readable summary. Add no production hooks.

Extend one existing Python fixture with an explicit multi-client mode. It accepts an exact client count, uses a 120-second initial accept window, a 10-second per-client timeout, and a finite overall deadline. It prints clients expected/accepted/passed, total received/sent, content matches, clean/abrupt outcomes, and PASS=1, then exits. Post-accept bounds remain finite. The I/O mode echoes 64-byte records containing an ASCII prefix, zero-padded sequence, and deterministic fill; both sides validate each sequence.

Failure output identifies iteration, mode, stage, PST_RESULT, operation, diagnostic result/operation/backend, step counts, and elapsed milliseconds. It never dumps secrets, DER, or arbitrary payload beyond the public non-secret sequence marker.

## Resource and progress methodology

Exact counters are primary. After cleanup require initialize=shutdown, runtime_create=runtime_destroy, connection_create=connection_destroy, ownership_accept=provider_transport_close, caller close only before unaccepted transfer, no live object, and no late callback. Diagnostics remain value snapshots: later success has no stale failure and generations are compared only within one object.

The real runner samples its own Windows process handle count and private-bytes/working-set at baseline, after batches 1-25, 26-50, 51-75, 76-100, and after cleanup. Handle count after cleanup must return to baseline plus a documented fixed instrumentation allowance and must not grow monotonically across settled batches. Memory is supporting evidence: working-set caching alone is not failure; repeated private-byte growth must first be reduced. Python/server resources are excluded.

Each connection records maximum handshake, read, write, shutdown, and wait steps. Existing 200-step ceilings remain hard bounds. A cycle reaching 75 percent of a ceiling or rapidly consuming no-progress budget preserves trace and fails pending diagnosis. Batch elapsed times identify gross monotonic slowdown, not an SLA.

Logging uses an in-memory counter sink. OFF requires zero callbacks. INFO scales exactly with completed normal lifecycles and excludes I/O/readiness. TRACE may vary with readiness timing but stays bounded by actual step totals; successful traffic permits no ERROR/WARN. No trace file is enabled unless reducing a failure.

## Recovery, global state, and final state

Every failure batch is followed by valid TLS. Fifty truncations are followed by clean close; 100 policy failures by authenticated echo. A/B/C and 50 runtime cycles end with an extra runtime create/release. This proves NSS initialization, active state, close_notify observation, credentials/trust references, and diagnostics do not contaminate later operations.

Every runner ends with all objects released, backend shutdown, no expected socket, and no pending server accept. TLS session resumption may be observed but is neither required nor disabled; no session-cache API is added. Entropy/RNG, certificate policy, readiness, ownership, and close classification remain unchanged unless a reduced defect proves otherwise.

## Optional and excluded gates

An NT4 mini-soak of 10 sequential TLS 1.3 connections is optional if the existing package supports it without new infrastructure. It is not a blocker when production is unchanged. Interleaved connections are optional. Excluded: multithreaded same-connection access, worker pools, locks, Schannel long soak, packet chaos, bandwidth benchmarking, multi-day runs, and Phase 8 work.

Outputs belong under build/phase7h. Exact RetroZilla NSS source/provenance preservation needed to make C:\PSTW disposable remains separate housekeeping.

## Versions and closure path

API remains 1.2.0, library 0.3.0, SPI 2.3. Implementation adds only tests, fixture support, Makefile target, and docs unless a deterministic defect appears. Next: implement runner/server mode, execute matrix with versioned NSS runtime, run official clean VC6 /W4 regressions with zero warnings, then request 7.H closure audit. Phase 8 must not start automatically.
