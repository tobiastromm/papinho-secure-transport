# PapinhoSecureTransport

PapinhoSecureTransport (PST) is a small C library for secure connections without coupling an application to one TLS engine or one Windows generation. It serves public-Internet, LAN, private/corporate, legacy-to-modern, and modern-to-modern software. HTTP, SMTP, IMAP, ERP protocols, and custom application protocols run above PST; PST does not implement them.

Today PST implements TLS through three real providers: RetroZilla NSS for the Windows NT 4.0 target, Schannel for modern Windows, and OpenSSL 3.5.8. TLS is the current secure transport—not a promise that SPI 2.4 already supports DTLS, QUIC, Noise, or other future designs.

## Start here

- [English documentation](docs/en/README.md)
- [Documentação em Português (Brasil)](docs/pt-BR/README.md)
- [English Getting Started](docs/en/getting-started.md)
- [Primeiros passos em português](docs/pt-BR/primeiros-passos.md)
- [Examples](examples/README.md)
- [Build overview](docs/en/build.md)
- [Contributing](docs/en/contributing.md) · [Como contribuir](docs/pt-BR/como-contribuir.md)

A normal Win32 program calls `pst_win32_register_builtin_providers()`, selects a runtime, configures authenticated TLS, attaches a connected socket, and drives bounded incremental operations.

## Why legacy support matters

Preserving old computers should not require modern servers to re-enable obsolete cryptography. PST explores moving modern, fail-closed secure transport toward legacy applications where technically feasible. PapinhoBrowser and PapinhoLegacyMail are example consumers. PapinhoAccelerator is specific to PapinhoBrowser; it is not PST infrastructure and is not a PST or LegacyMail dependency.

API 1.3.0 and internal provider SPI 2.4 are frozen for the Phase-9 baseline; Library 0.3.0 remains current. Packaging and release validation are still in progress; this is not yet the final Phase-9 release. PST's own license is under review for Phase 9.E. Dependency notices and provenance remain authoritative in `third_party/` and the linked engineering records.
