<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport 0.6.5 candidate release notes

PapinhoSecureTransport 0.6.5 is an unpublished candidate with public API 3.0.0 and provider SPI 4.0.

- Adds an immutable, bounded public transport-termination provenance snapshot.
- Replaces legacy void connection release with failure-aware `pst_connection_try_release` and a fixed 28-byte `PST_RELEASE_INFO`.
- Reports `DESTROYED`, `PRECONDITION_RETAINED`, or terminal `CLOSE_UNRESOLVED_RETAINED`; there is no public retry or global quarantine limit.
- Normalizes native FIN, reset and socket-error evidence without exposing provider-native types.
- Preserves reciprocal TLS shutdown, truncation, provider binding, ownership and wait-set semantics.
- Retains separate VC6 RetroZilla NSS /ML and /MD SDK identities.
- Corrects the governed RetroZilla NSS reciprocal-shutdown path so queued
  `close_notify` ciphertext is drained incrementally before native SEND
  shutdown; real NT4 `/ML` and `/MD` qualification passed for the candidate.
- Does not alter the published 0.6.4 release or its assets.

Publication remains blocked on the targeted external NT4, clean-machine and consumer recertification gates.

## Compatibility and package audit

API 3 is an intentional breaking lifecycle revision: legacy `pst_connection_release(void)` is removed. SPI 4 requires `connection_try_close` before final infallible destruction. `PST_RELEASE_INFO` is 28 bytes; `PST_DIAGNOSTIC_INFO` remains 64 bytes and `PST_TRANSPORT_TERMINATION_INFO` remains 32 bytes. Published 0.6.4 artifacts and their API 2.1/SPI 3 contracts remain immutable.
