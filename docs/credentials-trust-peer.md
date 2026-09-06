<!-- SPDX-License-Identifier: MPL-2.0 -->

# Credentials, trust, and peer identity

Phase 4 materializes the backend-neutral identity objects without starting the public TLS runtime from Phase 5.

`pst_credentials` answers "who am I?". Its implemented source is one X.509 leaf certificate DER plus its unencrypted PKCS#8 DER private key, supplied from memory with explicit sizes. PST copies both inputs, never logs them, and overwrites the key copy before release. Password formats, file sources, hardware and backend-native references are deferred. No test private key is stored in Git.

`pst_trust` answers "whom do I trust?". Custom CA DER memory is implemented. System trust is representable, but RetroZilla NSS returns `PST_RESULT_UNSUPPORTED`; it neither substitutes a hidden bundle nor combines trust modes.

`pst_config` contains only identity configuration in this phase. It copies the expected hostname, retains immutable credentials/trust, and becomes immutable on freeze. Freeze fails closed when required peer authentication lacks trust/hostname or required client authentication lacks credentials.

The NSS provider passes CA DER to `CERT_NewTempCertificate`/`CERT_ChangeCertTrust`, and certificate plus PKCS#8 DER to `CERT_NewTempCertificate`/`PK11_ImportDERPrivateKeyInfoAndReturnKey`. `SSL_GetClientAuthDataHook` supplies NSS-owned duplicates. NSS alone parses X.509/PKCS#8, performs private-key operations, builds and validates the chain, and verifies hostname through `SSL_SetURL` and `SSL_AuthCertificate`.

Configuration is one-shot before attachment. Failure returns no usable connection; destruction releases every acquired NSS resource. On success, SSL closes first, followed by certificate, key, anchor and copied hostname.

After authenticated handshake, NSS supplies the leaf certificate, channel information, and SHA-256 through `SSL_PeerCertificate`, `SSL_GetChannelInfo`, and `PK11_HashBuf`. `pst_peer_info` owns its summary and leaf DER copies and survives destruction of connection/config/credentials/trust. Its four-state facts distinguish unknown, false, true and unsupported. ALPN and early data are unsupported here; full chains and textual DN parsing are deferred.

Functional evidence on Windows 10 Pro 10.0.19045 x64 used OpenSSL 3.5.7 only to generate ephemeral fixtures and a Python standard-library TLS 1.2 loopback server requiring a client certificate. The VC6 Win32 PST client completed mTLS, authenticated `localhost`, captured a 790-byte leaf and 32-byte SHA-256, echoed 27 bytes, and shut down. Wrong hostname returned `PST_RESULT_HOSTNAME_MISMATCH`; unrelated CA returned `PST_RESULT_AUTH_FAILURE`. Both failed closed.

The process loaded `nspr4.dll`, `nss3.dll`, `ssl3.dll`, `nssutil3.dll`, `plc4.dll`, `plds4.dll`, `softokn3.dll`, and `freebl3.dll` only from the versioned runtime. The canonical `freebl3.dll`/`.chk` SHA-256 values remained `12808c651528c9e08f5ccf86af00f9061b19103c0c712d376755664f41ee474d` and `122aefebbfd76eb68352eb23d59d4caff70183b520f81b5017bf299f7b745daa`.
