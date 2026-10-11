<!-- SPDX-License-Identifier: MPL-2.0 -->

# 0.6.5 API 3 / SPI 4 documentation remediation

Status: PASS for the candidate source at the commit containing this record.
Published 0.6.4 remains immutable. Version 0.6.5 is an unpublished candidate
with library 0.6.5, public API 3.0.0 and provider SPI 4.0.

| Documentation surface | Earlier conflict | Current resolution | Distributed impact |
|---|---|---|---|
| root and EN/PT overview | API 2.1 wording could appear current | API 3 is current; API 2.1 scheduler/SNI material is explicitly inherited | source ZIP and SDK docs |
| EN/PT getting started | section 5 and wait-set text named API 2.1 as the current mental model | API 3 is primary; Model E release and historical origin are explicit | source ZIP and SDK docs |
| public API/ABI | historical API 2.1 sections preceded the current contract without an explicit boundary | historical headings and the normative API 3 section are explicit | source ZIP and SDK docs |
| providers/interoperability | current headings named SPI 3/M9 evidence | SPI 4 is current; M9 executions remain historical evidence for identified bytes | source ZIP and SDK docs |
| lifecycle/SPI 4/NSS | retained ownership and synthetic evidence limits were split across ADR and design records | no public retry/global quarantine and the NSS layered-close/synthetic-vs-real boundary are explicit | source ZIP and SDK docs |
| target/package/examples | older release text could be read without chronology | historical headings and current 0.6.5 candidate identity are explicit | source ZIP and SDK docs |

The breaking contract is deliberate: API 3 removes the void connection
release and requires `pst_connection_try_release` plus the fixed 28-byte
`PST_RELEASE_INFO`. Only `DESTROYED` permits the consumer to discard the
reference. `PRECONDITION_RETAINED` and `CLOSE_UNRESOLVED_RETAINED` preserve it.
SPI 4 requires provider `connection_try_close` before final destruction.

No document claims a real native-close failure execution. Synthetic failure
injection proves state-machine and ownership behavior only. External NT4,
clean-machine, Browser and Accelerator certification for the candidate bytes
remains not performed until the generated bundles are executed and audited.
