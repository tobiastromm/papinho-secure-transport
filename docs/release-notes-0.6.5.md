<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport 0.6.5 candidate release notes

PapinhoSecureTransport 0.6.5 is a candidate patch release with additive public API 2.2.0 and unchanged provider SPI 3.0.

- Adds an immutable, bounded public transport-termination provenance snapshot.
- Normalizes native FIN, reset and socket-error evidence without exposing provider-native types.
- Preserves reciprocal TLS shutdown, truncation, provider binding, ownership and wait-set semantics.
- Retains separate VC6 RetroZilla NSS /ML and /MD SDK identities.
- Does not alter the published 0.6.4 release or its assets.

Publication remains blocked on the targeted external NT4, clean-machine and consumer recertification gates.
