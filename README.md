<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport

**PapinhoSecureTransport (PST)** gives applications a common interface for secure communication while keeping provider-specific security code out of the application's main logic.

Instead of making an application depend directly on the APIs, types, lifecycle, and particular behavior of a specific TLS implementation, PST places a common boundary between the application and compatible security providers.

TLS is the secure-transport protocol implemented by PST today. Current providers include **RetroZilla NSS**, **Windows Schannel**, and **OpenSSL**.

PST supports both **CLIENT** and **SERVER** TLS connections through public API **2.0.0** and provider SPI **3.0**. The role is explicit per connection, provider selection is also per connection, and capabilities are evaluated for the requested role before the transport is bound.

Validated scenarios include **TLS 1.2 and TLS 1.3 on Windows NT 4.0 SP6 x86 through RetroZilla NSS**, **TLS 1.2 on Windows 10 build 19045 x64 through Schannel**, and **TLS 1.2 and TLS 1.3 through OpenSSL 3.5.8 on Windows 10 build 19045 x64**. The 0.5.0 package candidates were also validated from extracted SDKs on a separate clean Windows 10 Pro x64 system, including real TLS with the Combined Schannel/OpenSSL package.

PST is not limited to Internet software. It can sit underneath browsers, e-mail clients, business client/server applications, LAN services, messaging systems, and custom protocols.

The same separation can also help software age more gracefully: as operating systems, security libraries, and standards evolve, provider-specific changes can remain concentrated in the secure-transport layer instead of spreading throughout the application.

## CLIENT and SERVER

For a **CLIENT** connection, the application creates and connects the native transport and then gives the already-connected transport to PST.

For a **SERVER** connection, the application still owns the server architecture around TLS: it creates the listener, performs `bind`, `listen` and `accept`, decides admission and application-session policy, and then gives the accepted connected transport to PST.

PST is therefore **not** an HTTP server, listener framework, application-session manager, or authorization framework. It provides the secure-transport boundary around an already-connected transport.

Each connection explicitly chooses `PST_CONNECTION_ROLE_CLIENT` or `PST_CONNECTION_ROLE_SERVER`.

Provider selection is also per connection:

- **EXACT** requests one provider;
- **ORDERED** supplies a preference order;
- **AUTOMATIC** follows the registration order for the target.

Role-scoped capability masks eliminate providers that cannot satisfy the requested connection before binding. After binding, the selected provider is pinned: handshake, authentication, trust, ALPN, I/O, transport, or shutdown failure does **not** cause fallback to another provider.

This also means an application can use different providers for different connections—for example, one provider for outbound CLIENT traffic and another for accepted SERVER connections—when the target contains both and the requested capabilities allow it.

## Identity, authentication, trust, and peer name

API 2.0 keeps several security concepts deliberately separate:

- **Local Identity** is the certificate chain and private key this endpoint presents.
- **Peer Authentication** controls whether a peer certificate is disabled, optional, or required.
- **Peer Trust** is either **CUSTOM** or **SYSTEM**; PST does not silently union the two or fall back from one to the other.
- **Expected Peer Name** is independent CLIENT-side peer-name validation and is not applicable to SERVER.
- **Peer Info** reports copied TLS/certificate facts after negotiation.

TLS authentication does not perform application authorization. A valid certificate is not automatically an application Principal, and `authenticated != authorized`.

## Incremental operation and shutdown

Handshake, readiness, encrypted I/O, and shutdown are incremental. `NEED_READ`, `NEED_WRITE`, and `NEED_READ_WRITE` mean that the operation needs later progress; they are not successful completion.

Graceful TLS shutdown requires reciprocal `close_notify`. Local emission alone is not treated as complete shutdown. After TLS is established, EOF/reset without the peer's `close_notify` is classified as truncation, including the case where authenticated application data was delivered first.

## Providers and factual asymmetry

The three providers intentionally expose only capabilities that are factual for each role.

| Provider | CLIENT | SERVER | TLS | Important SERVER notes |
|---|---:|---:|---|---|
| RetroZilla NSS/NSPR | yes | yes | 1.2, 1.3 | CUSTOM trust; SERVER SYSTEM_TRUST and complete PST SERVER ALPN are not advertised; real NT4 SP6 x86 package validation passed |
| Windows Schannel | yes | yes | CLIENT 1.2; SERVER 1.2 on validated Win10 19045 | CUSTOM/SYSTEM trust according to role masks; SERVER TLS 1.3 and complete PST SERVER ALPN are not advertised on the validated environment; chain delivery may temporarily use `CurrentUser\CA` with controlled cleanup |
| OpenSSL 3.5.8 | yes | yes | 1.2, 1.3 | CUSTOM and Windows SYSTEM trust where advertised; complete PST SERVER ALPN is supported |

Do not assume that a capability available in one role or provider is automatically available in another.

## Documentation

| Language | Project introduction | Practical guide |
|---|---|---|
| 🇬🇧 English | [Full project introduction](docs/en/README.md) | [Build, integration, and examples](docs/en/getting-started.md) |
| 🇧🇷 Português (Brasil) | [Apresentação completa do projeto](docs/pt-BR/README.md) | [Build, integração e exemplos](docs/pt-BR/getting-started.md) |

The public examples include [basic CLIENT](examples/basic_client.c) and [basic SERVER](examples/basic_server.c). The API contract is documented in [API 2.0](docs/api-2.0.md), the provider-author contract in [SPI 3.0](docs/provider-spi-3.0.md), and migration from the previous public generation in [API 1.3/SPI 2.4 to API 2.0/SPI 3.0](docs/api-1.3-to-2.0-migration.md).

## Current highlights

- CLIENT and SERVER roles through public API 2.0.0
- Provider SPI 3.0 with role-scoped capabilities
- TLS 1.2 validated with all three current providers
- TLS 1.3 validated with RetroZilla NSS and OpenSSL
- Windows NT 4.0 SP6 x86 CLIENT/SERVER validation with RetroZilla NSS
- Windows 10 build 19045 x64 CLIENT/SERVER validation with Schannel and OpenSSL
- Real cross-provider CLIENT/SERVER interoperability
- Per-connection EXACT / ORDERED / AUTOMATIC provider selection
- Provider pinning with no post-binding fallback
- Reciprocal TLS shutdown and explicit truncation detection
- Extracted-package and clean-machine validation for 0.5.0

The current release candidate is **0.5.0**, with public API **2.0.0** and provider SPI **3.0**. It is distributed as target-specific static libraries and SDKs. Platforms outside the documented validation matrix remain unvalidated.

## Distribution

The 0.5.0 distribution consists of a source package and separate static SDKs for the canonical targets based on RetroZilla NSS, Schannel, OpenSSL 3.5.8, and the optional Combined Schannel/OpenSSL target.

The Combined SDK is an official optional package for provider selection, not a fourth TLS implementation and not a default recommendation.

See the practical guides above, the canonical [Target Matrix](docs/target-matrix.md), and [release packaging](docs/release-packaging.md) for target selection and integration details.

## Development transparency

PapinhoSecureTransport was developed with the assistance of OpenAI Codex, which was used extensively as an engineering assistant for implementation, testing, auditing, and documentation workflows. Architectural, product, and release decisions remained the responsibility of the project maintainer.

The repository preserves selected [engineering history and release evidence](docs/codex/README.md) for transparency and auditability.

## Contributing

Contributions are welcome in documentation, real-hardware testing, older Windows systems, NSS/NSPR research, Schannel, OpenSSL, modern TLS on older systems, new providers, platform ports, examples, dependency origin and reproducibility, and licensing review.

**TLS is the only secure-transport protocol implemented by PST today.** If there is a real use case, an appropriate architecture, and community interest, contributors may also explore other secure-transport families in the future.

A particularly valuable area of research is maintaining or developing reproducible NSS/NSPR-based paths capable of bringing modern TLS to older operating systems.

See the documentation above for the project's motivation, architecture, provider model, trust concepts, retrocomputing perspective, practical integration, and community goals.

## Support the project

PapinhoSecureTransport is free and open-source software under MPL-2.0. If the project is useful to you and you would like to voluntarily support the work done around it, you can do so through GitHub Sponsors.

Sponsorship does not change access to the software or the rights granted by its license, and it does not constitute a contract for support, maintenance, or future development.

## License

PapinhoSecureTransport is licensed under the [Mozilla Public License 2.0](LICENSE). Redistributed dependencies retain their own terms; see [Third-party notices](THIRD_PARTY_NOTICES.md).
