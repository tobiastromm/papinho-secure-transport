# PapinhoSecureTransport

**PapinhoSecureTransport (PST)** gives applications a common interface
for secure communication while keeping provider-specific security code
out of the application's main logic.

Instead of making an application depend directly on the APIs, types,
lifecycle, and particular behavior of a specific TLS implementation, PST
places a common boundary between the application and compatible security
providers.

TLS is the secure-transport protocol implemented by PST today. Current
providers include **RetroZilla NSS**, **Windows Schannel**, and
**OpenSSL**.

Validated scenarios currently include **TLS 1.2 and TLS 1.3 on Windows
NT 4.0 SP6 x86 through RetroZilla NSS**, **TLS 1.2 on Windows 10 build
19045 x64 through Schannel**, and **TLS 1.2 and TLS 1.3 on Windows 10
build 19045 x64 through OpenSSL**.

PST is not limited to Internet software. It can sit underneath browsers,
e-mail clients, business client/server applications, LAN services,
messaging systems, and custom protocols.

The same separation can also help software age more gracefully: as
operating systems, security libraries, and standards evolve,
provider-specific changes can remain concentrated in the
secure-transport layer instead of spreading throughout the application.

## Documentation

-   🇬🇧 [English --- full project introduction](docs/en/README.md)
-   🇬🇧 [English --- practical guide: build, integration, and
    examples](docs/en/getting-started.md)
-   🇧🇷 [Português (Brasil) --- apresentação completa do
    projeto](docs/pt-BR/README.md)
-   🇧🇷 [Português (Brasil) --- guia prático: build, integração e
    exemplos](docs/pt-BR/getting-started.md)

## Current highlights

-   TLS 1.2 validated with all three current providers
-   TLS 1.3 validated with RetroZilla NSS and OpenSSL
-   Windows NT 4.0 SP6 x86 validation with RetroZilla NSS
-   Windows 10 build 19045 x64 validation with Schannel and OpenSSL
-   Public API 1.3.0
-   Provider SPI 2.4
-   Explicit built-in provider bootstrap through
    `pst_win32_register_builtin_providers()`

The project is still preparing its first stabilized public distribution.
Additional platform validation, packaging, licensing review, and release
preparation are still in progress.

## Contributing

Contributions are welcome in documentation, real-hardware testing,
legacy Windows, NSS/NSPR research, Schannel, OpenSSL, modern TLS on
older systems, new providers, platform ports, examples, dependency
origin and reproducibility, and licensing review.

**TLS is the only secure-transport protocol implemented by PST today.**
If there is a real use case, an appropriate architecture, and community
interest, contributors may also explore other secure-transport families
in the future.

A particularly valuable area of research is maintaining or developing
reproducible NSS/NSPR-based paths capable of bringing modern TLS to
older operating systems.
