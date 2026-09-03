# Public API and ABI baseline

Status: **Phase 9.B complete**. This document freezes the public API/ABI baseline at API 1.2.0 and library 0.3.0. SPI 2.4 remains internal and Phase 9.C has not started.

## Public-header boundary

The intentional consumer headers are:

- `include/papinho_secure_transport.h`: portable, provider-neutral API.
- `include/papinho_secure_transport_win32.h`: optional Win32 adapter entry points; it includes the portable header but no Windows SDK or provider header.

Every header under `src/`, including `pst_backend.h`, is private. Headers under `third_party/` belong to dependencies and are not PST public API. A consumer can compile either public header first, without including Windows, Winsock, NSS/NSPR, Schannel/SSPI/CryptoAPI, or OpenSSL headers. Automated VC6 x86 and modern MSVC x64 `/W4` include-only tests prove this boundary.

The portable header exposes no `SOCKET`, `HANDLE`, `BOOL`, `DWORD`, `PRFileDesc`, `SECItem`, SSPI handle, OpenSSL object, provider vtable, or native certificate type. The Win32 adapter represents a socket-sized value as `pst_size`; conversion and ownership are private. Provider IDs are the case-sensitive ASCII strings `retrozilla-nss`, `schannel`, and `openssl`.

## Linkage and compatibility scope

Public functions use `PST_CALL`, which is `__cdecl` under MSVC, and C++ declarations are enclosed in `extern "C"`. `PST_API` expands to `dllexport` for `PST_BUILD_DLL`, `dllimport` for `PST_USE_DLL`, and empty otherwise. Public names occupy the `pst_`/`PST_` namespace. The headers set no packing pragma and therefore cannot leak a changed packing state.

Current canonical Makefiles produce static `.lib` files, not a PST DLL. The frozen binary layout and calling contract applies separately to VC6 Win32 x86 and modern MSVC Windows x64 with their documented default packing and toolchains. It does not promise binary compatibility across architectures, arbitrary compilers, CRT models, or unsupported DLL packaging. A future DLL must validate its export table and preserve `__cdecl`; `PST_API` alone is not evidence that a DLL release exists. Static consumers must link the library built for their exact target/toolchain model.

Opaque types (`pst_runtime`, `pst_config`, `pst_credentials`, `pst_trust`, `pst_connection`, `pst_peer_info`, and `pst_transport`) have no public size or fields. Their pointer representation follows the target ABI; consumers never allocate, copy, inspect, or free them through the CRT.

## Scalar, boolean, string, and buffer rules

`pst_u8`, `pst_u16`, `pst_u32`, and `pst_i32` are compile-time checked as 1, 2, 4, and 4 bytes. `PST_RESULT` is signed 32-bit. Public booleans and tri-state values are `pst_u32`, never C++ `bool` or Win32 `BOOL`; valid boolean values are 0 and 1 unless a named four-state `PST_KNOWN_*` value is specified.

`pst_size` is `size_t`: 32-bit on the VC6 x86 target and 64-bit on the modern x64 target. It describes memory buffer sizes/counts and the adapter-sized native-socket token. Conversion to provider-native signed/count types must reject overflow before a native call. `pst_size` is therefore target-specific and not wire-format data.

Backend IDs and `pst_result_string` outputs are NUL-terminated narrow ASCII. Runtime-info backend IDs and result strings are borrowed; the former is valid while its runtime/provider descriptor remains alive and the latter has static lifetime. Hostname and ALPN inputs use explicit lengths and are copied. Hostnames reject embedded NUL and gain a private terminator; no IDNA or IP-literal expansion is promised. ALPN entries are arbitrary nonempty bytes of length 1..255 and may contain NUL. `backend_id[32]` outputs copy at most 31 bytes and always terminate. DER buffers are binary and length-delimited.

## Frozen numeric values

`PST_RESULT` values are: OK 0, INVALID_ARGUMENT 1, INVALID_STATE 2, UNSUPPORTED 3, UNAVAILABLE 4, OUT_OF_MEMORY 5, RESOURCE_FAILURE 6, TRANSPORT_FAILURE 7, PROTOCOL_FAILURE 8, AUTH_FAILURE 9, HOSTNAME_MISMATCH 10, POLICY_VIOLATION 11, BACKEND_FAILURE 12, TRUNCATED 13, CLOSED 14, and INCOMPATIBLE_API 15.

TLS versions deliberately use normalized PST values TLS1.2=12 and TLS1.3=13, not wire protocol numbers. Existing values are frozen:

- selection: EXACT=1, ORDERED=2, AUTOMATIC=3;
- features: DISABLED=0, OPTIONAL=1, REQUIRED=2;
- progress: COMPLETE=0, NEED_READ=1, NEED_WRITE=2, NEED_READ_WRITE=3, CLOSED=4, FAILED=5;
- interest bits: NONE=0, READ=1, WRITE=2; READ_WRITE is the bitwise union;
- close: NONE=0, CLEAN=1, TRUNCATED=2;
- trust: CUSTOM_CA_DER=1, SYSTEM=2; credential DER/PKCS#8 kind=1;
- logging: OFF=0, ERROR=1, WARN=2, INFO=3, DEBUG=4, TRACE=5;
- diagnostic operations: NONE=0 and RUNTIME through PEER_INFO=1..11;
- log event IDs and categories are the documented values 1..8;
- capability bits TLS1.2 through BACKEND_WAIT are `0x001` through `0x800` in successive powers of two.

These numbers and the 32-bit mask width are frozen. Future enum-like values use unused numbers; future capabilities use unused bits. Unknown inputs are rejected where an input domain is closed. Unknown output values must be handled by consumers without indexing unchecked arrays or assuming the maximum known value.

## Structure layout baseline

All offsets are decimal bytes using canonical default MSVC packing. `s/a/mn/mx` below means `struct_size`, `api_version`, `minimum_version`, and `maximum_version`.

| Structure | x86 size / offsets | x64 size / offsets |
|---|---|---|
| `PST_DIAGNOSTIC_INFO` | 56: s0,a4,valid8,generation12,result16,operation20,backend24 | identical |
| `PST_LOG_EVENT` | 60: s0,a4,level8,event12,category16,result20,operation24,backend28 | identical |
| `PST_LOG_CONFIG` | 20: s0,a4,level8,callback12,context16 | 32: s0,a4,level8,callback16,context24 |
| `PST_VERSION_INFO` | 32: fields at 0,4,8,12,16,20,24,28 | identical |
| `PST_CREDENTIAL_SOURCE` | 28: s0,a4,kind8,cert12,cert_size16,key20,key_size24 | 48: s0,a4,kind8,cert16,cert_size24,key32,key_size40 |
| `PST_TRUST_SOURCE` | 20: s0,a4,kind8,data12,size16 | 32: s0,a4,kind8,data16,size24 |
| `PST_IDENTITY_CONFIG` | 32: s0,a4,credentials8,trust12,hostname16,size20,peer24,client28 | 48: s0,a4,credentials8,trust16,hostname24,size32,peer40,client44 |
| `PST_PEER_INFO_SUMMARY` | 84: u32 fields 0..40,hash44,hash_size76,leaf_size80 | 96: u32 fields 0..40,hash44,hash_size80,leaf_size88 |
| `PST_RUNTIME_OPTIONS` | 28: s0,a4,selection8,exact12,preferred16,count20,caps24 | 48: s0,a4,selection8,exact16,preferred24,count32,caps40 |
| `PST_RUNTIME_INFO` | 16: s0,a4,backend8,caps12 | 24: s0,a4,backend8,caps16 |
| `PST_ALPN_PROTOCOL` | 8: data0,size4 | 16: data0,size8 |
| `PST_TLS_POLICY` | 40: s0,a4,mn8,mx12,alpn16,count20,requirement24,resumption28,early32,graceful36 | 48: s0,a4,mn8,mx12,alpn16,count24,requirement32,resumption36,early40,graceful44 |
| `PST_IO_RESULT` | 16: bytes0,operation4,close8,error12 | 24: bytes0,operation8,close12,error16 |
| `PST_WAIT_RESULT` | 8: interest0,timeout4 | identical |

`PST_DIAGNOSTIC_INFO` and `PST_LOG_EVENT` have fixed provider-neutral value layouts. Their backend buffers are inline and contain no pointer. `PST_LOG_EVENT` is ephemeral and callback-only. `PST_IO_RESULT`, `PST_WAIT_RESULT`, and `PST_ALPN_PROTOCOL` are complete value records and are not independently version tagged.

Versioned structures begin with `struct_size` and `api_version`. Current minimum sizes equal the current complete records, except the explicitly fixed diagnostic/log-event constants. Callers initialize the current complete record; same-major API versions and larger records are accepted where an API consumes a versioned record. The implementation reads only the frozen known prefix and preserves caller bytes beyond it. Existing prefixes and offsets cannot move. Future compatible growth appends optional tail fields, raises the initializer's size, and reads each tail only after a complete-field size guard. A required prefix change is an API-major ABI break.

Initializers deterministically zero their known record and set current size/version. `pst_version_info_init`, `pst_diagnostic_info_init`, and `pst_log_config_init` reject NULL. Reserved/unknown future fields supplied by callers must be zero unless their defining API says otherwise.

## Public function inventory

All 40 declarations have matching implementations in the applicable target build; no duplicate, obsolete, or experimental public declaration was found. Historical compatible constructors remain.

| Domain | Functions | Ownership, state, and failure contract |
|---|---|---|
| version/result | `pst_api_version`, `pst_library_version`, `pst_version_info_init`, `pst_get_version`, `pst_result_string` | value/static outputs; no allocation; version record validates size and API major |
| diagnostics/logging | `pst_diagnostic_info_init`, `pst_log_config_init`, `pst_runtime_copy_diagnostic`, `pst_connection_copy_diagnostic` | copied caller-owned snapshots; larger tail preserved; no native detail or pointer |
| credentials/trust | `pst_credentials_create/release`, `pst_trust_create/release` | constructors copy DER and return NULL on failure; release accepts NULL; handles are retained by config |
| config | `pst_config_create`, `pst_config_set_identity`, `pst_config_set_tls_policy`, `pst_config_freeze`, `pst_config_release` | setters are transactional and allowed before freeze; frozen setters fail; connection retains config |
| peer | `pst_peer_info_get_summary`, `pst_peer_info_copy_leaf_der`, `pst_peer_info_release` | owned snapshot independent of connection; summary/DER copied; release accepts NULL |
| runtime | `pst_runtime_create`, `pst_runtime_create_ex`, `pst_runtime_create_with_logging`, `pst_runtime_get_info`, `pst_runtime_release` | output NULL on failure; `_ex` preserves diagnostic; early release is guarded while children exist |
| connection | `pst_connection_create`, `pst_connection_create_ex`, `pst_connection_attach`, `pst_connection_release` | constructor transactional and retains config; attach reports explicit ownership acceptance; release closes owned transport exactly once |
| progress/I/O | `pst_connection_handshake`, `pst_connection_get_interest`, `pst_connection_wait`, `pst_connection_read`, `pst_connection_write`, `pst_connection_shutdown` | bounded incremental calls; deterministic outputs; readiness is not progress; terminal states do not resurrect |
| negotiated data | `pst_connection_get_peer_info`, `pst_connection_get_negotiated_alpn` | only established state; owned peer handle or copied ALPN; outputs reset on failure |
| transport | `pst_transport_release` | accepts NULL; releases only caller-owned/unaccepted transport |
| Win32 adapter | `pst_win32_register_retrozilla_nss`, `pst_win32_socket_transport_create` | target-specific factory/registration; socket represented without exposing a native type; output NULL on failure |

API 1.0 established versions/results, opaque handles, configuration/runtime/connection/transport, TLS policy, readiness and I/O. API 1.1 added structured diagnostics and compatible `_ex` constructors. API 1.2 added consumer logging and `pst_runtime_create_with_logging`. Exact per-symbol introduction metadata before those recorded milestones is historical documentation, not a runtime dispatch mechanism.

## Lifecycle and ownership

Create functions require an output pointer and set it to NULL before validation. Release functions accept NULL. No public handle may be passed to `free`; each is released through its matching PST function. Double release of a stale non-NULL pointer is undefined misuse; the ABI does not promise tombstone tracking.

Credential/trust inputs are copied, including private PKCS#8 bytes; caller mutation or release after successful creation does not affect PST. Configuration retains credential/trust handles and copies hostname/ALPN. A connection retains its frozen config. A runtime owns provider state and tracks live connections; an attempted early runtime release is a guarded no-op, and the caller must release it again after all children. Peer snapshots own their DER and outlive the connection.

Before `pst_connection_attach`, the caller owns the transport. `ownership_accepted=0` means the caller still owns it on every failure path. `ownership_accepted=1` means PST/provider owns it even if the call subsequently fails; exactly one close occurs through connection destruction. Only `PST_OWNERSHIP_TRANSFERRED` is accepted.

PST allocations never cross the CRT boundary: handles are released by PST, callback events are borrowed, string pointers are static/borrowed, and variable outputs use consumer-provided buffers. The API has no function requiring the consumer to free PST-allocated raw memory.

## Configuration and policy

CUSTOM and SYSTEM trust are exclusive source kinds; PST does not union or silently fall back between them. A provider without the requested capability returns UNSUPPORTED during selection/validation. Credentials remain certificate DER plus unencrypted PKCS#8 DER only.

Setters validate before committing. Rejected identity/TLS policies leave the previous config unchanged. Freeze is idempotent; post-freeze setters return INVALID_STATE. Peer authentication requires trust and hostname; client authentication requires credentials.

TLS min/max must be normalized supported constants with min <= max. PST does not clamp, widen, or downgrade. ALPN preserves caller order and copies multiple protocols. Required ALPN absence/mismatch is POLICY_VIOLATION; optional absence is represented by an empty negotiated output. Unsupported requested features fail rather than silently enabling a weaker policy.

## State and operation matrix

| Operation | CREATED | ATTACHED/HANDSHAKING | ESTABLISHED | SHUTTING | CLOSED/FAILED |
|---|---|---|---|---|---|
| attach | allowed once | INVALID_STATE | INVALID_STATE | INVALID_STATE | INVALID_STATE |
| handshake | INVALID_STATE | incremental | INVALID_STATE | INVALID_STATE | INVALID_STATE |
| get interest / wait | INVALID_STATE | allowed while progress needs readiness | allowed for pending I/O | allowed | INVALID_STATE |
| read/write | INVALID_STATE | INVALID_STATE | incremental | INVALID_STATE | INVALID_STATE |
| peer info / ALPN | INVALID_STATE | INVALID_STATE | allowed | INVALID_STATE | INVALID_STATE |
| shutdown | INVALID_STATE | INVALID_STATE | begins/increments | incremental | INVALID_STATE |
| release | allowed | allowed | allowed without shutdown | allowed/aborts bounded work | allowed |

A successful handshake transitions to ESTABLISHED. Authenticated `close_notify` yields CLOSED/CLEAN. Data received before a clean close remains deliverable. Unexpected EOF/reset yields FAILED/TRUNCATED. Fatal operation/wait errors yield FAILED. CLOSED and FAILED are terminal.

Handshake, read, write, and shutdown report progress separately from API-call validity. NEED_READ/WRITE/READ_WRITE means retry only after appropriate readiness. Wait is bounded by `timeout_ms`; timeout is a successful wait result with `timed_out=1`, not operation progress. No public API creates a hidden infinite wait. Partial read/write byte counts are authoritative; callers advance only by `bytes_transferred`, preventing duplication.

On validation/state failure, handshake/shutdown return operation FAILED plus the normalized error; interest returns NONE; wait returns NONE/not-timed-out; I/O returns zero bytes, FAILED, close NONE, and the normalized error. Constructor and owned-handle outputs are NULL; size outputs are zero unless an API deliberately returns the required capacity (`pst_peer_info_copy_leaf_der`). This deterministic-output rule was hardened during 9.B without changing signatures/layouts or valid-call behavior.

## Diagnostics, logging, callbacks, and peer data

`PST_DIAGNOSTIC_INFO` is a copied, allocation-free snapshot. It carries only validity, modular generation, normalized result, coarse operation, and copied backend ID. Clear/reset increments generation; generation is neither time nor a global correlation ID. Larger caller tails remain untouched. Hostnames, ALPN, DER, keys, payload, paths, handles, native codes, and arbitrary text never cross this boundary.

`PST_LOG_CALLBACK` is `void PST_CALL callback(void *context, const PST_LOG_EVENT *event)`. Delivery is synchronous on the calling thread. The event pointer is valid only during the callback; consumers copy it if needed. Context is consumer-owned and must outlive the runtime. There are no worker-thread or late callbacks. Same-object reentrancy from a callback is unsupported; callbacks must not mutate/release the object currently emitting. Threshold only changes observation and cannot affect TLS behavior.

Peer summary exposes normalized TLS version, cipher-suite number, authentication/validation state, negotiated-ALPN availability, resumption/early-data knowledge, SHA-256 leaf fingerprint, and leaf-DER sizes. Leaf DER is copied separately. It exposes neither a provider-native certificate nor a full public chain API.

## Forward and hostile-input policy

Tests cover undersized, exact, and larger versioned records where the APIs support them; incompatible API major; preserved larger tails; unknown enum/flag values; NULL/count-pointer mismatches; empty and embedded-NUL strings; fixed backend-ID truncation/termination; ALPN item/count/aggregate overflow; DER pointer/size pairs; transactional setter rejection; output initialization; ownership acceptance; and lifecycle retain/release balance.

Smaller records below their frozen minimum fail INVALID_ARGUMENT. Equal records are accepted. Larger same-major records are accepted and unknown tails are not read or overwritten. Unknown closed-domain selector, feature, TLS, trust, logging, and ownership values fail. Unknown required capability bits cannot be silently satisfied. Integer overflow is rejected before allocation or native conversion.

No public ABI change may weaken fail-closed trust, hostname, mTLS, ALPN, no-downgrade, terminality, disclosure, ownership, or bounded-progress invariants.

## ABI regression artifact

`tests/test_public_abi.c` is the executable frozen baseline. Compile-time C89 assertions cover every public structure size and all alignment-sensitive/fixed-buffer offsets for VC6 x86 and modern MSVC x64. Runtime checks cover versions, result/value sets, capabilities, logging/progress/close constants, and deterministic failure outputs. `tests/test_public_header.c` and `tests/test_public_win32_header.c` separately prove standalone header compilation without prior platform/provider headers.

The test deliberately excludes compiler-specific debug records, object timestamps, library member order, and internal symbol layout. Those are not public ABI. The static archive contains internal link-visible C symbols because it is not a DLL export boundary; consumers contract only with declarations in the two public headers.

## Threading and release decision

PST makes no general thread-safety guarantee in API 1.2.0. Registry setup is caller-serialized; a connection and its configuration/lifecycle must not be concurrently mutated; callbacks are synchronous. Independent provider/runtime behavior is only guaranteed where explicitly tested and documented. This avoids inventing locking or reentrancy promises during an ABI freeze.

Phase 9.B found one release-blocking deterministic-output defect and fixed it at the portable core boundary. The correction initializes outputs only on invalid/pre-provider paths and does not alter successful calls, structure layout, ownership, readiness, TLS, provider behavior, or wire behavior. API remains 1.2.0, library 0.3.0, SPI 2.4.

The public source/ABI baseline is frozen for the two canonical target/toolchain pairs above. Phase 9.C, the internal SPI/provider contract freeze, is next but remains not started until explicitly requested.
