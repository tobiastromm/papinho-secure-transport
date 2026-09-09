<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport 0.5.0

PapinhoSecureTransport (PST) gives applications one API 2.0 contract for TLS CLIENT and SERVER connections while providers implement SPI 3.0. It separates application protocols from provider-native APIs. PST does not implement HTTP, bind/listen/accept, admission, sessions or application authorization.

## Using CLIENT and SERVER

Register the built-in providers for the selected target, create a shareable runtime, and create each connection with an explicit role and complete immutable configuration. A runtime may be shared across connections; this does not imply that one runtime or connection is safe for concurrent calls from multiple threads.

For CLIENT, the consumer creates and connects a socket and supplies it to PST. For SERVER, the consumer creates the listener, binds, listens, accepts and decides whether to admit the peer; only the accepted connected socket is supplied to PST. Before `ownership_accepted` the caller closes on failure. After acceptance the selected provider is the exactly-one close root.

Drive handshake, read, write, wait and shutdown incrementally. `NEED_READ`, `NEED_WRITE` and `NEED_READ_WRITE` request bounded retry, not completion. A graceful result requires reciprocal close_notify. EOF/reset after establishment without peer close_notify is `TRUNCATED`, including when authenticated data was delivered first.

See the [CLIENT example](../../examples/basic_client.c), [SERVER example](../../examples/basic_server.c) and [getting-started guide](getting-started.md).

## Selection and capabilities

Selection is per connection. EXACT names one provider, ORDERED supplies a copied preference list, and AUTOMATIC follows target registration order. Role-scoped capability masks filter candidates before transport binding. CLIENT and SERVER connections may therefore select different providers. Once bound, a provider is pinned; no handshake, authentication, trust, ALPN, I/O, transport or shutdown failure causes fallback.

The aggregate capability mask is a discovery union. Eligibility always uses the requested role mask. Combined is an optional Schannel/OpenSSL selection target, not another TLS implementation.

## Identity, authentication and trust

- Local Identity is the certificate chain and private key this endpoint presents.
- Peer Authentication is DISABLED, OPTIONAL or REQUIRED certificate presentation.
- Peer Trust is exactly CUSTOM or SYSTEM; PST never silently unions or substitutes them.
- Expected Peer Name is independent CLIENT-side peer-name validation. It is not applicable to SERVER.
- Peer Info reports copied TLS/certificate facts. Authenticated does not mean authorized and a certificate is not automatically an application Principal.

CLIENT ALPN is an ordered offer. SERVER ALPN selects the first server-preference item offered by the client. REQUIRED rejects no match, OPTIONAL permits establishment without a selection, and DISABLED does not negotiate ALPN.

## Provider matrix

| Provider | CLIENT | SERVER | TLS | Trust and SERVER notes |
|---|---:|---:|---|---|
| RetroZilla NSS/NSPR | yes | yes | 1.2/1.3 | CUSTOM; SERVER SYSTEM_TRUST and complete SERVER ALPN absent; real NT4 SP6 x86 validated |
| Schannel | yes | yes | CLIENT 1.2; SERVER 1.2 on validated Win10 19045 | CUSTOM/SYSTEM per role mask; SERVER TLS 1.3 and complete SERVER ALPN absent there; CurrentUser\\CA chain-delivery adaptation has safe normal cleanup and documented crash-residue limitation |
| OpenSSL 3.5.8 | yes | yes | 1.2/1.3 | CUSTOM and Windows SYSTEM where advertised; complete SERVER ALPN |

Capabilities are factual and role-scoped; provider symmetry is never implied. See [Providers](../providers.md), [API 2.0](../api-2.0.md), [SPI 3.0](../provider-spi-3.0.md), [Target Matrix](../target-matrix.md), [Security](../security-and-limitations.md), and the [migration guide](../api-1.3-to-2.0-migration.md).

## Distribution

The static 0.5.0 SDKs use canonical target IDs for x86 VC6/NSS, x64 Schannel, x64 OpenSSL and x64 Combined. Each manifest gives exact toolchain, architecture, provider set, capabilities, libraries and adjacent runtimes. A toolchain label is not an OS-version label. Choose the target whose factual matrix and deployment requirements fit the application.

PST is MPL-2.0 software. Third-party components keep their own licenses and corresponding-source obligations.
