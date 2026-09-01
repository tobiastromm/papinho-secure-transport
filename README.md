# PapinhoSecureTransport

Phase 6 legacy-platform validation is complete and Phase 7 interoperability/hardening is in progress. Phase 7.A5 adds the first limited public diagnostic snapshot ABI while retaining normalized `PST_RESULT` control flow. See [docs/tls-runtime.md](docs/tls-runtime.md), [docs/error-diagnostics.md](docs/error-diagnostics.md), and [docs/diagnostic-api-design.md](docs/diagnostic-api-design.md).

PapinhoSecureTransport is an independent secure-transport abstraction project. Its direction is portable C, initially focused on TLS through mature cryptographic/TLS backends. It will not implement cryptography, certificate validation, or a TLS state machine of its own.

The public API remains C89/VC6-compatible and backend-neutral. API 1.1.0/library 0.2.0 provide size/version-tagged `PST_DIAGNOSTIC_INFO`, typed runtime/connection snapshot copies, and optional-output extended constructors. The public snapshot deliberately excludes native error codes, private phases, pointers, secrets, peer data, payload, paths, and arbitrary text.

Project licensing is pending a decision. Third-party files retain their own licenses under `third_party/`.

Build and run the portable foundation test from a Visual C++ 6.0 command environment:

```bat
nmake /f Makefile.vc6 test
```

The public entry point is `include/papinho_secure_transport.h`. Generated objects, the static library, and the test executable are placed under `build/vc6/`.

See [architecture](docs/architecture.md), [roadmap](docs/roadmap.md), and [legacy NSS provenance](docs/legacy-nss-provenance.md).
