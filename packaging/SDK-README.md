<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport technical SDK staging

This target-separated SDK contains the public PST headers, one static provider build, its declared link/runtime dependencies, public examples, and release documentation.

This is a local 0.4.0 release candidate SDK, not a published release. PapinhoSecureTransport is licensed under MPL-2.0; see LICENSE and THIRD_PARTY_NOTICES.md. The exact corresponding source is papinho-secure-transport-0.4.0-src.zip. The legacy NSS source package also contains the exact modified runtime snapshot and fail-closed patch.

## Use

1. Read `manifest.ini`, `VERSION`, and `consumer-link.ini`.
2. Add `include/` to the compiler include path.
3. Link the libraries listed by `consumer-link.ini` from `lib/<target-id>/` and the platform SDK.
4. For targets with package runtime DLLs, deploy the contents of `runtime/<target-id>/` beside the application executable.
5. Call `pst_win32_register_builtin_providers()` and use only the public API.

Do not copy runtime DLLs into Windows system directories or rely on an arbitrary global PATH. Do not mix files from different target IDs.

See [consumer linking](docs/consumer-linking.md), [packaging decisions](docs/release-packaging.md), [security and limitations](docs/security-and-limitations.md), and [public examples](examples/README.md). `SHA256SUMS.txt` records package integrity, not publisher authenticity.
