# PapinhoSecureTransport

Phase 7 interoperability/hardening is complete. Phase 8, Multiple Backends / Provider Evolution, is next but has not started. Phase 7.A8 added an optional consumer-controlled structured logging sink while retaining normalized PST_RESULT and diagnostic snapshots as authoritative state. See [docs/phase7-closure.md](docs/phase7-closure.md), [docs/logging-design.md](docs/logging-design.md), [docs/error-diagnostics.md](docs/error-diagnostics.md), and [docs/diagnostic-api-design.md](docs/diagnostic-api-design.md).

PapinhoSecureTransport is an independent secure-transport abstraction project. Its direction is portable C, initially focused on TLS through mature cryptographic/TLS backends. It will not implement cryptography, certificate validation, or a TLS state machine of its own.

The public API remains C89/VC6-compatible and backend-neutral. API 1.2.0/library 0.3.0 retain the diagnostic ABI and add Papinho Logging Levels v1 plus an immutable runtime-level structured sink. PST supplies no console/file logger, worker, queue, arbitrary message, payload, peer text, native error code, secret, path, endpoint, or backend pointer through this interface.

Project licensing is pending a decision. Third-party files retain their own licenses under `third_party/`.

The canonical VC6 environment, clean-build, regression, NSS SDK, and runtime-DLL instructions are in [docs/build-vc6.md](docs/build-vc6.md). Quick start:

```bat
tools\build-vc6.bat clean
tools\build-vc6.bat test test-nss-unit
```

The public entry point is `include/papinho_secure_transport.h`. Generated objects, the static library, and the test executable are placed under `build/vc6/`.

See [architecture](docs/architecture.md), [roadmap](docs/roadmap.md), and [legacy NSS provenance](docs/legacy-nss-provenance.md).
