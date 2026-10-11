# Native-close failure-aware design

Status: Model E design aligned with proposed ADR-0008. No production contract
is changed by this document.

## Historical audited baseline before the accepted Model E change

- Branch `fix/idle-connection-waitset-close`.
- HEAD `6cbfeeb2a5d3473d296e9119bd5775ae0333a5e8`.
- At the time of this design audit: API 2.2.0, SPI 3.0, candidate library 0.6.5. The accepted and implemented current candidate is API 3.0.0/SPI 4.0; see ADR-0008 and `provider-spi-4.0.md`.
- Published v0.6.4 remains immutable.

Current `pst_connection_try_release` rejects wait-set membership, calls the
provider's `void connection_destroy`, destroys the consumed transport and
frees the connection (`src/pst_runtime.c:151`). It cannot represent a failed
native close.

## Selected Model E

PST/provider owns invocation, classification and containment of native close.
Consumers own only the opaque reference and operational policy. The initial
contract supports no native-close retry: unresolved outcomes become terminal
retained objects that remain queryable.

Public dispositions:

```text
NONE
PRECONDITION_RETAINED
DESTROYED
CLOSE_UNRESOLVED_RETAINED
```

`OK` means DESTROYED and invalid pointer. Every non-OK preserves the same
caller-owned reference. PRECONDITION permits correcting the precondition;
CLOSE_UNRESOLVED permits query only. There is no I/O, wait, shutdown, retry,
fallback or resurrection after unresolved close.

## Public surface candidate

```c
PST_RESULT pst_connection_try_release(
    pst_connection *, PST_RELEASE_INFO *);
PST_RESULT pst_connection_get_release_info(
    const pst_connection *, PST_RELEASE_INFO *);
PST_RESULT pst_release_info_init(PST_RELEASE_INFO *, pst_u32 struct_size);
```

API 3 removes `pst_connection_release(void)`. V1 release info is 28 bytes:

```text
0  struct_size          pst_u32
4  api_version          pst_u32
8  disposition          pst_u32
12 result               pst_i32
16 native_domain        pst_u32
20 native_code          pst_i32
24 native_code_valid    pst_u32
```

The fixed minimum is literal `28UL`. Generation, flags, terminal and reserved
fields were removed: there is no implemented generation authority; terminal
duplicates disposition; the other fields have no accepted semantics.
Oversized tails provide future extensibility.

Init and connection calls reject NULL/short/incompatible-major inputs without
attempting close or mutating output. Init zeros only 28 known bytes and
preserves an oversized tail. A valid result is fully written before object
free. Stale pointers remain outside detectable C API behavior.

## SPI 4 boundary

Every transport-owning provider implements mandatory `connection_try_close`:

```text
NO_NATIVE_RESOURCE
CLOSED_CONFIRMED
CLOSE_UNRESOLVED_RETAINED
```

Only the first two permit final `connection_destroy`. SPI 3 vtables are
incompatible and cannot silently fallback. Combined delegates to the bound
provider only.

## Provider policy

OpenSSL and Schannel attempt native close before freeing state needed for
containment. Native success permits final cleanup. `WSAEWOULDBLOCK`,
`WSAENOTSOCK` and ambiguous errors are retained-unresolved; no generic retry
is exposed and linger is not silently changed.

NSS continues to use `PR_Close`. Governed source proves `ssl_DefClose` removes
the SSL layer before lower close. On failure the provider preserves NSPR
runtime/module lifetime, wrapper ownership and diagnostics, but records the
surviving descriptor identity as unknown. It never assumes the original SSL
layer remains, fabricates a usable lower pointer or repeats `PR_Close`.

The NSS implementation consumes a provider-private one-shot close authority
before calling `PR_Close`. It removes the former aggregate descriptor from all
usable provider state before the call. Success records confirmed destruction;
failure records an immutable native-error snapshot and an unknown retained
layer identity, because the governed source proves layered transformation but
the distributed binary has only Level-B source correspondence. The former SSL
pointer is never dereferenced or retried. Synthetic pre-close, post-pop,
lower-close and ambiguous-failure tests qualify this private state machine;
they are not evidence of a real NSPR/Winsock close failure.

The conservative implementation retains all provider-referenced state. Safe
partial free may be optimized only after provider-specific evidence and tests.

## Runtime and consumer behavior

A retained child keeps runtime/provider modules alive. Runtime release becomes
pending until children are destroyed; unresolved children can therefore live
until controlled process containment. Process exit is not successful close.

Consumer pattern:

```text
remove from wait-set
try_release(connection, info)
if OK: clear pointer
else: retain pointer, log portable info, apply admission/isolation policy
```

Browser and Accelerator use this same mechanic. Neither invokes or interprets
native close. Different restart thresholds are legitimate application policy.

No registry, hard process-wide bound, counter contract, limit 64, force-close
or retry scheduler exists initially. A pointer-to-pointer helper may reduce
lost-reference misuse but cannot make incorrect C code impossible.

## Future validation

- success and no-resource paths destroy exactly once;
- wait-set precondition performs no native close;
- every unresolved native failure retains and reports truthfully;
- no repeated close, numeric-handle authority, UAF, callback hazard or revival;
- NSS transformed lower descriptor is instrumented and retained;
- runtime/provider modules remain live with retained children;
- TLS close_notify/truncation semantics remain unchanged;
- Combined never falls back;
- API layout and tail probes pass VC6 `/ML`, `/MD` and x64;
- all tests/examples/package consumers migrate to retain-on-non-OK;
- Accelerator P11/P12, Browser x Accelerator Mode B/A-E, NT4 and clean-machine
  recertification follow internal PST qualification.

Synthetic pre-close faults prove state-machine behavior only. They do not
prove real Winsock/NSPR failure semantics.

## Deferred decisions

A future retry contract requires provider-owned same-lifetime progress
authority and a provider-neutral event mechanism. A future hard process bound
requires admission reservations for every connection and process-wide
synchronization. Neither belongs to the initial Model E contract.
