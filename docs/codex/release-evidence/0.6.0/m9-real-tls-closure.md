<!-- SPDX-License-Identifier: MPL-2.0 -->

# M9 real TLS closure evidence

Validated on 2026-09-10 on Windows x64. Generated process logs are retained
under `build/m9-real-tls` and are not release inputs.

## Cross-provider matrix

All nine CLIENT x SERVER pairs passed TLS 1.2 with CUSTOM_TRUST, verification
of `localhost`, encrypted WRITE=25 / READ=25 / CONTENT_MATCH=1, and reciprocal
TLS shutdown:

- OpenSSL -> OpenSSL, Schannel, RetroZilla NSS
- Schannel -> OpenSSL, Schannel, RetroZilla NSS
- RetroZilla NSS -> OpenSSL, Schannel, RetroZilla NSS

All four TLS 1.3 pairs eligible under the frozen capability model passed:

- OpenSSL -> OpenSSL and RetroZilla NSS
- RetroZilla NSS -> OpenSSL and RetroZilla NSS

The five combinations containing Schannel are ineligible for TLS 1.3 before
binding and are therefore UNSUPPORTED / NOT_APPLICABLE. No post-binding
provider fallback was used.

## Trust, authentication, closure, and diagnostics

- Required mTLS passed against OpenSSL, Schannel, and RetroZilla NSS servers
  on TLS 1.2, and against the TLS 1.3-capable OpenSSL and NSS servers.
- Schannel SYSTEM_TRUST TLS 1.2 passed with required client authentication;
  the fixture trust anchor was removed from CurrentUser/Root afterward.
- OpenSSL SYSTEM_TRUST passed online against two independent public endpoints
  on TLS 1.2 and TLS 1.3.
- Untrusted custom roots produced normalized AUTH_FAILURE diagnostics and
  terminal no-resurrection for OpenSSL, Schannel, and RetroZilla NSS.
- OpenSSL, Schannel, and NSS classified abrupt EOF as TRUNCATED. Runs with data
  before abrupt EOF preserved and validated the application data first.
- A scan of 88 real-process client/provider log files found zero private-key or
  application-payload disclosures.

## Defects found and corrected

The real matrix exposed two Schannel shutdown state-machine defects. The
provider now completes after draining its reciprocal close_notify when the
peer alert was already observed, and processes buffered encrypted input before
performing another socket receive during shutdown. Raw EOF without a peer
close_notify remains TRUNCATED.

The Schannel negative integration harness also now retains the original
handshake failure before executing its terminal no-resurrection probes.

Affected Schannel and Combined `/W4` suites passed with zero warnings after the
corrections. Previously accepted deterministic/failure-injection evidence and
the 250-cycle scheduler stress were not repeated because neither correction
changes the scheduler core or its deterministic models.

## Closure markers

```text
CROSS_PROVIDER_TLS12=PASS
CROSS_PROVIDER_TLS13=PASS_FOR_ELIGIBLE_PAIRS
TRUST_AUTH_NEGATIVE_MATRIX=PASS
CLEAN_CLOSE=PASS
RAW_EOF_TRUNCATED=PASS
DATA_THEN_EOF_DATA_PRESERVED=PASS
DIAGNOSTIC_CROSS_PROVIDER_MATRIX=PASS
LOG_SECRET_HITS=0
PRIVATE_KEY_LOG_HITS=0
APPLICATION_PAYLOAD_LOG_HITS=0
NATIVE_ERROR_REQUIRED_FOR_PUBLIC_CONTROL=NO
NO_POST_BINDING_FALLBACK=PASS
POST_BIND_PROVIDER_SWITCH_COUNT=0
NSS_PR_POLL_AUTHORITY_PRESERVED=PASS
MIXED_STRESS=PASS
MIXED_STRESS_CYCLES=250
```
