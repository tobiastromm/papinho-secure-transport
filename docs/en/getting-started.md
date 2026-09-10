<!-- SPDX-License-Identifier: MPL-2.0 -->

# Getting Started with PapinhoSecureTransport

> **This is the practical PapinhoSecureTransport guide.**
>
> If this is your first time encountering the project, we recommend starting with the
> [full English introduction](README.md), which explains the problem PST solves,
> its architecture, and the main concepts.
>
> [← English introduction](README.md) ·
> [Repository main README](../../README.md)

This guide takes you from preparing the environment to your first **CLIENT and SERVER** TLS connections using only the public PapinhoSecureTransport API.

By the end, you will have seen how to:

- choose the appropriate target;
- build PST;
- understand the generated files;
- integrate PST into your program;
- register the available providers;
- create CLIENT and SERVER role connections;
- establish a TLS connection;
- understand who owns `bind`, `listen`, and `accept` on the SERVER side;
- choose between system trust and a private CA;
- use mutual authentication (mTLS);
- select providers per connection;
- understand role-scoped capabilities;
- handle diagnostics and logging;
- perform TLS shutdown correctly.

You do not need to know OpenSSL, Schannel, or NSS to get started. When a TLS concept is needed, it is introduced where it becomes relevant.

---

# 1. What you need

Before building PST, it is important to understand that **there is not one single build for every computer**.

The project has different targets because platforms, architectures, compilers, and security implementations differ.

You do not need to prepare every environment. **Choose the target you intend to use and install only the tools required for that target.**

The four targets in the 0.6.0 release track are:

| Target | Provider(s) | Architecture | Actually validated environment |
|---|---|---:|---|
| `win32-x86-vc6-retrozilla-nss` | RetroZilla NSS/NSPR | x86 | Windows NT 4.0 SP6 x86 |
| `win32-x64-msvc-19.51-schannel` | Schannel | x64 | Windows 10 build 19045 x64 |
| `win32-x64-msvc-19.51-openssl3` | OpenSSL 3.5.8 LTS | x64 | Windows 10 build 19045 x64 |
| `win32-x64-msvc-19.51-schannel-openssl3` | Schannel + OpenSSL 3.5.8 | x64 | Windows 10 build 19045 x64 |

A target identifier describes durable build facts — platform/ABI, architecture, toolchain, and provider. It is **not** a declaration of operating-system version.

For example:

```text
win32-x86-vc6-retrozilla-nss
```

does not mean “NT4-only.” Windows NT 4.0 appears in the documentation because it is a system on which this target was actually validated.

Dependencies required by providers, such as the project-prepared RetroZilla NSS/NSPR runtime and OpenSSL 3.5.8, are already versioned and maintained by the project.

> **This guide uses the versioned dependencies prepared by the project.** Rebuilding OpenSSL, RetroZilla NSS/NSPR, or other dependencies from their own source trees is not required to start using PST.

So, to follow this guide, you primarily need to prepare **the compiler corresponding to the target you chose**. The following steps use the dependencies already provided by the project.

---

# 2. Choose your target

If you do not yet know which target to use, do not begin by choosing a library.

Start by asking:

> **Which platform will my application run on, and which security features does it require?**

That is one of the central ideas behind PST.

Instead of starting with:

```text
"I want OpenSSL."
```

the application can start from its requirements:

```text
"I need TLS 1.3."
"I want to use certificates trusted by the operating system."
"I need to authenticate the client too."
"I need to accept connections as SERVER."
"I need ALPN on the SERVER side."
```

From those requirements, you can choose a target containing a provider capable of satisfying them.

## `win32-x86-vc6-retrozilla-nss`

This is the x86/VC6 target using RetroZilla NSS/NSPR.

In the configuration actually validated on Windows NT 4.0 SP6 x86, the following were proven:

```text
CLIENT
  ├── TLS 1.2
  └── TLS 1.3

SERVER
  ├── TLS 1.2
  └── TLS 1.3
```

Depending on the role, CUSTOM_TRUST, Local Identity, peer-certificate authentication, mTLS, Peer Info, incremental I/O, reciprocal shutdown, and truncation detection were also validated.

For the current SERVER role:

```text
SYSTEM_TRUST          → not advertised
complete SERVER ALPN  → not advertised
```

Those are factual capabilities of the current provider under the PST contract.

## `win32-x64-msvc-19.51-schannel`

Schannel uses the security infrastructure supplied by Windows itself.

In the Windows 10 build 19045 x64 environment validated by the project:

```text
CLIENT
  └── TLS 1.2

SERVER
  └── TLS 1.2
```

SYSTEM_TRUST and CUSTOM_TRUST are available according to the published capability masks for each role.

SERVER TLS 1.3 and SERVER ALPN with complete PST semantics are not advertised in that environment.

This **does not** mean that Schannel is universally limited to TLS 1.2. It is a statement about what PST actually validated in that environment.

## `win32-x64-msvc-19.51-openssl3`

The OpenSSL target uses **OpenSSL 3.5.8 LTS**.

The following were validated:

```text
CLIENT
  ├── TLS 1.2
  └── TLS 1.3

SERVER
  ├── TLS 1.2
  └── TLS 1.3
```

Where advertised, the provider also supports CUSTOM_TRUST, Windows SYSTEM_TRUST, ALPN, mTLS, Peer Info, nonblocking operation, reciprocal shutdown, and truncation detection.

## `win32-x64-msvc-19.51-schannel-openssl3`

There is also a Combined target:

```text
              Application
                  │
        connection configuration
                  │
                  ▼
                 PST
                  │
        ┌─────────┴─────────┐
        ▼                   ▼
    Schannel             OpenSSL
```

This target is useful when the application wants PST to choose between more than one provider.

Selection happens **per connection**.

SERVER example:

```text
TLS 1.2
```

may leave both Schannel and OpenSSL eligible.

But:

```text
TLS 1.2 + SERVER ALPN
```

eliminates Schannel before binding in the validated environment because Schannel SERVER does not advertise complete PST SERVER ALPN semantics. OpenSSL may then be selected.

Combined is an official optional package. It is not a fourth TLS implementation and is not the default recommendation for every user.

## What about Windows 11 or Windows Server?

Those platforms still need to go through the project's formal validation matrix.

This guide therefore does not assume support merely because we expect a configuration to work technically.

Today we can state:

```text
Windows NT 4.0 SP6 x86     → validated for the NSS/VC6 target
Windows 10 build 19045 x64 → validated for the current x64 targets

Windows 11                 → not yet formally validated for this version
Windows Server             → not yet formally validated for this version
```

As new platforms are actually tested, the documentation can be expanded.

---

# 3. Build PST

Now that you have chosen a target, it is time to turn the PST source tree into the library your application can use.

Conceptually, the process is the same for every target:

```text
PST repository
        │
        │ choose target
        ▼
script / build system
        │
        │ builds PST + provider for that target
        ▼
PST library
        │
        └── runtime dependencies, when needed
```

Build outputs are kept separate. This avoids mixing incompatible artifacts produced by different architectures and toolchains.

> Run the commands below **from the PapinhoSecureTransport repository root**.

## x86 / VC6 / RetroZilla NSS

The build wrapper prepares the VC6 environment through `tools\vc6-env.bat` and invokes `Makefile.vc6`.

To clean:

```bat
tools\build-vc6.bat clean
```

To build and run the canonical suite:

```bat
tools\build-vc6.bat test
```

The PST library for this target is produced at:

```text
build\win32-x86-vc6-retrozilla-nss\papinho_secure_transport.lib
```

The RetroZilla NSS/NSPR dependencies used by this path are already prepared and versioned by the project.

## x64 / Schannel

To clean:

```bat
tools\build-win32-x64-msvc-19.51-schannel.bat clean
```

To build and test:

```bat
tools\build-win32-x64-msvc-19.51-schannel.bat test
```

The library is produced at:

```text
build\win32-x64-msvc-19.51-schannel\papinho_secure_transport.lib
```

Because Schannel is part of Windows, this target does not distribute a PST-provided Schannel DLL.

## x64 / OpenSSL 3.5.8

To clean:

```bat
tools\build-win32-x64-msvc-19.51-openssl3.bat clean
```

To build and test:

```bat
tools\build-win32-x64-msvc-19.51-openssl3.bat test
```

The library is produced at:

```text
build\win32-x64-msvc-19.51-openssl3\papinho_secure_transport.lib
```

The build also copies the OpenSSL DLLs required at runtime into the output directory:

```text
libssl-3-x64.dll
libcrypto-3-x64.dll
```

## x64 / Combined Schannel + OpenSSL

To clean:

```bat
tools\build-win32-x64-msvc-19.51-schannel-openssl3.bat clean
```

To build and run the Combined-specific test:

```bat
tools\build-win32-x64-msvc-19.51-schannel-openssl3.bat combined-test
```

The library is produced at:

```text
build\win32-x64-msvc-19.51-schannel-openssl3\papinho_secure_transport.lib
```

This target also uses:

```text
libssl-3-x64.dll
libcrypto-3-x64.dll
```

for its OpenSSL provider.

## Do I need to run the tests every time?

During PST development, test targets are used to make sure the build remains healthy.

For someone starting with the project, running the tests on the first build is a good way to verify that:

```text
toolchain
   +
dependencies
   +
PST
   +
provider
   │
   ▼
working environment
```

Some integration tests require specific conditions such as network access, fixtures, or execution on another system, and may remain separate from the normal offline suite.

---

# 4. What was generated?

After a successful build, each target produces its own PST library:

```text
build\
├── win32-x86-vc6-retrozilla-nss\
│   └── papinho_secure_transport.lib
│
├── win32-x64-msvc-19.51-schannel\
│   └── papinho_secure_transport.lib
│
├── win32-x64-msvc-19.51-openssl3\
│   └── papinho_secure_transport.lib
│
└── win32-x64-msvc-19.51-schannel-openssl3\
    └── papinho_secure_transport.lib
```

Although the `.lib` filename is the same, **these libraries are not interchangeable**.

Each one was built for a different target and contains the providers associated with that target.

```text
papinho_secure_transport.lib
          │
          ├── NSS / x86 / VC6 target
          │      └── RetroZilla NSS
          │
          ├── Schannel / x64 target
          │      └── Schannel
          │
          ├── OpenSSL / x64 target
          │      └── OpenSSL
          │
          └── Combined / x64 target
                 ├── Schannel
                 └── OpenSSL
```

## Public headers

The public headers live in:

```text
include\
├── papinho_secure_transport.h
└── papinho_secure_transport_win32.h
```

A Win32 application will typically start with:

```c
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
```

Your application should use **only the public headers under `include\`**.

It should not need to include files from:

```text
src\
src\backends\
third_party\
```

to use PST.

## What is the `.lib` for?

During your application build:

```text
your_program.c
      │
      │ compilation
      ▼
your_program.obj
      │
      ├──── papinho_secure_transport.lib
      │
      ▼
    linker
      │
      ▼
your_program.exe
```

The `.lib` is used during **linking**.

It is not, by itself, a file that end users need to place beside the `.exe`.

## What about DLLs?

That depends on the provider.

### Schannel

Schannel is supplied by Windows:

```text
your_application.exe
        │
        ▼
       PST
        │
        ▼
 Windows Schannel
```

### OpenSSL

The currently validated OpenSSL target uses:

```text
libssl-3-x64.dll
libcrypto-3-x64.dll
```

Those DLLs must be available when the application using that provider runs.

### RetroZilla NSS/NSPR

The NSS target uses the NSS/NSPR runtime prepared by the project. You do not need to rebuild it to follow this guide.

---

# 5. The public API: the API 2.1 mental model

API 2.1 preserves the API 2.0 concepts and adds multiplexed readiness without handing the event loop to PST.

```text
registered providers
        │
        ▼
     runtime
        │
        ├── CLIENT connection
        ├── SERVER connection
        └── other connections
```

The runtime acts as a context/provider registry that can be shared by connections.

That **does not** mean that runtime or connection objects are automatically thread-safe for concurrent calls from multiple threads.

Each connection receives its own complete configuration.

The main elements include:

- CLIENT or SERVER role;
- TLS policy;
- ALPN policy;
- Local Identity;
- Peer Authentication;
- Peer Trust;
- Expected Peer Name, for CLIENT;
- provider selection policy.

Configuration is copied/snapshotted by PST; the application should not rely on mutating it later as a mechanism for altering an already-created connection.

---

# 6. Registering providers

On Win32, the application explicitly registers the providers that were compiled into that target through PST's public bootstrap.

Conceptually:

```c
pst_win32_register_builtin_providers();
```

The application does not need private NSS, Schannel, or OpenSSL headers.

PST also does not go searching for arbitrary security libraries installed on the machine.

The target defines which providers are available:

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

After bootstrap, create the runtime and then create connections.

---

# 7. Your first CLIENT connection

For the CLIENT role, the application remains responsible for creating and connecting the TCP transport.

The general flow is:

```text
socket()
   │
connect()
   │
connected socket
   │
   ▼
PST connection (CLIENT)
   │
attach transport
   │
TLS handshake
   │
read / write
   │
TLS shutdown
```

PST does not perform DNS, HTTP, SMTP, IMAP, or any other application protocol for you.

A minimal real example is available at:

```text
examples\basic_client.c
```

The general idea is:

1. register the providers;
2. create the runtime;
3. create a configuration with `role=CLIENT`;
4. define TLS/trust/hostname policy;
5. create the connection;
6. provide the already-connected socket;
7. advance the handshake incrementally;
8. exchange data;
9. query negotiated information if needed;
10. perform reciprocal shutdown;
11. release the connection and runtime.

---

# 8. Your first SERVER connection

For the SERVER role, there is one important boundary:

> **PST does not create the listener and does not perform `bind`, `listen`, or `accept`.**

Those operations belong to the application.

The flow is:

```text
socket()
   │
bind()
   │
listen()
   │
accept()
   │
connected socket
   │
   ▼
PST connection (SERVER)
   │
attach transport
   │
TLS handshake
   │
read / write
   │
TLS shutdown
```

A minimal real example is available at:

```text
examples\basic_server.c
```

The application decides:

- which interface/port to listen on;
- how many listeners to create;
- which connections to admit;
- how to organize threads, event loops, or processes;
- which application protocol exists above TLS;
- how users or sessions are authorized.

PST begins **after an already-connected transport exists**.

## Local Identity on SERVER

A TLS server normally needs to present a certificate and private key.

In PST, that is **Local Identity**.

It is separate from:

- Peer Authentication;
- Peer Trust;
- application authorization.

If the server only presents its own certificate and does not require a client certificate:

```text
Local Identity      → configured
Peer Authentication → DISABLED
```

For mTLS:

```text
Local Identity      → server certificate/key
Peer Authentication → REQUIRED
Peer Trust          → CUSTOM or SYSTEM, depending on provider/capability
```

---

# 9. Local Identity, Peer Authentication, Peer Trust, and Expected Peer Name

These concepts are independent.

## Local Identity

This is what **this endpoint presents**:

```text
certificate/chain
+
private key
```

It may exist on CLIENT, SERVER, or both depending on the use case.

## Peer Authentication

Controls whether the peer certificate is:

```text
DISABLED
OPTIONAL
REQUIRED
```

## Peer Trust

When the peer certificate needs to be validated, trust is configured as:

```text
CUSTOM
```

or:

```text
SYSTEM
```

PST does not silently union the two and does not automatically replace one with the other.

## Expected Peer Name

This is independent peer-name verification on the CLIENT role.

Example:

```text
server presents certificate
        │
        ├── trusted chain?     → Peer Trust
        └── for example.com?   → Expected Peer Name
```

Expected Peer Name does not apply on SERVER.

## Authenticated does not mean authorized

Even if TLS confirms that the peer presented a valid certificate, the application still decides what that peer is allowed to do.

```text
TLS:
"valid authenticated certificate"

Application:
"which account does this certificate map to?"
"may that account perform this operation?"
```

PST does not automatically turn certificates into users, accounts, or application Principals.

---

# 10. SYSTEM_TRUST and CUSTOM_TRUST

## SYSTEM_TRUST

Uses the operating-system trust policy/store when the provider and role advertise that capability.

This is useful when you want the application to integrate with trust administered by Windows.

## CUSTOM_TRUST

The application explicitly provides its own CA or CA set.

Example:

```text
COMPANY CA
     │
     ├── internal server
     └── corporate client
```

This is especially useful for private networks, labs, and enterprise PKIs.

Not every provider offers SYSTEM_TRUST in every role.

For example, in PST 0.5.0:

```text
RetroZilla NSS SERVER SYSTEM_TRUST → not advertised
```

Always consult the capability mask or the [Target Matrix](../target-matrix.md).

---

# 11. mTLS

TLS can authenticate only the server:

```text
CLIENT
   │
   │ verifies server
   ▼
SERVER
```

or both sides:

```text
CLIENT ⇄ SERVER
certificate certificate
```

The second case is commonly called **mTLS — mutual TLS**.

In PST, mTLS emerges from separate identity and peer-authentication configuration.

On SERVER:

```text
Local Identity      → server identity
Peer Authentication → REQUIRED
Peer Trust          → trust used to validate the client
```

On CLIENT:

```text
Local Identity      → client identity
Peer Authentication → REQUIRED
Peer Trust          → trust used to validate the server
Expected Peer Name  → expected server name
```

Client-certificate authentication on SERVER uses the appropriate `clientAuth` purpose.

---

# 12. Selecting providers

API 2.x keeps provider selection **per connection**.

There are three modes.

## EXACT

Request one specific provider.

```text
EXACT = openssl
```

If it is not eligible for that configuration:

```text
failure
```

PST does not silently switch to another provider.

## ORDERED

Provide your own list:

```text
1. openssl
2. schannel
```

PST considers providers in that order and may skip a candidate **before binding** when it is not eligible for the requested role/capabilities.

## AUTOMATIC

PST uses the target's registration order.

For the current Combined target:

```text
1. Schannel
2. OpenSSL
```

and selects the first eligible provider.

## Role-scoped capabilities

A provider may offer one capability on CLIENT but not on SERVER.

That is why PST exposes separate role-scoped masks.

Example:

```text
SERVER + TLS 1.2 + SERVER ALPN
```

On the validated Combined target:

```text
Schannel → ineligible before binding
OpenSSL  → eligible
```

## There is no fallback after binding

This is important.

Once the provider accepts transport binding/ownership, it is pinned.

If a later failure occurs in:

- handshake;
- certificate validation;
- trust;
- ALPN;
- I/O;
- truncation;
- transport;
- shutdown;

PST **does not try another provider**.

That prevents a security or protocol failure from turning into an implicit downgrade.

---

# 13. ALPN

ALPN lets TLS negotiate an application protocol during the handshake.

Typical example:

```text
CLIENT offers:
h2
http/1.1
```

On CLIENT, the list is an **ordered offer**.

On SERVER, the list expresses the **server's local preference**.

The server selects the first item in its preference order that was also offered by the client.

The public modes are:

```text
REQUIRED
OPTIONAL
DISABLED
```

Not every provider offers complete ALPN support in both roles.

In PST 0.5.0:

```text
OpenSSL SERVER ALPN              → supported
Schannel complete SERVER ALPN    → not advertised
RetroZilla NSS complete SERVER ALPN → not advertised
```

---

# 14. Incremental handshake and readiness

PST is designed for incremental/nonblocking operation.

A handshake step may conceptually return:

```text
NEED_READ
NEED_WRITE
NEED_READ_WRITE
```

That does not mean failure, and it does not mean completion.

It means the operation needs to be retried later when the appropriate condition exists.

A backend may have provider-specific readiness requirements. Therefore, an application should not assume that observing the native socket directly is always enough to represent internal TLS interest for every provider.

Use the public PST interest/wait operations.

---

# 15. Wait-set, external sources, wake, and backpressure

API 2.1 provides a portable **wait-set** for observing multiple connections through consumer-defined tokens. On Win32, `pst_external_source` can include a borrowed native source such as a listener without transferring ownership.

`timeout=0` performs an immediate poll; a positive timeout is the scheduler's maximum wait. `wake` may be called from another thread and is distinct from timeout and cancellation. The application still owns its deadlines.

Read/write may make partial progress. If `pst_write` accepts only part of a buffer, the application retains the unsent suffix and resumes it when readiness returns. PST does not implement implicit send-all and does not drain one connection indefinitely.

---

# 16. Read and write

After the handshake:

```text
application
   │
pst_write
   │
 TLS
   │
network
```

and:

```text
network
   │
 TLS
   │
pst_read
   │
application
```

PST protects bytes.

It does not interpret:

- HTTP;
- SMTP;
- IMAP;
- JSON;
- a business protocol;
- application messages.

Application framing remains the application's responsibility.

---

# 17. Shutdown and truncation

Closing TCP is not the same as shutting down TLS correctly.

TLS has the alert:

```text
close_notify
```

For graceful shutdown, PST expects reciprocal TLS closure.

```text
side A ── close_notify ──► side B
side A ◄─ close_notify ─── side B
```

Sending only your own `close_notify` does not mean reciprocal shutdown has completed.

If an established TLS connection receives EOF/reset without the peer's `close_notify`, PST classifies it as:

```text
TRUNCATED
```

including cases where authenticated data was delivered before the EOF.

That lets the application distinguish a clean TLS close from an abrupt transport close.

---

# 18. Peer Info

After a connection is established, the application can query normalized facts about the peer and session.

Depending on provider/capabilities, this may include:

- TLS version;
- cipher;
- certificate presence;
- chain;
- authentication state;
- hash/fingerprint;
- copied leaf certificate;
- negotiated ALPN.

These are TLS-layer facts.

Mapping them to application identity remains outside PST.

---

# 19. Diagnostics and logging

When something fails, knowing only:

```text
"TLS error"
```

is not enough.

PST provides normalized diagnostics and logging to help distinguish causes such as:

- missing certificate;
- invalid certificate;
- trust failure;
- Expected Peer Name mismatch;
- missing capability;
- provider ineligibility;
- handshake failure;
- truncation;
- transport error.

Logging must not expose sensitive key material, application payload, or private keys.

Release tests include explicit gates for those leaks.

---

# 20. Transport ownership

One important rule is knowing who closes the socket.

Before PST accepts transport ownership:

```text
failure
  │
  ▼
caller is still responsible for closing
```

After ownership is accepted:

```text
PST/provider
     │
     └── single close root
```

That prevents double-close bugs and lifecycle ambiguity.

On SERVER, this rule applies to the **connected socket returned by `accept`**, not to the listener. The listener remains owned by the application.

---

# 21. TLS after plaintext: STARTTLS and CONNECT

PST can receive the **same already-connected transport** after the application has used it for plaintext and reached a clean upgrade boundary.

```text
SMTP/IMAP-like: plaintext -> STARTTLS accepted -> attach PST -> TLS
proxy-like:     plaintext -> CONNECT accepted  -> attach PST -> TLS
```

The application remains responsible for SMTP, IMAP, HTTP, and boundary detection. PST does not reconnect and does not roll back to plaintext after accepting ownership. TLS bytes pre-read before attach are not supported in this version.

---

# 22. Public examples

The repository contains minimal public examples intended to demonstrate the public contract without coupling the application to private provider APIs.

Start with:

```text
examples\basic_client.c
examples\basic_server.c
```

Then explore examples covering:

- trust;
- mTLS;
- provider selection;
- logging;
- diagnostics.

The goal of these examples is to teach PST, not the full native API of NSS, Schannel, or OpenSSL.

---

# 23. Using an SDK instead of building the repository

Version 0.6.0 is distributed as target-specific SDKs.

An SDK contains, as applicable:

- public headers;
- `papinho_secure_transport.lib`;
- manifest;
- README/documentation;
- required link libraries;
- required runtime DLLs;
- notices/licenses;
- corresponding-source material.

Choose the SDK whose Target ID matches the artifact you want to integrate.

The official packages are:

```text
win32-x86-vc6-retrozilla-nss
win32-x64-msvc-19.51-schannel
win32-x64-msvc-19.51-openssl3
win32-x64-msvc-19.51-schannel-openssl3
```

The Combined target is optional.

---

# 24. Which path should I choose?

A practical way to think about it:

```text
Need to run in the validated NT4/x86 configuration?
        │
        └── win32-x86-vc6-retrozilla-nss

Windows x64 + want the validated OS-native TLS path?
        │
        └── win32-x64-msvc-19.51-schannel

Windows x64 + need TLS 1.3 / OpenSSL 3?
        │
        └── win32-x64-msvc-19.51-openssl3

Windows x64 + need Schannel/OpenSSL selection per connection?
        │
        └── win32-x64-msvc-19.51-schannel-openssl3
```

But remember:

> Target, provider, and tested operating system are different facts.

Always consult the [Target Matrix](../target-matrix.md).

---

# 25. Where to go next

After this guide, the most important documents are:

- [Full English introduction](README.md)
- [Target Matrix](../target-matrix.md)
- [API 2.0](../api-2.0.md)
- [SPI 3.0](../provider-spi-3.0.md)
- [API 1.3/SPI 2.4 → API 2.0/SPI 3.0 migration](../api-1.3-to-2.0-migration.md)
- [Security and limitations](../security-and-limitations.md)
- [Release packaging](../release-packaging.md)

If you are integrating PST into a real application, a useful reading order is:

```text
language README
      │
      ▼
getting-started
      │
      ▼
basic_client.c / basic_server.c
      │
      ▼
API 2.0
      │
      ▼
security-and-limitations
```

The underlying idea remains the same as it was at the beginning of the project:

> **the application states which secure-transport properties it needs; PST keeps provider-specific details behind one common boundary.**
