<!-- SPDX-License-Identifier: MPL-2.0 -->
# Runtime isolation, identity rotation, and connection metadata

This document records the bounded M7 audit. It does not add a profile manager,
credential watcher, renewal service, pooling layer, metrics system, or public native
handle access.

## Runtime and profile isolation

The portable core copies credential and custom-trust bytes into opaque retained
objects. Connection creation then creates and retains an immutable configuration
snapshot containing its role, provider selection, TLS/ALPN/SNI policy, identity, trust,
and expected peer name. Later caller mutation, release, or construction of a replacement
configuration cannot alter that connection. Provider selection and terminal state are
connection-scoped. Releasing one runtime does not invalidate an unrelated runtime; a
runtime with live children defers its own destruction.

CUSTOM_TRUST and SYSTEM_TRUST remain exclusive. PST never unions consumer profiles,
downgrades policy, or switches provider after binding. Independent objects wrapping
the same external material remain independently owned snapshots.

Provider boundaries are factual:

- OpenSSL has the strongest runtime isolation: each PST runtime creates its own
  `OSSL_LIB_CTX` and default provider. OpenSSL process/thread facilities still exist
  upstream, but PST TLS contexts, trust and connections use the runtime library context;
  tests prove simultaneous runtimes, isolated error queues and release isolation.
- Schannel uses independent PST runtime/connection state and per-connection credentials
  and trust. SSPI and Windows certificate stores are operating-system facilities. The
  SERVER chain-delivery adaptation may temporarily reference-count exact intermediates
  in `CurrentUser\CA`; preexisting certificates are never removed, PST-inserted entries
  are removed at the final normal reference, and abrupt-process crash residue remains a
  documented platform limitation. This is partial, not process-global-state-free,
  isolation.
- RetroZilla NSS/NSPR lifecycle and the SERVER session cache are process-global in the
  preserved snapshot. PST deliberately permits only one active NSS backend state.
  Connection configuration snapshots remain isolated, but simultaneous independent NSS
  runtimes are a factual provider limitation. No NSS/NSPR patch is introduced.

Consequently, Browser profiles and mail accounts should use distinct immutable
configuration/trust snapshots. They may share a runtime where the selected provider's
documented lifecycle permits it; consumers needing isolation stronger than an upstream
provider can supply should choose an eligible provider or process boundary.

## Local Identity rotation

Rotation is replacement, never in-place mutation:

```text
connection A created with snapshot V1 -> retains V1
consumer constructs identity/configuration V2
connection B created with snapshot V2 -> retains V2
connection A remains unchanged and need not be torn down
```

The consumer controls when new connections adopt V2. Established TLS sessions are not
re-keyed and PST performs no filesystem watching or certificate renewal. Recreating the
runtime is unnecessary for the portable snapshot model; provider-global restrictions,
not identity mutation, may constrain a particular deployment. The deterministic M7 test
runs 50 V1/V2 rotations and verifies distinct retained identity and trust bytes with no
snapshot mutation.

## Connection metadata shape and lifetime

Stable public facts are deliberately split across existing APIs rather than duplicated
in a new aggregate record:

- the requested role belongs to the immutable connection configuration and is repeated
  in diagnostics/log events and Peer Info;
- `pst_connection_get_provider_info` reports the fixed selected provider and capability
  snapshot;
- operation results and readiness expose lifecycle progress; terminal diagnostics and
  I/O close kind expose failure/clean/truncated closure;
- an established connection can produce immutable Peer Info containing role, provider,
  negotiated TLS version, cipher, peer-authentication facts and ALPN availability;
- `pst_connection_get_negotiated_alpn` copies the negotiated protocol;
- the wait-set token remains consumer-selected scheduler identity, not a global
  connection identifier.

Negotiated Peer Info and ALPN are unavailable before handshake completion. A Peer Info
snapshot acquired after handshake owns its copied values/DER and remains readable after
clean close, terminal failure, connection destruction, and runtime destruction. A new
Peer Info or ALPN query on a terminal connection returns `INVALID_STATE`; it does not
call the provider or resurrect the connection. Diagnostics remain copied terminal facts.
No new aggregate metadata API is required for M7.

The public surface excludes `SOCKET`, `HANDLE`, `PRFileDesc *`, `SSL *`, `BIO *`, SSPI
handles, provider pointers, and internal indices. Native wait sources remain private to
the platform adapter/scheduler implementation.

## Consumer foundations

- Browser-profile foundation: ready for separate configuration/trust snapshots,
  simultaneous connections, Browser-owned future pooling, independent SNI and expected
  peer name, and CONNECT-then-TLS. HTTP pooling itself is not implemented by PST.
- LegacyMail-account foundation: ready for separate account identity/trust snapshots,
  STARTTLS, long-lived connections, and replacement identity on reconnect/new
  connection. Mail account management is not implemented by PST.
- Accelerator long-lived-runtime foundation: ready for multiple clients, new
  connections using replacement snapshots, existing connections retaining old
  snapshots, and M1-M4 scheduling. Credential renewal orchestration remains a consumer
  responsibility.

```text
NEW_METADATA_API_REQUIRED=NO
BROWSER_PROFILE_FOUNDATION=READY
LEGACYMAIL_ACCOUNT_FOUNDATION=READY
ACCELERATOR_LONG_LIVED_RUNTIME_FOUNDATION=READY
```
