<!-- SPDX-License-Identifier: MPL-2.0 -->

# Future legacy SSL/TLS compatibility audit

Status: planning/audit only  
Target release: post-0.5.0  
Implementation in 0.5.0: no  
Public API/SPI change in 0.5.0: no

## Purpose

Record future compatibility work for legacy SSL/TLS without delaying PST 0.5.0.

The goal is not to re-enable weak protocols silently. The goal is to study whether PST can expose legacy SSL/TLS deliberately, explicitly, and safely enough for applications that must interoperate with old equipment, appliances, printers, embedded devices, industrial systems, management interfaces, and preserved software.

This track must remain isolated from the normal secure defaults.

## Core policy

Legacy SSL/TLS must never become an implicit downgrade path. Any legacy protocol must require explicit opt-in by the application.

```text
modern secure policy
    └── default

legacy protocol policy
    └── explicit opt-in only
```

No provider selection mode may silently fall back from a secure protocol set to an older SSL/TLS generation after binding or negotiation failure. `EXACT`, `ORDERED`, and `AUTOMATIC` remain provider-selection semantics, not protocol-downgrade semantics.

## Protocols to audit

Audit feasibility and provider support separately for:

- SSL 2.0;
- SSL 3.0;
- TLS 1.0;
- TLS 1.1.

Do not assume all will be implemented. Classify each independently as `SUPPORTED`, `IMPLEMENTABLE`, `PROVIDER_LIMITATION`, `OS_LIMITATION`, `TOOLCHAIN_LIMITATION`, `SECURITY_POLICY_BLOCKED`, or `NOT_WORTH_IMPLEMENTING`.

## Provider audit

### OpenSSL

Determine, by concrete OpenSSL release:

- which legacy protocols still exist in code;
- whether they are compiled by default;
- whether a legacy provider or configuration is needed;
- minimum security-level changes required;
- certificate, signature, and cipher compatibility;
- Windows version support for each OpenSSL generation;
- whether two OpenSSL generations may coexist as separate PST targets/providers if a future OpenSSL release drops an older supported Windows version.

A new OpenSSL major release does not automatically obsolete the previous PST target. PST may maintain multiple factual OpenSSL targets when required for incompatible OS, toolchain, or runtime support, for example:

```text
win32-x64-<toolchain>-openssl3
win32-x64-<toolchain>-openssl4
```

Exact names must follow the global factual target-ID ADR.

### Schannel

Audit by actual Windows version, not by current Windows behavior. At minimum investigate Windows NT 4.0, Windows 2000, Windows XP, Windows 7, Windows 10, and Windows 11.

Determine which SSL/TLS generations are exposed by the native security stack on each system and whether PST can use them through the available SSPI/Schannel APIs.

Special deferred item already recorded:

> Audit whether a Schannel-backed PST target is technically meaningful on Windows NT 4.0, instead of assuming current Schannel APIs/behavior represent historical Windows.

Do not call a historical Windows security package “Schannel-compatible” without factual proof.

### RetroZilla NSS / NSS family

Audit the exact retained snapshot and any maintainable NSS lineage for:

- SSL 3.0;
- TLS 1.0;
- TLS 1.1;
- ability to enable them explicitly;
- cipher and signature restrictions;
- compatibility with the Windows NT 4.0 / VC6 target;
- whether support exists but is disabled by policy;
- whether third-party source patching would be required.

Do not patch NSS/NSPR merely to claim protocol coverage without a real compatibility case.

## Application-facing policy model

The future public contract should distinguish clearly between protocol capability, protocol policy, provider capability, and provider selection.

A possible future shape to audit is:

```text
allowed_protocols:
    TLS_1_3
    TLS_1_2
    TLS_1_1
    TLS_1_0
    SSL_3_0
    SSL_2_0

legacy_protocol_opt_in:
    DISABLED by default
    REQUIRED for any protocol below the project's secure baseline
```

Exact API design is not frozen by this document. Do not overload existing provider-selection modes to represent this policy.

## Security rules

Any future legacy protocol support must satisfy all of these:

1. Off by default.
2. Explicit application opt-in.
3. No automatic downgrade from modern policy.
4. No post-binding provider fallback.
5. No silent expansion of the allowed protocol range.
6. Diagnostics identify when a legacy protocol was requested or negotiated.
7. Peer Info exposes the actually negotiated protocol.
8. Logging does not hide that a legacy protocol is active.
9. Documentation clearly warns about security limitations.
10. A legacy-capable build/provider does not weaken unrelated connections that did not opt in.

## Device and interoperability research

Build a factual compatibility corpus of real devices and software that still require old SSL/TLS. Candidate categories include network printers, MFP/scanners, switches/routers, UPS management cards, KVM/IPMI/BMC interfaces, industrial HMIs/PLCs, NAS appliances, old hypervisor management interfaces, embedded web servers, legacy mail servers, old database middleware, and preserved enterprise applications.

For each tested device record:

- vendor/model;
- firmware/version;
- protocol actually required;
- cipher/signature requirements;
- certificate behavior;
- whether TLS 1.2 or newer is available after firmware/configuration changes;
- whether a gateway/proxy would be safer than direct legacy support.

The existence of old hardware alone is not sufficient justification. Implementation priority must be driven by concrete interoperability cases.

## Certificates and PKI audit

Determine separately for each protocol/provider:

- whether new certificates can still be generated;
- which key types and signature algorithms are accepted;
- whether modern CA tooling can issue a technically compatible certificate;
- whether trust-chain limitations come from the protocol, provider, device, or firmware;
- whether old devices require obsolete hashes or key sizes;
- whether custom/private CA use is realistic.

An old TLS protocol does not necessarily require an old existing certificate. A new certificate may be technically possible while the real constraint is the endpoint's accepted signature algorithm, key type, chain format, or cipher suite. Do not weaken normal PST certificate policy globally to accommodate such endpoints.

## Direct legacy connection versus gateway

For every legacy interoperability case, compare:

```text
Application -> PST legacy protocol -> device
```

with:

```text
Application -> modern PST/TLS -> controlled gateway -> legacy protocol -> device
```

The gateway model may be preferable when the weak protocol must be isolated to a small LAN segment, multiple modern applications need the same old device, the legacy endpoint has severe certificate/cipher limitations, or policy requires keeping weak cryptography out of the main application process.

This connects naturally with the broader Papinho gateway/compatibility architecture, but no component choice is frozen here.

## Target naming

Do not create identities such as `legacy-openssl`, `old-schannel`, or `modern-openssl`.

Use factual target identifiers based on platform/ABI, architecture, concrete toolchain, and provider generation. OS version belongs in support/validation facts unless it truly defines an incompatible implementation target.

## Testing requirements

Any implemented legacy protocol must have:

- positive handshake;
- negative protocol-policy test;
- explicit opt-in test;
- default-deny test;
- no-downgrade test;
- no-post-binding-fallback test;
- read/write;
- clean shutdown where the protocol/provider permits;
- abrupt-close classification;
- Peer Info protocol reporting;
- diagnostic reporting;
- secret-safe logging;
- stress/reuse;
- cross-provider interoperability where meaningful;
- real target-OS/device validation.

A protocol must not be advertised solely because headers or provider symbols exist.

## Documentation policy

Legacy support, if implemented, must be described as compatibility functionality, not as a recommended security baseline. Prefer factual wording such as:

> SSL 3.0 is available only through explicit compatibility opt-in for endpoints that cannot negotiate a supported modern protocol.

Availability must never be presented as evidence of safety.

## Release strategy

Do not include this work in PST 0.5.0. The 0.5.0 line remains focused on API 2.0, SPI 3.0, CLIENT and SERVER roles, OpenSSL, Schannel, RetroZilla NSS, TLS 1.2/TLS 1.3, role-scoped capabilities, packaging/validation/publication, and Accelerator handoff.

After 0.5.0 publication, this audit may become a dedicated roadmap phase for a later release:

```text
LSSL-1  Historical provider/OS capability audit
LSSL-2  Device compatibility corpus
LSSL-3  Public policy/API design
LSSL-4  OpenSSL feasibility
LSSL-5  Schannel historical feasibility
LSSL-6  NSS feasibility
LSSL-7  Direct-vs-gateway security decision
LSSL-8  Selected protocol implementations
LSSL-9  Cross-provider/device validation
LSSL-10 Documentation / packaging / release
```

## Current decision

```text
LEGACY_SSL_TLS_AUDIT_RECORDED=YES
IMPLEMENT_IN_0_5_0=NO
BLOCK_0_5_0_RELEASE=NO
DEFAULT_LEGACY_PROTOCOLS=DISABLED
EXPLICIT_OPT_IN_REQUIRED=YES
AUTOMATIC_DOWNGRADE_ALLOWED=NO
POST_BINDING_FALLBACK_ALLOWED=NO
```
