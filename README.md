<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport

**PapinhoSecureTransport (PST)** gives applications a common interface for secure communication while keeping provider-specific security code out of the application's main logic.

Instead of making an application depend directly on the APIs, types, lifecycle, and particular behavior of a specific TLS implementation, PST places a common boundary between the application and compatible security providers.

TLS is the secure-transport protocol implemented by PST today. Current providers include **RetroZilla NSS**, **Windows Schannel**, and **OpenSSL**.

Validated scenarios include **TLS 1.2 and TLS 1.3 on Windows NT 4.0 SP6 x86 through RetroZilla NSS**, **TLS 1.2 on Windows 10 build 19045 x64 through Schannel**, and **TLS 1.2 and TLS 1.3 with Windows system trust on Windows 10 build 19045 x64 through OpenSSL**. The 0.4.0 packages were also validated on a separate clean Windows 10 Pro 22H2 x64 system.

PST is not limited to Internet software. It can sit underneath browsers, e-mail clients, business client/server applications, LAN services, messaging systems, and custom protocols.

The same separation can also help software age more gracefully: as operating systems, security libraries, and standards evolve, provider-specific changes can remain concentrated in the secure-transport layer instead of spreading throughout the application.

## Documentation

| Language | Project introduction | Practical guide |
|---|---|---|
| 🇬🇧 English | [Full project introduction](docs/en/README.md) | [Build, integration, and examples](docs/en/getting-started.md) |
| 🇧🇷 Português (Brasil) | [Apresentação completa do projeto](docs/pt-BR/README.md) | [Build, integração e exemplos](docs/pt-BR/getting-started.md) |

## Current highlights

- TLS 1.2 validated with all three current providers
- TLS 1.3 validated with RetroZilla NSS and OpenSSL
- Windows NT 4.0 SP6 x86 validation with RetroZilla NSS
- Windows 10 build 19045 x64 validation with Schannel and OpenSSL
- Public API 1.3.0
- Provider SPI 2.4
- Explicit built-in provider bootstrap through `pst_win32_register_builtin_providers()`

The current release baseline is **0.4.0**, with public API **1.3.0** and provider SPI **2.4**. It is distributed as target-specific static libraries and SDKs. Platforms outside the documented validation matrix remain unvalidated.

## Distribution

The 0.4.0 distribution consists of a source package and separate static SDKs for RetroZilla NSS, Schannel, OpenSSL 3.5.8, and the optional combined Schannel/OpenSSL target. The combined SDK is an official optional package, not a default recommendation. See the practical guides above, the canonical [Target Matrix](docs/target-matrix.md), and [release packaging](docs/release-packaging.md) for target selection and integration details.

## Development transparency

PapinhoSecureTransport was developed with the assistance of OpenAI Codex, which was used extensively as an engineering assistant for implementation, testing, auditing, and documentation workflows. Architectural, product, and release decisions remained the responsibility of the project maintainer.

The repository preserves selected [engineering history and release evidence](docs/codex/README.md) for transparency and auditability.

## Contributing

Contributions are welcome in documentation, real-hardware testing, legacy Windows, NSS/NSPR research, Schannel, OpenSSL, modern TLS on older systems, new providers, platform ports, examples, dependency origin and reproducibility, and licensing review.

**TLS is the only secure-transport protocol implemented by PST today.** If there is a real use case, an appropriate architecture, and community interest, contributors may also explore other secure-transport families in the future.

A particularly valuable area of research is maintaining or developing reproducible NSS/NSPR-based paths capable of bringing modern TLS to older operating systems.

See the documentation above for the project's motivation, architecture, provider model, trust concepts, retrocomputing perspective, practical integration, and community goals.

## Support the project

PapinhoSecureTransport is free and open-source software under MPL-2.0. If the project is useful to you and you would like to voluntarily support the work done around it, you can do so through GitHub Sponsors.

Sponsorship does not change access to the software or the rights granted by its license, and it does not constitute a contract for support, maintenance, or future development.

## License

PapinhoSecureTransport is licensed under the [Mozilla Public License 2.0](LICENSE). Redistributed dependencies retain their own terms; see [Third-party notices](THIRD_PARTY_NOTICES.md).