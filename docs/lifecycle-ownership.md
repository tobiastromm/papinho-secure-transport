<!-- SPDX-License-Identifier: MPL-2.0 -->

# Lifecycle and Ownership

> The published 0.4.0 evidence below remains historical. The API 2.0 CLIENT/SERVER development lifecycle is frozen in [client-server-lifecycle.md](client-server-lifecycle.md): selection is per connection, listeners remain consumer-owned, and exactly-one-close after transport acceptance is unchanged.

## Status

Phase 7.E Lifecycle / Ownership Hardening is complete. The focused deterministic matrix, real NSS lifecycle revalidation, and formal closure audit satisfy every mandatory 7.E contract. No public API, ABI, SPI, or production behavior changed.

## Object graph

```text
consumer
  |-- owns runtime ---------------------> backend state + backend runtime state
  |      |-- borrows logging sink/context for the runtime lifetime
  |      `-- owns zero or more connections
  |             |-- retains frozen config
  |             |-- owns backend connection state
  |             `-- owns accepted transport wrapper
  |                    `-- provider owns imported descriptor after acceptance
  |-- owns config
  |      |-- retains credentials
  |      |-- retains trust
  |      |-- owns copied hostname
  |      `-- owns copied ALPN wire list and embedded TLS policy values
  |-- owns credentials (copied certificate and private-key DER)
  |-- owns trust (copied custom-CA DER or system-trust tag)
  |-- owns transport until ownership_accepted
  |-- owns peer_info snapshot returned after establishment
  `-- owns value copies of diagnostics and log events
```

There are no worker threads, timers, asynchronous queues, or callbacks retained beyond the runtime. Public handles are invalid after their release function returns; double release of a stale opaque pointer is outside the contract.

## M1 wait-set membership lifetime

A wait-set borrows registered connections and never owns or releases them. Each
connection stores only its private membership link and may be registered in at most one
wait-set. The consumer must remove a connection before releasing it. The result-returning
`pst_connection_try_release` rejects release while registered and leaves the connection
and accepted transport intact; the compatibility `pst_connection_release` likewise does
not free or close a registered connection.

Destroying a nonempty wait-set is rejected and leaves both the set and its members valid.
Removal changes neither connection ownership nor transport ownership, performs no close,
and permits later registration or release. Allocation or duplicate-registration failure
does not alter existing membership. M3 adds one private platform wake object per
wait-set. It owns no member transport and is released exactly once when an empty,
inactive wait-set is destroyed. No worker thread is created.

## M2 borrowed external-source lifetime

An external source and its native resource remain consumer-owned. PST borrows the
wrapper while registered and never closes, shuts down, accepts from or otherwise assumes
ownership of the native resource. `pst_external_source_try_release()` rejects a
registered wrapper; the consumer removes it first, then releases the wrapper and closes
the native resource on its own schedule.

The same wrapper can be registered in only one wait-set at a time. Successful removal
clears that membership. Distinct wrappers around the same native resource are distinct
to the portable core and require consumer-side coordination.

For M3, one wait may be active at a time. Add/remove remain owner-thread operations;
wake may run concurrently and does not cancel, remove, release or close a member.
Destroy during an active wait is rejected, as is destroy with registered members.
Timeout and wake leave all connection and external-source ownership unchanged.

## Ownership table

| Object or edge | Rule | Failure and release rule |
|---|---|---|
| runtime -> backend state | owned | backend shutdown exactly once after backend runtime destruction |
| runtime -> backend runtime state | owned | destroyed before backend shutdown |
| runtime -> log callback/context | borrowed immutable pair | consumer keeps it valid through final runtime release; delivery is synchronous and stops when release returns |
| connection -> runtime | borrowed parent with enforced child count | consumer must release every connection before the final runtime release |
| connection -> config | retained | caller may release its config handle after successful connection creation |
| config -> credentials/trust | retained | caller may release original handles after successful setter |
| config -> hostname | copied | caller buffer may change or disappear after setter success |
| config -> TLS scalar policy | embedded | rejected setter leaves prior values intact |
| config -> ALPN | copied ordered wire representation | rejected setter is transactional |
| connection -> backend connection state | owned | destroyed once from every reachable connection state |
| caller -> transport before acceptance | caller-owned | caller releases/closes after any result with `ownership_accepted == 0` |
| connection/provider -> transport after acceptance | transferred | caller invalidates its former handle; connection cleanup consumes wrapper without a second native close |
| peer_info -> summary/leaf DER | copied snapshot | survives connection and runtime destruction until peer snapshot release |
| public diagnostic | inline value snapshot | survives source reset/destruction; backend ID is inline copied |
| public log event | callback-duration pointer to a value record | consumer may copy the known public record during callback |

## Runtime and connection lifetime

The runtime is the parent of every connection. The supported order is connection release followed by runtime release. `pst_runtime_release` does not destroy a runtime while its internal child count is nonzero; this guard is not an ownership transfer or automatic deferred-release facility. A consumer that calls it early must still perform a final release after all children are gone. Connections do not expose an independent public runtime reference.

Multiple connections on one mock runtime already prove isolated per-connection progress and diagnostics. Closing one does not destroy shared runtime state. Multiple NSS backend states are intentionally unavailable concurrently because NSS/NSPR initialization is process-global; a later runtime is valid after the active runtime is fully released.

## Transport and descriptor transition

The Win32 adapter owns the native `SOCKET` before acceptance. `FIONBIO` occurs while the caller remains owner. Successful `PR_ImportTCPSocket` is the irreversible transition and immediately sets `ownership_accepted = 1`. From that point NSPR owns the socket.

`SSL_ImportFD(NULL, tcp_fd)` layers SSL over the same descriptor chain. On success the SSL descriptor is the single aggregate close root. A failure after NSPR import closes the current NSPR or SSL aggregate exactly once. A connection that retains an accepted transport wrapper later destroys the wrapper with `consumed = 1`, so the adapter frees itself without calling `closesocket` again. Before acceptance, the backend does not close and the caller releases the adapter, which calls `closesocket` once.

| Failure stage | Owner after return | Cleanup |
|---|---|---|
| validation or `FIONBIO` | caller/adapter | caller releases transport; adapter closes socket |
| `PR_ImportTCPSocket` fails | caller/adapter | no accepted descriptor; caller closes |
| after successful NSPR import | provider | provider closes `PRFileDesc`; caller invalidates handle |
| after successful SSL layering | provider through SSL aggregate | provider closes SSL aggregate |
| handshake/read/write/policy/truncation | connection/provider | connection release closes aggregate once |
| shutdown or release without shutdown | connection/provider | shutdown is bounded; destructor performs local close without network wait |

No NSS or NSPR descriptor appears in the public ownership graph.

## Constructor transactionality

Public constructors initialize required output handles to NULL. Runtime creation copies failure diagnostics into an operation context before destroying failed backend state. Candidate backend/runtime state is destroyed before selection continues. Connection creation releases partial backend connection state if identity/TLS configuration fails and retains config only after all creation/configuration steps succeed.

Credential and trust constructors copy caller DER. Config setters allocate/copy before replacing current state. Peer snapshot creation copies its leaf DER. Win32 transport creation publishes only a fully initialized adapter. No partial object escapes on the audited failure paths.

## Memory and snapshot rules

Credential certificate and PKCS#8 buffers are independently copied. Private-key memory is wiped through a volatile byte loop before free on normal destruction and on a partial constructor failure after a key copy exists. This is not a general secure allocator.

Custom trust DER, hostname, configured ALPN, peer leaf DER, diagnostic backend ID, and public log events are owned copies. Negotiated ALPN is copied into a caller-provided buffer; no NSS pointer escapes. Credentials and trust are reference-counted internally so a frozen config remains valid after caller handle release.

## Boundedness and callbacks

Connection destruction invokes provider destruction directly and performs no handshake retry, peer wait, or graceful-close loop. NSS connection destruction closes only the local SSL aggregate and frees local certificate/key/trust/hostname/ALPN state. Runtime destruction frees runtime state, shuts NSS/NSPR down, unloads modules, and returns without network I/O.

Logging delivery is synchronous. The event pointer is valid only during the callback; the consumer owns the context and must retain it through final runtime release. Same-object release/reentrancy from the callback is outside the contract. Destructors emit no new event, and no callback can occur after final runtime teardown because PST has no asynchronous executor.

## Existing proof

- `test_identity`: credential/trust/config reference and copy lifetime; peer DER snapshot independence.
- `test_backend_spi`: lifecycle dispatch counters, accepted transport cleanup, multiple connections, state isolation, diagnostic snapshot retention, handshake/readiness/shutdown/failure paths.
- `test_diagnostic_creation`: constructor failure diagnostics survive failed provider state destruction.
- `test_logging`: immutable sink inheritance, multiple runtimes/connections, inline event copy and context isolation.
- `test_backend_nss`: backend/runtime/connection allocation and NSS lifecycle unit paths.
- Phase 5/6 real fixtures: TLS 1.2/1.3, mTLS, ALPN, I/O and shutdown; Phase 6 includes three complete NT4 cycles and peer snapshot lifetime.
- Phase 7.B/7.C/7.D: clean/truncated failure release, shutdown boundedness, readiness isolation and terminal policy failures.

## Deterministic gaps closed

1. Add a single lifecycle mock with exact initialize/shutdown, runtime create/destroy, connection create/destroy, and transport close counters.
2. Release connections from CREATED, ATTACHED, HANDSHAKING, ESTABLISHED, SHUTTING, CLOSED, and FAILED and assert exactly-once deltas.
3. Exercise attach failure before acceptance and after acceptance end-to-end through the public core, proving caller-close versus provider-close ownership.
4. Inject connection configuration failure after backend connection creation and prove diagnostic-copy-before-destroy plus exactly-once cleanup.
5. Prove the documented runtime-parent order, including an early runtime-release guard followed by connection release and final runtime release.
6. Prove multiple connections do not destroy their shared runtime and that sink context is untouched after final runtime release.
7. Prove the NSS process-global second-runtime rejection and successful re-creation after the first runtime is destroyed.

These are bounded semantic tests, not long-run stress. Large cycle counts and process-memory observation belong to Phase 7.H. Real NSS revalidation follows deterministic closure; a new NT4 run is required only if production ownership/lifecycle code changes.

## Phase 7.E2 deterministic counter matrix

`test_lifecycle_ownership` is part of the regular VC6 `/W4` suite. Every scenario finishes with exact deltas; `RT+C/RT-D` and `Conn+C/Conn-D` mean provider runtime and connection create/destroy callbacks.

| Scenario | Init | Shut | RT+C | RT-D | Conn+C | Conn-D | Attach | Accept | Provider close | Caller close |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| CREATED release | 1 | 1 | 1 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| ATTACHED release | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 0 |
| HANDSHAKING release | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 0 |
| ESTABLISHED direct release | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 0 |
| SHUTTING release | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 0 |
| CLOSED release | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 0 |
| FAILED release | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 0 |
| pre-accept attach failure | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 0 | 0 | 1 |
| post-accept attach failure | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 0 |
| early runtime release, final totals | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 0 |
| two connections, mixed outcomes | 1 | 1 | 1 | 1 | 2 | 2 | 2 | 2 | 2 | 0 |

The early runtime release has intermediate `RT-D=0` and `Shut=0`; after child release, the required final runtime release produces exactly one of each. Connection-configuration failure performs two total diagnostic copies in its complete scenario (successful runtime snapshot, then failing live connection snapshot); the second copy has a lower sequence number than the sole connection destroy. Backend initialize, provider runtime-create, and provider connection-create failures leave public outputs NULL, perform their layer-owned temporary cleanup once, preserve diagnostics, and allow a later successful creation.

Release itself never increments handshake, wait, read, write, or shutdown-step counters. No destructor emits a log event. Two connections inherit the same synchronous sink; releasing A leaves B usable, final runtime release produces no callback, and the consumer context is never owned by PST. The focused lifecycle test mutates caller-owned hostname, ALPN, certificate, private-key, and CA buffers and confirms the retained copies. Existing identity tests retain the peer DER snapshot proof, and the credentials cleanup path structurally retains the volatile private-key wipe proof.

The directed NSS lifecycle test passed with the canonical versioned runtime: runtime A succeeded, runtime B was rejected while A held the process-global NSS/NSPR state, A was released, and runtime C then succeeded. Real TLS 1.2 and TLS 1.3 each authenticated mTLS/`fixture/1` and completed `WRITE=25 READ=25 CONTENT_MATCH=1`. A TLS 1.3-only client against TLS 1.2-only server failed terminally with `PROTOCOL_FAILURE`, a valid HANDSHAKE diagnostic, one ERROR, and `NO_RESURRECTION=1`. The historical 0.4.0 lifecycle runner completed three TLS 1.3 cycles, each with 25-byte echo, one-step shutdown, and `SNAPSHOT_AFTER_DESTROY=1`.

For API 2.0/SPI 3.0 development, RetroZilla NSS CLIENT graceful shutdown is
incremental: local `close_notify` emission does not imply COMPLETE. The
connection retains its single owned SSL/NSPR transport while waiting for the
reciprocal TLS alert; raw EOF remains TRUNCATED, and final destruction still
closes the aggregate exactly once.

All seven deterministic gaps are closed. No production defect was reproduced and no production source changed. The formal closure audit found no mandatory untested lifecycle contract. At that closure point, Phase 7.E was complete while Phase 7 remained in progress and 7.F was next. No new NT4 run was required.
## Deferred housekeeping

The later RetroZilla NSS provenance housekeeping preserved the exact source and build record; `C:\PSTW` is no longer required.
