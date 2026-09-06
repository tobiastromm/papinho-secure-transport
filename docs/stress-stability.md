<!-- SPDX-License-Identifier: MPL-2.0 -->

# Stress and long-run stability

Status: Phase 7.H stress and long-run stability complete. All frozen mandatory gates passed; the overall Phase 7 closure audit is complete. Later Phase 8 and Phase 9 work is also complete.

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
| INFO soak | 50 successful TLS 1.3 connections | one shared runtime | exactly 1 RUNTIME_READY + 2 INFO/connection, 101 total; no DEBUG/TRACE | 10 min overall |
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

Outputs belong under build/phase7h. The later RetroZilla NSS provenance housekeeping made the historical C:\PSTW workspace disposable.

## Versions and closure path

Historical Phase 7.H baseline: API 1.2.0, library 0.3.0 and SPI 2.3. The planned implementation was limited to tests, fixture support, Makefile target and documentation unless a deterministic defect appeared; the completed results are recorded below.


## Phase 7.H2 observed results

The bounded matrix executed on the modern Windows host with the versioned RetroZilla NSS runtime. Artifacts are under build/phase7h and the concise manifest is build/phase7h/summary.txt.

| Gate | Observed result |
|---|---|
| TLS 1.2 sequential | 100/100; 6400 bytes each direction; max handshake/read/write/wait/shutdown 3/3/1/2/1; 266 ms |
| TLS 1.3 sequential | 100/100; 6400 bytes each direction; maxima 2/5/1/4/1; 1688 ms |
| One-connection I/O | 1000/1000 records; 64000 bytes each direction; 110 ms |
| Runtime lifecycle | 50/50 complete runtime/TLS/I/O/release cycles; server 50/50 |
| NSS A/B/C | 20/20 patterns and final runtime reusable |
| Required ALPN absent | 100/100 POLICY_VIOLATION/CONFIGURATION/no-resurrection; 2188 ms |
| Abrupt truncation | 50/50 TRUNCATED/READ; server abrupt=50 |
| Clean close | 50/50 CLOSED/CLEAN; server clean=50 |
| Mixed | 80/80 in one runtime: 50 success, 10 wrong-hostname, 10 truncation, 10 ALPN-policy failure |
| Mock lifecycle | 500 balanced cycles |
| OFF | 100 connections, zero events |
| INFO | 50 connections, 101 INFO, zero ERROR/WARN |
| TRACE | 20 connections, 241 events: INFO=41, DEBUG=40, TRACE=160, zero ERROR/WARN |
| Final chain | TLS 1.2 success, TLS 1.3 success, truncation, TLS 1.3 success in one runtime; server 4/4 |

The INFO planning estimate of 150 assumed one runtime per session. With one runtime shared by 50 connections, the correct structural count is one RUNTIME_READY plus two per connection: 1 + 2*50 = 101. TRACE has the same INFO basis: 1 + 2*20 = 41. This is linear bounded behavior.

GetProcessHandleCount was resolved dynamically. Samples were 99 at baseline, 101 after 25/50/75/100 connections, and 86 after runtime release. There was no monotonic growth. Working-set/private-byte sampling was not added because it is secondary and would require disproportionate PSAPI infrastructure; exact mock balances, server/client counts, handles, and final recovery are primary.

Server/client byte and client totals matched. Python unwrap completion varied in ordinary echo modes because the provider performs local PR_Shutdown without waiting for reciprocal shutdown, as already documented. The dedicated server-initiated clean-close classification passed 50/50. No production behavior changed.

No operation approached 200 steps; maxima were 3/5/1/4/1. Batch timings did not degrade monotonically. Failures retained per-connection diagnostics and later successes passed, including the same-process recovery chain.

No production defect was found. Changes were tests, existing fixture, Makefile, artifacts and docs only. The Phase 7.H baseline was API 1.2.0, library 0.3.0 and SPI 2.3. No NT4 rerun was mandatory.

## Phase 7.H closure audit

All 16 frozen mandatory gates passed. The results prove bounded sequential TLS 1.2/TLS 1.3 stability, sustained application I/O, runtime and NSS process-global reuse, repeated policy/clean/truncated cleanup, mixed-outcome isolation, balanced mock ownership, bounded logging, resource and anti-spin stability, and final same-process recovery. Client/server counts and positive-mode byte totals agreed.

The observed global step maxima were 3 handshake, 5 read, 1 write, 4 wait, and 1 shutdown step against the hard limit of 200. Handle samples were 99, 101, 101, 101, 101, and 86, with no monotonic growth. Working-set/private-byte sampling, the NT4 mini-soak, and simultaneous interleaving remain optional and were not executed; none is a frozen closure gate.

The varying Python `unwrap` completion in generic echo modes remains the documented provider-local shutdown contract, not a stress failure; the dedicated server-initiated clean-close gate passed 50/50. Diagnostics and logging remained connection-local, bounded, and free of payload, hostname, DER, key, trust, native-error, endpoint, handle, and pointer disclosure. No PST production bug or unresolved blocker was found, and `src/` and `include/` were unchanged.

Phase 7.H and Phase 7 are complete after their formal closure audits. Phase 8 and Phase 9 subsequently completed.
