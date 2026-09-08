<!-- SPDX-License-Identifier: MPL-2.0 -->

# Provider SPI 3.0 contract

SPI version is `0x00030000`. This is an in-process provider contract, not a public application ABI or dynamic plugin ABI.

SS-2 implementation baseline: the core registry and all three existing providers use SPI 3.0 for CLIENT behavior. SS-3 subsequently implemented and validated `ROLE_SERVER` only in OpenSSL. Schannel and RetroZilla NSS remain CLIENT-only and do not advertise `ROLE_SERVER`.

## Descriptor and capabilities

Descriptors retain stable ID, informational name, metadata, static capabilities and vtable. Capabilities use the API 2.0 role-aware numeric values. Runtime-effective query may remove static claims but never invent unsupported behavior. Registration order is the deterministic AUTOMATIC order.

## Runtime and connection

Provider global/runtime initialization is lazy per PST runtime. `connection_create` receives one immutable `PST_BACKEND_CONNECTION_OPTIONS` containing size, SPI version, explicit role, calculated required capabilities and the frozen/caller-independent connection configuration. It creates and configures state transactionally. The SPI 2.4 split `connection_create` plus client-centric `connection_configure_identity` is removed.

```c
PST_RESULT (*connection_create)(
    void *runtime_state,
    const PST_BACKEND_CONNECTION_OPTIONS *options,
    void **out_connection_state);
```

The provider must reject unsupported role/policy before publishing state. It may convert copied PST credentials/trust into native objects but retains no caller buffer and frees every native allocation in its own module. No provider-native type crosses the SPI.

## Remaining operations

`connection_destroy`, `attach_transport`, `handshake_step`, `get_interest`, `wait`, `read`, `write`, `shutdown_step`, peer snapshot creation/destruction, negotiated ALPN query and diagnostic copy remain role-neutral. Their SPI 2.4 boundedness, partial-I/O, clean/truncated close, output initialization and terminality rules remain in force.

Transport attach receives the provider-neutral native envelope and explicit ownership. Before acceptance the core owns it; after acceptance the provider owns the aggregate and is the only close root. Providers receive connected transports only and never implement listener policy.

## Validation

The mandatory vtable prefix ends through `shutdown_step`; capability-gated peer-info, ALPN and diagnostic hooks are size checked. ROLE_CLIENT/ROLE_SERVER, ALPN_CLIENT/ALPN_SERVER, trust, identity and peer-auth claims require corresponding behavior. No post-selection failure can ask the core to reselect. Unknown required capability bits fail.

SPI 2.4 providers require explicit compile-safe CLIENT adapters during migration. Such adapters advertise ROLE_CLIENT only and do not constitute SERVER implementation or evidence.
