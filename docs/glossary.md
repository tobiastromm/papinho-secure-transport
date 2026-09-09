<!-- SPDX-License-Identifier: MPL-2.0 -->

# Glossary

- **Secure transport:** authenticated and encrypted byte transport below an application protocol.
- **TLS:** the secure transport implemented today.
- **Provider:** adapter to NSS, Schannel, OpenSSL, or another future engine.
- **API:** application-facing PST contract. Current frozen version: 2.0.0.
- **SPI:** internal provider contract. Current frozen version: 3.0; not a dynamic-plugin ABI.
- **Trust / root CA:** basis used to validate a peer certificate chain.
- **Hostname verification:** proof that the authenticated certificate names the intended server.
- **mTLS:** mutual TLS, where client and server authenticate with certificates.
- **ALPN:** TLS negotiation of the application protocol name.
- **Readiness:** indication that network I/O may proceed; it is not TLS progress.
- **Clean close:** authenticated TLS `close_notify`.
- **Truncation:** unexpected transport end without authenticated clean close.
- **`win32-x86-vc6-retrozilla-nss`:** the canonical target ID for the VC6 x86 build using preserved RetroZilla NSS/NSPR; Windows NT 4.0 SP6 x86 is a validated operating system for this target.
