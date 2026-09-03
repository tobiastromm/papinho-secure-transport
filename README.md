# PapinhoSecureTransport

Phase 7 interoperability/hardening is complete. Phase 8, Multiple Backends / Provider Evolution, is in progress through completed cross-backend validation. RetroZilla NSS provenance/reproducibility housekeeping is complete; the Phase 8 closure audit remains pending. See [docs/provider-evolution.md](docs/provider-evolution.md) and [docs/phase7-closure.md](docs/phase7-closure.md).

PapinhoSecureTransport is an independent secure-transport abstraction project. Its direction is portable C, initially focused on TLS through mature cryptographic/TLS backends. It will not implement cryptography, certificate validation, or a TLS state machine of its own.

The public API remains C89/VC6-compatible and backend-neutral. API 1.2.0/library 0.3.0 retain the diagnostic ABI and add Papinho Logging Levels v1 plus an immutable runtime-level structured sink. PST supplies no console/file logger, worker, queue, arbitrary message, payload, peer text, native error code, secret, path, endpoint, or backend pointer through this interface.

Project licensing is pending a decision. Third-party files retain their own licenses under `third_party/`.

The canonical VC6 environment, clean-build, regression, repository-contained NSS SDK, and runtime-DLL instructions are in [docs/build-vc6.md](docs/build-vc6.md). Historical NSS/NSPR source reproduction is documented in [docs/build-retrozilla-nss.md](docs/build-retrozilla-nss.md) and [third_party/retrozilla-nss/PROVENANCE.md](third_party/retrozilla-nss/PROVENANCE.md). The separate modern MSVC x64 bootstrap is documented in [docs/build-modern-msvc.md](docs/build-modern-msvc.md). VC6 quick start:

```bat
tools\build-vc6.bat clean
tools\build-vc6.bat test test-nss-unit
```

The public entry point is `include/papinho_secure_transport.h`. Generated objects, the static library, and the test executable are placed under `build/vc6/`.

See [architecture](docs/architecture.md), [roadmap](docs/roadmap.md), and [legacy NSS provenance](docs/legacy-nss-provenance.md).
