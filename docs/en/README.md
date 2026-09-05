# PapinhoSecureTransport

## Secure communication without tying your application to a single security library

When two programs communicate over a network, their data travels through
a path that is not always under the application developer's control.

Without proper protection, someone with access to that path may try to
read or alter the data, or impersonate one of the parties. This is why
secure transport protocols such as **TLS** exist: they encrypt
communication and use digital certificates to help verify that a program
is communicating with the intended party, reducing the risk that
information is read, changed, or delivered to the wrong person or
system.

For developers, the problem is that using TLS usually means integrating
a specific implementation such as OpenSSL, Schannel, or NSS directly
into the application. That choice then starts spreading into the rest of
the software.

``` text
Application tied directly to one implementation

┌────────────────────────────────────────┐
│            YOUR APPLICATION            │
│ application protocol • business rules │
│ + TLS-specific APIs                    │
│ + certificates • lifecycle • errors   │
│ + readiness • provider details        │
└──────────────────┬─────────────────────┘
                   ▼
          OpenSSL / NSS / Schannel
```

That may work very well today. But operating systems change. Libraries
change. TLS versions change. What is current today eventually becomes
legacy.

**PapinhoSecureTransport (PST)** was created to place a boundary between
the application and those implementations.

``` text
┌────────────────────────────────────────────┐
│              YOUR APPLICATION              │
│ browser • e-mail • ERP • messaging        │
│ service • custom protocol • another app   │
└────────────────────┬───────────────────────┘
                     │ what it needs
                     ▼
┌────────────────────────────────────────────┐
│         PapinhoSecureTransport             │
│              common contract               │
└────────────────────┬───────────────────────┘
                     │
           ┌─────────┼─────────┐
           ▼         ▼         ▼
    RetroZilla NSS Schannel  OpenSSL
                     │
                     ▼
             operating system / network
```

The application states **what it needs** without having to know NSS,
Schannel, or OpenSSL details. **PST sits between the application and
those implementations**, provides the common contract, checks required
capabilities, and routes work to a compatible provider. The provider
handles **how the work is performed**.

------------------------------------------------------------------------

# A concrete example

Imagine an **inventory management system for a chain of stores**. A
client program sends commands to a central server:

``` text
QUERY_PRODUCT 18472
UPDATE_STOCK 18472 35
REGISTER_INCOMING 18472 10
```

Those messages should not cross the network in a form that lets another
party read or modify them. You decide to protect the connection with
TLS.

Without PST, business code can become mixed with `SSL_CTX`, `SSL`,
`X509`, `WANT_READ / WANT_WRITE`, provider error handling, and provider
lifecycle rules.

With PST:

``` text
       CLIENT APPLICATION
           AT THE STORE
              │
       inventory protocol
              │
              ▼
     ┌─────────────────┐
     │       PST       │
     │ protects data   │
     └────────┬────────┘
              │ TLS
           network/LAN
              │ TLS
              ▼
     ┌─────────────────┐
     │ CENTRAL SERVER  │
     │   TLS server    │
     └─────────────────┘
```

PST does not know what `UPDATE_STOCK` means. It does not need to. The
business protocol remains the application's responsibility; PST handles
the layer that establishes TLS, protects data in transit, and handles
secure shutdown. This applies to Internet connections, LANs, and private
corporate networks.

------------------------------------------------------------------------

# Why not simply use OpenSSL, Schannel, or NSS directly?

You can. PST does not exist because those technologies are bad; they are
what make PST possible. The difference is **where the dependency
lives**.

``` text
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
          ▼             ▼             ▼
        NSS          Schannel       OpenSSL
                        │
                        ▼
                supported platform
```

Replacing or adding a compatible provider may still require a new build,
a new target, or integration work inside PST, **but that change remains
concentrated in the secure-transport layer instead of requiring the
application's main logic to be rewritten around the new provider's
native API**.

------------------------------------------------------------------------

# Security is also a longevity problem

PST is intended to **reduce how strongly a program written today is tied
to the security-layer choices made when it was developed**, because
libraries, operating systems, and security standards that are current
today will also age.

``` text
TODAY                         TOMORROW
Application                   Application
    │                             │
    ▼                             ▼
   PST                           PST
    │                             │
    ▼                             ▼
provider for today's target   updated/different provider
```

This does not make software automatically eternal, but it reduces an
important kind of coupling. **The benefit can grow with the community:**
providers and platform ports developed once can be reused by multiple
applications using the same contract.

------------------------------------------------------------------------

# What does PST implement today?

**TLS is currently the only secure-transport protocol implemented and
contracted by PST.** PST currently works with **TLS 1.2 and TLS 1.3**,
depending on provider and target capabilities.

  ---------------------------------------------------------------------------
  Validated   Provider         TLS 1.2          TLS 1.3        System trust
  system /                                                   
  target                                                     
  ----------- ------------ ---------------- ---------------- ----------------
  Windows NT  RetroZilla          ✅               ✅              ---
  4.0 SP6 x86 NSS                                            

  Windows 10  Schannel            ✅         not advertised         ✅
  build 19045                                   in this      
  x64                                         environment    

  Windows 10  OpenSSL             ✅               ✅               ✅
  build 19045 3.5.8                                          
  x64                                                        

  Windows 11  ---           ⏳ validation    ⏳ validation          ⏳
                               pending          pending      

  Windows     ---           ⏳ validation    ⏳ validation          ⏳
  Server                       pending          pending      
  ---------------------------------------------------------------------------

This table represents **what the project has actually validated**, not
everything each operating system or library may theoretically support.

------------------------------------------------------------------------

# TLS 1.3 on Windows NT 4.0

One of the most interesting results obtained during PST development was
validating **TLS 1.3 connections on Windows NT 4.0** through the
provider based on the RetroZilla NSS/NSPR lineage.

PST did not create the TLS 1.3 cryptographic implementation used in that
path. NSS/NSPR comes from upstream Mozilla/RetroZilla work. PST provides
its own integration layer, transport abstraction, contracts, lifecycle,
ownership, readiness, policies, tests, and interoperability validation.

------------------------------------------------------------------------

# Providers

A **provider** connects PST's common contract to a concrete security
technology.

## RetroZilla NSS

Validated capabilities include TLS 1.2, TLS 1.3, hostname verification,
custom trust, ALPN, mTLS, normalized peer information, and nonblocking
operation. It has been validated on Windows NT 4.0. The project
preserves and documents the origin, versions, modifications, build
process, and licenses of the RetroZilla NSS/NSPR lineage it uses.

The current provider does not expose `SYSTEM_TRUST` through PST and has
an implementation-specific singleton limitation.

## Schannel

Uses Windows security infrastructure. On Windows 10 build 19045, PST
validated TLS 1.2, system trust, custom trust, hostname verification,
ALPN, mTLS, peer info, and nonblocking operation. TLS 1.3 is not
advertised by PST in this tested environment; this is not a universal
statement about Schannel.

## OpenSSL

The validated target uses **OpenSSL 3.5.8 LTS** and supports TLS 1.2,
TLS 1.3, custom trust, Windows system trust, hostname verification,
ALPN, mTLS, peer info, and nonblocking operation. For `SYSTEM_TRUST`,
PST combines OpenSSL TLS with Windows certificate-policy evaluation.

------------------------------------------------------------------------

# What are certificates and CAs?

A TLS server normally presents a **digital certificate** as evidence of
identity. A **Certificate Authority (CA)** is an authority whose
signatures can establish a chain of trust for that certificate.

``` text
Trusted CA
    │ signs / validates
    ▼
server certificate
    │ presented during TLS
    ▼
application
```

Depending on the provider, PST supports **SYSTEM_TRUST** (the operating
system's trust policy) and **CUSTOM_TRUST** (authorities explicitly
supplied by the application, such as a corporate CA).

------------------------------------------------------------------------

# Example: corporate network with its own CA

``` text
COMPANY CA
    │
    ▼
ERP Server
    ▲ TLS
    │
   PST
    │
ERP Client
```

The application can supply that CA through `CUSTOM_TRUST`; the business
protocol does not need to change.

------------------------------------------------------------------------

# Selecting providers

## AUTOMATIC

PST uses the target order and chooses the first provider that has
**all** requested capabilities. In the validated Combined target the
order is Schannel, then OpenSSL. `TLS 1.2 + SYSTEM_TRUST` selects
Schannel; `TLS 1.3 + SYSTEM_TRUST` selects OpenSSL because Schannel
lacks proven TLS 1.3 capability in the tested Windows 10 environment.

## EXACT

The application explicitly requests a provider. If it cannot satisfy the
requested policy, the operation fails. PST **does not silently switch
providers**.

## ORDERED

The application supplies its own preference order.

------------------------------------------------------------------------

# Initializing providers

Since API 1.3.0, Win32 applications can register the providers compiled
into their target with:

``` c
pst_win32_register_builtin_providers();
```

``` text
Legacy target   → RetroZilla NSS
Schannel target → Schannel
OpenSSL target  → OpenSSL
Combined target → Schannel + OpenSSL
```

The bootstrap is explicit: PST does not randomly discover libraries
installed on the computer.

------------------------------------------------------------------------

# TLS today; other secure transports perhaps tomorrow

**TLS is currently the only secure-transport protocol implemented and
contracted by PST.** If there is **community interest and
collaboration**, other secure-transport families may be researched in
the future, such as DTLS, QUIC-related transports, Noise-based
protocols, or other technologies that fit real use cases.

None is supported today, and the current SPI is not guaranteed to
support them without changes.

------------------------------------------------------------------------

# New providers can also emerge through collaboration

The community can propose integrations with other security libraries. A
provider must be evaluated for security, maintenance, supported
systems/toolchains, licensing, redistribution, source/notices
obligations, dependency origin/history (provenance), reproducibility,
and interoperability testing.

------------------------------------------------------------------------

# Note to the retrocomputing community

An important goal is to reduce the distance between legacy software and
modern security standards. Instead of making the modern side accept
weaker legacy protocol versions, PST helps explore another direction:

> **How far can we bring modern security standards to old applications
> and systems without requiring the other side to lower its security?**

This can matter for browsers, e-mail clients, corporate applications,
client/server programs, specialized software, and other preserved
systems. The same architecture also helps applications written today
avoid unnecessary coupling to today's security technologies.

------------------------------------------------------------------------

# A special invitation: NSS, NSPR, and modern TLS on old systems

The legacy provider uses work from the **RetroZilla / Mozilla NSS /
NSPR** lineage. Research into NSS, NSPR, TLS 1.3, modern cryptography,
VC6 and other historical compilers, legacy Win32, and old Windows
versions could help create or maintain reproducible security stacks for
older platforms.

PST does not promise to build or maintain that future lineage alone.
This is an area where external collaboration can expand what the project
can achieve.

------------------------------------------------------------------------

# Where can PST be used?

``` text
Browser      → HTTP          → PST → TLS → Internet
E-mail client→ SMTP / IMAP   → PST → TLS → mail server
ERP client   → business proto→ PST → TLS → corporate network/server
Custom app   → custom proto  → PST → secure transport → another computer
```

------------------------------------------------------------------------

# PST in the Papinho ecosystem

PST is an independent project.

### PapinhoBrowser

Uses PST as the secure layer below HTTP/HTTPS.

### PapinhoLegacyMail

Uses PST below SMTP and IMAP. OAuth, accounts, mail providers, XOAUTH2,
and the mail protocols themselves remain PapinhoLegacyMail
responsibilities.

### PapinhoAccelerator

PapinhoAccelerator is **specific to PapinhoBrowser**. PST protects the
connection between Browser and Accelerator. Depending on configuration,
the Accelerator may also make external connections on behalf of the
Browser and use PST again as its secure-transport layer.

------------------------------------------------------------------------

# Current project state

PST currently has three functional providers; TLS 1.2 validated on all
three; TLS 1.3 validated on RetroZilla NSS and OpenSSL; public API
1.3.0; provider SPI 2.4; isolated builds for different targets;
cross-implementation interoperability tests; and real validation on
Windows NT 4.0 and Windows 10 build 19045.

The first stabilized public distribution is still being prepared, so
some platforms and packaging formats remain **pending validation**
rather than being presented as guaranteed support.
