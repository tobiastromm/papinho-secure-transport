---
title: "Failure-aware native-close lifecycle"
status: accepted
date: 2026-10-10
decision-makers: [Tobias Tromm]
scope: project
---

# ADR-0008: Failure-aware native-close lifecycle

## Status and authority

This ADR is **accepted** by owner decision dated 2026-10-10. It governs the
API 3.0/SPI 4.0 implementation on the 0.6.5 candidate line. Package publication
remains separately unauthorized.

## Context

SPI 3.0 exposes `void connection_destroy`. API 2.2 calls it and frees transport
and connection unconditionally (`src/pst_runtime.c:151-152`;
`src/pst_backend.h:45-47`). OpenSSL and Schannel free provider state before
`closesocket` and discard its result (`src/backends/openssl/pst_backend_openssl.c:99`;
`src/backends/schannel/pst_backend_schannel.c:237`). NSS discards `PR_Close`
and clears the descriptor (`src/backends/nss/pst_backend_nss.c:625-638`).

Winsock documents retained ownership only for specific nonblocking positive-
linger `WSAEWOULDBLOCK`, plus handle reuse and concurrency constraints. This
does not authorize generic retry. Governed NSPR source shows `ssl_DefClose`
destroys the SSL layer before closing the lower `PRFileDesc`; `SocketClose`
preserves the lower descriptor when native close fails. The original SSL
descriptor cannot be assumed intact.

Published v0.6.4 remains unchanged. The 0.6.5/API 2.2/SPI 3 baseline remains
an unpublished candidate.

## Decision drivers

- no false success while a PST-owned native resource is unclosed;
- no premature free, double close, stale-handle close, UAF or resurrection;
- provider-exclusive authority over provider/native close mechanics;
- truthful, observable retention when close cannot be proven;
- no consumer native-error decoding or post-binding fallback;
- no arbitrary registry or unproven hard bound;
- C89/VC6/NT4, x64 and `/ML`/`/MD` feasibility.

## Considered alternatives

1. Void destroy or only normalize linger: rejected; residual failures remain.
2. Consumer-owned close: rejected; breaks transferred ownership/NSS layering.
3. Public retry plus manager: deferred; no portable progress authority exists.
4. **Model E:** provider close policy behind a mandatory failure-aware boundary
   with terminal retained containment: proposed.

## Proposed decision: Model E

PST/core/provider invokes and classifies native close, preserves ownership on
unconfirmed close, distinguishes preconditions, destroys only after confirmed
close/no resource, prevents double close/UAF/resurrection and exposes portable
facts.

Consumers unbind wait-set, invoke one release operation, discard the reference
only after `DESTROYED`/`PST_RESULT_OK`, retain it otherwise, and own admission,
logging, isolation and restart. They never call or interpret `closesocket`,
`PR_Close`, linger or provider retry. Retention is not a resource bound.

## Public API 3.0 candidate

API 3 removes `pst_connection_release(void)` and exposes:

```c
PST_API PST_RESULT PST_CALL pst_connection_try_release(
    pst_connection *connection, PST_RELEASE_INFO *release_info);
PST_API PST_RESULT PST_CALL pst_connection_get_release_info(
    const pst_connection *connection, PST_RELEASE_INFO *release_info);
PST_API PST_RESULT PST_CALL pst_release_info_init(
    PST_RELEASE_INFO *release_info, pst_u32 struct_size);
```

No production symbol changes are made by this ADR.

### Public dispositions

- `NONE`: no release disposition has been established.
- `PRECONDITION_RETAINED`: no native close was attempted; caller corrects the
  precondition while retaining the same reference.
- `DESTROYED`: close was confirmed or no native resource existed; final
  destruction completed and the pointer is invalid.
- `CLOSE_UNRESOLVED_RETAINED`: close was attempted or failed without a proven
  safe resolution; the same reference remains owned and query-only.

The last name describes the supported contract, not physical impossibility.
`RETRYABLE_RETAINED` is absent initially. Even `WSAEWOULDBLOCK` does not define
a safe portable retry schedule by itself.

`PST_RESULT_OK` is exclusive to `DESTROYED`. `INVALID_STATE` accompanies
`PRECONDITION_RETAINED`; `RESOURCE_FAILURE` accompanies
`CLOSE_UNRESOLVED_RETAINED`. Function and record results match.

### Minimal V1 layout

The prior 48/64-byte candidates had speculative or duplicate fields. V1 is 28
bytes with explicit 32-bit types:

| Offset | Field | Type |
| ---: | --- | --- |
| 0 | `struct_size` | `pst_u32` |
| 4 | `api_version` | `pst_u32` |
| 8 | `disposition` | `pst_u32` |
| 12 | `result` | `pst_i32` |
| 16 | `native_domain` | `pst_u32` |
| 20 | `native_code` | `pst_i32` |
| 24 | `native_code_valid` | `pst_u32` |

`PST_RELEASE_INFO_V1_MIN_SIZE` is literal `28UL`, never `sizeof`.

Generation fields are removed: no implemented source, uniqueness scope or
consumer purpose exists. `terminal` duplicates disposition. `flags` and
`reserved0` lack accepted semantics. Oversized caller tails permit future
minor extensions without speculative V1 fields.

Init rejects NULL/size below 28, zeros only the known prefix, records API 3.0
and preserves an oversized tail. Operations reject NULL, short records and
incompatible API major before close or output mutation. The result is fully
written before free; no post-free dereference occurs. Stale pointers cannot be
validated. Required future probes cover VC6 `/ML`, VC6 `/MD` and x64 offsets,
size, signed result and oversized-tail preservation.

## SPI 4.0 candidate

SPI 4 requires `connection_try_close` before final destroy. It returns exactly:

- `NO_NATIVE_RESOURCE`;
- `CLOSED_CONFIRMED`;
- `CLOSE_UNRESOLVED_RETAINED`, with native domain/code when factual.

Only the first two permit `connection_destroy`, transport-wrapper destruction
and core free. `connection_destroy` is then infallible final cleanup. SPI 3
providers are incompatible; there is no optional hook or silent fallback.

OpenSSL and Schannel attempt native close before freeing state needed for proof
or containment. Success permits destroy. `WSAEWOULDBLOCK`, `WSAENOTSOCK` and
ambiguous errors remain unresolved in the initial contract; none authorizes
retry. PST does not silently change linger or TLS `close_notify` behavior.

NSS retains `PR_Close` as sole close root. Governed source proves that the SSL
layer may already be destroyed when the lower close fails. The provider must
retain loaded NSPR runtime, ownership and diagnostics, record the remaining
descriptor identity as unknown when exact identity is unproven, and discard
the former SSL pointer as unusable. It must not call `PR_Close` again without
separately approved source/runtime proof. Combined delegates only to the bound
provider.

## Terminal retained state

`CLOSE_UNRESOLVED_RETAINED` permits no handshake, read, write, shutdown,
wait-set registration, retry, fallback, provider switch or resurrection.
Read-only release-info query remains available. No late evidence changes the
state in this initial contract.

The conservative first implementation retains the core connection, runtime,
provider connection, consumed transport, native/lower descriptor identity,
diagnostic and all provider-referenced state. Provider-specific partial free
requires separate proof; a retained pointer alone does not prove an intact
resource. This policy favors verifiability over speculative memory reduction.

No force-destroy API, global registry, process-wide counter, hard bound, magic
64 or public retry scheduler exists. Consumers may stop admission and choose
isolation/process replacement. Process exit is OS containment, not confirmed
close, exactly-once proof or leak-free cleanup.

The runtime remains alive while retained children exist. Runtime release marks
release requested but cannot unload providers/modules until all children are
destroyed; unresolved children therefore retain the runtime until process
containment. Consumers observe the condition through release info and
normalized logging, without native-provider knowledge.

## Compatibility and migration

API 3 is deliberately incompatible with API 2.x; SPI 4 is deliberately
incompatible with SPI 3. Migration includes all in-tree release callers,
examples, eight package-only CLIENT/SERVER consumers, scheduler consumer,
OpenSSL/Schannel/NSS/Combined tests, Browser and Accelerator. Consumers use the
same release semantics but may select different operational policies.

Certification order after separate implementation authorization:

1. PST unit/integration/negative/ABI/provider qualification;
2. Accelerator P11/P12 and affected gates;
3. Browser x Accelerator Mode B/A-E;
4. real NT4 and clean-machine `/ML`/`/MD`;
5. final PST release decision.

## Verification gates

- success/no-resource destroys exactly once;
- wait-set bound yields PRECONDITION_RETAINED without close attempt;
- every native failure becomes CLOSE_UNRESOLVED_RETAINED, never false OK;
- no repeated close, I/O, resurrection, provider switch or late callback;
- Winsock errors and numeric handle reuse remain diagnostic, not authority;
- NSS lower-layer identity and SSL-layer destruction are instrumented;
- retained runtime/provider modules remain loaded;
- API init/short/oversized/major/offset probes pass on VC6 `/ML`, `/MD`, x64;
- consumers retain on non-OK and never implement native close;
- close_notify/truncation and ADR-0007 provenance remain unchanged.

## Consequences and non-goals

The model truthfully permits unresolved resource retention. It does not promise
a leak-free process after an unresolvable native failure. It is smaller than a
retry/quarantine manager because it publishes only behavior currently
verifiable cross-provider.

No implementation, header/SPI edit, package generation, consumer change,
commit, push, tag or publication is authorized by this proposed ADR.

## Revision history

| Date | Status | Description |
| --- | --- | --- |
| 2026-10-10 | proposed | Initial failure-aware native-close proposal. |
| 2026-10-10 | proposed | Explored API 3/SPI 4, retry and bounded quarantine candidates. |
| 2026-10-10 | proposed | Found per-runtime 64 bypassable across runtimes. |
| 2026-10-10 | proposed | Replaced generalized retry/limit design with final Model E candidate and minimal 28-byte release info. |
| 2026-10-10 | accepted | NSS implementation records one-shot close authority and treats every failed layered `PR_Close` as an unknown retained descriptor identity; synthetic layer-boundary tests do not claim real native-failure proof. |
