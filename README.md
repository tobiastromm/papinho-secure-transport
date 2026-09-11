<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport

**PapinhoSecureTransport (PST)** gives applications a common interface for secure communication while keeping provider-specific security code out of the application's main logic.

Instead of making an application depend directly on the APIs, types, lifecycle, readiness model, and particular behavior of a specific TLS implementation, PST places a common boundary between the application and compatible security providers.

TLS is the secure-transport protocol implemented by PST today. Current providers include **RetroZilla NSS**, **Windows Schannel**, and **OpenSSL**.

The current 0.6.1 bugfix candidate uses public API **2.1.0** and provider SPI **3.0**. It preserves the CLIENT/SERVER and scheduler contracts while correcting the tri-state graceful-shutdown capability requirement.

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

## API 2.1: many connections without exposing provider internals

The 0.6.0 release track extends the common boundary to multiplexed readiness.

A portable **wait-set** can observe multiple PST connections through stable consumer tokens. On Win32, it can also include borrowed application-owned native sources such as a listening socket. The listener remains owned by the application: PST does not call `accept`, `shutdown`, or `closesocket` on it.

Finite blocking waits and an explicit cross-thread wake allow one application-owned I/O thread to sleep until useful work exists without mandatory periodic polling or a sequential `N × timeout` loop. Provider-specific readiness remains authoritative: in particular, raw socket readiness never replaces RetroZilla NSS/NSPR `PR_Poll` semantics.

Read and write remain bounded and incremental. Partial progress and `NEED_READ`, `NEED_WRITE`, or `NEED_READ_WRITE` are normal states; the application owns its buffers, unsent suffixes, operation deadlines, and fairness policy.

## Identity, authentication, trust, peer name, and SNI

PST deliberately keeps several security concepts separate:

- **Local Identity** is the certificate chain and private key this endpoint presents.
- **Peer Authentication** controls whether a peer certificate is disabled, optional, or required.
- **Peer Trust** is either **CUSTOM** or **SYSTEM**; PST does not silently union the two or fall back from one to the other.
- **Expected Peer Name** is independent CLIENT-side certificate-name validation and is not applicable to SERVER.
- **SNI** is routing information sent by a CLIENT and, in API 2.1, has explicit `COMPAT`, `DISABLED`, and `EXPLICIT` modes.
- **Peer Info** reports copied TLS/certificate facts after negotiation.

Expected Peer Name and SNI are therefore not the same setting. OpenSSL and Schannel have validated independent SNI control. The current RetroZilla NSS snapshot remains factually partial because its CLIENT hostname/SNI behavior is coupled through `SSL_SetURL`; PST filters unsupported combinations before binding instead of patching NSS or pretending the capability exists.

TLS authentication does not perform application authorization. A valid certificate is not automatically an application Principal, and `authenticated != authorized`.

## TLS after plaintext: STARTTLS and CONNECT foundations

PST remains protocol-agnostic, but the 0.6.0 release track proves a useful boundary for browsers and e-mail software: an application may use a connected transport for plaintext protocol negotiation, stop exactly at a clean upgrade boundary, and then transfer that **same connected transport** to PST for TLS.

That model was validated across OpenSSL, Schannel, and RetroZilla NSS for generic **STARTTLS-style** and **HTTP CONNECT-style** flows. PST does not parse SMTP, IMAP, or HTTP and does not reconnect behind the application's back.

Once PST accepts ownership, a TLS failure does not roll the connection back to plaintext and does not return transport ownership. Pre-reading bytes that already belong to the TLS stream before attach is deliberately outside the current supported boundary.

This provides the secure-transport foundation needed by projects such as PapinhoLegacyMail and PapinhoBrowser without moving their application protocols into PST.

## Incremental operation and shutdown

Handshake, readiness, encrypted I/O, and shutdown are incremental. `NEED_READ`, `NEED_WRITE`, and `NEED_READ_WRITE` mean that the operation needs later progress; they are not successful completion.

Graceful TLS shutdown requires reciprocal `close_notify`. Local emission alone is not treated as complete shutdown. After TLS is established, EOF/reset without the peer's `close_notify` is classified as truncation, including the case where authenticated application data was delivered first.

The M9 cross-provider closure also corrected two Schannel shutdown defects in the 0.6.0 release tree: completion after a reciprocal `close_notify` had already been observed, and processing already-buffered TLS before requesting another socket read.

## Providers and factual asymmetry

The three providers intentionally expose only capabilities that are factual for each role.

| Provider | CLIENT | SERVER | TLS | Important notes |
|---|---:|---:|---|---|
| RetroZilla NSS/NSPR | yes | yes | 1.2, 1.3 | real NT4 SP6 x86 validation; CUSTOM SERVER trust; complete SERVER SYSTEM_TRUST/ALPN absent; independent CLIENT SNI control remains partial |
| Windows Schannel | yes | yes | 1.2 on validated Win10 environment | CUSTOM/SYSTEM trust according to role masks; complete SERVER TLS 1.3/ALPN not advertised in the validated environment |
| OpenSSL 3.5.8 | yes | yes | 1.2, 1.3 | CUSTOM and Windows SYSTEM trust where advertised; complete SERVER ALPN and independent CLIENT SNI control validated |

M9 executed all **9/9 TLS 1.2 CLIENT×SERVER provider pairs** and all **4/4 TLS 1.3 pairs eligible under the capability model**. The five TLS 1.3 combinations involving Schannel were rejected as ineligible before binding rather than attempted and downgraded.

Do not assume that a capability available in one role or provider is automatically available in another.

## Documentation

| Language | Project introduction | Practical guide |
|---|---|---|
| 🇬🇧 English | [Full project introduction](docs/en/README.md) | [Build, integration, and examples](docs/en/getting-started.md) |
| 🇧🇷 Português (Brasil) | [Apresentação completa do projeto](docs/pt-BR/README.md) | [Build, integração e exemplos](docs/pt-BR/getting-started.md) |

The public examples include [basic CLIENT](examples/basic_client.c) and [basic SERVER](examples/basic_server.c). The API 2.0 CLIENT/SERVER contract remains documented in [API 2.0](docs/api-2.0.md); the additive API 2.1 wait-set/SNI release contract is recorded in [Public API and ABI](docs/public-api-abi.md) and [Readiness / Progress](docs/readiness-progress.md). Provider authors use [SPI 3.0](docs/provider-spi-3.0.md).

## Current highlights

- Current **0.6.1 / API 2.1.0 / SPI 3.0** bugfix candidate
- Published **0.5.0 / API 2.0.0 / SPI 3.0** historical baseline
- CLIENT and SERVER roles with per-connection provider selection
- TLS 1.2 validated with all three providers
- TLS 1.3 validated with RetroZilla NSS and OpenSSL
- Windows NT 4.0 SP6 x86 CLIENT/SERVER validation with RetroZilla NSS
- Real cross-provider CLIENT/SERVER interoperability
- Portable wait-set with stable consumer tokens
- Consumer-owned external/native source integration
- Finite blocking wait and cross-thread wake without mandatory polling
- Partial-I/O and backpressure hardening
- SNI separated from Expected Peer Name where the provider can support it
- Generic STARTTLS-style and CONNECT-style same-transport TLS upgrade proofs
- Provider pinning with no post-binding fallback
- Reciprocal TLS shutdown and explicit truncation detection
- 250-cycle mixed M9 scheduler/security/lifecycle stress with no crashes or hangs

The 0.6.0 release candidate is in final M10 physical NT4, clean-machine, packaging, reproduction, documentation, and publication validation. Platforms outside the documented validation matrix remain unvalidated.

## Distribution

The 0.6.0 distribution consists of a source package and separate static SDKs for the canonical targets based on RetroZilla NSS, Schannel, OpenSSL 3.5.8, and the optional Combined Schannel/OpenSSL target.

The Combined SDK is an official optional package for provider selection, not a fourth TLS implementation and not a default recommendation.

See the practical guides above, the canonical [Target Matrix](docs/target-matrix.md), and [release packaging](docs/release-packaging.md) for target selection and integration details.

## Development transparency

PapinhoSecureTransport was developed with the assistance of OpenAI Codex, which was used extensively as an engineering assistant for implementation, testing, auditing, and documentation workflows. Architectural, product, and release decisions remained the responsibility of the project maintainer.

The repository preserves selected [engineering history and release evidence](docs/codex/README.md) for transparency and auditability.

## Contributing

Contributions are welcome in documentation, real-hardware testing, older Windows systems, NSS/NSPR research, Schannel, OpenSSL, modern TLS on older systems, new providers, platform ports, examples, dependency origin and reproducibility, and licensing review.

**TLS is the only secure-transport protocol implemented by PST today.** If there is a real use case, an appropriate architecture, and community interest, contributors may also explore other secure-transport families in the future.

A particularly valuable area of research is maintaining or developing reproducible NSS/NSPR-based paths capable of bringing modern TLS to older operating systems.

## Support the project

PapinhoSecureTransport is free and open-source software under MPL-2.0. If the project is useful to you and you would like to voluntarily support the work done around it, you can do so through GitHub Sponsors.

Sponsorship does not change access to the software or the rights granted by its license, and it does not constitute a contract for support, maintenance, or future development.

## License

PapinhoSecureTransport is licensed under the [Mozilla Public License 2.0](LICENSE). Redistributed dependencies retain their own terms; see [Third-party notices](THIRD_PARTY_NOTICES.md).
