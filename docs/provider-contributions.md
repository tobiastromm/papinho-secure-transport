<!-- SPDX-License-Identifier: MPL-2.0 -->

# Provider contribution checklist

Dynamic plugins are not supported; providers are deliberately compiled into target manifests. A proposal must state:

- engine, exact version, upstream, license, redistribution/notices, maintenance and provenance;
- supported platforms/toolchains and whether the dependency is vendored or external;
- truthful capabilities and TLS-version policy;
- CUSTOM/SYSTEM trust behavior, hostname verification, mTLS identity, ALPN and peer information;
- incremental readiness/wait mapping, partial I/O, clean close/truncation and bounded shutdown;
- transport ownership acceptance, lifecycle/singleton constraints, normalized errors and diagnostic redaction;
- deterministic tests, interoperability peers, real-platform evidence and target-separated build outputs.

Read the normative [SPI 2.4 contract](provider-spi.md). Provider-private native types remain outside public API and generic SPI vocabulary. A proposal that requires changing the frozen SPI must first justify a separately reviewed version impact.