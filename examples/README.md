<!-- SPDX-License-Identifier: MPL-2.0 -->

# Public API examples

These sources include only public PST headers. They are intentionally small building blocks; replace fixture paths, addresses and certificates with deployment-controlled values.

- `basic_client.c`: bootstrap, runtime/config, transport ownership, bounded handshake and shutdown helpers.
- `basic_server.c`: SERVER configuration for an application-owned listener and an accepted transport.
- `custom_trust.c`: explicit CA DER with no system fallback.
- `system_trust.c`: SYSTEM_TRUST capability selection.
- `mtls.c`: explicit client certificate DER and PKCS#8 identity.
- `provider_selection.c`: EXACT, ORDERED and AUTOMATIC policies.
- `diagnostics_logging.c`: copied diagnostics and synchronous structured logging.

Build verification compiles them with `/W4`. The basic helpers are C89-compatible; modern SYSTEM_TRUST examples require a capable Schannel/OpenSSL target. Examples are educational and remain separate from the completed cross-machine release-validation kit.

## Current API 2.2 transport-termination contract

The current candidate is library 0.6.5 (0x00000605UL), additive public API 2.2.0 and unchanged provider SPI 3.0. API 2.1 layouts and symbols remain compatible; no SPI vtable hook or layout was added.

API 2.2 adds pst_transport_termination_info_init(PST_TRANSPORT_TERMINATION_INFO *) and pst_connection_copy_transport_termination(const pst_connection *, PST_TRANSPORT_TERMINATION_INFO *). The structure has eight pst_u32 fields in order: struct_size, api_version, valid, termination, operation, native_domain, native_code, native_code_valid. Minimum and actual size are 32 bytes, alignment 4, offsets 0,4,8,12,16,20,24,28. A prefix below 32 or incompatible API major is rejected; a larger same-major record preserves its unknown tail.

valid=0 means no event was observed. valid=1 with UNKNOWN means an event was observed but not classified portably. native_code_valid=0 means unavailable; otherwise native_domain names the provider namespace and native_code is an opaque 32-bit pattern. TLS result, portable FIN/RST/SOCKET_ERROR/UNKNOWN, and optional native evidence are independent. Native evidence is diagnostic, not causal proof or authority for trust, PST_RESULT_TRUNCATED, retry, fallback, framing, shutdown, ownership or selection.

The first event is immutable while the connection exists. close_notify alone does not establish FIN; timeout, wake, local cancel and release fabricate no event. After peer close_notify, read may report CLOSED/CLEAN; later application read/write are invalid while get_interest and reciprocal shutdown remain available until shutdown_step reaches final CLOSED. That intermediate bookkeeping state is private. Fatal/truncated states remain terminal, with exactly-once release and no resurrection.

NSS maps factual NSPR EOF/reset/socket errors, Schannel maps recv zero/WSAECONNRESET/socket errors, and OpenSSL maps unexpected EOF or reliable SSL_ERROR_SYSCALL socket evidence; ambiguity remains UNKNOWN. No provider switches after binding. VC6/NSS /ML and /MD remain distinct; Browser uses /MD and Accelerator the governed x64 OpenSSL target.
