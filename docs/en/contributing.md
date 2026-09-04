# Contributing

Start with a focused issue: platform, target, provider, exact command, expected result, actual result, logs with secrets removed, and whether hardware/OS is real. Keep changes narrow and preserve unrelated work.

All affected targets must build warning-free: VC6 C89 `/W4` through `tools\build-vc6.bat`, or modern x64 `/MD /W4` through the documented scripts. Keep output directories separate. Security-sensitive changes need deterministic negative tests and real provider validation proportional to risk. Do not weaken trust, hostname, TLS policy, ownership, terminality, readiness, or diagnostic redaction.

API 1.3.0 and SPI 2.4 are frozen baselines. Propose additions explicitly; never change layouts, values, signatures, or semantics casually. Legacy code must remain C89/VC6-safe and must be tested on real old systems when claims depend on them.

Provider/dependency contributions require license, redistribution, notice, source/modification-obligation, provenance, maintenance, and external-versus-vendored review. Technical compatibility is not permission to redistribute and this project does not provide legal advice. See the [provider proposal checklist](../provider-contributions.md).

Research and reproducibility help are welcome, especially around NSS/NSPR, VC6 and legacy Win32, and maintained, updated, or new lineages capable of modern TLS—including TLS 1.3 where feasible. RetroZilla NSS is valuable preserved evidence; this invitation does not promise a PST-owned fork. Documentation, translations, real-machine testing, adapters, integrations, provenance, and license review are equally useful; contributors need not be cryptographers.
