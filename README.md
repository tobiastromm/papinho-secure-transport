# PapinhoSecureTransport

**PapinhoSecureTransport (PST)** gives applications a common interface for secure communication without tying their main code to a single TLS/security implementation or a single generation of operating system.

TLS is the secure-transport protocol implemented by PST today. Current providers include **RetroZilla NSS**, **Windows Schannel**, and **OpenSSL**, with validated scenarios ranging from Windows NT 4.0 to Windows 10.

PST is designed for more than Internet software: it can sit underneath browsers, e-mail clients, business client/server applications, LAN services, and custom protocols while keeping application logic separate from provider-specific TLS code.

A longer project introduction is currently available in Portuguese. The full English version will be prepared after that text is finalized.

## Documentation

- 🇧🇷 [Português (Brasil) — apresentação completa do projeto](docs/pt-BR/README.md)
- 🇬🇧 Full English documentation — coming after the Portuguese text is finalized

## Current highlights

- TLS 1.2 validated with all three current providers
- TLS 1.3 validated with RetroZilla NSS and OpenSSL
- Windows NT 4.0 SP6 x86 validation with RetroZilla NSS
- Windows 10 build 19045 x64 validation with Schannel and OpenSSL
- Public API 1.3.0
- Provider SPI 2.4
- Explicit built-in provider bootstrap through `pst_win32_register_builtin_providers()`

The project is still preparing its first stabilized public distribution. Platform validation, packaging, and documentation are still being completed.

## Contributing

Contributions are welcome in documentation, real-hardware testing, legacy Windows, NSS/NSPR research, Schannel, OpenSSL, modern TLS on older systems, new providers, platform ports, examples, provenance, licensing review, and — when there is a real use case and community interest — research into other secure-transport families.

See the full Portuguese introduction above for the project’s motivation and architecture.
