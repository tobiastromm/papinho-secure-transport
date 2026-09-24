# PapinhoSecureTransport 0.6.4 release notes

PapinhoSecureTransport 0.6.4 corrects established-connection liveness in the
public wait-set. An established idle TLS connection retains passive read
interest, allowing a remote TLS close, TCP FIN, or reset to wake the wait-set
without application polling or protocol keepalives.

The provider remains authoritative for TLS readiness and terminal
classification. Reciprocal TLS shutdown remains clean, while abrupt EOF/reset
retains the documented truncation or governed transport-error semantics.

Public API remains 2.1.0 and provider SPI remains 3.0. No public structure,
signature, provider hook, ownership rule, or capability mask changes in this
release. The explicit VC6 `/ML` and `/MD` SDK identities remain unchanged.
