<!-- SPDX-License-Identifier: MPL-2.0 -->

# Provider SPI 4.0 contract

SPI 4.0 is the provider contract used by the 0.6.5 / API 3.0 candidate. It is
intentionally incompatible with SPI 3.0: every provider vtable must expose
`connection_try_close` immediately before `connection_destroy`.

`connection_try_close` is the sole provider-side native-close authority. It
reports NO_NATIVE_RESOURCE, CLOSED_CONFIRMED, or
CLOSE_UNRESOLVED_RETAINED. Only the first two dispositions authorize the core
to call `connection_destroy`. An unresolved native close retains the provider
state, transport and public connection as a terminal, query-only object. The
core does not retry, does not transfer native-close responsibility to the
consumer, and does not select another provider after binding.

OpenSSL and Schannel report Winsock close failures in the Winsock native-error
domain. RetroZilla NSS reports `PR_Close` failures in the NSPR domain and
retains the factual descriptor resulting from the governed NSPR close path.
Attach failure after ownership acceptance follows the same close authority.

For the layered NSS/NSPR path, a failed `PR_Close` does not prove that the
lower native socket remains independently usable: the SSL layer may already
have transformed or consumed descriptor state. PST therefore retains the
provider-owned layered descriptor identity and exposes no public retry or
consumer native-close escape hatch. Synthetic boundary injection proves the
Model E state machine and ownership containment; it is not evidence that a
real Winsock or NSPR close failure occurred on a validated external system.

See [ADR-0008](adr/ADR-0008-lifecycle-failure-aware-de-fechamento-nativo.md)
and [Public API and ABI](public-api-abi.md).
