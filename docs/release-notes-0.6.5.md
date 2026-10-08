<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport 0.6.5 candidate release notes

PapinhoSecureTransport 0.6.5 is a candidate patch release with additive public API 2.2.0 and unchanged provider SPI 3.0.

- Adds an immutable, bounded public transport-termination provenance snapshot.
- Normalizes native FIN, reset and socket-error evidence without exposing provider-native types.
- Preserves reciprocal TLS shutdown, truncation, provider binding, ownership and wait-set semantics.
- Retains separate VC6 RetroZilla NSS /ML and /MD SDK identities.
- Does not alter the published 0.6.4 release or its assets.

Publication remains blocked on the targeted external NT4, clean-machine and consumer recertification gates.

## Compatibility and package audit

API 2.2 is additive, API 2.1 layouts remain compatible, PST_DIAGNOSTIC_INFO remains 64 bytes, and SPI 3.0 has no vtable change. PST_TRANSPORT_TERMINATION_INFO has the fixed 32-byte contract documented in the public guides. AUDIT-PACKAGE-001 found a stale printed 2.1/0.6.4 label even though the consumer validated numeric 2.2/0.6.5. Correcting that label is not evidence of an ABI break.
