# Public TLS runtime

Phase 5 joins provider selection, frozen policy, transport attachment, incremental TLS operations and peer identity behind the public PST API. Consumers do not include the SPI or any NSS/NSPR header.

`pst_runtime_create` supports exact backend ID, ordered preference and automatic registry order. Capability requirements are checked before a runtime is returned. Selection ends before connection creation; handshake, authentication, hostname or policy failure never retries another provider. Runtime info exposes only the textual backend ID and portable capability bits.

`pst_config` copies identity and TLS policy, then freezes. TLS versions use `PST_TLS_VERSION_1_2`/`1_3`, not provider IDs. ALPN inputs are ordered length-delimited byte strings and may be optional or required. Required ALPN without a negotiated match fails closed. Resumption-required and all 0-RTT activation currently return unsupported; 0-RTT defaults off.

The generic public header contains only opaque `pst_transport`. The separate `papinho_secure_transport_win32.h` adapter accepts an opaque-width native socket value and is implemented beside the Win32 NSS provider. Before attach, the adapter closes the transferred socket if released. Once `ownership_accepted` is one, the connection owns the wrapper and NSS aggregate and performs the sole close path.

The core lifecycle is CREATED, ATTACHED, HANDSHAKING, ESTABLISHED, SHUTTING_DOWN, CLOSED or FAILED. It is an API misuse guard, not a TLS implementation. Handshake/read/write/shutdown calls each perform one bounded provider operation. Interest and bounded wait are separate; NSS maps wait to `PR_Poll` on its SSL descriptor. No busy loop, hidden unbounded wait or assumption that write only needs WRITE exists.

Read/write results keep byte progress, progress state, clean close, truncation and fatal error separate. Readiness is not equivalent to operation progress. A conservative `NEED_READ_WRITE` combined with level-triggered WRITE readiness could previously repeat WRITE-only wakeups without advancing an application read. The internal, backend-neutral progress guard now allows the auxiliary readiness once, then suppresses that same auxiliary-only readiness for the next wait when the retried operation transfers no bytes, keeps the same interest, and does not complete. A timeout re-enables the auxiliary interest, while primary readiness, real byte progress, interest changes, handshake, and shutdown reset the guard. Peer info is available only when established and remains an owned independent snapshot. Shutdown is incremental; connection release is bounded and does not wait for peer `close_notify`.

The initial threading rule is one application thread at a time per connection. No worker or global locking framework was introduced. PST-owned private-key bytes are overwritten through a volatile internal byte loop before release; this is a minimal C89 best effort rather than a secure allocator.

## Minimal flow

```c
pst_win32_register_retrozilla_nss();
pst_runtime_create(&runtime_options, &runtime);
pst_config_create(&config);
pst_config_set_identity(config, &identity);
pst_config_set_tls_policy(config, &policy);
pst_config_freeze(config);
pst_connection_create(runtime, config, &connection);
pst_win32_socket_transport_create(native_value, &transport);
pst_connection_attach(connection, transport, PST_OWNERSHIP_TRANSFERRED, &accepted);
/* Repeated bounded handshake + get_interest/wait calls. */
/* Secure read/write, peer snapshot, incremental shutdown. */
pst_connection_release(connection);
pst_runtime_release(runtime);
```

Functional validation on Windows 10 used the VC6 Win32 public-only integration client with the canonical versioned NSS runtime. TLS 1.2 and TLS 1.3 mTLS both negotiated required `fixture/1`, authenticated `localhost`, echoed 25 bytes and completed shutdown. Server-auth without a client certificate also passed. Wrong hostname, untrusted CA, missing client credential, TLS 1.3 policy against a TLS 1.2-only server, and required ALPN mismatch all failed closed.

## Public diagnostic snapshots

API 1.1.0 adds `PST_DIAGNOSTIC_INFO` as an optional local troubleshooting snapshot. Initialize caller storage with `pst_diagnostic_info_init`; copy from live objects with `pst_runtime_copy_diagnostic` or `pst_connection_copy_diagnostic`. Use `pst_runtime_create_ex`/`pst_connection_create_ex` when constructor failure detail is needed without a returned object. Existing constructors remain supported and behave as before.

The snapshot is supplemental: branch on returned `PST_RESULT`, operation, `PST_IO_RESULT`, and `PST_WAIT_RESULT`. It owns its bytes, allocates nothing, survives source destruction, and contains only normalized result, coarse operation, generation, validity, and copied backend ID. It contains no native codes, private phases, pointers, hostname/ALPN/certificate content, credentials, trust, payload, paths, or arbitrary text. WOULD_BLOCK/NEED and wait timeout remain progress, not diagnostic failures.
