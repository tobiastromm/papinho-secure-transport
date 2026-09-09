<!-- SPDX-License-Identifier: MPL-2.0 -->

# Getting started with PST 0.5.0

This guide targets public API 2.0.0. Choose one canonical SDK from the [Target Matrix](../target-matrix.md), then use its `manifest.ini` and `consumer-link.ini` as the exact build/deployment contract.

## Build and link

Add the SDK `include` directory and link `lib/<target-id>/papinho_secure_transport.lib` plus every library in `consumer-link.ini`. Put package-supplied NSS/OpenSSL runtime DLLs beside the executable; do not install them in system directories or rely on a global PATH. Schannel uses Windows system libraries and has no packaged TLS DLL.

Source builds use `tools\build-vc6.bat` for `win32-x86-vc6-retrozilla-nss`, and the documented modern scripts for Schannel, OpenSSL and Combined. Toolchain identity does not imply an OS support range.

## Common setup

1. Call `pst_win32_register_builtin_providers()` once during serialized process setup.
2. Initialize `PST_RUNTIME_OPTIONS` with `struct_size` and `PST_API_VERSION`, then call `pst_runtime_create` or its diagnostic/logging variant.
3. Create credentials/trust snapshots with `pst_credentials_create` and `pst_trust_create`. Caller buffers may be released after successful creation.
4. Initialize every nested field of one complete `PST_CONNECTION_CONFIG`, including its size/API version.
5. Call `pst_connection_create`. Selection occurs transactionally here, not at runtime creation.

Use `PST_CONNECTION_ROLE_CLIENT` for an outbound connection and `PST_CONNECTION_ROLE_SERVER` for an accepted inbound connection. See [basic_client.c](../../examples/basic_client.c), [basic_server.c](../../examples/basic_server.c), [custom_trust.c](../../examples/custom_trust.c), [system_trust.c](../../examples/system_trust.c) and [mtls.c](../../examples/mtls.c).

## Transport and ownership

The application creates and connects a CLIENT socket. For SERVER it also creates the listener, binds, listens, accepts and applies admission policy. PST receives only the connected CLIENT or accepted SERVER socket through `pst_win32_socket_transport_create` and `pst_connection_attach`.

Before `ownership_accepted != 0`, the consumer remains responsible for closing/releasing after failure. Once accepted, PST/provider is the only close root. The listener is never transferred to PST.

## Identity and trust

Local Identity is this endpoint's certificate chain plus unencrypted PKCS#8 private key. Peer Authentication selects DISABLED, OPTIONAL or REQUIRED certificate presentation. Peer Trust is one explicit CUSTOM anchor set or SYSTEM trust; PST never combines or substitutes them. CLIENT Expected Peer Name is configured independently and is not applicable to SERVER.

Authentication reports TLS identity facts, not authorization. The application must map any accepted certificate/Principal to its own permissions.

## TLS, ALPN and provider selection

Set exact TLS minimum/maximum values. CLIENT ALPN lists an ordered offer. SERVER ALPN lists local preference; REQUIRED rejects no match, OPTIONAL permits no negotiated value, and DISABLED sends/selects none.

`PST_BACKEND_SELECTION_EXACT` tries one ID. `ORDERED` copies and considers IDs in caller order. `AUTOMATIC` follows target registration order. Add explicit `required_capabilities` only for requirements not already calculated from configuration. Eligibility uses the requested role mask. After a provider is bound, it is pinned and no later failure causes fallback. See [provider_selection.c](../../examples/provider_selection.c) and [Providers](../providers.md).

## Incremental operation

Call handshake/read/write/shutdown once per step. On `NEED_READ`, `NEED_WRITE` or `NEED_READ_WRITE`, inspect interest and use bounded `pst_connection_wait` or the application's event loop before retrying. Honor partial byte counts. Query Peer Info only after establishment.

Shutdown is complete only when the provider reports `PST_OPERATION_COMPLETE`; local close_notify emission alone is not enough. Established EOF/reset without peer close_notify is `PST_RESULT_TRUNCATED`, even if data was read first. Always release connection, credentials, trust and runtime through PST functions.

## Provider limits

OpenSSL implements CLIENT/SERVER TLS 1.2/1.3 and complete SERVER ALPN. Schannel SERVER is TLS 1.2 on the validated Windows 10 build 19045 environment and does not advertise complete SERVER ALPN there. NSS implements CLIENT/SERVER TLS 1.2/1.3 and is validated on real NT4 SP6 x86, but does not advertise SERVER SYSTEM_TRUST or complete SERVER ALPN. Always query role-scoped capabilities rather than assuming symmetry.

Continue with [API 2.0](../api-2.0.md), [SPI 3.0](../provider-spi-3.0.md), [migration](../api-1.3-to-2.0-migration.md), [security](../security-and-limitations.md) and [consumer linking](../consumer-linking.md).
