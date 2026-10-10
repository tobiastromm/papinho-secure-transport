<!-- SPDX-License-Identifier: MPL-2.0 -->

# CLIENT/SERVER lifecycle contract

CLIENT flow: create runtime context, construct complete CLIENT configuration, create and bind a provider, create/connect a transport, attach with explicit ownership, drive handshake/wait, perform I/O, inspect peer, drive shutdown, release.

SERVER flow: application creates listener, bind/listen/accepts, constructs complete SERVER configuration, creates and binds a provider connection, wraps only the accepted connected transport, attaches ownership, drives the same handshake/wait/I/O/shutdown calls, then releases. PST never owns the listener, accept loop, admission, backlog or scheduling.

Selection is complete before transport binding. Ineligible candidates may be skipped only during selection. Once selected, provider identity is immutable and every protocol/authentication/policy failure is terminal for that connection.

The runtime may be logically shared by connections, but API 2.0 does not guarantee concurrent access from multiple threads. Each connection has independent lifecycle and provider-private state. Runtime destruction is guarded until children are gone; provider states are released in reverse successful-initialization order.

Ownership and readiness retain the 0.4.0 invariants: exactly one native close root after acceptance; native readiness differs from TLS readiness; all steps and waits are bounded; release never performs a hidden peer wait.

SS-3 validates this flow with the OpenSSL provider. The application retains the listener throughout. Before successful attach it owns the accepted socket; after `ownership_accepted=1`, the OpenSSL connection is the sole native close root. Successful reciprocal shutdown, handshake failure after acceptance, and truncated peer release all preserve that rule. Selection is pinned before attach, including Combined ORDERED/AUTOMATIC, and is never retried after a bound-provider failure.

## Current API 3.0 / SPI 4.0 Model E contract

The current candidate is library 0.6.5 (0x00000605UL), public API 3.0.0 and provider SPI 4.0. API 3 removes the legacy void connection release and requires `pst_connection_try_release` with the 28-byte `PST_RELEASE_INFO` Model E disposition contract. SPI 4 requires failure-aware `connection_try_close` before infallible final destruction.

The preceding API 2.2 candidate added `pst_transport_termination_info_init(PST_TRANSPORT_TERMINATION_INFO *)` and `pst_connection_copy_transport_termination(const pst_connection *, PST_TRANSPORT_TERMINATION_INFO *)`; those immutable transport-termination facts remain available in API 3. `PST_TRANSPORT_TERMINATION_INFO` remains 32 bytes, while the independent Model E `PST_RELEASE_INFO` V1 contract is exactly 28 bytes.

valid=0 means no event was observed. valid=1 with UNKNOWN means an event was observed but not classified portably. native_code_valid=0 means unavailable; otherwise native_domain names the provider namespace and native_code is an opaque 32-bit pattern. TLS result, portable FIN/RST/SOCKET_ERROR/UNKNOWN, and optional native evidence are independent. Native evidence is diagnostic, not causal proof or authority for trust, PST_RESULT_TRUNCATED, retry, fallback, framing, shutdown, ownership or selection.

The first event is immutable while the connection exists. close_notify alone does not establish FIN; timeout, wake, local cancel and release fabricate no event. After peer close_notify, read may report CLOSED/CLEAN; later application read/write are invalid while get_interest and reciprocal shutdown remain available until shutdown_step reaches final CLOSED. That intermediate bookkeeping state is private. Fatal/truncated states remain terminal, with exactly-once release and no resurrection.

NSS maps factual NSPR EOF/reset/socket errors, Schannel maps recv zero/WSAECONNRESET/socket errors, and OpenSSL maps unexpected EOF or reliable SSL_ERROR_SYSCALL socket evidence; ambiguity remains UNKNOWN. No provider switches after binding. VC6/NSS /ML and /MD remain distinct; Browser uses /MD and Accelerator the governed x64 OpenSSL target.
