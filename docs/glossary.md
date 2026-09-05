<!-- SPDX-License-Identifier: MPL-2.0 -->

# Glossary

- **Secure transport:** authenticated and encrypted byte transport below an application protocol.
- **TLS:** the secure transport implemented today.
- **Provider:** adapter to NSS, Schannel, OpenSSL, or another future engine.
- **API:** application-facing PST contract. Current frozen version: 1.3.0.
- **SPI:** internal provider contract. Current frozen version: 2.4; not a dynamic-plugin ABI.
- **Trust / root CA:** basis used to validate a peer certificate chain.
- **Hostname verification:** proof that the authenticated certificate names the intended server.
- **mTLS:** mutual TLS, where client and server authenticate with certificates.
- **ALPN:** TLS negotiation of the application protocol name.
- **Readiness:** indication that network I/O may proceed; it is not TLS progress.
- **Clean close:** authenticated TLS `close_notify`.
- **Truncation:** unexpected transport end without authenticated clean close.
- **Legacy target:** the VC6/Windows NT 4.0 build using preserved RetroZilla NSS/NSPR.