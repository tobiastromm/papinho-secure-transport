<!-- SPDX-License-Identifier: MPL-2.0 -->

# Public API and ABI baseline

This document keeps the historical ABI work visible while recording the current release contract. The published 0.6.0 baseline is API 2.1.0 / SPI 3.0 / library 0.6.0. The current bugfix track is API **2.1.0**, SPI **3.0**, library **0.6.1**.

API 2.1 is an additive evolution of API 2.0. It preserves the API 2.0 prefixes and adds the scheduler/readiness surface frozen by ADR-0005. M0–M9 found no need for an SPI change.

## What API 2.1 adds

The additive surface includes opaque `pst_wait_set` and `pst_external_source` handles, consumer-selected `pst_wait_token`, bounded `PST_WAIT_EVENT` / `PST_WAIT_SET_RESULT` records, wait-set membership operations, finite wait, cross-thread wake, external-source membership, and result-returning `pst_connection_try_release` / `pst_external_source_try_release` lifecycle helpers.

The portable API does not expose `SOCKET`, `HANDLE`, `PRFileDesc`, OpenSSL objects or SSPI handles. Win32 native integration remains behind `papinho_secure_transport_win32.h` and private platform code.

M1 implements portable wait-set membership, stable registration order, bounded enumeration, timeout-zero provider-authoritative polling and persistent terminal visibility. M2 adds borrowed external Win32 socket sources without ownership transfer. M3 adds finite blocking waits and coalescing cross-thread wake without periodic polling or sequential `N * timeout` waiting. M4 confirms that the existing `PST_IO_RESULT.bytes_transferred` contract already expresses bounded partial read/write and backpressure correctly.

## API 2.1 result additions

API 2.1 retains the normalized results 0–15 and adds scheduler/lifecycle results:

| Value | Result |
|---:|---|
| 16 | `PST_RESULT_INSUFFICIENT_CAPACITY` |
| 17 | `PST_RESULT_WAIT_TIMEOUT` |
| 18 | `PST_RESULT_WAIT_WOKEN` |
| 19 | `PST_RESULT_ALREADY_REGISTERED` |
| 20 | `PST_RESULT_NOT_REGISTERED` |
| 21 | `PST_RESULT_CONCURRENT_OPERATION` |

Timeout and wake are deliberately different outcomes. Neither cancels a connection or changes terminal state.

## SNI / peer identity additive contract

API 2.1 separates routing SNI from authenticated Expected Peer Name through the additive tail of `PST_CONNECTION_CONFIG`:

```text
PST_SNI_MODE_COMPAT   = 0
PST_SNI_MODE_DISABLED = 1
PST_SNI_MODE_EXPLICIT = 2
```

`COMPAT = 0` preserves zero-initialized API 2.0 behavior. `PST_CAP_SNI_CONTROL` (`0x00020000`) advertises full independent CLIENT SNI control. OpenSSL and Schannel are FULL; RetroZilla NSS remains PARTIAL because the published NSS snapshot couples hostname/SNI behavior through `SSL_SetURL`. Unsupported combinations are filtered before binding; no post-binding provider fallback is permitted.

`PST_CONNECTION_CONFIG_V2_0_SIZE` is the prefix ending before the SNI tail, and `PST_CONNECTION_CONFIG_MIN_SIZE` remains that API 2.0 prefix. This is the key additive ABI mechanism for zero-initialized/older callers.

## Wait-set ownership and threading

A wait-set references but does not own PST connections. A registered connection must be removed before release. An external source is borrowed: the consumer owns the native resource and PST never calls `accept`, `shutdown` or `closesocket` on it. The same external-source object may belong to only one wait-set at a time; identity is the wrapper object, not the underlying native socket.

One wait may be active per wait-set. `pst_wait_set_wake()` is the cross-thread operation. Membership mutation remains owner-thread-only in API 2.1 and is rejected during an active wait. Operation deadlines remain consumer-owned monotonic policy; a finite wait timeout is only the scheduler's maximum sleep.

## Read/write and shutdown contract

Read and write remain bounded incremental operations. A successful read may return fewer bytes than caller capacity; a successful write may accept fewer bytes than requested. The consumer retains the unsent suffix and owns buffering/framing. `NEED_READ`, `NEED_WRITE` and `NEED_READ_WRITE` are nonterminal provider-authoritative progress states, including cross-direction dependencies.

Shutdown does not imply send-all. The caller completes its retained unsent application remainder before beginning shutdown. Clean close requires reciprocal TLS shutdown where the provider exposes it; raw EOF without authenticated reciprocal close remains `TRUNCATED`, and data delivered before truncation remains valid.

`PST_TLS_POLICY.require_graceful_shutdown` uses the closed `PST_FEATURE_*` vocabulary. `DISABLED` and `OPTIONAL` do not make provider support mandatory; `REQUIRED` adds `PST_CAP_GRACEFUL_SHUTDOWN` to pre-binding eligibility. The field never initiates shutdown, creates a deadline, weakens truncation, or turns connection release into an implicit TLS shutdown. All current CLIENT and SERVER providers advertise the capability from proven reciprocal-shutdown evidence.

M9 corrected two Schannel shutdown implementation defects without changing this public contract: already-observed reciprocal `close_notify` now completes correctly after draining, and already-buffered TLS is processed before requesting another socket read.

## TLS after prior plaintext use

M6 proved that the existing attach/ownership contract supports STARTTLS-style and HTTP CONNECT-style upgrade boundaries without a new API. Before attach acceptance the transport remains consumer-owned; after acceptance it is PST-owned even if TLS later fails. Ownership never rolls back to plaintext and PST does not reconnect.

The consumer must stop at a clean boundary. TLS bytes pre-read by the consumer before attach are intentionally `NOT_SUPPORTED_IN_THIS_SCOPE`.

## Public-header boundary

The intentional consumer headers are:

- `include/papinho_secure_transport.h`: portable, provider-neutral API;
- `include/papinho_secure_transport_win32.h`: optional Win32 adapter surface without provider headers.

Everything under `src/` is private. Dependency headers under `third_party/` are not PST API. Public functions retain `PST_CALL` (`__cdecl` under MSVC), C linkage, and the `pst_` / `PST_` namespace. Canonical builds remain static `.lib` artifacts; a DLL ABI is not claimed merely because `PST_API` supports import/export decoration.

Opaque public types include runtime/configuration/provider-independent TLS objects plus `pst_wait_set` and `pst_external_source`. Consumers never allocate or free their internals through the CRT.

## Capability model

The current known capability mask extends through `PST_CAP_GRACEFUL_SHUTDOWN` and remains 32-bit. Exact role-scoped masks are:

| Provider | Aggregate | CLIENT | SERVER |
|---|---:|---:|---:|
| OpenSSL | `0x00067fff` | `0x00067eb7` | `0x0004777b` |
| Schannel | `0x00067efd` | `0x00067eb5` | `0x00047679` |
| RetroZilla NSS | `0x00047aff` | `0x00047ab7` | `0x0004727b` |

Capabilities are eligibility facts, not aspirations. Missing requirements reject a provider before binding. A failure after binding is terminal for that connection; another provider is not tried.

## Provider and metadata boundary

Provider IDs remain the case-sensitive ASCII identifiers `retrozilla-nss`, `schannel`, and `openssl`. `PST_PROVIDER_INFO` exposes normalized aggregate and role-scoped capabilities without exposing provider-native objects.

Existing provider info, peer info and diagnostics were sufficient for M7's future-safe metadata design; no new metadata API was required. Public metadata includes normalized TLS/provider/authentication facts but not native handles. Wait-set tokens remain consumer scheduler identities, not global connection IDs.

## Configuration snapshots and isolation

Connection configuration is snapshot-oriented. Existing connections retain the identity/trust/configuration with which they were created; new connections can use a rotated configuration without mutating established connections. M7 proved 50 V1/V2 rotation cycles without snapshot mutation.

Core multi-runtime configuration isolation passes. Provider-global limitations remain factual: OpenSSL is classified FULL for the audited runtime-isolation model, Schannel PARTIAL because Windows store/state can be external/process-visible, and RetroZilla NSS has upstream process-global limitations. PST does not overclaim isolation from upstream global state.

## Security and ownership invariants

The following remain ABI/semantic invariants:

```text
CUSTOM_TRUST != SYSTEM_TRUST
no silent trust union
no automatic TLS downgrade
no post-binding provider fallback
terminal states do not resurrect
exactly-one-close for accepted transports
borrowed external sources remain consumer-owned
Expected Peer Name is authentication, not SNI routing
secret-safe structured diagnostics/logging
```

M9's real-process closure recorded `LOG_SECRET_HITS=0`, `PRIVATE_KEY_LOG_HITS=0`, and `APPLICATION_PAYLOAD_LOG_HITS=0`.

## Current validation status

M9 completed the release candidate's cross-provider scheduler/security/stress matrix:

- TLS 1.2: all 9 CLIENT×SERVER provider pairs PASS;
- TLS 1.3: all 4 eligible pairs PASS; 5 Schannel-ineligible pairs rejected before binding;
- wait-set, finite wait, wake races, bounded enumeration and external-source scheduling PASS;
- partial I/O/backpressure and hot/slow scheduling PASS with no duplication/loss;
- STARTTLS-style and CONNECT-style same-transport upgrade proofs PASS;
- trust/auth negative matrix, clean shutdown, strict truncation and terminal no-resurrection PASS;
- 250 mixed stress cycles PASS with zero crashes/hangs.

API remains `2.1.0`; SPI remains `3.0`. M10 owns final physical NT4 validation, clean-machine package consumers, deterministic packaging and release verification for the 0.6.0 candidate.

## Historical baselines

The earlier published baselines remain historical facts rather than being rewritten by this document:

- v0.4.0: API 1.3 / library 0.4.0, with the earlier SPI 2.4 release baseline;
- v0.5.0: API 2.0.0 / SPI 3.0 / library 0.5.0, introducing the CLIENT/SERVER contract and published provider SDKs;
- published v0.6.0: API 2.1.0 / SPI 3.0 / library 0.6.0;
- current bugfix track: API 2.1.0 / SPI 3.0 / library 0.6.1.

Detailed historical release evidence remains under `docs/codex/release-evidence/` and the API 2.0 migration documents. M10 will freeze the final 0.6.0 package/ABI evidence rather than retroactively changing the published 0.4.0 or 0.5.0 contracts.
