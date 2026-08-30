# PapinhoSecureTransport

PapinhoSecureTransport is an independent secure-transport abstraction project. Its direction is portable C, initially focused on TLS through mature cryptographic/TLS backends. It will not implement cryptography, certificate validation, or a TLS state machine of its own.

Phase 0 is repository and provenance bootstrap. The first preserved legacy backend candidate is RetroZilla NSS/NSPR, built with Visual C++ 6.0 for Win32 x86 and validated separately on Windows NT 4.0 SP6. The backend is not yet integrated and no public PapinhoSecureTransport API exists.

Project licensing is pending a decision. Third-party files retain their own licenses under `third_party/`.

See [architecture](docs/architecture.md), [roadmap](docs/roadmap.md), and [legacy NSS provenance](docs/legacy-nss-provenance.md).
