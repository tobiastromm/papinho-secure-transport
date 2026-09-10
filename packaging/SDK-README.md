<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport technical SDK staging

This target-separated SDK contains the public PST headers, one static provider build, its declared link/runtime dependencies, public examples, and release documentation.

This is the PapinhoSecureTransport 0.6.0 SDK with public API 2.1 and provider SPI 3.0. CLIENT and SERVER remain selected per connection; API 2.1 adds the portable wait-set and scheduler surface without creating role-specific SDKs. PapinhoSecureTransport is licensed under MPL-2.0; see LICENSE and THIRD_PARTY_NOTICES.md. The exact corresponding source is papinho-secure-transport-0.6.0-src.zip. The NSS source package also contains the exact modified runtime snapshot and fail-closed patch.

## Use

1. Read `manifest.ini`, `VERSION`, and `consumer-link.ini`.
2. Add `include/` to the compiler include path.
3. Link the libraries listed by `consumer-link.ini` from `lib/<target-id>/` and the platform SDK.
4. For targets with package runtime DLLs, deploy the contents of `runtime/<target-id>/` beside the application executable.
5. Call `pst_win32_register_builtin_providers()` and use only the public API.

For SERVER, the consumer owns bind, listen and accept. Wrap and transfer only
the already-connected accepted socket to PST. Read the role-scoped capability
masks before selecting a provider: Schannel SERVER does not advertise TLS 1.3
or complete SERVER ALPN, and NSS SERVER does not advertise SYSTEM trust or
complete SERVER ALPN. PST itself is distributed as a static library; packaged
OpenSSL and NSS DLLs are provider runtime dependencies, not a PST shared DLL.

Do not copy runtime DLLs into Windows system directories or rely on an arbitrary global PATH. Do not mix files from different target IDs.

See [consumer linking](../docs/consumer-linking.md), [packaging decisions](../docs/release-packaging.md), [security and limitations](../docs/security-and-limitations.md), the [English guide](../docs/en/README.md), the [guia em português](../docs/pt-BR/README.md), and [public examples](../examples/README.md). `SHA256SUMS.txt` records package integrity, not publisher authenticity. The SDK stager rewrites these repository-relative links for the package-root copy.

The canonical target identity, toolchain, provider provenance, and tested environments are recorded in `docs/target-matrix.md`. The `target_id` in `manifest.ini` and `consumer-link.ini` identifies this extracted SDK.
