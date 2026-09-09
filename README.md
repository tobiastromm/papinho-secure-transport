<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport

PapinhoSecureTransport (PST) is a provider-neutral TLS transport layer for CLIENT and SERVER applications. Library/package 0.5.0 exposes public API 2.0.0 and provider SPI 3.0. TLS is the only secure-transport protocol currently implemented.

An application creates and connects a native transport. For SERVER use, the application also owns bind, listen, accept, admission and session policy. PST receives one already-connected transport and, after explicit ownership acceptance, drives TLS handshake, encrypted I/O and reciprocal shutdown incrementally. PST is not an HTTP server, listener or application authorization framework.

## Core model

- Every connection explicitly chooses `PST_CONNECTION_ROLE_CLIENT` or `PST_CONNECTION_ROLE_SERVER`.
- Provider selection is per connection, so CLIENT and SERVER connections sharing one runtime may select different providers.
- EXACT tries one provider; ORDERED considers a copied preference list; AUTOMATIC uses target registration order.
- Role-scoped capabilities eliminate ineligible providers before binding. After binding the provider is pinned: handshake, authentication, trust, transport or shutdown failure never triggers provider fallback.
- Local Identity is what this endpoint presents. Peer Authentication controls whether a peer certificate is disabled, optional or required. Peer Trust is exactly CUSTOM or SYSTEM, without silent union or fallback. Expected Peer Name is CLIENT-side name validation and is not applicable to SERVER.
- Authentication establishes TLS identity facts; application authorization remains the consumer's responsibility.
- CLIENT ALPN is an ordered offer. SERVER ALPN is local preference. REQUIRED, OPTIONAL and DISABLED retain distinct semantics.
- `NEED_READ`, `NEED_WRITE` and `NEED_READ_WRITE` expose bounded nonblocking progress. Local close_notify emission alone is not completion; successful shutdown requires reciprocal TLS closure. Established EOF/reset without peer close_notify is `TRUNCATED`.

## Providers and factual asymmetry

| Provider | CLIENT | SERVER | TLS | Important SERVER limitations |
|---|---:|---:|---|---|
| RetroZilla NSS/NSPR | yes | yes | 1.2, 1.3 | no SYSTEM_TRUST or complete PST SERVER ALPN |
| Windows Schannel | yes | yes | CLIENT 1.2; SERVER 1.2 on validated Win10 19045 | no validated SERVER TLS 1.3 or complete PST SERVER ALPN; chain delivery may temporarily use CurrentUser\\CA |
| OpenSSL 3.5.8 | yes | yes | 1.2, 1.3 | SERVER peer-name verification is not applicable |

CUSTOM_TRUST is supported by all three. SYSTEM_TRUST is advertised only by the role/provider masks that implement it. See [Providers](docs/providers.md) and the canonical [Target Matrix](docs/target-matrix.md); do not assume provider symmetry.

## Start here

| Language | Overview | Getting started |
|---|---|---|
| English | [Project guide](docs/en/README.md) | [Build and integration](docs/en/getting-started.md) |
| Português (Brasil) | [Guia do projeto](docs/pt-BR/README.md) | [Build e integração](docs/pt-BR/getting-started.md) |

Public examples include [basic CLIENT](examples/basic_client.c), [basic SERVER](examples/basic_server.c), trust, mTLS, provider selection, diagnostics and logging. The API contract is [API 2.0](docs/api-2.0.md), the provider-author contract is [SPI 3.0](docs/provider-spi-3.0.md), and migration is covered by [API 1.3/SPI 2.4 to API 2.0/SPI 3.0](docs/api-1.3-to-2.0-migration.md).

## Packages

The 0.5.0 release-candidate set contains a source archive and static SDKs for the four canonical target IDs: RetroZilla NSS x86/VC6, Schannel x64, OpenSSL x64, and optional Combined Schannel/OpenSSL x64. Combined is a provider-selection package, not a fourth TLS implementation. Package manifests state exact architecture, toolchain, providers, role masks, link libraries and runtime files.

Validated environments include real Windows NT 4.0 SP6 x86 for NSS CLIENT/SERVER TLS 1.2 and TLS 1.3, and Windows 10 build 19045 x64 for Schannel/OpenSSL/Combined. Toolchain identifiers are not claims about OS support. Platforms outside the [validation matrix](docs/target-matrix.md) remain unvalidated.

## Security, contribution and license

Read [Security and limitations](docs/security-and-limitations.md) and [SECURITY.md](SECURITY.md) before deployment. Contributions are welcome, especially for reproducible NSS/NSPR work, legacy Windows, provider ports, interoperability and documentation.

PST is licensed under the [Mozilla Public License 2.0](LICENSE). Redistributed dependencies retain their own terms; see [Third-party notices](THIRD_PARTY_NOTICES.md).
