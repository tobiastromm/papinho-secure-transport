<!-- SPDX-License-Identifier: MPL-2.0 -->

# Provider SPI 3.0 contract

SPI version is `0x00030000`. This is an in-process provider contract, not a public application ABI or dynamic plugin ABI.

The core registry and all three providers use SPI 3.0 for CLIENT and their factual SERVER behavior. OpenSSL implements CLIENT/SERVER TLS 1.2 and TLS 1.3. Schannel implements a capability-limited TLS 1.2 SERVER; its SERVER mask excludes TLS 1.3 on the validated environment, `ALPN_SERVER`, and SERVER peer-name verification. RetroZilla NSS implements CLIENT/SERVER TLS 1.2 and TLS 1.3; its SERVER mask excludes SYSTEM_TRUST, `ALPN_SERVER`, and SERVER peer-name verification.

## Descriptor and capabilities

Descriptors retain stable ID, informational name, metadata, an aggregate static capability mask, CLIENT and SERVER capability masks, and the vtable. The aggregate mask is exactly the union of the two role masks and exists for discovery; it does not assert every role/capability cross-product. Selection uses only the mask for the frozen connection role. Capabilities use the public numeric values, including additive API 2.1 vocabulary bits `SNI_CONTROL` and `GRACEFUL_SHUTDOWN`; no SPI layout change is required. Runtime-effective query may remove static claims but never invent unsupported behavior, and the effective result is intersected with both role masks. Registration order is the deterministic AUTOMATIC order.

## Runtime and connection

Provider global/runtime initialization is lazy per PST runtime. `connection_create` receives one immutable `PST_BACKEND_CONNECTION_OPTIONS` containing size, SPI version, explicit role, calculated required capabilities and the frozen/caller-independent connection configuration. It creates and configures state transactionally. The SPI 2.4 split `connection_create` plus client-centric `connection_configure_identity` is removed.

```c
PST_RESULT (*connection_create)(
    void *runtime_state,
    const PST_BACKEND_CONNECTION_OPTIONS *options,
    void **out_connection_state);
```

The core rejects requirements absent from the candidate's role-specific mask before binding. The provider must still reject unsupported role/policy before publishing state. It may convert copied PST credentials/trust into native objects but retains no caller buffer and frees every native allocation in its own module. No provider-native type crosses the SPI.

## Remaining operations

`connection_destroy`, `attach_transport`, `handshake_step`, `get_interest`, `wait`, `read`, `write`, `shutdown_step`, peer snapshot creation/destruction, negotiated ALPN query and diagnostic copy remain role-neutral. Their SPI 2.4 boundedness, partial-I/O, clean/truncated close, output initialization and terminality rules remain in force.

The vtable also owns global initialize/shutdown, provider runtime create/destroy and effective-capability query/connection validation. State lifetime is provider-global -> runtime -> connection; partial failure unwinds in reverse order. Expensive native initialization remains lazy until an eligible provider is attempted.

Transport attach receives the provider-neutral native envelope and explicit ownership. Before acceptance the core owns it; after acceptance the provider owns the aggregate and is the only close root. Providers receive connected transports only and never implement listener policy.

## Validation

The mandatory vtable prefix ends through `shutdown_step`; capability-gated peer-info, ALPN and diagnostic hooks are size checked. ROLE_CLIENT/ROLE_SERVER, ALPN_CLIENT/ALPN_SERVER, trust, identity and peer-auth claims require corresponding behavior. No post-selection failure can ask the core to reselect. Unknown required capability bits fail.

SPI 2.4 providers require explicit compile-safe CLIENT adapters during migration. Such adapters advertise ROLE_CLIENT only and do not constitute SERVER implementation or evidence.
