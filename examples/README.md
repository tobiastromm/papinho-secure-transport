<!-- SPDX-License-Identifier: MPL-2.0 -->

# Public API examples

These sources include only public PST headers. They are intentionally small building blocks; replace fixture paths, addresses and certificates with deployment-controlled values.

- `basic_client.c`: bootstrap, runtime/config, transport ownership, bounded handshake and shutdown helpers.
- `custom_trust.c`: explicit CA DER with no system fallback.
- `system_trust.c`: SYSTEM_TRUST capability selection.
- `mtls.c`: explicit client certificate DER and PKCS#8 identity.
- `provider_selection.c`: EXACT, ORDERED and AUTOMATIC policies.
- `diagnostics_logging.c`: copied diagnostics and synchronous structured logging.

Build verification compiles them with `/W4`. The basic helpers are C89-compatible; modern SYSTEM_TRUST examples require a capable Schannel/OpenSSL target. Examples are education, not the cross-machine release-validation kit planned for Phase 9.F.