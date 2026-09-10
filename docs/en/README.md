<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport

[Project home](../../README.md) · [Practical guide](getting-started.md) · [Português (Brasil)](../pt-BR/README.md)

## Secure communication without locking your application to a single security library

When two programs communicate over a network, their data travels through a path that is not always under the control of the application developer.

Without adequate protection, someone with access to that path may try to read the data, modify it, or impersonate one of the communicating parties. That is why secure-transport protocols such as **TLS** exist: they encrypt communication and, through digital certificates, help verify whether a program is talking to the endpoint it is supposed to talk to, reducing the risk that information is read, altered, or delivered to the wrong person or system.

For the developer, however, using TLS usually means integrating a specific implementation directly, such as OpenSSL, Schannel, or NSS.

And that choice starts to leak into the rest of the software.

```text
Application tied directly to one implementation

┌────────────────────────────────────────┐
│            YOUR APPLICATION            │
│                                        │
│ application protocol                   │
│ business rules                         │
│ user interface                         │
│                                        │
│ + provider-specific TLS APIs           │
│ + certificates                         │
│ + library lifecycle                    │
│ + error handling                       │
│ + readiness / nonblocking              │
│ + provider-specific details            │
└──────────────────┬─────────────────────┘
                   │
                   ▼
              OpenSSL / NSS /
             Schannel / another
```

That may work perfectly well today.

But operating systems change. Libraries change. TLS versions change. **What is current today also grows old.**

**PapinhoSecureTransport (PST)** was created to place a boundary between the application and those implementations.

```text
┌────────────────────────────────────────────┐
│              YOUR APPLICATION              │
│                                            │
│ browser • e-mail • ERP • messaging        │
│ service • custom protocol • another app   │
└────────────────────┬───────────────────────┘
                     │
                     │ "I need a secure
                     │  connection with
                     │  these characteristics."
                     ▼
┌────────────────────────────────────────────┐
│         PapinhoSecureTransport             │
│                                            │
│              common contract               │
└────────────────────┬───────────────────────┘
                     │
          ┌──────────┼──────────┐
          │          │          │
          ▼          ▼          ▼
   RetroZilla NSS  Schannel   OpenSSL
          │          │          │
          └──────────┼──────────┘
                     │
                     ▼
          operating system / network
```

The application states **what it needs** without having to know the details of NSS, Schannel, or OpenSSL.

**PST sits between the application and those implementations.** It exposes the common contract used by the application, checks which capabilities are required for that connection, and routes the work to a compatible provider available in that target.

The provider takes care of **how that requirement is implemented** through the security technology it integrates.

That keeps NSS-, Schannel-, and OpenSSL-specific details behind the PST boundary instead of spreading them throughout the application's main code.

---

# A concrete example

Imagine that you are developing an **inventory-management system for a chain of stores**.

There is a client program on the store computers and a central server.

The client needs to send commands such as:

```text
LOOK_UP_PRODUCT 18472
UPDATE_STOCK 18472 35
REGISTER_INBOUND 18472 10
```

That information should not cross the network in a way that lets someone simply read or modify it.

You decide to protect the communication with TLS.

Without a layer such as PST, your program may end up integrating a security library directly:

```text
Inventory system
      │
      ├── inventory protocol
      ├── business rules
      ├── SSL_CTX
      ├── SSL
      ├── X509
      ├── WANT_READ / WANT_WRITE
      ├── OpenSSL error handling
      └── OpenSSL lifecycle
```

The security layer starts to become part of the inventory application's own implementation.

With PST:

```text
       CLIENT APPLICATION
          AT THE STORE
              │
      inventory protocol
              │
              ▼
     ┌─────────────────┐
     │       PST       │
     │   role CLIENT   │
     └────────┬────────┘
              │
             TLS
              │
           network/LAN
              │
             TLS
              ▼
     ┌─────────────────┐
     │       PST       │
     │   role SERVER   │
     └────────┬────────┘
              │
        CENTRAL SERVER
```

PST does not know what `UPDATE_STOCK` means.

It does not need to.

The business protocol still belongs to the application. PST does not need to understand the exchanged messages; it handles the layer that establishes TLS, protects data in transit, and performs secure connection shutdown.

This applies equally to Internet communication and to computers inside a LAN or a private corporate network.

---

# CLIENT and SERVER: what changes?

API 2.x makes the **connection role explicit**.

For a **CLIENT** connection, the application creates and connects the native socket and then gives PST that already-connected transport.

For a **SERVER** connection, PST does not replace the application's server architecture. The application remains responsible for:

```text
socket
  │
bind
  │
listen
  │
accept
  │
application admission / policy
  │
connected socket
  │
  ▼
 PST (role SERVER)
  │
 TLS
```

In other words, the consumer owns `bind`, `listen`, `accept`, admission, and application sessions. PST receives **only the connected transport returned by `accept`** and, after ownership is accepted, drives TLS for that connection.

That lets the same contract serve both client and server software without turning PST into an HTTP server, session framework, or authorization system.

One application may also have CLIENT and SERVER connections in the same runtime and choose different providers for each connection when the target and requested capabilities allow it.

---

# Many connections without handing your event loop to PST

API 2.1 adds an important piece without changing the project's philosophy: a portable **wait-set** can observe multiple PST connections and, on Win32, borrowed external sources such as a listening socket that remains application-owned.

```text
PST connection A ─┐
PST connection B ─┼──> wait-set ──> ready members
app listener ─────┤
wake ─────────────┘
```

A finite wait can sleep until useful work, timeout, or a cross-thread `wake`. PST does not call `accept()` or close borrowed sources. Read/write remain incremental and bounded; buffers, deadlines, and fairness remain application policy.

---

# Why not simply use OpenSSL, Schannel, or NSS directly?

You can.

PST does not exist because those technologies are bad. Quite the opposite: they are precisely the technologies that make PST possible.

The difference is **where the dependency lives**.

When you use one implementation directly, your application becomes dependent on its API, types, lifecycle, and particular behavior.

With PST, those differences remain behind one common boundary:

```text
                  YOUR APPLICATION
                        │
                     PST API
                        │
                        ▼
               ┌──────────────────┐
               │       PST        │
               └────────┬─────────┘
                        │
          ┌─────────────┼─────────────┐
          │             │             │
          ▼             ▼             ▼
        NSS          Schannel       OpenSSL
          │             │             │
          └─────────────┼─────────────┘
                        │
                        ▼
               supported platform
```

Replacing or adding a compatible provider may still require a different build, a different target, or integration work inside PST, but the change remains concentrated in the secure-transport layer instead of forcing the application's core logic to learn the native API of the new provider.

That decoupling is one of the project's most important reasons for existing.

---

# Security is also a matter of longevity

PST is also intended to **reduce how strongly software written today becomes tied to the security choices available today**, because the libraries, operating systems, and security standards we currently consider modern also age.

In practice, that separation lets the application keep using the same PST contract while the implementation responsible for security evolves over time:

```text
TODAY

Application
    │
    ▼
   PST
    │
    ▼
provider suitable for today's target


TOMORROW

Application
    │
    ▼
   PST
    │
    ▼
updated or different provider
```

If a new provider implements the capabilities required by the application, the change stays concentrated in the secure-transport layer.

That does not make software automatically eternal, nor does it guarantee that any future provider can replace any present provider without work.

But it reduces an important kind of coupling that often makes applications harder to maintain as security, operating systems, and libraries evolve.

**That benefit can grow with the community**: as more projects use and contribute to PST, new providers and new platform support can be developed once and reused by different applications. Compatibility work that might be unrealistic for the developer of a single application can become shared infrastructure.

---

# What does PST implement today?

At present, **TLS is the only secure-transport protocol implemented and contracted by PST**.

TLS can, among other things:

- encrypt transmitted data;
- verify server identity;
- optionally verify client identity too;
- negotiate secure parameters before application communication starts.

PST currently works with **TLS 1.2 and TLS 1.3**, depending on the provider, role, and target capabilities.

The current 0.6.0 release candidate uses **API 2.1.0 / SPI 3.0**. The published 0.5.0 / API 2.0.0 baseline remains historical.

### Currently validated state

| Tested on | Provider | CLIENT | SERVER | TLS 1.2 | TLS 1.3 |
|---|---|:---:|:---:|:---:|:---:|
| Windows NT 4.0 SP6 x86 | RetroZilla NSS | ✅ | ✅ | ✅ | ✅ |
| Windows 10 build 19045 x64 | Schannel | ✅ | ✅ | ✅ | SERVER not advertised |
| Windows 10 build 19045 x64 | OpenSSL 3.5.8 | ✅ | ✅ | ✅ | ✅ |
| Windows 10 build 19045 x64 | Combined Schannel/OpenSSL | ✅ | ✅ | ✅ | ✅ when OpenSSL is eligible |

This table records configurations that were actually tested, not every system on which PST or an underlying provider might work.

The packaged x64 SDKs were also built and exercised from extracted packages on a separate physical Windows 10 Pro x64 machine, without using the development checkout or a global OpenSSL installation for the test runtime.

---

# TLS 1.3 on Windows NT 4.0

One of the more interesting results obtained during PST development was validating **TLS 1.3 connections on Windows NT 4.0** through the provider based on the RetroZilla NSS/NSPR lineage.

That does not mean PST created the cryptographic TLS 1.3 implementation used by that path.

The NSS/NSPR implementation comes from upstream work in the Mozilla/RetroZilla ecosystem.

PST's contribution is its own layer: provider integration, transport abstraction, common contracts, lifecycle, ownership, readiness, policy, testing, and interoperability validation inside the project architecture.

That distinction matters both technically and historically.

---

# Providers

A **provider** connects the common PST contract to a concrete security technology.

There are currently three real providers.

## RetroZilla NSS

For the CLIENT role, validated capabilities include TLS 1.2/TLS 1.3, hostname validation, CUSTOM_TRUST, ALPN, mTLS, Peer Info, and nonblocking operation.

The SERVER role has also been validated with TLS 1.2 and TLS 1.3, including Local Identity, client-certificate authentication, CUSTOM_TRUST, mTLS, incremental I/O, reciprocal shutdown, and truncation detection.

It was validated on real Windows NT 4.0 SP6 x86.

The version used by PST derives from the RetroZilla NSS/NSPR lineage, and the project preserves and documents its origin, versions, modifications, build process, and licenses.

For the current SERVER role, `SYSTEM_TRUST` and SERVER ALPN with complete PST semantics are **not advertised**.

---

## Schannel

Schannel uses the security infrastructure supplied by Windows itself.

In the currently validated environment — Windows 10 build 19045 — the CLIENT role provides TLS 1.2, SYSTEM/CUSTOM trust, hostname validation, ALPN, mTLS, Peer Info, and nonblocking operation according to the published capability mask.

The SERVER role was validated with TLS 1.2, Local Identity, CUSTOM_TRUST, SYSTEM_TRUST, client-certificate authentication, incremental I/O, reciprocal shutdown, and truncation detection.

SERVER TLS 1.3 and SERVER ALPN with complete PST semantics are not advertised in that validated environment.

To deliver the intermediate chain for SERVER Local Identity, the backend may temporarily use `CurrentUser\CA`, preserving preexisting certificates, using reference counting, and performing controlled normal cleanup. A documented limitation remains: a crash may leave residue behind.

This describes the validated environment, not a universal statement about every Schannel version.

---

## OpenSSL

The currently validated target uses **OpenSSL 3.5.8 LTS**.

Both CLIENT and SERVER roles were validated with TLS 1.2 and TLS 1.3.

Validated capabilities include CUSTOM_TRUST, Windows SYSTEM_TRUST where advertised, hostname verification on CLIENT, ALPN, mTLS, Peer Info, nonblocking operation, bidirectional I/O, reciprocal shutdown, and truncation detection.

For `SYSTEM_TRUST` on Windows, PST combines OpenSSL TLS with trust evaluation performed through Windows certificate APIs.

For the SERVER role, Expected Peer Name does not apply; client certificates are validated for the appropriate `clientAuth` usage.

---

# What are certificates and CAs?

Before a program trusts that it is talking to the right server — or, when configured, before a server trusts a certificate presented by a client — it needs some way to verify the TLS identity of the peer.

In TLS, that usually involves a **digital certificate**.

In simplified terms, a certificate acts like an identity document presented by one endpoint.

But receiving an identity document is not enough: you also need to know **who declared it trustworthy**.

That is where **Certificate Authorities**, or **CAs**, come in.

```text
Trusted Certificate Authority
              │
              │ signs / validates
              ▼
       endpoint certificate
              │
              │ presented during TLS
              ▼
          other endpoint
```

The system checks that a valid chain of trust exists and, on CLIENT when requested, that the presented identity matches the expected name.

PST deliberately keeps four concepts separate:

- **Local Identity**: certificate chain + private key presented by this endpoint;
- **Peer Authentication**: peer certificate DISABLED, OPTIONAL, or REQUIRED;
- **Peer Trust**: exactly CUSTOM or SYSTEM;
- **Expected Peer Name**: name validation on the CLIENT role.

`CUSTOM_TRUST` and `SYSTEM_TRUST` are not silently merged or substituted for each other.

TLS authentication is also not application authorization: a valid certificate does not automatically become a user, account, or application Principal.

---

# Example: a corporate network with its own CA

Imagine the inventory system again.

The company has internal servers and its own certificate authority.

```text
             COMPANY CA
                  │
             signs certificates
                  │
                  ▼
              ERP Server
                  ▲
                  │ TLS
                  │
             ┌────┴────┐
             │   PST   │
             └────┬────┘
                  │
              ERP Client
```

The application can provide that CA to PST through `CUSTOM_TRUST`.

If the company also requires a client certificate, the PST server can configure `Peer Authentication=REQUIRED` and validate the certificate presented by the client against the configured trust.

The provider then validates the connection according to the requested policy without changing the business protocol.

---

# Selecting providers

Selection is **per connection**.

## AUTOMATIC

PST follows the target's provider registration order and chooses the first provider that is eligible for the requested role and **all** requested capabilities.

Example with the Combined target:

```text
Providers in the target:

1. Schannel
2. OpenSSL
```

SERVER request A:

```text
TLS 1.2
```

If both providers are eligible and Schannel comes first, Schannel may be selected.

SERVER request B:

```text
TLS 1.2 + SERVER ALPN
```

Because Schannel SERVER does not advertise complete PST SERVER ALPN semantics in the validated environment, it is eliminated **before binding**. OpenSSL may then be selected.

## EXACT

The application explicitly requests one provider.

If that provider cannot satisfy the requested role/policy, the operation fails.

## ORDERED

The application provides its own preference order. Ineligible providers may be discarded before binding and the next candidate may be considered.

Once a provider accepts transport binding/ownership, however, it becomes **pinned**. A later failure in handshake, certificates, trust, ALPN, I/O, transport, or shutdown does not cause PST to switch to another provider.

That is pre-binding selection, not post-handshake fallback.

---

# Initializing providers

A Win32 application explicitly registers the providers included in that target through PST's public bootstrap.

The application does not need private NSS, Schannel, or OpenSSL headers to perform that bootstrap.

The providers that can be registered are still defined by the target that was built.

```text
win32-x86-vc6-retrozilla-nss
    └── RetroZilla NSS

win32-x64-msvc-19.51-schannel
    └── Schannel

win32-x64-msvc-19.51-openssl3
    └── OpenSSL

win32-x64-msvc-19.51-schannel-openssl3
    ├── Schannel
    └── OpenSSL
```

The bootstrap is explicit: PST does not randomly search for installed libraries and does not secretly register providers behind the application's back.

---

# Expected Peer Name, SNI, and upgrading to TLS

In API 2.1, **Expected Peer Name** authenticates the certificate name while **SNI** is TLS routing information. CLIENT supports `COMPAT`, `DISABLED`, and `EXPLICIT`. OpenSSL and Schannel have validated independent control; RetroZilla NSS remains factually partial because of `SSL_SetURL`, and unsupported combinations are rejected before binding.

The 0.6.0 track also proved TLS on the **same connected transport after a plaintext phase**, covering generic STARTTLS-style and CONNECT-style flows across all three providers. SMTP, IMAP, and HTTP remain outside PST. The application must stop at a clean boundary: TLS bytes already pre-read are not recovered in this version. Once PST accepts ownership, TLS failure does not roll back to plaintext.

---

# TLS today; maybe other secure transports tomorrow

**TLS is currently the only secure-transport protocol implemented and contracted by PST.**

The architecture was designed to avoid dependency on one specific TLS implementation.

That also leaves room for other secure-transport families to be studied in the future **if there is community interest and collaboration**.

Examples include:

- DTLS;
- transports related to QUIC;
- protocols based on Noise;
- other technologies that may make sense for the project.

None of them are supported today.

There is also no guarantee that the current SPI could accept them unchanged.

Adding a new technology would require a real use case, appropriate architecture, security review, testing, maintenance, and people willing to develop it.

---

# New providers can also emerge through collaboration

The three current providers do not have to represent every possible implementation forever.

The community can propose integrations with other libraries or security technologies.

But a provider does not enter the project merely because it "works."

The project also needs to consider:

- security;
- maintenance;
- supported systems and compilers;
- licensing;
- redistribution feasibility;
- corresponding-source and notice obligations;
- dependency provenance;
- build reproducibility;
- independent interoperability testing.

In some cases, it may make more sense for users to provide a required library separately rather than for PST to redistribute it.

Each case should be evaluated individually.

---

# A note for the retrocomputing community

One important goal of the project is to help reduce the distance between older software and modern security standards.

As security standards evolve, older applications and systems can lose the ability to communicate with current services even when they remain useful for their original purpose.

One workaround is to make the modern side accept older and weaker protocol versions again.

PST helps explore a different direction:

> **how far can we bring modern security standards to older applications and systems without requiring the other side to reduce its security?**

That lets us investigate cases such as:

- older browsers;
- e-mail clients;
- corporate applications;
- client/server programs;
- specialized software;
- other systems preserved by the community.

The same architecture that helps study this bridge for older systems also reduces how tightly software written today is coupled to today's security technologies.

After all, what we call modern today will also grow old.

---

# A special invitation: NSS, NSPR, and modern TLS on older systems

There is a particularly interesting area for contribution.

The provider used by target `win32-x86-vc6-retrozilla-nss` builds on work from the **RetroZilla / Mozilla NSS / NSPR** lineage.

It would be valuable to see people interested in:

- NSS;
- NSPR;
- TLS 1.3;
- modern cryptography;
- VC6 and other historical compilers;
- Win32 on older systems;
- older Windows versions;

studying how to maintain, update, or create a reproducible maintained lineage of those technologies for older platforms.

PST does not promise to create or maintain that future lineage by itself. This is exactly the kind of area where **external collaboration can expand what the project can reach**.

---

# Where can PST be used?

Some examples:

```text
Browser
    │ HTTP
    ▼
   PST
    │ TLS
    ▼
Internet
```

```text
E-mail client
    │ SMTP / IMAP
    ▼
   PST
    │ TLS
    ▼
E-mail server
```

```text
ERP client
    │ business protocol
    ▼
   PST
    │ TLS
    ▼
LAN / corporate network
    │
    ▼
ERP server
```

```text
Custom application
    │ custom protocol
    ▼
   PST
    │ secure transport
    ▼
Another computer
```

HTTP, SMTP, IMAP, and the business protocol **are not part of PST**.

They appear only to show the kinds of software that can use the secure-transport layer.

---

# PST in the Papinho ecosystem

PST is an independent project.

Some projects in the Papinho ecosystem help illustrate different uses.

### PapinhoBrowser

Uses PST as the secure layer below HTTP/HTTPS.

### PapinhoLegacyMail

Uses PST below SMTP and IMAP.

OAuth, accounts, e-mail providers, XOAUTH2, and the e-mail protocols themselves remain the responsibility of PapinhoLegacyMail.

### PapinhoAccelerator

PapinhoAccelerator is a component **specific to PapinhoBrowser**.

It uses PST to establish the secure connection between PapinhoBrowser and PapinhoAccelerator, protecting the exchanged data.

Depending on configuration, Accelerator may also create external connections on behalf of Browser, using PST again as the secure-transport layer.

---

# Current project state

The current 0.6.0 release candidate uses **API 2.1.0 / SPI 3.0**. Through M9 it has proven wait-set, external sources, finite wait, wake, backpressure, tri-state SNI, and TLS-after-plaintext. M9 passed 9/9 TLS 1.2 pairs and 4/4 eligible TLS 1.3 pairs; M10 performs final physical/package and publication validation.

TLS 1.2 was validated with all three providers. TLS 1.3 was validated with RetroZilla NSS and OpenSSL. SERVER was validated with all three providers within their factual role-scoped capability masks.

Validation includes real Windows NT 4.0 SP6 x86 execution, Windows 10 build 19045 x64 execution, cross-provider interoperability, EXACT/ORDERED/AUTOMATIC selection, the security/lifecycle negative matrix, extracted-package consumers, and testing on a separate clean Windows machine.

Platforms outside the documented validation matrix remain unvalidated.

# Distribution

Version 0.6.0 provides a source package and separate static SDKs for:

- `win32-x86-vc6-retrozilla-nss`;
- `win32-x64-msvc-19.51-schannel`;
- `win32-x64-msvc-19.51-openssl3`;
- `win32-x64-msvc-19.51-schannel-openssl3`.

The Combined Schannel/OpenSSL package is an official optional provider-selection package; it is not a fourth TLS implementation and it is not the default recommendation.

See the [practical guide](getting-started.md), [target matrix](../target-matrix.md), [API 2.0](../api-2.0.md), [SPI 3.0](../provider-spi-3.0.md), [migration guide](../api-1.3-to-2.0-migration.md), and [release packaging documentation](../release-packaging.md).

# Security and limitations

For security boundaries, provider limitations, and details such as shutdown, truncation, trust, and Schannel SERVER adaptation, see [Security and limitations](../security-and-limitations.md).

# Development transparency

PapinhoSecureTransport was developed with the assistance of OpenAI Codex, used extensively as an engineering assistant for implementation, testing, auditing, and documentation. Architectural, product, and release decisions remained the responsibility of the project maintainer.

The repository preserves selected [engineering history and release evidence](../codex/README.md) for transparency and auditability.

# Support the project

PapinhoSecureTransport is free and open-source software under MPL-2.0. If the project is useful to you and you would like to voluntarily support the work around it, you can do so through GitHub Sponsors.

Sponsorship does not change access to the software or the rights granted by its license, and it does not constitute a contract for support, maintenance, or future development.

## License

PapinhoSecureTransport is licensed under the [Mozilla Public License 2.0](../../LICENSE). Redistributed dependencies retain their own terms; see [Third-party notices](../../THIRD_PARTY_NOTICES.md).
