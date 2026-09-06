# Security Policy

## Reporting a vulnerability

Do not report suspected security vulnerabilities through a public GitHub Issue. Use the repository's **Report a vulnerability** option on GitHub so the report can be handled privately.

When possible, include:

- the affected PapinhoSecureTransport (PST) version;
- the target and provider;
- the operating system and relevant environment details;
- reproduction steps, expected behavior, and observed behavior;
- the potential security impact;
- relevant logs or diagnostics, sanitized of secrets; and
- a proof of concept only when it is safe and necessary.

Do not include production credentials, private keys, access tokens, passwords, unnecessary personal data, or unrelated proprietary information. Sanitize diagnostics and logs before submitting them.

Please avoid publicly disclosing vulnerability details before reasonable coordination, especially while users may remain exposed. No fixed embargo or disclosure timeline is promised.

## Supported release and response

Security reports are evaluated based on the project state at the time of the report. Maintenance of any particular release is not guaranteed indefinitely.

Reports will be reviewed when reasonably possible. Reporting does not guarantee acceptance, a fix, a response or disclosure deadline, or continued maintenance. It does not create a support or maintenance contract, an SLA, entitlement to a fix, or entitlement to compensation.

The project does not currently operate a bug-bounty program unless one is explicitly announced in the future. No payment, reward, or recognition is promised.

## Other reports and component boundaries

Ordinary bugs, documentation issues, compatibility questions, and other non-security problems may be reported through normal GitHub Issues.

Issues involving NSS/NSPR, Schannel, OpenSSL, or another third-party component may ultimately need to be addressed upstream. When attribution is unclear, report a suspected PST integration or security issue privately here first so it can be classified with sufficient information.

For the project's technical security boundaries and diagnostic-disclosure model, see [Security and Limitations](docs/security-and-limitations.md) and [Security Disclosure](docs/security-disclosure.md). These documents provide technical context; they are not vulnerability-reporting channels.