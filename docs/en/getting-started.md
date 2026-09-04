# Getting Started

PST secures a byte stream; your application protocol remains yours. A typical client follows this order:

1. Call `pst_win32_register_builtin_providers()` before any runtime creation.
2. Create a runtime with AUTOMATIC, EXACT, or ORDERED selection and required capabilities.
3. Create trust and, for mTLS, credentials; configure hostname and TLS policy; freeze the config.
4. Connect a native TCP socket, wrap it with `pst_win32_socket_transport_create`, create a connection, and attach with `PST_OWNERSHIP_TRANSFERRED`.
5. Drive handshake, read, write, and shutdown incrementally. `NEED_READ`, `NEED_WRITE`, or `NEED_READ_WRITE` means wait boundedly for the reported readiness and retry. Readiness is permission to retry, not proof of progress.
6. Release connection, config inputs, and runtime through PST functions.

A LAN example follows the same security model: a branch workstation can connect to `erp.internal` using an explicit corporate CA through CUSTOM_TRUST, verify that hostname, and exchange the ERP protocol above PST. No public Internet or HTTP dependency is implied.`r`n`r`nNever disable authentication to make a connection pass. CUSTOM_TRUST accepts explicit CA DER and never falls back to system trust. SYSTEM_TRUST requires a provider advertising that capability. Always provide the expected hostname.

The built-in set belongs to the linked target: VC6/NT4 has RetroZilla NSS; modern Schannel has Schannel; modern OpenSSL has OpenSSL; the deliberate combined validation target registers Schannel then OpenSSL. Bootstrap performs no PATH or filesystem discovery.

See [the examples](../../examples/README.md), [providers](../providers.md), and [security and limitations](../security-and-limitations.md).