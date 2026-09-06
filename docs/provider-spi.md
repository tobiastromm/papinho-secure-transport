<!-- SPDX-License-Identifier: MPL-2.0 -->

# Provider SPI 2.4 contract

This document freezes the in-process contract between the PapinhoSecureTransport core and independently implemented TLS providers. It is normative for SPI 2.4. It is not a public application API, a DLL plugin ABI, or a claim that the same SPI can support DTLS, QUIC, Noise, or other future transports unchanged.

## Boundaries and compatibility

Applications include only `include/papinho_secure_transport.h`. Provider authors use `src/pst_backend.h`, the provider-neutral SPI header, plus core-private snapshot/config declarations currently needed by its signatures. `src/pst_transport_internal.h` is the internal native-transport envelope; its WIN32_SOCKET kind is platform-specific but not provider-specific. `src/pst_internal.h`, `src/pst_identity_internal.h`, and diagnostic/log helpers are core-private. Files below `src/backends/<provider>` are provider-private; their platform subdirectories are platform-private. NSS/NSPR, Schannel/SSPI, OpenSSL, WinSock, and CryptoAPI headers never become SPI vocabulary. A provider must not include another provider's internals. A later provider SDK may publish the provider-neutral subset, but Phase 9.C does not package one.

SPI version is `0x00020004` (major 2, minor 4). The high 16 bits are the compatibility boundary: core accepts the same major, including a later minor whose advertised prefixes remain usable, and rejects another major. This is not a promise to accept arbitrary future majors. Structures start with `struct_size` and a version, existing fields retain order/type/meaning, and extensions append. A consumer reads an appended field only when `struct_size >= offsetof(field) + sizeof(field)`. Unknown tails are ignored after known-prefix validation. Security-affecting public flags and enum values remain fail-closed unless their contract explicitly defines otherwise; unknown capability bits are merely unconsumed provider claims and cannot satisfy a known requirement.

No non-default packing is used. Fixed protocol words are `pst_u32` (32 bits); pointers and `pst_size` are target width. Provider-owned allocation must be freed by the same provider/module. The SPI never transfers heap blocks for another CRT to free.

## Descriptor baseline

`PST_BACKEND_DESCRIPTOR` fields are frozen in this order:

| Field | Type | Rule |
| --- | --- | --- |
| `struct_size` | `pst_u32` | available prefix size |
| `spi_version` | `pst_u32` | provider SPI version |
| `id` | `const char *` | immutable setup-lifetime stable ID |
| `name` | `const char *` | informational name |
| `capabilities` | `pst_u32` | static honest capability mask |
| `vtable` | `const PST_BACKEND_VTABLE *` | immutable dispatch table |
| `metadata` | `const PST_BACKEND_METADATA *` | optional appended SPI 2.4 metadata |

The minimum remains `offsetof(PST_BACKEND_DESCRIPTOR, vtable) + sizeof(vtable)`: 24 bytes on x86 and 40 on x64. Full sizes are 28 and 48. Below-minimum is rejected; the exact legacy prefix, current size, and a larger same-major record are accepted. Metadata is never read from a legacy prefix.

IDs are 1–31 ASCII bytes plus terminator, using lowercase letters, digits, `-`, `_`, or `.`. The descriptor and strings remain valid from registration through the last dependent runtime. Stable production IDs are `retrozilla-nss`, `schannel`, and `openssl`; they must not be renamed. Duplicate IDs are rejected. Registration is setup-time only, before concurrent object use; the fixed historical maximum is eight. There is no runtime discovery, unload protocol, or DLL plugin ABI.

## Vtable baseline

The exact hook order is frozen below. The full table is 88 bytes on x86 and 168 on x64; the legacy minimum ends immediately before `connection_configure_identity` (76/144 bytes).

| # | Hook | Status and contract |
| --- | --- | --- |
| 1 | `initialize(void **)` | mandatory; create provider-global state transactionally |
| 2 | `shutdown(void *)` | mandatory; destroy successful/partial global state exactly once |
| 3 | `runtime_create(void *, void **)` | mandatory; create isolated provider runtime |
| 4 | `runtime_destroy(void *)` | mandatory; free that runtime in its own module |
| 5 | `query_capabilities(void *, pst_u32 *)` | mandatory; runtime-effective honest mask |
| 6 | `validate_requirements(void *, pst_u32)` | mandatory; fail before an insecure connection |
| 7 | `connection_create(void *, void **)` | mandatory; transactional private connection |
| 8 | `connection_destroy(void *)` | mandatory; release accepted resources exactly once |
| 9 | `attach_transport(... ownership, *accepted)` | mandatory; explicit ownership boundary |
| 10 | `handshake_step(... *operation, *error)` | mandatory; one bounded incremental step |
| 11 | `get_interest(... *interest)` | mandatory; current TLS-aware readiness interest |
| 12 | `wait(... interest, timeout_ms, *result)` | optional only without BACKEND_WAIT; bounded wait |
| 13 | `read(... capacity, *result)` | mandatory; bounded secure read/partial result |
| 14 | `write(... length, *result)` | mandatory; bounded secure write/application consumption |
| 15 | `shutdown_step(... *operation, *error)` | mandatory; bounded incremental shutdown |
| 16 | `peer_info_create(... **snapshot)` | required with PEER_INFO; normalized copied snapshot |
| 17 | `peer_info_destroy(void *)` | required with PEER_INFO; provider/core-owned matching release |
| 18 | `connection_configure_identity(... config)` | required with CLIENT_AUTH, CUSTOM_TRUST, or HOSTNAME_VERIFY |
| 19 | `connection_get_alpn(... buffer, capacity, *size)` | required with ALPN |
| 20 | `diagnostic_copy(const void *, ...)` | optional, size-gated copied internal snapshot |

Mandatory prefix hooks must be non-NULL. Optional hooks are checked by their own field-end size, never by full-struct size, and never called through NULL. Absence yields safe generic diagnostics or `UNSUPPORTED`/`UNAVAILABLE`. Configuration receives a frozen config; providers copy/import anything needed and do not retain caller pointers beyond guaranteed object lifetime. Failure destroys partial connection state and exposes no connection.

## Capability ABI

| Bit | Meaning |
| --- | --- |
| `0x00000001 TLS_1_2` | TLS 1.2 is usable under PST semantics |
| `0x00000002 TLS_1_3` | TLS 1.3 is usable under PST semantics |
| `0x00000004 CLIENT_AUTH` | explicit configured DER certificate + PKCS#8 client identity |
| `0x00000008 ALPN` | ordered offers, requirement enforcement, selected-protocol query |
| `0x00000010 CUSTOM_TRUST` | only explicit caller trust is used for that mode |
| `0x00000020 SYSTEM_TRUST` | target system trust is used for that mode |
| `0x00000040 HOSTNAME_VERIFY` | expected hostname is verified fail-closed |
| `0x00000080 RESUMPTION` | PST-controlled resumption semantics are implemented |
| `0x00000100 EARLY_DATA` | PST early-data semantics; requires RESUMPTION |
| `0x00000200 PEER_INFO` | normalized peer snapshot is available |
| `0x00000400 NONBLOCKING` | incremental calls contain no hidden unbounded network block |
| `0x00000800 BACKEND_WAIT` | provider maps internal TLS interest to bounded waiting |

Capability is ability, not policy. Consumer requirements must be a subset of runtime-effective capabilities and still pass provider validation. Static/runtime claims must agree honestly. Current masks are NSS `0x00000e5f` (no SYSTEM_TRUST, RESUMPTION, or EARLY_DATA), Schannel `0x00000e7d` (tested TLS 1.2 only; no TLS 1.3, RESUMPTION, or EARLY_DATA), and OpenSSL `0x00000e7f` (TLS 1.2/1.3; no RESUMPTION or EARLY_DATA). Library/OS presence alone never justifies a bit.

TLS min/max are enforced exactly: no widening, clamp, downgrade, trust union, or silent fallback. CUSTOM_TRUST mechanisms may differ; NSS's supported explicit database/import path, Schannel native Windows mechanisms, and OpenSSL's provider-local implementation are not normative. Hostname, ALPN wire encoding, certificate import, cipher choice, and native validation remain provider-private. Peer info contains normalized values and copied bytes, never native handles. Cipher suite is the numeric TLS registry value, not a promise that providers choose the same suite.

## Selection, lifecycle, and ownership

Availability is the compiled and registered set. EXACT performs one stable-ID lookup and never falls back. ORDERED attempts the caller's sequence. AUTOMATIC attempts compatible providers in registration order; the combined modern manifest order is Schannel then OpenSSL. Candidate initialization/runtime-create failure may advance during initial selection, with exact cleanup. Once a runtime is selected, connection, handshake, authentication, I/O, wait, or shutdown failure never reselects. A rejected candidate's diagnostic is cleared if a later candidate succeeds; all-candidate failure retains the final relevant normalized diagnostic. There is no global last error.

Core permits simultaneous and multiple runtimes. A provider-local singleton such as legacy NSS affects only that provider. Successful initialize pairs with shutdown; successful runtime_create with runtime_destroy; successful connection_create with connection_destroy. Partial output never escapes. Current SPI implies no workers or deferred callbacks; providers must not retain callbacks into destroyed state. Registration and registry mutation are not concurrent-runtime operations; no broader thread-safety promise is made.

Before `ownership_accepted != 0`, core/caller owns the native transport and a failed attach must not close it. After acceptance, provider connection destruction is the sole native close root; the wrapper is discarded as consumed and exactly one close occurs. `PST_NATIVE_TRANSPORT` is version `0x00010000`, fields `struct_size, version, kind, native_socket, hostname`, size 20 x86/32 x64. `WIN32_SOCKET` is an internal platform envelope; it carries no backend routing ID. FIONBIO, imports, SSPI, BIO, PR_Poll, select, and handles remain private.

## Progress, I/O, close, and errors

Operations are COMPLETE, NEED_READ, NEED_WRITE, NEED_READ_WRITE, CLOSED, or FAILED. Interests are NONE/READ/WRITE. Readiness means retry may be useful, never completion; TLS-aware interest can differ from raw socket readiness. Handshake, read, write, and shutdown perform bounded work and may require either direction. Internal buffered progress is valid, but repeated no-progress calls obey core/provider anti-spin rules. BACKEND_WAIT consumes a bounded timeout; fatal wait becomes terminal normalized failure.

Reads may return partial plaintext; data before a clean close is delivered first. Writes report bytes consumed from the application exactly once, including partial/backpressure cases. An authenticated `close_notify` becomes CLOSED/CLEAN. Unexpected EOF/reset without authenticated close becomes FAILED/TRUNCATED. Reciprocal shutdown timing remains provider-local and bounded.

Native error domains and sensitive provider strings never cross public diagnostics or logs. Reliable TLS/auth/hostname/policy causes outrank later cleanup EOF; otherwise BACKEND_FAILURE is honest. `diagnostic_copy` copies a provider-neutral snapshot with no native pointer. Core logging is the only public event channel.

## Metadata, evolution, and validation

Metadata version is `0x00010000`; `PST_BACKEND_METADATA` is 180 bytes on both targets, with a fixed inline implementation version, component count up to two, and inline 24-byte name/16-byte qualifier. Availability is explicit. Values describe real adapter/provider provenance, not semantic OS promises. Invalid present metadata rejects registration; a legacy descriptor without the appended pointer remains valid.

The deterministic baseline is `tests/test_provider_spi_abi.c`; it freezes constants, offsets, x86/x64 sizes, version compatibility, descriptor/vtable prefixes, future tails, capability bits, bounded IDs, and the optional diagnostic tail. `test_backend_spi`, `test_multi_backend`, `test_tls_policy`, lifecycle/readiness tests, and provider tests cover selection, cleanup, isolation, ownership, diagnostics, logging, progress, shutdown, and optional hooks. A future provider must additionally prove honest capabilities, independent interoperability, fail-closed policy, lifecycle/ownership, disclosure safety, isolated build, and license/provenance.

Provider-author facts for Phase 9.D: implement the mandatory prefix; append only; advertise only tested capability-gated hooks; use transactional state creation and module-local frees; accept transport ownership explicitly; expose bounded incremental progress/readiness; normalize diagnostics; prove clean/truncated close and failure terminality; ship isolated tests and provenance. The full community guide remains Phase 9.D work.
