# Provisional runtime and API design

Phase 4 implements only the identity subset in [credentials-trust-peer.md](credentials-trust-peer.md). Runtime selection, connection orchestration, and the remaining surface stay deferred to Phase 5.

Status: Phase 0.C design specification with the Phase 1 portable foundation materialized. The transport/runtime surface remains provisional; only the foundation declarations in `include/papinho_secure_transport.h` are currently implemented and tested.

## Design constraints and naming

The public C namespace uses lowercase `pst_` for functions, portable scalar types, and opaque handles. Public ABI records and result types use uppercase `PST_`; constants and macros also use uppercase `PST_`. Public identifiers must not expose backend or platform types.

The interface targets C89 and Visual C++ 6.0. It does not require C99, C++, `stdint.h`, VLAs, atomics, or modern threading. Fixed-width values use PST-owned `pst_u8`, `pst_u16`, `pst_u32`, and `pst_i32` compatibility typedefs selected through `<limits.h>`, with compile-time width checks. `pst_size` aliases `size_t`.

All extensible input structures begin conceptually with `struct_size` and `api_version`. Callers zero unknown/reserved fields. Output structures use caller-provided size or opaque accessors so older binaries do not require recompilation when fields are added.

## Public object model

The minimum public object set is:

1. `pst_runtime`: owns one selected backend instance and its lifecycle.
2. `pst_config`: describes generic secure-transport policy.
3. `pst_credentials`: immutable local identity/key material prepared for a backend.
4. `pst_trust`: immutable peer-trust configuration prepared for a backend.
5. `pst_connection`: one secure connection and its attached transport.
6. `pst_peer_info`: immutable snapshot of authenticated peer/negotiation information.

There is no separate public library-global object. Process-wide coordination required by a backend is hidden behind runtime creation/destruction. There is no public certificate object model in the initial design.

### Runtime

- Created explicitly from runtime options and backend selection criteria.
- Creation initializes the selected backend or fails without returning a usable object.
- Owns backend-global/per-runtime state and exposes backend identity and capabilities.
- Multiple runtimes are allowed only when supported safely by the selected backend; otherwise creation returns a normalized unsupported/state error.
- Destroyed only after all child objects/connections created from it are released.
- Not mutated after creation except for internal synchronization/bookkeeping.

### Configuration

- Created for a runtime, populated before use, then frozen explicitly or implicitly on first connection creation.
- Holds generic requirements, never a PapinhoAccelerator or PapinhoBrowser profile name.
- A frozen configuration is immutable and shareable when the runtime/backend supports the documented threading rule.
- The caller owns its reference. Connections retain the immutable configuration data they require.

### Credentials and trust

- Created/imported through generic source descriptors or future platform-specific helper APIs.
- Become immutable after successful preparation.
- May be referenced by multiple frozen configurations/connections in the same runtime when supported.
- Secrets remain backend-owned/protected after import; APIs must not casually export private key bytes.
- Destruction releases backend resources only after dependent references are gone.

### Connection

- Created from a runtime, frozen configuration, and transport attachment.
- Exclusively owns its secure protocol state and, according to the selected attachment mode, may own the underlying transport.
- Mutable and not safe for simultaneous calls from multiple application threads in the initial contract.
- Repeated operations are incremental and nonblocking unless a later explicit convenience layer says otherwise.
- Destroy frees local state but does not promise a graceful TLS shutdown.

### Peer information

- Created as an immutable snapshot only after sufficient handshake/authentication progress.
- Contains backend-neutral facts and optional encoded certificate data.
- Independently releasable and does not expose backend certificate pointers.
- Snapshot lifetime is independent of later connection progress or destruction.

## Provisional API surface

The following C-like sketch defines responsibilities, not final declarations:

```c
pst_result pst_runtime_create(const pst_runtime_options *options,
                              pst_runtime **out_runtime);
void       pst_runtime_release(pst_runtime *runtime);
pst_result pst_runtime_get_backend_info(const pst_runtime *runtime,
                                        pst_backend_info *out_info);

pst_result pst_config_create(pst_runtime *runtime, pst_config **out_config);
pst_result pst_config_set_policy(pst_config *config,
                                 const pst_policy *policy);
pst_result pst_config_set_credentials(pst_config *config,
                                      pst_credentials *credentials);
pst_result pst_config_set_trust(pst_config *config, pst_trust *trust);
pst_result pst_config_freeze(pst_config *config);
void       pst_config_release(pst_config *config);

pst_result pst_credentials_import(pst_runtime *runtime,
                                  const pst_credential_source *source,
                                  pst_credentials **out_credentials);
void       pst_credentials_release(pst_credentials *credentials);
pst_result pst_trust_create(pst_runtime *runtime,
                            const pst_trust_source *source,
                            pst_trust **out_trust);
void       pst_trust_release(pst_trust *trust);

pst_result pst_connection_create(pst_runtime *runtime,
                                 const pst_config *config,
                                 pst_transport *transport,
                                 pst_transport_ownership ownership,
                                 pst_connection **out_connection);
pst_result pst_connection_handshake(pst_connection *connection,
                                    pst_operation *out_operation);
pst_result pst_connection_read(pst_connection *connection,
                               void *buffer, pst_size capacity,
                               pst_io_result *out_io);
pst_result pst_connection_write(pst_connection *connection,
                                const void *buffer, pst_size length,
                                pst_io_result *out_io);
pst_result pst_connection_get_interest(pst_connection *connection,
                                       pst_interest *out_interest);
pst_result pst_connection_wait(pst_connection *connection,
                               const pst_wait_options *options,
                               pst_interest *out_ready);
pst_result pst_connection_get_peer_info(pst_connection *connection,
                                        pst_peer_info **out_peer_info);
pst_result pst_connection_shutdown(pst_connection *connection,
                                   pst_operation *out_operation);
void       pst_connection_release(pst_connection *connection);

void       pst_peer_info_release(pst_peer_info *peer_info);
```

Phase 1 may split setters or replace bulky structures with accessors, but it must preserve the semantics below.

## Backend selection

Runtime options support three conceptual selection policies:

- **Exact**: require a stable backend identifier; fail if unavailable or incapable.
- **Ordered preference**: try identifiers in caller order, validating required capabilities before selection.
- **Automatic**: choose from compiled/registered backends using documented library preference rules.

Selection occurs during runtime creation, before connections exist. The chosen backend never changes within a runtime. Failure after backend selection does not retry another backend silently. No selection mode may weaken explicit policy.

Backend identifiers are stable diagnostic strings, not enums tied to a fixed backend list. The runtime exposes selected identifier, implementation version, and capability snapshot without exposing backend handles. Phase 2 provides an internal in-process descriptor registry with exact-ID lookup and validation. Ordered and automatic runtime selection remain deferred.

## Capability model

Capabilities are immutable facts reported by the selected backend/runtime. They are distinct from:

1. backend capability: what the implementation can support in this environment;
2. requested policy: what the consumer requires or permits;
3. negotiated result: what this connection actually selected.

Capability queries cover protocol-version ranges and named features such as client authentication, ALPN, custom trust, system trust, resumption, 0-RTT, hostname verification, peer-certificate extraction, nonblocking operation, and backend-correct waiting.

Phase 2 materializes backend capabilities as an internal controlled-width bitmask plus a query hook. Descriptor bits represent implementation facts, while the query may refine environment-dependent availability. Unknown capabilities are treated as unavailable by current core logic, never implicitly supported. Requested-policy and negotiated-result representations remain separate and deferred. Freezing configuration will validate requirements that can be checked statically; connection/handshake will validate the rest.

## Configuration and policy

`pst_policy` expresses generic requirements:

- minimum/maximum or permitted TLS versions;
- whether peer authentication is required;
- whether local/client authentication is required;
- ordered ALPN offers and whether an acceptable negotiation is mandatory;
- expected hostname when hostname verification is required;
- credential and trust references;
- whether resumption and 0-RTT are prohibited, permitted, or required where meaningful;
- shutdown expectations;
- other versioned requirements added later.

Security-sensitive fields use explicit tri-state/requirement values rather than ambiguous zero defaults where zero could weaken security. A requested requirement that is unsupported or not satisfied fails closed. The core contains no `papacc/1`, private Accelerator root, Browser Web PKI default, or consumer profile name.

## Transport contract

High-level connection creation accepts a PST transport adapter, never a public native socket type. The minimum adapter contract provides:

- opaque caller/backend-neutral state pointer;
- nonblocking read and write operations;
- transport-level interest/wait integration where applicable;
- close/abort operation;
- optional native-import capability exposed only through a platform-specific adapter boundary, not the generic public header;
- version/size and capability fields.

PST is not a generic transport framework: callbacks exist only to support secure transport. A platform-specific companion adapter may wrap/import Winsock, POSIX descriptors, NSPR, or another facility. A backend may use callbacks or consume a native-import facility internally; all produce the same public connection semantics.

Transport callbacks report bytes and transport operation state separately. They must not call back into the same connection recursively.

## Ownership contract

Transport attachment declares exactly one mode:

- **Borrowed**: caller retains ownership; PST never closes it. Caller must keep it valid until the connection is destroyed and must not close it concurrently.
- **Transferred**: ownership moves to PST only when connection creation succeeds. PST closes exactly once. On failure, ownership remains with the caller.
- **Retained**: adapter supplies an explicit retain/release mechanism. PST obtains a reference only on successful creation and later releases that reference exactly once.

The create operation is transactional: before success the caller remains owner; after success the declared contract applies. Backend import failure must unwind intermediate NSPR/SSL/backend layers without closing a caller-owned borrowed resource or a not-yet-transferred resource.

The legacy NSS adapter may implement successful transfer as native socket -> NSPR -> SSL, after which closing the SSL aggregate is the single close path. The caller must invalidate its former handle after successful transfer.

Credentials, trust, configuration, and runtime follow retained-reference semantics internally: a child either retains what it needs on successful creation or creation fails without changing caller ownership.

## Operation and handshake state

`pst_result` reports whether the API call itself was valid/successfully evaluated. Normal nonblocking progress is returned in `pst_operation`, not encoded as a fatal error.

The minimal operation states are:

- `COMPLETE`: requested protocol operation completed;
- `NEED_READ`: backend needs read-side readiness/progress;
- `NEED_WRITE`: backend needs write-side readiness/progress;
- `NEED_READ_WRITE`: either/both may permit progress;
- `CLOSED`: peer clean-close or local shutdown state prevents the requested operation;
- `FAILED`: fatal normalized error is available.

Handshake is called repeatedly until COMPLETE, CLOSED, or FAILED. A call may advance internal state even when it ultimately returns a need state. Deadlines and retry scheduling belong to the consumer/event-loop orchestration, not implicit blocking inside handshake.

## Readiness model

Readiness has two layers:

1. **Interest query** returns the current secure/backend interest after an operation needs progress.
2. **Wait/poll** optionally asks the backend/adapter to wait using the backend-correct mechanism and a caller-supplied bounded timeout/deadline.

External event loops may use the query plus an adapter integration token/callback supplied by a platform-specific integration layer. They must not infer secure interest solely from a native socket.

For NSS, backend wait may use `PR_Poll()` on the SSL descriptor. No `PRPollDesc`, native socket, or backend descriptor is public. A backend that cannot integrate external polling must report that capability accurately; callers can use the bounded PST wait operation if available.

Interest is a hint tied to current connection state and becomes stale after any operation. Readiness means an operation may make progress, not that it will complete.

## Secure read and write

`pst_io_result` separates:

- `bytes_transferred`;
- operation state;
- clean EOF/peer close classification;
- normalized fatal error when state is FAILED.

Partial reads and writes are successful progress. A write caller retries only the unconsumed suffix. NEED_READ may occur during write and NEED_WRITE during read because TLS can require protocol traffic in either direction.

Zero transferred bytes is never overloaded to mean every condition:

- clean peer `close_notify` is CLOSED with clean-close classification;
- abrupt transport EOF is a transport/TLS truncation failure unless policy explicitly allows otherwise;
- would-block is a NEED state, not an error;
- fatal TLS or authentication failure is FAILED with normalized error;
- local shutdown/closed state is reported distinctly.

Buffers remain caller-owned and need only remain valid for the duration of each call; the initial API does not retain application buffers.

## Error and diagnostics model

Normalized result/error categories include:

- success;
- invalid argument;
- invalid state;
- unsupported;
- unavailable backend/capability;
- memory/resource exhaustion;
- transport failure;
- TLS/protocol failure;
- authentication/certificate failure;
- hostname mismatch;
- policy violation;
- backend initialization/internal failure;
- truncated/abrupt close;
- clean closed/EOF where applicable.

NEED_READ/NEED_WRITE are operation states, not error categories.

Each runtime/connection retains the latest structured diagnostic record with normalized category, operation stage, selected backend identifier, and optional backend-specific numeric/text detail. Backend detail is for logging and support; portable control flow must use normalized results only. Diagnostic storage lifetime and thread-local versus object-local mechanics are deferred to implementation, but querying it must not expose backend pointers.

## Peer identity and negotiated result

`pst_peer_info` is an immutable backend-neutral snapshot that can report:

- whether peer authentication completed and under which policy;
- whether a peer certificate was present;
- hostname verification result and expected name when applicable;
- negotiated protocol version, cipher identifier/name, ALPN, resumption, and early-data result;
- cryptographic fingerprint with named hash algorithm;
- optional DER-encoded leaf certificate and chain entries;
- optional normalized subject/issuer display strings when provided reliably.

PST does not parse or validate X.509 itself. The backend performs validation; PST copies/normalizes selected results. Encoded certificate bytes are opaque evidence for consumers that need deeper parsing through another appropriate library.

## Credentials and trust

Credential sources and trust sources are extensible descriptors. Initial required categories are:

- local certificate chain plus private-key reference for mTLS;
- private/custom trust anchors;
- system trust when the backend/platform supports it;
- file/memory sources through later loader helpers;
- backend-native references through platform/backend-specific companion APIs.

Generic APIs must not require one file format or expose a backend database handle. Import validates source compatibility and binds the resulting immutable object to its runtime/backend. Cross-runtime/backend use fails explicitly.

Password callbacks, hardware keys, NSS databases, OS stores, and Web PKI loading details are later credential/backend work. Phase 0.C defines their boundary, not loaders.

## ALPN

Configuration supplies an ordered list of length-delimited protocol byte strings and whether successful negotiation is optional or required. The connection snapshot returns the exact negotiated protocol bytes.

If ALPN is required and no offered acceptable protocol is negotiated, handshake fails with a policy error. Protocol identifiers are data; `papacc/1` is never compiled into PST core behavior.

## Shutdown and close

Four operations/concepts remain distinct:

- **graceful TLS shutdown**: incremental protocol operation that may need read/write and produces/consumes `close_notify`;
- **transport close/abort**: closes or aborts according to transport ownership;
- **connection destruction**: releases memory/backend resources and performs the single required owned-resource close, but does not block to complete TLS shutdown;
- **peer close classification**: clean `close_notify`, abrupt EOF/truncation, or fatal protocol/transport close.

Consumers that require graceful shutdown call it and drive it incrementally before release. Release is always bounded/nonblocking from the API perspective. Policy may require reporting incomplete graceful shutdown but cannot make destructor blocking.

## Threading model

The initial contract is deliberately restrictive:

- A connection is single-application-thread-at-a-time. Concurrent calls, including close versus I/O, are invalid/unsupported.
- A frozen configuration, immutable credentials, immutable trust, peer snapshots, and runtime capability information may be shared only if the runtime reports thread-safe sharing capability; otherwise callers serialize access.
- Runtime create/release and backend-global coordination are internally safe to the extent needed to prevent corruption, but PST does not provide a general synchronization layer.
- Callbacks execute synchronously on the calling thread unless a later API explicitly states otherwise.

Phase 1 must document exact reference/lifetime synchronization. Per-connection worker threads are not required or implied.

## ABI and evolution

- Public stateful objects remain opaque.
- Public option/result structures are size- and version-tagged, zero-initialized by callers, and append-only within an ABI generation.
- Public enums/constants use explicit stable numeric values; unknown values are rejected or ignored only where documented.
- Capabilities use extensible queries rather than assuming a fixed forever bitmask.
- Strings/byte spans carry explicit length; ownership and encoding are specified per field.
- Allocation/free crosses the library boundary only through PST create/release/accessor functions or caller-provided buffers.
- Backend identifiers are strings; adding a backend does not extend a public enum.
- A runtime API-version query and compile-time header version allow compatibility checks.

Phase 1 fixes the portable foundation ABI at API version 1.0.0: `PST_CALL` is `__cdecl` on MSVC, `PST_API` supports future static/DLL builds, integer widths are checked at compile time, and stateful handles are incomplete public types. `PST_VERSION_INFO` begins with `struct_size` and `api_version`; the library rejects undersized records and incompatible API major versions, accepts same-major versions, and leaves caller extensions beyond the known record untouched. Plugin ABI, dynamic backend loading, and transport/runtime structure layouts remain deferred.

## Deferred beyond Phase 0.C

- Portable header spelling, calling convention, visibility macros, integer typedefs, result codes, version record, and opaque handle declarations: implemented in Phase 1.
- Transport/runtime option and result layouts beyond `PST_VERSION_INFO`: deferred to the phase that implements each surface.
- Internal backend SPI/vtable, validation, capabilities, and setup-time registry: implemented in Phase 2; see [backend-spi.md](backend-spi.md).
- Opt-in RetroZilla NSS/NSPR descriptor, lifecycle, private Win32 socket import, incremental operations, and `PR_Poll` readiness: implemented in Phase 3; credential/trust integration remains deferred.
- Credential/trust loaders and complete peer-identity accessors: Phase 4.
- Runtime/event-loop implementations and convenience blocking orchestration: Phase 5.
- Legacy-platform compilation and behavioral validation of the implemented API: Phase 6.
- Interoperability, fuzzing, negative tests, hardening, and stable release ABI: later phases.

Phase 3 adds an opt-in real NSS/NSPR provider behind the internal SPI. It does not add a public runtime, credential/trust loader, peer-info model, consumer integration, or custom TLS/cryptography implementation.
