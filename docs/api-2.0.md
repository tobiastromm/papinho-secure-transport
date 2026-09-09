<!-- SPDX-License-Identifier: MPL-2.0 -->

# Public API 2.0 CLIENT/SERVER contract

Status: frozen contract implemented by the library 0.5.0 development core. SS-2 migrated existing CLIENT behavior for RetroZilla NSS, Schannel and OpenSSL; real SERVER provider implementations remain outside this phase.

## Roles and object model

Every connection has exactly one mandatory `PST_CONNECTION_ROLE_CLIENT` or `PST_CONNECTION_ROLE_SERVER`; zero is invalid. Role is supplied in `PST_CONNECTION_CONFIG` and is never inferred. A runtime owns a sealed view of the registered provider descriptors and lazily created provider runtime states. It is logically shareable by multiple connections but API 2.0 does not guarantee concurrent use from multiple threads.

Provider selection occurs transactionally during connection creation. EXACT tests only its named provider, ORDERED tests the caller's copied order, and AUTOMATIC tests registration order. A candidate must satisfy the role and every calculated/configured requirement. Initialization failure may advance to the next candidate only during selection. Once bound, a connection never reselects after handshake, authentication, policy, ALPN, I/O, wait, or shutdown failure. Selected provider information belongs to the connection.

## Configuration

`PST_CONNECTION_CONFIG` contains provider selection, explicit role, Local Identity, Peer Authentication/Trust, TLS policy and ALPN. Connection creation validates and copies the complete configuration before publishing a handle; no mutable builder or partial configured connection escapes.

Local Identity answers what this endpoint presents. Its credential object owns copied leaf/chain DER and unencrypted PKCS#8 DER and securely clears its private-key copy before release. Certificate/key correspondence is validated no later than provider conversion during connection creation. CLIENT identity may be absent. Certificate-based SERVER requires it; a provider that cannot use it is ineligible.

Peer Authentication is DISABLED, OPTIONAL or REQUIRED. DISABLED accepts absence and never claims an authenticated peer. OPTIONAL accepts absence, but a presented invalid or untrusted certificate fails. REQUIRED fails on absence, invalidity or untrusted status. Peer Trust is an exclusive CUSTOM or SYSTEM object: modes are never unioned or silently substituted. Expected Peer Name is copied, independent of trust, normally required for authenticated CLIENT and normally absent for SERVER. Authentication does not perform application authorization and an established certificate is not automatically an application Principal.

## ALPN and TLS

Each ALPN item is 1..255 arbitrary bytes; pointer/count pairs must agree, aggregate encoding must not overflow provider limits, and duplicates are rejected. The list is copied. For CLIENT it is an ordered offer. For SERVER it is server preference: select the first local item also offered by the client. REQUIRED with an empty list or no intersection fails with `PST_RESULT_POLICY_VIOLATION`; OPTIONAL may establish without ALPN; DISABLED sends/selects none. API 2.0 has no selection callback or `selection_policy`.

TLS minimum and maximum are exact PST values and never widened. Resumption and early data remain independently capability-gated; early data requires resumption. No plaintext or provider fallback exists.

## Capabilities

Capability bits are independent factual claims: TLS_1_2 `0x00000001`, TLS_1_3 `0x00000002`, ROLE_CLIENT `0x00000004`, ROLE_SERVER `0x00000008`, LOCAL_IDENTITY `0x00000010`, PEER_CERT_AUTH `0x00000020`, PEER_CERT_OPTIONAL `0x00000040`, ALPN_CLIENT `0x00000080`, ALPN_SERVER `0x00000100`, CUSTOM_TRUST `0x00000200`, SYSTEM_TRUST `0x00000400`, PEER_NAME_VERIFY `0x00000800`, PEER_INFO `0x00001000`, NONBLOCKING `0x00002000`, BACKEND_WAIT `0x00004000`, RESUMPTION `0x00008000`, EARLY_DATA `0x00010000`. No bit implies another unless this contract states it; EARLY_DATA requiring RESUMPTION is validation, not inferred advertisement. The current 32-bit container is storage, not semantic architecture. Each provider also reports a CLIENT mask and a SERVER mask. The aggregate mask is their union for discovery and must never be interpreted as every possible role/capability cross-product.

Required capabilities are the union of explicit caller requirements and requirements calculated from role/configuration. Unknown required bits fail closed.

## Runtime and provider queries

Runtime info reports provider count and no selected backend. `PST_PROVIDER_INFO` reports copied stable ID, aggregate effective capabilities, `client_capabilities`, `server_capabilities`, availability and initialization state. Connection provider info reports the provider actually bound. Selection tests the mask for the requested role against the whole frozen configuration before binding. Availability, aggregate discovery and per-connection eligibility are distinct.

Provider runtime state is lazy: first eligible use initializes provider-global and provider-runtime state transactionally. A failed candidate retains a normalized selection diagnostic and may be skipped according to selection mode. Successfully initialized states live until the last dependent connection is destroyed and runtime release completes. Runtime release while children exist is guarded. Providers are destroyed in reverse successful-initialization order. No thread-safety guarantee is implied.

The public surface groups are: version/result initialization and queries; credential and trust snapshot creation/release; runtime creation, logging, diagnostics and provider enumeration; connection creation/provider query/diagnostics; transport attach; handshake/interest/wait; read/write; Peer Info and negotiated ALPN; incremental shutdown and release. Every object created by PST is released by its matching PST release function.

## Transport, progress, and lifetime

PST accepts only a connected transport. Consumer owns socket creation, bind, listen, accept, admission and scheduling. Before `ownership_accepted != 0`, caller owns and closes after failure. After acceptance, the selected provider is the sole close root even if attach later fails. Destroy without graceful shutdown performs bounded local cleanup. Graceful shutdown remains incremental.

Handshake, read, write, wait and shutdown are role-neutral, caller-driven and bounded. NEED_READ/NEED_WRITE/NEED_READ_WRITE mean retry may be useful, never completion. Native readiness is not secure-backend readiness. Timeout is a successful wait with no operation progress. No hidden unbounded loop is permitted.

## Peer info, diagnostics, and logging

Peer info records local role, provider ID, certificate presence, chain validation, authentication, peer-name validation, TLS version, cipher, ALPN, fingerprint and copied leaf DER. CLIENT peer is SERVER; SERVER peer is CLIENT. Facts use UNKNOWN, FALSE, TRUE, UNSUPPORTED or NOT_APPLICABLE. SERVER name validation is normally NOT_APPLICABLE and must never be fabricated.

Diagnostics add role and a normalized reason: NONE, PEER_CERT_ABSENT, PEER_CERT_INVALID, PEER_CERT_UNTRUSTED, PEER_NAME_MISMATCH, ALPN_MISMATCH or TLS_POLICY_MISMATCH. Logging events add role plus normalized peer-auth and policy facts. Consumers never need text parsing. Native errors remain private secondary detail. Keys, complete credentials/trust, payload, passwords/PINs, handles and arbitrary sensitive provider data are prohibited.

## Validation and forward compatibility

All input records begin with size/version. Undersized records fail; API-major mismatch fails; larger same-major records preserve unknown tails. Closed enum domains reject unknown values. Outputs are initialized deterministically. API 2.0 intentionally removes API 1.3's mutable `pst_config`, `PST_IDENTITY_CONFIG`, runtime-time selection, `CLIENT_AUTH`, combined ALPN/TLS policy, and hostname-specific peer fact.

The authoritative declarations are `include/papinho_secure_transport.h` and the platform bootstrap/transport functions in `include/papinho_secure_transport_win32.h`. This document describes their semantics; it does not add listener ownership, application protocol framing or authorization behavior.
