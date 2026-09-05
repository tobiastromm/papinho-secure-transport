# Getting Started with PapinhoSecureTransport

> **This is the practical PapinhoSecureTransport guide.**
>
> If this is your first contact with the project, start with the [full
> English introduction](README.md), which explains the problem PST
> solves, its architecture, and the main concepts.
>
> [← English introduction](README.md) · [Repository
> README](../../README.md)

This guide takes you from preparing the environment to a first TLS
connection using only the public PapinhoSecureTransport API.

You will learn how to choose a target, build PST, understand the
generated files, integrate PST, register providers, establish TLS, use
system or custom trust, configure mTLS, select providers, and use
diagnostics/logging.

You do not need to know OpenSSL, Schannel, or NSS to begin. TLS concepts
are introduced when they become necessary.

------------------------------------------------------------------------

# 1. What you need

There is **no single build for every computer**. PST has different
targets because platforms, architectures, compilers, and security
implementations differ.

  -----------------------------------------------------------------------
  Build for...            Provider                Toolchain used by the
                                                  project
  ----------------------- ----------------------- -----------------------
  Windows NT 4.0 SP6 x86  RetroZilla NSS          Visual C++ 6 SP5 +
                                                  Processor Pack;
                                                  `cl.exe` 12.00.8804;
                                                  `link.exe` 6.00.8447

  Windows 10 build 19045  Schannel                Visual Studio Build
  x64                                             Tools 2026 18.9.2;
                                                  `cl.exe` 19.51.36256;
                                                  NMAKE 14.51.36256.0;
                                                  Windows SDK
                                                  10.0.26100.0

  Windows 10 build 19045  OpenSSL 3.5.8           same validated MSVC/SDK
  x64                                             toolchain

  Windows 10 build 19045  Schannel + OpenSSL      same validated MSVC/SDK
  x64                                             toolchain
  -----------------------------------------------------------------------

Provider dependencies such as RetroZilla NSS/NSPR and OpenSSL 3.5.8 have
prepared, versioned copies maintained by the project.

> **This guide uses the dependencies versioned and prepared by the
> project.** Rebuilding OpenSSL, RetroZilla NSS/NSPR, or other
> dependencies from their own source trees is not required to start
> using PST.

------------------------------------------------------------------------

# 2. Choose your target

Start with: **Which platform will my application run on, and which
security capabilities does it need?**

For Windows NT 4.0 SP6 x86, the validated path is RetroZilla NSS with
TLS 1.2 and TLS 1.3.

For Windows 10 build 19045 x64, PST validates Schannel for TLS 1.2 and
`SYSTEM_TRUST`, and OpenSSL 3.5.8 for TLS 1.2, TLS 1.3, and
`SYSTEM_TRUST`.

The Combined target contains both. In the validated environment:

``` text
TLS 1.2 + SYSTEM_TRUST → Schannel
TLS 1.3 + SYSTEM_TRUST → OpenSSL
```

Windows 11 and Windows Server remain pending formal project validation.

------------------------------------------------------------------------

# 3. Build PST

Run from the repository root.

## Windows NT 4.0 x86 --- RetroZilla NSS

``` bat
tools\build-vc6.bat clean
tools\build-vc6.bat test
```

Output: `build\vc6\papinho_secure_transport.lib`.

## Windows 10 x64 --- Schannel

``` bat
tools\build-modern-msvc.bat clean
tools\build-modern-msvc.bat test
```

Output: `build\win64-modern-msvc\papinho_secure_transport.lib`.

## Windows 10 x64 --- OpenSSL 3.5.8

``` bat
tools\build-modern-msvc-openssl.bat clean
tools\build-modern-msvc-openssl.bat test
```

Output: `build\win64-modern-msvc-openssl\papinho_secure_transport.lib`,
plus `libssl-3-x64.dll` and `libcrypto-3-x64.dll` for runtime.

## Windows 10 x64 --- Schannel + OpenSSL

``` bat
tools\build-modern-msvc-combined.bat clean
tools\build-modern-msvc-combined.bat combined-test
```

Output: `build\win64-modern-msvc-combined\papinho_secure_transport.lib`,
plus the OpenSSL runtime DLLs.

Running tests on the first build is a useful environment check. Some
integration tests require specific fixtures or network access and are
separate from the normal offline suite.

------------------------------------------------------------------------

# 4. What was generated?

Each target produces its own `papinho_secure_transport.lib`. The
libraries are **not interchangeable** because each belongs to a specific
target/provider set.

Public headers:

``` text
include\papinho_secure_transport.h
include\papinho_secure_transport_win32.h
```

``` c
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
```

Application code should not need headers from `src\`, `src\backends\`,
or `third_party\`.

  --------------------------------------------------------------------------------
  Target                  PST library                      Additional runtime
  ----------------------- -------------------------------- -----------------------
  RetroZilla NSS / x86    `papinho_secure_transport.lib`   NSS/NSPR DLLs

  Schannel / x64          `papinho_secure_transport.lib`   Windows components

  OpenSSL / x64           `papinho_secure_transport.lib`   `libssl-3-x64.dll`,
                                                           `libcrypto-3-x64.dll`

  Combined / x64          `papinho_secure_transport.lib`   OpenSSL DLLs + Windows
                                                           components
  --------------------------------------------------------------------------------

The prepared NSS/NSPR runtime is under
`third_party\retrozilla-nss\prebuilt\win32-x86-vc6\runtime\`.

------------------------------------------------------------------------

# 5. Integrate PST into your project

Your build needs the public headers, target `.lib`, and required runtime
DLLs. The final public SDK/package format has not yet been frozen.

A consumer may keep a small dependency record:

``` text
MyProject\
├── src\
├── third_party\
│   └── pst\
│       ├── README.md
│       └── version.txt
└── build\
```

Register the providers compiled into the target before creating a
runtime:

``` c
PST_RESULT result;
result = pst_win32_register_builtin_providers();
if (result != PST_RESULT_OK) { /* handle error */ }
```

Canonical provider IDs:

``` text
retrozilla-nss
schannel
openssl
```

------------------------------------------------------------------------

# 6. Your first connection

DNS, destination address, port, and TCP connection establishment remain
application responsibilities.

``` text
application → connected TCP socket → PST → TLS handshake → secure connection
```

Normal sequence:

``` text
register providers → create runtime → create trust → create config
→ set identity/hostname → set TLS policy → freeze config
→ create connection → wrap socket → attach transport
→ handshake → read/write → shutdown
```

Incremental operations can report:

``` text
PST_OPERATION_NEED_READ
PST_OPERATION_NEED_WRITE
PST_OPERATION_NEED_READ_WRITE
```

Bounded wait:

``` c
PST_WAIT_RESULT wait_result;
memset(&wait_result, 0, sizeof(wait_result));
result = pst_connection_wait(connection, 5000UL, &wait_result);
if (result != PST_RESULT_OK) { /* wait/transport error */ }
if (wait_result.timed_out) { /* application timeout */ }
```

A bounded handshake loop repeatedly calls `pst_connection_handshake()`,
stops on `PST_OPERATION_COMPLETE` or `PST_OPERATION_FAILED`, and uses
`pst_connection_wait()` between progress steps. Step and timeout limits
belong to application policy.

Wrap and attach a connected Win32 socket:

``` c
pst_transport *transport = NULL;
pst_u32 ownership_accepted = 0;

result = pst_win32_socket_transport_create((pst_size)native_socket, &transport);
result = pst_connection_attach(
    connection, transport, PST_OWNERSHIP_TRANSFERRED, &ownership_accepted);
```

If `ownership_accepted == 0`, the caller still owns the
transport/socket. If it becomes `1`, PST has accepted ownership.

------------------------------------------------------------------------

# 7. SYSTEM_TRUST example

`SYSTEM_TRUST` means using the operating system's trust policy.

``` c
PST_TRUST_SOURCE trust_source;
pst_trust *trust = NULL;
memset(&trust_source, 0, sizeof(trust_source));
trust_source.struct_size = sizeof(trust_source);
trust_source.api_version = PST_API_VERSION;
trust_source.kind = PST_TRUST_SOURCE_SYSTEM;
result = pst_trust_create(&trust_source, &trust);
```

Create a runtime requiring specific capabilities:

``` c
PST_RUNTIME_OPTIONS options;
pst_runtime *runtime = NULL;
memset(&options, 0, sizeof(options));
options.struct_size = sizeof(options);
options.api_version = PST_API_VERSION;
options.selection = PST_BACKEND_SELECTION_AUTOMATIC;
options.required_capabilities =
    PST_CAP_TLS_1_2 |
    PST_CAP_SYSTEM_TRUST |
    PST_CAP_HOSTNAME_VERIFY |
    PST_CAP_NONBLOCKING |
    PST_CAP_BACKEND_WAIT;
result = pst_runtime_create(&options, &runtime);
```

## Available `required_capabilities`

  Capability                  Meaning
  --------------------------- -----------------------------------
  `PST_CAP_TLS_1_2`           TLS 1.2
  `PST_CAP_TLS_1_3`           TLS 1.3
  `PST_CAP_CLIENT_AUTH`       client authentication / mTLS
  `PST_CAP_ALPN`              ALPN negotiation
  `PST_CAP_CUSTOM_TRUST`      application-supplied trust
  `PST_CAP_SYSTEM_TRUST`      operating-system trust policy
  `PST_CAP_HOSTNAME_VERIFY`   expected-hostname verification
  `PST_CAP_RESUMPTION`        session resumption
  `PST_CAP_EARLY_DATA`        early data / 0-RTT
  `PST_CAP_PEER_INFO`         normalized peer information
  `PST_CAP_NONBLOCKING`       incremental/nonblocking operation
  `PST_CAP_BACKEND_WAIT`      PST backend wait mechanism

`PST_CAP_RESUMPTION` and `PST_CAP_EARLY_DATA` are public contract bits
but are **not advertised by any current provider**.

Validated masks:

``` text
RetroZilla NSS  0x00000e5f
Schannel        0x00000e7d
OpenSSL         0x00000e7f
```

Trust and hostname answer different questions: is the chain trusted, and
does the certificate belong to the intended server?

``` c
PST_IDENTITY_CONFIG identity;
memset(&identity, 0, sizeof(identity));
identity.struct_size = sizeof(identity);
identity.api_version = PST_API_VERSION;
identity.credentials = NULL;
identity.trust = trust;
identity.expected_hostname = hostname;
identity.expected_hostname_size = strlen(hostname);
identity.require_peer_authentication = PST_REQUIREMENT_REQUIRED;
identity.require_client_authentication = PST_REQUIREMENT_DISABLED;
```

Configure TLS and freeze:

``` c
pst_config *config = NULL;
PST_TLS_POLICY policy;
result = pst_config_create(&config);
if (result == PST_RESULT_OK) result = pst_config_set_identity(config, &identity);

memset(&policy, 0, sizeof(policy));
policy.struct_size = sizeof(policy);
policy.api_version = PST_API_VERSION;
policy.minimum_version = PST_TLS_VERSION_1_2;
policy.maximum_version = PST_TLS_VERSION_1_2;
policy.alpn_requirement = PST_FEATURE_DISABLED;
policy.resumption = PST_FEATURE_DISABLED;
policy.early_data = PST_FEATURE_DISABLED;
policy.require_graceful_shutdown = PST_REQUIREMENT_DISABLED;

if (result == PST_RESULT_OK) result = pst_config_set_tls_policy(config, &policy);
if (result == PST_RESULT_OK) result = pst_config_freeze(config);
```

For TLS 1.3 only, set both min/max to `PST_TLS_VERSION_1_3`.

``` c
pst_connection *connection = NULL;
result = pst_connection_create(runtime, config, &connection);
```

------------------------------------------------------------------------

# 8. CUSTOM_TRUST example

Use `CUSTOM_TRUST` when the application explicitly supplies a CA, such
as a corporate CA.

``` c
PST_TRUST_SOURCE source;
pst_trust *trust = NULL;
memset(&source, 0, sizeof(source));
source.struct_size = sizeof(source);
source.api_version = PST_API_VERSION;
source.kind = PST_TRUST_SOURCE_CUSTOM_CA_DER;
source.data = ca_der;
source.data_size = ca_der_size;
result = pst_trust_create(&source, &trust);
```

DER is a standardized binary representation used for certificates. The
application must obtain those bytes from a trusted source.

``` c
options.required_capabilities =
    PST_CAP_TLS_1_3 |
    PST_CAP_CUSTOM_TRUST |
    PST_CAP_HOSTNAME_VERIFY |
    PST_CAP_NONBLOCKING |
    PST_CAP_BACKEND_WAIT;
```

`CUSTOM_TRUST` does not silently fall back to `SYSTEM_TRUST`.

------------------------------------------------------------------------

# 9. mTLS example

With **mutual TLS (mTLS)** the server also authenticates the client.

``` c
PST_CREDENTIAL_SOURCE source;
pst_credentials *credentials = NULL;
memset(&source, 0, sizeof(source));
source.struct_size = sizeof(source);
source.api_version = PST_API_VERSION;
source.kind = PST_CREDENTIAL_SOURCE_CERT_DER_PKCS8_DER;
source.certificate_der = certificate_der;
source.certificate_der_size = certificate_der_size;
source.private_key_der = private_key_der;
source.private_key_der_size = private_key_der_size;
result = pst_credentials_create(&source, &credentials);
```

Request `PST_CAP_CLIENT_AUTH`, set `identity.credentials = credentials`,
and require both peer and client authentication. mTLS does **not**
replace server authentication. Private keys are sensitive and must not
be published or logged.

------------------------------------------------------------------------

# 10. Provider selection

Public selection modes:

``` text
PST_BACKEND_SELECTION_AUTOMATIC
PST_BACKEND_SELECTION_EXACT
PST_BACKEND_SELECTION_ORDERED
```

## AUTOMATIC

``` c
options.selection = PST_BACKEND_SELECTION_AUTOMATIC;
options.required_capabilities = PST_CAP_TLS_1_3 | PST_CAP_SYSTEM_TRUST;
```

In the validated Combined target, provider order is:

``` text
1. schannel
2. openssl
```

For TLS 1.3 + SYSTEM_TRUST, Schannel is incompatible and OpenSSL is
selected.

## EXACT

Provider IDs are exactly:

``` text
"retrozilla-nss"
"schannel"
"openssl"
```

Require OpenSSL:

``` c
options.selection = PST_BACKEND_SELECTION_EXACT;
options.exact_backend_id = "openssl";
options.required_capabilities = PST_CAP_TLS_1_3 | PST_CAP_SYSTEM_TRUST;
```

Require Schannel:

``` c
options.selection = PST_BACKEND_SELECTION_EXACT;
options.exact_backend_id = "schannel";
```

Require RetroZilla NSS:

``` c
options.selection = PST_BACKEND_SELECTION_EXACT;
options.exact_backend_id = "retrozilla-nss";
```

**EXACT does not fall back.**

## ORDERED

``` c
static const char *preferred[] = { "openssl", "schannel" };

options.selection = PST_BACKEND_SELECTION_ORDERED;
options.preferred_backend_ids = preferred;
options.preferred_backend_count = sizeof(preferred) / sizeof(preferred[0]);
options.required_capabilities = PST_CAP_TLS_1_2 | PST_CAP_SYSTEM_TRUST;
```

Selection happens when the runtime is created. A later TLS,
authentication, or I/O failure does **not** transparently switch
providers.

Discover the selected provider:

``` c
PST_RUNTIME_INFO info;
memset(&info, 0, sizeof(info));
info.struct_size = sizeof(info);
info.api_version = PST_API_VERSION;

result = pst_runtime_get_info(runtime, &info);
if (result == PST_RESULT_OK)
    printf("Selected provider: %s\n", info.backend_id);
```

------------------------------------------------------------------------

# 11. Diagnostics and logging

Important normalized results include:

``` text
PST_RESULT_OK
PST_RESULT_INVALID_ARGUMENT
PST_RESULT_INVALID_STATE
PST_RESULT_UNSUPPORTED
PST_RESULT_UNAVAILABLE
PST_RESULT_OUT_OF_MEMORY
PST_RESULT_RESOURCE_FAILURE
PST_RESULT_TRANSPORT_FAILURE
PST_RESULT_PROTOCOL_FAILURE
PST_RESULT_AUTH_FAILURE
PST_RESULT_HOSTNAME_MISMATCH
PST_RESULT_POLICY_VIOLATION
PST_RESULT_BACKEND_FAILURE
PST_RESULT_TRUNCATED
PST_RESULT_CLOSED
PST_RESULT_INCOMPATIBLE_API
```

Use `pst_result_string(result)` for a textual description.

Copy normalized diagnostic information with:

``` c
PST_DIAGNOSTIC_INFO diagnostic;
result = pst_diagnostic_info_init(&diagnostic);
if (result == PST_RESULT_OK)
    result = pst_connection_copy_diagnostic(connection, &diagnostic);
```

Runtime/connection creation also has `_ex` variants that accept a
diagnostic output.

Logging levels are:

``` text
PST_LOG_LEVEL_OFF
PST_LOG_LEVEL_ERROR
PST_LOG_LEVEL_WARN
PST_LOG_LEVEL_INFO
PST_LOG_LEVEL_DEBUG
PST_LOG_LEVEL_TRACE
```

Example callback:

``` c
static void PST_CALL on_log(void *user_context, const PST_LOG_EVENT *event)
{
    (void)user_context;
    printf("level=%lu event=%lu result=%ld operation=%lu backend=%s\n",
        (unsigned long)event->level,
        (unsigned long)event->event_id,
        (long)event->normalized_result,
        (unsigned long)event->operation,
        event->backend_id);
}
```

Configure logging:

``` c
PST_LOG_CONFIG logging;
result = pst_log_config_init(&logging);
if (result == PST_RESULT_OK)
{
    logging.level = PST_LOG_LEVEL_INFO;
    logging.callback = on_log;
    logging.user_context = NULL;
}
```

Create a runtime with logging using `pst_runtime_create_with_logging()`.

The callback is synchronous and each `PST_LOG_EVENT` is ephemeral. Copy
any information you need to retain during the callback. Public logging
is designed not to automatically expose private keys, application
payloads, certificate contents, handles, pointers, or arbitrary native
provider errors.

------------------------------------------------------------------------

# 12. Next steps

The main flow is now:

``` text
choose target
→ build PST
→ integrate headers + .lib + runtime
→ register providers
→ define capabilities
→ configure trust + hostname
→ attach socket
→ handshake
→ read/write
→ shutdown
```

From there, the application's own protocol becomes the main concern
again. PST does not need to understand whether the protected bytes
represent HTTP, SMTP, IMAP, ERP messages, messaging, or a custom
protocol.

The repository's `examples\` directory contains public examples such as:

``` text
basic_client.c
system_trust.c
custom_trust.c
mtls.c
provider_selection.c
diagnostics_logging.c
```

They use only public PST headers.

If your integration scenario is not covered by this guide, opening an
issue is a good way to document the use case, ask questions, and
potentially turn the answer into useful community documentation.

------------------------------------------------------------------------

# Quick summary

``` text
I WANT TO USE PST
      │
      ▼
Choose platform
      │
      ▼
Choose target
      │
      ▼
Build
      │
      ▼
Integrate headers / .lib / runtime
      │
      ▼
Register providers
      │
      ▼
Configure security
      │
      ▼
Establish TLS
      │
      ▼
My application exchanges its own data
```

The goal of PST is to let the security-layer implementation evolve
without forcing the rest of the application to know NSS, Schannel, or
OpenSSL directly.
