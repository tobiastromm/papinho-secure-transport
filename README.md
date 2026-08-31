# PapinhoSecureTransport

Phase 5 provides the public incremental TLS runtime. See [docs/tls-runtime.md](docs/tls-runtime.md) for backend selection, frozen policy, the separate Win32 adapter, readiness, secure I/O, peer identity and shutdown. Phase 6 legacy-platform validation has not started.

PapinhoSecureTransport is an independent secure-transport abstraction project. Its direction is portable C, initially focused on TLS through mature cryptographic/TLS backends. It will not implement cryptography, certificate validation, or a TLS state machine of its own.

Phase 1 provides the portable public foundation: C89-compatible fixed-width types, stable result codes, API/library version queries, size/version-tagged public records, and opaque handle declarations. The foundation builds and its unit test runs with Visual C++ 6.0. It does not yet implement TLS, backend selection, networking, credentials, trust, or the backend SPI.

Project licensing is pending a decision. Third-party files retain their own licenses under `third_party/`.

Build and run the portable foundation test from a Visual C++ 6.0 command environment:

```bat
nmake /f Makefile.vc6 test
```

The public entry point is `include/papinho_secure_transport.h`. Generated objects, the static library, and the test executable are placed under `build/vc6/`.

See [architecture](docs/architecture.md), [roadmap](docs/roadmap.md), and [legacy NSS provenance](docs/legacy-nss-provenance.md).
