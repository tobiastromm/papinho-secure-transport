<!-- SPDX-License-Identifier: MPL-2.0 -->

# Providers and selection

SPI 3.0 makes CLIENT and SERVER independent factual capabilities and selects an eligible provider per connection. All three current providers implement both roles, with deliberately asymmetric role masks.

A provider is the engine adapter that implements PST's common secure-transport contract.

| Capability | RetroZilla NSS | Schannel | OpenSSL 3.5.8 |
|---|---:|---:|---:|
| TLS 1.2 | yes | yes | yes |
| TLS 1.3 CLIENT / SERVER | yes / yes | not advertised / not advertised on tested Win10 19045 | yes / yes |
| CUSTOM_TRUST | yes | yes | yes |
| SYSTEM_TRUST CLIENT / SERVER | no / no | yes / yes | yes / yes, through Windows chain evaluation |
| ALPN CLIENT / SERVER | yes / no | yes / no | yes / yes |
| PEER_NAME_VERIFY CLIENT / SERVER | yes / no | yes / no | yes / no |
| full independent SNI control CLIENT / SERVER | no / no | yes / no | yes / no |
| LOCAL_IDENTITY / peer certificate auth / PEER_INFO | yes | yes | yes |
| NONBLOCKING / BACKEND_WAIT | yes | yes | yes |
| reciprocal graceful shutdown | yes | yes | yes |

Exact API 2.1/SPI 3.0 role masks are:

| Provider | Aggregate | CLIENT | SERVER |
|---|---:|---:|---:|
| RetroZilla NSS | `0x00047aff` | `0x00047ab7` | `0x0004727b` |
| Schannel | `0x00067efd` | `0x00067eb5` | `0x00047679` |
| OpenSSL 3.5.8 | `0x00067fff` | `0x00067eb7` | `0x0004777b` |

The aggregate is exactly the union of the two role masks; it is discovery metadata, not permission to combine bits from different roles. `PEER_CERT_AUTH` covers required authentication, `PEER_CERT_OPTIONAL` is additionally required for OPTIONAL, and DISABLED is the baseline policy and needs no capability bit. mTLS is a configuration using the proven local-identity and peer-authentication capabilities, not a separate public bit. `PST_CAP_GRACEFUL_SHUTDOWN` records proven reciprocal shutdown: `PST_FEATURE_REQUIRED` makes it mandatory before binding, whereas OPTIONAL and DISABLED do not. It never initiates shutdown or creates a PST-owned deadline.

Wait-set scheduling, mixed borrowed external sources, wake, bounded partial I/O, TLS attach after a clean plaintext boundary, reciprocal shutdown and strict truncation are proven provider behaviors rather than additional public capability bits. All providers preserve provider-authoritative TLS readiness; RetroZilla NSS specifically retains `PR_Poll` authority after native wake-up. OpenSSL has full per-runtime provider isolation, Schannel has partial isolation because Windows certificate-store adaptation is process-external state with owned cleanup, and the RetroZilla NSS snapshot retains its documented process-global singleton limitation.

NSS targets legacy Win32/NT4 and has a provider-local singleton. Schannel is native Windows; its SERVER intermediate-chain delivery may install only required intermediates in CurrentUser\\CA with shared ownership accounting and normal cleanup. A crash can leave a PST-inserted intermediate because safe stale-entry deletion cannot distinguish every ownership history. OpenSSL is the staged 3.5.8 target with custom trust and Windows SYSTEM_TRUST integration.

OpenSSL and Schannel CLIENT advertise complete independent `SNI_CONTROL`. RetroZilla NSS preserves its narrower historical `SSL_SetURL` COMPAT behavior but does not advertise that bit; EXPLICIT or DISABLED-with-independent-name configurations therefore filter NSS before binding. None of the current SERVER roles advertises CLIENT SNI control.

After public bootstrap, EXACT tries only the named provider. ORDERED considers caller IDs in order. AUTOMATIC walks built-in registration order and filters by requested role and required capabilities before binding. It does not fall back after binding because a handshake, authentication, trust, transport, I/O or shutdown operation failed. In the combined target, Schannel is registered first: eligible TLS1.2 SERVER without ALPN may select Schannel, while SERVER ALPN or TLS1.3 skips it before binding and selects OpenSSL.

`pst_win32_register_retrozilla_nss()` remains a supported specific compatibility helper; general programs should use `pst_win32_register_builtin_providers()`.

## Current API 2.2 transport-termination contract

The current candidate is library 0.6.5 (0x00000605UL), additive public API 2.2.0 and unchanged provider SPI 3.0. API 2.1 layouts and symbols remain compatible; no SPI vtable hook or layout was added.

API 2.2 adds pst_transport_termination_info_init(PST_TRANSPORT_TERMINATION_INFO *) and pst_connection_copy_transport_termination(const pst_connection *, PST_TRANSPORT_TERMINATION_INFO *). The structure has eight pst_u32 fields in order: struct_size, api_version, valid, termination, operation, native_domain, native_code, native_code_valid. Minimum and actual size are 32 bytes, alignment 4, offsets 0,4,8,12,16,20,24,28. A prefix below 32 or incompatible API major is rejected; a larger same-major record preserves its unknown tail.

valid=0 means no event was observed. valid=1 with UNKNOWN means an event was observed but not classified portably. native_code_valid=0 means unavailable; otherwise native_domain names the provider namespace and native_code is an opaque 32-bit pattern. TLS result, portable FIN/RST/SOCKET_ERROR/UNKNOWN, and optional native evidence are independent. Native evidence is diagnostic, not causal proof or authority for trust, PST_RESULT_TRUNCATED, retry, fallback, framing, shutdown, ownership or selection.

The first event is immutable while the connection exists. close_notify alone does not establish FIN; timeout, wake, local cancel and release fabricate no event. After peer close_notify, read may report CLOSED/CLEAN; later application read/write are invalid while get_interest and reciprocal shutdown remain available until shutdown_step reaches final CLOSED. That intermediate bookkeeping state is private. Fatal/truncated states remain terminal, with exactly-once release and no resurrection.

NSS maps factual NSPR EOF/reset/socket errors, Schannel maps recv zero/WSAECONNRESET/socket errors, and OpenSSL maps unexpected EOF or reliable SSL_ERROR_SYSCALL socket evidence; ambiguity remains UNKNOWN. No provider switches after binding. VC6/NSS /ML and /MD remain distinct; Browser uses /MD and Accelerator the governed x64 OpenSSL target.
