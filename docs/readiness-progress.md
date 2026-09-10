<!-- SPDX-License-Identifier: MPL-2.0 -->

# Readiness and progress audit

The historical Phase 7.C work established the provider-neutral rule that **readiness is not progress**. API 2.0/SPI 3.0 made that rule role-neutral for CLIENT and SERVER. The current API 2.1/library 0.6 release track builds a multiplexed scheduler on top of that same contract rather than replacing it.

Current release versions: API `2.1.0`, SPI `3.0`, library `0.6.0`.

## M1 — portable wait-set core

M1 adds opaque `pst_wait_set` membership for PST connections. Consumer-selected tokens remain stable while registered and expose no connection/provider/native pointer. A connection may belong to at most one wait-set. Duplicate connection/token registration is rejected; remove/re-add creates a new registration position.

`pst_wait_set_wait(..., 0, ...)` is a bounded nonblocking poll. PST asks each provider through the existing provider-authoritative readiness path; raw socket readiness does not replace TLS readiness. RetroZilla NSS therefore continues to use NSPR `PR_Poll`. Capacity exhaustion reports exact ready/event counts and does not consume readiness. Terminal members remain visible until explicitly removed.

A registered connection must be removed before release. The wait-set references rather than owns the connection.

## M2 — external Win32 sources

M2 adds borrowed external-source membership through the Win32 adapter. The portable core stores only an opaque source, portable READ/WRITE interests and the consumer token. Native `SOCKET`, `fd_set`, `timeval`, `INVALID_SOCKET` and `select()` remain private to the Win32 platform implementation.

The consumer owns the native resource. PST does not `accept`, `shutdown` or `closesocket` it. One `pst_external_source` object may belong to at most one wait-set at a time. Identity is the wrapper object, not the native socket; two distinct wrappers around one socket are not deduplicated.

The chosen surface is compatible with the VC6/NT4 target: the audited i386 artifact uses OS/subsystem 4.00 and `WSOCK32.dll` with Winsock 1.1-era mechanisms.

## M3 — wake and finite blocking scheduler

M3 completes finite `timeout_ms > 0` waits and cross-thread `pst_wait_set_wake()`.

The Win32 adapter uses a private nonblocking loopback socket pair as the wake source. Its read side participates in the same bounded `select()` as external sockets and non-owning native hints for attached PST transports. Wake is thread-safe, coalescing, reusable, nonterminal and does not cancel connections or change membership.

Before blocking and after native signalling, PST performs bounded timeout-zero provider confirmation in stable registration order. Raw socket readiness is only a hint. If a provider rejects a hinted interest bit, that bit can be suppressed for the remainder of the current application wait while the monotonic timeout continues. This prevents writable-socket spin without creating sequential `N * timeout` waits.

There is no hidden worker and no mandatory periodic polling loop. A positive timeout is only the scheduler's maximum wait; application handshake/read/write/shutdown deadlines remain consumer-owned monotonic policy.

One wait may be active per wait-set. Wake may come from another thread. Membership changes are owner-thread operations and are rejected during an active wait. Destroy is rejected during a wait or while members remain.

M3 proved an external listener plus multiple PST connections, wake before/during wait, wake/timeout distinction, stable multiple-ready enumeration, bounded provider confirmation and 100-cycle stress without leaks or busy-loop behavior. This resolved the PapinhoAccelerator Phase 3.B4 architectural blocker.

## M4 — partial I/O and backpressure

Read/write remain single bounded progress attempts. A successful read may deliver fewer bytes than caller capacity; a successful write may accept fewer bytes than requested. The application owns buffering and retains the unsent suffix. PST promises neither send-all nor read-until-full.

`NEED_READ`, `NEED_WRITE` and `NEED_READ_WRITE` are normal nonterminal states, including cross-direction cases where a read needs transport write readiness or a write needs transport read readiness. The provider's interest remains authoritative. After readiness the consumer performs bounded work and returns to the scheduler rather than draining a hot connection indefinitely.

Deterministic tests forced writes to 7-byte chunks and reads to 5-byte chunks, exercised `write -> NEED_READ` and `read -> NEED_WRITE`, and compared a 257-byte stream byte-for-byte. Real OpenSSL transferred 4 MiB under forced backpressure; Schannel and NSS used 3-byte server fragmentation. All providers preserved exact data and reciprocal shutdown. Hot/slow multi-connection tests proved that a continuously ready connection does not hide another ready member. Scheduling fairness itself remains application policy.

Shutdown is not an implicit flush/send-all operation. The caller completes its retained unsent application remainder before beginning shutdown. Data already delivered before a later truncation remains valid.

## M6 — TLS after prior plaintext use

M6 proved that the same connected transport can be used by the consumer for plaintext protocol setup, transferred to PST at an exact boundary, and then used for TLS without reconnecting.

The generic STARTTLS-style and CONNECT-style fixtures deliberately contain no SMTP, IMAP or HTTP parser inside PST. Fragmented plaintext boundaries, exact boundary consumption, wait-set use after attach, custom trust, mTLS where applicable, encrypted I/O and reciprocal shutdown passed across OpenSSL, Schannel and RetroZilla NSS.

Ownership is explicit: before attach acceptance the consumer owns the transport; after acceptance PST owns it even if TLS subsequently fails. There is no ownership rollback and no plaintext fallback. TLS bytes pre-read by the consumer before attach remain intentionally unsupported in this scope; the negative test terminates as protocol failure rather than attempting unsafe recovery.

This establishes the PST foundation for PapinhoLegacyMail STARTTLS and PapinhoBrowser HTTP CONNECT-to-origin TLS.

## M7/M8 — future safety and provider consolidation

M7 proved core multi-runtime/configuration snapshot isolation and immutable V1/V2 identity rotation without changing production/API/SPI. Existing connections keep their old snapshot while new connections can use a rotated identity/trust configuration. Provider-global limitations remain documented rather than hidden: OpenSSL FULL for the audited model, Schannel PARTIAL, RetroZilla NSS with factual upstream process-global limitations.

M8 consolidated exact role-scoped provider capability masks and reran representative wait-set, partial-I/O, TLS-after-plaintext, shutdown/truncation, ownership and diagnostic gates. Unsupported capability combinations are rejected before binding; no provider switch occurs after binding.

## M9 — cross-provider scheduler/security/stress closure

M9 combines the previously isolated guarantees. TLS 1.2 passed for all 9 CLIENT×SERVER provider pairs. TLS 1.3 passed for all 4 eligible pairs; the 5 combinations involving an ineligible Schannel role were classified before binding rather than attempted and downgraded.

The deterministic scheduler matrix passed finite wait, wake races, timeout/wake distinction, stable registration order, bounded enumeration, membership/lifetime negatives, hot/slow backpressure and failure injection. Combined selection reconfirmed AUTOMATIC, EXACT, ORDERED and connection-level pinning with zero post-binding provider switches.

A 250-cycle mixed stress campaign alternated VC6/NSS, OpenSSL, Schannel and Combined security/lifecycle matrices with zero crashes and zero hangs. Real-process closure also passed trust/authentication negatives, reciprocal clean shutdown, raw-EOF truncation, preservation of data before truncation, terminal no-resurrection and secret-safe diagnostics/logging.

M9 found two real Schannel shutdown defects and fixed them without API/SPI changes:

1. when the reciprocal peer `close_notify` had already been observed, shutdown now completes after draining that alert instead of waiting for a condition already satisfied;
2. Schannel now processes TLS data already buffered by SSPI before requesting another socket read.

The Schannel, Combined and full relevant regressions pass after these corrections.

## Readiness is not progress

Readiness says that a backend may be attempted without an ordinary blocking wait. It does **not** promise that the current TLS operation will consume/produce application bytes, complete, change interest or enter a terminal state.

For application READ, READ is the primary direction and WRITE can be auxiliary. For application WRITE, WRITE is primary and READ can be auxiliary. Handshake has no fixed primary direction because TLS may alternate both. Provider interest and confirmation remain authoritative.

The core's no-progress guard is private per connection. Repeating the same auxiliary-ready, zero-byte, same-operation, same-interest state is not useful progress. Temporary suppression is reset by positive byte progress, operation/interest transition, COMPLETE/CLOSED/FAILED, provider failure or normal timeout. No global/thread-local suppression state is used.

## Operation/interest summary

| Operation | Interest / readiness | Required behavior |
|---|---|---|
| HANDSHAKE | READ, WRITE or both | retry only after provider-confirmed readiness; either direction may be legitimate |
| READ | READ primary; WRITE may be auxiliary | bounded read attempt; partial bytes are progress |
| WRITE | WRITE primary; READ may be auxiliary | bounded write attempt; caller retains unsent suffix |
| SHUTDOWN | provider-specific incremental interest | reciprocal TLS close required for CLEAN where provider exposes it; raw EOF remains TRUNCATED |
| Any pending operation | timeout | normal bounded scheduler result; not failure/cancellation |
| Any operation | fatal provider/readiness error | terminal FAILED; no resurrection |

RetroZilla NSS continues to map readiness through `PR_Poll`; raw HUP is not itself authenticated TLS close. Fatal ERR/NVAL takes precedence over useful readiness bits. OpenSSL and Schannel likewise retain provider-authoritative TLS progression behind the common scheduler.

## Current closure and next validation

M0–M9 are complete on `feature/multiplexed-readiness`. API remains `2.1.0`, SPI remains `3.0`, library track remains `0.6.0`.

The scheduler architecture is complete. M10 performs final physical NT4 validation for the 0.6.0 candidate, separate clean-machine/package-only validation, deterministic packaging and publication verification.
