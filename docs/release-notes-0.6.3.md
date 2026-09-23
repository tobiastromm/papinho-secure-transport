# PapinhoSecureTransport 0.6.3 release notes

PapinhoSecureTransport 0.6.3 is a focused RetroZilla NSS peer-information
normalization correction. Public API remains 2.1.0 and provider SPI remains
3.0.

The RetroZilla NSS provider now reports negotiated TLS versions through the
public PST vocabulary: TLS 1.2 as `PST_TLS_VERSION_1_2` and TLS 1.3 as
`PST_TLS_VERSION_1_3`. Native NSS protocol numbers are no longer exposed to
consumers. A native version outside the public PST TLS 1.2/TLS 1.3 vocabulary
makes that peer-information metadata unavailable without failing an otherwise
valid connection.

The release preserves the explicit VC6 `/ML` and `/MD` SDK identities introduced
in 0.6.2. No API or SPI layout, signature, provider hook, or public TLS-version
enum expansion is included.
