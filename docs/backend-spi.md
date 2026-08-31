# Internal backend SPI

Status: Phase 2 implementation. This contract is internal to PapinhoSecureTransport and is not a consumer API or an external plugin ABI.

## Boundary and versioning

The SPI is declared in `src/pst_backend.h`. Consumers include only `include/papinho_secure_transport.h`; they never receive descriptors, vtables, or backend-private pointers. The SPI has its own packed version, currently 2.0 (`0x00020000`), independent of the public API version. Descriptor and vtable records carry both `struct_size` and `spi_version`. The current core accepts the current SPI major and records at least as large as the known layouts.

There is no dynamic loading, DLL discovery, or binary plugin contract. Backends are linked into the process and registered during controlled setup.

## Descriptor and capabilities

`PST_BACKEND_DESCRIPTOR` contains:

- a size and SPI version;
- a stable lowercase textual ID composed of ASCII letters, digits, `.`, `_`, or `-`;
- an optional human-readable name;
- a static capability mask;
- a pointer to the internal function table.

Capability bits describe backend availability, not caller requirements or a negotiated connection result. The descriptor advertises implementation-level facts; `query_capabilities` may report environment-dependent availability when backend state exists. Unknown future bits do not imply support to current core code.

Phase 2 defines bits for the protocol/features already anticipated by the architecture: TLS 1.2, TLS 1.3, client authentication, ALPN, custom/system trust, hostname verification, resumption, early data, peer information, incremental nonblocking operation, and backend-correct waiting. The test backend advertises only nonblocking operation and backend-correct waiting. It does not advertise or implement TLS.

Validation rejects early-data capability without resumption, backend-wait capability without a wait hook, and peer-info capability without both peer-info lifecycle hooks.

## Registry and validation

The initial registry is an in-process array with capacity for eight linked descriptors. It supports validation, registration, exact lookup by stable ID, duplicate rejection, unregister, count, and reset. Registry entries retain descriptor pointers rather than copying them, so registered descriptors and vtables must have static or otherwise registry-long lifetime.

The registry is intentionally not a dynamic plugin manager and performs no locking. Registration, unregister, and reset are setup/test operations and must not race or occur while runtimes use a descriptor. Runtime selection policy and process-wide synchronization are deferred to the runtime phase.

Validation rejects null or undersized records, incompatible SPI majors, invalid/empty IDs, absent vtables, undersized or incompatible vtables, missing mandatory hooks, and inconsistent capabilities. Registration validates before mutation and rejects duplicate IDs.

## Lifecycle and private state

The vtable separates backend-global initialization from runtime and connection lifecycle:

1. `initialize` creates opaque backend-private state; failure returns no usable state.
2. `runtime_create` creates opaque state owned by one future PST runtime.
3. `connection_create` creates opaque state owned by one future PST connection.
4. Destruction occurs in reverse order through `connection_destroy`, `runtime_destroy`, and `shutdown`.

The core treats every state pointer as opaque `void *`. Concrete implementations define their private types in backend-owned source files.

## Dispatch and operation results

Handshake and graceful shutdown are incremental step hooks. They return call validity through `PST_RESULT` and progress separately as COMPLETE, NEED_READ, NEED_WRITE, NEED_READ_WRITE, CLOSED, or FAILED. A FAILED operation carries a normalized `PST_RESULT`; backend-native statuses are not part of the SPI contract.

Read and write return `PST_BACKEND_IO_RESULT`, separating bytes transferred, incremental operation state, close classification, and normalized error. Partial progress and cross-direction needs are representable. CLEAN and TRUNCATED close classifications remain distinct.

Peer-information create/destroy hooks are optional and may be advertised only with the matching capability. The concrete snapshot format remains a Phase 4 concern.

## Readiness and waiting

`get_interest` reports NONE, READ, WRITE, or READ|WRITE for the backend's current secure state. `wait` is a separate optional hook and receives the backend connection state, requested interest, and a bounded timeout in milliseconds. Its result distinguishes ready interest from timeout.

This separation permits a future backend to use its correct secure-layer polling mechanism without exposing a native or backend handle. Phase 2 performs no polling and opens no transport.

## Transport ownership

`attach_transport` receives opaque transport state and one explicit mode: BORROWED, TRANSFERRED, or RETAINED. The hook initializes `ownership_accepted` to zero and changes it to one exactly when a TRANSFERRED resource crosses an irreversible backend boundary. This separately reports transfer from the final operation result:

- on failure with `ownership_accepted == 0`, ownership remains with the caller;
- on failure with `ownership_accepted == 1`, the backend has already consumed and cleaned up the resource, so the caller must invalidate its former handle;
- BORROWED never grants close ownership to the backend;
- TRANSFERRED makes the backend/PST path the sole closer after success;
- RETAINED requires the later transport adapter contract to obtain and release one reference transactionally.

Phase 2 defines and tests this transition contract but does not define a public transport adapter or import any native resource. The complete adapter and release mechanics remain deferred.

## Phase 2 test backend

`tests/test_backend_spi.c` contains a private, deterministic mock named `test-backend`. It uses static counters and opaque mock state to exercise validation, registry behavior, lifecycle dispatch, capability queries, incremental handshake states, readiness, timeout, partial I/O, shutdown, and transactional ownership. It is not linked as a production backend and performs no cryptography, protocol processing, networking, or plaintext transport.

## First implementation

Phase 3 implements `retrozilla-nss` behind this SPI. The NSS descriptor is compiled only by explicit backend targets and remains absent from the portable public header. Native import required SPI 2.0 to add `ownership_accepted` to `attach_transport`, because ownership can cross irreversibly at a successful intermediate import even when later secure-layer construction fails. See [backend-nss.md](backend-nss.md).
## Deferred work

Phase 3 may implement the first real backend behind this contract. Later phases still own public runtime/configuration/transport APIs, credential and trust material, peer identity records, selection policy, concurrency, diagnostics, and actual secure operations. Phase 2 does not begin any of that work.